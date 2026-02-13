bool g_bOnlyOneRagdollPlusPerCycleForPerformance = false;

void entity_loop ( void )
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif

	// so avoid ALL physics activating at once and slowing the level start,
	// we pace it out from the camera position outward
	if (g_fActivationWaveDistance < 999999)
	{
		float fSlice = 50.0f;
		g_fActivationWaveDistance = ODEProjectActivationWave (g_fActivationWaveDistance, fSlice);
	}

	//  Handle all entities in level 
	g_bOnlyOneRagdollPlusPerCycleForPerformance = true;
	for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
	{
		// 011016 - scenes with LARGE number of static entities hitting perf hard
		if ( t.entityelement[t.e].staticflag == 1 && t.entityelement[t.e].eleprof.phyalways == 0 ) continue;
		// NOTE: Determine essential tasks static needs (i.e. plrdist??)

		// only handle DYNAMIC entities
		t.entid=t.entityelement[t.e].bankindex;
		if ( t.entid>0 ) 
		{
			//  Entity object
			t.tobj=t.entityelement[t.e].obj;

			extern bool g_bShowRecastDetourDebugVisuals;
			if (t.entityprofile[t.entid].ischaracter == 1)
			{
				sObject* pObject = GetObjectData(t.tobj);
				float fObjectOriginalRotated = t.entityelement[t.e].ry;
				int iTempObjectForTest = g.ragdollplussystemdebugobj;
				if (t.entityelement[t.e].ragdollplusactivate > 0)
				{
					// ragdoll plus triggered
					if (t.entityelement[t.e].ragdollplusactivate < 10)
					{
						t.entityelement[t.e].ragdollplusactivate += 10;
					}

					// see if we can fit a death animation into the scene
					bool bDropAnyWeaponNow = false;
					bool bGoToRagdollNow = false;
					GGVECTOR3 vecMinOrig = pObject->collision.vecMin;
					GGVECTOR3 vecMaxOrig = pObject->collision.vecMax;
					GGVECTOR3 vecCenterOrig = pObject->collision.vecCentre;
					int iSpareRagdollPlusPhyObj = t.entityelement[t.e].ragdollifiedplusphyobj;
					if (iSpareRagdollPlusPhyObj == 0)
					{
						// only alow one creation per frame cycle
						if (g_bOnlyOneRagdollPlusPerCycleForPerformance == false)
							continue;

						// show available floor area according to nav mesh (simple for now, need smartypants one for any angle and area shape)
						GGVECTOR3 vecCurrentPos = GGVECTOR3(ObjectPositionX(t.tobj), ObjectPositionY(t.tobj), ObjectPositionZ(t.tobj));
						GGVECTOR3 vecNearestPt = GGVECTOR3(0, 0, 0);
						// ensure starting position is INSIDE a navmesh
						bool bBeInsideNavMesh = false;
						bool bNoSpaceBehindOrInFrontCharacterInNavMesh = false;
						while (bBeInsideNavMesh == false)
						{
							if (g_RecastDetour.isWithinNavMeshEx(vecCurrentPos.x, vecCurrentPos.y, vecCurrentPos.z, (float*)&vecNearestPt, true) == true)
							{
								// good, we are on the nav mesh
								bBeInsideNavMesh = true;

								// check if there is navmesh behind character (can choose better anim later if so)
								GGVECTOR3 vecBehindOrInFrontPos = vecCurrentPos;
								float fWhoShotMeX = CameraPositionX(0);// this should be who shot this character, not necessarily the player!!
								float fWhoShotMeZ = CameraPositionZ(0);// this should be who shot this character, not necessarily the player!!
								float fDX = fWhoShotMeX - vecCurrentPos.x;
								float fDZ = fWhoShotMeZ - vecCurrentPos.z;
								float fDA = atan2(fDX, fDZ);
								for (float fTraceOutward = 5.0f; fTraceOutward < 40.0f; fTraceOutward += 5.0f)
								{
									if (bNoSpaceBehindOrInFrontCharacterInNavMesh == false)
									{
										float fDistanceToStepCheckBack = -fTraceOutward; // behind
										vecBehindOrInFrontPos.x = NewXValue(vecCurrentPos.x, GGToDegree(fDA), fDistanceToStepCheckBack);
										vecBehindOrInFrontPos.z = NewZValue(vecCurrentPos.z, GGToDegree(fDA), fDistanceToStepCheckBack);
										if (g_RecastDetour.isWithinNavMeshEx(vecBehindOrInFrontPos.x, vecBehindOrInFrontPos.y, vecBehindOrInFrontPos.z, (float*)&vecNearestPt, true) == false)
										{
											// seems behind the character is NO navmesh!
											bNoSpaceBehindOrInFrontCharacterInNavMesh = true;
										}
										fDistanceToStepCheckBack = fTraceOutward; // in front
										vecBehindOrInFrontPos.x = NewXValue(vecCurrentPos.x, GGToDegree(fDA), fDistanceToStepCheckBack);
										vecBehindOrInFrontPos.z = NewZValue(vecCurrentPos.z, GGToDegree(fDA), fDistanceToStepCheckBack);
										if (g_RecastDetour.isWithinNavMeshEx(vecBehindOrInFrontPos.x, vecBehindOrInFrontPos.y, vecBehindOrInFrontPos.z, (float*)&vecNearestPt, true) == false)
										{
											// seems behind the character is NO navmesh!
											bNoSpaceBehindOrInFrontCharacterInNavMesh = true;
										}
									}
								}
							}
							else
							{
								// if out of navmesh, we can assume obstacles abound!!
								bNoSpaceBehindOrInFrontCharacterInNavMesh = true;

								// not exactly on nav mesh, are we near one
								if (g_RecastDetour.isWithinNavMesh(vecCurrentPos.x, vecCurrentPos.y, vecCurrentPos.z) == true)
								{
									// move current starting pos for expanding box to point inside nav mesh
									GGVECTOR3 vecDir = vecNearestPt - vecCurrentPos;
									GGVec3Normalize(&vecDir, &vecDir);
									float fMargin = 11.0f; // pushes well into the nav mesh which must account for character breadth
									vecCurrentPos = vecNearestPt + (vecDir * fMargin);
									bBeInsideNavMesh = true;
								}
								else
								{
									// no nav mesh, so no restrictions
									break;
								}
							}
						}
						GGMATRIX matRotY;
						GGMatrixRotationY(&matRotY, GGToRadian(-fObjectOriginalRotated));
						GGMATRIX matInvRotY;
						GGMatrixRotationY(&matInvRotY, GGToRadian(fObjectOriginalRotated));
						GGVECTOR3 vecNavMeshMin = vecCurrentPos;
						GGVECTOR3 vecNavMeshMax = vecCurrentPos;
						if (bBeInsideNavMesh == true)
						{
							// first find best center position to start box growth
							int iBestSpoke = -1;
							float fSpokeDist = 0.0f;
							GGVECTOR3 vecNewPosAlongSpoke = GGVECTOR3(0, 0, 0);
							for (int iSpokes = 0; iSpokes < 8; iSpokes++)
							{
								float fAngle = iSpokes * 45.0f;
								float fDestX = NewXValue(0, fAngle, 1.0f);
								float fDestZ = NewZValue(0, fAngle, 1.0f);
								GGVECTOR3 vecAt = vecCurrentPos;
								int iStepExtent = 20;// 100;
								for (int iStep = 0; iStep < iStepExtent; iStep++)
								{
									vecAt.x += fDestX;
									vecAt.z += fDestZ;
									bool bWeCanHaveThisNow = false;
									if (g_RecastDetour.isWithinNavMeshEx(vecAt.x, vecAt.y, vecAt.z, (float*)&vecNearestPt, true) == false)
									{
										// left nav mesh, stop here
										if (iStep > fSpokeDist)
										{
											bWeCanHaveThisNow = true;
										}
										else
										{
											break;
										}
									}
									if (iStep >= (iStepExtent - 2)) bWeCanHaveThisNow = true;
									if (bWeCanHaveThisNow == true)
									{
										iBestSpoke = iSpokes;
										fSpokeDist = iStep;
										vecNewPosAlongSpoke = vecCurrentPos + ((vecAt - vecCurrentPos) / 2);
										break;
									}
								}
							}
							if (iBestSpoke >= 0)
							{
								// new best current position for ideal box expansion
								vecCurrentPos = vecNewPosAlongSpoke;
							}
							// expand box to find max size area here (account for fObjectOriginalRotated)
							float fFoundAreaSizeX, fFoundAreaSizeZ;
							GGVECTOR3 vecLocaMin, vecLocaMax;
							for (int iAlignModeTry = 0; iAlignModeTry < 2; iAlignModeTry++)
							{
								vecLocaMin = GGVECTOR3(0, 0, 0);
								vecLocaMax = GGVECTOR3(0, 0, 0);
								bool bExpanding = true;
								bool bExpandingMinX = true;
								bool bExpandingMaxX = true;
								bool bExpandingMinZ = true;
								bool bExpandingMaxZ = true;
								float fStep = 2.0f;
								while (bExpanding)
								{
									if (bExpandingMinX == true)
									{
										for (int stepz = vecLocaMin.z; stepz <= vecLocaMax.z; stepz += 5)
										{
											GGVECTOR3 vecWorld = GGVECTOR3(vecLocaMin.x - fStep, vecLocaMin.y, stepz);
											GGVec3TransformCoord(&vecWorld, &vecWorld, &matInvRotY);
											vecWorld += vecCurrentPos;
											if (g_RecastDetour.isWithinNavMeshEx(vecWorld.x, vecWorld.y, vecWorld.z, (float*)&vecNearestPt, true) == false)
											{
												bExpandingMinX = false;
											}
										}
										if (bExpandingMinX == true) vecLocaMin.x -= fStep;
									}
									if (bExpandingMinZ == true)
									{
										for (int stepx = vecLocaMin.x; stepx <= vecLocaMax.x; stepx += 5)
										{
											GGVECTOR3 vecWorld = GGVECTOR3(stepx, vecLocaMin.y, vecLocaMin.z - fStep);
											GGVec3TransformCoord(&vecWorld, &vecWorld, &matInvRotY);
											vecWorld += vecCurrentPos;
											if (g_RecastDetour.isWithinNavMeshEx(vecWorld.x, vecWorld.y, vecWorld.z, (float*)&vecNearestPt, true) == false)
											{
												bExpandingMinZ = false;
											}
										}
										if (bExpandingMinZ == true) vecLocaMin.z -= fStep;
									}
									if (bExpandingMaxX == true)
									{
										for (int stepz = vecLocaMin.z; stepz <= vecLocaMax.z; stepz += 5)
										{
											GGVECTOR3 vecWorld = GGVECTOR3(vecLocaMax.x + fStep, vecLocaMin.y, stepz);
											GGVec3TransformCoord(&vecWorld, &vecWorld, &matInvRotY);
											vecWorld += vecCurrentPos;
											if (g_RecastDetour.isWithinNavMeshEx(vecWorld.x, vecWorld.y, vecWorld.z, (float*)&vecNearestPt, true) == false)
											{
												bExpandingMaxX = false;
											}
										}
										if (bExpandingMaxX == true) vecLocaMax.x += fStep;
									}
									if (bExpandingMaxZ == true)
									{
										for (int stepx = vecLocaMin.x; stepx <= vecLocaMax.x; stepx += 5)
										{
											GGVECTOR3 vecWorld = GGVECTOR3(stepx, vecLocaMin.y, vecLocaMax.z + fStep);
											GGVec3TransformCoord(&vecWorld, &vecWorld, &matInvRotY);
											vecWorld += vecCurrentPos;
											if (g_RecastDetour.isWithinNavMeshEx(vecWorld.x, vecWorld.y, vecWorld.z, (float*)&vecNearestPt, true) == false)
											{
												bExpandingMaxZ = false;
											}
										}
										if (bExpandingMaxZ == true) vecLocaMax.z += fStep;
									}
									fFoundAreaSizeX = vecLocaMax.x - vecLocaMin.x;
									fFoundAreaSizeZ = vecLocaMax.z - vecLocaMin.z;
									if (fFoundAreaSizeX >= 100.0f) { bExpandingMinX = false; bExpandingMaxX = false; }
									if (fFoundAreaSizeZ >= 100.0f) { bExpandingMinZ = false; bExpandingMaxZ = false; }
									if (bExpandingMinX == false && bExpandingMaxX == false && bExpandingMinZ == false && bExpandingMaxZ == false)
										bExpanding = false;
								}
								if (fabs(fFoundAreaSizeX + fFoundAreaSizeZ) < 100.0f)
								{
									// means this volume of floor is less than the needed area, try again with an aligned rotation
									fObjectOriginalRotated = (int)((fObjectOriginalRotated + 45.0f) / 90.0f);
									fObjectOriginalRotated = (fObjectOriginalRotated * 90.0f);
									GGMatrixRotationY(&matRotY, GGToRadian(-fObjectOriginalRotated));
									GGMatrixRotationY(&matInvRotY, GGToRadian(fObjectOriginalRotated));
								}
								else
								{
									// happy with this floor size
									break;
								}
							}

							// creating objects is a performance hit, can have specific fall areas when
							// this can be resolved
							bool bSpecificFallAreaSizes = false;
							if (bSpecificFallAreaSizes == true)
							{
								// specific fall area
								if (ObjectExist(iTempObjectForTest) == 1)
								{
									DeleteObject(iTempObjectForTest);
								}
								MakeObjectBox (iTempObjectForTest, fFoundAreaSizeX, 5, fFoundAreaSizeZ);
							}
							else
							{
								// fixed fall area
								fFoundAreaSizeX = 80;
								fFoundAreaSizeZ = 80;
								if (ObjectExist(iTempObjectForTest) == 0)
								{
									MakeObjectBox (iTempObjectForTest, fFoundAreaSizeX, 5, fFoundAreaSizeZ);
									HideObject(iTempObjectForTest);
								}
							}

							GGVECTOR3 vecCenter = (vecLocaMax - vecLocaMin) / 2;
							GGVECTOR3 vecWorld = vecLocaMin + vecCenter;
							GGVec3TransformCoord(&vecWorld, &vecWorld, &matInvRotY);
							vecWorld += vecCurrentPos;
							vecNavMeshMin = vecLocaMin + vecCurrentPos; // we use these unrotated for 'within box calc' :)
							vecNavMeshMax = vecLocaMax + vecCurrentPos;
							PositionObject (iTempObjectForTest, vecWorld.x, ObjectPositionY(t.tobj) + 2.5f, vecWorld.z);
							RotateObject(iTempObjectForTest, 0, fObjectOriginalRotated, 0);
							if (g_bShowRecastDetourDebugVisuals == false)
							{
								HideObject(iTempObjectForTest);
							}
						}
						// create death database
						struct sDeathListType
						{
							float fScore;
							cStr pName;
							float iStartFrame;
							float iEndFrame;
						};
						std::vector<sDeathListType> pDeathList;
						pDeathList.clear();
						if (pObject->pAnimationSet)
						{
							// get animation slot data
							extern std::vector<sAnimSlotStruct> g_pAnimSlotList;
							g_pAnimSlotList.clear();
							animsystem_buildanimslots(t.tobj);

							// go through all animations
							for (int slot = 0; slot < g_pAnimSlotList.size(); slot++)
							{
								LPSTR pName = g_pAnimSlotList[slot].pName;
								if (pName && strnicmp(pName, "death", 5) == NULL)
								{
									// death anim found, add to list
									sDeathListType item;
									item.pName = Lower(pName);
									item.iStartFrame = g_pAnimSlotList[slot].fStart;
									item.iEndFrame = g_pAnimSlotList[slot].fFinish;
									pDeathList.push_back(item);
								}
							}
						}
						// sort database by preference and reject early choices
						LPSTR pPreferredKeyword = "front";
						if (t.entityelement[t.e].ragdollplusactivate == 11) pPreferredKeyword = "front";
						if (t.entityelement[t.e].ragdollplusactivate == 12) pPreferredKeyword = "back";
						if (t.entityelement[t.e].ragdollplusactivate == 13) pPreferredKeyword = "right";
						if (t.entityelement[t.e].ragdollplusactivate == 14) pPreferredKeyword = "left";
						if (t.entityelement[t.e].ragdollplusactivate == 15) pPreferredKeyword = "back";
						if (t.entityelement[t.e].ragdollplusactivate == 16) pPreferredKeyword = "";
						// if detect wall in front or behind character and BACK used, specify wall specific
						if (t.entityelement[t.e].ragdollplusactivate == 11)
						{
							// scan to see if a solid obstacle behind
							if (bNoSpaceBehindOrInFrontCharacterInNavMesh == true)
							{
								// choose the special back animation!
								pPreferredKeyword = "front_fall_wall";
							}
						}
						if (t.entityelement[t.e].ragdollplusactivate == 12 || t.entityelement[t.e].ragdollplusactivate == 15)
						{
							// scan to see if a solid obstacle behind
							if (bNoSpaceBehindOrInFrontCharacterInNavMesh == true)
							{
								// choose the special back animation!
								pPreferredKeyword = "back_fall_wall";
							}
							else
							{
								// if killed by shotgun type, and going back, special force back preferred
								if (t.entityelement[t.e].ragdollplusweapontypeused == 2) // 1- pierce, 2-shotgun shell
								{
									pPreferredKeyword = "back_fall_slope";
								}
							}
						}
						std::vector<sDeathListType> pDeathListPreferred;
						pDeathListPreferred.clear();
						bool pDone[32];
						memset(pDone, 0, sizeof(pDone));
						if (pDeathList.size() > 0)
						{
							for (int preferences = 0; preferences < 2; preferences++)
							{
								int iMaxDeathChoices = pDeathList.size();
								if (iMaxDeathChoices > 30) iMaxDeathChoices = 30;
								for (int deathindex = 0; deathindex < iMaxDeathChoices; deathindex++)
								{
									bool bAddToPrefList = false;
									if (preferences == 0)
									{
										// add specific ones preferred (i.e. back for when shot from the front)
										if (strstr (pDeathList[deathindex].pName.Get(), pPreferredKeyword) > 0)
										{
											bAddToPrefList = true;
										}
									}
									if (preferences == 1)
									{
										// add remaining
										bAddToPrefList = true;
									}
									if (bAddToPrefList == true && pDone[deathindex] == false)
									{
										sDeathListType item;
										item.pName = cStr(pDeathList[deathindex].pName.Get());
										item.iStartFrame = pDeathList[deathindex].iStartFrame;
										item.iEndFrame = pDeathList[deathindex].iEndFrame;
										pDeathListPreferred.push_back(item);
										pDone[deathindex] = true;
									}
								}
							}
						}

						// now choose a shape that represents the final resting place of the death anim
						float fBestScore = -1.0f;
						int iPreferredAnimChosen = -1;
						GGVECTOR3 vecBestSize = GGVECTOR3(0, 0, 0);
						GGVECTOR3 vecBestCenterOffset = GGVECTOR3(0, 0, 0);
						GGVECTOR3 vecBestShiftInObjectSpace = GGVECTOR3(0, 0, 0);
						GGVECTOR3 vecObjPos = GGVECTOR3(ObjectPositionX(t.tobj), ObjectPositionY(t.tobj), ObjectPositionZ(t.tobj));
						GGVECTOR3 vecNavMeshFloorSize = vecNavMeshMax - vecNavMeshMin;
						if (strlen(pPreferredKeyword) > 0)
						{
							for (int preflist = 0; preflist < pDeathListPreferred.size(); preflist++)
							{
								// get a death choice
								float fFinalFrameOfChosenDeath = pDeathListPreferred[preflist].iEndFrame;
								// work out how this fits as a shape into floor area
								float fBiasTowardsX = (vecNavMeshFloorSize.x / vecNavMeshFloorSize.z);
								float fBiasTowardsZ = (vecNavMeshFloorSize.z / vecNavMeshFloorSize.x);
								CalculateObjectFrameBounds(t.tobj, fFinalFrameOfChosenDeath);
								GGVECTOR3 vecCenter = pObject->collision.vecCentre;
								float xx = vecCenter.x;
								float yy = vecCenter.y;
								float zz = vecCenter.z;
								GGVECTOR3 vecMin = pObject->collision.vecMin;
								GGVECTOR3 vecMax = pObject->collision.vecMax;
								// try to fit this into the floor area box
								GGVECTOR3 vecCenterOffset = GGVECTOR3(xx, yy, zz);
								GGVECTOR3 vecSize = vecMax - vecMin;
								GGVECTOR3 vecTopLeft = vecCenterOffset - (vecSize / 2);
								GGVECTOR3 vecTopLeftOriginal = vecTopLeft;
								GGVECTOR3 vecDiff = vecObjPos - vecCurrentPos;
								GGVec3TransformCoord(&vecDiff, &vecDiff, &matRotY);
								float fDistToCreep = 200; //floor area box can be quite some way off in cramped spots!!
								for (int iEachSide = 0; iEachSide < 4; iEachSide++)
								{
									for (int iStep = 0; iStep < fDistToCreep; iStep++)
									{
										if (iEachSide == 0)
										{
											vecTopLeft.x += 1;
											GGVECTOR3 vecUnrotatedWorld = vecCurrentPos + vecDiff + vecTopLeft;
											if (vecUnrotatedWorld.x >= vecNavMeshMin.x)
											{
												// now inside on left X
												break;
											}
										}
										if (iEachSide == 1)
										{
											vecTopLeft.z += 1;
											GGVECTOR3 vecUnrotatedWorld = vecCurrentPos + vecDiff + vecTopLeft;
											if (vecUnrotatedWorld.z >= vecNavMeshMin.z)
											{
												// now inside on top Z
												break;
											}
										}
										if (iEachSide == 2)
										{
											vecTopLeft.x -= 1;
											GGVECTOR3 vecUnrotatedWorld = vecCurrentPos + vecDiff + vecTopLeft + vecSize;
											if (vecUnrotatedWorld.x <= vecNavMeshMax.x)
											{
												// now inside on right X
												break;
											}
										}
										if (iEachSide == 3)
										{
											vecTopLeft.z -= 1;
											GGVECTOR3 vecUnrotatedWorld = vecCurrentPos + vecDiff + vecTopLeft + vecSize;
											if (vecUnrotatedWorld.z <= vecNavMeshMax.z)
											{
												// now inside on bottom Z
												break;
											}
										}
									}
								}
								// give the final position of the shape as a score to use later
								float fScore = 0.0f;
								GGVECTOR3 vecUnrotatedShapeWorld1 = vecCurrentPos + vecDiff + vecTopLeft;
								GGVECTOR3 vecUnrotatedShapeWorld2 = vecCurrentPos + vecDiff + vecTopLeft + vecSize;
								if (vecUnrotatedShapeWorld1.x >= vecNavMeshMin.x)
								{
									fScore += 1 * fBiasTowardsZ;
								}
								else
								{
									fScore -= (((vecNavMeshMin.x - vecUnrotatedShapeWorld1.x) / vecSize.x) * fBiasTowardsZ);
								}
								if (vecUnrotatedShapeWorld2.x <= vecNavMeshMax.x)
								{
									fScore += 1 * fBiasTowardsZ;
								}
								else
								{
									fScore -= (((vecUnrotatedShapeWorld2.x - vecNavMeshMax.x) / vecSize.x) * fBiasTowardsZ);
								}
								if (vecUnrotatedShapeWorld1.z >= vecNavMeshMin.z)
								{
									fScore += 1 * fBiasTowardsX;
								}
								else
								{
									fScore -= (((vecNavMeshMin.z - vecUnrotatedShapeWorld1.z) / vecSize.z) * fBiasTowardsX);
								}
								if (vecUnrotatedShapeWorld2.z <= vecNavMeshMax.z)
								{
									fScore += 1 * fBiasTowardsX;
								}
								else
								{
									fScore -= (((vecUnrotatedShapeWorld2.z - vecNavMeshMax.z) / vecSize.z) * fBiasTowardsX);
								}
								if (fScore > fBestScore)
								{
									fBestScore = fScore;
									vecBestSize = vecSize;
									vecBestShiftInObjectSpace = vecCenterOffset + (vecTopLeft - vecTopLeftOriginal);
									vecBestCenterOffset = vecCenterOffset;
									iPreferredAnimChosen = preflist;
								}
								pDeathListPreferred[preflist].fScore = fScore;
							}
						}
						if (iPreferredAnimChosen >= 0)
						{
							float fThresholdForMovingTooFarX = 22.5f;
							float fThresholdForMovingTooFarZ = 25.0f;
							if (fabs(vecBestShiftInObjectSpace.x) > fThresholdForMovingTooFarX || fabs(vecBestShiftInObjectSpace.z) > fThresholdForMovingTooFarZ)
							{
								iPreferredAnimChosen = -1;
							}
						}
						if (iPreferredAnimChosen >= 0)
						{
							// if no floor area truncation, can use preferred anim
							if (vecNavMeshFloorSize.x == 100 && vecNavMeshFloorSize.z == 100)
							{
								// no worries about space, so can have ideal animation based on shot trajectory
								if (bNoSpaceBehindOrInFrontCharacterInNavMesh == true)
									iPreferredAnimChosen = 0;
								else
									iPreferredAnimChosen = rand() % 3;
							}
							// ensure size is not bigger than detected floor (so no silly physicsness)
							if (vecBestSize.x > vecNavMeshFloorSize.x) vecBestSize.x = vecNavMeshFloorSize.x;
							if (vecBestSize.y > 20.0f) vecBestSize.y = 20.0f;
							if (vecBestSize.z > vecNavMeshFloorSize.z) vecBestSize.z = vecNavMeshFloorSize.z;
							// find spare physics obj for ragdoll plus
							int iSpareRagdollPlusPhyObj = g.ragdollplussystemobjstart;
							for (; iSpareRagdollPlusPhyObj < g.ragdollplussystemobjfinish; iSpareRagdollPlusPhyObj++)
							{
								if (ObjectExist(iSpareRagdollPlusPhyObj) == 0)
									break;
							}
							// update new shape box bounds at new position that sits inside floor box
							if (ObjectExist(iSpareRagdollPlusPhyObj) == 1)
							{
								ODEDestroyObject(iSpareRagdollPlusPhyObj);
								DeleteObject(iSpareRagdollPlusPhyObj);
							}
							t.entityelement[t.e].ragdollifiedplusphyobj = iSpareRagdollPlusPhyObj;
							MakeObjectBox (iSpareRagdollPlusPhyObj, vecBestSize.x, vecBestSize.y, vecBestSize.z);
							GGVECTOR3 vecBestShiftInWorldSpace;
							GGVec3TransformCoord(&vecBestShiftInWorldSpace, &vecBestShiftInObjectSpace, &matInvRotY);
							vecObjPos += vecBestShiftInWorldSpace;
							float fRaiseForTheFall = 20.0f;
							PositionObject (iSpareRagdollPlusPhyObj, vecObjPos.x, vecObjPos.y + fRaiseForTheFall, vecObjPos.z);
							RotateObject(iSpareRagdollPlusPhyObj, 0, fObjectOriginalRotated, 0);
							if (g_bShowRecastDetourDebugVisuals == false)
							{
								HideObject(iSpareRagdollPlusPhyObj);
							}
							if (t.entityelement[t.e].usingphysicsnow == 1)
							{
								ODEDestroyObject(t.tobj);
								t.entityelement[t.e].usingphysicsnow = 0;
							}
							ODECreateDynamicBox(iSpareRagdollPlusPhyObj, -1, 13);
							ODESetBodyFriction(iSpareRagdollPlusPhyObj, 55);

							// completely end this character
							t.smoothanim[t.tobj].playflag = 0;
							t.entityelement[t.e].destroyme = 0;
							t.entityelement[t.e].active = 0;
							t.entityelement[t.e].health = 0;
							t.entityelement[t.e].lua.flagschanged = 2;

							// remove pivot and readjust char object (undo this when restore from test level)
							RotateLimb(t.tobj, 0, 0, 180, 0);
							ResetObjectPivot(t.tobj);

							// play the chosen animation
							PlayObject (t.tobj, pDeathListPreferred[iPreferredAnimChosen].iStartFrame, pDeathListPreferred[iPreferredAnimChosen].iEndFrame);
							t.smoothanim[t.tobj].fn = pDeathListPreferred[iPreferredAnimChosen].iEndFrame;
							t.smoothanim[t.tobj].usefulTimer = timeGetTime();
							SetObjectSpeed(t.tobj, 25.0f);

							// created for this frame 
							g_bOnlyOneRagdollPlusPerCycleForPerformance = false;
						}
						else
						{
							// absolutely no animation would work, defer to ragdoll method
							bGoToRagdollNow = true;
						}
						// restore original bounds after above per-frame calcs
						pObject->collision.vecMin = vecMinOrig;
						pObject->collision.vecMax = vecMaxOrig;
						pObject->collision.vecCentre = vecCenterOrig;
					}
					else
					{
						// steal orientation from shape box finding a nice floor
						if (ObjectExist(iSpareRagdollPlusPhyObj) == 1)
						{
							sObject* pObject = GetObjectData(t.tobj);
							float fCurrentFrame = WickedCall_GetObjectFrame(pObject);
							if (fCurrentFrame >= pObject->fAnimFrameEnd)
							{
								// stop animation
								StopObject(t.tobj);

								// perhaps transition to ragdoll for final settlement
								t.entityelement[t.e].ragdollplusactivate = 99; // special mode - ignore ragdoll!!
								t.entityelement[t.e].ragdollplusweapontypeused = 0;
								//PE: Make sure to update lua health ... sometimes npc keep shooting ragdolls
								t.entityelement[t.e].lua.flagschanged = 2;
								bGoToRagdollNow = true;
							}
							else
							{
								// after one second of death anim, can release weapon
								if (timeGetTime() - t.smoothanim[t.tobj].usefulTimer > 500)
								{
									bDropAnyWeaponNow = true;
								}

								// use the last frame of the death anim to anchor for final resting position
								float fInitialWaitBeforeCopyPhysicsBoxAngle = 10; //ms
								if (timeGetTime() > t.smoothanim[t.tobj].usefulTimer + fInitialWaitBeforeCopyPhysicsBoxAngle)
								{
									SetObjectToObjectOrientation(t.tobj, iSpareRagdollPlusPhyObj);
								}
								float fInitialWaitBeforeCopyPhysicsBoxFalling = 20; //ms
								if (timeGetTime() > t.smoothanim[t.tobj].usefulTimer + fInitialWaitBeforeCopyPhysicsBoxFalling)
								{
									float fDX = ObjectPositionX(iSpareRagdollPlusPhyObj) - t.entityelement[t.e].x;
									float fDY = (ObjectPositionY(iSpareRagdollPlusPhyObj) - (ObjectSizeY(iSpareRagdollPlusPhyObj) / 2)) - t.entityelement[t.e].y;
									float fDZ = ObjectPositionZ(iSpareRagdollPlusPhyObj) - t.entityelement[t.e].z;
									fDX /= 20.0f; fDY /= 10.0f; fDZ /= 20.0f;
									float fInitialWaitBeforeCopyPhysicsBoxFalling = 350; //ms
									if (timeGetTime() > t.smoothanim[t.tobj].usefulTimer + fInitialWaitBeforeCopyPhysicsBoxFalling)
									{
										// we slerp to the physics object center - all axis
										t.entityelement[t.e].x += fDX;
										t.entityelement[t.e].y += fDY;
										t.entityelement[t.e].z += fDZ;
									}
									else
									{
										// we slerp to the physics object center - just X and Z while physics settles
										t.entityelement[t.e].x += fDX;
										t.entityelement[t.e].z += fDZ;
									}
									PositionObject (t.tobj, t.entityelement[t.e].x, t.entityelement[t.e].y, t.entityelement[t.e].z);
								}
							}
						}
					}
					pObject->collision.vecMin = vecMinOrig;
					pObject->collision.vecMax = vecMaxOrig;
					pObject->collision.vecCentre = vecCenterOrig;

					// can drop weapon after death anim is underway
					if ( bDropAnyWeaponNow == true || bGoToRagdollNow == true )
					{
						// make attachment object a physics object so it drops in better place (either immediate if ragdoll, or at end of anim if death anim - see above)
						t.tattobj = t.entityelement[t.e].attachmentobj;
						if (t.tattobj > 0)
						{
							// unglue from character
							float fWorldPosOfWeaponX = LimbPositionX(t.tattobj, 0);
							float fWorldPosOfWeaponY = LimbPositionY(t.tattobj, 0);
							float fWorldPosOfWeaponZ = LimbPositionZ(t.tattobj, 0);
							UnGlueObject(t.tattobj);
							PositionObject(t.tattobj, fWorldPosOfWeaponX, fWorldPosOfWeaponY, fWorldPosOfWeaponZ);

							// and ensure it does not bury into surface by raising it
							if (ODEFind(t.tattobj) == 0)
							{
								ODECreateDynamicBox (t.tattobj, -1, 1);
								ODESetLinearVelocity(t.tattobj, 0, 20.0f, 0);
							}
						}
					}
					if (bGoToRagdollNow == true)
					{
						// delete any assist physics objects and start ragdoll
						int iSpareRagdollPlusPhyObj = t.entityelement[t.e].ragdollifiedplusphyobj;
						if (ObjectExist(iSpareRagdollPlusPhyObj) == 1)
						{
							ODEDestroyObject(iSpareRagdollPlusPhyObj);
							DeleteObject(iSpareRagdollPlusPhyObj);
						}
						t.entityelement[t.e].ragdollifiedplusphyobj = 0;
						if (t.entityelement[t.e].ragdollplusactivate != 99 )
						{
							// pretty horrible - ragdoll overhaul much needed!!
							// create the ragdoll
							t.ttte = t.e;
							ragdoll_setcollisionmask (t.entityelement[t.ttte].eleprof.colondeath);
							t.tphye = t.ttte; t.tphyobj = t.entityelement[t.ttte].obj; 
							ragdoll_create();
							// prep ragdolified vars for zero force, just collapse
							t.entityelement[t.ttte].ragdollified = 1;
							t.entityelement[t.ttte].ragdollifiedforcex_f = 0.8f;
							t.entityelement[t.ttte].ragdollifiedforcey_f = 0.0f;
							t.entityelement[t.ttte].ragdollifiedforcez_f = 0.8f;
							t.entityelement[t.ttte].ragdollifiedforcevalue_f = 0.0f;
							t.entityelement[t.ttte].ragdollifiedforcelimb = 0;
						}
						t.entityelement[t.e].ragdollplusactivate = 0;
						t.entityelement[t.e].ragdollplusweapontypeused = 0;
					}
				}
			}

			// Entity Prompt Local
			extern bool bActivatePromptXYOffset;
			extern bool bActivatePromptOffset3D;
			extern int iPromptXOffset;
			extern int iPromptYOffset;
			extern int iPromptZOffset;

			if ( t.entityelement[t.e].overprompttimer>0 ) 
			{
				if ( ObjectExist(t.tobj) == 1 ) 
				{
					if ( MAXTimer()>(int)t.entityelement[t.e].overprompttimer ) 
					{
						if ( t.entityelement[t.e].overpromptuse3D == false ) 
							t.entityelement[t.e].overprompttimer=0;
						else
							lua_hideperentity3d ( t.e );
						bActivatePromptXYOffset = false;
						bActivatePromptOffset3D = false;
						iPromptXOffset = 0;
						iPromptYOffset = 0;
					}
					else
					{
						if ( t.entityelement[t.e].overpromptuse3D == false )
						{
							if ( GetInScreen(t.tobj) == 1 ) 
							{
								t.t_s=t.entityelement[t.e].overprompt_s ; t.twidth=getbitmapfontwidth(t.t_s.Get(),1)/2;
								if (bActivatePromptXYOffset)
								{
									if (bActivatePromptOffset3D && t.tobj > 0)
									{
										sObject* pObject = g_ObjectList[t.tobj];
										if (pObject)
										{
											DARKSDK_DLL void DB_ObjectScreenData(sObject* pObject, int* x, int* y);
											GGVECTOR3 vecPosOld = pObject->position.vecPosition;
											pObject->position.vecPosition.x += iPromptXOffset;
											pObject->position.vecPosition.y += iPromptYOffset;
											pObject->position.vecPosition.z += iPromptZOffset;
											int scx = 0, scy = 0;
											//PE: More precise placement of prompt text.
											ImVec2 Convert3DTo2D(float x, float y, float z);
											ImVec2 v2DPos = Convert3DTo2D(pObject->position.vecPosition.x, pObject->position.vecPosition.y, pObject->position.vecPosition.z);
											pastebitmapfont(t.t_s.Get(), v2DPos.x - t.twidth, v2DPos.y, 1, 255);
											pObject->position.vecPosition = vecPosOld;
										}
									}
									else
										pastebitmapfont(t.t_s.Get(), (GetScreenX(t.tobj) - t.twidth) + iPromptXOffset, GetScreenY(t.tobj) + iPromptYOffset, 1, 255);
								}
								else
									pastebitmapfont(t.t_s.Get(),GetScreenX(t.tobj)-t.twidth,GetScreenY(t.tobj),1,255);
							}
						}
						else
						{
							lua_updateperentity3d ( t.e, t.entityelement[t.e].overprompt_s.Get(), t.entityelement[t.e].overprompt3dX, t.entityelement[t.e].overprompt3dY, t.entityelement[t.e].overprompt3dZ, t.entityelement[t.e].overprompt3dAY, t.entityelement[t.e].overprompt3dFaceCamera );
						}
					}
				}
			}

			// if ragdoll and has force, apply it repeatedly
			if ( t.tobj>0 ) 
			{
				if (t.entityelement[t.e].ragdollified == 1 && t.entityelement[t.e].ragdollifiedforcevalue_f > 1.0)
				{
					if (BPhys_RagDollApplyForce (t.tobj, t.entityelement[t.e].ragdollifiedforcelimb, 0, 0, 0, t.entityelement[t.e].ragdollifiedforcex_f, t.entityelement[t.e].ragdollifiedforcey_f, t.entityelement[t.e].ragdollifiedforcez_f, t.entityelement[t.e].ragdollifiedforcevalue_f) == true)
					{
						t.entityelement[t.e].ragdollifiedforcevalue_f = t.entityelement[t.e].ragdollifiedforcevalue_f*0.75;
						if (t.entityelement[t.e].ragdollifiedforcevalue_f <= 1.0)
						{
							t.entityelement[t.e].ragdollifiedforcevalue_f = 0;
						}
					}
				}
			}

			//  obtain distance from camera/player
			entity_controlrecalcdist ( );
			if (t.entityelement[t.e].lua.flagschanged == 0)
			{
				if (abs(t.entityelement[t.e].plrdist - t.dist_f) > 10)
				{
					t.entityelement[t.e].lua.flagschanged = 1;
				}
			}
			t.entityelement[t.e].plrdist=t.dist_f;

			// control immunity for entities
			if ( t.entityelement[t.e].briefimmunity > 0 )
			{
				t.entityelement[t.e].briefimmunity--;
			}
			else
			{
				// when not immune, check if characters are underwater and damage them as a result
				if (t.entityprofile[t.entid].ischaracter == 1)
				{
					if (t.entityelement[t.e].y + 65.0f < t.terrain.waterliney_f)
					{
						// damage from drowning (means we do not need handling in ALL character scripts)
						if (t.entityelement[t.e].iCanGoUnderwater == 0)
						{
							t.tdamage = 10;
							t.tdamageforce = 0;
							t.brayx1_f = t.entityelement[t.e].x;
							t.brayy1_f = t.entityelement[t.e].y + 10.0f;
							t.brayz1_f = t.entityelement[t.e].z;
							t.brayx2_f = t.entityelement[t.e].x;
							t.brayy2_f = t.entityelement[t.e].y;
							t.brayz2_f = t.entityelement[t.e].z;
							t.tdamagesource = 0;
							t.ttte = t.e; entity_applydamage(); t.e = t.ttte;
						}
					}
				}
			}

			// in all active states, must repell player to avoid penetration
			if ( t.tobj > 0 ) 
			{
				if ( (t.entityprofile[t.entid].ischaracter == 1 && t.entityprofile[t.entid].collisionmode != 22) || t.entityprofile[t.entid].collisionmode == 21 )
				{
					if ( t.entityelement[t.e].health>0 && t.entityelement[t.e].usingphysicsnow == 1 ) 
					{
						bool bThirdPersonPlayer = false;
						if ( t.playercontrol.thirdperson.enabled == 1 && t.playercontrol.thirdperson.charactere == t.e ) bThirdPersonPlayer = true;
						if ( bThirdPersonPlayer == false )
						{
							t.tplrproxx_f=ObjectPositionX(t.aisystem.objectstartindex)-ObjectPositionX(t.tobj);
							t.tplrproyy_f=ObjectPositionY(t.aisystem.objectstartindex)-ObjectPositionY(t.tobj);
							t.tplrproxz_f=ObjectPositionZ(t.aisystem.objectstartindex)-ObjectPositionZ(t.tobj);
							t.tplrproxd_f=Sqrt(abs(t.tplrproxx_f*t.tplrproxx_f)+abs(t.tplrproyy_f*t.tplrproyy_f)+abs(t.tplrproxz_f*t.tplrproxz_f));
							t.tplrproxa_f=atan2deg(t.tplrproxx_f,t.tplrproxz_f);
							if (  t.tplrproxd_f<t.entityprofile[t.entid].fatness ) 
							{
								float fDepthOfPen = t.entityprofile[t.entid].fatness - t.tplrproxd_f;
								if (fDepthOfPen > 1.0f)
								{
									t.playercontrol.pushforce_f += fDepthOfPen / 100.0f;
									t.playercontrol.pushangle_f = t.tplrproxa_f;
								}
							}
						}
					}
				}
			}

			// Handle when entity limb flinch hurt system
			if ( t.entityelement[t.e].limbhurt>0 && t.entityelement[t.e].health>0 ) 
			{
				//  known limbs
				t.headlimbofcharacter=t.entityprofile[t.entityelement[t.e].bankindex].headlimb;
				t.spine2limbofcharacter=t.entityprofile[t.entityelement[t.e].bankindex].spine2;
				//  determine which segment the limb belongs
				t.tsegmenttoflinch=0;
				if (  t.entityelement[t.e].limbhurt == t.headlimbofcharacter  )  t.tsegmenttoflinch = 1;
				//  degrade flinch value until finished
				t.tsmoothspeed_f=3.0/g.timeelapsed_f;
				t.entityelement[t.e].limbhurta_f=CurveValue(0,t.entityelement[t.e].limbhurta_f,t.tsmoothspeed_f);
				if (  abs(t.entityelement[t.e].limbhurta_f)<1.0 ) 
				{
					t.entityelement[t.e].limbhurta_f=0;
					t.entityelement[t.e].limbhurt=0;
				}
				//  modify character limbs based on segment hurt
				if (  t.tobj>0 ) 
				{
					if (  ObjectExist(t.tobj) == 1 ) 
					{
						if (  t.tsegmenttoflinch == 0 ) 
						{
							if (  t.spine2limbofcharacter>0 ) 
							{
								if (  LimbExist(t.tobj,t.spine2limbofcharacter) == 1 ) 
								{
									RotateLimb (  t.tobj,t.spine2limbofcharacter,t.entityelement[t.e].limbhurta_f/3.0,t.entityelement[t.e].limbhurta_f*-1,0 );
								}
							}
						}
						if (  t.tsegmenttoflinch == 1 ) 
						{
							if (  t.headlimbofcharacter>0 ) 
							{
								if (  LimbExist(t.tobj,t.headlimbofcharacter) == 1 ) 
								{
									RotateLimb (  t.tobj,t.headlimbofcharacter,t.entityelement[t.e].limbhurta_f,LimbAngleY(t.tobj,t.headlimbofcharacter),LimbAngleZ(t.tobj,t.headlimbofcharacter) );
								}
							}
						}
					}
				}
			}

			// if entity using non-3d sound, needs to update based on camera position
			// (can also be used for moving entities that LoopSound ( later) )
			if ( t.entityelement[t.e].soundisnonthreedee == 1 ) 
			{
				t.entityelement[t.e].soundisnonthreedee=0;
				if ( t.entityelement[t.e].soundset>0 ) 
				{
					PositionSound (  t.entityelement[t.e].soundset,CameraPositionX(0),CameraPositionY(0),CameraPositionZ(0) );
					if ( SoundPlaying(t.entityelement[t.e].soundset) == 1 ) 
					{
						t.entityelement[t.e].soundisnonthreedee=1;
					}
				}
				if ( t.entityelement[t.e].soundset1>0 ) 
				{
					PositionSound (  t.entityelement[t.e].soundset1,CameraPositionX(0),CameraPositionY(0),CameraPositionZ(0) );
					if (  SoundPlaying(t.entityelement[t.e].soundset1) == 1 ) 
					{
						t.entityelement[t.e].soundisnonthreedee=1;
					}
				}
				if (  t.entityelement[t.e].soundset2>0 ) 
				{
					PositionSound (  t.entityelement[t.e].soundset2,CameraPositionX(0),CameraPositionY(0),CameraPositionZ(0) );
					if (  SoundPlaying(t.entityelement[t.e].soundset2) == 1 ) 
					{
						t.entityelement[t.e].soundisnonthreedee=1;
					}
				}
				if (  t.entityelement[t.e].soundset3>0 ) 
				{
					PositionSound (  t.entityelement[t.e].soundset3,CameraPositionX(0),CameraPositionY(0),CameraPositionZ(0) );
					if (  SoundPlaying(t.entityelement[t.e].soundset3) == 1 ) 
					{
						t.entityelement[t.e].soundisnonthreedee=1;
					}
				}
				if (t.entityelement[t.e].soundset5 > 0)
				{
					PositionSound(t.entityelement[t.e].soundset5, CameraPositionX(0), CameraPositionY(0), CameraPositionZ(0));
					if (SoundPlaying(t.entityelement[t.e].soundset5) == 1)
					{
						t.entityelement[t.e].soundisnonthreedee = 1;
					}
				}
				if (t.entityelement[t.e].soundset6 > 0)
				{
					PositionSound (t.entityelement[t.e].soundset6, CameraPositionX(0), CameraPositionY(0), CameraPositionZ(0));
					if (SoundPlaying(t.entityelement[t.e].soundset6) == 1)
					{
						t.entityelement[t.e].soundisnonthreedee = 1;
					}
				}
			}

			// character creator object
			if ( t.entityprofile[t.entid].ischaractercreator == 1 ) 
			{
				t.tccobj = g.charactercreatorrmodelsoffset+((t.e*3)-t.characterkitcontrol.offset);
				if ( ObjectExist(t.tccobj) == 1 ) 
				{
					// only glue head if enemy is visible
					t.tconstantlygluehead=0;
					if ( t.tobj>0 ) { if ( GetVisible(t.tobj)==1 ) { t.tconstantlygluehead=1; } } 
					if ( t.game.runasmultiplayer == 1 ) 
					{
						// deal with multiplayer issues - if ( its me, ) only show me when im dead
						if ( t.characterkitcontrol.showmyhead == 1 && t.e == t.mp_playerEntityID[g.mp.me] ) 
						{
							t.tconstantlygluehead=1;
						}
						// if other players are dead and transitioning to a new spawn postion
						for ( t.ttemploop = 0 ; t.ttemploop <= MP_MAX_NUMBER_OF_PLAYERS; t.ttemploop++ )
						{
							if ( t.ttemploop != g.mp.me ) 
							{
								 int iAlive = PhotonGetPlayerAlive(t.ttemploop);
								if ( t.e == t.mp_playerEntityID[t.ttemploop] && t.mp_forcePosition[t.ttemploop]>0 && iAlive == 1 ) 
								{
									t.tconstantlygluehead=0;
								}
							}
						}
					}
					// if head is flagged to by glued, attach to body now
					if ( t.tconstantlygluehead == 1 ) 
					{
						// NOTE; re-searching for head limb is a performance hit
						t.tSourcebip01_head=getlimbbyname(t.entityelement[t.e].obj, "Bip01_Head");
						if ( t.tSourcebip01_head>0 ) 
						{
							//Dave - fix to heads being backwards for characters when switched off (3000 units away)
							float tdx = CameraPositionX(0) - ObjectPositionX(t.entityelement[t.e].obj);
							float tdy = CameraPositionY(0) - ObjectPositionY(t.entityelement[t.e].obj);
							float tdz = CameraPositionZ(0) - ObjectPositionZ(t.entityelement[t.e].obj);
							float tdist = sqrt ( tdx*tdx + tdy*tdy + tdz*tdz );
							t.te = t.e; entity_getmaxfreezedistance ( );
							if ( tdist > t.maximumnonefreezedistance )
							{
								YRotateObject (  t.tccobj, ObjectAngleY(t.entityelement[t.e].obj)-180 );
							}
							else
							{
								YRotateObject (  t.tccobj, 0 );								
							}							
							GlueObjectToLimbEx (  t.tccobj,t.entityelement[t.e].obj,t.tSourcebip01_head,2 );
						}
					}
					else
					{
						//  else unglue and hide the head
						UnGlueObject (  t.tccobj );
						PositionObject (  t.tccobj,100000,100000,100000 );
					}
				}
			}

			// handle particle emitter entity (for when in game)
			entity_updateparticleemitter(t.e);
			entity_updateautoflatten(t.e);

			// flag to destroy entity dead (can be set from LUA command or explosion trigger)
			if ( t.entityelement[t.e].destroyme == 1 ) 
			{
				// mark as destroyed officially
				t.entityelement[t.e].destroyme = 0;
				entity_adddestroyevent(t.e);

				// remove entity from game play
				t.entityelement[t.e].eleprof.phyalways = 0;
				t.entityelement[t.e].active = 0;
				t.entityelement[t.e].health = 0;
				t.entityelement[t.e].eleprof.lootpercentage = 0;
				t.entityelement[t.e].lua.flagschanged = 2;
				if ( t.game.runasmultiplayer == 1 ) 
				{
					mp_addDestroyedObject ( );
				}
				t.obj=t.entityelement[t.e].obj;

				// hide if NOT ragdollified OR explodable
				if (t.entityelement[t.e].ragdollified == 0 || t.entityelement[t.e].eleprof.explodable != 0 )
				{
					if (t.obj > 0)
					{
						if (ObjectExist(t.obj) == 1)
						{
							HideObject (t.obj);
						}
					}
				}

				// and also hide the attachment (if any)
				entity_freeattachment();

				// attempt to remove collision object
				entity_lua_collisionoff ( );

				// possible remove character
				entity_lua_findcharanimstate ( );
				if (  t.tcharanimindex != -1 ) 
				{
					//  deactivate DarkA.I for this dead entity
					darkai_killai ( );

					//  Convert object back to instance and hide it
					t.charanimstates[t.tcharanimindex] = t.charanimstate;
				}
				else
				{
					//  can still have non-character ragdoll (zombie), so remove ragdoll if so
					t.tphyobj=t.obj ; ragdoll_destroy ( );
				}

				// find and remove if in any inventory
				for (int inventoryindex = 0; inventoryindex < t.inventoryContainers.size(); inventoryindex++)
				{
					for (int n = 0; n < t.inventoryContainer[inventoryindex].size(); n++)
					{
						if (t.inventoryContainer[inventoryindex][n].e == t.e)
						{
							t.inventoryContainer[inventoryindex].erase(t.inventoryContainer[inventoryindex].begin() + n);
							break;
						}
					}
				}
			}

			// loot when dropped can be maintained
			entity_monitorloot();
		}
	}

	//  handle explosion triggers in separate loop as they call
	//  other subroutines that use E and other entity calls (i.e. physics_explodesphere)
	#define PRESCANEVENTCOUNT 5
	int iOnlySomePreScanEventPerCyce = PRESCANEVENTCOUNT;
	for ( t.ee = 1 ; t.ee <= g.entityelementlist; t.ee++ )
	{
		t.eentid = t.entityelement[t.ee].bankindex;
		if (t.eentid > 0)
		{
			// the prescan
			// scan all entities that are capable of being exploded, and see what they can see and add to list (saves performant freeze later)
			if (t.entityelement[t.ee].eleprof.explodable != 0 && t.entityprofile[t.eentid].ischaracter == 0 )
			{
				if (iOnlySomePreScanEventPerCyce > 0)
				{
					if (t.entityelement[t.ee].iPreScanVisibleCurrent < g.entityelementlist)
					{
						while (iOnlySomePreScanEventPerCyce > 0)
						{
							t.entityelement[t.ee].iPreScanVisibleCurrent++;
							int iCanWeSeeThisE = t.entityelement[t.ee].iPreScanVisibleCurrent;
							if (iCanWeSeeThisE <= g.entityelementlist)
							{
								if (t.ee != iCanWeSeeThisE)
								{
									if (t.entityelement[iCanWeSeeThisE].obj > 0 && t.entityelement[iCanWeSeeThisE].staticflag == 0 && t.entityprofile[t.entityelement[iCanWeSeeThisE].bankindex].ischaracter == 0)
									{
										float fYOff = 10.0f;
										float fDX = t.entityelement[iCanWeSeeThisE].x - t.entityelement[t.ee].x;
										float fDY = t.entityelement[iCanWeSeeThisE].y - t.entityelement[t.ee].y;
										float fDZ = t.entityelement[iCanWeSeeThisE].z - t.entityelement[t.ee].z;
										float fDD = sqrt(fabs(fDX * fDX) + fabs(fDY * fDY) + fabs(fDZ * fDZ));
										float fExplodeRange = 300.0f; // seems range fixed at 300!
										if (fDD <= fExplodeRange)
										{
											// first ensure not going through physics terrain
											t.ttokay = 1;
											if (ODERayTerrain(t.entityelement[t.ee].x, t.entityelement[t.ee].y + fYOff, t.entityelement[t.ee].z, t.entityelement[iCanWeSeeThisE].x, t.entityelement[iCanWeSeeThisE].y + fYOff, t.entityelement[iCanWeSeeThisE].z, false) == 1)
											{
												t.ttokay = 0;
											}
											if (t.ttokay == 1)
											{
												// static ray test (only statics can stop force of exploding ray)
												int tintersectvalue = IntersectAllEx(g.entityviewstartobj, g.entityviewendobj, t.entityelement[t.ee].x, t.entityelement[t.ee].y + fYOff, t.entityelement[t.ee].z, t.entityelement[iCanWeSeeThisE].x, t.entityelement[iCanWeSeeThisE].y + fYOff, t.entityelement[iCanWeSeeThisE].z, t.entityelement[t.ee].obj, 1, 0, 0, 1, false);
												if (tintersectvalue != 0 && tintersectvalue != t.entityelement[iCanWeSeeThisE].obj )
												{
													// hit something that was not the destination object
													t.ttokay = 0;
												}
											}
											if (t.ttokay==1)
											{
												// line of sight between this and the exploding one, add to list for quick resolution when explode happens
												t.entityelement[t.ee].iPreScannedVisible.push_back(iCanWeSeeThisE);
											}
											iOnlySomePreScanEventPerCyce--;
										}
									}
								}
							}
							else
							{
								break;
							}
						}
					}
				}
			}
			else
			{
				// not explodable, no prescannedvis list needed
				if (t.entityelement[t.ee].iPreScanVisibleCurrent == 0)
				{
					t.entityelement[t.ee].iPreScanVisibleCurrent = g.entityelementlist;
					t.entityelement[t.ee].iPreScannedVisible.clear();
				}
			}

			// the fuse
			// flag to explode entity after a fused amount of time
			if ( t.entityelement[t.ee].explodefusetime>0 ) 
			{
				if ( MAXTimer()>t.entityelement[t.ee].explodefusetime ) 
				{
					t.entityelement[t.ee].explodefusetime = -1;
					// explode from beneath
					t.tdamage=t.entityelement[t.ee].eleprof.explodedamage;
					t.tdamageforce=0;
					t.brayx1_f=ObjectPositionX(t.entityelement[t.ee].obj)+GetObjectCollisionCenterX(t.entityelement[t.ee].obj);
					t.brayy1_f=(ObjectPositionY(t.entityelement[t.ee].obj)+GetObjectCollisionCenterY(t.entityelement[t.ee].obj))-100;
					t.brayz1_f=ObjectPositionZ(t.entityelement[t.ee].obj)+GetObjectCollisionCenterZ(t.entityelement[t.ee].obj);
					t.brayx2_f=t.brayx1_f;
					t.brayy2_f=(t.brayy1_f+100);
					t.brayz2_f=t.brayz1_f;
					t.tdamagesource=0;
					t.ttte = t.ee ; entity_applydamage() ; t.ee=t.ttte;
					// create a huge bang
					t.entityelement[t.ee].destroyme=1;
					// customize the barrel explosion in "projectiletypes\common\explode"
					t.tProjectileName_s = "";
					t.tProjectileResult = WEAPON_PROJECTILERESULT_EXPLODE;
					for (int w = 0; w < t.WeaponProjectileBase.size(); w++)
					{
						if (strstr (t.WeaponProjectileBase[w].name_s.Get(), "common\\explode") > 0)
						{
							t.tProjectileResult = WEAPON_PROJECTILERESULT_CUSTOM;
							t.tProjectileName_s = t.WeaponProjectileBase[w].name_s.Get();
							t.tProjectileResultExplosionImageID = t.WeaponProjectileBase[w].explosionImageID;
							t.tProjectileResultLightFlag = t.WeaponProjectileBase[w].explosionLightFlag;
							t.tProjectileResultSmokeImageID = t.WeaponProjectileBase[w].explosionSmokeImageID;
							t.tProjectileResultSparksCount = t.WeaponProjectileBase[w].explosionSparksCount;
							t.tProjectileResultSize = t.WeaponProjectileBase[w].explosionSize;
							t.tProjectileResultSmokeSize = t.WeaponProjectileBase[w].explosionSmokeSize;
							t.tProjectileResultSparksSize = t.WeaponProjectileBase[w].explosionSparksSize;
							break;
						}
					}
					t.tx_f=t.entityelement[t.ee].x; 
					float fRaiseAboveTheFloor = t.entityelement[t.ee].eleprof.explodeheight;
					if (fRaiseAboveTheFloor < 5) fRaiseAboveTheFloor = 5;
					t.ty_f=t.entityelement[t.ee].y + fRaiseAboveTheFloor;
					t.tz_f=t.entityelement[t.ee].z;
					t.tDamage_f = t.entityelement[t.ee].eleprof.explodedamage; 
					float fRadiusBasedOnDamage = 300; // feature request to control this via properties
					if (fRadiusBasedOnDamage < 300) fRadiusBasedOnDamage = 300;
					t.tradius_f = fRadiusBasedOnDamage;
					t.tSourceEntity = t.ee;
					// provide the explosion sound (as it cannot come from projectile)
					t.tSoundID=0;
					#define TENCLONESOUNDSFOREXPLOSIONS 10
					for ( t.tscanexp = 1 ; t.tscanexp <= TENCLONESOUNDSFOREXPLOSIONS; t.tscanexp++ )
					{
						if (t.tscanexp > 0 && SoundExist(g.explodesoundoffset + t.tscanexp) == 0)
						{
							CloneSound(g.explodesoundoffset + t.tscanexp, g.explodesoundoffset);
						}
						if ( SoundExist(g.explodesoundoffset+t.tscanexp) == 1 ) 
						{
							if ( SoundPlaying(g.explodesoundoffset+t.tscanexp) == 0 ) 
							{
								t.tSoundID=g.explodesoundoffset+t.tscanexp;
							}
						}
					}
					weapon_projectileresult_make ( );
				}
			}
		}
	}
}

// Manual UV Data changes are OH SO SLOW, so add a system that only does it once per
// object per cycle, in sequence, to improve general performance while retaining the
// visual effect of changing the UV of the object being rendered
bool g_bUVDataChangeObjectSeqOnce = false;
int g_iUVDataChangeObjectSeqE = -1;

void entity_loopanim ( void )
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif

	// In game or editor, must control entity animation speed (machine indie)#
	static int currentsynccount = 0;
	currentsynccount++;

	for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
	{
		t.entid = t.entityelement[t.e].bankindex;
		if (t.entid <= 0)
		{
			//PE: Also control objects in properties.
			extern bool bImGuiInTestGame;
			if (t.game.gameisexe == 0 && t.gridentityinzoomview == t.e && !bImGuiInTestGame)
			{
				t.entid = t.gridentity; //t.entityelement[t.gridentityinzoomview].bankindex;
			}
			else
			{
				continue;
			}
		}
		t.tparentobj = g.entitybankoffset + t.entid;

		// 011016 - scenes with LARGE number of static entities hitting perf hard
		//PE: @Lee if the same master object is used without any per object animspeed changes , it will be like normal.
		//PE: Only if animspeed is changed (per object) it will create a new clone , so should not bring down fps that bad.
		if (t.entityelement[t.e].staticflag == 1 && t.entityelement[t.e].eleprof.phyalways == 0)
		{
			//Quickly skip entry, but still allow custom anim on static objects.
			if ( t.entityelement[t.e].eleprof.animspeed == t.entityprofile[t.entid].animspeed) 
			{
				//PE: We still need to set speed on master object. once each sync.
				if ( t.entityprofile[t.entid].synccount != currentsynccount && t.entityprofile[t.entid].ischaracter == 0 && t.entityelement[t.e].isclone == 0 && ObjectExist(t.tparentobj) == 1) {

					t.entityprofile[t.entid].synccount = currentsynccount; //Only update one time per sync.
					if (GetNumberOfFrames(t.tparentobj) > 0)
					{
						t.tanimspeed_f = t.entityprofile[t.entid].animspeed;
						if (ObjectExist(t.tparentobj) == 1) 
						{
							SetObjectSpeed(t.tparentobj, t.tanimspeed_f);
						}
					}
				}
				continue;
			}
		}

		// NOTE: Determine essential tasks static needs (i.e. plrdist??)
		// only handle DYNAMIC entities 
		if ( t.entid>0 ) 
		{
			//PE: Add decal support here.
			if (t.entityprofile[t.entid].bIsDecal)
			{
				t.tobj = t.entityelement[t.e].obj;
				if (t.tobj > 0)
				{
					float fNextFrame = g.timeelapsed_f*t.entityelement[t.e].fDecalSpeed;
					if ((int)t.entityelement[t.e].fDecalFrame != (int)(t.entityelement[t.e].fDecalFrame + fNextFrame))
					{
						t.entityelement[t.e].fDecalFrame = t.entityelement[t.e].fDecalFrame + fNextFrame;
						if (t.entityelement[t.e].fDecalFrame < 0) t.entityelement[t.e].fDecalFrame = 0;
						if (t.entityelement[t.e].fDecalFrame > (t.entityprofile[t.entid].iDecalRows * t.entityprofile[t.entid].iDecalColumns))
							t.entityelement[t.e].fDecalFrame = 0;

						//PE: Way faster way to set decals UV now also animate smooth.
						sObject* pObject = GetObjectData(t.tobj);
						if (pObject)
						{
							float scaleu = 1.0f / t.entityprofile[t.entid].iDecalColumns;
							float scalev = 1.0f / t.entityprofile[t.entid].iDecalRows;
							int iFrameIndex = t.entityelement[t.e].fDecalFrame;
							int iColumns = t.entityprofile[t.entid].iDecalColumns;
							int row = iFrameIndex / iColumns;
							int col = iFrameIndex % iColumns;
							float offsetu = col * scaleu;
							float offsetv = row * scalev;
							WickedCall_SetObjectTextureUV(pObject, 1.0f, 1.0f, offsetu, offsetv);
						}
					}
					else
					{
						t.entityelement[t.e].fDecalFrame = t.entityelement[t.e].fDecalFrame + fNextFrame;
					}
				}
			}
			else if (t.entityprofile[t.entid].ischaracter == 0)
			{
				// but not for characters which have their own speed control
				t.tobj=t.entityelement[t.e].obj;
				if ( t.tobj>0 ) 
				{
					if ( ObjectExist(t.tobj) == 1 && ObjectExist(t.tparentobj) == 1 ) 
					{
						if ( GetNumberOfFrames(t.tparentobj)>0 ) 
						{
							// 120417 - now modulate anim speed with script controlled modulation
							float fFinalAnimSpeed = t.entityelement[t.e].eleprof.animspeed * t.entityelement[t.e].animspeedmod;

							// Detect if entity instance speed different from base default, and if so, need CLONE!
							if ( t.entityelement[t.e].isclone == 0 ) 
							{
								if ( fFinalAnimSpeed != t.entityprofile[t.entid].animspeed ) 
								{
									// Entity must be unique to allow different speed from parent
									t.tte = t.e ; entity_converttoclone ( );

									//Start animation.
									t.tobj = t.entityelement[t.e].obj;
									if (GetNumberOfFrames(t.tobj) > 0)
									{
										//  allow first animation
										if ((t.game.set.ismapeditormode == 0 || t.game.gameisexe == 1) && t.entityprofile[t.entid].startanimingame > 0 && t.entityprofile[t.entid].animmax > 0) 
										{ 
											//PE:
											t.q = t.entityprofile[t.entid].startanimingame - 1;
											if (t.q >= 0)
												LoopObject(t.tobj, t.entityanim[t.entid][t.q].start, t.entityanim[t.entid][t.q].finish);
										}
										else if (t.entityprofile[t.entid].animmax > 0 && t.entityprofile[t.entid].playanimineditor > 0)
										{
											SetObjectFrame(t.tobj, 0); LoopObject(t.tobj); StopObject(t.tobj);
											t.q = t.entityprofile[t.entid].playanimineditor - 1;
											if (t.q >= 0)
												LoopObject(t.tobj, t.entityanim[t.entid][t.q].start, t.entityanim[t.entid][t.q].finish);
										}
										else if (t.entityprofile[t.entid].playanimineditor < 0)
										{
											// uses name instead of index, the negative is the ordinal into the animset
											extern void entity_loop_using_negative_playanimineditor(int e, int obj, cstr animname);
											entity_loop_using_negative_playanimineditor(t.e, t.tobj, t.entityprofile[t.entid].playanimineditor_name);
										}
									}
								}
							}
							bool isWicked = false;
							isWicked = true;
							if ( t.entityelement[t.e].isclone == 1 || isWicked )
							{
								// Control animation speed of cloned object
								t.tanimspeed_f = fFinalAnimSpeed * t.entityelement[t.e].speedmodulator_f;
								SetObjectSpeed (t.tobj, t.tanimspeed_f);
							}
							else
							{
								//PE: This is never called, (continue; above).
								//  Control animation speed of parent object associated with instance
								t.tanimspeed_f = t.entityprofile[t.entid].animspeed;
								if (ObjectExist(t.tparentobj) == 1)
								{
									SetObjectSpeed (t.tparentobj, t.tanimspeed_f);
								}
							}
							//  if animation in progress (handle any transitioning)
							if ( t.entityelement[t.e].lua.animating == 1 ) 
							{
								smoothanimupdate(t.tobj);
							}
						}
					}
				}
			}

			// also handle entity footfall sounds from loopanim
			t.tobj = t.entityelement[t.e].obj;
			if (t.tobj > 0)
			{
				float fCurrentFrame = GetFrame(t.tobj);
				sObject* pObject = GetObjectData(t.tobj);
				if (pObject)
				{
					sAnimationSet* pAnimSet = pObject->pAnimationSet;
					while (pAnimSet)
					{
						int leftorright = 0;
						int iFootFallKeyFrame = (int)pAnimSet->fAnimSetStep1;
						float fDistanceFromFrame = fabs(fCurrentFrame - iFootFallKeyFrame);
						if (iFootFallKeyFrame > 0 && fCurrentFrame >= iFootFallKeyFrame && fDistanceFromFrame < 5.0f && iFootFallKeyFrame != t.entityelement[t.e].lastfootfallframeindex)
						{
							// okay to use this footfall (left one)
							leftorright = 0;
						}
						else
						{
							// if no luck, try the right foot
							iFootFallKeyFrame = (int)pAnimSet->fAnimSetStep2;
							fDistanceFromFrame = fabs(fCurrentFrame - iFootFallKeyFrame);
							if (iFootFallKeyFrame > 0 && fCurrentFrame >= iFootFallKeyFrame && fDistanceFromFrame < 5.0f && iFootFallKeyFrame != t.entityelement[t.e].lastfootfallframeindex)
							{
								// okay to use this footfall (right one)
								leftorright = 1;
							}
							else
							{
								// else try the final step (any one)
								iFootFallKeyFrame = (int)pAnimSet->fAnimSetStep3;
								fDistanceFromFrame = fabs(fCurrentFrame - iFootFallKeyFrame);
								leftorright = 0;
							}
						}
						if (iFootFallKeyFrame > 0 && fCurrentFrame >= iFootFallKeyFrame && fDistanceFromFrame < 5.0f && iFootFallKeyFrame != t.entityelement[t.e].lastfootfallframeindex)
						{
							// ensure this footfall frame not triggered again until another one gets triggered
							t.entityelement[t.e].lastfootfallframeindex = iFootFallKeyFrame;

							// choose footfall sound for character
							int iFootFallType = -1;

							// above or below water line
							if (t.entityelement[t.e].y > t.terrain.waterliney_f + 36 || t.hardwareinfoglobals.nowater != 0)
							{
								// this arb value is CHANGED if a capsule (character) when it touches physics!
								iFootFallType = ODEGetBodyAttribValue (t.tobj);
							}
							else
							{
								if (t.entityelement[t.e].y > t.terrain.waterliney_f - 33)
								{
									// footfall water wading sound
									iFootFallType = 17;
								}
								else
								{
									// underwater sound for character
									iFootFallType = 18;
								}
							}

							// manage trigger of footfall sound effects
							if (iFootFallType != -1)
							{
								// play footfall sound effect at character position
								sound_footfallsound (iFootFallType, t.entityelement[t.e].x, t.entityelement[t.e].y, t.entityelement[t.e].z, leftorright, &t.entityelement[t.e].lastfootfallsound);
							}
						}

						// next anim
						pAnimSet = pAnimSet->pNext;
					}
				}
			}
		}
	}

	// resets for next UV Data change that may be required
	if (g_bUVDataChangeObjectSeqOnce == false)
	{
		// detect if no UV event was made, so assume reached end of SeqE list, and should reset to -1
		g_iUVDataChangeObjectSeqE = -1;
	}
	else
	{
		// ensure next cycle allows one more event to happen
		g_bUVDataChangeObjectSeqOnce = false;
	}
}

void entity_controlrecalcdist ( void )
{
	//  Distance between player camera and entity
	t.tobj=t.entityelement[t.e].obj;
	if (  t.tobj>0 && (t.entityelement[t.e].active != 0 || t.entityelement[t.e].eleprof.phyalways != 0) ) 
	{
		if (  ObjectExist(t.tobj)  ==  1 ) 
		{
			t.distx_f=CameraPositionX(0)-ObjectPositionX(t.tobj);
			t.disty_f=CameraPositionY(0)-ObjectPositionY(t.tobj);
			t.distz_f=CameraPositionZ(0)-ObjectPositionZ(t.tobj);
			t.dist_f=Sqrt(abs(t.distx_f*t.distx_f)+abs(t.disty_f*t.disty_f)+abs(t.distz_f*t.distz_f));
			t.diffangle_f=atan2deg(t.distx_f,t.distz_f);
			if (  t.diffangle_f<0  )  t.diffangle_f = t.diffangle_f+360;
		}
		else
		{
			t.dist_f=9999999;
		}
	}
	else
	{
		t.dist_f=9999999;
	}
return;

}

void entity_getmaxfreezedistance ( void )
{
	if (  t.entityelement[t.te].eleprof.phyalways != 0 ) 
	{
		//  always active characters NEVER freeze at distance
		t.maximumnonefreezedistance=MAXNEVERFREEZEDISTANCE;
	}
	else
	{
		//  distance at which logic is frozen
		t.maximumnonefreezedistance=MAXFREEZEDISTANCE;
	}
	//  AI that is attacking another player counts as on
	if (  t.game.runasmultiplayer  ==  1 ) 
	{
		if (  t.entityelement[t.te].mp_updateOn  ==  1  )  t.maximumnonefreezedistance = MAXNEVERFREEZEDISTANCE;
	}
}

bool g_bResetPosIgnoreGravity = false;

void entity_updatepos ( void )
{
	// takes te - but not tv# as already dealth with when moved entity X Y Z
	t.tobj=t.entityelement[t.te].obj;

	//PE: If physics is diabled in setting use PositionObject.
	int ODEFind(int iID);
	bool bPhysicsActive = ODEFind(t.tobj);

	// move entity using physics
	if ( t.entityelement[t.te].usingphysicsnow == 1 && bPhysicsActive)
	{
		if(t.entityprofile[t.entityelement[t.te].bankindex].ischaracter == 1 && t.entityelement[t.te].eleprof.disableascharacter == 0)
		{
			// old legacy system was glitchy, pulling back real XZ and causing huge jumps into the air
			// simpler system gives priority to navmesh guide movement and rays to find surface, 
			// then move physics capsule to push dynamic stuff out of the way (no longer a controller)
			ODESetLinearVelocityUsingWorldPosTarget (t.tobj, t.entityelement[t.te].x, t.entityelement[t.te].y, t.entityelement[t.te].z);
		}
		else
		{
			// non-characters subjecty to simpler physics forces
			float fNewDistanceX = t.entityelement[t.te].x - ObjectPositionX(t.tobj);
			float fNewDistanceZ = t.entityelement[t.te].z - ObjectPositionZ(t.tobj);
			float fNewDistanceTotal = Sqrt(fabs(fNewDistanceX * fNewDistanceX) + fabs(fNewDistanceZ * fNewDistanceZ));
			if (fNewDistanceTotal <= 0) return;
			// work out normalized increment
			float fForceToApplyX = fNewDistanceX / fNewDistanceTotal;
			float fForceToApplyZ = fNewDistanceZ / fNewDistanceTotal;
			// apply the specified speed
			float fSpeed = fNewDistanceTotal;
			float fSpeedMax = 25.0f * ((t.entityelement[t.te].eleprof.speed + 0.0) / 100.0);
			if (fSpeed > fSpeedMax) fSpeed = fSpeedMax;
			fForceToApplyX *= (fSpeed * 15.0f);
			fForceToApplyZ *= (fSpeed * 15.0f);

			if (t.entityelement[t.te].nogravity == 1)
			{
				// special case of non character entity with gravity off (pickupable objects)
				float fNoGravY = t.entityelement[t.te].y - ObjectPositionY(t.tobj);
				if (fabs(fNoGravY) > 0.0f)
				{
					if (fNoGravY > fSpeedMax) fNoGravY = fSpeedMax;
					if (fNoGravY < -fSpeedMax) fNoGravY = -fSpeedMax;
					fNoGravY *= 30.0f; // keep it in eye view when look up and down 15.0f;
				}

				ODESetLinearVelocity(t.tobj, fForceToApplyX, fNoGravY * 2, fForceToApplyZ);
			}
			else
			{
				// apply force to physics object to get to new XZ position (eventually)
				ODESetLinearVelocityXZWithGravity(t.tobj, fForceToApplyX, fForceToApplyZ, t.tvgravity_f);
			}
		}
	}
	else
	{
		if (g_bResetPosIgnoreGravity == true)
		{
			// used when resetpositionxyz needs to set exact XYZ position of refreshed/loaded entity
		}
		else
		{ 
			/* removed to fix CineGuru collision bug where entity would snap to ground - that is, we retain the position set in the level 
			if (t.entityelement[t.te].nogravity == 0 && t.entityelement[t.te].collected == 0)
			{
				//LB: if collision mode sets no physics, cannot apply a gravity assumption (fixes CineGuru collision bug)
				int entid = t.entityelement[t.te].bankindex;
				// non physics objects stick with the floor
				t.tterrainfloorposy_f = BT_GetGroundHeight (t.terrain.TerrainID, t.entityelement[t.te].x, t.entityelement[t.te].z);
				t.entityelement[t.te].y = t.tterrainfloorposy_f;
			}
			else
			{
				// no gravity allows entities to be in the sky (birds and blimps)
			}
			*/
		}
		PositionObject (t.tobj, t.entityelement[t.te].x, t.entityelement[t.te].y, t.entityelement[t.te].z);
	}
}

void entity_determinedamagemultiplier ( void )
{
	t.tdamagemultiplier_f=1.0f;
}

void entity_determinegunforce ( void )
{
	//  bulletraytype (1-pierce, 2-shotgun pellets)
	t.bulletraytype=g.firemodes[t.gunid][g.firemode].settings.damagetype;
	t.tforce_f = g.firemodes[t.gunid][g.firemode].settings.force*2.0 * t.playercontrol.fWeaponDamageMultiplier;
	if (  t.gun[t.gunid].settings.ismelee == 2 || g.firemodes[t.gunid][g.firemode].settings.usemeleedamageonly > 0)
	{
		//  100415 - added separate force for melee attacks
		t.tforce_f=g.firemodes[t.gunid][g.firemode].settings.meleeforce*2.0 * t.playercontrol.fMeleeDamageMultiplier;
	}
	else
	{
		//  regular bullet type force modifiers
		if (  t.bulletraytype == 2  )  t.tforce_f = t.tforce_f*2;
		if (  t.bulletraytype == 1 ) 
		{
			if (  t.tforce_f>40  )  t.tforce_f = 40;
		}
	}
	t.tforce_f=t.tforce_f*20.0;
return;

}

void entity_find_charanimindex_fromttte ( void )
{
	t.tcharanimindex=0;
	if ( t.ttte>0 ) 
	{
		for ( t.ttcharanimindex = 1 ; t.ttcharanimindex <= g.charanimindexmax; t.ttcharanimindex++ )
		{
			if ( t.charanimstates[t.ttcharanimindex].e == t.ttte ) 
			{
				t.tcharanimindex=t.ttcharanimindex; 
				break;
			}
		}
	}
}

void entity_adddestroyevent(int e)
{
	bool bUniqueForList = true;
	for (int n = 0; n < g_iDestroyedEntitiesList.size(); n++)
	{
		if (g_iDestroyedEntitiesList[n] == e)
		{
			bUniqueForList = false;
			break;
		}
	}
	if (bUniqueForList == true)
	{
		g_iDestroyedEntitiesList.push_back(e);
	}
}

