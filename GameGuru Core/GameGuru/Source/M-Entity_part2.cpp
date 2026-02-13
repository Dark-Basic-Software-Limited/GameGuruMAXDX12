void entity_loadactivesoundsandvideo ( void )
{
	// sounds in each entity
	char pSoundSet0[MAX_PATH];
	char pSoundSet1[MAX_PATH];
	char pSoundSet2[MAX_PATH];
	char pSoundSet3[MAX_PATH];
	char pSoundSet5[MAX_PATH];
	char pSoundSet6[MAX_PATH];

	// go through all entities in level
	for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if ( t.entid>0 ) 
		{
			bool bNeedAssetsLoading = false;
			if (t.entityelement[t.e].active == 1) bNeedAssetsLoading = true;
			if (t.entityelement[t.e].eleprof.spawnatstart == 0) bNeedAssetsLoading = true;
			if ( bNeedAssetsLoading == true )
			{
				// original base filenames for sound
				strcpy(pSoundSet0, t.entityelement[t.e].eleprof.soundset_s.Get());
				strcpy(pSoundSet1, t.entityelement[t.e].eleprof.soundset1_s.Get());
				strcpy(pSoundSet2, t.entityelement[t.e].eleprof.soundset2_s.Get());
				strcpy(pSoundSet3, t.entityelement[t.e].eleprof.soundset3_s.Get());
				strcpy(pSoundSet5, t.entityelement[t.e].eleprof.soundset5_s.Get());
				strcpy(pSoundSet6, t.entityelement[t.e].eleprof.soundset6_s.Get());

				// new system can adjust sound at load time to provide automatic variances
				if (t.entityprofile[t.entid].ischaracter != 0 && t.entityelement[t.e].eleprof.iUseSoundVariants)
				{
					// only apply variant system to characters (for now to limit additional setup time)
					for (int allfour = 0; allfour <= 6; allfour++)
					{
						bool bMightHaveVariant = false;
						LPSTR pThisStr = NULL;
						if (allfour == 0) pThisStr = pSoundSet0;
						if (allfour == 1) pThisStr = pSoundSet1;
						if (allfour == 2) pThisStr = pSoundSet2;
						if (allfour == 3) pThisStr = pSoundSet3;
						if (allfour == 5) pThisStr = pSoundSet5;
						if (allfour == 6) pThisStr = pSoundSet6;
						if (bMightHaveVariant == false && pThisStr && strnicmp (pThisStr + strlen(pThisStr) - 5, "1.wav", 5) == NULL) bMightHaveVariant = true;
						if (bMightHaveVariant == false && pThisStr && strnicmp (pThisStr + strlen(pThisStr) - 5, "2.wav", 5) == NULL) bMightHaveVariant = true;
						if (bMightHaveVariant == false && pThisStr && strnicmp (pThisStr + strlen(pThisStr) - 5, "3.wav", 5) == NULL) bMightHaveVariant = true;
						if (bMightHaveVariant == false && pThisStr && strnicmp (pThisStr + strlen(pThisStr) - 5, "4.wav", 5) == NULL) bMightHaveVariant = true;
						if (bMightHaveVariant == false && pThisStr && strnicmp (pThisStr + strlen(pThisStr) - 5, "5.wav", 5) == NULL) bMightHaveVariant = true;
						if (bMightHaveVariant == true)
						{
							int iOriginal = pThisStr[strlen(pThisStr) - 5] - '1';
							int iRnd = rand() % 5;
							int iAttempts = 3;
							while (iAttempts > 0)
							{
								iRnd = rand() % 5;
								pThisStr[strlen(pThisStr) - 5] = 0;
								char pNewStr[MAX_PATH];
								sprintf(pNewStr, "%s%d.wav", pThisStr, 1 + iRnd);
								strcpy(pThisStr, pNewStr);
								if (FileExist(pThisStr) == 1)
								{
									// found variant, use this!
									if (iRnd != iOriginal)
										break;
								}
								else
								{
									// not exist
									pThisStr[strlen(pThisStr) - 5] = 0;
									sprintf(pNewStr, "%s%d.wav", pThisStr, 1 + iOriginal);
									strcpy(pThisStr, pNewStr);
									iRnd = iOriginal;
								}
								iAttempts--;
							}
						}
					}
				}

				// sounds or videos
				if ( t.entityelement[t.e].soundset == 0 ) 
				{
					t.tvideofile_s = pSoundSet0; entity_loadvideoid ( );
					if ( t.tvideoid == -999 )
					{
						t.entityelement[t.e].soundset = 0;
					}
					else
					{
						if ( t.tvideoid > 0 ) 
							t.entityelement[t.e].soundset=t.tvideoid*-1;
						else
							t.entityelement[t.e].soundset=loadinternalsoundcore(pSoundSet0,1);
					}
					if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
				}
				if (  t.entityelement[t.e].soundset1 == 0 ) 
				{
					t.tvideofile_s = pSoundSet1; entity_loadvideoid ( );
					if ( t.tvideoid == -999 )
					{
						t.entityelement[t.e].soundset1 = 0;
					}
					else
					{
						if (  t.tvideoid>0 ) 
							t.entityelement[t.e].soundset1=t.tvideoid*-1;
						else
							t.entityelement[t.e].soundset1=loadinternalsoundcore(pSoundSet1,1);
					}
					if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
				}
				if (  t.entityelement[t.e].soundset2 == 0 ) 
				{
					t.entityelement[t.e].soundset2 = loadinternalsoundcore(pSoundSet2,1);
					if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
				}
				if (t.entityelement[t.e].soundset3 == 0)
				{
					t.entityelement[t.e].soundset3 = loadinternalsoundcore(pSoundSet3, 1);
					if (t.game.runasmultiplayer == 1) mp_refresh ();
				}
				if (t.entityelement[t.e].soundset5 == 0)
				{
					t.entityelement[t.e].soundset5 = loadinternalsoundcore(pSoundSet5, 1);
					if (t.game.runasmultiplayer == 1) mp_refresh ();
				}
				if (t.entityelement[t.e].soundset6 == 0)
				{
					t.entityelement[t.e].soundset6 = loadinternalsoundcore(pSoundSet6, 1);
					if (t.game.runasmultiplayer == 1) mp_refresh ();
				}

				// lipsync LIP data (associated with a sound file)
				if ( t.entityprofile[t.entid].ischaracter != 0 )
				{
					for (int s = 0; s <= 3; s++)
					{
						bool bFoundSound = false;
						if (s == 0 && t.entityelement[t.e].soundset != 0) bFoundSound = true;
						if (s == 1 && t.entityelement[t.e].soundset1 != 0) bFoundSound = true;
						if (s == 2 && t.entityelement[t.e].soundset2 != 0) bFoundSound = true;
						if (s == 3 && t.entityelement[t.e].soundset3 != 0) bFoundSound = true;
						if (bFoundSound == true)
						{
							// construct LIP filename for this sound
							char pWAVFile[2048];
							char pLIPFile[2048];
							if (s == 0) strcpy(pLIPFile, pSoundSet0);
							if (s == 1) strcpy(pLIPFile, pSoundSet1);
							if (s == 2) strcpy(pLIPFile, pSoundSet2);
							if (s == 3) strcpy(pLIPFile, pSoundSet3);
							strcpy(pWAVFile, pLIPFile);
							pLIPFile[strlen(pLIPFile) - 4] = 0;
							strcat(pLIPFile, ".lip");

							// and then get real location
							char pRealLIPFile[MAX_PATH];
							strcpy(pRealLIPFile, pLIPFile);
							GG_GetRealPath(pRealLIPFile, 0);

							// load in mouth shape data from LIP file
							if (s == 0) t.entityelement[t.e].lipset.clear();
							if (s == 1) t.entityelement[t.e].lipset1.clear();
							if (s == 2) t.entityelement[t.e].lipset2.clear();
							if (s == 3) t.entityelement[t.e].lipset3.clear();
							Dim(t.data_s, 9999);
							LoadArray(pRealLIPFile, t.data_s);
							for (t.l = 0; t.l <= 9999; t.l++)
							{
								t.line_s = t.data_s[t.l];
								LPSTR pLine = t.line_s.Get();
								if (Len(pLine) > 0)
								{
									char pTimeStr[32];
									char pMouthShapeStr[32];
									strcpy(pTimeStr, pLine);
									strcpy(pMouthShapeStr, "X");
									for (int n = 0; n < strlen(pTimeStr); n++)
									{
										if (pTimeStr[n] == 9)
										{
											pTimeStr[n] = 0;
											strcpy(pMouthShapeStr, pLine + n + 1);
											break;
										}
									}
									sCharacterCreatorPlusMouthData mouthDataShape;
									mouthDataShape.fTimeStamp = atof(pTimeStr);
									int iMouthShapeFrameIndex = 0;
									switch (pMouthShapeStr[0])
									{
									case 'A': iMouthShapeFrameIndex = 6;  break; // M
									case 'B': iMouthShapeFrameIndex = 2;  break; // K, S, T, EE
									case 'C': iMouthShapeFrameIndex = 1;  break; // AE 
									case 'D': iMouthShapeFrameIndex = 11; break; // AA
									case 'E': iMouthShapeFrameIndex = 7;  break; // AO
									case 'F': iMouthShapeFrameIndex = 9;  break; // W
									case 'G': iMouthShapeFrameIndex = 3;  break; // F V
									case 'H': iMouthShapeFrameIndex = 5;  break; // L
									case 'X': iMouthShapeFrameIndex = 0;  break; // CLOSED RELAXED MOUTH
									}
									mouthDataShape.iMouthShape = iMouthShapeFrameIndex;
									if (s == 0) t.entityelement[t.e].lipset.push_back(mouthDataShape);
									if (s == 1) t.entityelement[t.e].lipset1.push_back(mouthDataShape);
									if (s == 2) t.entityelement[t.e].lipset2.push_back(mouthDataShape);
									if (s == 3) t.entityelement[t.e].lipset3.push_back(mouthDataShape);
								}
							}
							UnDim(t.data_s);
						}
					}
				}
			}
		}
	}
}

void entity_cleargrideleprofrelationshipdata (void)
{
	// wipe out relational data when adding new object
	t.grideleprof.iObjectLinkID = 0;
	for (int i = 0; i < 10; i++)
	{
		t.grideleprof.iObjectRelationships[i] = 0;
		t.grideleprof.iObjectRelationshipsData[i] = 0;
		t.grideleprof.iObjectRelationshipsType[i] = 0;
	}
}

void entity_fillgrideleproffromprofile ( void )
{
	// Name
	t.grideleprof.name_s=t.entityprofileheader[t.entid].desc_s;

	t.grideleprof.blendmode = t.entityprofile[t.entid].blendmode;

	t.grideleprof.iFlattenID = -1; // never carries ID of individual elements
	if (!g_bEnableAutoFlattenSystem) //PE: If disabled always disable autoflatten.
		t.grideleprof.bAutoFlatten = false;

	// smart system to auto assign whether character is an ally or enemy
	t.grideleprof.iCharAlliance = 0;
	if ( strstr(t.grideleprof.name_s.Lower().Get(), "ally" ) != NULL )
	{
		t.grideleprof.iCharAlliance = 1;
	}

	// Group reference
	t.grideleprof.groupreference = t.entityprofile[t.entid].groupreference;

	// AI values
	t.grideleprof.aimain_s=t.entityprofile[t.entid].aimain_s;

	// AI use vars
	t.grideleprof.usekey_s=t.entityprofile[t.entid].usekey_s;
	t.grideleprof.ifused_s=t.entityprofile[t.entid].ifused_s;

	//  Spawn
	t.grideleprof.spawnatstart=t.entityprofile[t.entid].spawnatstart;
	t.grideleprof.spawnmax=t.entityprofile[t.entid].spawnmax;
	t.grideleprof.spawndelay=t.entityprofile[t.entid].spawndelay;
	t.grideleprof.spawnqty=t.entityprofile[t.entid].spawnqty;
	t.grideleprof.spawnupto=t.entityprofile[t.entid].spawnupto;
	t.grideleprof.spawnafterdelay=t.entityprofile[t.entid].spawnafterdelay;
	t.grideleprof.spawnwhendead=t.entityprofile[t.entid].spawnwhendead;
	t.grideleprof.spawndelayrandom=t.entityprofile[t.entid].spawndelayrandom;
	t.grideleprof.spawnqtyrandom=t.entityprofile[t.entid].spawnqtyrandom;
	t.grideleprof.spawnvel=t.entityprofile[t.entid].spawnvel;
	t.grideleprof.spawnvelrandom=t.entityprofile[t.entid].spawnvelrandom;
	t.grideleprof.spawnangle=t.entityprofile[t.entid].spawnangle;
	t.grideleprof.spawnanglerandom=t.entityprofile[t.entid].spawnanglerandom;
	t.grideleprof.spawnlife=t.entityprofile[t.entid].spawnlife;

	//  Scale, Cone
	t.grideleprof.scale=t.entityprofile[t.entid].scale;
	t.grideleprof.coneheight=t.entityprofile[t.entid].coneheight;
	t.grideleprof.coneangle=t.entityprofile[t.entid].coneangle;
	t.grideleprof.conerange=t.entityprofile[t.entid].conerange;

	//  Texture and Effect Data
	t.grideleprof.uniqueelement=0;
	t.grideleprof.texd_s=t.entityprofile[t.entid].texd_s;
	t.grideleprof.texaltd_s=t.entityprofile[t.entid].texaltd_s;
	t.grideleprof.effect_s=t.entityprofile[t.entid].effect_s;
	t.grideleprof.transparency=t.entityprofile[t.entid].transparency;
	t.grideleprof.castshadow=t.entityprofile[t.entid].castshadow;
	t.grideleprof.reducetexture=t.entityprofile[t.entid].reducetexture;

	//  Strength and Quantity
	t.grideleprof.strength=t.entityprofile[t.entid].strength;
	t.grideleprof.lives=t.entityprofile[t.entid].lives;
	t.grideleprof.isimmobile = t.entityprofile[t.entid].isimmobile;
	t.grideleprof.iscollectable = t.entityprofile[t.entid].iscollectable;
	t.grideleprof.lodmodifier=t.entityprofile[t.entid].lodmodifier;
	t.grideleprof.isocluder=t.entityprofile[t.entid].isocluder;
	t.grideleprof.isocludee=t.entityprofile[t.entid].isocludee;
	t.grideleprof.lootpercentage=t.entityprofile[t.entid].lootpercentage;
	t.grideleprof.colondeath=t.entityprofile[t.entid].colondeath;
	t.grideleprof.parententityindex=t.entityprofile[t.entid].parententityindex;
	t.grideleprof.parentlimbindex=t.entityprofile[t.entid].parentlimbindex;
	t.grideleprof.isviolent=t.entityprofile[t.entid].isviolent;
	t.grideleprof.cantakeweapon=t.entityprofile[t.entid].cantakeweapon;
	t.grideleprof.hasweapon_s=t.entityprofile[t.entid].hasweapon_s;
	t.grideleprof.quantity=t.entityprofile[t.entid].quantity;
	t.grideleprof.isobjective = t.entityprofile[t.entid].isobjective;
	t.grideleprof.hurtfall=t.entityprofile[t.entid].hurtfall;
	t.grideleprof.speed=t.entityprofile[t.entid].speed;
	t.grideleprof.animspeed=t.entityprofile[t.entid].animspeed;

	//  Decal and Sound Name
	t.grideleprof.voiceset_s=t.entityprofile[t.entid].voiceset_s;
	t.grideleprof.voicerate=t.entityprofile[t.entid].voicerate;
	t.grideleprof.soundset_s=t.entityprofile[t.entid].soundset_s;
	t.grideleprof.soundset1_s=t.entityprofile[t.entid].soundset1_s;
	t.grideleprof.soundset2_s=t.entityprofile[t.entid].soundset2_s;
	t.grideleprof.soundset3_s=t.entityprofile[t.entid].soundset3_s;
	t.grideleprof.soundset4_s=t.entityprofile[t.entid].soundset4_s;
	t.grideleprof.soundset5_s=t.entityprofile[t.entid].soundset5_s;
	t.grideleprof.soundset6_s=t.entityprofile[t.entid].soundset6_s;
	t.grideleprof.overrideanimset_s = "";

	//  FPGC - 310710 - decal particle settings
	t.particlefile_s = ""; //t.grideleprof.basedecal_s; //PE: Not used anymore.
	decal_getparticlefile ( );
	t.grideleprof.particleoverride=1;
	t.grideleprof.particle=g.gotparticle;

	//  Marker Data
	t.grideleprof.markerindex=t.entityprofile[t.entid].markerindex;
	t.grideleprof.light=t.entityprofile[t.entid].light;
	t.grideleprof.trigger=t.entityprofile[t.entid].trigger;
	t.grideleprof.usespotlighting=t.entityprofile[t.entid].usespotlighting;

	//  Data Extracted From GUN and FLAK
	t.tgunid_s=t.entityprofile[t.entid].isweapon_s;
	entity_getgunidandflakid ( );
	t.grideleprof.rateoffire=t.entityprofile[t.entid].rateoffire;
	t.grideleprof.weaponisammo=0;
	if (  t.tgunid>0 ) 
	{
		t.grideleprof.accuracy=g.firemodes[t.tgunid][0].settings.accuracy;
		t.grideleprof.reloadqty=g.firemodes[t.tgunid][0].settings.reloadqty;
		t.grideleprof.fireiterations=g.firemodes[t.tgunid][0].settings.iterate;
		t.grideleprof.usespotlighting=g.firemodes[t.tgunid][0].settings.usespotlighting;
		if (  t.tflakid == 0 ) 
		{
			t.grideleprof.damage=g.firemodes[t.tgunid][0].settings.damage;
			t.grideleprof.range=g.firemodes[t.tgunid][0].settings.range;
			t.grideleprof.dropoff = g.firemodes[t.tgunid][0].settings.dropoff;
			t.grideleprof.clipcapacity = g.firemodes[t.tgunid][0].settings.clipcapacity;
			t.grideleprof.weaponpropres1 = g.firemodes[t.tgunid][0].settings.weaponpropres1;
			t.grideleprof.weaponpropres2 = g.firemodes[t.tgunid][0].settings.weaponpropres2;
		}
		else
		{
			t.grideleprof.damage=0;
			t.grideleprof.lifespan=0;
			t.grideleprof.throwspeed=0;
			t.grideleprof.throwangle=0;
			t.grideleprof.bounceqty=0;
			t.grideleprof.explodeonhit=0;
			t.grideleprof.weaponisammo=t.tflakid;
		}
	}

	// Collision Data Overide
	t.grideleprof.iOverrideCollisionMode = -1;

	// Physics Data
	t.grideleprof.physics=t.entityprofile[t.entid].physics;
	t.grideleprof.phyalways=t.entityprofile[t.entid].phyalways;
	t.grideleprof.phyweight=t.entityprofile[t.entid].phyweight;
	t.grideleprof.phyfriction=t.entityprofile[t.entid].phyfriction;
	t.grideleprof.phyforcedamage=t.entityprofile[t.entid].phyforcedamage;
	t.grideleprof.rotatethrow=t.entityprofile[t.entid].rotatethrow;
	t.grideleprof.explodable=t.entityprofile[t.entid].explodable;
	t.grideleprof.explodedamage=t.entityprofile[t.entid].explodedamage;
	t.grideleprof.explodeheight =t.entityprofile[t.entid].explodeheight;
	t.grideleprof.explodable_decalname = t.entityprofile[t.entid].explodable_decalname;
	
	// 301115 - data extracted from neighbors (LOD Modifiers are shared across all parent copies)
	int iThisBankIndex = t.entid;
	if ( t.entityprofile[iThisBankIndex].addhandlelimb==0 )
	{
		for ( int e=1; e<=g.entityelementlist; e++ )
		{
			if ( t.entityelement[e].bankindex==iThisBankIndex )
			{
				t.grideleprof.lodmodifier = t.entityelement[e].eleprof.lodmodifier;
				break;
			}
		}
	}

	//PE: Make sure when we create we use default variables in eleprof.
	// Users don't want to have to enable custom materials before editing materials
	// ...non-custom materials are also causing issues with emissive, so just enable by default
	// LB: Preben, OLDFLAK argues that users WANT this off by default and was the previous default behavior
	// so we will make this false for default and fix any new issues that may arise, including the new demand
	// that other users want if ON by default :)  Perhaps something in editor pref settings ;)
	// LB: Additional - Preben notes doing the above breaks many things, including FPE material settings, so restore and rethink
	// PE: Keep it as is , and now use bUseFPESettings to control what will get updated.
	t.grideleprof.bCustomWickedMaterialActive = true;
	t.grideleprof.WEMaterial = t.entityprofile[t.entid].WEMaterial;

	//Need default particle setup here. or if will use the last inside "t.grideleprof".
	t.grideleprof.newparticle.emitterid = -1;
	t.grideleprof.newparticle.emittername = "particlesbank/default";

	// wipe out relational data when adding new object
	entity_cleargrideleprofrelationshipdata();

	// when first load an object, need to populate sound4 so _properties is ALWAYS called!
	cstr script_name = "scriptbank\\";
	script_name += t.grideleprof.aimain_s;
	extern void ParseLuaScript(entityeleproftype *tmpeleprof, char * script);
	ParseLuaScript(&t.grideleprof, script_name.Get());
}

void entity_updatetextureandeffectfromeleprof ( void )
{

	//  Texture and Effect (use entityprofile loader)
	t.storeentdefaults=t.entityprofile[t.entid];
	t.entityprofile[t.entid].texd_s=t.entityelement[t.e].eleprof.texd_s;
	t.entityprofile[t.entid].texaltd_s=t.entityelement[t.e].eleprof.texaltd_s;
	t.entityprofile[t.entid].texdid=t.entityelement[t.e].eleprof.texdid;
	t.entityprofile[t.entid].texaltdid=t.entityelement[t.e].eleprof.texaltdid;
	t.entityprofile[t.entid].effect_s=t.entityelement[t.e].eleprof.effect_s;
	t.entityprofile[t.entid].iscollectable =t.entityelement[t.e].eleprof.iscollectable;
	t.entityprofile[t.entid].texnid=t.entityelement[t.e].eleprof.texnid;
	t.entityprofile[t.entid].texsid=t.entityelement[t.e].eleprof.texsid;
	t.entityprofile[t.entid].texidmax=t.entityelement[t.e].eleprof.texidmax;
	t.entityprofile[t.entid].transparency=t.entityelement[t.e].eleprof.transparency;
	t.entityprofile[t.entid].reducetexture=t.entityelement[t.e].eleprof.reducetexture;
	entity_loadtexturesandeffect ( );
	t.entityelement[t.e].eleprof.texd_s=t.entityprofile[t.entid].texd_s;
	t.entityelement[t.e].eleprof.texaltd_s=t.entityprofile[t.entid].texaltd_s;
	t.entityelement[t.e].eleprof.texdid=t.entityprofile[t.entid].texdid;
	t.entityelement[t.e].eleprof.texaltdid=t.entityprofile[t.entid].texaltdid;
	t.entityelement[t.e].eleprof.effect_s=t.entityprofile[t.entid].effect_s;
	t.entityelement[t.e].eleprof.iscollectable =t.entityprofile[t.entid].iscollectable;
	t.entityelement[t.e].eleprof.texnid=t.entityprofile[t.entid].texnid;
	t.entityelement[t.e].eleprof.texsid=t.entityprofile[t.entid].texsid;
	t.entityelement[t.e].eleprof.texidmax=t.entityprofile[t.entid].texidmax;
	t.entityelement[t.e].eleprof.transparency=t.entityprofile[t.entid].transparency;
	t.entityelement[t.e].eleprof.reducetexture=t.entityprofile[t.entid].reducetexture;
	t.entityprofile[t.entid]=t.storeentdefaults;
}

void entity_updatetextureandeffectfromgrideleprof ( void )
{
	//  Texture and Effect (use entityprofile loader)
	//t.storeentdefaults as entityprofiletype;
	t.storeentdefaults=t.entityprofile[t.entid];
	t.entityprofile[t.entid].texd_s=t.grideleprof.texd_s;
	t.entityprofile[t.entid].texaltd_s=t.grideleprof.texaltd_s;
	t.entityprofile[t.entid].texdid=t.grideleprof.texdid;
	t.entityprofile[t.entid].texaltdid=t.grideleprof.texaltdid;
	t.entityprofile[t.entid].effect_s=t.grideleprof.effect_s;
	t.entityprofile[t.entid].iscollectable =t.grideleprof.iscollectable;
	t.entityprofile[t.entid].texnid=t.grideleprof.texnid;
	t.entityprofile[t.entid].texsid=t.grideleprof.texsid;
	t.entityprofile[t.entid].texidmax=t.grideleprof.texidmax;
	t.entityprofile[t.entid].transparency=t.grideleprof.transparency;
	t.entityprofile[t.entid].reducetexture=t.grideleprof.reducetexture;
	entity_loadtexturesandeffect ( );
	t.grideleprof.texd_s=t.entityprofile[t.entid].texd_s;
	t.grideleprof.texaltd_s=t.entityprofile[t.entid].texaltd_s;
	t.grideleprof.texdid=t.entityprofile[t.entid].texdid;
	t.grideleprof.texaltdid=t.entityprofile[t.entid].texaltdid;
	t.grideleprof.effect_s=t.entityprofile[t.entid].effect_s;
	t.grideleprof.iscollectable =t.entityprofile[t.entid].iscollectable;
	t.grideleprof.texnid=t.entityprofile[t.entid].texnid;
	t.grideleprof.texsid=t.entityprofile[t.entid].texsid;
	t.grideleprof.texidmax=t.entityprofile[t.entid].texidmax;
	t.grideleprof.transparency=t.entityprofile[t.entid].transparency;
	t.grideleprof.reducetexture=t.entityprofile[t.entid].reducetexture;
	t.entityprofile[t.entid]=t.storeentdefaults;
}

void entity_getgunidandflakid ( void )
{
	//  Use Weapon Name to get GUNID and FLAKID
	if (  t.tgunid_s != "" ) 
	{
		//  get gun
		t.findgun_s=Lower(t.tgunid_s.Get());
		gun_findweaponindexbyname ( );
		t.tgunid=t.foundgunid;
		//  no flak - old system
		t.tflakid=0;
	}
	else
	{
		t.tgunid=0 ; t.tflakid=0;
	}
}

void entity_loadtexturesandeffect ( void )
{
	//  If entity object not exist, reset var
	if (  t.entobj>0 ) 
	{
		if (  ObjectExist(t.entobj) == 0  )  t.entobj = 0;
	}

	//  Only characters need a higher quality texture, rest use divide standard settings
	t.tfullorhalfdivide=0;
	if (  t.segobjusedformapeditor == 0 ) 
	{
		if (  t.entityprofile[t.entid].ischaracter == 1 ) 
		{
			t.tfullorhalfdivide=2;
		}
		else
		{
			if (  t.entityprofile[t.entid].reducetexture != 0 ) 
			{
				if (  t.entityprofile[t.entid].reducetexture == -1 ) 
				{
					t.tfullorhalfdivide=1;
				}
				else
				{
					t.tfullorhalfdivide=2;
				}
			}
		}
	}

	//  Apply TEXTURE to entity object
	bool bMultiMaterialObject = false;
	t.tuseeffecttexture=0;
	t.texdir_s = "";
	t.texaltdir_s = "";
	t.tfile_s=t.entityprofile[t.entid].texd_s;
	t.tfilealt_s=t.entityprofile[t.entid].texaltd_s;
	if (t.tfile_s != "")
	{
		if (t.entityprofile[t.entid].texpath_s != "")
		{
			t.texdir_s = t.entityprofile[t.entid].texpath_s + t.tfile_s;
			t.texaltdir_s = t.entityprofile[t.entid].texpath_s + t.tfilealt_s;
		}
		else
		{
			// wicked relies on accurate texture path and name entries, and a Classic bug caused an error
			// when a full texture path was already provided, i.e. gamecore\guns\modern, etc
			bool bPathInsideFilename = false;
			LPSTR pFilenamePart = t.tfile_s.Get();
			for (int n = 0; n < strlen(pFilenamePart); n++)
				if (pFilenamePart[n] == '\\' || pFilenamePart[n] == '/')
					bPathInsideFilename = true;
			if ( bPathInsideFilename == true )
			{
				// path is already provided in t.tfile_s!
				t.texdir_s = t.tfile_s;
				t.texaltdir_s = t.tfilealt_s;
			}
			else
			{
				// regular assembly of entity folder, entity path and entity filename only
				t.texdir_s = t.entdir_s + t.entpath_s + t.tfile_s;
				t.texaltdir_s = t.entdir_s + t.entpath_s + t.tfilealt_s;
			}
		}
	}
	// Wicked does not support old method of loading images/effects
	t.entityprofile[t.entid].texaltdid=0;

	// Texture and apply effect
	// remove '.dds' from texture filename
	char pNoExtFilename[1024];
	strcpy ( pNoExtFilename, t.texdir_s.Get() );
	if (strlen(pNoExtFilename) > 4)
	{
		// strip .DDS/.JPG/etc from filename
		pNoExtFilename[strlen(pNoExtFilename) - 4] = 0;

		// for valid objects
		sObject* pObject = GetObjectData(t.entobj);
		if (pObject)
		{
			// Leave texture filename alone with .DDS/.PNG intact (not a WickedPBR texture set designation)
			t.texdirnoext_s = t.texdir_s;

			// go through all meshes in object and texture them
			bool bApplyTexture = false;
			for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
			{
				sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
				if (pMesh)
				{
					if (pMesh->dwTextureCount > 0)
					{
						// if not enough textures create required number
						if (pMesh->dwTextureCount < GG_MESH_TEXTURE_SURFACE)
						{
							extern bool EnsureTextureStageValid (sMesh* pMesh, int iTextureStage);
							EnsureTextureStageValid (pMesh, GG_MESH_TEXTURE_SURFACE);
						}

						// determine if base texture is a _color, in which case we can organize the texture array properly
						if (strnicmp (pNoExtFilename + strlen(pNoExtFilename) - 6, "_color", 6) == NULL)
						{
							// strip _color from pNoExtFilename
							pNoExtFilename[strlen(pNoExtFilename) - 6] = 0;

							// construct cousin texture references
							if (pMesh->dwTextureCount >= GG_MESH_TEXTURE_SURFACE)
							{
								strcpy(pMesh->pTextures[GG_MESH_TEXTURE_DIFFUSE].pName, pNoExtFilename);
								strcat(pMesh->pTextures[GG_MESH_TEXTURE_DIFFUSE].pName, "_color.dds");
								strcpy(pMesh->pTextures[GG_MESH_TEXTURE_NORMAL].pName, pNoExtFilename);
								strcat(pMesh->pTextures[GG_MESH_TEXTURE_NORMAL].pName, "_normal.dds");
								strcpy(pMesh->pTextures[GG_MESH_TEXTURE_ROUGHNESS].pName, pNoExtFilename);
								strcat(pMesh->pTextures[GG_MESH_TEXTURE_ROUGHNESS].pName, "_surface.dds");
								pMesh->pTextures[GG_MESH_TEXTURE_ROUGHNESS].channelMask = (15) + (1 << 4);
								strcpy(pMesh->pTextures[GG_MESH_TEXTURE_METALNESS].pName, pNoExtFilename);
								strcat(pMesh->pTextures[GG_MESH_TEXTURE_METALNESS].pName, "_surface.dds");
								pMesh->pTextures[GG_MESH_TEXTURE_METALNESS].channelMask = (15) + (2 << 4);
								strcpy(pMesh->pTextures[GG_MESH_TEXTURE_OCCLUSION].pName, pNoExtFilename);
								strcat(pMesh->pTextures[GG_MESH_TEXTURE_OCCLUSION].pName, "_surface.dds");
								pMesh->pTextures[GG_MESH_TEXTURE_OCCLUSION].channelMask = (15) + (0 << 4);
								strcpy(pMesh->pTextures[GG_MESH_TEXTURE_EMISSIVE].pName, pNoExtFilename);
								strcat(pMesh->pTextures[GG_MESH_TEXTURE_EMISSIVE].pName, "_emissive.dds");
								strcpy(pMesh->pTextures[GG_MESH_TEXTURE_SURFACE].pName, pNoExtFilename);
								strcat(pMesh->pTextures[GG_MESH_TEXTURE_SURFACE].pName, "_surface.dds");
							}
						}
						else
						{
							// simple base texture
							strcpy(pMesh->pTextures[0].pName, t.texdirnoext_s.Get());
						}
						bApplyTexture = true;
					}
				}
			}
			if (bApplyTexture == true)
			{
				WickedSetEntityId(t.entid);
				WickedCall_TextureObject(pObject, NULL);
				WickedSetEntityId(-1);
			}
		}
	}

	// Set any entity transparenct
	if ( t.entobj>0 ) 
	{
		if (t.entityprofile[t.entid].transparency >= 0)
		{
			WickedSetEntityId(t.entid);
			SetObjectTransparency(t.entobj, t.entityprofile[t.entid].transparency);
			WickedSetEntityId(-1);
		}
	}

	// Set entity culling (added COLLMODE 300114)
	if (t.entityprofile[t.entid].cullmode >= 0)
	{
		// For Wicked, cull mode controlled per-mesh with parent default as normal 
		
		//PE: Prefer WEMaterial over old cullmode
		bool bUseWEMaterial = false;
		if (t.entityprofile[t.entid].WEMaterial.MaterialActive)
		{
			WickedSetEntityId(t.entid);
			WickedSetElementId(0);
			sObject* pObject = g_ObjectList[t.entobj];
			if (pObject)
			{
				bUseWEMaterial = true;
				for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
				{
					sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
					if (pMesh)
					{
						// set properties of mesh
						WickedSetMeshNumber(iMeshIndex);
						bool bDoubleSided = WickedDoubleSided();
						if (bDoubleSided)
						{
							pMesh->bCull = false;
							pMesh->iCullMode = 0;
							WickedCall_SetMeshCullmode(pMesh);
						}
						else
						{
							pMesh->iCullMode = 1;
							pMesh->bCull = true;
							WickedCall_SetMeshCullmode(pMesh);
						}
					}
				}
			}
			WickedSetEntityId(-1);
		}

		if(!bUseWEMaterial)
		{
			SetObjectCull(t.entobj, 1);
		}
	}

	// Set cull mode for limbs (if hair specified)
	if ( t.entityprofile[t.entid].hairframestart != -1 )
	{
		for ( int tlmb = t.entityprofile[t.entid].hairframestart; tlmb <= t.entityprofile[t.entid].hairframefinish; tlmb++ )
		{
			if ( LimbExist ( t.entobj, tlmb ) == 1 )
			{
				SetLimbCull ( t.entobj, tlmb, 0 );
			}
		}
	}

	// hide specified limbs
	if ( t.entityprofile[t.entid].hideframestart != -1 )
	{
		for ( int tlmb = t.entityprofile[t.entid].hideframestart; tlmb <= t.entityprofile[t.entid].hideframefinish; tlmb++ )
		{
			if ( LimbExist ( t.entobj, tlmb ) == 1 )
			{
				ExcludeLimbOn ( t.entobj, tlmb );
			}
		}
	}

	//PE: Below will enable additive i wicked.

}

//PE: Faster loading
#define USEFASTLOADING

#ifdef USEFASTLOADING
char * c_data = NULL;
char * c_data_pointer = NULL;
int c_data_size = 0;
HANDLE c_hFile;
extern DBPRO_GLOBAL char m_pWorkString[_MAX_PATH];
DARKSDK LPSTR GetReturnStringFromWorkString(char* WorkString = m_pWorkString);

void c_OpenToRead(int f, LPSTR pFilename)
{
	//Read everything.

	HANDLE hreadfile = GG_CreateFile(pFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hreadfile != INVALID_HANDLE_VALUE)
	{
		int filebuffersize = GetFileSize(hreadfile, NULL);
		c_data = new char[filebuffersize+256];
		// Read file into memory
		DWORD bytesread;
		ReadFile(hreadfile, c_data, filebuffersize, &bytesread, NULL);
		CloseHandle(hreadfile);
		c_data_pointer = c_data;
		c_data_size = filebuffersize;
	}
	else
	{
		c_data = NULL;
		c_data_pointer = c_data;
		c_data_size = 0;
	}

}
//delete(c_data);

bool c_ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
	if ( ((c_data_pointer-c_data) + nNumberOfBytesToRead) > c_data_size)
		return false;
	memcpy(lpBuffer, c_data_pointer, nNumberOfBytesToRead);
	c_data_pointer += nNumberOfBytesToRead;
	*lpNumberOfBytesRead = nNumberOfBytesToRead;
	return true;
}

int c_ReadLong(int f)
{
	if (!c_data) return 0;
	int iResult = 0;
	DWORD bytes;
	// Read from file
	DWORD data;
	if (c_ReadFile(c_hFile, &data, sizeof(data), &bytes, NULL) == 0)
		return(0); // RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
	iResult = data;
	return iResult;
}


void c_CloseFile(int f)
{
	if(c_data) delete(c_data);
	c_data = NULL;
	c_data_pointer = c_data;
	c_data_size = 0;
}

int c_ReadByte(int f)
{
	if (!c_data) return 0;

	int iResult = 0;
	DWORD bytes;
	// Read from file
	unsigned char data;
	if (c_ReadFile(c_hFile, &data, sizeof(data), &bytes, NULL) == 0)
		return(0); //RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);

	iResult = data;
	return iResult;
}

int c_ReadWord(int f)
{
	if (!c_data) return 0;

	int iResult = 0;
	DWORD bytes;
	// Read from file
	WORD data;
			
	if (c_ReadFile(c_hFile, &data, sizeof(data), &bytes, NULL) == 0)
		return(0); //RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);

	iResult = data;
	return iResult;
}


float c_ReadFloat(int f)
{
	if (!c_data) return 0;

	float fResult = 0.0f;
	DWORD bytes;
	// Read from file
	float data;
	if (c_ReadFile(c_hFile, &data, sizeof(data), &bytes, NULL) == 0)
		return(0); //RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);

	fResult = data;

	return fResult;
}

char c_filerror[2] = "";

//PE: We use cstr t.a_s = c_ReadString ( 1 ) , so mem is never freed.
char c_ReadStringRing[20][1024];
int c_ReadStringCount = 0;
LPSTR c_ReadString(int f)
{
	if (!c_data) return 0;

	c_ReadStringCount = c_ReadStringCount + 1;
	if (c_ReadStringCount > 19)
		c_ReadStringCount = 0;
	
	LPSTR pReturnString = 0;

	unsigned char c = 0;
	DWORD bytes;
	std::vector<char> WorkString;

	bool eof = false;
	do
	{
		if (c_ReadFile(c_hFile, &c, 1, &bytes, NULL) == 0)
		{
			//RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			return(&c_filerror[0]);
		}
		if (bytes == 0)
		{
			eof = true;
		}
		else if (c >= 32 || c == 9)
		{
			WorkString.push_back(c);
		}
	} while ((c >= 32 || c == 9) && !eof);

	WorkString.push_back(0);

	if (c == 13)
	{
		if (c_ReadFile(c_hFile, &c, 1, &bytes, NULL) == 0)
		{
			//RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			return(&c_filerror[0]);
		}
	}

	// Create and return string
	if (WorkString.size() < 1024)
	{
		strcpy(&c_ReadStringRing[c_ReadStringCount][0], &WorkString[0]);
		pReturnString = &c_ReadStringRing[c_ReadStringCount][0];
	}
	else
		pReturnString = GetReturnStringFromWorkString(&WorkString[0]);

	return pReturnString;
}

LPSTR c_ReadStringOLD(int f)
{
	if (!c_data) return 0;

	LPSTR pReturnString = 0;

	unsigned char c = 0;
	DWORD bytes;
	std::vector<char> WorkString;

	bool eof = false;
	do
	{
		if (c_ReadFile(c_hFile, &c, 1, &bytes, NULL) == 0)
		{
			//RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			return(&c_filerror[0]);
		}
		if (bytes == 0)
		{
			eof = true;
		}
		else if (c >= 32 || c == 9)
		{
			WorkString.push_back(c);
		}
	} while ((c >= 32 || c == 9) && !eof);

	WorkString.push_back(0);

	if (c == 13)
	{
		if (c_ReadFile(c_hFile, &c, 1, &bytes, NULL) == 0)
		{
			//RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			return(&c_filerror[0]);
		}
	}

	// Create and return string
	pReturnString = GetReturnStringFromWorkString(&WorkString[0]);

	return pReturnString;
}

//PE: We can have 0x0a in soundset4 (entered text) so always use 13 to stop.
LPSTR c_ReadStringIncl0xA(int f)
{
	if (!c_data) return 0;

	c_ReadStringCount = c_ReadStringCount + 1;
	if (c_ReadStringCount > 19)
		c_ReadStringCount = 0;

	LPSTR pReturnString = 0;

	unsigned char c = 0;
	DWORD bytes;
	std::vector<char> WorkString;

	bool eof = false;
	do
	{
		if (c_ReadFile(c_hFile, &c, 1, &bytes, NULL) == 0)
		{
			//RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			return(&c_filerror[0]);
		}
		if (bytes == 0)
		{
			eof = true;
		}
		else if (c >= 32 || c == 9 || c == 10)
		{
			WorkString.push_back(c);
		}
	} while ((c >= 32 || c == 9 || c == 10) && !eof);

	WorkString.push_back(0);

	if (c == 13)
	{
		if (c_ReadFile(c_hFile, &c, 1, &bytes, NULL) == 0)
		{
			//RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			return(&c_filerror[0]);
		}
	}

	// Create and return string
	if (WorkString.size() < 1024)
	{
		strcpy(&c_ReadStringRing[c_ReadStringCount][0], &WorkString[0]);
		pReturnString = &c_ReadStringRing[c_ReadStringCount][0];
	}
	else
		pReturnString = GetReturnStringFromWorkString(&WorkString[0]);
	return pReturnString;
}

LPSTR c_ReadStringIncl0xAOLD(int f)
{
	if (!c_data) return 0;


	LPSTR pReturnString = 0;

	unsigned char c = 0;
	DWORD bytes;
	std::vector<char> WorkString;

	bool eof = false;
	do
	{
		if (c_ReadFile(c_hFile, &c, 1, &bytes, NULL) == 0)
		{
			//RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			return(&c_filerror[0]);
		}
		if (bytes == 0)
		{
			eof = true;
		}
		else if (c >= 32 || c == 9 || c == 10)
		{
			WorkString.push_back(c);
		}
	} while ((c >= 32 || c == 9 || c == 10) && !eof);

	WorkString.push_back(0);

	if (c == 13)
	{
		if (c_ReadFile(c_hFile, &c, 1, &bytes, NULL) == 0)
		{
			//RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			return(&c_filerror[0]);
		}
	}

	// Create and return string
	pReturnString = GetReturnStringFromWorkString(&WorkString[0]);
	return pReturnString;
}

