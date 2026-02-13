void gun_scaninall_ref ( void )
{
	// Scan entire guns folder
	t.gunid=1;
	cstr pRootDir = GetDir();
	for (int iGunsInTwoFolders = 0; iGunsInTwoFolders < 2; iGunsInTwoFolders++)
	{
		char pathToUse[MAX_PATH];
		if (iGunsInTwoFolders == 0)
		{
			// root folder - already here
			strcpy(pathToUse, "");
		}
		if (iGunsInTwoFolders == 1)
		{
			// writable folder - switch to it
			strcpy(pathToUse, pRootDir.Get());
			GG_GetRealPath(pathToUse, 0);
			SetDir(pathToUse);
		}
		if ( PathExist("gamecore") == 1 )
		{
			SetDir ("gamecore");
			UnDim (t.filelist_s);
			buildfilelist(g.fpgchuds_s.Get(), "");
			SetDir ("..");
			if (ArrayCount(t.filelist_s) > 0)
			{
				for (t.chkfile = 0; t.chkfile <= ArrayCount(t.filelist_s); t.chkfile++)
				{
					t.file_s = t.filelist_s[t.chkfile];
					if (t.file_s != "." && t.file_s != "..")
					{
						if (cstr(Lower(Right(t.file_s.Get(), 11))) == "gunspec.txt")
						{
							t.gun[t.gunid].path_s = pathToUse;
							t.gun[t.gunid].name_s = Left(t.file_s.Get(), Len(t.file_s.Get()) - 12);
							t.strwork = ""; t.strwork = t.strwork + "gun " + Str(t.gunid) + ":" + t.file_s;
							timestampactivity(0, t.strwork.Get());
							++t.gunid;
							if (t.gunid > g.maxgunsinengine)  t.gunid = g.maxgunsinengine;
						}
					}
				}
			}
		}
	}

	// and restore root folder
	SetDir(pRootDir.Get());

	// report total number of guns
	g.gunmax = t.gunid - 1;
	t.strwork = ""; t.strwork = t.strwork + "total guns=" + Str(g.gunmax);
	timestampactivity(0, t.strwork.Get());

	//  Now sort the gun list into alphabetical order (MP needs gunid identical on each PC)
	gun_sortintoorder ( );

	// and as there is no connection back to entity that links to guns (and we need it for collection filling)
	// create a database here of known stock weapons
	for (int gunid = 1; gunid <= g.gunmax; gunid++)
	{
		LPSTR pGunName = t.gun[gunid].name_s.Get();
		if (t.gun[gunid].pathtostockentity_s.Len() == 0)
		{
			cstr MCW = "Max Collection\\Weapons\\";
			if (stricmp (pGunName, "enhanced\\AK") == NULL) t.gun[gunid].pathtostockentity_s = MCW + "Assault Rifle.fpe";
			if (stricmp (pGunName, "enhanced\\MK18") == NULL) t.gun[gunid].pathtostockentity_s = MCW + "Compact Assault Rifle.fpe";
			if (stricmp (pGunName, "enhanced\\M67") == NULL) t.gun[gunid].pathtostockentity_s = MCW + "Frag Grenade.fpe";
			if (stricmp (pGunName, "enhanced\\MK19T") == NULL) t.gun[gunid].pathtostockentity_s = MCW + "Magnum Pistol.fpe";
			if (stricmp (pGunName, "enhanced\\AR") == NULL) t.gun[gunid].pathtostockentity_s = MCW + "Patrol Rifle.fpe";
			if (stricmp (pGunName, "enhanced\\B810") == NULL) t.gun[gunid].pathtostockentity_s = MCW + "Pocket Knife.fpe";
			if (stricmp (pGunName, "enhanced\\SledgeHammer") == NULL) t.gun[gunid].pathtostockentity_s = MCW + "Sledgehammer.fpe";
			if (stricmp (pGunName, "enhanced\\M29S") == NULL) t.gun[gunid].pathtostockentity_s = MCW + "Snubnose Revolver.fpe";
			if (stricmp (pGunName, "enhanced\\R870") == NULL) t.gun[gunid].pathtostockentity_s = MCW + "Tactical Pump Shotgun.fpe";
			if (stricmp (pGunName, "max\\colt") == NULL) t.gun[gunid].pathtostockentity_s = MCW + "Colt Pistol.fpe";
			cstr AGK = "Aztec Game Kit\\Weapons\\";
			if (stricmp (pGunName, "aztec\\AztecAxe") == NULL) t.gun[gunid].pathtostockentity_s = AGK + "Aztec Axe.fpe";
			if (stricmp (pGunName, "aztec\\AztecDagger") == NULL) t.gun[gunid].pathtostockentity_s = AGK + "Aztec Dagger.fpe";
			if (stricmp (pGunName, "aztec\\AztecSpear") == NULL) t.gun[gunid].pathtostockentity_s = AGK + "Aztec Spear.fpe";

		}
	}
}

void gun_sortintoorder ( void )
{
	//  Now sort the gun list into alphabetical order
	for ( t.tgid1 = 1 ; t.tgid1<=  g.gunmax; t.tgid1++ )
	{
		for ( t.tgid2 = 1 ; t.tgid2<=  g.gunmax; t.tgid2++ )
		{
			if (  t.tgid1 != t.tgid2 ) 
			{
				t.tname1_s=Lower(t.gun[t.tgid1].name_s.Get());
				t.tname2_s=Lower(t.gun[t.tgid2].name_s.Get());
				//C++ISSUE
				if (  strlen( t.tname1_s.Get() ) > strlen( t.tname2_s.Get() ) ) 
				{
					//  smallest at top
					t.gun[t.tgid1].name_s=t.tname2_s;
					t.gun[t.tgid2].name_s=t.tname1_s;
				}
			}
		}
	}

	// once have correct order, populate weapon slots for player
	void gun_gatherslotorder_load (void);
	gun_gatherslotorder_load();
}

void gun_findweaponindexbyname_core ( void )
{
	t.foundgunid=0;
	if (  t.findgun_s != "" ) 
	{
		for ( t.tid = 1 ; t.tid<=g.gunmax; t.tid++ )
		{
			if (  cstr(Lower(t.findgun_s.Get())) == cstr(Lower(t.gun[t.tid].name_s.Get())) ) 
			{
				t.foundgunid=t.tid;
				break;
			}
		}
	}
}

void gun_scaninall_findnewlyaddedgun (void)
{
	// if find gun that is not in list, if find, flag is 'found'
	// this is expensive, so only do it IF gunlist flag has been reset (when exe starts, start of every level, etc)
	if (g_bGunListNeedsRefreshing == true)
	{
		// store original gunid
		t.storegunid = t.gunid;

		// Scan entire guns folder (again)
		cstr pRootDir = GetDir();
		for (int iGunsInTwoFolders = 0; iGunsInTwoFolders < 2; iGunsInTwoFolders++)
		{
			char pathToUse[MAX_PATH];
			if (iGunsInTwoFolders == 0)
			{
				// root folder - already here
				strcpy(pathToUse, "");
			}
			if (iGunsInTwoFolders == 1)
			{
				// writable folder - switch to it
				strcpy(pathToUse, pRootDir.Get());
				GG_GetRealPath(pathToUse, 0);
				SetDir(pathToUse);
			}
			if (PathExist("gamecore") == 1)
			{
				SetDir ("gamecore");
				UnDim (t.filelist_s);
				buildfilelist(g.fpgchuds_s.Get(), "");
				SetDir ("..");
				if (ArrayCount(t.filelist_s) > 0)
				{
					for (t.chkfile = 0; t.chkfile <= ArrayCount(t.filelist_s); t.chkfile++)
					{
						t.file_s = t.filelist_s[t.chkfile];
						if (t.file_s != "." && t.file_s != "..")
						{
							if (cstr(Lower(Right(t.file_s.Get(), 11))) == "gunspec.txt")
							{
								t.findgun_s = Left(t.file_s.Get(), Len(t.file_s.Get()) - 12);
								gun_findweaponindexbyname_core ();
								if (t.foundgunid == 0)
								{
									++g.gunmax;
									if (g.gunmax > g.maxgunsinengine)  g.gunmax = g.maxgunsinengine;
									t.gun[g.gunmax].path_s = pathToUse;
									t.gun[g.gunmax].name_s = t.findgun_s;
									t.gunid = g.gunmax;
									t.gun_s = t.findgun_s;
									gun_loaddata ();
									t.strwork = ""; t.strwork = t.strwork + "newly added gun " + Str(t.gunid) + ":" + t.file_s;
									timestampactivity(0, t.strwork.Get());
								}
							}
						}
					}
				}
			}
		}

		// and restore root folder
		SetDir(pRootDir.Get());

		// report total number of guns
		t.strwork = ""; t.strwork = t.strwork + "new total guns=" + Str(g.gunmax);
		timestampactivity(0, t.strwork.Get());

		// only refresh when necessary (performance hit)
		g_bGunListNeedsRefreshing = false;

		// restore original gunid
		t.gunid = t.storegunid;
	}
}

void gun_scaninall_findnewlyaddedgun_old ( void )
{
	//  if find gun that is not in list, if find, flag is 'found'
	t.storegunid=t.gunid;

	//  gather files
	SetDir (  "gamecore" );
	UnDim ( t.filelist_s );
	buildfilelist(g.fpgchuds_s.Get(),"");
	SetDir (  ".." );

	//  go through file list of latest guns
	if (  ArrayCount(t.filelist_s)>0 ) 
	{
		for ( t.chkfile = 0 ; t.chkfile<=  ArrayCount(t.filelist_s); t.chkfile++ )
		{
			t.file_s=t.filelist_s[t.chkfile];
			if (  t.file_s != "." && t.file_s != ".." ) 
			{
				t.findgun_s="";
				if ( cstr( Lower(Right(t.file_s.Get(),11))) == "gunspec.txt" ) 
				{
					t.findgun_s=Left(t.file_s.Get(),Len(t.file_s.Get())-12);
				}
				if (  t.findgun_s != "" ) 
				{
					gun_findweaponindexbyname_core ( );
					if (  t.foundgunid == 0 ) 
					{
						++g.gunmax;
						if (  g.gunmax>g.maxgunsinengine  )  g.gunmax = g.maxgunsinengine;
						t.gun[g.gunmax].name_s=t.findgun_s;
						t.gunid=g.gunmax;
						t.gun_s=t.findgun_s; 
						gun_loaddata ( );
					}
				}
			}
		}
	}
	t.gunid=t.storegunid;
}

void gun_findweaponindexbyname ( void )
{
	//  try to find gun
	t.storefindgun_s=t.findgun_s; 
	gun_findweaponindexbyname_core ( );

	//  if not found, try rescanning (as new guns can be added by store), and try a second time
	if (  t.foundgunid == 0 ) 
	{
		gun_scaninall_findnewlyaddedgun ( );
		t.findgun_s=t.storefindgun_s; 
		gun_findweaponindexbyname_core ( );
	}
}

cstr gun_names_tonormal(cstr thisLabel)
{
	// clean up if 2 slashes, can only have one to propery identify weapon name
	LPSTR pOldStr = thisLabel.Get();
	char pNewStr[MAX_PATH];
	strcpy(pNewStr, pOldStr);
	LPSTR pFourSlashes = strstr(pNewStr, "\\\\");
	if(pFourSlashes)
	{
		strcpy(pFourSlashes, pFourSlashes + 1);
		thisLabel = pNewStr;
	}

	if (stricmp (thisLabel.Get(), "enhanced\\Gloves_Unarmed") == NULL) thisLabel = "Melee Combat";
	if (stricmp (thisLabel.Get(), "enhanced\\AK") == NULL) thisLabel = "Assault Rifle";
	if (stricmp (thisLabel.Get(), "enhanced\\AR") == NULL) thisLabel = "Patrol Rifle";
	if (stricmp (thisLabel.Get(), "enhanced\\B810") == NULL) thisLabel = "Pocket Knife";
	if (stricmp (thisLabel.Get(), "enhanced\\M29S") == NULL) thisLabel = "Snubnose Revolver";
	if (stricmp (thisLabel.Get(), "enhanced\\Mk18") == NULL) thisLabel = "Compact Assault Rifle";
	if (stricmp (thisLabel.Get(), "enhanced\\Mk19T") == NULL) thisLabel = "Magnum Pistol";
	if (stricmp (thisLabel.Get(), "enhanced\\R870") == NULL) thisLabel = "Tactical Pump Shotgun";
	if (stricmp (thisLabel.Get(), "enhanced\\SledgeHammer") == NULL) thisLabel = "SledgeHammer";
	if (stricmp (thisLabel.Get(), "aztec\\AztecAxe") == NULL) thisLabel = "Aztec Axe";
	if (stricmp (thisLabel.Get(), "aztec\\AztecDagger") == NULL) thisLabel = "Aztec Dagger";
	if (stricmp (thisLabel.Get(), "aztec\\AztecSpear") == NULL) thisLabel = "Aztec Spear";
	return thisLabel;
}

cstr gun_names_tointernal(cstr thisLabel)
{
	if (stricmp (thisLabel.Get(), "No Weapon") == NULL) thisLabel = "";
	if (stricmp (thisLabel.Get(), "No Preference") == NULL) thisLabel = "";
	if (stricmp (thisLabel.Get(), "Melee Combat") == NULL) thisLabel = "enhanced\\Gloves_Unarmed";
	if (stricmp (thisLabel.Get(), "Grenades Only") == NULL) thisLabel = "enhanced\\M67";
	if (stricmp (thisLabel.Get(), "Assault Rifle") == NULL) thisLabel = "enhanced\\AK";
	if (stricmp (thisLabel.Get(), "Patrol Rifle") == NULL) thisLabel = "enhanced\\AR";
	if (stricmp (thisLabel.Get(), "Pocket Knife") == NULL) thisLabel = "enhanced\\B810";
	if (stricmp (thisLabel.Get(), "Snubnose Revolver") == NULL) thisLabel = "enhanced\\M29S";
	if (stricmp (thisLabel.Get(), "Compact Assault Rifle") == NULL) thisLabel = "enhanced\\Mk18";
	if (stricmp (thisLabel.Get(), "Magnum Pistol") == NULL) thisLabel = "enhanced\\Mk19T";
	if (stricmp (thisLabel.Get(), "Tactical Pump Shotgun") == NULL) thisLabel = "enhanced\\R870";
	if (stricmp (thisLabel.Get(), "SledgeHammer") == NULL) thisLabel = "enhanced\\SledgeHammer";
	if (stricmp (thisLabel.Get(), "Aztec Axe") == NULL) thisLabel = "aztec\\AztecAxe";
	if (stricmp (thisLabel.Get(), "Aztec Dagger") == NULL) thisLabel = "aztec\\AztecDagger";
	if (stricmp (thisLabel.Get(), "Aztec Spear") == NULL) thisLabel = "aztec\\AztecSpear";
	return thisLabel;
}
