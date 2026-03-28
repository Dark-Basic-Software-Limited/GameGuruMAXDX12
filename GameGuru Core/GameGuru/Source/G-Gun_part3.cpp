void gun_load ( void )
{
	// Custom Arms system
	bool bCanBeCustomized = false;
	bool bForceRefreshWeapon = false;
	cstr customArms_s = g_guns_customArms_s;
	cstr pHUDDAT_s = cstr("gamecore\\") + g.fpgchuds_s + "\\" + t.gun_s + "\\resources\\hud.dat";
	cstr HUDCustom_s = cstr("gamecore\\") + g.fpgchuds_s + "\\" + t.gun_s + "\\hudcustom.txt";
	cstr legacygunfile_s = cstr("gamecore\\") + g.fpgchuds_s + "\\" + t.gun_s + "\\HUD-ORIG.dbo";
	char pHUDCustomForThisWeapon[MAX_PATH];
	strcpy(pHUDCustomForThisWeapon, HUDCustom_s.Get());
	GG_GetRealPath(pHUDCustomForThisWeapon, 1);
	if (FileExist(pHUDDAT_s.Get()) == 1 || FileExist(legacygunfile_s.Get()) == 1)
	{
		bCanBeCustomized = true;
		if (customArms_s.Len() > 0)
		{
			// if different from previous custom model
			if(FileExist(pHUDCustomForThisWeapon)==1)
			{
				// if choice made, read and compare with current
				OpenToRead(4, pHUDCustomForThisWeapon);
				cstr pChoice = ReadString(4);
				CloseFile(4);
				if (stricmp(pChoice.Get(), g_guns_customArms_s.Get()) != NULL)
				{
					// if not same, we need to regenerate and make new model
					bForceRefreshWeapon = true;
				}
			}
			else
			{
				// first time
				bForceRefreshWeapon = true;
			}
		}
	}

	// do not load gun if already present
	if (bForceRefreshWeapon == false)
	{
		if (t.gun[t.gunid].obj > 0)
		{
			if (ObjectExist(t.gun[t.gunid].obj) == 1)
			{
				// no need to reload
				return;
			}
			else
			{
				// gun data present, object missing
				t.gun[t.gunid].obj = 0;
			}
		}
	}

	// the HUD to load
	t.currentgunfile_s = "gamecore\\";
	t.currentgunfile_s += g.fpgchuds_s + "\\" + t.gun_s + "\\HUD.dbo";
	t.currentgunobj = loadgun(t.gunid, t.currentgunfile_s.Get());

	// check if legacy weapon supporting arm swap trick (thanks BOND1!)
	int iGunSecondaryObj = 0;
	if (t.currentgunobj > 0)
	{
		iGunSecondaryObj = g.gunbankextraobjoffset + (t.currentgunobj - g.gunbankoffset);
		if (ObjectExist(iGunSecondaryObj) == 1) DeleteObject(iGunSecondaryObj);
	}
	bool bUsingLegacyArmReplacementTrick = false;
	if (customArms_s.Len() > 0)
	{
		if (FileExist(legacygunfile_s.Get()) == 1)
		{
			// load secondary hands 
			cstr pHands_s = g.fpscrootdir_s + cstr("\\Files\\gamecore\\hands\\") + customArms_s + cstr("\\arms.dbo");
			if(FileExist(pHands_s.Get())==0)
			{
				// player start marker specified hands not exist on this install, so default to legacy ones
				customArms_s = "Combat Gloves Light";
				pHands_s = g.fpscrootdir_s + cstr("\\Files\\gamecore\\hands\\") + customArms_s + cstr("\\arms.dbo");
			}
			if (FileExist(pHands_s.Get()) == 1)
			{
				// load the hands
				LoadObject(pHands_s.Get(), iGunSecondaryObj);

				// texture these seoncdary arms/hands
				cstr pCustomArms_s = cstr("gamecore\\hands\\") + customArms_s + cstr("\\arms_color.dds");
				if (ImageExist(g.weaponstempimageoffset) == 1) DeleteImage(g.weaponstempimageoffset);
				LoadImage(pCustomArms_s.Get(), g.weaponstempimageoffset);
				TextureObject(iGunSecondaryObj, g.weaponstempimageoffset);

				// apply correct legacy animations
				cstr pAbsPathToAnim = g.fpscrootdir_s + cstr("\\Files\\gamecore\\hands\\Animations\\Legacy");
				char pNoSpacesInGunName[MAX_PATH];
				strcpy(pNoSpacesInGunName, "");
				int n2 = 0;
				LPSTR pGunPathAndName = t.gun_s.Get();
				for (int n = 0; n < strlen(pGunPathAndName); n++)
				{
					if (pGunPathAndName[n] == ' ' || pGunPathAndName[n] == '\\' || pGunPathAndName[n] == '/')
					{
					}
					else
					{
						pNoSpacesInGunName[n2++] = pGunPathAndName[n];
					}
				}
				pNoSpacesInGunName[n2] = 0;

				if (t.gun[t.gunid].legacy_animation_s.Len() > 0)
				{
					cstr path = pAbsPathToAnim;
					path += t.gun[t.gunid].legacy_animation_s;
					path += ".dbo";
					if (!FileExist(path.Get()))
					{
						pAbsPathToAnim += pNoSpacesInGunName;// "EnhancedAK";
						pAbsPathToAnim += ".dbo";
					}
					else
					{
						pAbsPathToAnim = path;
					}
				}
				else
				{
					pAbsPathToAnim += pNoSpacesInGunName;// "EnhancedAK";
					pAbsPathToAnim += ".dbo";
				}

				sObject* pSecondaryObject = GetObjectData(iGunSecondaryObj);
				if (pSecondaryObject && FileExist(pAbsPathToAnim.Get()))
				{
					if (AppendAnimationFromFile(pSecondaryObject, pAbsPathToAnim.Get(), 0) == true)
					{
						WickedCall_RefreshObjectAnimations(pSecondaryObject, pSecondaryObject->wickedloaderstateptr);
					}
					bUsingLegacyArmReplacementTrick = true;
				}
				else
				{
					//PE: Anim not there , cant replace arms.
					if (ObjectExist(iGunSecondaryObj) == 1)
						DeleteObject(iGunSecondaryObj);
					bUsingLegacyArmReplacementTrick = false;
				}
				if (ObjectExist(iGunSecondaryObj) == 1)
					HideObject(iGunSecondaryObj);
			}
		}
	}

	// Custom arms system
	if (t.currentgunobj != 0 && customArms_s.Len() > 0)
	{
		// Load gun (using custom arms)
		if (FileExist(pHUDDAT_s.Get()) == 1)
		{
			if (bForceRefreshWeapon == true)
			{
				// only force a refresh if needed (i.e. HUD.DBO not matching last custom hands choice)
				if (ObjectExist(t.currentgunobj) == 1) DeleteObject(t.currentgunobj);
				gun_createhud(customArms_s);

				// if successful creation, save the new HUD.DBO
				if (ObjectExist(t.currentgunobj) == 1)
				{
					// before save, match created animsets to actual DBO structure
					sObject* pObject = GetObjectData(t.currentgunobj);
					extern void UpdateObjectWithAnimSlotList (sObject*);
					UpdateObjectWithAnimSlotList(pObject);

					// do the save to writables
					char pTempHUDDBOFile[MAX_PATH];
					strcpy(pTempHUDDBOFile, t.currentgunfile_s.Get());
					GG_GetRealPath(pTempHUDDBOFile, 1);
					if (FileExist(pTempHUDDBOFile) == 1) DeleteFileA(pTempHUDDBOFile);
					SaveObject (pTempHUDDBOFile, t.currentgunobj);
					DeleteObject(t.currentgunobj);
					LoadObject(pTempHUDDBOFile, t.currentgunobj);
				}
			}
		}

		// record choice
		if (FileExist(pHUDCustomForThisWeapon) == 1) DeleteFileA(pHUDCustomForThisWeapon);
		OpenToWrite(4, pHUDCustomForThisWeapon);
		WriteString(4, g_guns_customArms_s.Get());
		CloseFile(4);
	}
	if (t.currentgunobj > 0)
	{
		gun_loaddata();
		preparegun(t.gunid, t.currentgunobj);
	}

	// Weapon is not valid, and in editing mode
	if (t.currentgunobj == 0 && t.game.gameisexe != 1)
	{
		// if no HUD.dbo, we may recreate it from the resources folder if present
		if (FileExist(pHUDDAT_s.Get()) == 1)
		{
			// prompt
			timestampactivity(0, "Constructing new HUD.dbo for weapon");

			// create HUD.dbo object (always with default arms provided by artist - see above for custom arms system)
			cstr noCustomArmsUseDefault_s = "";
			gun_createhud(noCustomArmsUseDefault_s);
			if (t.currentgunobj != 0)
			{
				// before save, match created animsets to actual DBO structure
				sObject* pObject = GetObjectData(t.currentgunobj);
				extern void UpdateObjectWithAnimSlotList (sObject*);
				UpdateObjectWithAnimSlotList(pObject);

				// for convenience, if HUD.dbo missing from root
				cstr rootHUDFile_s = g.fpscrootdir_s + cstr("\\Files\\gamecore\\") + g.fpgchuds_s + "\\" + t.gun_s + "\\hud.dbo";
				if (FileExist(rootHUDFile_s.Get()) == 0)
				{
					// save HUD.dbo in writables
					SaveObject (t.currentgunfile_s.Get(), t.currentgunobj);

					// copy new creation to root
					char pAbsPathToSourceHUDDBO[MAX_PATH];
					strcpy(pAbsPathToSourceHUDDBO, t.currentgunfile_s.Get());
					GG_GetRealPath(pAbsPathToSourceHUDDBO, 0);
					CopyFileA(pAbsPathToSourceHUDDBO, rootHUDFile_s.Get(), TRUE);
					if (FileExist(rootHUDFile_s.Get()) == 1)
					{
						// if copy successful, remove from writables area
						DeleteFileA(pAbsPathToSourceHUDDBO);
					}
				}
			}

			// need a fresh load
			if (ObjectExist(t.currentgunobj) == 1) DeleteObject(t.currentgunobj);

			// and load the newly created gun and HUD.DBO
			t.currentgunobj = loadgun(t.gunid, t.currentgunfile_s.Get());
			if (t.currentgunobj > 0)
			{
				gun_loaddata();
				preparegun(t.gunid, t.currentgunobj);
			}
		}
		else
		{
			// missing file!
			timestampactivity(0, "No HUD.dbo found for weapon");
		}
	}

	// leave if no gun model
	if (  t.currentgunobj  ==  0  )  return;
	if (  ObjectExist(t.currentgunobj)  ==  0  )  return;

	// assign obj to gun ref
	t.gun[t.gunid].obj=t.currentgunobj;
	sprintf ( t.szwork , "Load Gun:%s Obj:%i" , t.currentgunfile_s.Get() , t.currentgunobj );
	timestampactivity(0, t.szwork );

	//  Bullet feed can be made invisible as bullets used up
	if (  t.gun[t.gunid].settings.bulletmod == 1 ) 
	{
		t.gun[t.gunid].settings.bulletlimbstart=g.bulletlimbsmax+1;
		g.bulletlimbsmax += t.gun[t.gunid].settings.bulletlimbsmax;
		t.gun[t.gunid].settings.bulletlimbend=g.bulletlimbsmax;
		Dim (  t.bulletlimbs,g.bulletlimbsmax  );
		for ( t.p = 0 ; t.p <= t.gun[t.gunid].settings.bulletlimbsmax; t.p++ )
		{
			if (t.gun[t.gunid].settings.bulletlimbstart + t.p <= g.bulletlimbsmax)  t.bulletlimbs[t.gun[t.gunid].settings.bulletlimbstart + t.p] = -1;
		}
	}

	//  Perform scan to determine hotspot markers
	t.flashlimb=-1 ; t.brasslimb=-1 ; t.smokelimb=-1 ; t.handlimb=-1;
	t.flashlimb2=-1 ; t.brasslimb2=-1 ; t.smokelimb2=-1;
	t.flaklimb1=-1;
	PerformCheckListForLimbs ( t.currentgunobj );
	for ( t.c = 1 ; t.c <= ChecklistQuantity(); t.c++ )
	{
		t.name_s=Upper(ChecklistString(t.c));
		if (  t.name_s == "FIRESPOT"  )  t.flashlimb = t.c-1;
		if (  t.name_s == "X3DS_FIRESPOT"  )  t.flashlimb = t.c-1;
		if (  t.name_s == "FIRESPOT02"  )  t.flashlimb2 = t.c-1;
		if (  t.name_s == "BRASS"  )  t.brasslimb = t.c-1;
		if (  t.name_s == "X3DS_BRASS"  )  t.brasslimb = t.c-1;
		if (  t.name_s == "BRASS02"  )  t.brasslimb2 = t.c-1;
		if (  t.name_s == "_SMOKE"  )  t.smokelimb = t.c-1;
		if (  t.name_s == "SMOKE"  )  t.smokelimb = t.c-1;
		if (  t.name_s == "X3DS_SMOKE"  )  t.smokelimb = t.c-1;
		if (  t.name_s == "SMOKE02"  )  t.smokelimb2 = t.c-1;
		if (  t.name_s == "HAND"  )  t.handlimb = t.c-1;
		if (  t.name_s == "X3DS_HAND"  )  t.handlimb = t.c-1;
		if (  t.name_s == "ROCKET"  )  t.flaklimb1 = t.c-1;
		if (  t.name_s == "GUN_ROOT_BONE"  )  t.handlimb = t.c-1;
		if (  t.name_s == "GRENADE_COMBINED"  )  t.flaklimb1 = t.c-1;
		if (  t.gun[t.gunid].settings.bulletmod == 1 ) 
		{
			for ( t.p = 0 ; t.p<=  t.gun[t.gunid].settings.bulletlimbsmax; t.p++ )
			{
				cstr tempstring = "BULLET";
				tempstring += Str(t.p);
				if (t.name_s == tempstring && t.gun[t.gunid].settings.bulletlimbstart + t.p <= g.bulletlimbsmax)  t.bulletlimbs[t.gun[t.gunid].settings.bulletlimbstart + t.p] = t.c - 1;
			}
		}
		if (bUsingLegacyArmReplacementTrick == true)
		{
			if (stricmp(t.name_s.Get(), "LEGACYARMS" ) == NULL )
			{
				HideLimb(t.currentgunobj, t.c - 1);
			}
		}
	}
	g.firemodes[t.gunid][0].settings.flaklimb=t.flaklimb1;
	g.firemodes[t.gunid][1].settings.flaklimb=t.flaklimb1;

	// if new weapon system, can set limbs to -2 to autocalculate smoke and brass positions
	if (t.gun[t.gunid].handusesnewweaponsystem == 1)
	{
		if (t.smokelimb == -1) t.smokelimb = -2;
		if (t.brasslimb == -1) t.brasslimb = -2;
	}

	// Use neighbor limb if certain other limbs not in model
	if ( t.smokelimb == -1 ) 
	{
		if ( t.flashlimb != -1  )  t.smokelimb = t.flashlimb;
	}

	//  brass limb not specified, use smoke hole
	if (  t.brasslimb == -1 && t.smokelimb >= 0  )  t.brasslimb = t.smokelimb;

	//  Store limbs in limb-data
	t.gun[t.gunid].settings.flashlimb=t.flashlimb;
	t.gun[t.gunid].settings.brasslimb=t.brasslimb;
	t.gun[t.gunid].settings.handlimb=t.handlimb;
	t.gun[t.gunid].settings.smokelimb=t.smokelimb;
	t.gun[t.gunid].settings.flashlimb2=t.flashlimb2;
	t.gun[t.gunid].settings.brasslimb2=t.brasslimb2;
	t.gun[t.gunid].settings.smokelimb2=t.smokelimb2;

	//  Also revert to GUN_D.DDS if no texture specified BUT there are no
	//  valid textures built into the HUD model
	t.tfoundvalidinternaltexture=0;
	sObject* pObject = GetObjectData ( t.currentgunobj );
	if ( pObject )
	{
		for ( int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++ )
		{
			sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
			if ( pMesh )
			{
				if ( pMesh->dwTextureCount > 0 )
				{
					if ( WickedCall_GetMeshMaterialName(pMesh) != NULL )
					{
						// at least one texture was successfully loaded by the gun model (so prefer them)
						t.tfoundvalidinternaltexture = 1;
					}
				}
			}
		}
	}
	if ( t.tfoundvalidinternaltexture == 0 ) 
	{
		t.gun[t.gunid].texd_s = "gun_D.dds";
	}

	//  Determine number of frames per keyframe
	if (  t.keyframeratio>0 ) 
	{
		t.ratio_f=t.keyframeratio;
	}
	else
	{
		t.ratio_f=1;
	}
	//  Determine number of alt frames per keyframe
	if (  t.altkeyframeratio>0 ) 
	{
		t.altratio_f=t.altkeyframeratio;
	}
	else
	{
		t.altratio_f=1;
	}

	//  Adjust animation data based on actual number of keyframes
	for ( t.i = 0 ; t.i<=  1; t.i++ )
	{
		if (  t.i == 1  )  t.ratio_f = t.altratio_f;
		g.firemodes[t.gunid][t.i].action.show.s = g.firemodes[t.gunid][t.i].action.show.s * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.show.e = g.firemodes[t.gunid][t.i].action.show.e * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.idle.s = g.firemodes[t.gunid][t.i].action.idle.s * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.idle.e = g.firemodes[t.gunid][t.i].action.idle.e * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.move.s = g.firemodes[t.gunid][t.i].action.move.s * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.move.e = g.firemodes[t.gunid][t.i].action.move.e * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.run.s = g.firemodes[t.gunid][t.i].action.run.s * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.run.e = g.firemodes[t.gunid][t.i].action.run.e * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.flattentochest.s = g.firemodes[t.gunid][t.i].action.flattentochest.s * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.flattentochest.e = g.firemodes[t.gunid][t.i].action.flattentochest.e * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.start.s = g.firemodes[t.gunid][t.i].action.start.s * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.start.e = g.firemodes[t.gunid][t.i].action.start.e * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.automatic.s = g.firemodes[t.gunid][t.i].action.automatic.s * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.automatic.e = g.firemodes[t.gunid][t.i].action.automatic.e * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.finish.s = g.firemodes[t.gunid][t.i].action.finish.s * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.finish.e = g.firemodes[t.gunid][t.i].action.finish.e * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.laststart.s = g.firemodes[t.gunid][t.i].action.laststart.s * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.laststart.e = g.firemodes[t.gunid][t.i].action.laststart.e * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.lastfinish.s = g.firemodes[t.gunid][t.i].action.lastfinish.s * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.lastfinish.e = g.firemodes[t.gunid][t.i].action.lastfinish.e * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.startreload.s = g.firemodes[t.gunid][t.i].action.startreload.s * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.startreload.e = g.firemodes[t.gunid][t.i].action.startreload.e * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.reloadloop.s = g.firemodes[t.gunid][t.i].action.reloadloop.s * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.reloadloop.e = g.firemodes[t.gunid][t.i].action.reloadloop.e * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.endreload.s = g.firemodes[t.gunid][t.i].action.endreload.s * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.endreload.e = g.firemodes[t.gunid][t.i].action.endreload.e * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.cock.s = g.firemodes[t.gunid][t.i].action.cock.s * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.cock.e = g.firemodes[t.gunid][t.i].action.cock.e * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.hide.s = g.firemodes[t.gunid][t.i].action.hide.s * t.ratio_f;
		g.firemodes[t.gunid][t.i].action.hide.e = g.firemodes[t.gunid][t.i].action.hide.e * t.ratio_f;
	}

	// If no effect, use global weapon shader (new weapons)
	if ( Len(t.gun[t.gunid].effect_s.Get())<3 ) 
	{
		// standard shader
		t.gun[t.gunid].effect_s = "effectbank\\reloaded\\weapon_basic.fx";

		// 300718 - also OLD weapons did not specify shader, so boost diffuse as they are DNS textures in PBR shader
		// but some other weapons are PBR but don't specify shader, so tone down this auto correction!
		t.gun[t.gunid].boostintensity = 0.1f;
	}

	// If weapon used old entity shader, use new weapon one
	if ( t.gun[t.gunid].effect_s == "effectbank\\reloaded\\entity_basic.fx" ) t.gun[t.gunid].effect_s = "effectbank\\reloaded\\weapon_basic.fx";

	// Load Effect ( (default is weapon_basic shader) )
	if ( t.gun[t.gunid].effect_s != "" ) 
	{
		t.tfile_s=t.gun[t.gunid].effect_s;
		t.teffectid=loadinternaleffect(t.tfile_s.Get());
		if ( stricmp ( t.gun[t.gunid].effect_s.Get(), "effectbank\\reloaded\\weapon_bone.fx" ) == NULL ) 
		{
			// store weapon shader effect IDs for NoZDepth renderer which uses 'CutOutDepth' technique to chizzel out gun
			// from the depth buffer without needing to reset the depth surface (messing up SAO and other depth effects)
			if ( g_weaponboneshadereffectindex == 0 ) g_weaponboneshadereffectindex = t.teffectid;
		}
		if (  t.teffectid == 0 ) 
		{
			//  revert to standard weapon shader if custom specified not exist
			t.gun[t.gunid].effect_s="effectbank\\reloaded\\weapon_basic.fx";
			t.tfile_s=t.gun[t.gunid].effect_s;
			t.teffectid=loadinternaleffect(t.tfile_s.Get());
		}
		if ( stricmp ( t.gun[t.gunid].effect_s.Get(), "effectbank\\reloaded\\weapon_basic.fx" ) == NULL ) 
		{
			// store weapon shader effect IDs for NoZDepth renderer which uses 'CutOutDepth' technique to chizzel out gun
			// from the depth buffer without needing to reset the depth surface (messing up SAO and other depth effects)
			if ( g_weaponbasicshadereffectindex == 0 ) g_weaponbasicshadereffectindex = t.teffectid;
		}
	}
	else
	{
		t.teffectid=0;
	}

	//  Load in special non-bone effect to accompany standard bone shader
	t.teffectid2=0;
	if ( t.gun[t.gunid].effect_s == "effectbank\\reloaded\\weapon_basic.fx" ) 
	{
		t.tfile_s="effectbank\\reloaded\\weapon_bone.fx";
		t.teffectid2=t.teffectid;
		t.teffectid=loadinternaleffect(t.tfile_s.Get());

		// store weapon shader effect IDs for NoZDepth renderer which uses 'CutOutDepth' technique to chizzel out gun
		// from the depth buffer without needing to reset the depth surface (messing up SAO and other depth effects)
		if ( g_weaponboneshadereffectindex == 0 ) g_weaponboneshadereffectindex = t.teffectid;
	}

	// 301117 - ensure weapns with bones and nonbones are handled
	if ( t.gun[t.gunid].effect_s == "effectbank\\reloaded\\weapon_bone.fx" ) 
	{
		t.teffectid2 = loadinternaleffect("effectbank\\reloaded\\weapon_basic.fx");
		if ( g_weaponbasicshadereffectindex == 0 ) g_weaponbasicshadereffectindex = t.teffectid2;
	}

	//  Reset gun textures
	t.gun[t.gunid].texdid=0;
	t.gun[t.gunid].texnid=0;
	t.gun[t.gunid].texmid=0;
	t.gun[t.gunid].texgid=0;
	t.gun[t.gunid].texaoid=0;
	t.gun[t.gunid].texiid=0;
	t.gun[t.gunid].texhid=0;

	//  First Textures are PLATES
	if ( 1 ) // always texture weapon 
	{
		//  if texd$ specified, use that as the single texture to use
		cstr timgGloss_s = "";
		cstr timgAO_s = "";
		cstr timgHeight_s = "";
		t.tguntextureoverride=1;
		t.tguntexture_s=t.gun[t.gunid].texd_s;
		if ( Len(t.tguntexture_s.Get())<2 ) t.tguntextureoverride = 0;
		t.tguntexture_s=Left(t.tguntexture_s.Get(),Len(t.tguntexture_s.Get())-Len("_D.dds"));
		timestampactivity(0, "Loading gun textures" );
		if ( t.tguntextureoverride == 1 ) 
		{
			cstr pGunPath = ""; 
			pGunPath = pGunPath+"gamecore\\"+g.fpgchuds_s+"\\"+t.gun_s+"\\";
			if (  g.gdividetexturesize == 0 ) 
			{
				t.imgD_s="effectbank\\reloaded\\media\\white_D.dds";
			}
			else
			{
				t.imgD_s = pGunPath + t.tguntexture_s+"_D.dds";
				if ( FileExist(t.imgD_s.Get()) == 0 ) t.imgD_s = pGunPath + t.tguntexture_s+"_color.dds";
				if ( FileExist(t.imgD_s.Get()) == 0 ) t.imgD_s = "effectbank\\reloaded\\media\\white_D.dds";
			}
			timestampactivity(0, cstr(cstr("Color=")+t.imgD_s).Get() );
			t.imgN_s = pGunPath + t.tguntexture_s+"_N.dds";
			if ( FileExist(t.imgN_s.Get()) == 0 ) t.imgN_s = pGunPath + t.tguntexture_s+"_normal.dds";
			if ( FileExist(t.imgN_s.Get()) == 0 ) t.imgN_s = "effectbank\\reloaded\\media\\blank_N.dds";
			timestampactivity(0, cstr(cstr("Normal=")+t.imgN_s).Get() );
			t.imgS_s = pGunPath + t.tguntexture_s+"_metalness.dds";
			if ( FileExist(t.imgS_s.Get()) == 0 ) t.imgS_s = "effectbank\\reloaded\\media\\blank_black.dds";
			timestampactivity(0, cstr(cstr("Metalness=")+t.imgS_s).Get() );
			t.imgI_s = pGunPath + t.tguntexture_s+"_I.dds";
			if ( FileExist(t.imgI_s.Get()) == 0 ) t.imgI_s = pGunPath + t.tguntexture_s+"_illumination.dds";
			if ( FileExist(t.imgI_s.Get()) == 0 ) t.imgI_s = "effectbank\\reloaded\\media\\blank_I.dds";
			timestampactivity(0, cstr(cstr("Illumination=")+t.imgI_s).Get() );
			timgGloss_s = pGunPath + t.tguntexture_s+"_gloss.dds";
			if ( FileExist(timgGloss_s.Get()) == 0 ) timgGloss_s = pGunPath + t.tguntexture_s+"_S.dds";;
			if ( FileExist(timgGloss_s.Get()) == 0 ) timgGloss_s = "effectbank\\reloaded\\media\\white_D.dds";
			timestampactivity(0, cstr(cstr("Gloss=")+timgGloss_s).Get() );
			timgAO_s = pGunPath + t.tguntexture_s+"_ao.dds";
			if ( FileExist(timgAO_s.Get()) == 0 ) timgAO_s = "effectbank\\reloaded\\media\\white_D.dds";
			timestampactivity(0, cstr(cstr("AO=")+timgAO_s).Get() );
			timgHeight_s = pGunPath + t.tguntexture_s+"_height.dds";
			if ( FileExist(timgHeight_s.Get()) == 0 ) timgHeight_s = "effectbank\\reloaded\\media\\blank_black.dds";
			timestampactivity(0, cstr(cstr("Height=")+timgHeight_s).Get() );
		}
		else
		{
			t.imgD_s="effectbank\\reloaded\\media\\white_D.dds";
			t.imgN_s="effectbank\\reloaded\\media\\blank_N.dds";
			t.imgS_s="effectbank\\reloaded\\media\\blank_black.dds";
			t.imgI_s="effectbank\\reloaded\\media\\blank_I.dds";
			timgGloss_s="effectbank\\reloaded\\media\\white_D.dds";
			timgAO_s="effectbank\\reloaded\\media\\white_D.dds";
			timgHeight_s="effectbank\\reloaded\\media\\blank_black.dds";
		}
		if ( t.gun[t.gunid].transparency > 2 ) 
		{
			t.imgDid=loadinternaltextureex(t.imgD_s.Get(),0,1);
			t.imgNid=loadinternaltextureex(t.imgN_s.Get(),0,1);
			t.imgSid=loadinternaltextureex(t.imgS_s.Get(),0,1);
			t.imgIid=loadinternaltextureex(t.imgI_s.Get(),0,1);
		}
		else
		{
			t.imgDid=loadinternaltextureex(t.imgD_s.Get(),5,0);
			t.imgNid=loadinternaltexture(t.imgN_s.Get());
			t.imgSid=loadinternaltexture(t.imgS_s.Get());
			t.imgIid=loadinternaltexture(t.imgI_s.Get());
		}
		int imgGlossid=loadinternaltexture(timgGloss_s.Get());
		int imgAOid=loadinternaltexture(timgAO_s.Get());
		int imgHeightid=loadinternaltexture(timgHeight_s.Get());

		// determine if need to texture ALL of model, or just the cube maps
		if ( t.tfoundvalidinternaltexture == 0 )
		{
			// apply textures to whole gun model
			if (g.memskipibr == 0) 
			{
				t.entityprofiletexibrid = t.terrain.imagestartindex + 32;
				TextureObject(t.currentgunobj, 8, t.entityprofiletexibrid);
			}
			TextureObject ( t.currentgunobj, 7, t.imgIid );
			if ( t.tguntextureoverride == 1 ) TextureObject ( t.currentgunobj, 0, t.imgDid );
			TextureObject ( t.currentgunobj, 1, imgAOid );
			TextureObject ( t.currentgunobj, 2, t.imgNid );
			TextureObject ( t.currentgunobj, 3, t.imgSid );
			TextureObject ( t.currentgunobj, 4, imgGlossid );
			TextureObject ( t.currentgunobj, 5, imgHeightid );
			int iPBRCubeImg = t.terrain.imagestartindex+31;
			TextureObject ( t.currentgunobj, 6, iPBRCubeImg );
			t.gun[t.gunid].texdid=t.imgDid;
			t.gun[t.gunid].texnid=t.imgNid;
			t.gun[t.gunid].texmid=t.imgSid;
			t.gun[t.gunid].texiid=t.imgIid;
			t.gun[t.gunid].texgid=imgGlossid;
			t.gun[t.gunid].texaoid=imgAOid;
			t.gun[t.gunid].texhid=imgHeightid;
		}
		else
		{
			// no need to fill in the blanks with Wicked Engine textured meshes
		}

		//  Apply effect to object (special extra parameter to specify both BONE and NON-BONE effect types)
		t.gun[t.gunid].effectidused=t.teffectid;
		SetObjectEffectCore ( t.currentgunobj,t.teffectid,t.teffectid2,0 );

		//  Set position of sun for weapon shader
		SetVector4 ( g.weaponvectorindex,t.terrain.sundirectionx_f,t.terrain.sundirectiony_f,t.terrain.sundirectionz_f,0.0 );
		SetEffectConstantV ( t.teffectid,"LightSource",g.weaponvectorindex );
		SetVector4 ( g.weaponvectorindex,1,1,1,1.0 );
		SetEffectConstantV ( t.teffectid,"SurfColor",g.weaponvectorindex );
	}

	// apply all textures applied to this weapon
	sObject* pGunObject = GetObjectData(t.currentgunobj);
	if (pGunObject) 
	{
		WickedCall_TextureObject(pGunObject, NULL);
		WickedCall_SetObjectCastShadows(pGunObject, false);
	}
	SetObjectDiffuseEx(t.currentgunobj, 0xFFFFFFFF, 0);

	//PE: Emissive.
	WickedCall_SetObjectEmissiveStrength(pGunObject, t.gun[t.gunid].settings.fEmissiveStrength);

	// until weapons are provided that have a SEPARATE transparent mesh for semi-opaque
	// alpha clip transparency out so weapons can be included in the PREPASS to solve lightray issue
	// WickedCall_SetObjectAlphaRef (pGunObject, 0.01f); // Wicked modified so prepass only renders transparent objects that have this LOW alpha ref
	// can scan the texture names of the weapon and specifically leave "_glass" meshes alone
	if (pGunObject)
	{
		for (int meshindex = 0; meshindex < pGunObject->iMeshCount; meshindex++)
		{
			sMesh* pMesh = pGunObject->ppMeshList[meshindex];
			if (pMesh && pMesh->pTextures)
			{
				if (strstr(pMesh->pTextures[0].pName, "_glass") != NULL)
				{
					// leave alpha clip in tact to llow transparency!
				}
				else
				{
					// rest of solid gun should use alpha clip to ensure lightrays are blocked
					WickedCall_SetMeshAlphaRef(pMesh, 0.01f);
				}
			}
		}
	}

	// STANDARD and ALT modes
	image_setlegacyimageloading(true);
	for ( t.i = 0 ; t.i <= 1; t.i++ )
	{
		// load in scope if any
		if ( g.firemodes[t.gunid][t.i].zoomscope_s != "" ) 
		{
			t.img_s = "";
			t.img_s=t.img_s +"gamecore\\"+g.fpgchuds_s+"\\"+t.gun_s+"\\"+g.firemodes[t.gunid][t.i].zoomscope_s;
			g.firemodes[t.gunid][t.i].zoomscope=loadinternaltextureex(t.img_s.Get(),5,0);
		}
		else
		{
			//  V109 BETA8 - try to load common scope files in case not specified in gunspec
			t.tzoomscope_s="scope_d2.dds";
			t.img_s = "";
			t.img_s=t.img_s+"gamecore\\"+g.fpgchuds_s+"\\"+t.gun_s+"\\"+t.tzoomscope_s;
			g.firemodes[t.gunid][t.i].zoomscope=loadinternaltextureex(t.img_s.Get(),5,0);
			if (  g.firemodes[t.gunid][t.i].zoomscope == 0 ) 
			{
				t.tzoomscope_s="scope.dds";
				t.img_s = "";
				t.img_s=t.img_s+"gamecore\\"+g.fpgchuds_s+"\\"+t.gun_s+"\\"+t.tzoomscope_s;
				g.firemodes[t.gunid][t.i].zoomscope=loadinternaltextureex(t.img_s.Get(),5,0);
				if (  g.firemodes[t.gunid][t.i].zoomscope == 0 ) 
				{
					t.tzoomscope_s="scope1.dds";
					t.img_s = "";
					t.img_s=t.img_s+"gamecore\\"+g.fpgchuds_s+"\\"+t.gun_s+"\\"+t.tzoomscope_s;
					g.firemodes[t.gunid][t.i].zoomscope=loadinternaltextureex(t.img_s.Get(),5,0);
				}
			}

			//  load in ammo and icon images for status panel (if exist)
			if (  t.i == 0  )  t.talt_s = ""; else t.talt_s = "alt";
			t.timg_s = "";
			t.timg_s=t.timg_s+"gamecore\\"+g.fpgchuds_s+"\\"+t.gun_s+"\\"+t.talt_s+"icon.png";
			g.firemodes[t.gunid][t.i].iconimg=loadinternaltextureex(t.timg_s.Get(),0,1);
			t.timg_s = "";
			t.timg_s=t.timg_s+"gamecore\\"+g.fpgchuds_s+"\\"+t.gun_s+"\\"+t.talt_s+"ammo.png";
			g.firemodes[t.gunid][t.i].ammoimg=loadinternaltextureex(t.timg_s.Get(),0,1);

		}

		// rotate gun and fix pivot if GUNSPEC changes it
		if (  g.firemodes[t.gunid][t.i].settings.rotx_f != 0 || g.firemodes[t.gunid][t.i].settings.roty_f != 0 || g.firemodes[t.gunid][t.i].settings.rotz_f != 0 ) 
		{
			SetObjectRotationZYX (  t.currentgunobj );
			RotateObject (  t.currentgunobj,g.firemodes[t.gunid][t.i].settings.rotx_f,g.firemodes[t.gunid][t.i].settings.roty_f,g.firemodes[t.gunid][t.i].settings.rotz_f );
			FixObjectPivot (  t.currentgunobj );
		}
	}
	image_setlegacyimageloading(false);

	// Glue gun to HUD-Gun-Marker (mode 0 default glue mode)
	GlueObjectToLimbEx ( t.currentgunobj, g.hudbankoffset+2, 0, 0 );

	// and glue any secondary gun object also
	if (iGunSecondaryObj > 0)
	{
		if (ObjectExist(iGunSecondaryObj) == 1)
		{
			// anim sync did not work as n-core anim system use different read/write values per frame - alas
			//int iMode = 3; // mode 3 syncs the secondary object to the primary object anim timer with no lag (pass in the animating objID (can be different from obj being glued to)
			GlueObjectToLimbEx (iGunSecondaryObj, g.hudbankoffset + 2, 0, 0);// t.currentgunobj);
		}
	}

	//  Setup gun for correct visuals (special transparency for after-shadow setting)
	if (t.gun[t.gunid].transparency > 2)
	{
		SetObjectTransparency (t.currentgunobj, t.gun[t.gunid].transparency);
		if (iGunSecondaryObj > 0 && ObjectExist(iGunSecondaryObj) == 1) SetObjectTransparency (iGunSecondaryObj, t.gun[t.gunid].transparency);
	}
	else
	{
		SetObjectTransparency (  t.currentgunobj,2 );
		if (iGunSecondaryObj > 0 && ObjectExist(iGunSecondaryObj) == 1) SetObjectTransparency (iGunSecondaryObj, 2);
	}

	DisableObjectZDepth(t.currentgunobj);
	if (iGunSecondaryObj > 0 && ObjectExist(iGunSecondaryObj) == 1) DisableObjectZDepth (iGunSecondaryObj);

	//  Setup gun for animation
	t.currentgunanimspeed_f = (t.genericgunanimspeed_f*0.75);
	gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
	gun_LoopObject (  t.currentgunobj );

	// Set art flags for weapon object (can use 32 bit flags here eventually)
	DWORD dwArtFlags = 0;
	if ( t.gun[t.gunid].invertnormal == 1 ) dwArtFlags = 1;
	if ( t.gun[t.gunid].preservetangents == 1 ) dwArtFlags |= 1<<1;
	SetObjectArtFlags ( t.currentgunobj, dwArtFlags, t.gun[t.gunid].boostintensity );

	//  Setup gun with muzzle flash image
	for ( t.i = 0 ; t.i <= 1; t.i++ )
	{
		// Mussle flash
		t.num = g.firemodes[t.gunid][t.i].settings.muzzleflash;
		if (t.num == 0)
		{
			// when settings.muzzleflash is zero, disable the muzzleflash
			g.firemodes[t.gunid][t.i].settings.flashimg = 0;
		}
		else
		{
			// standard muzzle flash
			t.size_f = g.firemodes[t.gunid][t.i].settings.muzzlesize_f; 
			if (t.size_f == 0.0)  t.size_f = 100.0;
			t.muzzleflash_s = "gamecore\\muzzleflash\\flash";
			t.muzzleflash_s += Str(t.num);
			t.muzzleflash_s += ".dds";
			t.imgid = loadmuzzle(t.muzzleflash_s.Get());
			g.firemodes[t.gunid][t.i].settings.flashimg = t.imgid;
		}

		// Setup gun with brass models
		t.num = g.firemodes[t.gunid][t.i].settings.brass; 
		if (t.num == 0)
		{
			// when settings.brass is zero, disable the brass
			g.firemodes[t.gunid][t.i].settings.brassobjmaster = 0;
		}
		else
		{
			// regular brass
			if (t.num == 0)  t.num = 1;
			t.brass_s = "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"; // insane fix for debug build
			t.brass_s = "";
			t.brass_s = t.brass_s + "gamecore\\brass\\brass";
			t.brass_s = t.brass_s + Str(t.num);
			t.brass_s = t.brass_s + "\\brass";
			t.brass_s = t.brass_s + Str(t.num);
			t.brass_s = t.brass_s + ".x";
			t.brassobj = loadbrass(t.brass_s.Get());
			if (t.brassobj == 0)
			{
				// specifying a brass value that does not exist crashes engine
				t.num = 1; g.firemodes[t.gunid][t.i].settings.brass = 0;
				t.brass_s = "";
				t.brass_s = t.brass_s + "gamecore\\brass\\brass" + Str(t.num) + "\\brass" + Str(t.num) + ".x";
				t.brassobj = loadbrass(t.brass_s.Get());
			}
			g.firemodes[t.gunid][t.i].settings.brassobjmaster = t.brassobj;
		}

		// Setup gun with smoke images
		t.num=g.firemodes[t.gunid][t.i].settings.smoke;
		if (t.num == 0)
		{
			// when settings.smoke is zero, disable the smoke
			g.firemodes[t.gunid][t.i].settings.smokeimg = 0;
		}
		else
		{
			// regular smoke
			if (Len(g.firemodes[t.gunid][t.i].settings.smokedecal_s.Get()) > 0)
			{
				t.smoke_s = "";
				t.smoke_s = t.smoke_s + "gamecore\\decals\\" + g.firemodes[t.gunid][t.i].settings.smokedecal_s + "\\decal.dds";
				t.imgid = loadsmoke(t.smoke_s.Get());
			}
			else
			{
				if (t.num == 0)  t.num = 1;
				if (t.num == 1)
				{
					t.smoke_s = "gamecore\\decals\\gunsmoke\\decal.dds";
				}
				else
				{
					t.smoke_s = "gamecore\\decals\\smoke";
					t.smoke_s += Str(t.num);
					t.smoke_s += "\\decal.dds";
				}
				t.imgid = loadsmoke(t.smoke_s.Get());
			}
			g.firemodes[t.gunid][t.i].settings.smokeimg = t.imgid;
		}
	}

	//  Setup gun with crosshair
	t.crosshair_s = "";
	t.crosshair_s=t.crosshair_s+"gamecore\\"+g.fpgchuds_s+"\\"+t.gun_s+"\\crosshair.dds";
	image_setlegacyimageloading(true);
	t.crosshairimage=loadinternalimagecompressquality(t.crosshair_s.Get(),5,1);
	image_setlegacyimageloading(false);
	t.gun[t.gunid].settings.crosshairimg=t.crosshairimage;
	t.gun[t.gunid].secondobj=0;

	//  Load gun sounds and companions
	for ( t.p = 0 ; t.p <= 15; t.p++ )
	{
		t.tgname_s = t.gunsound[t.gunid][t.p].name_s;
		if ( t.tgname_s != "" ) 
		{
			// main sound for player
			t.tvariationsavailable=0;
			t.snd_s = "";
			t.snd_s=t.snd_s+"gamecore\\"+g.fpgchuds_s+"\\"+t.gun_s+"\\"+t.tgname_s;
			if (  cstr(Lower(Right(t.tgname_s.Get(),5))) == "1.wav" ) 
			{
				sprintf ( t.szwork , "%s1.wav" , Left(t.snd_s.Get(),Len(t.snd_s.Get())-5) );
				t.gunsound[t.gunid][t.p].soundid1=loadinternalsound( t.szwork );
				sprintf ( t.szwork , "%s2.wav" , Left(t.snd_s.Get(),Len(t.snd_s.Get())-5) );
				t.gunsound[t.gunid][t.p].soundid2=loadinternalsound( t.szwork );
				sprintf ( t.szwork , "%s3.wav" , Left(t.snd_s.Get(),Len(t.snd_s.Get())-5) );
				t.gunsound[t.gunid][t.p].soundid3=loadinternalsound( t.szwork );
				sprintf ( t.szwork , "%s4.wav" , Left(t.snd_s.Get(),Len(t.snd_s.Get())-5) );
				t.gunsound[t.gunid][t.p].soundid4=loadinternalsound( t.szwork );
				t.soundloopcheckpoint[t.gunsound[t.gunid][t.p].soundid1]=2;
				t.soundloopcheckpoint[t.gunsound[t.gunid][t.p].soundid2]=2;
				t.soundloopcheckpoint[t.gunsound[t.gunid][t.p].soundid3]=2;
				t.soundloopcheckpoint[t.gunsound[t.gunid][t.p].soundid4]=2;
				t.tvariationsavailable=1;
			}
			else
			{
				t.gunsound[t.gunid][t.p].soundid1=loadinternalsound(t.snd_s.Get());
				t.gunsound[t.gunid][t.p].soundid2=0;
				t.gunsound[t.gunid][t.p].soundid3=0;
				t.gunsound[t.gunid][t.p].soundid4=0;
				t.soundloopcheckpoint[t.gunsound[t.gunid][t.p].soundid1]=2;
			}
			// extra sound value checks in many places
			if (  t.gunsound[t.gunid][t.p].soundid1>0 ) 
			{
				if (  SoundExist(t.gunsound[t.gunid][t.p].soundid1) == 0 ) 
				{
					t.gunsound[t.gunid][t.p].soundid1=0;
				}
			}
			// companion sounds for other weapon sound uses
			if (  t.gunsound[t.gunid][t.p].soundid1>0 ) 
			{
				if (  t.p <= 3 ) 
				{
					if (  t.tvariationsavailable == 1  ) { t.snd_s = Left(t.snd_s.Get(),Len(t.snd_s.Get())-5); t.snd_s += "2.wav"; }
					t.gunsoundcompanion[t.gunid][t.p][0].soundid=loadinternalsoundcore(t.snd_s.Get(),1);
					if (  t.tvariationsavailable == 1  ) { t.snd_s = Left(t.snd_s.Get(),Len(t.snd_s.Get())-5); t.snd_s += "3.wav"; }
					t.gunsoundcompanion[t.gunid][t.p][1].soundid=loadinternalsoundcorecloneflag(t.snd_s.Get(),1,t.gunsoundcompanion[t.gunid][t.p][0].soundid);
					if (  t.tvariationsavailable == 1  ) { t.snd_s = Left(t.snd_s.Get(),Len(t.snd_s.Get())-5); t.snd_s += "4.wav"; }
					t.gunsoundcompanion[t.gunid][t.p][2].soundid=loadinternalsoundcorecloneflag(t.snd_s.Get(),1,t.gunsoundcompanion[t.gunid][t.p][0].soundid);
				}
			}
			else
			{
				if (  t.p <= 3 ) 
				{
					t.gunsoundcompanion[t.gunid][t.p][0].soundid=0;
					t.gunsoundcompanion[t.gunid][t.p][1].soundid=0;
					t.gunsoundcompanion[t.gunid][t.p][2].soundid=0;
				}
			}
		}
	}

	//  Load gun altsounds
	for ( t.p = 0 ; t.p <= 15; t.p++ )
	{
		if (  t.p != 2 ) 
		{
			t.snd_s = "";
			t.snd_s=t.snd_s+"gamecore\\"+g.fpgchuds_s+"\\"+t.gun_s+"\\"+t.gunsound[t.gunid][t.p].altname_s;
			t.gunsound[t.gunid][t.p].altsoundid=loadinternalsound(t.snd_s.Get());
			if (  t.gunsound[t.gunid][t.p].altsoundid>0 ) 
			{
				if (  SoundExist(t.gunsound[t.gunid][t.p].altsoundid) == 0 ) 
				{
					t.gunsound[t.gunid][t.p].altsoundid=0;
				}
			}
		}
	}

	//  Load HUD image (ammo and weapon selected image)
	t.img_s = "";
	t.img_s=t.img_s+"gamecore\\"+g.fpgchuds_s+"\\"+t.gun_s+"\\hud_icon.dds";
	image_setlegacyimageloading(true);
	t.gun[t.gunid].hudimage=loadinternalimagecompressquality(t.img_s.Get(),5,1);
	image_setlegacyimageloading(false);

	//  Find and store projectile index for later use
	t.tProjectileType_s=t.gun[t.gunid].projectile_s  ; weapon_getprojectileid ( );
	for ( t.i = 0 ; t.i<=  1; t.i++ )
	{
		g.firemodes[t.gunid][t.i].settings.flakindex=t.tProjectileType;
	}

	//  reset this as only used when gun 'selected'
	t.currentgunobj=0;

	//PE: Load any bullet tracer. tracers::
	cstr tracerimage = "";
	tracerimage = tracerimage + "gamecore\\" + g.fpgchuds_s + "\\" + t.gun_s + "\\tracer.dds";
	Tracers::LoadTracerImage(tracerimage.Get(), t.gunid);
}

void gun_updategunshaders ( void )
{
}

void gun_freeafterlevel ( void )
{
	// remember gun when leave the level
	t.lastgunid = t.gunid;

	// 020516 - only if not standalone in game
	g.autoloadgun = 0; gun_change ( );
}

void gun_freeguns ( void )
{
	//  deselect gun in hand
	gun_freeafterlevel ( );

	//  hide and clean guns (going back to level)
	for (t.gunid = 1; t.gunid <= g.gunmax; t.gunid++)
	{
		t.tobj = t.gun[t.gunid].obj;
		t.gun[t.gunid].settings.alternate = 0;
		if (t.tobj > 0)
		{
			if (ObjectExist(t.tobj) == 1)  HideObject (t.tobj);
		}
	}
}

void gun_free ( void )
{
	//  Hide gun from HUD
	if (  t.currentgunobj>0 ) 
	{
		if (  ObjectExist(t.currentgunobj) == 1 ) 
		{
			gun_SetObjectInterpolation (  t.currentgunobj,100 );
			gun_SetObjectFrame (  t.currentgunobj,g.firemodes[t.gunid][0].action.show.s );
			HideObject (  t.currentgunobj );
		}
	}

	//  Stop any gun sounds if free suddenly
	if (  t.gunid>0 ) 
	{
		for ( t.p = 0 ; t.p<=  15; t.p++ )
		{
			if (  t.p != 4 ) 
			{
				//  080415 - except put away which we do not want to cut off
				if (  t.gunsound[t.gunid][t.p].soundid1>0  )  StopSound (  t.gunsound[t.gunid][t.p].soundid1 );
				if (  t.gunsound[t.gunid][t.p].soundid2>0  )  StopSound (  t.gunsound[t.gunid][t.p].soundid2 );
				if (  t.gunsound[t.gunid][t.p].soundid3>0  )  StopSound (  t.gunsound[t.gunid][t.p].soundid3 );
				if (  t.gunsound[t.gunid][t.p].soundid4>0  )  StopSound (  t.gunsound[t.gunid][t.p].soundid4 );
			}
		}
		for ( t.p = 1 ; t.p<=  3; t.p++ )
		{
			if (  t.gunsoundcompanion[t.gunid][t.p][0].soundid>0  )  StopSound (  t.gunsoundcompanion[t.gunid][t.p][0].soundid );
			if (  t.gunsoundcompanion[t.gunid][t.p][1].soundid>0  )  StopSound (  t.gunsoundcompanion[t.gunid][t.p][1].soundid );
			if (  t.gunsoundcompanion[t.gunid][t.p][2].soundid>0  )  StopSound (  t.gunsoundcompanion[t.gunid][t.p][2].soundid );
		}
	}

	//  Disassociate gun with player
	t.currentgunobj=0;

	// hide any secondary guns
	for (int iSecObj = g.gunbankextraobjoffset; iSecObj < g.gunbankextraobjoffset + 150; iSecObj++)
	{
		if (ObjectExist(iSecObj) == 1)
		{
			HideObject(iSecObj);
		}
	}

	//  Hide support objects for gun
	if (  t.gun[t.gunid].settings.flashlimb != -1 ) 
	{
		t.obj=g.hudbankoffset+5;
		if (  ObjectExist(t.obj) == 1  )  HideObject (  t.obj );
	}
	if (  t.gun[t.gunid].settings.brasslimb != -1 ) 
	{
		for ( t.o = 6 ; t.o<=  20; t.o++ )
		{
			t.obj=g.hudbankoffset+t.o;
			if ( ObjectExist(t.obj) == 1 )  
			{
				ODEDestroyObject ( t.obj );
				HideObject ( t.obj );
			}
		}
	}
	if (  t.gun[t.gunid].settings.smokelimb != -1 ) 
	{
		for ( t.o = 21 ; t.o<=  30; t.o++ )
		{
			t.obj=g.hudbankoffset+t.o;
			if (  ObjectExist(t.obj) == 1  )  HideObject (  t.obj );
		}
	}

	// stop and clear any idle sound loop
	if ( t.gunactiveidlesoundloopindex != 0 )
	{
		if ( t.gunactiveidlesoundloopindex > 0 && SoundExist ( t.gunactiveidlesoundloopindex ) == 1 ) StopSound ( t.gunactiveidlesoundloopindex );
		t.gunactiveidlesoundloopindex = 0;
	}

	//  Clear basic gun vars
	t.gunflash=0 ; t.gunsmoke=0 ; t.gunbrass=0 ; t.gunshoot=0 ; t.gunmode=5;
	t.gunbrasstrigger = 0;
}

void gun_releaseresources ( void )
{
	//  delete gun resources
	timestampactivity(0,"_gun_releaseresources");
	for ( t.gunid = 1 ; t.gunid<=  g.gunmax; t.gunid++ )
	{
		t.tobj=t.gun[t.gunid].obj;
		if (  t.tobj>0 ) 
		{
			if (  ObjectExist(t.tobj) == 1 ) 
			{
				DeleteObject (  t.tobj );
			}
			t.gun[t.gunid].obj=0;

			//PE: MemLeak - Changing level - Delete all gun sounds here.
			for (int i = 0; i <= 15; i++)
			{
				if (t.gunid < t.gunsound.size())
				{
					t.tgname_s = t.gunsound[t.gunid][i].name_s;
					if (t.tgname_s != "")
					{
						if (t.gunsound[t.gunid][i].soundid1 > 0)
						{
							if (SoundExist(t.gunsound[t.gunid][i].soundid1) == 1) { DeleteSound(t.gunsound[t.gunid][i].soundid1); }
							t.gunsound[t.gunid][i].soundid1 = 0;
						}
						if (t.gunsound[t.gunid][i].soundid2 > 0)
						{
							if (SoundExist(t.gunsound[t.gunid][i].soundid2) == 1) { DeleteSound(t.gunsound[t.gunid][i].soundid2); }
							t.gunsound[t.gunid][i].soundid2 = 0;
						}
						if (t.gunsound[t.gunid][i].soundid3 > 0)
						{
							if (SoundExist(t.gunsound[t.gunid][i].soundid3) == 1) { DeleteSound(t.gunsound[t.gunid][i].soundid3); }
							t.gunsound[t.gunid][i].soundid3 = 0;
						}
						if (t.gunsound[t.gunid][i].soundid4 > 0)
						{
							if (SoundExist(t.gunsound[t.gunid][i].soundid4) == 1) { DeleteSound(t.gunsound[t.gunid][i].soundid4); }
							t.gunsound[t.gunid][i].soundid4 = 0;
						}

						if (i <= 3)
						{
							if (t.gunsoundcompanion[t.gunid][i][0].soundid > 0)
							{
								if (SoundExist(t.gunsoundcompanion[t.gunid][i][0].soundid) == 1) { DeleteSound(t.gunsoundcompanion[t.gunid][i][0].soundid); }
								t.gunsoundcompanion[t.gunid][i][0].soundid = 0;
							}
							if (t.gunsoundcompanion[t.gunid][i][1].soundid > 0)
							{
								if (SoundExist(t.gunsoundcompanion[t.gunid][i][1].soundid) == 1) { DeleteSound(t.gunsoundcompanion[t.gunid][i][1].soundid); }
								t.gunsoundcompanion[t.gunid][i][1].soundid = 0;
							}
							if (t.gunsoundcompanion[t.gunid][i][2].soundid > 0)
							{
								if (SoundExist(t.gunsoundcompanion[t.gunid][i][2].soundid) == 1) { DeleteSound(t.gunsoundcompanion[t.gunid][i][2].soundid); }
								t.gunsoundcompanion[t.gunid][i][2].soundid = 0;
							}
						}
						if (i != 2)
						{
							if (t.gunsound[t.gunid][i].altsoundid > 0)
							{
								if (SoundExist(t.gunsound[t.gunid][i].altsoundid) == 1) { DeleteSound(t.gunsound[t.gunid][i].altsoundid); }
								t.gunsound[t.gunid][i].altsoundid = 0;
							}
						}
					}
				}
			}

		}
		//  remove images fully
		removeinternaltexture(t.gun[t.gunid].texdid);
		removeinternaltexture(t.gun[t.gunid].texnid);
		removeinternaltexture(t.gun[t.gunid].texmid);
		removeinternaltexture(t.gun[t.gunid].texiid);
		removeinternaltexture(t.gun[t.gunid].texgid);
		removeinternaltexture(t.gun[t.gunid].texaoid);
		removeinternaltexture(t.gun[t.gunid].texhid);
	}

	// delete any secondary guns
	for (int iSecObj = g.gunbankextraobjoffset; iSecObj < g.gunbankextraobjoffset + 150; iSecObj++)
	{
		if ( ObjectExist(iSecObj)==1 )
		{
			DeleteObject(iSecObj);
		}
	}

	//  reset gunbank
	for ( t.g = 0; t.g <= g.gunbankmaxlimit; t.g++ )
	{
		t.gunbank_s[t.g]="";
	}
	g.gunbankmax=0;
}

void gun_tagmpgunstolist ( void )
{
	if (  t.game.runasmultiplayer == 1 ) 
	{
		timestampactivity(0,"tagging MP guns to end of gun bank list");
		t.olddir_s=GetDir();
		t.tpath_s = SteamGetWorkshopItemPath();
		if (  PathExist(t.tpath_s.Get()) == 1 ) 
		{
			SetDir (  t.tpath_s.Get() );
			//  looking for i.e. gamecore_guns_futuristic_futuretekshotgun_gunspec.txt
			ChecklistForFiles (  );
			for ( t.c = 1 ; t.c<=  ChecklistQuantity(); t.c++ )
			{
				t.tfile_s=Lower(ChecklistString(t.c));
				if (  t.tfile_s != "." && t.tpath_s != ".." ) 
				{
					if (  cstr(Right(t.tfile_s.Get(),11)) == "gunspec.txt" ) 
					{
						t.tfullfile_s = t.tfile_s;
						//  include this gun
						t.tgunname_s=Right(t.tfile_s.Get(),Len(t.tfile_s.Get())-14);
						t.tgunname_s=Left(t.tgunname_s.Get(),Len(t.tgunname_s.Get())-12);
						for ( t.n = 1 ; t.n<=  Len(t.tgunname_s.Get()); t.n++ )
						{
							if (  cstr(Mid(t.tgunname_s.Get(),t.n)) == "_" ) 
							{
								t.tgunfolder_s=Left(t.tgunname_s.Get(),t.n-1);
								t.tgunname_s=Right(t.tgunname_s.Get(),Len(t.tgunname_s.Get())-Len(t.tgunfolder_s.Get())-1);
								t.n=Len(t.tgunname_s.Get()) ; break;
							}
						}
						//  add temporarily to gun list for this game session
						t.tfile_s=t.tgunfolder_s+"\\"+t.tgunname_s;
						t.treplace_s = t.tfile_s;
						t.tfile_s = "";
						//  07032015 - 016 - filenames that have _ in this as a character get swapped to @ so we dont think they are folders
						for ( t.n = 1 ; t.n<=  Len(t.treplace_s.Get()); t.n++ )
						{
							if (  t.treplace_s.Get()[t.n-1]  ==  '@'  )  t.tfile_s  =  t.tfile_s + "_"; else t.tfile_s  =  t.tfile_s + Mid(t.treplace_s.Get(),t.n);
						}
						++g.gunmax ; if (  g.gunmax>g.maxgunsinengine  )  g.gunmax = g.maxgunsinengine;
						t.gun[g.gunmax].name_s=t.tfile_s;
						t.gun[g.gunmax].extraformp=1;
						sprintf ( t.szwork , "mp t.temp t.gun %s:%s" , Str(g.gunmax) , t.tfile_s.Get() );
						timestampactivity(0, t.szwork );

						//  09032014 - 018 - adding decals into mp
						if (  FileOpen(3)  )  CloseFile (  3 );
						t.tfoundflash = 0;
						OpenToRead (  3,t.tfullfile_s.Get() );
						t.tfoundflash = 0;
						while (  FileEnd(3)  ==  0 && t.tfoundflash  ==  0 ) 
						{
							t.tisthisdecl_s = ReadString ( 3 );
							if (  cstr(Left(t.tisthisdecl_s.Get(),5) ) ==  "decal" ) 
							{
								t.tlocationofequals = FindLastChar(t.tisthisdecl_s.Get(),"=");
								if (  t.tlocationofequals > 1 ) 
								{
									if ( cstr( Mid(t.tisthisdecl_s.Get(),t.tlocationofequals+1) )  ==  " " ) 
									{
										t.tdecal_s = Right(t.tisthisdecl_s.Get(),Len(t.tisthisdecl_s.Get())-(t.tlocationofequals+1));
									}
									else
									{
										t.tdecal_s = Right(t.tisthisdecl_s.Get(),Len(t.tisthisdecl_s.Get())-(t.tlocationofequals));
									}
									t.tfext_s = "";
									t.tfounddecaltex = 0;
									sprintf ( t.szwork , "gamecore\\decals\\%s\\decal.png" , t.tdecal_s.Get() );
									if (  FileExist( t.szwork )  ==  1  )  t.tfext_s  =  ".png";
									sprintf ( t.szwork , "gamecore\\decals\\%s\\decal.dds" , t.tdecal_s.Get() );
									if (  FileExist( t.szwork )  ==  1  )  t.tfext_s  =  ".dds";
									if (  t.tfext_s  !=  "" ) 
									{
										t.tfounddecaltex = 1;
									}
									t.tfext_s = "";
									t.tfounddecalspec = 0;
									sprintf ( t.szwork , "gamecore\\decals\\%s\\decalspec.txt" , t.tdecal_s.Get() );
									if (  FileExist( t.szwork )  ==  1  )  t.tfext_s  =  ".txt";
									if (  t.tfext_s  !=  "" ) 
									{
										t.tfounddecalspec = 1;
									}
									if (  t.tfounddecalspec  ==  1 && t.tfounddecaltex  ==  1 ) 
									{
										t.newdecal_s=t.tdecal_s;
										for ( t.tdecalid = 1 ; t.tdecalid<=  g.decalmax; t.tdecalid++ )
										{
											if (  t.decal[t.tdecalid].name_s == t.newdecal_s  )  break;
										}
										if (  t.tdecalid>g.decalmax ) 
										{
											if (  t.decalid>g.decalmax ) 
											{
												g.decalmax=t.decalid;
												Dim (  t.decal,g.decalmax  );
											}
											t.decal[t.decalid].name_s=t.newdecal_s;
											t.decal[t.decalid].active=1;
											t.decal_s = t.newdecal_s;
											decal_load ( );
											++t.decalid;
											g.decalmax=t.decalid-1;
										}
									}
								}
							}
						}
						CloseFile (  3 );
					}
				}
			}
			//  09032014 - 018 - adding decals into mp
		}
		SetDir (  t.olddir_s.Get() );
	}
}

void gun_removempgunsfromlist ( void )
{
	//  remove any guns added for MP session
	if (  t.game.runasmultiplayer == 1 ) 
	{
		t.twasgunmax=g.gunmax;
		while (  t.gun[g.gunmax].extraformp == 1 && g.gunmax >= 1 ) 
		{
			t.gun[g.gunmax].extraformp=0;
			--g.gunmax;
		}
		sprintf ( t.szwork , "removing MP guns, reducing t.gun list from %i to %i" , t.twasgunmax , g.gunmax );
		timestampactivity(0, t.szwork );
	}
	return;
}

void gun_playerdead ( void )
{
	t.gunzoommode=0;
	t.gunzoommag_f=0;
	t.gunshoot=0;
	g.firemode=0;
	t.gunmode=131;
	if (  t.gunmodeloopsnd>0 ) 
	{
		if (  SoundExist(t.gunmodeloopsnd) == 1 ) 
		{
			StopSound (  t.gunmodeloopsnd );
		}
		t.gunmodeloopsnd=0;
	}
	if (  t.currentgunobj>0 ) 
	{
		if (  ObjectExist(t.currentgunobj) == 1 ) 
		{
			gun_StopObject (  t.currentgunobj );
		}
	}
	return;
}

int preparegun ( int gunid, int index)
{
	//  hide any limbs in the weapon which should NEVER be rendered (and cause D3DX shader error)
	PerformCheckListForLimbs (  index );
	if (  t.gun[gunid].settings.minpolytrim>0 ) 
	{
		for ( int tc = 1 ; tc <= ChecklistQuantity(); tc++ )
		{
			cstr tcc_s = Lower(ChecklistString(tc));
			int tflag = 0;
			if (  tcc_s == "firespot"  )  tflag = 1;
			if (  tcc_s == "firespot02"  )  tflag = 1;
			if (  tcc_s == "x3ds_firespot"  )  tflag = 1;
			if (  tcc_s == "brass"  )  tflag = 1;
			if (  tcc_s == "brass02"  )  tflag = 1;
			if (  tcc_s == "x3ds_brass"  )  tflag = 1;
			if (  tcc_s == "_smoke"  )  tflag = 1;
			if (  tcc_s == "smoke"  )  tflag = 1;
			if (  tcc_s == "smoke2"  )  tflag = 1;
			if (  tcc_s == "x3ds_smoke"  )  tflag = 1;
			if (  tcc_s == "hand"  )  tflag = 1;
			if (  tcc_s == "x3ds_hand"  )  tflag = 1;
			if (  tcc_s == "camera01"  )  tflag = 1;
			//  lee - 091014 - some weapons have leftovers!! (sniper)
			if (  tcc_s == "lens001"  )  tflag = 1;
			if (  tcc_s == "omni001"  )  tflag = 1;
			if (  tcc_s == "omni002"  )  tflag = 1;
			//  lee - 071014 - also hide any limbs that are boxes (leftovers from gun marker work)
			if (  GetLimbPolygonCount(index,tc-1) <= t.gun[gunid].settings.minpolytrim  )  tflag = 1;
			if (  tflag == 1 ) 
			{
				HideLimb (  index,tc-1 );
			}
		}
	}

	//  prepare weapon object
	SetObjectCollisionOff (  index );
	SetObjectInterpolation (  index,100 );
	SetObjectFrame (  index,g.firemodes[gunid][0].action.show.s );
	HideObject (  index );

	// guns cast no shadows!
	sObject* pObject = GetObjectData(index);
	if (pObject) 
	{
		WickedCall_SetObjectCastShadows(pObject, false);
	}
	if (ObjectExist(index))
	{
		SetObjectDiffuseEx(index, 0xFFFFFFFF, 0);
	}

	//PE: Emissive.
	WickedCall_SetObjectEmissiveStrength(pObject, t.gun[t.gunid].settings.fEmissiveStrength);

	// success
	return 1;
}

int loadgun (int gunid, char* tfile_s)
{
	int index = 0;
	cstr tcc_s = "";
	int tflag = 0;
	int tc = 0;
	int tt;
	index = 0;
	if (g.gunbankmax > 0)
	{
		for (tt = 1; tt <= g.gunbankmax; tt++)
		{
			if (strcmp (tfile_s, t.gunbank_s[tt].Get()) == 0) { index = g.gunbankoffset + tt; break; }
		}
	}
	else
	{
		tt = g.gunbankmax + 1;
	}
	if (tt > g.gunbankmax)
	{
		++g.gunbankmax;

		if (FileExist(tfile_s) == 1)
		{
			index = g.gunbankoffset + g.gunbankmax;
			t.gunbank_s[g.gunbankmax] = tfile_s;

			//  07032014 - 016 - the object can sometimes exist already after workshop guns have been used, so we deleted it if it does
			if (ObjectExist(index))  DeleteObject (index);

			char pChopFile[512];
			strcpy (pChopFile, tfile_s);
			if (strnicmp(pChopFile + strlen(pChopFile) - 2, ".x", 2) == NULL)
			{
				pChopFile[strlen(pChopFile) - 2] = 0;
				cstr pFileToLoad = cstr(pChopFile) + cstr(".DBO");
				if (FileExist(pFileToLoad.Get()) == 1)
					LoadObject(pFileToLoad.Get(), index);
				else
					LoadObject(tfile_s, index);
			}
			else
				LoadObject(tfile_s, index);

			// prepare gun
			preparegun (gunid, index);
		}
	}
	return index;
}

int createsecondgun ( void )
{
	int index = 0;
	++g.gunbankmax;
	t.gunbank_s[g.gunbankmax]="second";
	index=g.gunbankoffset+g.gunbankmax;
	return index;
}

int loadbrass ( char* tfile_s )
{
	int tbrassDimg = 0;
	int tbrassNimg = 0;
	int tbrassSimg = 0;
	cstr tdbofile_s =  "";
	cstr ttexdiff_s =  "";
	int teffectid = 0;
	int index = 0;
	int tt = 0;
	index=0;
	if ( g.brassbankmax>0 ) 
	{
		for ( tt = 1 ; tt<=  g.brassbankmax; tt++ )
		{
			if ( strcmp ( tfile_s , t.brassbank_s[tt].Get() ) == 0 ) {  index = g.brassbankoffset+tt ; break; }
		}
	}
	else
	{
		tt=g.brassbankmax+1;
	}
	if ( tt>g.brassbankmax ) 
	{
		// get texture file from X file
		if (cstr(Lower(Right(tfile_s, 2))) == ".x")
		{
			ttexdiff_s = Left(tfile_s, Len(tfile_s) - 2);
		}
		else
		{
			ttexdiff_s = Left(tfile_s, Len(tfile_s) - 4);
		}
		// brass only DBO in MAX
		tdbofile_s = ttexdiff_s + ".dbo";
		strcpy (tfile_s, tdbofile_s.Get());
		tdbofile_s = "";
		if ( FileExist(tfile_s) == 1 || FileExist(tdbofile_s.Get()) == 1 ) 
		{
			++g.brassbankmax;
			index=g.brassbankoffset+g.brassbankmax;
			t.brassbank_s[g.brassbankmax]=tfile_s;
			if ( FileExist(tdbofile_s.Get()) == 1 ) 
			{
				strcpy ( tfile_s, tdbofile_s.Get() );
				tdbofile_s="";
			}
			else
			{
				// allowed to save DBO (once only)
			}
			LoadObject ( tfile_s, index );
			// no DBO saves in MAX

			// Determine if PBR or non-PBR
			bool bHavePBRTextures = false;
			sprintf ( t.szwork, "%s_color.dds", ttexdiff_s.Get() ); if ( FileExist (t.szwork) ) bHavePBRTextures = true;
			sprintf ( t.szwork, "%s_color.png", ttexdiff_s.Get() ); if ( FileExist (t.szwork) ) bHavePBRTextures = true;
			if ( g.gpbroverride == 1 && bHavePBRTextures == true )
			{
				// PBR texturing
				// Wicked engine does the rest to load other PBR textures from set :)
				sprintf (t.szwork, "%s_color.png", ttexdiff_s.Get());
				int tbrassCOLORimg = loadinternalimage(t.szwork);
				TextureObject (index, 0, tbrassCOLORimg);
			}
			else
			{
				// DNS texturing (non-PBR)
				sprintf ( t.szwork , "%s_D.dds" , ttexdiff_s.Get() );
				tbrassDimg=loadinternalimage(t.szwork);
				sprintf ( t.szwork , "%s_N.dds" , ttexdiff_s.Get() );
				tbrassNimg=loadinternalimage(t.szwork);
				sprintf ( t.szwork , "%s_S.dds" , ttexdiff_s.Get() );
				tbrassSimg=loadinternalimage(t.szwork);
				// and texture the object
				TextureObject (  index,0,tbrassDimg );
				TextureObject (  index,1,loadinternalimagecompressquality("effectbank\\reloaded\\media\\blank_O.dds",1,0) );
				TextureObject (  index,2,tbrassNimg );
				TextureObject (  index,3,tbrassSimg );
				TextureObject (  index,4,t.terrain.imagestartindex );
				TextureObject (  index,5,g.postprocessimageoffset+5 );
				TextureObject (  index,6,loadinternalimagecompressquality("effectbank\\reloaded\\media\\blank_I.dds",1,0) );
				teffectid=loadinternaleffect("effectbank\\reloaded\\entity_basic.fx");
			}
			SetObjectEffect (  index,teffectid );
			RotateObject (  index,0,180,0  ); FixObjectPivot (  index );
			SetObjectCollisionOff (  index );
			SetObjectMask (  index, 1 );
			HideObject (  index );
			if (ObjectExist(index))
			{
				//SetObjectDiffuseEx(index, 0xFFFFFFFF, 0);
				sObject* pObject = GetObjectData(index);
				if (pObject)
				{
					WickedCall_SetObjectCastShadows(pObject, false);
				}
			}
		}
	}
	return index;
}

int loadmuzzle ( char* tfile_s )
{
	int index = 0;
	int tt = 0;
	index=0;
	if (  g.muzzlebankmax>0 ) 
	{
		for ( tt = 1 ; tt<=  g.muzzlebankmax; tt++ )
		{
			if (  strcmp ( tfile_s , t.muzzlebank_s[tt].Get() ) == 0 ) {  index = g.muzzlebankoffset+tt  ; break; }
		}
	}
	else
	{
		tt=g.muzzlebankmax+1;
	}
	if (  tt>g.muzzlebankmax ) 
	{
		++g.muzzlebankmax;
		t.muzzlebank_s[g.muzzlebankmax]=tfile_s;
		index=g.muzzlebankoffset+g.muzzlebankmax;
		loadinternalimageexcompress(tfile_s,index,5);
	}
	return index;
}

int loadsmoke ( char* tfile_s )
{
	int index = 0;
	int tt = 0;
	index=0;
	if (  g.smokebankmax>0 ) 
	{
		for ( tt = 1 ; tt<=  g.smokebankmax; tt++ )
		{
			if ( strcmp ( tfile_s , t.smokebank_s[tt].Get() ) == 0 ) { index = g.smokebankoffset+tt ; break; }
		}
	}
	else
	{
		tt=g.smokebankmax+1;
	}
	if (  tt>g.smokebankmax ) 
	{
		++g.smokebankmax;
		t.smokebank_s[g.smokebankmax]=tfile_s;
		index=g.smokebankoffset+g.smokebankmax;
		loadinternalimageexcompress(tfile_s,index,5);
	}
	return index;
}

