void debugviewtext ( int progress, char* gamedebugviewtext_s )
{
	int thisisaonetimeglobalbeforethisbuildends = 0;
	int gamedebugviewlastmem = 0;
	int gamedebugviewtime = 0;
	int gamedebugviewmem = 0;
	cstr gamedebugview_s =  "";
	float tscrhighdiff_f = 0;
	float tscrwiddiff_f = 0;
	float progress_f = 0;
	int progressh = 0;
	int progressw = 0;
	int progressx = 0;
	int progressy = 0;
	cstr thediff_s =  "";
	cstr thetime_s =  "";
	cstr themem_s =  "";
	int guiUsed = 0;
	cstr stat1_s =  "";
	cstr stat2_s =  "";
	cstr stat3_s =  "";
	int thediff = 0;
	int thetime = 0;
	cstr stat_s =  "";
	int themem = 0;
	int vmsize = 0;
	int stat2 = 0;
	int tokay = 0;
	int stat = 0;
	int st = 0;
	int t1 = 0;
	int t2 = 0;
	int ty = 0;

	//  leave immediately if mapeditor
	if (  g.gmapeditmode == 1  )  return;

	//  FPSCV104RC9 - loading time readout to file
	if (  g.gloadreportstate == 1 ) 
	{
		if (  g.loadreportarraydimmed == 0 ) { Dim (  t.loadreport_s,100000   ) ; g.loadreportarraydimmed = 1; }
		//  get time, distance from last time, description, etc..
		if (  g.gloadreporttime == 0  )  g.gloadreporttime = MAXTimer();
		thetime= MAXTimer()-g.gloadreporttime;
		thetime_s=Right(Str(1000000+thetime),6);
		thediff= MAXTimer()-g.gloadreportlasttime;
		thediff_s=Right(Str(1000000+thediff),6);
		g.gloadreportlasttime= MAXTimer();
		t.loadreport_s[g.gloadreportindex]=thetime_s+" : "+thediff_s+" : "+gamedebugviewtext_s;
		++g.gloadreportindex ; if (  g.gloadreportindex>10000  )  g.gloadreportindex = 10000;
		if (  progress>g.gprogressmax-10 ) 
		{
			//  only save in last 10 counts before end of progress bar
			sprintf ( t.szwork , "%s%s" , g.rootdir_s.Get() , "\\loadreport.txt" );
			SaveArray (  t.szwork , t.loadreport_s );
		}
	}

	//  progress bar based on progress/gprogressmax (some white and black)
	if (  progress != -1 ) 
	{
		//  Memory counters
		g.mshoti=progress;
		//  FPGC - 111209 - more info to indicate potential reason for a sudden crash (too much memory?)
		sprintf ( t.szwork , "[%s] %s  (%sMB used)" , Right(Str(1000+g.mshoti),3) , gamedebugviewtext_s , Str(SMEMAvailable(2)/1024) );
		strcpy ( gamedebugviewtext_s , t.szwork );
		g.mshotmem=GetMemoryAvailable(0);
		if (  g.lastmshotmem == 0 ) { g.lastmshotmem = g.mshotmem  ; g.mshotfirst = GetMemoryAvailable(0); }
		t.mshot[g.mshoti]=g.lastmshotmem-g.mshotmem;
		if (  t.mshot[g.mshoti]>g.mshotmemlargest  )  g.mshotmemlargest = t.mshot[g.mshoti];
		if (  g.mshoti>g.lastmshoti ) 
		{
			//  deposit memory use result to report
			sprintf ( t.szwork , "%s%sK  %s" , t.strarr_s[56].Get() , Str(t.mshot[g.mshoti]) , gamedebugviewtext_s );
			timestampactivity(0, t.szwork);
			g.lastmshoti=g.mshoti;
			g.lastmshotmem=g.mshotmem;
		}
		//  Game GUI Readout
		if (  progress>g.gprogressmax  )  progress = g.gprogressmax;
		progress_f=progress;
		progress_f=progress_f/(g.gprogressmax+0.0001);
		//  FPGC - 121009 - adjust progress# to account for GetDisplayWidth (  different from 1024 )
		tscrwiddiff_f=GetDisplayWidth();
		tscrwiddiff_f=tscrwiddiff_f/1024.0 ; progress_f=progress_f*tscrwiddiff_f;
		tscrhighdiff_f=GetDisplayHeight();
		tscrhighdiff_f=tscrhighdiff_f/800.0;
		if (  guiUsed  ==  0 ) 
		{
			if (  g.internalloaderhud>0 && g.internalloaderhud <= ArrayCount(t.hud) ) 
			{
							//  Image Expand Bar Progress
							t1=t.hud[g.internalloaderhud].width*progress_f;
							SizeSprite (  g.internalloaderhud,t1,t.hud[g.internalloaderhud].height*tscrhighdiff_f );
							PasteSprite (  g.internalloaderhud,t.hud[g.internalloaderhud].posx,t.hud[g.internalloaderhud].posy );
			}
		}
		SyncMask (  0x1  ); 
		Sync (  );
	}

	//  Report On Progress Percentage
	if (  progress != -1 ) 
	{
		if (  progress>g.gprogressmax  )  progress = g.gprogressmax;
		progress_f=progress ; progress_f=progress_f/(g.gprogressmax+0.0001);
		progress_f=progress_f*100.0;
	}

	//  TestGameFromEditor Mode
	if (  g.gtestgamemodefromeditor == 1 ) 
	{
		//  detect if CANCEL early (while building)
		if (  g.gtestgamemodefromeditorokaypressed == 0 ) 
		{
			OpenFileMap (  2, "FPSEXCHANGE" );
			SetEventAndWait (  2 );
			if (  GetFileMapDWORD( 2, 994 )  ==  1 ) 
			{
				//  As can take time, tell user can take time
				OpenFileMap (  3, "FPSTESTGAMEDIALOG" );
				SetFileMapDWORD (  3, 12, 1 );
				SetFileMapString (  3, 1000, t.strarr_s[630].Get() );
				SetEventAndWait (  3 );
				while (  GetFileMapDWORD ( 3, 12 )  ==  1 ) 
				{
					SetEventAndWait (  3 );
				}

				// terminate test game mid-build
				SetFileMapString (  2, 1000, "Guru-MapEditor.exe" );
				SetFileMapString (  2, 1256, "-r" );
				SetFileMapDWORD (  2, 994, 2 );
				SetFileMapDWORD (  2, 924, 1 );
				SetEventAndWait (  2 );

				//  Terminate
				if (  1  )  timestampactivity(0,t.strarr_s[57].Get());
				ExitProcess ( 0 );
			}
			else
			{
				//  Update Test Game Dialog progress and status Text (  )
				OpenFileMap (  1, "FPSTESTGAMEDIALOG" );
				if (  progress != -1 ) 
				{
					SetFileMapDWORD (  1, 8, int(progress_f) );
				}
				//  Display Status Text (  )
				SetFileMapString (  1, 1000, gamedebugviewtext_s );
				SetFileMapDWORD (  1, 12, 1 );
				SetEventAndWait (  1 );
			}
		}
	}
	//  Build Executable Game Mode
	if (  g.gcompilestandaloneexe == 1 ) 
	{
		//  check if build cancelled
		tokay=0;
		OpenFileMap (  2, "FPSEXCHANGE" );
		SetEventAndWait (  2 );
		if (  GetFileMapDWORD( 2, 994 )  ==  1  )  tokay = 1;
		if (  tokay == 1 ) 
		{
			//  terminate build early
			OpenFileMap (  1, "FPSBUILDGAME" );
			SetFileMapDWORD (  1, 108, 1 );
			SetFileMapDWORD (  1, 112, 0 );
			//  close dialog (cannot reload data into it when RELOAD MAPEDITOR)
			SetFileMapDWORD (  1, 24, 1 );
			SetEventAndWait (  1 );
			//  call map editor back
			OpenFileMap (  2, "FPSEXCHANGE" );
			SetFileMapString (  2, 1000, "Guru-MapEditor.exe" );
			SetFileMapString (  2, 1256, "-r" );
			SetFileMapDWORD (  2, 994, 0 );
			SetFileMapDWORD (  2, 924, 1 );
			SetEventAndWait (  2 );
			//  terminate
			if (  1  )  timestampactivity(0,t.strarr_s[58].Get());
			ExitProcess( 0 );
		}
		else
		{
			//  game build progress bar and Text (  )
			OpenFileMap (  1, "FPSBUILDGAME" );
			if (  progress != -1 ) 
			{
				SetFileMapDWORD (  1, 108, 1 );
				SetFileMapDWORD (  1, 112, int(progress_f) );
			}
			SetFileMapDWORD (  1, 40, 12 );
			SetFileMapDWORD (  1, 44, 1 );
			if (  g.level>0 && g.level <= g.glevelmax ) 
			{
				sprintf ( t.szwork , "%s%i\\%i : %s" , t.strarr_s[59].Get(), g.level , g.glevelmax , gamedebugviewtext_s );
				SetFileMapString (  1, 1000, t.szwork );
			}
			else
			{
				SetFileMapString (  1, 1000, gamedebugviewtext_s );
			}
			SetEventAndWait (  1 );
		}
	}

	//  FPGC - 110210 - some systems having issues with builds exceeding 1.5GB, so provide a graceful cap
	//  FPGC - 050510 - turns out virtual memory address fragmentation crashes apps exceeding 2GB
	if (  g.gsystemmemorycapoff == 0 ) 
	{
		vmsize=(SMEMAvailable(2)/1024);
		if (  vmsize>1850 ) 
		{
			if (  thisisaonetimeglobalbeforethisbuildends == 0 ) 
			{
				while ( 1 )
				{
					//  use a global to ensure this function does not call itself (recursively)
					thisisaonetimeglobalbeforethisbuildends=1;
					sprintf ( t.szwork , "Build process has exceed 1.85GB of virtual memory, using %iMB. Press CANCEL to abort this build!" , vmsize );
					debugviewtext(-1, t.szwork );
					SleepNow (  10 );
				}
				ExitProcess ( 0 );
			}
		}
	}
}

void printvalue ( int x, int y, int value )
{
	int tactualtextwidth = 0;
	float tcenterx_f = 0;
	float ttbitx_f = 0;
	float ttbity_f = 0;
	cstr text_s =  "";
	int ttnumy = 0;
	int ttnum = 0;
	int sid = 0;
	int tt = 0;

	//  prepare Sprite (  for Text (  Print ( ing ) ) )
	sid=g.effectmenuimagestart+4;
	MAXSprite (  sid,-10000,-10000,g.effectmenuimagestart+31 );
	SetSpriteDiffuse (  sid,255,255,255 );
	SetSpriteAlpha (  sid,255 );
	SizeSprite (  sid,10,10 );

	//  Print (  ValF ( ue ) )
	text_s = Str(value);
	tactualtextwidth=10;
	tcenterx_f=(Len(text_s.Get())*tactualtextwidth)/2;
	for ( tt = 1 ; tt<=  Len(text_s.Get()); tt++ )
	{
		ttnum=-1;
		if (  cstr(Mid(text_s.Get(),tt)) == "."  )  ttnum = 10 ;
		if (  cstr(Mid(text_s.Get(),tt)) == "\\"  )  ttnum = 11 ;
		if (  ttnum == -1  )  ttnum = Asc(Mid(text_s.Get(),tt))-Asc("0");
		if (  ttnum != -1 ) 
		{
			ttnumy=ttnum/4 ; ttnum=ttnum-(ttnumy*4);
			ttbitx_f=(1.0/64.0)*16 ; ttbity_f=(1.0/64.0)*16;
			SetSpriteTextureCoordinates (  sid,0,(ttbitx_f*ttnum),(ttbity_f*ttnumy) );
			SetSpriteTextureCoordinates (  sid,1,(ttbitx_f*ttnum)+ttbitx_f,(ttbity_f*ttnumy) );
			SetSpriteTextureCoordinates (  sid,2,(ttbitx_f*ttnum),(ttbity_f*ttnumy)+ttbity_f );
			SetSpriteTextureCoordinates (  sid,3,(ttbitx_f*ttnum)+ttbitx_f,(ttbity_f*ttnumy)+ttbity_f );
			PasteSprite (  sid,(x-tcenterx_f)+((tt-1)*tactualtextwidth),y );
		}
	}

	//  restore Sprite (  image )
	MAXSprite (  sid,-10000,-10000,sid );
}

//FUNCTION TO LAUNCH BROWSER

char* browseropen_s ( int browsemode )
{
	int segobjusedforsegeditor = 0;
	cstr baseimagepath_s =  "";
	int browseextcount = 0;
	cstr resultstring_s =  "";
	cstr browsetitle_s =  "";
	cstr filemapname_s =  "";
	cstr baseimage_s =  "";
	cstr extstring_s =  "";
	int browsetype = 0;
	cstr baselib_s =  "";
	cstr tresult_s =  "";
	cstr curdir_s =  "";
	cstr tfile_s =  "";
	cstr tpath_s =  "";
	int tresult = 0;

	//  this way still used by segment editor
	g.localdesc_s="";
	if (  segobjusedforsegeditor == 1 ) 
	{
		int segobjusedforsegeditor = 0;
		cstr baseimagepath_s =  "";
		int browseextcount = 0;
		cstr resultstring_s =  "";
		cstr browsetitle_s =  "";
		cstr filemapname_s =  "";
		cstr baseimage_s =  "";
		cstr extstring_s =  "";
		int browsetype = 0;
		cstr baselib_s =  "";
		cstr tresult_s =  "";
		cstr curdir_s =  "";
		cstr tfile_s =  "";
		cstr tpath_s =  "";
		int tresult = 0;

		//FOR SEGMENT EDITOR

		//  Clear result
		resultstring_s="";

		//  Store directory
		curdir_s=GetDir();

		//  Launch browser in freeze mode
		SetDir ( g.rootdir_s.Get() ); SetDir ("..");

		//  Run if not currently active
		if (  WindowExist(g.browsername_s.Get()) == 0 ) 
		{
			ExecuteFile (  g.browserexe_s.Get(),"","" );
			while (  WindowExist(g.browsername_s.Get()) == 0 ) 
			{
				Sync (  );
			}
		}
		else
		{
			WindowToFront (  g.browsername_s.Get() );
		}

		//  Trigger it to provide correct cateogory for browse
		filemapname_s=g.browsername_s+"(ACTIVE)";
		WriteFilemapValue (  filemapname_s.Get(),1 );
		filemapname_s=g.browsername_s+"(MODE)";
		WriteFilemapValue (  filemapname_s.Get(),browsemode );
		WriteFilemapString (  filemapname_s.Get(),t.strarr_s[42].Get() );

		//  Switch this app to processor friendly
		SyncOff ( ); AlwaysActiveOff ( );

		//  Must wait for response..
		filemapname_s=g.browsername_s+"(ACTIVE)";
		while (  ReadFilemapValue(filemapname_s.Get()) != 2 ) 
		{
		}

		//  This app must wait for..
		tresult=0;
		tresult_s="";
		filemapname_s=g.browsername_s+"(RESULT)";
		while (  tresult == 0 ) 
		{
			if (  WindowExist(g.browsername_s.Get()) == 1 ) 
			{
				WindowToFront (  g.browsername_s.Get() );
				tresult=ReadFilemapValue(filemapname_s.Get());
			}
			else
			{
				break;
			}
		}

		//  Take action based on result
		if (  tresult == 0  )  resultstring_s = "";
		if (  tresult == 1  )  resultstring_s = ReadFilemapString(filemapname_s.Get());
		if (  tresult == 2  )  resultstring_s = "";

		//  Restore primary activity
		WindowToBack (  g.browsername_s.Get() );
		WindowToFront (  );
		SyncOn (   ); AlwaysActiveOn (   ); Sync (  );

		//  set directory to return string (or restore)
		if (  Len(resultstring_s.Get())>0 ) 
		{
			tfile_s=getfile(resultstring_s.Get());
			tpath_s=Left(resultstring_s.Get(),Len(resultstring_s.Get())-Len(tfile_s.Get()));
			SetDir ( g.rootdir_s.Get() ); SetDir ( tpath_s.Get() );
			resultstring_s=GetDir();
			resultstring_s += "\\";
			resultstring_s += tfile_s;
		}
		else
		{
			SetDir (  curdir_s.Get() );
		}

	}
	else
	{
		// FOR MAP EDITOR
		//  Prepare browse type settings
		browsetype=browsemode;
		baseimagepath_s=".\\editors\\gfx\\browser\\";
		if (  browsetype == 1 ) 
		{
			browsetitle_s=t.strarr_s[43];
			baselib_s="";
			baseimage_s="all.bmp";
		}
		if (  browsetype == 2 ) 
		{
			browsetitle_s=t.strarr_s[44];
			baselib_s="texturebank\\";
			baseimage_s="texture.bmp";
		}
		if (  browsetype == 3 ) 
		{
			browsetitle_s=t.strarr_s[45];
			baselib_s="meshbank\\";
			baseimage_s="mesh.bmp";
		}
		if (  browsetype == 4 ) 
		{
			browsetitle_s=t.strarr_s[46];
			baselib_s="audiobank\\";
			baseimage_s="audio.bmp";
		}
		if (  browsetype == 5 ) 
		{
			browsetitle_s=t.strarr_s[47];
			baselib_s="effectbank\\";
			baseimage_s="effect.bmp";
		}
		if (  browsetype == 6 ) 
		{
			browsetitle_s=t.strarr_s[48];
			baselib_s="segments\\";
			baseimage_s="segment.bmp";
		}
		if (  browsetype == 7 ) 
		{
			browsetitle_s=t.strarr_s[49];
			baselib_s="prefabs\\";
			baseimage_s="prefab.bmp";
		}
		if (  browsetype == 8 ) 
		{
			browsetitle_s=t.strarr_s[50];
			baselib_s="mapbank\\";
			baseimage_s="map.bmp";
		}
		if (  browsetype == 9 ) 
		{
			browsetitle_s=t.strarr_s[51];
			baselib_s="entitybank\\";
			baseimage_s="entity.bmp";
		}

		//  Assign filters to browse types
		browseextcount=0;
		Dim (  t.browseext_s,64  );
		if (  browsetype == 1 ) 
		{
			t.browseext_s[browseextcount+1]=".*";
			++browseextcount;
		}
		if (  browsetype == 2 ) 
		{
			t.browseext_s[browseextcount+1]="tga";
			++browseextcount;
		}
		if (  browsetype == 3 ) 
		{
			t.browseext_s[browseextcount+1]="x";
			++browseextcount;
		}
		if (  browsetype == 4 ) 
		{
			t.browseext_s[browseextcount+1]="wav";
			t.browseext_s[browseextcount+2]="mp3";
			browseextcount += 2;
		}
		if (  browsetype == 5 ) 
		{
			t.browseext_s[browseextcount+1]="fx";
			++browseextcount;
		}
		if (  browsetype == 6 ) 
		{
			t.browseext_s[browseextcount+1]="fps";
			++browseextcount;
		}
		if (  browsetype == 7 ) 
		{
			t.browseext_s[browseextcount+1]="fpp";
			++browseextcount;
		}
		if (  browsetype == 8 ) 
		{
			t.browseext_s[browseextcount+1]="fpm";
			++browseextcount;
		}
		if (  browsetype == 9 ) 
		{
			t.browseext_s[browseextcount+1]="fpe";
			++browseextcount;
		}

		//  Build extension string (ie .wav,.mp3)
		extstring_s="";
		for ( t.t = 1 ; t.t <= browseextcount; t.t++ )
		{
			if (  t.t>1  )  extstring_s = extstring_s+",";
			extstring_s=extstring_s+t.browseext_s[t.t];
		}

		//  Call up browser dialog
		sprintf ( t.szwork , "%s\\%s" , g.rootdir_s.Get() , baselib_s.Get() );
		SetFileMapString (  1, 1000, cstr(g.rootdir_s+cstr("\\")+baselib_s).Get() );
		SetFileMapString (  1, 1256, browsetitle_s.Get() );

		//  File Filter
		SetFileMapString (  1, 1768,extstring_s.Get() );

		//  Default image if no thumbnail found
		sprintf ( t.szwork , "%s\\%s%s" , g.rootdir_s.Get() , baseimagepath_s.Get() , baseimage_s.Get() );
		SetFileMapString (  1, 2024, t.szwork );

		//  Window Title
		SetFileMapString (  1, 2280, browsetitle_s.Get() );

		//  Set last location for navigation
		if (  t.browserfolderhistory_s[browsetype] != "" ) 
		{
			SetFileMapString (  1, 2536, t.browserfolderhistory_s[browsetype].Get() );
		}

		//  Wait for dialog session to end
		SetFileMapDWORD (  1, 800, 1 );
		SetEventAndWait (  1 );
		while (  GetFileMapDWORD(1,800) == 1 ) 
		{
			SetEventAndWait (  1 );
		}

		g.localdesc_s=GetFileMapString( 1, 1256 );

		//  return string from browser dialog
		SetDir (  g.rootdir_s.Get() );
		resultstring_s=GetFileMapString( 1, 1512 );
		if (  resultstring_s != "" ) 
		{
			//  Final return string
			SetDir (  g.rootdir_s.Get() );
			//  Store location as we leave browser
			resultstring_s=Right(resultstring_s.Get(),Len(resultstring_s.Get())-Len(g.rootdir_s.Get()));
			t.browserfolderhistory_s[browsetype]=Right(resultstring_s.Get(),Len(resultstring_s.Get())-Len(baselib_s.Get()));
			sprintf ( t.szwork ,"%s\\%s" ,  GetDir() , resultstring_s.Get() );
			resultstring_s=t.szwork;

		}
		else
		{
			//  No return string
		}
		strcpy ( t.szreturn , resultstring_s.Get() );
		return (LPSTR)t.szreturn;
	}
	return NULL;
} 

void loadscreenpromptassets ( int iUseVRTest )
{
	if ( t.levelsforstandalone == 0 )
	{
		int tclosestresheight = 0;
		int tclosestreswidth = 0;
		int tresheight = 0;
		cstr respart_s =  "";
		int treswidth = 0;
		int tclosest = 0;
		cstr tfile_s =  "";
		int tdiff = 0;
		int tres = 0;
		if (  ImageExist(g.testgamesplashimage) == 1  )  DeleteImage (  g.testgamesplashimage );
		if (  ImageExist(g.testgamesplashimage) == 0 ) 
		{
			//  determine the resolution we should use
			tclosest=9999999;
			tclosestreswidth=0 ; tclosestresheight=0;
			for ( tres = 1 ; tres<=  12; tres++ )
			{
				if (  tres == 1 ) { treswidth = 1024  ; tresheight = 768;}
				if (  tres == 2 ) { treswidth = 1152  ; tresheight = 864;}
				if (  tres == 3 ) { treswidth = 1280  ; tresheight = 720;}
				if (  tres == 4 ) { treswidth = 1280  ; tresheight = 800;}
				if (  tres == 5 ) { treswidth = 1280  ; tresheight = 960;}
				if (  tres == 6 ) { treswidth = 1366  ; tresheight = 768;}
				if (  tres == 7 ) { treswidth = 1440  ; tresheight = 900;}
				if (  tres == 8 ) { treswidth = 1600  ; tresheight = 900;}
				if (  tres == 9 ) { treswidth = 1600  ; tresheight = 1200;}
				if (  tres == 10 ) { treswidth = 1680  ; tresheight = 1050;}
				if (  tres == 11 ) { treswidth = 1920  ; tresheight = 1080;}
				if (  tres == 12 ) { treswidth = 1920  ; tresheight = 1200;}
				tdiff=abs(GetDisplayWidth()-treswidth)+abs(GetDisplayHeight()-tresheight);
				if ( tdiff<tclosest ) 
				{
					// 050917 - check if this file exists for consideration
					if ( t.game.gameisexe == 1 ) 
					{
						sprintf ( t.szwork , "languagebank\\%s\\artwork\\watermark\\watermark-%ix%i.jpg", g.language_s.Get(), treswidth, tresheight );			
						if ( FileExist ( t.szwork ) == 1 )
						{
							tclosest=tdiff;
							tclosestreswidth=treswidth;
							tclosestresheight=tresheight;
						}
					}
					else
					{
						tclosest=tdiff;
						tclosestreswidth=treswidth;
						tclosestresheight=tresheight;
					}
				}
			}
			if ( t.game.gameisexe == 0 ) 
			{
				// used for test game splash screen
				sprintf ( t.szwork , "%ix%i" , tclosestreswidth , tclosestresheight );
				respart_s = t.szwork;
			}
			else
			{
				if ( tclosest != 9999999 )
				{
					// use closest to current resolution
					 sprintf ( t.szwork , "watermark-%ix%i.jpg" , tclosestreswidth , tclosestresheight );
					respart_s = t.szwork;
				}
				else
				{
					// could not find any matching resolution files, just pick any file in the watermark folder
					cstr pOldDir = GetDir();
					sprintf ( t.szwork , "languagebank\\%s\\artwork\\watermark", g.language_s.Get() );
					SetDir(t.szwork);
					ChecklistForFiles (  );
					for ( int c = 1 ; c<=  ChecklistQuantity(); c++ )
					{
						if (  ChecklistValueA(c) == 0 ) 
						{
							tfile_s = ChecklistString(c);
							if (  tfile_s != "." && tfile_s != ".." ) 
							{
								respart_s = tfile_s;
								break;
							}
						}
					}
					SetDir(pOldDir.Get());
				}
			}
			if ( t.game.gameisexe == 1 ) 
			{
				if ( g.iStandaloneIsReloading==0 )
				{
					// show splash initially
					tfile_s = respart_s;
					sprintf ( t.szwork, "languagebank\\%s\\artwork\\watermark\\%s", g.language_s.Get(), tfile_s.Get() );
					SetMipmapNum(1);
					LoadImage ( t.szwork, g.testgamesplashimage );
					SetMipmapNum(-1);
				}
				else
				{
					// when replay game, we actually reload the whole EXE (and dont show the init prompt and splash again)
					if ( ImageExist ( g.testgamesplashimage )==1 )
						DeleteImage ( g.testgamesplashimage );
				}
			}
			else
			{
				// do not show the key/vr instructions during test level - it is too quick (hardly shows)
			}
		}
	}
}

DWORD g_SensibleMessageTimer = 0;

void printscreenprompt ( char* screenprompt_s )
{
	// do not do any progress bar rendering if in standalone/play game mode (done by loading page)
	if (t.game.gameisexe == 1) return;
	// allow early progress bar to creep each time this called sao can see progress!
	extern int g_iLastProgressPercentage;
	if (g_iLastProgressPercentage < 20) g_iLastProgressPercentage++;
	if ( t.levelsforstandalone == 0 )
	{
		if (strlen(screenprompt_s) == 0)
		{
			g_SensibleMessageTimer = 0;
		}
		else
		{
			// do not show any prompts in first 2 seconds (if test level instant)
			if (g_SensibleMessageTimer == 0) g_SensibleMessageTimer = MAXTimer() + 500;
			if (MAXTimer() > g_SensibleMessageTimer + 500)
			{
				g_SensibleMessageTimer = MAXTimer();
				// only for developer mode users
				extern int g_iDevToolsOpen;
				if ((bool)g_iDevToolsOpen == true)
				{
					// more detailed information
					pastebitmapfont(screenprompt_s, (GetChildWindowWidth() / 2) - (getbitmapfontwidth (screenprompt_s, 4) / 2), 64, 4, 255);
				}
				else
				{
					// simple progress bar instead
					//float fProgress = (float)g_iLastProgressPercentage / 100.0f; ignore last 75 percent to exxagerate first half for EA
					float fProgress = (float)g_iLastProgressPercentage / 25.0f;
					if (fProgress < 0.0f) fProgress = 0.0f;
					if (fProgress > 1.0f) fProgress = 1.0f;
					if (ImageExist(g.editorimagesoffset + 14) == 1)
					{
						MAXSprite(124, -10000, -10000, g.editorimagesoffset + 14);
						SizeSprite(124, 500, 2);
						OffsetSprite(124, 0, 0);
						PasteSprite(124, (GetChildWindowWidth() / 2) - 250, (GetChildWindowHeight() / 2) - 32);
						PasteSprite(124, (GetChildWindowWidth() / 2) - 250, (GetChildWindowHeight() / 2) + 32);
						SizeSprite(124, 2, 64);
						OffsetSprite(124, 0, 0);
						PasteSprite(124, (GetChildWindowWidth() / 2) - 250, (GetChildWindowHeight() / 2) - 32);
						PasteSprite(124, (GetChildWindowWidth() / 2) + 250, (GetChildWindowHeight() / 2) - 32);
						SizeSprite(124, (int)(500.0f*fProgress), 64);
						OffsetSprite(124, 0, 0);
						PasteSprite(124, (GetChildWindowWidth() / 2) - 250, (GetChildWindowHeight() / 2) - 32);
					}
				}
				void StartForceRender(void);
				StartForceRender();
			}
		}
	}

	// steam refresh to keep it live
	mp_refresh ( );

	//  quickly check contiguous memory (as this gets called every time prompt printed)
	checkmemoryforgracefulexit();
}

int mod ( int num, int modulus )
{
	int value = 0;
	value = num-((num/modulus)*modulus);
	return value;
}

void GGBoxGradient ( int iLeft, int iTop, int iRight, int iBottom, DWORD dw1, DWORD dw2, DWORD dw3, DWORD dw4 )
{
	return; //PE: No sliders in Max.
	int iWidth = iRight - iLeft;
	int iHeight = iBottom - iTop;
	MAXSprite ( 1235, -100000, -100000, g.slidersmenuimageoffset + 8 );
	SetSprite ( 1235, 0, 1 );
	SizeSprite ( 1235, iWidth, iHeight );
	SetSpriteDiffuse ( 1235, dw2, dw3, dw4 );
	SetSpriteAlpha ( 1235, dw1 );
	PasteSprite ( 1235, iLeft, iTop );
}

// copied from Common.cpp (above)
int geditorimagesoffset = 65110;

void InkEx ( int r, int g, int b )
{
	if ( ImageExist ( geditorimagesoffset+14 ) == 0 ) LoadImage (  "editors\\gfx\\14.png", geditorimagesoffset+14 );
	MAXSprite ( 123, -10000, -10000, geditorimagesoffset+14 );
	SetSpriteDiffuse ( 123, r, g, b );
}
void BoxEx ( int x1, int y1, int x2, int y2 )
{
	if ( x2 < x1 ) { int St = x1; x1 = x2; x2 = St; }
	if ( y2 < y1 ) { int St = y1; y1 = y2; y2 = St; }
	SizeSprite ( 123, x2-x1, y2-y1 );
	PasteSprite ( 123, x1, y1 );
}
void LineEx ( int x1, int y1, int x2, int y2 )
{
	if ( x2 < x1 ) { int St = x1; x1 = x2; x2 = St; }
	if ( y2 < y1 ) { int St = y1; y1 = y2; y2 = St; }
	int width = x2-x1;
	int height = y2-y1;
	if ( width > height )
		SizeSprite ( 123, width, 1+(y2-y1) );
	else
		SizeSprite ( 123, 1+(x2-x1), height );
	PasteSprite ( 123, x1, y1 );
}

void GetSetupIniEarly( void )
{
	// GGMAX 1.79: the low-VRAM preset's LAZY OBJECT PSO half has to be decided before the engine
	// builds its object pipelines in wi::renderer::LoadShaders, and that happens long before
	// FPSC_LoadSETUPINI runs. Handling the key only there did nothing at all — measured, not
	// assumed: with `lowvram=1` set, pso_creates stayed at 7496, identical to the control. So the
	// key is read here too, in main()'s early pass, alongside the standalone keys below.
	// Deliberately simple parsing: no engine services exist yet, and note `lowvramgrassdist` must
	// NOT satisfy `lowvram`, so the key is matched exactly rather than by substring (which is why
	// this does not use the pestrcasestr style of the block below).
	// The grass-cap half has no such constraint — grass entities are built per chunk at spawn
	// time, so FPSC_LoadSETUPINI is early enough for it.
	//
	// GGMAX 1.82: `lazypso` joins it here for the same reason. Lazy object PSOs are now DEFAULT
	// ON for everyone (−633 MB measured, POLYS bit-identical), so this key exists as the revert
	// switch — `lazypso=0` restores eager pipeline creation. It has to be read in this same early
	// pass or it would be equally inert.
	{
		FILE* lvf = nullptr;
		if (fopen_s(&lvf, "setup.ini", "r") == 0 && lvf != nullptr)
		{
			char lvline[2048];
			while (fgets(lvline, sizeof(lvline), lvf))
			{
				const char* p = lvline;
				while (*p == ' ' || *p == '\t') p++;

				// Both keys are 7 characters; the '=' check is what makes the match exact, so
				// "lowvramgrassdist=..." cannot satisfy "lowvram".
				const bool bLowVram = (_strnicmp(p, "lowvram", 7) == 0);
				const bool bLazyPso = (_strnicmp(p, "lazypso", 7) == 0);
				if (!bLowVram && !bLazyPso) continue;
				const char* q = p + 7;
				while (*q == ' ' || *q == '\t') q++;
				if (*q != '=') continue;                 // "lowvramgrassdist=..." lands here
				const int iValue = atoi(q + 1);

				if (bLowVram && iValue != 0)
				{
					extern void GGSetLowVRAM(int);
					GGSetLowVRAM(1);
				}
				if (bLazyPso)
				{
					extern void GGSetLazyPSO(int);
					GGSetLazyPSO(iValue);
				}
			}
			fclose(lvf);
		}
	}

	//PE: Standalone option to make sure no mem is used by terrain system.
	char appname[1024];
	GetModuleFileNameA(g_pGlob->hInstance, appname, 1024);
	if(!pestrcasestr(appname,"gamegurumax.exe"))
	{
		//PE: Cant use any special commands at this point. so simple parsing only what we need at this point.
		FILE* file = fopen("setup.ini", "r");
		if (file)
		{
			char t[2048];
			while (!feof(file))
			{
				fgets(t, 2047, file);
				if (pestrcasestr(t, "disableterrainsystem"))
				{
					if (pestrcasestr(t, "1"))
					{
						extern int g_iDisableTerrainSystem;
						g_iDisableTerrainSystem = 1;
					}
				}
				if (pestrcasestr(t, "disablewparticlesystem"))
				{
					if (pestrcasestr(t, "1"))
					{
						extern int g_iDisableWParticleSystem;
						g_iDisableWParticleSystem = 1;
					}
				}				
			}
			fclose(file);
		}
	}
}
