//----------------------------------------------------
//--- GAMEGURU - M-DAINew
//----------------------------------------------------

#include "stdafx.h"
#include "gameguru.h"

#include "GGRecastDetour.h"
extern GGRecastDetour g_RecastDetour;

#ifdef OPTICK_ENABLE
#include "optick.h"
#endif

#include "tracers/TracerManager.h"
using namespace Tracers;

// Globals 
bool g_bDormantCheckForThisCycle = true;
int i_LastExplosionSoundID = 0;

void darkai_init ( void )
{
	// Inits
	t.aisystem.on=1;
	t.aisystem.processlogic = 1;
	t.aisystem.processplayerlogic = 1;

	// create debug objects to visualize AI
	#ifdef NEWMAXAISYSTEM
	darkai_createinternaldebugvisuals();
	#endif
	
	// reset smoothanim array so no carryover of transitions to new test game
	darkai_resetsmoothanims();
}

void darkai_free ( void )
{
	// free A.I resources
	t.aisystem.on=0;

	// free debug objects
	#ifdef NEWMAXAISYSTEM
	darkai_destroyinternaldebugvisuals();
	#endif
}

void darkai_createinternaldebugvisuals_coneofsight (int iOuterViewObject, float fOuterViewArc, float fViewRange)
{
	extern bool g_bShowRecastDetourDebugVisuals;
	if (g_bShowRecastDetourDebugVisuals == true)
	{
		// if no cone, assume character can see in all directions!
		if (fOuterViewArc == 0.0f) fOuterViewArc = 179.0f;

		// if existing cone of sight object, delete it
		if (ObjectExist(iOuterViewObject) == 1) DeleteObject(iOuterViewObject);

		// create a fan outline using Zak's prisms
		WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_CURSOROBJECT);
		WickedCall_PresetObjectPutInEmissive(1);

		// Find a free memblock
		int iMemblockToMakeConeOfSight = 0;
		for (int i = 1; i <= 257; i++) if (MemblockExist(i) == 0) { iMemblockToMakeConeOfSight = i; break; }
		if (iMemblockToMakeConeOfSight == 0) return;

		// fan outline shape
		int iSegmentCount = 36;
		int iLineCount = 2 + iSegmentCount;
		float fOuterViewArcRad = GGToRadian(fOuterViewArc);
		float fAngleStart = (fOuterViewArc / -2.0f);
		float fAngleStartRad = GGToRadian(fAngleStart);
		float fSegAngle = (fOuterViewArc / iSegmentCount);
		float fSegAngleRad = GGToRadian(fSegAngle);

		// create object memblock
		int vertsize = 32;
		int iSizeBytes = 0;
		int vertexCount = 36 * iLineCount;
		iSizeBytes = vertsize * vertexCount;
		iSizeBytes += 12;
		MakeMemblock(iMemblockToMakeConeOfSight, iSizeBytes);
		WriteMemblockDWord(iMemblockToMakeConeOfSight, 0, GGFVF_XYZ | GGFVF_NORMAL | GGFVF_TEX1);
		WriteMemblockDWord(iMemblockToMakeConeOfSight, 4, 32);
		WriteMemblockDWord(iMemblockToMakeConeOfSight, 8, vertexCount);

		// all lines needed
		int v = 0;
		for (int iLineIndex = 0; iLineIndex < iLineCount; iLineIndex++)
		{
			// create a set of prism lines to create the outline of a cone fan
			float fromx = 0, fromy = 0, fromz = 0;
			float tox = 0, toy = 0, toz = 0;
			if (iLineIndex == 0)
			{
				// left side
				tox = sin(fAngleStartRad) * fViewRange;
				toz = cos(fAngleStartRad) * fViewRange;
			}
			else if (iLineIndex == 1)
			{
				// right side
				tox = sin(fAngleStartRad + fOuterViewArcRad) * fViewRange;
				toz = cos(fAngleStartRad + fOuterViewArcRad) * fViewRange;
			}
			else
			{
				// line at end of fan
				fromx = sin(fAngleStartRad + ((iLineIndex - 2) * fSegAngleRad)) * fViewRange;
				fromz = cos(fAngleStartRad + ((iLineIndex - 2) * fSegAngleRad)) * fViewRange;
				tox = sin(fAngleStartRad + ((iLineIndex - 1) * fSegAngleRad)) * fViewRange;
				toz = cos(fAngleStartRad + ((iLineIndex - 1) * fSegAngleRad)) * fViewRange;
			}

			// line coordinates
			float p0[3];
			float p1[3];
			p0[0] = fromx; p0[1] = fromy; p0[2] = fromz;
			p1[0] = tox; p1[1] = toy; p1[2] = toz;

			// create points
			float points[18];
			physics_debug_make_prism_between_points(p0, p1, points, 0.25f);

			// Corners of the prism
			float x0, x1, x2, x3, x4, x5;
			float y0, y1, y2, y3, y4, y5;
			float z0, z1, z2, z3, z4, z5;
			x0 = points[0]; y0 = points[1]; z0 = points[2];
			x1 = points[3]; y1 = points[4]; z1 = points[5];
			x2 = points[6]; y2 = points[7]; z2 = points[8];
			x3 = points[9]; y3 = points[10]; z3 = points[11];
			x4 = points[12]; y4 = points[13]; z4 = points[14];
			x5 = points[15]; y5 = points[16]; z5 = points[17];

			// Midpoints
			float mx03, mx14, mx25;
			float my03, my14, my25;
			float mz03, mz14, mz25;
			mx03 = (x0 + x3) / 2.0f; my03 = (y0 + y3) / 2.0f; mz03 = (z0 + z3) / 2.0f;
			mx14 = (x1 + x4) / 2.0f; my14 = (y1 + y4) / 2.0f; mz14 = (z1 + z4) / 2.0f;
			mx25 = (x2 + x5) / 2.0f; my25 = (y2 + y5) / 2.0f; mz25 = (z2 + z5) / 2.0f;

			// setup UV for the prism colors
			float fRelationUVs[24];
			const float off = 0.16666667f;
			fRelationUVs[0] = 0; fRelationUVs[1] = 0;
			fRelationUVs[2] = 0; fRelationUVs[3] = 1;
			fRelationUVs[4] = 1; fRelationUVs[5] = 0;
			fRelationUVs[6] = 1; fRelationUVs[7] = 0;
			fRelationUVs[8] = 0; fRelationUVs[9] = 1;
			fRelationUVs[10] = 1; fRelationUVs[11] = 1;
			fRelationUVs[12] = 0 + off; fRelationUVs[13] = 0 + off;
			fRelationUVs[14] = 0 + off; fRelationUVs[15] = 1 + off;
			fRelationUVs[16] = 1 + off; fRelationUVs[17] = 0 + off;
			fRelationUVs[18] = 1 + off; fRelationUVs[19] = 0 + off;
			fRelationUVs[20] = 0 + off; fRelationUVs[21] = 1 + off;
			fRelationUVs[22] = 1 + off; fRelationUVs[23] = 1 + off;

			// Bottom, right and left face
			int iFound = iMemblockToMakeConeOfSight;
			extern void AddVertToObjectRelation(float x, float y, float z, float texU, float texV, int v, int memblock);
			AddVertToObjectRelation(x0, y0, z0, fRelationUVs[0], fRelationUVs[1], v++, iFound);
			AddVertToObjectRelation(x2, y2, z2, fRelationUVs[2], fRelationUVs[3], v++, iFound);
			AddVertToObjectRelation(mx03, my03, mz03, fRelationUVs[4], fRelationUVs[5], v++, iFound);
			AddVertToObjectRelation(mx03, my03, mz03, fRelationUVs[6], fRelationUVs[7], v++, iFound);
			AddVertToObjectRelation(x2, y2, z2, fRelationUVs[8], fRelationUVs[9], v++, iFound);
			AddVertToObjectRelation(mx25, my25, mz25, fRelationUVs[10], fRelationUVs[11], v++, iFound);
			AddVertToObjectRelation(mx03, my03, mz03, fRelationUVs[0], fRelationUVs[1], v++, iFound);
			AddVertToObjectRelation(mx25, my25, mz25, fRelationUVs[2], fRelationUVs[3], v++, iFound);
			AddVertToObjectRelation(x3, y3, z3, fRelationUVs[4], fRelationUVs[5], v++, iFound);
			AddVertToObjectRelation(x3, y3, z3, fRelationUVs[6], fRelationUVs[7], v++, iFound);
			AddVertToObjectRelation(mx25, my25, mz25, fRelationUVs[8], fRelationUVs[9], v++, iFound);
			AddVertToObjectRelation(x5, y5, z5, fRelationUVs[10], fRelationUVs[11], v++, iFound);
			AddVertToObjectRelation(x1, y1, z1, fRelationUVs[0], fRelationUVs[1], v++, iFound);
			AddVertToObjectRelation(x0, y0, z0, fRelationUVs[2], fRelationUVs[3], v++, iFound);
			AddVertToObjectRelation(mx14, my14, mz14, fRelationUVs[4], fRelationUVs[5], v++, iFound);
			AddVertToObjectRelation(mx14, my14, mz14, fRelationUVs[6], fRelationUVs[7], v++, iFound);
			AddVertToObjectRelation(x0, y0, z0, fRelationUVs[8], fRelationUVs[9], v++, iFound);
			AddVertToObjectRelation(mx03, my03, mz03, fRelationUVs[10], fRelationUVs[11], v++, iFound);
			AddVertToObjectRelation(mx14, my14, mz14, fRelationUVs[0], fRelationUVs[1], v++, iFound);
			AddVertToObjectRelation(mx03, my03, mz03, fRelationUVs[2], fRelationUVs[3], v++, iFound);
			AddVertToObjectRelation(x4, y4, z4, fRelationUVs[4], fRelationUVs[5], v++, iFound);
			AddVertToObjectRelation(x4, y4, z4, fRelationUVs[6], fRelationUVs[7], v++, iFound);
			AddVertToObjectRelation(mx03, my03, mz03, fRelationUVs[8], fRelationUVs[9], v++, iFound);
			AddVertToObjectRelation(x3, y3, z3, fRelationUVs[10], fRelationUVs[11], v++, iFound);
			AddVertToObjectRelation(x2, y2, z2, fRelationUVs[0], fRelationUVs[1], v++, iFound);
			AddVertToObjectRelation(x1, y1, z1, fRelationUVs[2], fRelationUVs[3], v++, iFound);
			AddVertToObjectRelation(mx25, my25, mz25, fRelationUVs[4], fRelationUVs[5], v++, iFound);
			AddVertToObjectRelation(mx25, my25, mz25, fRelationUVs[6], fRelationUVs[7], v++, iFound);
			AddVertToObjectRelation(x1, y1, z1, fRelationUVs[8], fRelationUVs[9], v++, iFound);
			AddVertToObjectRelation(mx14, my14, mz14, fRelationUVs[10], fRelationUVs[11], v++, iFound);
			AddVertToObjectRelation(mx25, my25, mz25, fRelationUVs[0], fRelationUVs[1], v++, iFound);
			AddVertToObjectRelation(mx14, my14, mz14, fRelationUVs[2], fRelationUVs[3], v++, iFound);
			AddVertToObjectRelation(x5, y5, z5, fRelationUVs[4], fRelationUVs[5], v++, iFound);
			AddVertToObjectRelation(x5, y5, z5, fRelationUVs[6], fRelationUVs[7], v++, iFound);
			AddVertToObjectRelation(mx14, my14, mz14, fRelationUVs[8], fRelationUVs[9], v++, iFound);
			AddVertToObjectRelation(x4, y4, z4, fRelationUVs[10], fRelationUVs[11], v++, iFound);
		}

		// create the object
		int iMeshID = g.meshgeneralwork;
		if (GetMeshExist(iMeshID) == 1) DeleteMesh(iMeshID);
		CreateMeshFromMemblock(iMeshID, iMemblockToMakeConeOfSight);
		MakeObject(iOuterViewObject, iMeshID, 0);
		SetObjectCull(iOuterViewObject, 0);
		ShowObject(iOuterViewObject);
		SetObjectMask (iOuterViewObject, 1);
		DisableObjectZDepth (iOuterViewObject);
		SetObjectCollisionOff (iOuterViewObject);

		// provide with a texture
		if (ImageExist(g.coneofsightimageoffset) == 0) LoadImage("editors\\uiv3\\nodeconeofsight.png", g.coneofsightimageoffset);
		TextureObject(iOuterViewObject, g.coneofsightimageoffset);

		// wicked settings
		sObject* pObject = GetObjectData(iOuterViewObject);
		WickedCall_SetObjectCastShadows(pObject, false);
		WickedCall_SetObjectLightToUnlit(pObject, (int)wiScene::MaterialComponent::SHADERTYPE::SHADERTYPE_UNLIT);

		// finished creating object
		WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_NORMAL);
		WickedCall_PresetObjectPutInEmissive(0);

		// free resources used
		DeleteMemblock(iMemblockToMakeConeOfSight);
		if (GetMeshExist(iMeshID) == 1) DeleteMesh(iMeshID);
	}
}

void darkai_createinternaldebugvisuals (void)
{
	// created in 'darkai_setupcharacter' initially
}

void darkai_destroyinternaldebugvisuals (void)
{
	// Free all debug objects
	#ifdef NEWMAXAISYSTEM
	if (ObjectExist(g.debugraycastvisual) == 1) DeleteObject(g.debugraycastvisual);
	for (int ichardebugobj = g.debugconeofsightstart; ichardebugobj < g.debugconeofsightfinish; ichardebugobj++)
		if (ObjectExist(ichardebugobj) == 1)
			DeleteObject(ichardebugobj);
	#endif
}

void darkai_updatedebugobjects_forcharacter (bool bCharIsActive)
{
	// never in game executable
	if (t.game.gameisexe == 0)
	{
		// Control ONE raycaster line
		extern bool g_bShowRecastDetourDebugVisuals;
		if (ObjectExist(g.debugraycastvisual) == 0)
		{
			WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_CURSOROBJECT);
			WickedCall_PresetObjectPutInEmissive(1);
			MakeObjectBox(g.debugraycastvisual, 1, 1, 100);
			PositionObject(g.debugraycastvisual, -999999, -999999, -999999);
			if (ImageExist(g.coneofsightimageoffset) == 0) LoadImage("editors\\uiv3\\nodeconeofsight.png", g.coneofsightimageoffset);
			TextureObject(g.debugraycastvisual, g.coneofsightimageoffset);
			sObject* pObject = GetObjectData(g.debugraycastvisual);
			WickedCall_SetObjectLightToUnlit(pObject, (int)wiScene::MaterialComponent::SHADERTYPE::SHADERTYPE_UNLIT);
			WickedCall_SetObjectCastShadows(pObject, false);
			WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_NORMAL);
			WickedCall_PresetObjectPutInEmissive(0);
		}
		else
		{
			if (g_bShowRecastDetourDebugVisuals == true)
				ShowObject(g.debugraycastvisual);
			else
				HideObject(g.debugraycastvisual);
		}

		// get object facing angle, also apply any head turning activity
		float fAIObjX = ObjectPositionX(t.charanimstate.obj);
		float fAIObjY = ObjectPositionY(t.charanimstate.obj) + 5.0f;
		float fAIObjZ = ObjectPositionZ(t.charanimstate.obj);
		bool bWithinDebugViewRange = false;
		float fAIObjAngleY = ObjectAngleY(t.charanimstate.obj);
		fAIObjAngleY += t.charanimstate.neckRightAndLeft;
		float fDX = fAIObjX - CameraPositionX(0);
		float fDZ = fAIObjZ - CameraPositionZ(0);
		float fDist = sqrt(fabs(fDX * fDX) + fabs(fDZ * fDZ));
		float fViewRange = t.entityelement[t.charanimstate.e].eleprof.conerange;
		if (fDist < fViewRange * 1.25f) bWithinDebugViewRange = true;

		// Control cone of sight object
		int iOuterViewObject = g.debugconeofsightstart + g.charanimindex;
		if (ObjectExist(iOuterViewObject) == 1)
		{
			// position and show/hide
			PositionObject (iOuterViewObject, fAIObjX, fAIObjY, fAIObjZ);
			YRotateObject (iOuterViewObject, fAIObjAngleY);
			if (bCharIsActive == true && g_bShowRecastDetourDebugVisuals == true && bWithinDebugViewRange == true)
			{
				// cone of sight debug
				ShowObject(iOuterViewObject);

				// if using an attachment, and it has a firespot debug object, show it
				if (t.entityelement[t.charanimstate.e].attachmentobj > 0)
				{
					int tentityattachmentindex = t.entityelement[t.charanimstate.e].attachmentobj - g.entityattachmentsoffset;
					int iDebugFirespotObj = g.entityattachments2offset + tentityattachmentindex;
					if (ObjectExist(iDebugFirespotObj) == 1)
					{
						ShowObject(iDebugFirespotObj);
						sObject* pDebugFirespotObj = GetObjectData(iDebugFirespotObj);
						WickedCall_SetObjectRenderLayer(pDebugFirespotObj, GGRENDERLAYERS_NORMAL);
					}
				}
			}
			else
			{
				// hide the view
				HideObject(iOuterViewObject);

				// if using an attachment, and it has a firespot debug object, hide it
				if (t.entityelement[t.charanimstate.e].attachmentobj > 0)
				{
					int tentityattachmentindex = t.entityelement[t.charanimstate.e].attachmentobj - g.entityattachmentsoffset;
					int iDebugFirespotObj = g.entityattachments2offset + tentityattachmentindex;
					if (ObjectExist(iDebugFirespotObj) == 1)
					{
						HideObject(iDebugFirespotObj);
						sObject* pDebugFirespotObj = GetObjectData(iDebugFirespotObj);
						WickedCall_SetObjectRenderLayer(pDebugFirespotObj, GGRENDERLAYERS_CURSOROBJECT);
					}
				}
			}
		}

		// always get to see ID so can do LUA logic work
		if (g_bShowRecastDetourDebugVisuals == true)
		{
			// also show object entity ID (so can debug logic in behavior editor)
			if (fDist < 500)
			{
				int e = t.charanimstate.e;
				if (e > 0)
				{
					char pShowNavigationDebugVisualstext[256];
					sprintf(pShowNavigationDebugVisualstext, "%d", t.charanimstate.e);
					int entid = t.entityelement[e].bankindex;
					if (entid > 0)
					{
						if (t.entityprofile[entid].ischaracter != 0)
						{
							sprintf(pShowNavigationDebugVisualstext, "%d (health=%d)", t.charanimstate.e, t.entityelement[t.charanimstate.e].health);
						}
					}
					t.entityelement[t.charanimstate.e].overprompt_s = pShowNavigationDebugVisualstext;
					t.entityelement[t.charanimstate.e].overprompttimer = MAXTimer() + 250;
				}
			}
		}
	}
}

//PE: Now thread safe, not using any globals.
void darkai_calcplrvisible (charanimstatetype &cas)
{
	// takes tcharanimindex - work out if entity A.I can see (stored until recalculated)
	t.entityelement[cas.e].plrvisible = 0;
	t.entityelement[cas.e].lua.flagschanged = 1;
	if (t.player[t.plrid].health > 0)
	{
		// work out distance between player and entity
		float tttdx_f = ObjectPositionX(t.aisystem.objectstartindex) - ObjectPositionX(cas.obj);
		float tttdz_f = ObjectPositionZ(t.aisystem.objectstartindex) - ObjectPositionZ(cas.obj);
		float tttdd_f = Sqrt(abs(tttdx_f*tttdx_f) + abs(tttdz_f*tttdz_f));
		float fDistanceRangeToCheck = MAXFREEZEDISTANCE;// t.maximumnonefreezedistance;
		if (t.entityelement[cas.e].eleprof.conerange > 0) fDistanceRangeToCheck = t.entityelement[cas.e].eleprof.conerange;
		if (tttdd_f < fDistanceRangeToCheck)
		{
			// get object facing angle, also apply any head turning activity
			float fAIObjAngleY = ObjectAngleY(cas.obj);
			fAIObjAngleY += cas.neckRightAndLeft;

			// player within units, otherwise skip further vis checking
			float tttda_f = atan2deg(tttdx_f, tttdz_f);
			float tttdiff_f = WrapValue(tttda_f) - WrapValue(fAIObjAngleY);
			if (tttdiff_f < -180) tttdiff_f = tttdiff_f + 360;
			if (tttdiff_f > 180) tttdiff_f = tttdiff_f - 360;
			int ttconeangle = t.entityelement[cas.e].eleprof.coneangle;
			if (ttconeangle == 0) ttconeangle = 179;
			if (abs(tttdiff_f) <= ttconeangle)
			{
				// assume visible unless blocked (below)
				int tttokay = 1;
				float tbrayx1_f, tbrayy1_f, tbrayz1_f;
				// match ray cast with masterinterpreter raycasting (baseY+65)
				bool bFoundEyeball = false;
				int headlimbofcharacter = t.entityprofile[t.entityelement[cas.e].bankindex].headlimb;
				if (headlimbofcharacter > 0)
				{
					if (LimbExist(cas.obj, headlimbofcharacter) == 1)
					{
						sObject* pObject = GetObjectData(cas.obj);
						WickedCall_GetLimbData(pObject, headlimbofcharacter, &tbrayx1_f, &tbrayy1_f, &tbrayz1_f, 0, 0, 0, 0);
						bFoundEyeball = true;
					}
				}
				if (bFoundEyeball == false)
				{
					tbrayx1_f = ObjectPositionX(cas.obj);
					tbrayy1_f = ObjectPositionY(cas.obj) + 60;// need them to SEE from the head +35;// 65 - this fixes characters shooting from the eyeballs!
					tbrayz1_f = ObjectPositionZ(cas.obj);
				}

				// location of player (if player camera can see enemy, vice versa)
				float tbrayx2_f = CameraPositionX(t.terrain.gameplaycamera);
				float tbrayy2_f = CameraPositionY(t.terrain.gameplaycamera);
				float tbrayz2_f = CameraPositionZ(t.terrain.gameplaycamera);

				// first ensure not going through physics terrain
				if (ODERayTerrain(tbrayx1_f, tbrayy1_f, tbrayz1_f, tbrayx2_f, tbrayy2_f, tbrayz2_f, true) == 1)
				{
					tttokay = 0;
				}
				if (tttokay == 1)
				{
					// actually move ray BACK a little in case enemy right up against something!
					int ttintersectvalue = IntersectAllEx(g.entityviewstartobj, g.entityviewendobj, tbrayx1_f, tbrayy1_f, tbrayz1_f, tbrayx2_f, tbrayy2_f, tbrayz2_f, cas.obj, 0, cas.e, 500, 1, false);
					if (ttintersectvalue != 0)
					{
						tttokay = 0;
					}
				}
				if (tttokay == 1)
				{
					t.entityelement[cas.e].plrvisible = 1;
					t.entityelement[cas.e].lua.flagschanged = 1;
				}
			}
		}
	}
}

void darkai_mouthandheadtracking (void)
{
	// impose anim frame overrides on top of regular animation
	int iCharObj = t.charanimstate.obj;

	// mouth simulation
	float fTimeFromStartOfSpeak = 0;
	if (t.charanimstate.ccpo.speak.fMouthTimeStamp == 0.0f)
	{
		// waiting for mouth timer to be started (elsewhere)
		t.charanimstate.ccpo.speak.iMouthDataShape = 0;
	}
	else
	{
		// only if mouth data
		if (t.charanimstate.ccpo.speak.mouthData.size() > 0)
		{
			fTimeFromStartOfSpeak = ((float)MAXTimer() / 1000.0f) - t.charanimstate.ccpo.speak.fMouthTimeStamp;
			int iMouthDataNextIndex = t.charanimstate.ccpo.speak.iMouthDataIndex;
			if (fTimeFromStartOfSpeak > t.charanimstate.ccpo.speak.mouthData[iMouthDataNextIndex].fTimeStamp)
			{
				int iMouthDataShape = t.charanimstate.ccpo.speak.mouthData[iMouthDataNextIndex].iMouthShape;
				t.charanimstate.ccpo.speak.iMouthDataShape = iMouthDataShape;
				iMouthDataNextIndex++;
				t.charanimstate.ccpo.speak.fSmouthDataSpeedToNextShape = 4.0f;
				t.charanimstate.ccpo.speak.iMouthDataIndex = iMouthDataNextIndex;
				if (t.charanimstate.ccpo.speak.iMouthDataIndex >= t.charanimstate.ccpo.speak.mouthData.size())
				{
					t.charanimstate.ccpo.speak.fMouthTimeStamp = 0;
					t.charanimstate.ccpo.speak.iMouthDataIndex = 0;
				}
			}
			else
			{
				// modulate speed to final mouth shape based on closeness to next shape
				int iMouthDataCurrentIndex = iMouthDataNextIndex - 1;
				if (iMouthDataCurrentIndex >= 0)
				{
					float fTimeDifference = t.charanimstate.ccpo.speak.mouthData[iMouthDataNextIndex].fTimeStamp - t.charanimstate.ccpo.speak.mouthData[iMouthDataCurrentIndex].fTimeStamp;
					float fTimeToNextShape = fTimeFromStartOfSpeak - t.charanimstate.ccpo.speak.mouthData[iMouthDataCurrentIndex].fTimeStamp;
					t.charanimstate.ccpo.speak.fSmouthDataSpeedToNextShape = 1.0f + ((1.0f - (fTimeToNextShape / fTimeDifference))*3.0f);
				}
			}
		}
	}
	sObject* pCharObject = GetObjectData (iCharObj);
	int iFinalFrameToUse = -1;
	if (t.charanimstate.ccpo.speak.mouthData.size() > 0)
	{
		iFinalFrameToUse = t.charanimstate.ccpo.speak.iMouthDataShape;
		if (iFinalFrameToUse == 0)
		{
			iFinalFrameToUse = 12;
			if (t.charanimstate.ccpo.speak.fNeedToBlink > 1.0f)
			{
				// randomise blink (maybe use blink logic in future)
				t.charanimstate.ccpo.speak.fNeedToBlink = -0.05f;
			}
			if (t.charanimstate.ccpo.speak.fNeedToBlink < 0.0f)
			{
				// allow blink for a few frames
				t.charanimstate.ccpo.speak.fSmouthDataSpeedToNextShape = 5.0f;
				iFinalFrameToUse = 13;
			}
		}
		if (t.charanimstate.ccpo.speak.fNeedToBlink > 0.0f)
		{
			double dPowerRandom = rand() % 10000;
			if (dPowerRandom > 9900.0)
				dPowerRandom = 5.0;
			else
				dPowerRandom = dPowerRandom / 100000.0;
			t.charanimstate.ccpo.speak.fNeedToBlink += 0.0001f + (float)(dPowerRandom / 10.0f);
		}
		else
			t.charanimstate.ccpo.speak.fNeedToBlink += 0.01f;
	}

	// Neck(head) tracking is smoother as called all the time now
	float fLookAtX = 0;
	float fLookAtY = 0;
	float fLookAtZ = 0;
	if (t.charanimstate.entityTarget > 0)
	{
		fLookAtX = t.entityelement[t.charanimstate.entityTarget].x;
		fLookAtZ = t.entityelement[t.charanimstate.entityTarget].z;

		// so character looks at eye level from ground
		if (t.charanimstate.entityTargetYOffset_f != 0.0f)
			fLookAtY = t.entityelement[t.charanimstate.entityTarget].y + t.charanimstate.entityTargetYOffset_f;
		else
			fLookAtY = t.entityelement[t.charanimstate.entityTarget].y + 65.0f; 
	}
	else
	{
		fLookAtX = CameraPositionX();
		fLookAtY = CameraPositionY();
		fLookAtZ = CameraPositionZ();
	}
	float fDX = fLookAtX - ObjectPositionX (t.charanimstate.obj);
	float fDZ = fLookAtZ - ObjectPositionZ (t.charanimstate.obj);
	float fDD = sqrt (fabs(fDX*fDX) + fabs(fDZ*fDZ));

	// work out how far to twist spine left and right
	float fNeckLimit = t.charanimstate.neckRightAndLeftLimit;
	float fDA = atan2deg(fDX, fDZ);
	float fAIObjAngleY = ObjectAngleY (t.charanimstate.obj);
	float fDiffA = WrapValue(fDA) - WrapValue(fAIObjAngleY);
	if (t.charanimstate.neckAiming < -5000.0f )
	{
		float fForcedAngle = t.charanimstate.neckAiming + 10000;
		fDiffA = fForcedAngle;
	}
	if (t.charanimstate.neckAiming == 0.0f) fDiffA = 0.0f;
	if (fDiffA < -180) fDiffA = fDiffA + 360;
	if (fDiffA > 180) fDiffA = fDiffA - 360;
	if (fabs(fDiffA) > fNeckLimit)
	{
		float fLeftSideLimit = -fNeckLimit / 1.3f;
		float fRightSideLimit = fNeckLimit * 1.3f;
		if (fDiffA < fLeftSideLimit) fDiffA = fLeftSideLimit;
		if (fDiffA > fRightSideLimit) fDiffA = fRightSideLimit;
	}
	float fRightAndLeft = fDiffA + t.charanimstate.neckRightAndLeftOffset;

	// and the neck up and down
	float fDY = fLookAtY - (ObjectPositionY (t.charanimstate.obj) + 70.0f);
	fDA = WrapValue (GGToDegree(atan2(fDY, fDD)));
	if (t.charanimstate.neckAiming == 0.0f) fDA = 0.0f;
	fNeckLimit = t.charanimstate.neckUpAndDownLimit;
	if (fDA < 360 - fNeckLimit && fDA > 180.0f) fDA = 360 - fNeckLimit;
	if (fDA > fNeckLimit && fDA < 180.0f) fDA = fNeckLimit;
	float fYLookUpAndDown = fDA + t.charanimstate.neckUpAndDownOffset;
	fYLookUpAndDown -= t.charanimstate.spineYAdjust; // adjust for any spine tilt too!

	// smooth transition to final angle
	float fTransitionSpeed = t.charanimstate.neckAiming;
	if (fTransitionSpeed <= 0.0f)
	{
		// make head reset graceful and smooth
		if (t.charanimstate.neckAiming < -5000.0f)
			fTransitionSpeed = 3;
		else
			fTransitionSpeed = 10;
	}
	float fSmoothSpeed = fTransitionSpeed / 100.0f;
	float fDiffX = fRightAndLeft; if (fDiffX >= 180.0f) fDiffX -= 360.0f;
	float fDiffY = fYLookUpAndDown; if (fDiffY >= 180.0f) fDiffY -= 360.0f;
	fDiffX -= t.charanimstate.neckRightAndLeft;
	fDiffY -= t.charanimstate.neckUpAndDown;
	t.charanimstate.neckRightAndLeft += fDiffX * fSmoothSpeed;
	t.charanimstate.neckUpAndDown += fDiffY * fSmoothSpeed;

	// clever system to reset pose to use a specific frame, and then allow regular animation to transform on top of it
	if (t.charanimstate.ccpo.settings.iNeckBone > 0)
	{
		sFrame* pFrameOfLimb = pCharObject->ppFrameList[t.charanimstate.ccpo.settings.iNeckBone];
		if (pFrameOfLimb)
		{
			if (iFinalFrameToUse >= 0)
			{
				// only use mouth and blink if have mouth data (and the object supports mouthshapes)
				WickedCall_SetObjectPreFrames(pCharObject, "Bip01_Head", iFinalFrameToUse, t.charanimstate.ccpo.speak.fSmouthDataSpeedToNextShape, 2);
			}
			WickedCall_RotateLimb(pCharObject, pFrameOfLimb, t.charanimstate.neckRightAndLeft, t.charanimstate.neckUpAndDown, 0); 
		}
	}
}

void darkai_spinetracking (void)
{
	// difference between player and character
	float fLookAtX = CameraPositionX();
	float fLookAtY = CameraPositionY();
	float fLookAtZ = CameraPositionZ();
	if (t.charanimstate.entityTarget > 0)
	{
		int ee = t.charanimstate.entityTarget;
		fLookAtX = t.entityelement[ee].x;
		fLookAtZ = t.entityelement[ee].z;

		// so character looks at eye level from ground
		if (t.charanimstate.entityTargetYOffset_f != 0.0f)
			fLookAtY = t.entityelement[ee].y + t.charanimstate.entityTargetYOffset_f;
		else
			fLookAtY = t.entityelement[ee].y + 65.0f;
	}
	float fDX = fLookAtX - ObjectPositionX (t.charanimstate.obj);
	float fDZ = fLookAtZ - ObjectPositionZ (t.charanimstate.obj);
	float fDD = sqrt (fabs(fDX*fDX) + fabs(fDZ*fDZ));
	
	// work out how far to twist spine left and right
	float fSpineLimit = t.charanimstate.spineRightAndLeftLimit;
	float fDA = atan2deg(fDX, fDZ);
	float fAIObjAngleY = ObjectAngleY (t.charanimstate.obj);
	float fDiffA = WrapValue(fDA) - WrapValue(fAIObjAngleY);
	if (t.charanimstate.spineAiming == 0.0f ) fDiffA = 0.0f;
	if (fDiffA < -180) fDiffA = fDiffA + 360;
	if (fDiffA > 180) fDiffA = fDiffA - 360;
	if (fabs(fDiffA) > fSpineLimit)
	{
		if (fDiffA < -fSpineLimit) fDiffA = -fSpineLimit;
		if (fDiffA > fSpineLimit) fDiffA = fSpineLimit;
	}
	float fspineRightAndLeft = fDiffA + t.charanimstate.spineRightAndLeftOffset;

	// calculate adjustment based on primary twist angle (using idle pistol crouch as basis for adjustments!!)
	float fspineYAdjust = (fabs(fspineRightAndLeft) / fSpineLimit) * -15.0f;
	float fspineZAdjust = (fspineRightAndLeft / fSpineLimit) * -30.0f;

	// work out how far to aim up and down
	float fSpineVertLimit = t.charanimstate.spineUpAndDownLimit;
	float fDY = fLookAtY - (ObjectPositionY (t.charanimstate.obj)+65);
	float fVertAngle = atan2deg(fDY, fDD);
	if (t.charanimstate.spineAiming == 0.0f) fVertAngle = 0.0f;
	if (fVertAngle < -180) fVertAngle = fVertAngle + 360;
	if (fVertAngle > 180) fVertAngle = fVertAngle - 360;
	if (fabs(fVertAngle) > fSpineVertLimit)
	{
		if (fVertAngle < -fSpineVertLimit) fVertAngle = -fSpineVertLimit;
		if (fVertAngle > fSpineVertLimit) fVertAngle = fSpineVertLimit;
	}
	fspineYAdjust = fspineYAdjust + fVertAngle + t.charanimstate.spineUpAndDownOffset;

	// smooth transition to final angle
	float fTransitionSpeed = t.charanimstate.spineAiming;
	if (fTransitionSpeed == 0.0f)
	{
		// make aim reset graceful and smooth
		fTransitionSpeed = 10;
	}
	float fSmoothSpeed = fTransitionSpeed / 100.0f;
	float fDiffX = fspineRightAndLeft; if (fDiffX >= 180.0f) fDiffX -= 360.0f;
	float fDiffY = fspineYAdjust; if (fDiffY >= 180.0f) fDiffY -= 360.0f;
	float fDiffZ = fspineZAdjust; if (fDiffZ >= 180.0f) fDiffZ -= 360.0f;
	fDiffX -= t.charanimstate.spineRightAndLeft;
	fDiffY -= t.charanimstate.spineYAdjust;
	fDiffZ -= t.charanimstate.spineZAdjust;
	t.charanimstate.spineRightAndLeft += fDiffX * fSmoothSpeed;
	t.charanimstate.spineYAdjust += fDiffY * fSmoothSpeed;
	t.charanimstate.spineZAdjust += fDiffZ * fSmoothSpeed;

	// impose anim frame overrides on top of regular animation
	int iCharObj = t.charanimstate.obj;
	if (iCharObj > 0)
	{
		int iEntID = t.entityelement[t.charanimstate.e].bankindex;
		int iFrameIndex = t.entityprofile[iEntID].spine2;
		if (iFrameIndex > 0)
		{
			sObject* pCharObject = GetObjectData(iCharObj);
			sFrame* pFrameOfLimb = pCharObject->ppFrameList[iFrameIndex];
			if (pFrameOfLimb)
			{
				WickedCall_RotateLimb(pCharObject, pFrameOfLimb, t.charanimstate.spineRightAndLeft, t.charanimstate.spineYAdjust, t.charanimstate.spineZAdjust);
			}
		}
	}
}

void GetPositionFromAnimFrameLimb (GGVECTOR3* pvecPos, sAnimation* pAnim, float fTime)
{
	DWORD dwKey = 0;
	DWORD dwKeyMax = pAnim->dwNumPositionKeys;
	int keyMin = 0;
	int keyMax = (int)dwKeyMax;
	int keyDiff = keyMax - keyMin;
	int keyCentre = (int)(keyMin + ((keyDiff) / 2.0));
	for (; keyDiff > 2;)
	{
		if (pAnim->pPositionKeys[keyCentre + 1].dwTime > fTime)
		{
			// the correct key is in the first half of the divided section
			keyMax = keyCentre;
		}
		else
		{
			// the correct key is in the second half of the divided section
			keyMin = keyCentre;
		}

		// subdivide
		keyDiff = keyMax - keyMin;
		keyCentre = (int)(keyMin + ((keyDiff) / 2.0));
	}
	if (keyMax >= (int)dwKeyMax) keyMax = dwKeyMax - 1;
	for (int i = keyMin; i <= keyMax; i++)
	{
		if (pAnim->pPositionKeys[i].dwTime <= fTime)
			dwKey = i;
		else
			break;
	}
	if (dwKey == (pAnim->dwNumPositionKeys - 1))
	{
		// use final frame in animation data
		(*pvecPos) = pAnim->pPositionKeys[dwKey].vecPos;
	}
	else
	{
		// calculate the time difference and interpolate time
		float fIntTime = fTime - pAnim->pPositionKeys[dwKey].dwTime;
		(*pvecPos) = pAnim->pPositionKeys[dwKey].vecPos + pAnim->pPositionKeys[dwKey].vecPosInterpolation * (float)fIntTime;
	}
}

bool AdjustPositionSoNoOverlap (int iEntityIndex, float* pX, float* pZ, float fOldX, float fOldZ)
{
	// if character itself does not repell, no overlap adjust needed
	int iEntID = t.entityelement[iEntityIndex].bankindex;
	if (iEntID > 0 && t.entityprofile[iEntID].collisionmode == 22)
		return true;

	bool bCanSafelyShiftXYZ = true;
	float fGapNeeded = 12.0f * 2.0f; // two radius for both characters
	float fCurrentDirX = *pX - fOldX;
	float fCurrentDirZ = *pZ - fOldZ;
	float fCurrentDist = sqrt(fabs(fCurrentDirX*fCurrentDirX) + fabs(fCurrentDirZ*fCurrentDirZ));
	if (fCurrentDist > 0.0f)
	{
		// scan all characters and ensure returned XYZ does not overlap any of them
		for (int tcharanimindex = 1; tcharanimindex <= g.charanimindexmax; tcharanimindex++)
		{
			int ee = t.charanimstates[tcharanimindex].e;
			if (ee > 0 && ee != iEntityIndex && t.entityelement[ee].health > 1 && t.entityelement[ee].active != 0 && t.entityelement[ee].eleprof.disableascharacter == 0 )
			{
				int entid = t.entityelement[ee].bankindex;
				if (entid > 0 && t.entityprofile[entid].collisionmode != 22)
				{
					float fThisX, fThisY, fThisZ;
					fThisX = t.entityelement[ee].x;
					fThisY = t.entityelement[ee].y;
					fThisZ = t.entityelement[ee].z;
					float fEEX = *pX - fThisX;
					float fEEY = t.entityelement[iEntityIndex].y - fThisY;
					float fEEZ = *pZ - fThisZ;
					float fDist = sqrt(fabs(fEEX*fEEX) + fabs(fEEY*fEEY) + fabs(fEEZ*fEEZ));
					if (fDist < fGapNeeded)
					{
						// the XYZ overlaps another entity position - shift back to origin until no more overlaps
						float fOutsideAngle = GGToDegree(atan2(fEEX, fEEZ));
						float fOutsideX = NewXValue(fThisX, fOutsideAngle, fGapNeeded);
						float fOutsideZ = NewZValue(fThisZ, fOutsideAngle, fGapNeeded);
						*pX = fOutsideX;
						*pZ = fOutsideZ;

						// before use this new position, ensure it does not leave the navmesh
						float vecNearestPt[3];
						bool bMustBeOverPoly = true;
						if (g_RecastDetour.isWithinNavMeshEx (*pX, t.entityelement[iEntityIndex].y, *pZ, (float*)&vecNearestPt, bMustBeOverPoly) == true) // new XYZ only valid inside nav mesh!
						{
							// also ensure it does NOT overlap another
							for (int tcharanimindex2 = 1; tcharanimindex2 <= g.charanimindexmax; tcharanimindex2++)
							{
								int ee2 = t.charanimstates[tcharanimindex2].e;
								if (ee != iEntityIndex && ee2 != iEntityIndex && t.entityelement[ee2].eleprof.disableascharacter == 0)
								{
									float fThisX, fThisZ;
									fThisX = t.entityelement[ee2].x;
									fThisZ = t.entityelement[ee2].z;
									float fEEX = *pX - fThisX;
									float fEEZ = *pZ - fThisZ;
									float fDist = sqrt(fabs(fEEX*fEEX) + fabs(fEEZ*fEEZ));
									if (fDist < fGapNeeded)
									{
										// shifted into a neighbor, this move should not take place!
										bCanSafelyShiftXYZ = false;
										break;
									}
								}
							}
						}
						else
						{
							// shifted out of navmesh, this move should not take place!
							bCanSafelyShiftXYZ = false;
						}
						if (bCanSafelyShiftXYZ == false)
						{
							*pX = fOldX;
							*pZ = fOldZ;
							break;
						}
					}
				}
			}
		}
	}
	return bCanSafelyShiftXYZ;
}

void darkai_handlegotomove(void)
{
	// if any movement required, switch character into a mover (if never need to move, it can animate inplace with good foot planting)
	if (t.charanimstate.requiremovementnow == 0)
	{
		int iPointCount = t.charanimstate.pathPointCount;
		int iPointIndex = t.charanimstate.moveToMode;
		if ((iPointIndex > 0 && iPointCount > 0) || t.charanimstate.remainingMoveDistanceOnPath_f > 0)
		{
			t.charanimstate.requiremovementnow = 1;
		}
	}

	// first run the animation simlation to see if spine tracking movement occurs
	bool bObjectIsSpineTracked = false;
	int iID = t.charanimstate.obj;
	sObject* pObject = GetObjectData(iID);
	if (pObject)
	{
		// first handle any movewithanimation functionality (to get to distance we need the second part of the code to MOVE the object)
		if (pObject->ppFrameList)
		{
			if (pObject->dwSpineCenterLimbIndex > 0)
			{
				// we use animation for movement
				bObjectIsSpineTracked = true;

				// work out position offset of Bip01
				GGVECTOR3 vecBip01PosOffset;
				float fLastAnimFrame = pObject->fAnimLastFrame;
				float fAnimFrame = WickedCall_GetObjectFrame(pObject);
				pObject->fAnimLastFrame = fAnimFrame;
				pObject->fAnimFrame = fAnimFrame;
				if (fLastAnimFrame != -1)
				{
					sFrame* pFrame = pObject->ppFrameList[pObject->dwSpineCenterLimbIndex];
					if (pFrame)
					{
						// work out BIP01 shift since last anim
						GGVECTOR3 vecLastBip01PosOffset;
						GetPositionFromAnimFrameLimb(&vecLastBip01PosOffset, pFrame->pAnimRef, fLastAnimFrame);
						GetPositionFromAnimFrameLimb(&vecBip01PosOffset, pFrame->pAnimRef, fAnimFrame);
						vecBip01PosOffset = vecBip01PosOffset - vecLastBip01PosOffset;
						float fShiftSinceLastAnimX = vecBip01PosOffset.x;
						float fShiftSinceLastAnimZ = vecBip01PosOffset.z;

						// if animation loops back, invalidate any shift (as animation loop is resetting)
						bool bSupressTransitionToHideGlitchDuringLoop = false;
						if (pObject->bAnimLooping == true && fLastAnimFrame > fAnimFrame)
						{
							// ensure to massive backward movement delta when animation Bip01 pos resets
							// just as we deducted the length of anim time, deduct the full anim time offset (so matching the displacement)
							// still not perfect as I think there is time displacement between multithread anim and core thread movemement!
							GGVECTOR3 vecFirstBip01PosOffsetFrame;
							GetPositionFromAnimFrameLimb(&vecFirstBip01PosOffsetFrame, pFrame->pAnimRef, pObject->fAnimLoopStart);
							GGVECTOR3 vecLastBip01PosOffsetFrame;
							GetPositionFromAnimFrameLimb(&vecLastBip01PosOffsetFrame, pFrame->pAnimRef, pObject->fAnimFrameEnd);
							GGVECTOR3 vecFullBip01PosOffsetDisplacement;
							vecFullBip01PosOffsetDisplacement = vecLastBip01PosOffsetFrame - vecFirstBip01PosOffsetFrame;
							fShiftSinceLastAnimX -= vecFullBip01PosOffsetDisplacement.x;
							fShiftSinceLastAnimZ -= vecFullBip01PosOffsetDisplacement.z;
							bSupressTransitionToHideGlitchDuringLoop = true;
						}

						// also some play animations cause huge backward shifts, so detect and eliminate ones too large (unnatural)
						if (fabs(fShiftSinceLastAnimZ) > 10.0f || fabs(fShiftSinceLastAnimX) > 10.0f)
						{
							fShiftSinceLastAnimX = 0.0f;
							fShiftSinceLastAnimZ = 0.0f;
							bSupressTransitionToHideGlitchDuringLoop = true;
						}

						// delta Z takes more time as it uses physics to shift the object, and we damped it here too in an accumilator
						if (bSupressTransitionToHideGlitchDuringLoop == false)
						{
							t.smoothanim[iID].movedeltax *= 0.8f;
							t.smoothanim[iID].movedeltaz *= 0.8f;
							if (fabs(fShiftSinceLastAnimX) + fabs(fShiftSinceLastAnimZ) > 0.0f)
							{
								t.smoothanim[iID].movedeltacontrib += 0.05f;
								if (t.smoothanim[iID].movedeltacontrib > 1.0f) t.smoothanim[iID].movedeltacontrib = 1.0f;
								t.smoothanim[iID].movedeltax += fShiftSinceLastAnimX * t.smoothanim[iID].movedeltacontrib;
								if (fShiftSinceLastAnimX > 0.0f)
								{
									if (t.smoothanim[iID].movedeltax > fShiftSinceLastAnimX) t.smoothanim[iID].movedeltax = fShiftSinceLastAnimX;
								}
								else
								{
									if (t.smoothanim[iID].movedeltax < fShiftSinceLastAnimX) t.smoothanim[iID].movedeltax = fShiftSinceLastAnimX;
								}
								t.smoothanim[iID].movedeltaz += fShiftSinceLastAnimZ * t.smoothanim[iID].movedeltacontrib;
								if (fShiftSinceLastAnimZ > 0.0f)
								{
									if (t.smoothanim[iID].movedeltaz > fShiftSinceLastAnimZ) t.smoothanim[iID].movedeltaz = fShiftSinceLastAnimZ;
								}
								else
								{
									if (t.smoothanim[iID].movedeltaz < fShiftSinceLastAnimZ) t.smoothanim[iID].movedeltaz = fShiftSinceLastAnimZ;
								}
							}
							else
							{
								t.smoothanim[iID].movedeltacontrib -= 0.01f;
								if (t.smoothanim[iID].movedeltacontrib < 0.0f)
									t.smoothanim[iID].movedeltacontrib = 0.0f;
							}
							// and store good deltas
							t.smoothanim[iID].lastmovedeltax = t.smoothanim[iID].movedeltax;
							t.smoothanim[iID].lastmovedeltaz = t.smoothanim[iID].movedeltaz;
						}
						else
						{
							// use known move deltas to hide any transition glitches
							t.smoothanim[iID].movedeltax = t.smoothanim[iID].lastmovedeltax;
							t.smoothanim[iID].movedeltaz = t.smoothanim[iID].lastmovedeltaz;
						}

						// special mode wipes out BIP01 from animation calc
						if (t.charanimstate.requiremovementnow == 1)
						{
							// activate and keep active (non-moving characters can still retain their foot planting if they are just animating in place)
							WickedCall_SetBip01Position(pObject, pFrame, 3, 0, 0);
						}
					}
				}
			}
		}
	}

	// if have movement
	bool bReversingOrStrafing = false;
	float fAdvanceTheMovement = 0.0f;
	if (bObjectIsSpineTracked == true)
	{
		// movement controlled by animation (spinetracking)
		if (t.charanimstate.movingbackward == 1)
		{
			// we are reversing movement logic (so can face forward, animate a backward anim and move along projected backward path vector)
			bReversingOrStrafing = true;
			fAdvanceTheMovement = t.smoothanim[iID].movedeltaz;
		}
		else
		{
			// normal forward movement
			fAdvanceTheMovement = -t.smoothanim[iID].movedeltaz;
		}
		if (fabs(t.smoothanim[iID].movedeltax) > fAdvanceTheMovement) bReversingOrStrafing = true; // strafing
		if (t.charanimstate.iRotationAlongPathMode == 0) bReversingOrStrafing = true; // called by behavior when want animation to perform without rotating to follow projected path
	}
	else
	{
		// regular specified forward movement if spline tracking not used
		if (t.charanimstate.movespeed_f > 0.0f)
		{
			// need to apply FPS-indie speed modified (or 120fps will move it twice is fast along path for example)
			float fLegacy60fpsModulation = g.timeelapsed_f * 3.0f;
			fAdvanceTheMovement = t.charanimstate.movespeed_f * fLegacy60fpsModulation;
		}
	}
	// special mode to slowly re-introduce ability to rotate to face path direction (used when anims need to stay facing X for strafes/backoffs/etc)
	if (t.charanimstate.iRotationAlongPathMode > 0 && t.charanimstate.iRotationAlongPathMode < 100)
	{
		t.charanimstate.iRotationAlongPathMode += 2;
		if (t.charanimstate.iRotationAlongPathMode > 100)
			t.charanimstate.iRotationAlongPathMode = 100;
	}
	// record old position
	float fOldPosX = t.entityelement[t.charanimstate.e].x;
	float fOldPosZ = t.entityelement[t.charanimstate.e].z;
	float fOldMovementRequired = fAdvanceTheMovement;
	// if we are advancing a move, do it here
	if (fAdvanceTheMovement > 0.0f)
	{
		// we need to move the object
		float fCurrentMoveDistance = fAdvanceTheMovement;
		int iPointCount = t.charanimstate.pathPointCount;
		int iPointIndex = t.charanimstate.moveToMode;
		if (iPointIndex > 0 && iPointCount > 0)
		{
			// reset until need it (at end of path traversal)
			t.charanimstate.remainingMoveDistanceOnPath_f = 0;
			t.charanimstate.remainingOverallDistanceToDest_f = 0;

			// eat through points until above movement distance used up
			float fDiffA = t.charanimstate.moveangle_f; // redundant, but assighned here to show cyclic nature of fDiffA
			float fCurrentX = t.entityelement[t.charanimstate.e].x;
			float fCurrentZ = t.entityelement[t.charanimstate.e].z;
			bool bEatPointsInPath = true;
			while (bEatPointsInPath == true)
			{
				float thisPoint[3] = { -1, -1, -1 };
				while (iPointIndex > 0)
				{
					// work out angle and distance in direction of next point
					thisPoint[0] = t.charanimstate.pointx[iPointIndex];
					thisPoint[2] = t.charanimstate.pointz[iPointIndex];
					float fDiffX = thisPoint[0] - fCurrentX;
					float fDiffZ = thisPoint[2] - fCurrentZ;
					float tdisttopoint = sqrt(fabs(fDiffX * fDiffX) + fabs(fDiffZ * fDiffZ));
					fDiffA = GGToDegree(atan2(fDiffX, fDiffZ));
					if (fDiffA < -180) fDiffA = fDiffA + 360;
					if (fDiffA > 180) fDiffA = fDiffA - 360;
					// also work out distance to final point in current path
					float fFinalPointX = t.charanimstate.pointx[iPointCount - 1];
					float fFinalPointY = t.charanimstate.pointy[iPointCount - 1];
					float fFinalPointZ = t.charanimstate.pointz[iPointCount - 1];
					float fFinalDiffX = fFinalPointX - fCurrentX;
					float fFinalDiffZ = fFinalPointZ - fCurrentZ;
					float fFinalDist = sqrt(fabs(fFinalDiffX * fFinalDiffX) + fabs(fFinalDiffZ * fFinalDiffZ));
					t.charanimstate.remainingOverallDistanceToDest_f = fFinalDist;
					float fStopAheadOfFinishingPositionBy = t.charanimstate.iStopFromEnd;// 10.0f;// 75.0f; //LB: would be good to scale this by genral movement speed (anim or non-anim based)
					bool bEndThisPathTraversal = false;
					if (fFinalDist < fStopAheadOfFinishingPositionBy)
					{
						// special mode allows future animations to travel on the path (but never exceed it less they go through walls)
						t.charanimstate.remainingMoveDistanceOnPath_f = fStopAheadOfFinishingPositionBy;
						bEndThisPathTraversal = true;
					}
					else
					{
						if (tdisttopoint >= fCurrentMoveDistance)
						{
							// we found a point that is beyond our remaining distance to travel
							// no more distance to traverse - keep remaining fCurrentMoveDistance for below
							bEatPointsInPath = false;
							break;
						}
						else
						{
							// next point is within the overall remaining distance, so eat it
							fCurrentMoveDistance -= tdisttopoint;
							fCurrentX = thisPoint[0];
							fCurrentZ = thisPoint[2];
							iPointIndex++;
							if (iPointIndex >= iPointCount)
							{
								bEndThisPathTraversal = true;
							}
						}
					}
					if (bEndThisPathTraversal == true)
					{
						bEatPointsInPath = false;
						fCurrentMoveDistance = 0.0f;
						t.charanimstate.movingbackward = 0;
						t.charanimstate.movespeed_f = 0.0f;
						iPointIndex = 0;
						break;
					}
				}
			}
			// if reached end of path, will NOT call this last movement
			if (fCurrentMoveDistance > 0.0f)
			{
				// set the final position along the path of latest points
				t.entityelement[t.charanimstate.e].x = NewXValue(fCurrentX, fDiffA, fCurrentMoveDistance);
				t.entityelement[t.charanimstate.e].z = NewZValue(fCurrentZ, fDiffA, fCurrentMoveDistance);
				// hard angle set for the movement along this last part
				t.charanimstate.moveangle_f = fDiffA; // redundant, we have already moved the XZ
				// scan ahead to a point further than 10 units away, and make that the look angle (so never spinny spinny over own XZ position)
				float fDistNow = 0.0f;
				float fLookAheadAngle = 0.0f;
				int iScanPointIndex = iPointIndex;
				while (fDistNow < 10.0f && iScanPointIndex < iPointCount)
				{
					float fDX = t.charanimstate.pointx[iScanPointIndex] - t.entityelement[t.charanimstate.e].x;
					float fDZ = t.charanimstate.pointz[iScanPointIndex] - t.entityelement[t.charanimstate.e].z;
					fDistNow = sqrt(fabs(fDX * fDX) + fabs(fDZ * fDZ));
					fLookAheadAngle = GGToDegree(atan2(fDX, fDZ));
					if (fLookAheadAngle < -180) fLookAheadAngle = fLookAheadAngle + 360;
					if (fLookAheadAngle > 180) fLookAheadAngle = fLookAheadAngle - 360;
					iScanPointIndex++;
				}
				if (fDistNow >= 10.0f)
				{
					// this commands a smooth rotation using slowly system (separate from move angle)
					if (bReversingOrStrafing == false)
					{
						// calculate the object rotation from path movement
						t.charanimstate.currentangle_f = fLookAheadAngle;
						float fModulateRotSpeed = t.charanimstate.iRotationAlongPathMode / 100.0f;
						t.charanimstate.currentangleslowlyspeed_f = (t.charanimstate.turnspeed_f * fModulateRotSpeed);
					}
					else
					{
						// leave last angle along if reversing during path follow (back off)
					}
				}
			}
			// set the updated point index
			t.charanimstate.moveToMode = iPointIndex;
			// reset Z movement so not to affect footplant object reposition below
			if (t.charanimstate.remainingMoveDistanceOnPath_f == 0)
			{
				// only when complete stop, not for when we completed path early allowing anim to continue to use up remaining path move distance
				fAdvanceTheMovement = 0.0f;
			}
		}
		else
		{
			if (t.charanimstate.remainingMoveDistanceOnPath_f > 0)
			{
				t.charanimstate.remainingMoveDistanceOnPath_f -= fabs(fAdvanceTheMovement);
				if (t.charanimstate.remainingMoveDistanceOnPath_f < 0)
				{
					t.charanimstate.remainingMoveDistanceOnPath_f = 0;
					fAdvanceTheMovement = 0.0f;
				}
			}
			else
			{
				// need this otherwise char uses pure anim move to go through walls, etc!!
				fAdvanceTheMovement = 0.0f;
			}
		}
	}
	// always resolve animation movement deltas
	if (t.charanimstate.requiremovementnow == 1)
	{
		GGVECTOR3 vecShift = GGVECTOR3(0, 0, 0);
		vecShift = GGVECTOR3(-t.smoothanim[iID].movedeltax, 0, 0); // prevents subtle shifts in Z from moving away from navmesh good position (zombies through fences)
		GGVec3TransformCoord(&vecShift, &vecShift, &pObject->position.matRotation);
		t.entityelement[t.charanimstate.e].x -= vecShift.x;
		t.entityelement[t.charanimstate.e].z -= vecShift.z;
	}
	t.smoothanim[iID].movedeltax = 0;
	t.smoothanim[iID].movedeltaz = 0;

	// handle if actually moved or was dynamically blocked
	bool bDynamicAvoidanceTriggered = false;
	if (fOldMovementRequired > 0.0f)
	{
		float fDX = t.entityelement[t.charanimstate.e].x - fOldPosX;
		float fDZ = t.entityelement[t.charanimstate.e].z - fOldPosZ;
		float fDDActuallyTravelled = sqrt(fabs(fDX * fDX) + fabs(fDZ * fDZ));
		if (fDDActuallyTravelled < fOldMovementRequired * 0.25f)
			bDynamicAvoidanceTriggered = true;
	}
	// ensure free of other characters
	if (AdjustPositionSoNoOverlap(t.charanimstate.e, &t.entityelement[t.charanimstate.e].x, &t.entityelement[t.charanimstate.e].z, fOldPosX, fOldPosZ) == false)
		bDynamicAvoidanceTriggered = true;

	// was stopp4ed in moving as required
	if (bDynamicAvoidanceTriggered == false)
	{
		// no overlap, no blockage, or was allowed to shift around another character safely (still in nav mesh)
		t.entityelement[t.charanimstate.e].lua.dynamicavoidance = 0;
		t.entityelement[t.charanimstate.e].lua.dynamicavoidancestuckclock = MAXTimer();
	}
	else
	{
		// shift did not take place, character heading THROUGH another one
		// signal to AI so behavior can do something about walking through other people!
		// or signal that the path has been blocked and cannot move onwards for some reason
		t.entityelement[t.charanimstate.e].lua.dynamicavoidance = MAXTimer() - t.entityelement[t.charanimstate.e].lua.dynamicavoidancestuckclock;
	}
	// special flag which interupts any path in progress
	if (t.entityelement[t.charanimstate.e].lua.interuptpath > 0)
	{
		t.entityelement[t.charanimstate.e].lua.dynamicavoidance = 1000;
		t.entityelement[t.charanimstate.e].lua.interuptpath--;
	}

	//PE: Optimize 2024 Dont process below if we did not move (rays).
	//LB: However this stops characters falling down if placed above a surface, so added warmupcharacteratstart_f
	float fDX = t.entityelement[t.charanimstate.e].x - t.entityelement[t.charanimstate.e].lastx;
	float fDY = t.entityelement[t.charanimstate.e].y - t.entityelement[t.charanimstate.e].lasty;
	float fDZ = t.entityelement[t.charanimstate.e].z - t.entityelement[t.charanimstate.e].lastz;
	float fMoved = sqrt(fabs(fDX * fDX) + fabs(fDY * fDY) + fabs(fDZ * fDZ));
	if (fMoved > 0.05 || (t.charanimstate.dormant==0 && t.charanimstate.warmupcharacteratstart_f > 0.0f) )
	{
		// find surface for object at this XZ position, faster than capsule and ensures object is purely navmesh/movement driven (not softy physics driven)
		bool bSurfaceFound = false;
		float fStepUp = 35.0f;
		float fStepDown = 50000.0f;
		int iCollisionMode = (1 << (0)) | (1 << (1)) | (1 << (3)); //COL_TERRAIN | COL_OBJECT | COL_OBJECT_DYNAMIC;
		float fSurfaceYPosition = -10000.0f;
		for (int iBoxScan = 0; iBoxScan < 4; iBoxScan++)
		{
			float fOffsetX = -1.0f;
			float fOffsetZ = -1.0f;
			if (iBoxScan == 1) { fOffsetX = 1.0f; fOffsetZ = -1.0f; }
			if (iBoxScan == 2) { fOffsetX = -1.0f; fOffsetZ = 1.0f; }
			if (iBoxScan == 3) { fOffsetX = 1.0f; fOffsetZ = 1.0f; }
			if (ODERayTerrainEx(t.entityelement[t.charanimstate.e].x + fOffsetX, t.entityelement[t.charanimstate.e].y + fStepUp, t.entityelement[t.charanimstate.e].z + fOffsetZ, t.entityelement[t.charanimstate.e].x + fOffsetX, t.entityelement[t.charanimstate.e].y - fStepDown, t.entityelement[t.charanimstate.e].z + fOffsetZ, iCollisionMode, false) == 1)
			{
				float fThisSurfaceY = ODEGetRayCollisionY();
				if (fThisSurfaceY > fSurfaceYPosition) fSurfaceYPosition = fThisSurfaceY;
				bSurfaceFound = true;
			}
		}
		if (bSurfaceFound == false)
		{
			// probably on steep terrain, step up of 15 at high speed cannot handle this, so ensure LOWEST point is terrain 
			float fThisSurfaceY = BT_GetGroundHeight(0, t.entityelement[t.charanimstate.e].x, t.entityelement[t.charanimstate.e].z);
			if (fThisSurfaceY > fSurfaceYPosition) fSurfaceYPosition = fThisSurfaceY;
		}
		if (fSurfaceYPosition > -10000.0f)
		{
			float fYPosition = t.entityelement[t.charanimstate.e].y;
			float fDifference = fSurfaceYPosition - t.entityelement[t.charanimstate.e].y;
			if (fDifference < 0.0f)
			{
				// a drop, make object fall using gravity of 1G
				t.entityelement[t.charanimstate.e].climbgravity -= g.timeelapsed_f * 0.2f;
				if (t.entityelement[t.charanimstate.e].climbgravity < -50.0f) t.entityelement[t.charanimstate.e].climbgravity = -50.0f;
				fYPosition += g.timeelapsed_f * 6 * t.entityelement[t.charanimstate.e].climbgravity;
				if (fYPosition < fSurfaceYPosition) fYPosition = fSurfaceYPosition;

				// ensure fall for newly minted characters continues until we hit the surface
				t.charanimstate.warmupcharacteratstart_f = 1.0f;
			}
			else
			{
				// walkable surface raises or stays level, set fNewYPosition to surface using a lerp curve, and reset to no gravity fall
				fYPosition += g.timeelapsed_f * fDifference * 0.5f;
				t.entityelement[t.charanimstate.e].climbgravity = 0.0f;
			}
			t.entityelement[t.charanimstate.e].y = fYPosition;
		}
		// tilt object X axis if tilt mode active (for non-bipeds like rats, horses)
		if (t.charanimstate.iTiltMode == 1)
		{
			float fX = t.entityelement[t.charanimstate.e].x;
			float fY = t.entityelement[t.charanimstate.e].y;
			float fZ = t.entityelement[t.charanimstate.e].z;
			float fNewY = fY;
			if (g_RecastDetour.isWithinNavMesh(fX, fY, fZ) == true)
			{
				fY = g_RecastDetour.getYFromPos(fX, fY, fZ);
			}
			else
			{
				fY = BT_GetGroundHeight(0, fX, fZ);
			}
			float fNewX = NewXValue(fX, t.charanimstate.currentangle_f, 5.0f);
			float fNewZ = NewZValue(fZ, t.charanimstate.currentangle_f, 5.0f);
			if (g_RecastDetour.isWithinNavMesh(fNewX, fY, fNewZ) == true)
			{
				fNewY = g_RecastDetour.getYFromPos(fNewX, fY, fNewZ);
			}
			else
			{
				fNewY = BT_GetGroundHeight(0, fNewX, fNewZ);
			}
			float fDX = fNewX - fX;
			float fDZ = fNewZ - fZ;
			float fDistXZ = sqrt(fabs(fDX * fDX) + fabs(fDZ * fDZ));
			float fDistY = fNewY - fY;
			float fTiltXAxis = LimbAngleX(iID, 0);
			float fTiltXAxisDest = GGToDegree(atan2(fDistY, fDistXZ));
			fTiltXAxis += (fTiltXAxisDest - fTiltXAxis) * 0.1f;
			RotateLimb(iID, 0, fTiltXAxis, 0, 0);
		}
	}

	// run t.charanimstate.warmupcharacteratstart_f down to zero
	if (t.charanimstate.dormant == 0 && t.charanimstate.warmupcharacteratstart_f > 0.0f)
	{
		t.charanimstate.warmupcharacteratstart_f -= g.timeelapsed_f;
		if (t.charanimstate.warmupcharacteratstart_f < 0) t.charanimstate.warmupcharacteratstart_f = 0.0f;
	}

	// run physics to ensure capsule keeps up with world position of object (includes PositionObject for final XYZ placement)
	int stte = t.te;
	int sttv = t.tv_f;
	int stcharanimindex = g.charanimindex;
	t.te = t.charanimstate.e; t.tv_f = 0; g.charanimindex = 0;
	entity_updatepos();
	t.te = stte; t.tv_f = sttv; g.charanimindex = stcharanimindex;
	// final placement of object
	PositionObject (iID, t.entityelement[t.charanimstate.e].x, t.entityelement[t.charanimstate.e].y, t.entityelement[t.charanimstate.e].z);
}

void darkai_loop (void)
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif
	// all characters in game
	g_bDormantCheckForThisCycle = true;
	DWORD dwCurrentTime = timeGetTime();
	for (g.charanimindex = 1; g.charanimindex <= g.charanimindexmax; g.charanimindex++)
	{
		// This character
		t.charanimstate = t.charanimstates[g.charanimindex];
		t.te = t.charanimstate.e;

		// handle updating of debug objects (cone of sight for characters)
		bool bCharIsActive = true;
		if (t.entityelement[t.te].active == 0) bCharIsActive = false;

		// also do not show if invisible (maybe have this on a toggle down the road)
		int tobj = t.entityelement[t.te].obj;
		if (tobj > 0)
		{
			if (ObjectExist(tobj) == 1 && GetVisible(tobj) == 0)
			{
				bCharIsActive = false;
			}
		}

		// call the update
		darkai_updatedebugobjects_forcharacter (bCharIsActive);

		// no longer part of the system
		if (t.entityelement[t.charanimstate.e].ragdollplusactivate > 0 )
			continue;

		// Lua calls darkai_calcplrvisible 2 times per char per sync, very expensive, only do one check.
		t.entityelement[t.charanimstate.e].bPlrVisibleCheckDone = false;

		// Entity Element Index for this A.I character
		t.i = t.charanimstate.obj;

		// ensure can stop looping sound ANY time
		t.ttsnd = t.charanimstate.firesoundindex;
		if (t.ttsnd > 0)
		{
			if (SoundExist(t.ttsnd) == 1)
			{
				if (MAXTimer() > (int)t.charanimstate.firesoundexpiry)
				{
					StopSound (t.ttsnd);
				}
				if (SoundPlaying(t.ttsnd) == 0)
				{
					t.charanimstate.firesoundindex = 0;
				}
			}
		}

		// is entiy active here (and not disabled [as a character])
		if (t.entityelement[t.te].active == 1 && t.entityelement[t.te].eleprof.disableascharacter == 0)
		{
			// For valid A.I entities
			if (t.entityelement[t.charanimstate.e].health > 0)
			{
				// If in range for activity
				//if (t.entityelement[t.charanimstate.e].plrdist < t.maximumnonefreezedistance || t.entityelement[t.charanimstate.e].eleprof.phyalways != 0)
				if (t.entityelement[t.charanimstate.e].plrdist < MAXFREEZEDISTANCE || t.entityelement[t.charanimstate.e].eleprof.phyalways != 0)
				{
					// character can be flagged as back in range
					if (t.charanimstate.outofrange == 1)
					{
						t.entityelement[t.charanimstate.e].lua.outofrangefreeze = 0;
						t.charanimstate.outofrange = 0;
					}

					// active characters can be dormant, and awoken when they pass a line of sight check
					if (t.charanimstate.dormant == 1)
					{
						// infrequent check to awaken dormant AI
						if (dwCurrentTime > t.charanimstate.dormanttimer)
						{
							// only one check per cycle
							if (g_bDormantCheckForThisCycle == true)
							{
								t.charanimstate.dormanttimer = dwCurrentTime + 1000;
								int tthitvalue = 0;
								float fX = CameraPositionX(0);
								float fY = CameraPositionY(0);
								float fZ = CameraPositionZ(0);
								float fTargetPosX = t.entityelement[t.charanimstate.e].x;
								float fTargetPosY = t.entityelement[t.charanimstate.e].y + 65;// head height was 40;
								float fTargetPosZ = t.entityelement[t.charanimstate.e].z;
								float fDX = fTargetPosX - fX;
								float fDY = fTargetPosY - fY;
								float fDZ = fTargetPosZ - fZ;
								float fDistance = sqrt(fabs(fDX*fDX) + fabs(fDY*fDY) + fabs(fDZ*fDZ));
								if (fDistance < 500.0f || t.entityelement[t.charanimstate.e].eleprof.phyalways != 0 )
								{
									// closer than this, and wake up the character, player getting very close!
								}
								else
								{
									if (ODERayTerrain(fX, fY, fZ, fTargetPosX, fTargetPosY, fTargetPosZ, false) == 1) tthitvalue = -1;
									if (tthitvalue == 0) tthitvalue = ODERay (fX, fY, fZ, fTargetPosX, fTargetPosY, fTargetPosZ, (1 << 1));//COL_OBJECT);
								}
								if (tthitvalue == 0)
								{
									// released into game action - can see it!
									t.charanimstate.dormant = 0;

									// additionally, agro needs to alert other characters who are in proximity
									for ( int charanimindex2 = 1; charanimindex2 <= g.charanimindexmax; charanimindex2++)
									{
										int ee = t.charanimstates[charanimindex2].e;
										if (ee > 0 && t.entityelement[ee].active == 1 )
										{
											float fDX = t.entityelement[ee].x - t.entityelement[t.charanimstate.e].x;
											float fDY = t.entityelement[ee].y - t.entityelement[t.charanimstate.e].y;
											float fDZ = t.entityelement[ee].z - t.entityelement[t.charanimstate.e].z;
											float fDist = sqrt(fabs(fDX*fDX) + fabs(fDY*fDY) + fabs(fDZ*fDZ));
											if (fDist < t.entityelement[ee].eleprof.conerange)
											{
												t.charanimstates[charanimindex2].dormant = 0;
											}
										}
									}
								}
								g_bDormantCheckForThisCycle = false;
							}
						}
					}
					else
					{
						// character is active, in range, process mouth and head tracking
						if (g_ObjectList[t.charanimstate.obj]->iFrameCount >= 37) // prevent errors on old models
						{
							// apply frames over main animation for mouth phoneme and blinking
							darkai_mouthandheadtracking();

							// and handle any spine tracking
							darkai_spinetracking();
						}

						// if character has mobility
						if (t.entityelement[t.charanimstate.e].eleprof.isimmobile == 0)
						{
							// replaces old 'rotateYslowly' with better 'on-target' path following system
							darkai_handlegotomove();
						}
					}
				}
				else
				{
					// Character out of range
					if (t.charanimstate.outofrange == 0)
					{
						// freeze activity until back in operating range
						t.tte = t.charanimstate.e;
						t.tobj = t.charanimstate.obj;
						t.tentid = t.entityelement[t.charanimstate.e].bankindex;
						entity_resettodefaultanimation();
						// restart behavior (too many unknowns when stop LUA logic mid-sequence!)
						LuaSetFunction("UpdateEntityDebugger", 2, 0);
						LuaPushInt(t.charanimstate.e);
						LuaPushInt(3);
						LuaCall();
						// we will ensure when t.charanimstate.outofrange is ONE, no further logic processed
						t.entityelement[t.charanimstate.e].lua.outofrangefreeze = 1;
						t.charanimstate.outofrange = 1;
					}
				}
			}
		}
		else
		{
			// if not active, stop any fire sound
			t.ttsnd = t.charanimstate.firesoundindex;
			t.charanimstate.firesoundindex = 0;
			if (t.ttsnd > 0)
			{
				if (SoundExist(t.ttsnd) == 1)
				{
					StopSound (t.ttsnd);
				}
			}
		}

		// Handle character Animation Speed (allows for machine indie speeds with timeelapsed_f)
		if (t.charanimstate.dormant == 1)
		{
			if (t.charanimstate.obj > 0 && ObjectExist(t.charanimstate.obj) == 1)
			{
				// some characters have 'moving anims', so freeze these while dormant (setting speed to zero avoids interfering with anim choice and frame)
				SetObjectSpeed (t.charanimstate.obj, 0);
			}
		}
		else
		{
			float fPolarity = 1.0f;
			if (t.charanimstate.animationspeed_f >= 0.0f)
			{
				// only use reverse polarity if animationspeed is NOT negative
				if (GetSpeed(t.charanimstate.obj) < 0) fPolarity = -1; else fPolarity = 1;
			}
			t.tfinalspeed_f = t.entityelement[t.charanimstate.e].speedmodulator_f * t.charanimstate.animationspeed_f * fPolarity * 2.5f;
			SetObjectSpeed (t.charanimstate.obj, t.tfinalspeed_f);
		}

		// smoothing animations for this character
		char_loop();

		//  Store any changes
		t.charanimstates[g.charanimindex] = t.charanimstate;
	}

	// manage AI sound events
	darkai_managesound();
}

void darkai_update (void)
{
}

void darkai_setupcharacter (void)
{
	// Entity profile index
	t.ttentid = t.entityelement[t.charanimstates[g.charanimindex].e].bankindex;

	// Setup character defaults
	t.charanimstates[g.charanimindex].realheadangley_f = 0.0;
	t.charanimstates[g.charanimindex].animationspeed_f = (65.0 / 100.0)*t.entityelement[t.charanimstates[g.charanimindex].e].eleprof.animspeed;
	t.charanimstates[g.charanimindex].outofrange = 0;
	// sometimes want characters instantly aware, no dormancy
	if (t.entityelement[t.charanimstates[g.charanimindex].e].eleprof.phyalways == 1)
	{
		// 150723 - character aware instantly when level starts and Always Active is YES
		t.charanimstates[g.charanimindex].dormant = 0;
	}
	else
	{
		// character is dormant, and requires player line of sight to awaken
		t.charanimstates[g.charanimindex].dormant = 1;
		t.charanimstates[g.charanimindex].dormanttimer = timeGetTime() + (rand() % 1000);
	}
	t.charanimstates[g.charanimindex].warmupcharacteratstart_f = 2.0f;
	t.charanimstates[g.charanimindex].currentangle_f = t.entityelement[t.charanimstates[g.charanimindex].e].ry;
	t.charanimstates[g.charanimindex].moveangle_f = t.charanimstates[g.charanimindex].currentangle_f;
	t.charanimstates[g.charanimindex].moveToMode = 0;
	t.charanimstates[g.charanimindex].movetox_f = -1;
	t.charanimstates[g.charanimindex].movetoy_f = -1;
	t.charanimstates[g.charanimindex].movetoz_f = -1;
	t.charanimstates[g.charanimindex].iTiltMode = 0;
	t.charanimstates[g.charanimindex].iStopFromEnd = 10;	
	t.charanimstates[g.charanimindex].entityTarget = 0;
	t.charanimstates[g.charanimindex].entityTargetYOffset_f = 0;
	t.charanimstates[g.charanimindex].neckAiming = 0.0f;
	t.charanimstates[g.charanimindex].spineAiming = 0.0f;
	t.charanimstates[g.charanimindex].iRotationAlongPathMode = 100;

	// movement and turn speeds (taken from entity properties in scenarios where start/move path not controlling move and turn speeds)
	t.charanimstates[g.charanimindex].movingbackward = 0;
	t.charanimstates[g.charanimindex].requiremovementnow = 0;
	t.charanimstates[g.charanimindex].movespeed_f = 1.0f;
	t.charanimstates[g.charanimindex].turnspeed_f = 10.0f;
	t.charanimstates[g.charanimindex].movespeed_f = (float)t.entityelement[t.charanimstates[g.charanimindex].e].eleprof.iMoveSpeed / 100.0f;
	t.charanimstates[g.charanimindex].turnspeed_f = (float)t.entityelement[t.charanimstates[g.charanimindex].e].eleprof.iTurnSpeed / 100.0f;

	// setup head and spine tracker details	
	t.charanimstates[g.charanimindex].neckRightAndLeftLimit = t.entityprofile[t.ttentid].headspinetracker.headhlimit;
	t.charanimstates[g.charanimindex].neckRightAndLeftOffset = t.entityprofile[t.ttentid].headspinetracker.headhoffset;
	t.charanimstates[g.charanimindex].neckUpAndDownLimit = t.entityprofile[t.ttentid].headspinetracker.headvlimit;
	t.charanimstates[g.charanimindex].neckUpAndDownOffset = t.entityprofile[t.ttentid].headspinetracker.headvoffset;
	t.charanimstates[g.charanimindex].spineRightAndLeftLimit = t.entityprofile[t.ttentid].headspinetracker.spinehlimit;
	t.charanimstates[g.charanimindex].spineRightAndLeftOffset = t.entityprofile[t.ttentid].headspinetracker.spinehoffset;
	t.charanimstates[g.charanimindex].spineUpAndDownLimit = t.entityprofile[t.ttentid].headspinetracker.spinevlimit;
	t.charanimstates[g.charanimindex].spineUpAndDownOffset = t.entityprofile[t.ttentid].headspinetracker.spinevoffset;

	// By default, characters have default PISTOL weapon style OR new WEAPSTYLE value
	t.tgunid = t.entityelement[t.charanimstates[g.charanimindex].e].eleprof.hasweapon;
	if (t.entityprofile[t.ttentid].usesweapstyleanims == 0)
	{
		if (t.tgunid > 0)
		{
			t.charanimstates[g.charanimindex].weapstyle = 1;
		}
		else
		{
			t.charanimstates[g.charanimindex].weapstyle = 0;
		}
	}
	else
	{
		//  1-pistol, 2-rocket, 3-shotgun, 4-uzi, 5-assault, 51-meleenoammo
		t.charanimstates[g.charanimindex].weapstyle = t.gun[t.tgunid].weapontype;
		if (t.charanimstates[g.charanimindex].weapstyle > 5) t.charanimstates[g.charanimindex].weapstyle = 1;
	}

	// populate character with weapon details
	t.tgunid = t.entityelement[t.charanimstates[g.charanimindex].e].eleprof.hasweapon;
	t.charanimstates[g.charanimindex].ammoinclipmax = g.firemodes[t.tgunid][0].settings.reloadqty;
	t.charanimstates[g.charanimindex].ammoinclip = t.charanimstates[g.charanimindex].ammoinclipmax;
	if (t.charanimstates[g.charanimindex].ammoinclip > 0)
	{
		// allows characters to reload at different times
		t.charanimstates[g.charanimindex].ammoinclip = 1 + Rnd(t.charanimstates[g.charanimindex].ammoinclip - 1);
	}

	// Set collision property
	SetObjectCollisionProperty (t.charanimstates[g.charanimindex].obj, 0);

	// Set special apparent size culling exception
	sObject* pObject = GetObjectData(t.charanimstates[g.charanimindex].obj);
	WickedCall_SetObjectPreventAnyApparentOcclusion(pObject,true);

	// determine if character holds 'gun' or 'rocket' style weapon
	if (t.charanimstates[g.charanimindex].weapstyle <= 1)
	{
		//  only if older legacy character (newer Uber characters use weapstyle=2)
		t.charanimstates[g.charanimindex].rocketstyle = 0;
		if (t.tgunid > 0)
		{
			if (g.firemodes[t.tgunid][0].settings.flakindex > 0)
			{
				t.charanimstates[g.charanimindex].rocketstyle = 1;
			}
		}
	}

	//  Hard-Code ENEMY or NEUTRAL in entity profile (can be changed via LUA scripting)
	t.charanimstates[g.charanimindex].aiobjectexists = 1;

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
	t.tte = t.charanimstates[g.charanimindex].e; entity_converttoclone ();
	t.obj = iStoreOBJ; t.tentid = iStoreENTID; t.tte = iStoreTTE;

	// find neck bone for this base body model (can shift index)
	int iNeckBone = 0;
	PerformCheckListForLimbs(t.charanimstates[g.charanimindex].obj);
	for (int c = 1; c <= ChecklistQuantity(); c++)
		if (iNeckBone == 0 && (strstr (ChecklistString (c), "_Head") != NULL || strstr (ChecklistString (c), "_head") != NULL))
			iNeckBone = c - 1;
	t.charanimstates[g.charanimindex].ccpo.settings.iNeckBone = iNeckBone;

	// Create cone of sight debug object for character
	int e = t.charanimstates[g.charanimindex].e;
	float fConeAngle = t.entityelement[e].eleprof.coneangle;
	float fViewRange = t.entityelement[e].eleprof.conerange;
	#ifdef NEWMAXAISYSTEM
	int iDebugConeOfSightObj = g.debugconeofsightstart + g.charanimindex;
	if (iDebugConeOfSightObj < g.debugconeofsightfinish)
	{
		darkai_createinternaldebugvisuals_coneofsight (iDebugConeOfSightObj, fConeAngle, fViewRange);
	}
	#endif

	// always recreated as a regular character
	t.entityelement[t.charanimstates[g.charanimindex].e].eleprof.disableascharacter = 0;
}

void darkai_refresh_characters ( bool bScanForNewlySpawned )
{
	// required during level init and when new entities are spawned live
	for (t.e = 1; t.e <= g.entityelementlist; t.e++)
	{
		t.entid = t.entityelement[t.e].bankindex;
		if (t.entid > 0)
		{
			if (t.entityprofile[t.entid].ischaracter == 1 && t.entityelement[t.e].ragdollified == 0)
			{
				t.tobj = t.entityelement[t.e].obj;
				if (t.tobj > 0)
				{
					if (ObjectExist(t.tobj) == 1)
					{
						// but only if not already part of the char anim list
						bool bFound = false;
						for (int n = 1; n <= g.charanimindexmax; n++)
						{
							if (t.charanimstates[n].e == t.e)
							{
								bFound = true;
							}
						}
						if (bFound == false)
						{
							// and also check if a free one exists that was once used for same
							bool bCanReuse = false;
							for (int n = 1; n <= g.charanimindexmax; n++)
							{
								if (t.charanimstates[n].e == 0 && t.charanimstates[n].originale == t.e)
								{
									g.charanimindex = n;
									bCanReuse = true;
									break;
								}
							}
							if (bCanReuse == false)
							{
								// Set up object one as character
								++g.charanimindexmax;
								g.charanimindex = g.charanimindexmax;
								Dim (t.charanimcontrols, g.charanimindexmax);
								Dim (t.charanimstates, g.charanimindexmax);
								Dim2(t.charactergunpose, g.charanimindexmax, 36);
								t.charanimstates[g.charanimindex].originale = t.e;
							}
							t.charanimstates[g.charanimindex].obj = t.tobj;
							t.charanimstates[g.charanimindex].e = t.e;
							t.charanimstates[g.charanimindex].currentangle_f = t.entityelement[t.e].ry;
							t.entityelement[t.e].eleprof.disableascharacter = 0;
							darkai_setupcharacter ();
							for (t.i = 0; t.i <= 36; t.i++)
							{
								t.charactergunpose[g.charanimindex][t.i].x = 0;
								t.charactergunpose[g.charanimindex][t.i].y = 0;
								t.charactergunpose[g.charanimindex][t.i].z = 0;
							}
						}
						else
						{
							// if found, existing characters can be left along during a newly spawned scam
							if (bScanForNewlySpawned == true)
							{
								continue;
							}
						}

						// swap in animation override
						char pWeaponAnimFile[MAX_PATH];
						strcpy(pWeaponAnimFile, "");
						LPSTR pOverrideAnimSet = t.entityelement[t.e].eleprof.overrideanimset_s.Get();
						if (strlen(pOverrideAnimSet) > 1) // "" = default to weapon type, "-" = default to object anim
						{
							if (FileExist(pOverrideAnimSet))
								strcpy(pWeaponAnimFile, pOverrideAnimSet);
							else
								pOverrideAnimSet = NULL;
						}
						else
						{
							pOverrideAnimSet = NULL;
						}

						// force animation to weapon type if NOT "-" = default to object anim
						if (pOverrideAnimSet == NULL && strlen(t.entityelement[t.e].eleprof.overrideanimset_s.Get()) == 0)
						{
							// swap in animation for character base types if needed
							LPSTR animsystem_getweapontype (LPSTR, LPSTR);
							int gunid = t.entityelement[t.e].eleprof.hasweapon;
							LPSTR pWeaponHeld = animsystem_getweapontype(t.entityelement[t.e].eleprof.hasweapon_s.Get(), t.gun[gunid].animsetoverride.Get());
							LPSTR pGender = NULL;
							if (t.entityprofile[t.entid].characterbasetype == 0) pGender = "adult male";
							if (t.entityprofile[t.entid].characterbasetype == 1) pGender = "adult female";
							if (t.entityprofile[t.entid].characterbasetype == 2) pGender = "zombie male";
							if (t.entityprofile[t.entid].characterbasetype == 3) pGender = "zombie female";
							if (pGender == NULL)
							{
								if (t.entityprofile[t.entid].characterbasetype < g_CharacterType.size())
								{
									pGender = g_CharacterType[t.entityprofile[t.entid].characterbasetype].pPartsFolder;
								}
							}
							if (pGender != NULL)
							{
								if (t.entityprofile[t.entid].characterbasetype >= 0 && t.entityprofile[t.entid].characterbasetype <= 1)
								{
									sprintf(pWeaponAnimFile, "charactercreatorplus\\animations\\sets\\%s\\default animations%s.dbo", pGender, pWeaponHeld);
								}
							}
						}
						if (FileExist(pWeaponAnimFile))
						{
							// replace actual object animations
							sObject* pObject = GetObjectData(t.tobj);
							AppendObject(pWeaponAnimFile, t.tobj, 0);
							WickedCall_RefreshObjectAnimations(pObject, pObject->wickedloaderstateptr);
						}
					}
				}
			}
		}
	}
}

void darkai_setup_characters (void)
{
	// for MAX, we fully reset any previous character states so this can be fresh
	for (int n = 1; n <= g.charanimindexmax; n++)
	{
		t.charanimstates[n].e = 0;
	}
	g.charanimindexmax = 0;

	// Create A.I entities for all characters
	darkai_refresh_characters(false);
}

void darkai_killai (void)
{
	if (t.charanimstates[t.tcharanimindex].aiobjectexists == 1)
	{
		// free this AI from the game loop
		t.charanimstates[t.tcharanimindex].aiobjectexists = 0;
		if (t.entityelement[t.charanimstates[t.tcharanimindex].e].usingphysicsnow != 0)
		{
			t.tphyobj = t.charanimstates[t.tcharanimindex].obj; physics_disableobject ();
			t.entityelement[t.charanimstates[t.tcharanimindex].e].usingphysicsnow = 0;
		}
		SetObjectCollisionProperty (t.charanimstates[t.tcharanimindex].obj, 1);
	}

	// reset any limbs of character
	if (t.entityelement[t.charanimstates[t.tcharanimindex].e].health > 0)
	{
		t.headlimbofcharacter = t.entityprofile[t.entityelement[t.charanimstates[t.tcharanimindex].e].bankindex].headlimb;
		if (t.headlimbofcharacter > 0)
		{
			if (LimbExist(t.charanimstates[t.tcharanimindex].obj, t.headlimbofcharacter) == 1)
			{
				RotateLimb (t.charanimstates[t.tcharanimindex].obj, t.headlimbofcharacter, 0, 0, 0);
			}
		}
		t.spinelimbofcharacter = t.entityprofile[t.entityelement[t.charanimstates[t.tcharanimindex].e].bankindex].spine;
		if (t.spinelimbofcharacter > 0)
		{
			if (LimbExist(t.charanimstates[t.tcharanimindex].obj, t.spinelimbofcharacter) == 1)
			{
				RotateLimb (t.charanimstates[t.tcharanimindex].obj, t.spinelimbofcharacter, 0, 0, 0);
			}
		}
	}

	// stop any animations (in case we need to ragdoll)
	StopObject (t.charanimstates[t.tcharanimindex].obj);

	// reset any looping/sounds
	t.ttsnd = t.charanimstates[t.tcharanimindex].firesoundindex;
	t.charanimstates[t.tcharanimindex].firesoundindex = 0;
	if (t.ttsnd > 0)
	{
		if (SoundExist(t.ttsnd) == 1)
		{
			StopSound (t.ttsnd);
		}
	}
	// additionally any sounds triggered by this entity
	int e = t.charanimstates[t.tcharanimindex].e;
	for (int s = 0; s <= 6; s++)
	{
		int ttsnd = 0;
		if (s == 0) ttsnd = t.entityelement[e].soundset;
		if (s == 1) ttsnd = t.entityelement[e].soundset2;
		if (s == 2) ttsnd = t.entityelement[e].soundset3;
		if (s == 4) ttsnd = t.entityelement[e].soundset5;
		if (s == 5) ttsnd = t.entityelement[e].soundset6;
		if (ttsnd > 0)
		{
			if (SoundExist(ttsnd) == 1)
			{
				if (i_LastExplosionSoundID != ttsnd)
				{
					StopSound(ttsnd);
				}
				else
				{
					i_LastExplosionSoundID = 0;
				}
			}
		}
	}

}

// Smooth Anim System

void char_init (void)
{
	// Create array to hold transition information for per-object
	t.tmaxobjectnumber = 90000;
	Dim (t.smoothanim, t.tmaxobjectnumber);
}

void char_loop (void)
{
	// Update anim system for smoothing (or transitions if wicked)
	smoothanimupdate (t.charanimstate.obj);

	// Ensure object can be smoothly rotated from LUA instruction
	if (t.charanimstate.currentangleslowlyspeed_f != 0.0f)
	{
		// preserve t.e and t.obj
		int ste = t.e, stobj = t.obj;
		t.e = t.charanimstate.e;

		// get dest angle
		float fDestAngle = t.charanimstate.currentangle_f;

		// need to factor in entity speed for characters
		t.tsmooth_f = (100.0 / (t.charanimstate.currentangleslowlyspeed_f)) / g.timeelapsed_f;
		t.tsmooth_f /= (t.entityelement[t.charanimstate.e].eleprof.speed / 100.0f);

		// smoothly rotate to destination angle and update object
		t.entityelement[t.e].ry = CurveAngle(fDestAngle, t.entityelement[t.e].ry, t.tsmooth_f);
		entity_lua_rotateupdate ();

		// when reach destination can close this system down
		if (fabs(fDestAngle - t.entityelement[t.e].ry) < 0.1f) t.charanimstate.currentangleslowlyspeed_f = 0.0f;

		// restore t.e
		t.e = ste; t.obj = stobj;
	}
}

void darkai_resetsmoothanims (void)
{
	for (int n = 0; n < t.tmaxobjectnumber; n++)
	{
		t.smoothanim[n].fn = 0;
		t.smoothanim[n].playflag = 0;
		t.smoothanim[n].playstarted = 0;
		t.smoothanim[n].rev = 0;
		t.smoothanim[n].st = 0;
		t.smoothanim[n].transition = 0;
		t.smoothanim[n].movedeltacontrib = 0;
		t.smoothanim[n].movedeltax = 0;
		t.smoothanim[n].movedeltaz = 0;
		t.smoothanim[n].startat = 0;
		t.smoothanim[n].usefulTimer = 0;
	}
}

void smoothanimtriggerrev (int obj, float st, float fn, int speedoftransition, int rev, int playflag, float fStartFromPercentage)
{
	// transition to the start of the loop frame
	if (t.smoothanim[obj].st != st)
	{
		StopObject (obj);
		SetObjectInterpolation (obj, speedoftransition);
		if (rev == 1)
		{
			SetObjectFrame (obj, fn);
		}
		else
		{
			SetObjectFrame (obj, st);
		}
		t.smoothanim[obj].st = st;
		t.smoothanim[obj].fn = fn;
		t.smoothanim[obj].rev = rev;
		t.smoothanim[obj].playflag = playflag;
		t.smoothanim[obj].playstarted = 0;
		// transitions handled differently with MAX, we control a lerp factor that handles transitions
		// nicely within the Wicked animation system
		t.smoothanim[obj].transition = 1;

		// affect starting frame if specified
		float fThisAnimLength = fn - st;
		t.smoothanim[obj].startat = 0;
		if (fStartFromPercentage > 0.0f)
		{
			float fStartFrame = st;
			fStartFrame += (fThisAnimLength / 100.0f)*fStartFromPercentage;
			t.smoothanim[obj].startat = fStartFrame;
		}
	}
}

void smoothanimtrigger (int obj, float st, float fn, int speedoftransition)
{
	smoothanimtriggerrev(obj, st, fn, speedoftransition, 0, 0, 0);
}

