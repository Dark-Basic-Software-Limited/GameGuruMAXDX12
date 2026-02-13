void mp_selectedALevel ( void )
{
	cstr mlevel_s = g.mysystem.editorsGrideditAbs_s + "__multiplayerlevel__.fpm";
	if ( FileExist( mlevel_s.Get())  ) DeleteAFile ( mlevel_s.Get() );
	g.mp.fpmpicked = t.tfpmfilelist_s[g.mp.selectedLobby];
	mp_checkIfLevelHasCustomContent ( );
	if (  g.mp.fpmpicked  ==  "Level I just worked on" ) 
	{
		cstr worklevel_s = g.mysystem.editorsGrideditAbs_s + "worklevel.fpm";
		CopyAFile ( worklevel_s.Get(),mlevel_s.Get() );
	}
	else
	{
		//CopyAFile (  cstr(g.fpscrootdir_s+"\\Files\\mapbank\\"+g.mp.fpmpicked).Get(),cstr(g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerlevel__.fpm").Get() );
		CopyAFile ( cstr(g.mysystem.mapbankAbs_s+g.mp.fpmpicked).Get(), cstr(g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerlevel__.fpm").Get() );
	}
	if (  g.mp.levelContainsCustomContent  ==  1 ) 
	{
		//  first we check if the changed flag is set (they have saved since hosting) if not, we dont need to upload to steam
		if (  g.mp.fpmpicked  ==  "Level I just worked on" ) 
		{
			//t.tempsteammaptocheck_s = g.fpscrootdir_s+"\\Files\\mapbank\\worklevel.dat";
			t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s + "worklevel.dat";
		}
		else
		{
			//t.tempsteammaptocheck_s = g.fpscrootdir_s+"\\Files\\mapbank\\"+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
			t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
		}

		t.tmphopitemtocheckifchangedandversion_s = t.tempsteammaptocheck_s;
		mp_grabWorkshopChangedFlagAndVersion ( );
		g.mp.workshopItemChangedFlag = t.tMPshopHasItemChangedFlag;
		if (  t.tMPshopHasItemChangedFlag  ==  1 ) 
		{
			g.mp.mode = MP_SERVER_CHOOSING_TO_MAKE_FPS_WORKSHOP;
		}
		else
		{
			g.mp.workshopid = t.tMPshopTheIDNumber_s;
		}
	}

return;

}

void mp_checkIfLevelHasCustomContent ( void )
{

	if (  g.mp.fpmpicked  ==  "Level I just worked on" ) 
	{
		//t.tempsteammaptocheck_s = g.fpscrootdir_s+"\\Files\\mapbank\\worklevel.dat";
		t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s+"worklevel.dat";
	}
	else
	{
		//t.tempsteammaptocheck_s = g.fpscrootdir_s+"\\Files\\mapbank\\"+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
		t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
	}

	if (  FileExist(t.tempsteammaptocheck_s.Get()) ) 
	{
		g.mp.levelContainsCustomContent = 1;
	}
	else
	{
		g.mp.levelContainsCustomContent = 0;
	}

return;

}

void mp_buildWorkShopItem ( void )
{
	g.mp.mode = MP_CREATING_WORKSHOP_ITEM;
	switch (  g.mp.buildingWorkshopItemMode ) 
	{
		case 0:
			if (  PathExist( cstr(g.fpscrootdir_s+"\\Files\\editors\\workshopItem").Get() )  ==  0 ) 
			{
				MakeDirectory ( cstr(g.fpscrootdir_s+"\\Files\\editors\\workshopItem").Get() );
			}
			else
			{
				t.told_s=GetDir();
				SetDir (  cstr(g.fpscrootdir_s + "\\Files\\editors\\workshopItem").Get() );
				ChecklistForFiles (  );

				for ( t.c = 1 ; t.c<=  ChecklistQuantity(); t.c++ )
				{
					if (  ChecklistValueA(t.c) == 0 ) 
					{
						t.tfile_s=ChecklistString(t.c);
						if (  t.tfile_s != "." && t.tfile_s != ".." ) 
						{
							DeleteAFile (  t.tfile_s.Get() );
						}
					}
				}
				SetDir (  t.told_s.Get() );
			}

			if (  FileOpen(1)  ==  1  )  CloseFile (  1 );

			if (  g.mp.fpmpicked  ==  "Level I just worked on" ) 
			{
				t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s+"worklevel.dat";//g.fpscrootdir_s+"\\Files\\mapbank\\worklevel.dat";
				t.tempsteamleveltocopy_s = g.mysystem.mapbankAbs_s+"worklevel.fpm";//g.fpscrootdir_s+"\\Files\\mapbank\\worklevel.fpm";
			}
			else
			{
				t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";//g.fpscrootdir_s+"\\Files\\mapbank\\"+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
				t.tempsteamleveltocopy_s = g.mysystem.mapbankAbs_s+g.mp.fpmpicked;//g.fpscrootdir_s+"\\Files\\mapbank\\" + g.mp.fpmpicked;
			}

			t.tempsteamdestworkshopfolder_s = g.fpscrootdir_s + "\\Files\\editors\\workshopItem\\";
			CopyAFile (  t.tempsteamleveltocopy_s.Get(), cstr(t.tempsteamdestworkshopfolder_s + "editors_gridedit___multiplayerlevel__.fpm").Get() );
			//  grab clanged flag and version number (we dont need the changed flag here tho)
			t.tmphopitemtocheckifchangedandversion_s = t.tempsteammaptocheck_s;
			mp_grabWorkshopChangedFlagAndVersion ( );

			OpenToRead (  1,t.tempsteammaptocheck_s.Get() );
			g.mp.buildingWorkshopItemMode = 1;
			//  skip over the warning, change flag, version number and workshop id
			t.tempsteamstring_s = ReadString ( 1 );
			t.tempsteamstring_s = ReadString ( 1 );
			t.tempsteamstring_s = ReadString ( 1 );
			t.tempsteamstring_s = ReadString ( 1 );
			//  skip over the fpm in the file
			t.tempsteamstring_s = ReadString ( 1 );

		break;
//   ``````````````````````````````````

		case 1:
			t.tempsteamdestworkshopfolder_s = g.fpscrootdir_s + "\\Files\\editors\\workshopItem\\";
			//  Write out version number
			if (  FileOpen(2)  )  CloseFile (  2 );
			OpenToWrite (  2, cstr(t.tempsteamdestworkshopfolder_s+"version.dat").Get() );
			WriteString (  2,Str(t.tMPshopTheVersionNumber) );
			CloseFile (  2 );

			t.tempsteamstring_s = ReadString ( 1 );
			t.tempMPshopfilename_s = "";
			for ( t.c = 1 ; t.c<=  Len(t.tempsteamstring_s.Get()); t.c++ )
			{
				if (  cstr(Mid(t.tempsteamstring_s.Get(),t.c))  ==  "\\" || cstr(Mid(t.tempsteamstring_s.Get(),t.c))  ==  "/" ) 
				{
					t.tempMPshopfilename_s=t.tempMPshopfilename_s+"_";
				}
				else
				{
					if (  cstr(Mid(t.tempsteamstring_s.Get(),t.c))  ==  "_" ) 
					{
						t.tempMPshopfilename_s=t.tempMPshopfilename_s+"@";
					}
					else
					{
						t.tempMPshopfilename_s=t.tempMPshopfilename_s+Mid(t.tempsteamstring_s.Get(),t.c);
					}
				}
			}

			if (  t.tempsteamstring_s  !=  "" ) 
			{
				CopyAFile (  cstr(g.fpscrootdir_s+"\\Files\\"+t.tempsteamstring_s).Get() , cstr(t.tempsteamdestworkshopfolder_s + t.tempMPshopfilename_s).Get() );
				t.tsteamyesencrypt = 0;
				//  models
				if (  cstr(Right(t.tempMPshopfilename_s.Get(),4))  ==  ".dbo"  )  t.tsteamyesencrypt  =  1;
				if (  cstr(Right(t.tempMPshopfilename_s.Get(),2))  ==  ".x"  )  t.tsteamyesencrypt  =  1;
				//  images
				if (  cstr(Right(t.tempMPshopfilename_s.Get(),4))  ==  ".png"  )  t.tsteamyesencrypt  =  1;
				if ( cstr( Right(t.tempMPshopfilename_s.Get(),4))  ==  ".jpg"  )  t.tsteamyesencrypt  =  1;
				if (  cstr(Right(t.tempMPshopfilename_s.Get(),4))  ==  ".dds"  )  t.tsteamyesencrypt  =  1;
				if (  cstr(Right(t.tempMPshopfilename_s.Get(),4))  ==  ".bmp"  )  t.tsteamyesencrypt  =  1;
				//  sounds and music
				if ( cstr( Right(t.tempMPshopfilename_s.Get(),4))  ==  ".wav"  )  t.tsteamyesencrypt  =  1;
				if ( cstr( Right(t.tempMPshopfilename_s.Get(),4))  ==  ".mp3"  )  t.tsteamyesencrypt  =  1;
				if ( cstr( Right(t.tempMPshopfilename_s.Get(),4) ) ==  ".ogg"  )  t.tsteamyesencrypt  =  1;
				//  scripts
				if (  cstr(Right(t.tempMPshopfilename_s.Get(),4))  ==  ".fpe"  )  t.tsteamyesencrypt  =  1;

				if (  t.tsteamyesencrypt  ==  1 ) 
				{
					EncryptWorkshopDBPro ( cstr(t.tempsteamdestworkshopfolder_s + t.tempMPshopfilename_s).Get() );
					DeleteAFile (  cstr(t.tempsteamdestworkshopfolder_s + t.tempMPshopfilename_s).Get() );
				}
			}
			if (  FileEnd(1) ) 
			{
				g.mp.buildingWorkshopItemMode = 2;
			}
		break;
//   ``````````````````````````````````

		case 2:

			if (  g.mp.fpmpicked  ==  "Level I just worked on" ) 
			{
				t.tempMPshopname_s = "My Level";
			}
			else
			{
				t.tempMPshopname_s = Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-4);
			}

			SteamSetRoot( cstr( g.fpscrootdir_s+"\\Files\\" ).Get() );
			SteamCreateWorkshopItem (  t.tempMPshopname_s.Get() );

			g.mp.buildingWorkshopItemMode = 3;
		break;
//   ``````````````````````````````````

		case 3:
			if (  SteamIsWorkshopItemUploaded()  ==  1 ) 
			{
				//  set changed flag to 0
				if (  g.mp.fpmpicked  ==  "Level I just worked on" ) 
				{
					//t.tempsteammaptocheck_s = g.fpscrootdir_s+"\\Files\\mapbank\\worklevel.dat";
					t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s+"worklevel.dat";
				}
				else
				{
					//t.tempsteammaptocheck_s = g.fpscrootdir_s+"\\Files\\mapbank\\"+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
					t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
				}
				if (  FileOpen(1)  ==  1  )  CloseFile (  1 );
				//  Work out how many lines there are so we can Dim (  the right amount )
				t.thowmanylines = 0;
				OpenToRead (  1,t.tempsteammaptocheck_s.Get() );
					while (  FileEnd(1) == 0 ) 
					{
						t.tthrowawaystring_s = ReadString ( 1 );
						++t.thowmanylines;
					}
				CloseFile (  1 );
				////  now read in the whole thing
				Dim (  t.templines,t.thowmanylines );
				t.thowmanylines = 0;
				OpenToRead (  1,t.tempsteammaptocheck_s.Get() );
					while (  FileEnd(1) == 0 ) 
					{
						t.templines[t.thowmanylines] = ReadString ( 1 );
						++t.thowmanylines;
					}
				CloseFile (  1 );
				DeleteAFile (  t.tempsteammaptocheck_s.Get() );
				OpenToWrite (  1,t.tempsteammaptocheck_s.Get() );
				g.mp.workshopid = SteamGetWorkshopID();

				t.templines[1] = "0";
				t.templines[3] = g.mp.workshopid;
				for ( t.tloop = 0 ; t.tloop<=  t.thowmanylines-1; t.tloop++ )
				{
					WriteString (  1,t.templines[t.tloop].Get() );
				}
				CloseFile (  1 );

				//  get rid of the array
				UnDim (  t.templines );

				g.mp.buildingWorkshopItemMode = 99;
			}

			if (  SteamIsWorkshopItemUploaded()  ==  -1  )  g.mp.buildingWorkshopItemMode  =  98;
		break;
	}//~ return
return;

}

void mp_buildingWorkshopItemFailed ( void )
{
	t.tsteamlostconnectioncustommessage_s = "Could not build workshop item (Error MP015)";
	mp_lostConnection ( );
	g.mp.mode = MP_SERVER_CHOOSING_FPM_TO_USE;
return;

}

void mp_joinALobby ( void )
{
	t.a = g.mp.selectedLobby;
	if (  g.mp.selectedLobbyName  !=  "Getting Lobby details..." ) 
	{
		g.mp.lobbyjoinedname = g.mp.selectedLobbyName;
		t.tempsteamstringlobbyname_s = "";
		t.tempsteamgotto = 0;
		for ( t.tc = 1 ; t.tc<=  Len(g.mp.lobbyjoinedname.Get()); t.tc++ )
		{
			++t.tempsteamgotto;
			if (  cstr(Mid(g.mp.lobbyjoinedname.Get(),t.tc))  ==  ":" ) { t.tempsteamgotto+=2 ; break; }
			t.tempsteamstringlobbyname_s = t.tempsteamstringlobbyname_s + Mid(g.mp.lobbyjoinedname.Get(),t.tc);
		}
		g.mp.levelnametojoin = "";
		t.tempsteamfoundone = 0;
		for ( t.tc = 1 ; t.tc<=  Len(g.mp.lobbyjoinedname.Get()); t.tc++ )
		{
			if (  cstr(Mid(g.mp.lobbyjoinedname.Get(),t.tc))  ==  ":" )
			{
				++t.tempsteamfoundone;
			}
			else
			{
				if (  t.tempsteamfoundone == 1 ) 
				{
					g.mp.levelnametojoin = g.mp.levelnametojoin + Mid(g.mp.lobbyjoinedname.Get(),t.tc);
				}
			}
		}
		g.mp.workshopidtojoin = "";
		t.tempsteamfoundone = 0;
		for ( t.tc = 1 ; t.tc<=  Len(g.mp.lobbyjoinedname.Get()); t.tc++ )
		{
			if (  cstr(Mid(g.mp.lobbyjoinedname.Get(),t.tc))  ==  ":" )
			{
				++t.tempsteamfoundone;
			}
			else
			{
				if (  t.tempsteamfoundone == 2 ) 
				{
					g.mp.workshopidtojoin = g.mp.workshopidtojoin + Mid(g.mp.lobbyjoinedname.Get(),t.tc);
				}
			}
		}
		g.mp.workshopVersionNumberToJoin = "";
		t.tempsteamfoundone = 0;
		for ( t.tc = 1 ; t.tc<=  Len(g.mp.lobbyjoinedname.Get()); t.tc++ )
		{
			if (  cstr(Mid(g.mp.lobbyjoinedname.Get(),t.tc))  ==  ":" )
			{
				++t.tempsteamfoundone;
			}
			else
			{
				if (  t.tempsteamfoundone == 3 ) 
				{
					g.mp.workshopVersionNumberToJoin = g.mp.workshopVersionNumberToJoin + Mid(g.mp.lobbyjoinedname.Get(),t.tc);
				}
			}
		}
		g.mp.lobbyjoinedname = t.tempsteamstringlobbyname_s;

		//  Check here if there is a workshop item, if the user has subbed and downloaded
		if (  g.mp.workshopidtojoin  !=  "0" ) 
		{

			t.tempMPLobbyNameFromList_s = g.mp.selectedLobbyName;
			mp_canIJoinThisLobby ( );
			t.tsteamstring_s = g.mp.lobbyjoinedname;

			if (  SteamIsWorkshopItemInstalled(g.mp.workshopidtojoin.Get())  ==  0 ) 
			{
				//  show screen asking if they want to subscribe
				g.mp.mode = MP_ASKING_IF_SUBSCRIBE_TO_WORKSHOP_ITEM;
				titles_steamdoyouwanttosubscribetoworkshopitem ( );
				return;
			}
			else
			{
				if (  t.tsteamcanjoinlobby  ==  2 ) 
				{
					//  show screen asking if they want to subscribe
					g.mp.mode = MP_TELLING_THEY_NEED_TO_RESTART;
					titles_steamdTellingToRestart ( );
					return;
				}
			}
		}

		SteamJoinLobby(t.a);
		g.mp.mode = MP_JOINING_LOBBY;
		t.tsteamwaitedforlobbytimer = Timer();
		g.mp.AttemptedToJoinLobbyTime = Timer();
		g.mp.lobbycount = 0;
	}
return;

}

void mp_canIJoinThisLobby ( void )
{
	if (  g.mp.selectedLobbyName  !=  "Getting Lobby details..." ) 
	{
		g.mp.lobbyjoinedname = t.tempMPLobbyNameFromList_s;
		t.tempsteamstringlobbyname_s = "";
		t.tempsteamgotto = 0;
		for ( t.tc = 1 ; t.tc<=  Len(g.mp.lobbyjoinedname.Get()); t.tc++ )
		{
			++t.tempsteamgotto;
			if (  cstr(Mid(g.mp.lobbyjoinedname.Get(),t.tc) ) ==  ":" ) { t.tempsteamgotto += 2 ; break; }
			t.tempsteamstringlobbyname_s = t.tempsteamstringlobbyname_s + Mid(g.mp.lobbyjoinedname.Get(),t.tc);
		}
		g.mp.levelnametojoin = "";
		t.tempsteamfoundone = 0;
		for ( t.tc = 1 ; t.tc<=  Len(g.mp.lobbyjoinedname.Get()); t.tc++ )
		{
			if (  cstr(Mid(g.mp.lobbyjoinedname.Get(),t.tc))  ==  ")" )
			{
				++t.tempsteamfoundone;
			}
			else
			{
				if (  t.tempsteamfoundone == 1 ) 
				{
					g.mp.levelnametojoin = g.mp.levelnametojoin + Mid(g.mp.lobbyjoinedname.Get(),t.tc);
				}
			}
		}
		g.mp.workshopidtojoin = "";
		t.tempsteamfoundone = 0;
		for ( t.tc = 1 ; t.tc<=  Len(g.mp.lobbyjoinedname.Get()); t.tc++ )
		{
			if (  cstr(Mid(g.mp.lobbyjoinedname.Get(),t.tc))  ==  ":" )
			{
				++t.tempsteamfoundone;
			}
			else
			{
				if (  t.tempsteamfoundone == 2 ) 
				{
					g.mp.workshopidtojoin = g.mp.workshopidtojoin + Mid(g.mp.lobbyjoinedname.Get(),t.tc);
				}
			}
		}
		//  grab version number
		g.mp.workshopVersionNumberToJoin = "";
		t.tempsteamfoundone = 0;
		for ( t.tc = 1 ; t.tc<=  Len(g.mp.lobbyjoinedname.Get()); t.tc++ )
		{
			if (  cstr(Mid(g.mp.lobbyjoinedname.Get(),t.tc))  ==  ":" )
			{
				++t.tempsteamfoundone;
			}
			else
			{
				if (  t.tempsteamfoundone == 3 ) 
				{
					g.mp.workshopVersionNumberToJoin = g.mp.workshopVersionNumberToJoin + Mid(g.mp.lobbyjoinedname.Get(),t.tc);
				}
			}
		}
		g.mp.lobbyjoinedname = t.tempsteamstringlobbyname_s;

		//  Check here if there is a workshop item, if the user has subbed and downloaded
		if (  g.mp.workshopidtojoin  !=  "0" && g.mp.workshopidtojoin  !=  "" ) 
		{
			if (  SteamIsWorkshopItemInstalled(g.mp.workshopidtojoin.Get())  ==  0 ) 
			{
				t.tsteamcanjoinlobby = 0;
			}
			else
			{
				t.tsteamcanjoinlobby = 0;
				if (  SteamIsWorkshopItemInstalled(g.mp.workshopidtojoin.Get())  ==  2  )  t.tsteamcanjoinlobby  =  2;
				if (  t.tsteamcanjoinlobby  ==  0 ) 
				{
					t.tpath_s = SteamGetWorkshopItemPath();
					t.tfiletocheck_s = t.tpath_s + "\\version.dat";
					if (  FileExist(t.tfiletocheck_s.Get())  ==  1 ) 
					{
						if (  FileOpen(1)  )  CloseFile (  1 );
						OpenToRead (  1,t.tfiletocheck_s.Get() );
						t.tversioncheck_s = ReadString ( 1 );
						CloseFile (  1 );
						if (  t.tversioncheck_s  ==  g.mp.workshopVersionNumberToJoin ) 
						{
							t.tsteamcanjoinlobby = 1;
						}
						else
						{
							t.tsteamcanjoinlobby = 2;
						}
					}
				}
			}
		}
		else
		{
			t.tsteamcanjoinlobby = 1;
		}

	}
	else
	{
		t.tsteamcanjoinlobby = 0;
	}
return;

}

void mp_leaveALobby ( void )
{
	SteamLeaveLobby (  );
	mp_resetGameStats ( );
return;

}

void mp_SubscribeToWorkShopItem ( void )
{
	t.tlobbytring_s = g.mp.lobbyjoinedname;
	mp_subbedToItem ( );
	g.mp.mode = MP_ASKING_IF_SUBSCRIBE_TO_WORKSHOP_ITEM_WAITING_FOR_RESULTS;
}

void mp_save_workshop_files_needed ( void )
{
	cstr toriginalMasterLevelFile_s = "";
	cstr toriginalprojectname_s = "";

	toriginalMasterLevelFile_s = t.tmasterlevelfile_s;
	toriginalprojectname_s = g.projectfilename_s;
	//  If there is no baseList.dat file we cant proceed
	if (  FileExist("editors\\baseList.dat")  ==  0  )  return;

	//  Work out how many lines there are so we can Dim (  the right amount )
	t.thowmanyfpefiles = 0;
	OpenToRead (  1,"editors\\baseList.dat" );
	while (  FileEnd(1) == 0 ) 
	{
		t.tthrowawaystring_s = ReadString ( 1 );
		++t.thowmanyfpefiles;
	}
	CloseFile (  1 );

	//  Store the count in our global steamworks type
	g.mp.howmanyfpefiles = t.thowmanyfpefiles;

	Dim (  t.tallfpefiles_s,t.thowmanyfpefiles  );
	t.thowmanyfpefiles = 0;
	OpenToRead (  1,"editors\\baseList.dat" );
	while (  FileEnd(1) == 0 ) 
	{
		t.tallfpefiles_s[t.thowmanyfpefiles] = ReadString ( 1 );
		++t.thowmanyfpefiles;
	}
	CloseFile (  1 );

	t.exename_s=t.tsteamsavefilename_s;
	if (  cstr(Lower(Right(t.exename_s.Get(),4))) == ".fpm" ) 
	{
		t.exename_s=Left(t.exename_s.Get(),Len(t.exename_s.Get())-4);
	}
	for ( t.n = Len(t.exename_s.Get()) ; t.n >= 1 ; t.n+= -1 )
	{
		if (  cstr(Mid(t.exename_s.Get(),t.n)) == "\\" || cstr(Mid(t.exename_s.Get(),t.n)) == "/" ) 
		{
			t.exename_s=Right(t.exename_s.Get(),Len(t.exename_s.Get())-t.n);
			break;
		}
	}
	if (  Len(t.exename_s.Get())<1  )  t.exename_s = "sample";

	//  the level
	t.tmasterlevelfile_s=cstr("mapbank\\")+t.exename_s+".fpm";
	t.strwork = "" ; t.strwork = t.strwork + "Saving required files list for "+ t.tmasterlevelfile_s;
	timestampactivity(0, t.strwork.Get() );

	//  Get absolute My Games folder
	g.exedir_s="?";
	t.told_s=GetDir();
	t.tworkshoplistfile_s = t.told_s+"\\mapbank\\" + t.exename_s + ".dat";
	t.tMPshopTheVersionNumber = 1;
	if (  FileExist(t.tworkshoplistfile_s.Get())  ==  1 ) 
	{
		t.tmphopitemtocheckifchangedandversion_s = t.tworkshoplistfile_s;
		mp_grabWorkshopChangedFlagAndVersion ( );
		++t.tMPshopTheVersionNumber;
		DeleteAFile (  t.tworkshoplistfile_s.Get() );
	}
	OpenToWrite (  1,t.tworkshoplistfile_s.Get() );

	//  set the changed flag since we are saving, this way we dont rely on workshop info to know if a file is new or not
	WriteString (  1,"DO NOT MANUALY EDIT THIS FILE" );
	WriteString (  1,"1" );
	WriteString (  1,Str(t.tMPshopTheVersionNumber) );
	WriteString (  1,"0" );

	//  Collect ALL files in string array list
	g.filecollectionmax = 0;
	Dim (  t.filecollection_s,500  );

	//  include original FPM
	addtocollection(t.tmasterlevelfile_s.Get());

	//  Stage 2 - collect all files
	t.tlevelfile_s="";
	g.projectfilename_s=t.tmasterlevelfile_s;

	//  load in level FPM
	if (  Len(t.tlevelfile_s.Get())>1 ) 
	{
		g.projectfilename_s=t.tlevelfile_s;
	}

	//  chosen sky, terrain and veg
	addfoldertocollection( cstr(cstr("skybank\\")+t.skybank_s[g.skyindex]).Get() );
	addfoldertocollection("skybank\\night");
	if ( stricmp ( g.terrainstyle_s.Get(), "CUSTOM" ) != NULL )
	{
		addfoldertocollection( cstr(cstr("terrainbank\\")+g.terrainstyle_s).Get() );
	}
	addfoldertocollection( cstr(cstr("vegbank\\")+g.vegstyle_s).Get() );

	//  choose all entities and associated files
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if (  t.entid>0 ) 
		{
			//  check for lua scripts
			if (  t.entityelement[t.e].eleprof.aimain_s  !=  "" ) 
			{
				if (  mp_check_if_entity_is_from_install(t.entityelement[t.e].eleprof.aimain_s.Get())  ==  0 ) 
				{
					addtocollection( cstr(cstr("scriptbank\\")+t.entityelement[t.e].eleprof.aimain_s).Get() );
				}
			}
			//  entity profile file
			t.tentityname1_s=cstr("entitybank\\")+t.entitybank_s[t.entid];
			t.tentityname2_s=cstr(Left(t.tentityname1_s.Get(),Len(t.tentityname1_s.Get())-4))+".fpe";
			if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tentityname2_s).Get() ) == 1 ) 
			{
				t.tentityname_s=t.tentityname2_s;
			}
			else
			{
				t.tentityname_s=t.tentityname1_s;
			}
			//  Check to see if the entity is part of the base install
			//  If it is, we can skip checking any further with it
			if (  mp_check_if_entity_is_from_install(t.tentityname_s.Get())  ==  0 ) 
			{

				addtocollection(t.tentityname_s.Get());
				//  entity files in folder
				t.tentityfolder_s=t.tentityname_s;
				for ( t.n = Len(t.tentityname_s.Get()) ; t.n >=  1 ; t.n+= -1 )
				{
					if (  cstr(Mid(t.tentityname_s.Get(),t.n)) == "\\" || cstr(Mid(t.tentityname_s.Get(),t.n)) == "/" ) 
					{
						t.tentityfolder_s=Left(t.tentityfolder_s.Get(),t.n);
						break;
					}
				}
				//  model file
				t.tlocaltofpe=1;
				for ( t.n = 1 ; t.n<=  Len(t.entityprofile[t.entid].model_s.Get()); t.n++ )
				{
					if (  cstr(Mid(t.entityprofile[t.entid].model_s.Get(),t.n)) == "\\" || cstr(Mid(t.entityprofile[t.entid].model_s.Get(),t.n)) == "/" ) 
					{
						t.tlocaltofpe=0 ; break;
					}
				}
				if (  t.tlocaltofpe == 1 ) 
				{
					t.tfile1_s=t.tentityfolder_s+t.entityprofile[t.entid].model_s;
				}
				else
				{
					t.tfile1_s=t.entityprofile[t.entid].model_s;
				}
				t.tfile2_s=cstr(Left(t.tfile1_s.Get(),Len(t.tfile1_s.Get())-2))+".dbo";
				if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tfile2_s).Get() ) == 1 ) 
				{
					t.tfile_s=t.tfile2_s;
				}
				else
				{
					t.tfile_s=t.tfile1_s;
				}
				t.tmodelfile_s=t.tfile_s;
				addtocollection(t.tmodelfile_s.Get());
				//  entity characterpose file (if any)
				t.tfile3_s=cstr(Left(t.tfile1_s.Get(),Len(t.tfile1_s.Get())-2))+".dat";
				if (  FileExist(cstr(g.fpscrootdir_s+"\\Files\\"+t.tfile3_s).Get()) == 1 ) 
				{
					addtocollection(t.tfile3_s.Get());
				}

				//  texture files
				t.tlocaltofpe=1;
				for ( t.n = 1 ; t.n<=  Len(t.entityelement[t.e].eleprof.texd_s.Get()); t.n++ )
				{
					if (  cstr(Mid(t.entityelement[t.e].eleprof.texd_s.Get(),t.n)) == "\\" || cstr(Mid(t.entityelement[t.e].eleprof.texd_s.Get(),t.n)) == "/" ) 
					{
						t.tlocaltofpe=0 ; break;
					}
				}
				if (  t.tlocaltofpe == 1 ) 
				{
					t.tfile_s=t.tentityfolder_s+t.entityelement[t.e].eleprof.texd_s;
				}
				else
				{
					t.tfile_s=t.entityelement[t.e].eleprof.texd_s;
				}
				addtocollection(t.tfile_s.Get());
				timestampactivity(0, cstr(cstr("Exporting ")+t.entitybank_s[t.entid]+" texd:"+t.tfile_s).Get() );
				if (  cstr(Left(Lower(Right(t.tfile_s.Get(),6)),2)) == "_d" ) 
				{
					t.tfileext_s=Right(t.tfile_s.Get(),3);
					t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_n."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
					t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_s."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
					t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_i."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
					t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_o."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
				}
				if (  t.tlocaltofpe == 1 ) 
				{
					t.tfile_s=t.tentityfolder_s+t.entityelement[t.e].eleprof.texaltd_s;
				}
				else
				{
					t.tfile_s=t.entityelement[t.e].eleprof.texaltd_s;
				}
				addtocollection(t.tfile_s.Get());
				//  if entity did not specify texture it is multi-texture, so interogate model file
				findalltexturesinmodelfile(t.tmodelfile_s.Get(),t.tentityfolder_s.Get(),t.entityprofile[t.entityelement[t.e].bankindex].texpath_s.Get());
				//  shader file
				t.tfile_s=t.entityelement[t.e].eleprof.effect_s ; addtocollection(t.tfile_s.Get());
				//  script files
				// t.tfile_s=cstr("scriptbank\\")+t.entityelement[t.e].eleprof.aiinit_s ; addtocollection(t.tfile_s.Get()); //PE: Not used anymore.
				t.tfile_s=cstr("scriptbank\\")+t.entityelement[t.e].eleprof.aimain_s ; addtocollection(t.tfile_s.Get());
				// t.tfile_s=cstr("scriptbank\\")+t.entityelement[t.e].eleprof.aidestroy_s ; addtocollection(t.tfile_s.Get()); //PE: Not used anymore.
				// t.tfile_s=cstr("scriptbank\\")+t.entityelement[t.e].eleprof.aishoot_s ; addtocollection(t.tfile_s.Get()); //PE: Not used anymore.
				//  sound files
				t.tfile_s=t.entityelement[t.e].eleprof.soundset_s ; addtocollection(t.tfile_s.Get());
				t.tfile_s=t.entityelement[t.e].eleprof.soundset1_s ; addtocollection(t.tfile_s.Get());
				t.tfile_s=t.entityelement[t.e].eleprof.soundset2_s ; addtocollection(t.tfile_s.Get());
				t.tfile_s=t.entityelement[t.e].eleprof.soundset3_s ; addtocollection(t.tfile_s.Get());
				t.tfile_s=t.entityelement[t.e].eleprof.soundset4_s ; addtocollection(t.tfile_s.Get());
				//  collectable guns
				if (  Len(t.entityprofile[t.entid].isweapon_s.Get())>1 ) 
				{
					t.tfile_s=cstr("gamecore\\guns\\")+t.entityprofile[t.entid].isweapon_s ; addfoldertocollection(t.tfile_s.Get());
					t.foundgunid=t.entityprofile[t.entid].isweapon;
					if (  t.foundgunid>0 ) 
					{
						for ( t.x = 0 ; t.x<=  1; t.x++ )
						{
							t.tpoolindex=g.firemodes[t.foundgunid][t.x].settings.poolindex;
							if (  t.tpoolindex>0 ) 
							{
								t.tfile_s=cstr("gamecore\\ammo\\")+t.ammopool[t.tpoolindex].name_s ; addfoldertocollection(t.tfile_s.Get());
							}
						}
					}
				}
				//  associated guns and ammo
				if (  Len(t.entityelement[t.e].eleprof.hasweapon_s.Get())>1 ) 
				{
					t.tfile_s=cstr("gamecore\\guns\\")+t.entityelement[t.e].eleprof.hasweapon_s ; addfoldertocollection(t.tfile_s.Get());
					t.foundgunid=t.entityelement[t.e].eleprof.hasweapon;
					if (  t.foundgunid>0 ) 
					{
						for ( t.x = 0 ; t.x<=  1; t.x++ )
						{
							t.tpoolindex=g.firemodes[t.foundgunid][t.x].settings.poolindex;
							if (  t.tpoolindex>0 ) 
							{
								t.tfile_s=cstr("gamecore\\ammo\\")+t.ammopool[t.tpoolindex].name_s ; addfoldertocollection(t.tfile_s.Get());
							}
						}
					}
				}
				//  zone marker can reference other levels to jump to
				if (  t.entityprofile[t.entid].ismarker == 3 ) 
				{
					t.tlevelfile_s=t.entityelement[t.e].eleprof.ifused_s;
					if (  Len(t.tlevelfile_s.Get())>1 ) 
					{
						t.tlevelfile_s=cstr("mapbank\\")+t.tlevelfile_s+".fpm";
						addtocollection(t.tlevelfile_s.Get());
					}
				}
			}

		}
	}

	//  fill in the .dat file
	SetDir (  cstr(g.fpscrootdir_s+"\\Files\\").Get() );
	t.filesmax=g.filecollectionmax;
	t.thowmanyadded = 0;
	for ( t.fileindex = 1 ; t.fileindex<=  t.filesmax; t.fileindex++ )
	{
		t.name_s=t.filecollection_s[t.fileindex];
		if (  cstr(Left(t.name_s.Get(),12))  ==  "entitybank\\\\"  )  t.name_s  =  cstr("entitybank\\") + Right(t.name_s.Get(), Len(t.name_s.Get())-12);
		if (  cstr(Left(t.name_s.Get(),12))  ==  "scriptbank\\\\"  )  t.name_s  =  cstr("scriptbank\\") + Right(t.name_s.Get(), Len(t.name_s.Get())-12);
		if (  FileExist(t.name_s.Get()) == 1 ) 
		{
			if (  mp_check_if_entity_is_from_install(t.name_s.Get())  ==  0 ) 
			{
				WriteString (  1,t.name_s.Get() );
				//  check if it is character creator, if it is, check for the existance of a texture
				if (  cstr(Lower(Left(t.name_s.Get(),32)))  ==  "entitybank\\user\\charactercreator" ) 
				{
					t.tname_s = cstr(Left(t.name_s.Get(), Len(t.name_s.Get())-4)) + "_cc.dds";
					if (  FileExist(t.tname_s.Get())  ==  1 ) 
					{
						WriteString (  1,t.tname_s.Get() );
						++t.thowmanyadded;
					}
				}

				++t.thowmanyadded;
				//  09032015 - 017 - If its a gun, grab the muzzleflash, decals and include them
				if (  cstr(Right(t.name_s.Get(),11))  ==  "gunspec.txt" ) 
				{
					if (  FileOpen(3)  )  CloseFile (  3 );
					t.tfoundflash = 0;
					OpenToRead (  3,t.name_s.Get() );
					t.tfoundflash = 0;
					while (  FileEnd(3)  ==  0 && t.tfoundflash  ==  0 ) 
					{
						t.tisthisflash_s = ReadString ( 3 );
						if (  cstr(Left(t.tisthisflash_s.Get(),11))  ==  "muzzleflash" ) 
						{
							t.tlocationofequals = FindLastChar(t.tisthisflash_s.Get(),"=");
							if (  t.tlocationofequals > 1 ) 
							{
								if (  cstr(Mid(t.tisthisflash_s.Get(),t.tlocationofequals+1))  ==  " " ) 
								{
									t.tflash_s = Right(t.tisthisflash_s.Get(),Len(t.tisthisflash_s.Get())-(t.tlocationofequals+1));
								}
								else
								{
									t.tflash_s = Right(t.tisthisflash_s.Get(),Len(t.tisthisflash_s.Get())-(t.tlocationofequals));
								}
								t.tfext_s = "";
								if (  FileExist( cstr(cstr("gamecore\\muzzleflash\\flash")+t.tflash_s+".png").Get() )  ==  1  )  t.tfext_s  =  ".png";
								if (  FileExist( cstr(cstr("gamecore\\muzzleflash\\flash")+t.tflash_s+".dds").Get() )  ==  1  )  t.tfext_s  =  ".dds";
								if (  t.tfext_s  !=  "" ) 
								{
									WriteString (  1,cstr(cstr("gamecore\\muzzleflash\\flash")+t.tflash_s+t.tfext_s).Get() );
									++t.thowmanyadded;
								}
								t.tfext_s = "";
								if (  FileExist(cstr(cstr("gamecore\\decals\\muzzleflash")+t.tflash_s+"\\decal.png").Get() )  ==  1  )  t.tfext_s  =  ".png";
								if (  FileExist(cstr(cstr("gamecore\\decals\\muzzleflash")+t.tflash_s+"\\decal.dds").Get() )  ==  1  )  t.tfext_s  =  ".dds";
								if (  t.tfext_s  !=  "" ) 
								{
									WriteString (  1,cstr(cstr("gamecore\\decals\\muzzleflash")+t.tflash_s+"\\decal"+t.tfext_s).Get() );
									++t.thowmanyadded;
								}
								t.tfext_s = "";
								if (  FileExist(cstr(cstr("gamecore\\decals\\muzzleflash")+t.tflash_s+"\\decalspec.txt").Get() )  ==  1  )  t.tfext_s  =  ".txt";
								if (  t.tfext_s  !=  "" ) 
								{
									WriteString (  1,cstr(cstr("gamecore\\decals\\muzzleflash")+t.tflash_s+"\\decalspec"+t.tfext_s).Get() );
									++t.thowmanyadded;
								}
								t.tfoundflash = 1;
							}
						}
					}
					CloseFile (  3 );
				}
			}
		}
	}

	CloseFile (  1 );

	//  if (  it is just the fpm  )  there are is no custom media with this level
	if (  t.thowmanyadded  <=  1  )  DeleteAFile (  t.tworkshoplistfile_s.Get() );

	//  cleanup file array
	UnDim (  t.filecollection_s );

	//  Restore directory
	SetDir (  t.told_s.Get() );

	UnDim (  t.tallfpefiles );

	t.tmasterlevelfile_s = toriginalMasterLevelFile_s;
	g.projectfilename_s = toriginalprojectname_s;

}

void mp_grabWorkshopChangedFlagAndVersion ( void )
{
	if (  FileExist(t.tmphopitemtocheckifchangedandversion_s.Get())  ==  1 ) 
	{
		if (  FileOpen(1)  )  CloseFile (  1 );
		OpenToRead (  1,t.tmphopitemtocheckifchangedandversion_s.Get() );
		//  skip the warning message
		t.tnothing_s = ReadString ( 1 );
		//  read in flag changed
		t.tnothing_s = ReadString ( 1 );
		t.tMPshopHasItemChangedFlag = ValF(t.tnothing_s.Get());
		//  read in version number
		t.tnothing_s = ReadString ( 1 );
		t.tMPshopTheVersionNumber = ValF(t.tnothing_s.Get());
		//  read in workshop id
		t.tnothing_s = ReadString ( 1 );
		t.tMPshopTheIDNumber_s = t.tnothing_s;
		CloseFile (  1 );
	}
return;

//  Check to see if this file is part of the base install
}

int mp_check_if_entity_is_from_install ( char* name_s )
{
	int ttemploop = 0;
	int ttresult = 0;
	ttresult = 0;
	if (  cstr(Left(name_s,12))  ==  "entitybank\\\\"  )  strcpy ( name_s  , cstr(cstr("entitybank\\") + Right(name_s, Len(name_s)-12)).Get() );
	if (  cstr(Right(name_s,3))  ==  "bin" ) 
	{
		strcpy ( name_s , cstr(cstr(Lower(Left(name_s,Len(name_s)-3))) + cstr("fpe")).Get() );
	}
	else
	{
		name_s = Lower(name_s);
	}
	cstr nameCheck = cstr(name_s);
	for ( ttemploop = 0 ; ttemploop<=  g.mp.howmanyfpefiles-1; ttemploop++ )
	{
		cstr listFile = cstr( _strlwr(t.tallfpefiles_s[ttemploop].Get()) );
		if (  nameCheck  ==  listFile ) 
		{
			ttresult = 1;
			break;
		}
	}
//endfunction ttresult
	return ttresult;
}

void mp_resetSteam ( void )
{
		mp_free ( );
		mp_init ( );
		mp_resetGameStats ( );
		g.mp.needToResetOnStartup = 0;
return;

}

void mp_shoot ( void )
{
	if (  t.weaponammo[g.weaponammoindex+g.ammooffset]>0 ) 
	{
		SteamShoot (  );
	}
return;

}

void mp_chat ( void )
{

	//  check for chat
	t.tchat_s = SteamGetChat();
	if (  t.tchat_s  !=  "" ) 
	{
		mp_chatNew ( );
	}

	t.tscancode = ScanCode();
	if (  KeyState(g.keymap[28])  ==  1 && t.oldchatscancode  !=  28 ) 
	{
		g.mp.chaton = 1-g.mp.chaton;
		if (  g.mp.chaton  ==  1 ) 
		{
			//  start a new chat message
			ClearEntryBuffer (  );
			g.mp.chatstring = "";
		}
		else
		{
			//  send the chat message
			//  local send
			if (  Len(g.mp.chatstring.Get())  ==  1 ) 
			{
				if (  Asc(Mid(g.mp.chatstring.Get(),1))  <=  31  )  g.mp.chatstring  =  "";
			}
			else
			{
				if (  Asc(Mid(g.mp.chatstring.Get(),1))  <=  31  )  g.mp.chatstring  =  Right(g.mp.chatstring.Get(),Len(g.mp.chatstring.Get())-1);
			}
			if (  g.mp.chatstring !=  "" ) 
			{
				t.tchat_s = cstr(cstr(Str(g.mp.me)) + Left( cstr(cstr("<") + SteamGetPlayerName() + "> " + g.mp.chatstring).Get(),80)).Get();
				mp_chatNew ( );
				g.mp.chatstring = "";
				if (  t.tchatLobbyMode  ==  0 ) 
				{
					SteamSendChat (  t.tchat_s.Get() );
				}
				else
				{
					//  030315 - 013 - Lobby chat
					SteamSendLobbyChat (  t.tchat_s.Get() );
				}
			}
		}
	}
	if (  g.mp.chaton  ==  1 ) 
	{
		if (  Timer() - g.mp.lastSpawnedTime > 1000 ) 
		{
			t.aisystem.processplayerlogic=0;
		}
		else
		{
			t.aisystem.processplayerlogic=1;
		}
		g.mp.chattimer = Timer();

		g.mp.chatstring = Entry(1);
		//  show the Text (  we have typed )
		if (  Timer() - t.chatcursortime > 250 ) 
		{
			t.chatcursortime = Timer();
			g.mp.cursoron = 1-g.mp.cursoron;
		}
		if (  g.mp.cursoron  ==  0 ) 
		{
			t.tstringtoshow_s = cstr(Left(cstr(cstr("<") + SteamGetPlayerName() + "> " + g.mp.chatstring).Get(),80));
		}
		else
		{
			t.tstringtoshow_s = cstr(Left(cstr(cstr("<") + SteamGetPlayerName() + "> " + g.mp.chatstring).Get(),80)) + cstr("|");
		}
	}
	else
	{
		t.aisystem.processplayerlogic=1;
	}
	t.oldchatscancode = t.tscancode;

	if (  Timer() - g.mp.chattimer  <=  MP_CHAT_DELAY+2550 ) 
	{
		t.ttimegone = Timer()-t.toldchattime;
		if (  t.ttimegone > 50 ) 
		{
			t.toldchattime = Timer();
			t.ttimegone = 16;
		}
		t.toldchattime = Timer();
		if (  Timer() - g.mp.chattimer  >=  MP_CHAT_DELAY ) 
		{
			t.tsteamalpha = t.tsteamalpha - t.ttimegone;
		}
		else
		{
			t.tsteamalpha = t.tsteamalpha + t.ttimegone;
		}
		if (  t.tsteamalpha < 0  )  t.tsteamalpha  =  0;
		if (  t.tsteamalpha > 255  )  t.tsteamalpha  =  255;
		
		if (  t.tsteamalpha > 0 ) 
		{
			InkEx ( 20, 20, 20 );//  Rgb(20,20,20),Rgb(20,20,20) );
			BoxEx (  5,5,(40*10)+5,((MP_MAX_CHAT_LINES+1)*15)+10 );
		}

		t.tsteamy = 10;
		for ( t.tloop = 0 ; t.tloop<=  MP_MAX_CHAT_LINES-1; t.tloop++ )
		{
			if (  t.mp_chat[t.tloop] != "" ) 
			{
				if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "0" ) { t.r  =  255 ; t.g  =  255 ; t.b  =  50; }
				else if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "1" ) { t.r  =  255  ; t.g  =  100 ; t.b  =  100; }
				else if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "2" ) { t.r  =  100  ; t.g  =  255 ; t.b  =  100; }
				else if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "3" ) { t.r  =  100  ; t.g  =  100 ; t.b  =  255; }
				else if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "4" ) { t.r  =  255  ; t.g  =  255 ; t.b  =  100; }
				else if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "5" ) { t.r  =  255  ; t.g  =  100 ; t.b  =  255; }
				else if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "6" ) { t.r  =  100  ; t.g  =  255 ; t.b  =  255; }
				else if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "7" ) { t.r  =  200  ; t.g  =  255 ; t.b  =  200; }
				else if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "s" ) { t.r  =  255  ; t.g  =  255 ; t.b  =  255; }
				t.tluarealcoords = 1;
				t.tluatextalpha = t.tsteamalpha;
				mp_textColor(10,t.tsteamy,2,Right(t.mp_chat[t.tloop].Get(),Len(t.mp_chat[t.tloop].Get())-1),t.r,t.g,t.b);
			}
			t.tsteamy += 14;
		}

		if (  g.mp.chaton  ==  1 ) 
		{
			t.tluarealcoords = 1;
			t.tluatextalpha = t.tsteamalpha;
			t.tsteamy = 10+((MP_MAX_CHAT_LINES)*15);
			mp_textColor(10,t.tsteamy,2,t.tstringtoshow_s.Get(),255,255,255);
		}
	}
	InkEx ( 255, 255, 255 );// Rgb(255,255,255),Rgb(0,0,0) );
}

void mp_chatNew ( void )
{
	for ( t.tloop = 0 ; t.tloop<=  MP_MAX_CHAT_LINES-2; t.tloop++ )
	{
		t.mp_chat[t.tloop] = t.mp_chat[t.tloop+1];
	}
	if (  Len(t.tchat_s.Get()) > 80  )  t.tchat_s  =  Left(t.tchat_s.Get(),80);
	if (  cstr(Left(t.tchat_s.Get(),1))  !=  "s" ) 
	{
		t.mp_chat[MP_MAX_CHAT_LINES-1] = t.tchat_s;
		g.mp.chattimer = Timer();
	}
	//  200315 - 021 - pick up users joining the game from the server message sent
	if (  cstr(Left(t.tchat_s.Get(),1))  ==  "s" ) 
	{
		t.tnametocheckforjoining_s = Right(t.tchat_s.Get(),Len(t.tchat_s.Get())-1);
		for ( t.tn = 0 ; t.tn<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tn++ )
		{
			if (  cstr(Left(t.tnametocheckforjoining_s.Get(),Len(t.mp_joined[t.tn].Get())))  ==  t.mp_joined[t.tn] && t.mp_joined[t.tn]  !=  "" && cstr(Right(t.mp_joined[t.tn].Get(),6))  !=  "Joined" ) 
			{
				t.mp_joined[t.tn] = t.mp_joined[t.tn] + " - Joined";
//     `if tlastchatline$  !=  tchat$ then mp_chat(MP_MAX_CHAT_LINES-1)  ==  tchat$

//     `tlastchatline$ = tchat$

			}
		}
	}
}

void mp_quitGame ( void )
{
	// exit current game and return to multiplayer menu
	// 110315 - 019 - first lets fade out nice
	t.tstartoffade = Timer();
	t.tfadestealpha_f = 0.0;
	t.tspritetouse = 0;
	for ( t.tloop = 2000 ; t.tloop<=  3000; t.tloop++ )
	{
		if (  SpriteExist(t.tloop)  ==  0 ) { t.tspritetouse  =  t.tloop  ; break; }
	}
	while (  Timer() - t.tstartoffade < 500 ) 
	{
		t.tfadestealpha_f = (Timer() - t.tstartoffade)*2;
		if (  t.tfadestealpha_f < 0  )  t.tfadestealpha_f  =  0.0;
		if (  t.tfadestealpha_f > 255.0  )  t.tfadestealpha_f  =  255.0;
		if (  t.tspritetouse > 0 && ImageExist(g.panelimageoffset+1)  ==  1 ) 
		{
			Sprite (  t.tspritetouse,0,0,g.panelimageoffset+1 );
			SizeSprite (  t.tspritetouse,GetDisplayWidth()*10, GetDisplayHeight()*10 );
			SetSpriteDiffuse (  t.tspritetouse,0,0,0 );
			SetSpriteAlpha (  t.tspritetouse,t.tfadestealpha_f );
		}
		SteamLoop (  );
		Sync (  );
	}
	//  if the server, let everyone know instantly the server is dropping
	//  020315 - 012 - Send an end game message when the host decides to leave
	if (  g.mp.isGameHost  ==  1 ) 
	{
		SteamEndGame (  );
	}

	t.game.gameloop=0;
	t.game.levelloop=0;
	t.game.titleloop=0;
	t.game.quitflag=1;
}

void mp_freefadesprite ( void )
{
	// 240316 - v1.13b1 - free sprite now finished with fade
	if ( t.tspritetouse > 0 )
	{
		DeleteSprite ( t.tspritetouse );
		t.tspritetouse = 0;
	}
}

void mp_backToEditor ( void )
{
	t.game.gameloop=0;
	t.game.levelloop=0;
	t.game.titleloop=0;
	t.game.quitflag=1;
	g.mp.goBackToEditor = 1;
return;

//  remove all entities and lightmaps that are left from our gaming session
}

void mp_cleanupGame ( void )
{

	//  default start position is edit-camera XZ
	t.terrain.playerx_f=t.cx_f;
	t.terrain.playerz_f=t.cy_f;
	if (  t.terrain.TerrainID>0 ) 
	{
		t.terrain.playery_f=BT_GetGroundHeight(t.terrain.TerrainID,t.terrain.playerx_f,t.terrain.playerz_f)+150.0;
	}
	else
	{
		t.terrain.playery_f=1000.0+150.0;
	}
	t.terrain.playerax_f=0.0;
	t.terrain.playeray_f=0.0;
	t.terrain.playeraz_f=0.0;

	//  remove light map objects for return to IDE editor
	lm_restoreall ( );

	//  remove all entities
	if (  g.entityelementlist>0 ) 
	{
		for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
		{
			t.obj=t.entityelement[t.e].obj;
			if (  t.obj>0 ) 
			{
				if (  ObjectExist(t.obj) == 1 ) 
				{
					DeleteObject (  t.obj );
				}
			}
			t.entityelement[t.e].obj=0;
			t.entityelement[t.e].bankindex=0;
		}
		g.entityelementlist=0;
	}

return;

//  Send Steam User ID to editor via file mapping

}

void mp_sendSteamIDToEditor ( void )
{

	if (  g.mp.isRunning  ==  0 ) 
	{
		//  was 60*1000, changing to 5 to keep try and connecting
		if (  Timer() - g.mp.lastTimeTriedToConnectToSteamFromEditor > 5*1000 ) 
		{
			g.mp.lastTimeTriedToConnectToSteamFromEditor = Timer();
			mp_resetSteam ( );
		}
		if (  g.mp.isRunning  ==  0 ) 
		{
			return;
		}
	}
	else
	{
		//  send user id

		//  Debug code put it for a user that had issues with the downloader (which needs to know the steam id from steam, if steam doesnt give it, it wont work)
//   `set cursor 0,30

//   `print "steam id = " + tSteamGetID$


//   `print Timer()

//   `print mp.lastTimeISentMySteamID


		if (  Timer() - g.mp.lastTimeISentMySteamID > 5000 ) 
		{

			t.tSteamGetID_s = SteamGetPlayerID();

			if (  t.tSteamGetID_s  !=  "" ) 
			{
//     `print "sending id"

				g.mp.lastTimeISentMySteamID = Timer();

				OpenFileMap (  1, "FPSEXCHANGE" );

				//  params; filemap number, offset in bytes, value
				SetFileMapDWORD (  1, 6145, 1 );
				SetFileMapString (  1, 6149 , t.tSteamGetID_s.Get() );
				SetEventAndWait (  1 );

				//  Close when set all defaults
				//CloseFileMap (  1 );

				g.mp.haveSentSteamIDToEditor = 1;
			}
			else
			{
				mp_resetSteam ( );
				g.mp.lastTimeISentMySteamID = Timer()-3000;
//     `print "resetting steam"

			}


		}
	}

return;

//  020315 - 012 - enable check for lobbies while in editor
}

void mp_checkIfLobbiesAvailable ( void )
{
	if (  t.thowlongbetweenlobbychecks  <=  0  )  t.thowlongbetweenlobbychecks  =  15*1000;
	if (  Timer() - t.tlasttimecheckedforlobbiestimer > t.thowlongbetweenlobbychecks ) 
	{
		SteamLoop (  );
		t.tlasttimecheckedforlobbiestimer = Timer();
		if (  g.mp.checkiflobbiesavailablemode  ==  0 ) 
		{
			SteamGetLobbyList (  );
			g.mp.checkiflobbiesavailablemode = 1;
			return;
		}
		if (  g.mp.checkiflobbiesavailablemode  ==  1 ) 
		{
			g.mp.checkiflobbiesavailablemode = 0;

			if (  SteamIsLobbyListCreated()  ==  0 ) 
			{
				return;
			}


			t.thowmanylobbiesavailable = SteamGetLobbyListSize();
			if (  t.thowmanylobbiesavailable > 0 ) 
			{
				if (  t.thowmanylobbiesavailable > 10 ) 
				{
					t.thowlongbetweenlobbychecks = 60*1000;
				}
				if (  t.thowmanylobbiesavailable > 20 ) 
				{
					t.thowlongbetweenlobbychecks = 150*1000;
				}
				if (  t.thowmanylobbiesavailable > 40 ) 
				{
					t.thowlongbetweenlobbychecks = 300*1000;
				}
				t.steamStatusBar_s = "        |        Multiplayer Lobbies available to join";
			}
			else
			{
				t.thowlongbetweenlobbychecks = 15*1000;
				t.steamStatusBar_s = "";
			}
		}
	}
return;

//  200315 - 021 - flashlight of when starting a game
}

void mp_flashLightOff ( void )
{
	t.playerlight.flashlightcontrol_f=0.0;
return;

//  set everyone to team A for coop mode
}

void mp_setupCoopTeam ( void )
{
	for ( t.tc = 0 ; t.tc<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tc++ )
	{
		t.mp_team[t.tc] = 0;
	}
return;

//  requires; e, tSteamX# and tSteamZ#
}

void mp_COOP_aiMoveTo ( void )
{

	if (  g.mp.endplay  ==  1  )  return;

	if (  t.game.runasmultiplayer  ==  1 && g.mp.coop  ==  1 && t.tLuaDontSendLua  ==  0 ) 
	{

		SteamSendLua (  MP_LUA_AiGoToX,t.e,t.tSteamX_f );
		SteamSendLua (  MP_LUA_AiGoToZ,t.e,t.tSteamZ_f );
	}
	else
	{
		//  lets check if the distance is worth the effort of moving
		t.tdx_f=t.tSteamX_f-ObjectPositionX(t.e);
		t.tdz_f=t.tSteamZ_f-ObjectPositionZ(t.e);
		t.tdist_f=Sqrt((t.tdx_f*t.tdx_f)+(t.tdz_f*t.tdz_f));

		//  is it isn't very far, lets just stop the ai so it doesnt jerk about
		if (  t.tdist_f < 75.0 ) 
		{
//    `AI Set Entity Position e,ObjectPositionX(e),ObjectPositionY(e),ObjectPositionZ(e)

			AISetEntityPosition (  t.e,t.tSteamX_f,BT_GetGroundHeight(t.terrain.TerrainID,t.tSteamX_f,t.tSteamZ_f),t.tSteamZ_f );
			AIEntityStop (  t.e );
		//  if it is a real destination, lets head there
		}
		else
		{
			AIEntityGoToPosition (  t.e, t.tSteamX_f, t.tSteamZ_f );
		}

	}
return;

}

void mp_entity_lua_lookatplayer ( void )
{
	entity_lua_findcharanimstate ( );
	if (  t.tcharanimindex != -1 ) 
	{

		//  Simply look in direction of player
		t.ee = t.mp_playerEntityID[t.v];
		t.tdx_f= ObjectPositionX (t.entityelement[t.ee].obj) - ObjectPositionX(t.entityelement[t.e].obj);
		t.tdz_f= ObjectPositionZ (t.entityelement[t.ee].obj) - ObjectPositionZ(t.entityelement[t.e].obj);
		AISetEntityAngleY (  t.charanimstate.obj,atan2deg(t.tdx_f,t.tdz_f) );
		
		//  If angle beyond 'look angle range', perform full rotation
		t.tangley_f=AIGetEntityAngleY(t.charanimstate.obj) ;
		t.headangley_f=t.tangley_f-ObjectAngleY(t.charanimstate.obj) ;
		if (  t.headangley_f<-180  )  t.headangley_f = t.headangley_f+360;
		if (  t.headangley_f>180  )  t.headangley_f = t.headangley_f-360;
		if (  t.headangley_f<-75 || t.headangley_f>75 ) 
		{
			t.charanimstate.currentangle_f=t.tangley_f;
			t.charanimstate.updatemoveangle=1;
			AISetEntityAngleY (  t.charanimstate.obj,t.charanimstate.currentangle_f );
			t.charanimstates[t.tcharanimindex] = t.charanimstate;
		}

	}
return;

}

void mp_entity_lua_fireweaponEffectOnly ( void )
{
	//  update gun appearance
	if (  t.entityelement[t.e].attachmentobj > 0 ) 
	{
		entity_controlattachments ( );

		t.tgunid=t.entityelement[t.e].eleprof.hasweapon;
		t.tattachedobj=t.entityelement[t.e].attachmentobj;
		t.te = t.e;

		t.tgunid = t.entityelement[t.e].eleprof.hasweapon;
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
						t.charanimstate.firesoundindex=t.ttsnd ; t.tt=3;
						t.charanimstate.firesoundexpiry=Timer()+200;
					}
				}
			}
		}

		if (  t.charanimstate.firesoundindex>0 ) 
		{
			entity_lua_findcharanimstate ( );
			darkai_shooteffect ( );
		}

	}

	//  charanimstate is purely temporary, the firesoundindex will NOT be persistent!!
	t.charanimstate.firesoundindex=0;

return;

//  cycle through entities, pick out the ai and either take aggro or update depending on distance/ownership
}

void mp_updateAIForCOOP ( void )
{

		if (  g.mp.endplay  ==  1  )  return;

		t.tsentone = 0;
//   `set cursor 0,0


		if (  t.game.runasmultiplayer == 1 && g.mp.coop  ==  1 ) 
		{
			t.thowManyDoIHave = 0;
			//  only send one update to other players max
			//  so everyone gets a chance to update we keep track of where we were up to in the list last time we sent
			if (  t.tcoopyentityupdatetostartat  ==  0 || t.tcoopyentityupdatetostartat > g.entityelementlist ) 
			{
				t.tcoopyentityupdatetostartat = 1;
				++t.tcoopSendPositionUpdate;
				if (  t.tcoopSendPositionUpdate > 3  )  t.tcoopSendPositionUpdate  =  0;
			}
			for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
			{
				t.entid=t.entityelement[t.e].bankindex;
				if (  t.entid>0 ) 
				{
					if (  (t.entityprofile[t.entid].ischaracter  ==  1 || t.entityelement[t.e].mp_isLuaChar  ==  1) && t.entityprofile[t.entid].ismultiplayercharacter  ==  0 ) 
					{
						if (  t.entityelement[t.e].health  ==  0 ) 
						{
							t.distx_f = CameraPositionX() - ObjectPositionX(t.entityelement[t.e].obj);
							t.distz_f = CameraPositionZ() - ObjectPositionZ(t.entityelement[t.e].obj);
							t.tdist_f = Sqrt((t.distx_f*t.distx_f)+(t.distz_f*t.distz_f));
							if (  t.tdist_f > 3000  )  ScaleObject (  t.entityelement[t.e].obj,0,0,0 );
						}
						if (  t.entityelement[t.e].mp_coopControlledByPlayer  !=  g.mp.me && t.entityelement[t.e].active == 1 && t.entityelement[t.e].health > 0 ) 
						{
							// rotate or look at player for 1 second after receiving lua command, to cut down packets being sent
							if (  t.entityelement[t.e].mp_rotateType  ==  1 ) 
							{
								if (  Timer() - t.entityelement[t.e].mp_rotateTimer > 1000  )  t.entityelement[t.e].mp_rotateType  =  0;
								if (  t.entityelement[t.e].obj > 0 ) 
								{
									if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
									{
										if (  t.entityelement[t.e].mp_coopControlledByPlayer > -1 && t.entityelement[t.e].mp_coopControlledByPlayer < MP_MAX_NUMBER_OF_PLAYERS ) 
										{
											t.v = t.entityelement[t.e].mp_coopControlledByPlayer;
											mp_entity_lua_lookatplayer ( );
										}
									}
								}
							}
							if (  t.entityelement[t.e].mp_rotateType  ==  2 ) 
							{
								if (  Timer() - t.entityelement[t.e].mp_rotateTimer > 1000  )  t.entityelement[t.e].mp_rotateType  =  0;
								if (  t.entityelement[t.e].obj > 0 ) 
								{
									if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
									{
										if (  t.entityelement[t.e].mp_coopControlledByPlayer > -1 && t.entityelement[t.e].mp_coopControlledByPlayer < MP_MAX_NUMBER_OF_PLAYERS ) 
										{
											t.v = t.entityelement[t.e].mp_coopControlledByPlayer;
											mp_entity_lua_lookatplayer ( );
										}
									}
								}
							}

							t.distx_f = CameraPositionX() - ObjectPositionX(t.entityelement[t.e].obj);
							t.distz_f = CameraPositionZ() - ObjectPositionZ(t.entityelement[t.e].obj);
							t.tdist_f = Sqrt((t.distx_f*t.distx_f)+(t.distz_f*t.distz_f));
							if (  t.tdist_f < 1200 || ( AIGetEntityHeardSound(t.entityelement[t.e].obj)  ==  1 && t.entityelement[t.e].mp_coopControlledByPlayer  ==  -1 ) ) 
							{
								if (  t.entityelement[t.e].mp_coopControlledByPlayer  ==  -1 ) 
								{
									t.tsteamplayeralive = 0;
								}
								else
								{
									t.tsteamplayeralive = SteamGetPlayerAlive(t.entityelement[t.e].mp_coopControlledByPlayer);
								}
								if (  Timer() - t.entityelement[t.e].mp_coopLastTimeSwitchedTarget > 5000 || t.tsteamplayeralive  ==  0 ) 
								{
									t.tthrowaway = Rnd(1);
									if (  t.tsteamplayeralive  ==  0 || t.entityelement[t.e].mp_coopControlledByPlayer  ==  -1  )  t.tthrowaway  =  1;
									if (  t.tthrowaway  ==  1 ) 
									{
										t.entityelement[t.e].mp_coopControlledByPlayer = g.mp.me;
										SteamSendLua (  MP_LUA_TakenAggro,t.e,g.mp.me );
										SteamSendLua (  MP_LUA_AiGoToX,t.entityelement[t.e].obj,ObjectPositionX(t.entityelement[t.e].obj) );
										SteamSendLua (  MP_LUA_AiGoToZ,t.entityelement[t.e].obj,ObjectPositionZ(t.entityelement[t.e].obj) );
//           `AI Entity Stop entityelement(e).obj

										t.entityelement[t.e].mp_updateOn = 1;
										t.entityelement[t.e].mp_lastUpdateSent = 0;
									}
									t.entityelement[t.e].mp_coopLastTimeSwitchedTarget = Timer()+5000;
								}
							}
						}
						else
						{
							if (  t.entityelement[t.e].mp_coopControlledByPlayer  ==  g.mp.me ) 
							{
								if (  t.entityelement[t.e].mp_updateOn  ==  1 && t.tsentone  ==  0 ) 
								{
									if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
									{
										AISetEntityActive (  t.entityelement[t.e].obj,1 );
									}
									if (  t.tcoopyentityupdatetostartat  <=  t.e ) 
									{
//           ``

										if (  t.entityelement[t.e].active == 1 && t.entityelement[t.e].health > 0 ) 
										{

											if (  Timer() - t.tcoopLastUpdateSent > 500 || t.tcoopLastUpdateSent < 0 ) 
											{
													t.tsentone = 1;
													SteamSendLua (  MP_LUA_HaveAggro,t.e,g.mp.me );
													SteamSendLua (  MP_LUA_AiGoToX,t.entityelement[t.e].obj,ObjectPositionX(t.entityelement[t.e].obj) );
													SteamSendLua (  MP_LUA_AiGoToZ,t.entityelement[t.e].obj,ObjectPositionZ(t.entityelement[t.e].obj) );
													t.entityelement[t.e].mp_lastUpdateSent = Timer();
													t.tcoopLastUpdateSent = Timer();
													t.tcoopyentityupdatetostartat = t.e+1;

//             `endif

											}
											else
											{
												//  pretend way have sent one this time, since we need to wait a little while
												t.tsentone = 1;
											}

										}
//           ``

									}
								}
							}

						}
					}
				}
			}
			//  if we havent sent anything, reset the list
			if (  t.tsentone  ==  0 ) 
			{
				t.tcoopyentityupdatetostartat = 1;
				++t.tcoopSendPositionUpdate;
				if (  t.tcoopSendPositionUpdate > 3  )  t.tcoopSendPositionUpdate  =  0;
			}
		}

return;

}

void mp_coop_rotatetoplayer ( void )
{
	//  only rotate to player if enemy ai with proper rig
	if (  t.entityelement[t.e].mp_isLuaChar  ==  1  )  return;
	entity_lua_findcharanimstate ( );
	if (  t.tcharanimindex == -1 ) 
	{
		//  regular entity
		t.entityelement[t.e].ry=t.v;
		entity_lua_rotateupdate ( );
	}
	else
	{
		// character subsystem
		t.charanimstate.currentangle_f=t.v;
		t.charanimstate.updatemoveangle=1;
		AISetEntityAngleY ( t.charanimstate.obj,t.charanimstate.currentangle_f );
		t.charanimstates[t.tcharanimindex] = t.charanimstate;
		t.entityelement[t.e].ry=t.v;

		// 240217 - and update visually
		entity_lua_rotateupdate ( );
	}
}

void mp_storeOldEntityPositions ( void )
{

	Dim (  t.mp_oldEntityPositionsX,g.entityelementlist );
	Dim (  t.mp_oldEntityPositionsZ,g.entityelementlist );

	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if (  t.entid>0 ) 
		{
			if (  (t.entityprofile[t.entid].ischaracter  ==  1 || t.entityelement[t.e].mp_isLuaChar  ==  1) && t.entityprofile[t.entid].ismultiplayercharacter  ==  0 ) 
			{
				if (  t.entityelement[t.e].obj > 0 ) 
				{
					if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
					{
						t.mp_oldEntityPositionsX[t.e] = ObjectPositionX(t.entityelement[t.e].obj);
						t.mp_oldEntityPositionsZ[t.e] = ObjectPositionZ(t.entityelement[t.e].obj);
					}
				}
			}
		}
	}

return;

}

void mp_howManyEnemiesLeftToKill ( void )
{
		if (  g.mp.coop  ==  1 ) 
		{
			t.thowmanyenemies = 0;
			for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
			{
				t.entid=t.entityelement[t.e].bankindex;
				if (  t.entid>0 ) 
				{
					if (  t.entityprofile[t.entid].ischaracter  ==  1 || t.entityelement[t.e].mp_isLuaChar  ==  1 ) 
					{
						if (  t.entityelement[t.e].active  ==  1 && t.entityelement[t.e].health > 0 ) 
						{
							++t.thowmanyenemies;
						}
					}
				}
			}
			LuaSetInt (  "mp_enemiesLeftToKill", t.thowmanyenemies );
		}
return;

}

void mp_IKilledAnAI ( void )
{
	t.mp_kills[g.mp.me+1] = t.mp_kills[g.mp.me+1] + 1;
	SteamSendLua (  MP_LUA_ServerSetPlayerKills,g.mp.me+1,t.mp_kills[g.mp.me+1] );
	t.tnothing = LuaExecute( cstr(cstr("mp_playerKills[") + Str(g.mp.me+1) + "] = " + Str(t.mp_kills[g.mp.me+1])).Get());
return;

// `////////////////////////////////////////////////////////////////////////////////////////////////////////////


}

void mp_text ( int x, int y, int size, char* txt_s )
{
	t.luaText.txt = txt_s;
	t.luaText.x = x;
	t.luaText.y = y;
	t.luaText.size = size;
	lua_text ( );
//endfunction

}

void mp_textDots ( int x, int y, int size, char* txt_s )
{

	if (  Timer() - g.mp.steamdotsoldtime > 150 ) 
	{
		g.mp.steamdotsoldtime = Timer();
		g.mp.buildingDots = g.mp.buildingDots + ".";
		if (  Len(g.mp.buildingDots.Get()) > 5  )  g.mp.buildingDots  =  ".";
	}

	t.luaText.txt = g.mp.buildingDots + txt_s + g.mp.buildingDots;
	t.luaText.x = x;
	t.luaText.y = y;
	t.luaText.size = size;
	lua_text ( );
//endfunction

}

void mp_textColor ( int x, int y, int size, char* txt_s, int r, int gg, int b )
{
	g.mp.steamDoColorText = 1;
	t.luaText.txt = txt_s;
	t.luaText.x = x;
	t.luaText.y = y;
	t.luaText.size = size;
	g.mp.steamColorRed = r;
	g.mp.steamColorGreen = gg;
	g.mp.steamColorBlue = b;
	lua_text ( );
//endfunction

}

void mp_panel ( int x, int y, int x2, int y2 )
{
	t.luaPanel.x = x;
	t.luaPanel.y = y;
	t.luaPanel.x2 = x2;
	t.luaPanel.y2 = y2;
	lua_panel ( );
//endfunction

}

void mp_deleteFile ( char* tempFileToDelete_s )
{
	cstr fileToDelete;
	fileToDelete =  cstr(g.fpscrootdir_s + "\\Files\\" + tempFileToDelete_s).Get();
	if (  FileExist(fileToDelete.Get() ) )  DeleteAFile (  fileToDelete.Get() );
//endfunction

}

int mp_check_if_lua_entity_exists ( int tentitytocheck )
{
	int tcheckobj = 0;
	int result = 0;
	result = 0;

	if (  tentitytocheck  <=  g.entityelementlist ) 
	{
		tcheckobj = t.entityelement[tentitytocheck].obj;
		if (  tcheckobj > 0 ) 
		{
			if (  ObjectExist(tcheckobj)  ==  1 ) 
			{
				result = 1;
			}
		}
	}
	return result;
}

void mp_sendlua ( int code, int e, int v )
{
	SteamSendLua ( code, e, v );
}

#endif
