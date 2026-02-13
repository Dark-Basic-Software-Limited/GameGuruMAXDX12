void importer_delete_collision_box ( void )
{
	if (bRemoveSprites)
		return;
	if (  t.importer.collisionShapeCount > 0 && t.importer.selectedCollisionObject >= 0)
	{
		DeleteObject (  t.importerCollision[t.importer.selectedCollisionObject].object );
		t.importerCollision[t.importer.selectedCollisionObject].object = 0;
		DeleteObject (  t.importerCollision[t.importer.selectedCollisionObject].object2 );
		t.importerCollision[t.importer.selectedCollisionObject].object2 = 0;

		for ( int tCount = t.importer.selectedCollisionObject ; tCount<=  t.importer.collisionShapeCount-1; tCount++ )
		{
			t.importerCollision[tCount].object = t.importerCollision[tCount+1].object;
			t.importerCollision[tCount].object2 = t.importerCollision[tCount+1].object2;
			t.importerCollision[tCount].sizex = t.importerCollision[tCount+1].sizex;
			t.importerCollision[tCount].sizey = t.importerCollision[tCount+1].sizey;
			t.importerCollision[tCount].sizez = t.importerCollision[tCount+1].sizez;
			t.importerCollision[tCount].rotx = t.importerCollision[tCount+1].rotx;
			t.importerCollision[tCount].roty = t.importerCollision[tCount+1].roty;
			t.importerCollision[tCount].rotz = t.importerCollision[tCount+1].rotz;
		}

		--t.importer.collisionShapeCount;
		t.importer.selectedCollisionObject = t.importer.collisionShapeCount-1;
		if (  t.importer.selectedCollisionObject  >=  0 ) 
		{
			t.slidersmenuvalue[t.importer.properties2Index][7].value = t.importerCollision[t.importer.selectedCollisionObject].rotx;
			t.slidersmenuvalue[t.importer.properties2Index][8].value = t.importerCollision[t.importer.selectedCollisionObject].roty;
			t.slidersmenuvalue[t.importer.properties2Index][9].value = t.importerCollision[t.importer.selectedCollisionObject].rotz;
		}
		if (  t.importer.collisionShapeCount > 0 ) 
		{
			if (  t.importerCollision[t.importer.selectedCollisionObject].object > 0 && t.importerCollision[t.importer.selectedCollisionObject].object2 > 0 ) 
			{
				if (  ObjectExist(t.importerCollision[t.importer.selectedCollisionObject].object)  ==  1 && ObjectExist(t.importerCollision[t.importer.selectedCollisionObject].object2)  ==  1 ) 
				{
					RotateObject (  t.importerCollision[t.importer.selectedCollisionObject].object, t.slidersmenuvalue[t.importer.properties2Index][7].value, t.slidersmenuvalue[t.importer.properties2Index][8].value, t.slidersmenuvalue[t.importer.properties2Index][9].value );
					RotateObject (  t.importerCollision[t.importer.selectedCollisionObject].object2, t.slidersmenuvalue[t.importer.properties2Index][7].value, t.slidersmenuvalue[t.importer.properties2Index][8].value, t.slidersmenuvalue[t.importer.properties2Index][9].value );
					t.importerCollision[t.importer.selectedCollisionObject].rotx = t.slidersmenuvalue[t.importer.properties2Index][7].value;
					t.importerCollision[t.importer.selectedCollisionObject].roty = t.slidersmenuvalue[t.importer.properties2Index][8].value;
					t.importerCollision[t.importer.selectedCollisionObject].rotz = t.slidersmenuvalue[t.importer.properties2Index][9].value;
				}
			}
		}
	}
}

void importer_checkForShaderFiles ( void )
{
	t.tOriginalPath_s = t.importer.startDir;
	if (  strcmp ( Right(t.tOriginalPath_s.Get(),1) , "\\" ) != 0  )  t.tOriginalPath_s  =  t.tOriginalPath_s + "\\";
	SetDir (  cstr(t.tOriginalPath_s + "effectbank\\reloaded").Get() );
	t.importer.shaderFileCount = 0;
	FindFirst (  );
	do
	{
		t.ts_s = GetFileName();
		t.ts_s = Lower(t.ts_s.Get());

		//  Remove the shaders that are not for imported entities
		if ( t.ts_s == "apbr_core.fx" ) t.ts_s = "";
		if ( t.ts_s == "apbr_terrain.fx" ) t.ts_s = "";
		if ( t.ts_s == "cascadeshadows.fx" ) t.ts_s = "";
		if ( t.ts_s == "character_editor.fx" ) t.ts_s = "";
		if ( t.ts_s == "character_static.fx" ) t.ts_s = "";
		if ( t.ts_s == "constantbuffers.fx" ) t.ts_s = "";
		if ( t.ts_s == "dynamicterrainshadow_basic.fx" ) t.ts_s = "";
		if ( t.ts_s == "ebe_basic.fx" ) t.ts_s = "";
		if ( t.ts_s == "entity_core.fx" ) t.ts_s = "";
		if ( t.ts_s == "gui_basic.fx" ) t.ts_s = "";
		if ( t.ts_s == "gui_diffuse.fx" ) t.ts_s = "";
		if ( t.ts_s == "gui_showdepth.fx" ) t.ts_s = "";
		if ( t.ts_s == "gui_wireframe.fx" ) t.ts_s = "";
		if ( t.ts_s == "post-bloom.fx" ) t.ts_s = "";
		if ( t.ts_s == "post-core.fx" ) t.ts_s = "";
		if ( t.ts_s == "post-none.fx" ) t.ts_s = "";
		if ( t.ts_s == "post-rift.fx" ) t.ts_s = "";
		if ( t.ts_s == "post-sao.fx" ) t.ts_s = "";
		if ( t.ts_s == "scatter.fx" ) t.ts_s = "";
		if ( t.ts_s == "settings.fx" ) t.ts_s = "";
		if ( t.ts_s == "shadow_basic.fx" ) t.ts_s = "";
		if ( t.ts_s == "sky_basic.fx" ) t.ts_s = "";
		if ( t.ts_s == "sky_core.fx" ) t.ts_s = "";
		if ( t.ts_s == "sky_scroll.fx" ) t.ts_s = "";
		if ( t.ts_s == "skyscroll_basic.fx" ) t.ts_s = "";
		if ( t.ts_s == "sprite_basic.fx" ) t.ts_s = "";
		if ( t.ts_s == "static_basic.fx" ) t.ts_s = "";
		if ( t.ts_s == "terrain_basic.fx" ) t.ts_s = "";
		if ( t.ts_s == "vegetation_basic.fx" ) t.ts_s = "";
		if ( t.ts_s == "water_basic.fx" ) t.ts_s = "";
		if ( t.ts_s == "weapon_basic.fx" ) t.ts_s = "";
		if ( t.ts_s == "weapon_bone.fx" ) t.ts_s = "";
		if ( strcmp ( Right(t.ts_s.Get(),3) , ".fx" ) == 0 ) 
		{
			if ( t.importer.shaderFileCount < IMPORTERSHADERFILESMAX ) 
			{
				t.importerShaderFiles[t.importer.shaderFileCount+1] = t.ts_s;
				++t.importer.shaderFileCount;
			}
		}
		FindNext (  );
	} while ( !(  GetFileType() == -1 ) );
	SetDir (  t.importer.startDir.Get() );
}

void importer_checkForScriptFiles ( void )
{
	t.tOriginalPath_s = t.importer.startDir;
	if (  cstr(Right(t.tOriginalPath_s.Get(),1))  !=  "\\"  )  t.tOriginalPath_s  =  t.tOriginalPath_s + "\\";
	SetDir (  cstr(t.tOriginalPath_s + "scriptbank").Get() );

	t.importer.scriptFileCount = 0;

	FindFirst (  );
	do
	{

		t.ts_s = GetFileName();
		t.ts_s = Lower(t.ts_s.Get());

		if (  cstr(Right(t.ts_s.Get(),4))  ==  ".lua" ) 
		{
			if (  t.importer.scriptFileCount < IMPORTERSCRIPTFILESMAX ) 
			{
				t.importerScriptFiles[t.importer.scriptFileCount+1] = t.ts_s;
				++t.importer.scriptFileCount;
			}
		}
		FindNext (  );
	} while ( !(  GetFileType() == -1 ) );

	SetDir (  t.importer.startDir.Get() );
}

void importer_help ( void )
{
	if (bRemoveSprites)
		return;
	if (  t.inputsys.mclick  ==  1 && t.importer.oldMouseClick  ==  0 ) 
	{
		if (  t.importer.MouseX  >=  SpriteX(t.importer.helpSprite) && t.importer.MouseX  <=  SpriteX(t.importer.helpSprite) + 32 && t.importer.MouseY >=  SpriteY(t.importer.helpSprite) && t.importer.MouseY  <=  SpriteY(t.importer.helpSprite) + 32 ) 
		{
			t.importer.helpShow = 1 - t.importer.helpShow;
			t.importer.oldMouseClick = 1;
			if (  t.importer.helpShow  ==  1  )  t.importer.helpFade  =  0;
		}
		else
		{
			t.importer.helpShow = 0;
		}
	}

	if (  t.importer.helpShow  ==  0 ) 
	{
		SetSpriteDiffuse (  t.importer.helpSprite , 255,255,255 );
		if (  SpriteExist(t.importer.helpSprite4) ) 
		{
			SetSpriteAlpha (  t.importer.helpSprite4, t.importer.helpFade );
			if (  t.importer.helpFade > 0  )  t.importer.helpFade  =  t.importer.helpFade - 20;
			if (  t.importer.helpFade  <=  0 ) 
			{
				t.importer.helpFade = 0;
				DeleteSprite (  t.importer.helpSprite4 );
				if (  ImageExist(g.importermenuimageoffset+9)  )  DeleteImage (  g.importermenuimageoffset+9 );
			}
			else
			{
				MAXSprite (  t.importer.helpSprite4 , (GetChildWindowWidth()/2) - 303 , (GetChildWindowHeight()/2) - 213, g.importermenuimageoffset+9 );
			}
		}
		return;
	}

	t.importer.message ="Left click to hide help";

	SetSpriteDiffuse (  t.importer.helpSprite , 100,100,100 );

	t.tx = 40;
	t.ty = 64;

	if (  ImageExist(g.importermenuimageoffset+9)  ==  0 ) 
	{
		CreateBitmap (  32,605,435 );
		SetCurrentBitmap (  32 );

		CLS (  Rgb(28,34,39) );

		pastebitmapfont("Importer Help",(605/2) - getbitmapfontwidth("Importer Help",1)/2,20,1,255);

		t.tT_s = "Properties Shortcut Keys";
		pastebitmapfont(t.tT_s.Get(),t.tx,t.ty,1,255);

		t.ty += 32;
		t.tT_s = "When the Properties tab is selected, clicking and dragging on the object will allow you to rotate it.";
		pastebitmapfont(t.tT_s.Get(),t.tx,t.ty,2,255);
		t.ty += 20;
		t.tT_s  =  "Holding shift while dragging the object will limit the rotation to 45 degree steps" ;
		pastebitmapfont(t.tT_s.Get(),t.tx,t.ty,2,255);

		t.ty += 40;
		t.tT_s = "Collision Shortcut Keys";
		pastebitmapfont(t.tT_s.Get(),t.tx,t.ty,1,255);

		t.ty += 32;
		t.tT_s = "When the Collsion tab is selected, You can change the view by using the cursor keys:";
		pastebitmapfont(t.tT_s.Get(),t.tx,t.ty,2,255);

		t.ty += 20;
		t.tT_s = "UP: Top view, LEFT: Left view, RIGHT: Right View, DOWN: Front view";
		pastebitmapfont(t.tT_s.Get(),t.tx,t.ty,2,255);

		t.ty += 32;
		t.tT_s = "When resizing t.a collision Box ( , you can limit t.movement to an axis ):";
		pastebitmapfont(t.tT_s.Get(),t.tx,t.ty,2,255);

		t.ty += 20;
		t.tT_s = "X: Limit to X axis, C: Limit to Y axis, V: Limit to Z axis";
		pastebitmapfont(t.tT_s.Get(),t.tx,t.ty,2,255);

		t.ty += 20;
		t.tT_s = "You can limit more than one axis at t.a time by holding multiple keys down";
		pastebitmapfont(t.tT_s.Get(),t.tx,t.ty,2,255);

		t.ty += 20;
		t.tT_s  =  "Holding shift while dragging collision boxes will snap them to other boxes" ; 
		pastebitmapfont(t.tT_s.Get(),t.tx,t.ty,2,255);

		t.ty += 40;
		t.tT_s = "Thumbnail Shortcut Keys";
		pastebitmapfont(t.tT_s.Get(),t.tx,t.ty,1,255);

		t.ty += 32;
		t.tT_s = "When the Thumbnail tab is selected, clicking and dragging on the object will allow you to rotate it.";
		pastebitmapfont(t.tT_s.Get(),t.tx,t.ty,2,255);
		t.ty += 20;
		t.tT_s  =  "Holding shift while dragging the object will limit the rotation to 45 degree steps" ; 
		pastebitmapfont(t.tT_s.Get(),t.tx,t.ty,2,255);

		GrabImage (  g.importermenuimageoffset+9,0,0,605,435,3 );
		SetCurrentBitmap (  0 );
		DeleteBitmapEx (  32 );
	}
	MAXSprite (  t.importer.helpSprite4 , (GetChildWindowWidth()/2) - 303 , (GetChildWindowHeight()/2) - 213, g.importermenuimageoffset+9 );
	SetSpritePriority (  t.importer.helpSprite4,2 );
	SetSpriteAlpha (  t.importer.helpSprite4, t.importer.helpFade );
	if (  t.importer.helpFade < 255  )  t.importer.helpFade  =  t.importer.helpFade + 20;
	if (  t.importer.helpFade > 255  )  t.importer.helpFade  =  255;
}

void importer_screenSwitch ( void )
{ 
	if (bRemoveSprites)
		return;

	if (  t.importer.scaleMulti > 1.0 ) 
	{
		MAXSprite (  t.importer.helpSprite , (GetChildWindowWidth()/2) - 170 + 128 , 0, g.importermenuimageoffset+8 );
	}
	else
	{
		MAXSprite (  t.importer.helpSprite , (GetChildWindowWidth()/2) - 170 + 128 , (GetChildWindowHeight()/2) - 300 - 4, g.importermenuimageoffset+8 );
	}

	if (  t.importer.scaleMulti > 1.0 ) 
	{
		t.importerTabs[1].y = 0;
		t.importerTabs[2].y = 0;
		t.importerTabs[3].y = 0;
		t.importerTabs[4].y = 0;
		t.importerTabs[5].y = 0;
		t.importerTabs[11].y = 0;
		t.slidersmenu[t.importer.properties1Index].tleft= GetChildWindowWidth() - 255;
		t.slidersmenu[t.importer.properties2Index].tleft= GetChildWindowWidth() - 255;
		t.slidersmenu[t.importer.properties3Index].tleft= GetChildWindowWidth() - 255;
		t.slidersmenu[t.importer.properties4Index].tleft= GetChildWindowWidth() - 255;

		//  New button
		t.importerTabs[6].x = GetChildWindowWidth() - 159;
		t.importerTabs[7].x = GetChildWindowWidth() - 159;
		t.importerTabs[8].x = GetChildWindowWidth() - 159;
		t.importerTabs[9].x = GetChildWindowWidth() - 159;
		t.importerTabs[10].x = GetChildWindowWidth() - 159;
		t.importerTabs[12].x = GetChildWindowWidth() - 159;

	}
	else
	{
		t.slidersmenu[t.importer.properties1Index].tleft= (GetChildWindowWidth() / 2 ) + 330;
		t.slidersmenu[t.importer.properties2Index].tleft= (GetChildWindowWidth() / 2 ) + 330;
		t.slidersmenu[t.importer.properties3Index].tleft= (GetChildWindowWidth() / 2 ) + 330;
		t.slidersmenu[t.importer.properties4Index].tleft= (GetChildWindowWidth() / 2 ) + 330;
	
		t.importerTabs[1].x = (GetChildWindowWidth() / 2) + 65-128 + 250;
		t.importerTabs[1].y = (GetChildWindowHeight() / 2) - 304;

		t.importerTabs[2].x = t.importerTabs[1].x + 128;
		t.importerTabs[2].y = t.importerTabs[1].y;

		t.importerTabs[3].x = t.importerTabs[2].x + 128;
		t.importerTabs[3].y = t.importerTabs[1].y;

		t.importerTabs[4].x = t.importerTabs[3].x + 256;
		t.importerTabs[4].y = t.importerTabs[1].y;

		//  Importer button
		t.importerTabs[5].x = (GetChildWindowWidth() / 2) - 320;
		t.importerTabs[5].y = (GetChildWindowHeight() / 2) - 304;

		//  New button
		t.importerTabs[6].x = t.importerTabs[4].x - 28 + 10 - 256 + 1;
		t.importerTabs[6].y = t.importerTabs[1].y + 125;

		//  New button
		t.importerTabs[12].x = t.importerTabs[4].x - 28 + 10 - 256 + 1;
		t.importerTabs[12].y = t.importerTabs[1].y + 125 + 38;

		//  Delete button
		t.importerTabs[7].x = t.importerTabs[4].x - 28 + 10 - 256 + 1;
		t.importerTabs[7].y = t.importerTabs[1].y + 125 + (38*2);

		//  Next button
		t.importerTabs[8].x = t.importerTabs[4].x - 28 + 10 - 256 + 1;
		t.importerTabs[8].y = t.importerTabs[1].y + 125 + (38*3);

		//  Previous button
		t.importerTabs[9].x = t.importerTabs[4].x - 28 + 10 - 256 + 1;
		t.importerTabs[9].y = t.importerTabs[1].y + 125 + (38*4);

		//  Show Guide button
		t.importerTabs[10].x = t.importerTabs[4].x - 28 + 10 - 256 + 1;
		t.importerTabs[10].y = t.importerTabs[1].y + 125 + (38*10) - 5;

		//  Importer button
		t.importerTabs[11].x = (GetChildWindowWidth() / 2) - 320 +128;
		t.importerTabs[11].y = (GetChildWindowHeight() / 2) - 304;
	}
}

// 
//  File dialog functions
// 

int findFreeDll ( void )
{
	int tFoundFree = 0;
	int tCount = 0;

	tFoundFree = 0;

	for ( tCount = 1 ; tCount<=  50; tCount++ )
	{
		if (  DLLExist ( tCount )  ==  0 ) 
		{
			tFoundFree = tCount;
			break;
		}
	}
	return tFoundFree;
}

int findFreeMemblock ( void )
{
	int tFoundFree = 0;
	int tCount = 0;

	tFoundFree = 0;

	for ( tCount = 1 ; tCount<=  50; tCount++ )
	{
		if (  MemblockExist (  tCount )  ==  0 ) 
		{
			tFoundFree = tCount;
			break;
		}
	}
	return tFoundFree;
}

char* openFileBox(char* filter_, char* initdir_, char* dtitle_, char* defext_, unsigned char open)
{
	cStr tOldDir = GetDir();
	char* cFileSelected = NULL;
	cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, filter_, initdir_, NULL);
	SetDir(tOldDir.Get());
	if (cFileSelected && strlen(cFileSelected) > 0)
		return cFileSelected;
	else
		return "";
}

char* importerPadString ( char* tString )
{
	t.returnstring_s = tString;

	while (  Len(t.returnstring_s.Get()) < 17 ) 
	{
		t.returnstring_s += " ";
	}

	return t.returnstring_s.Get();
}

int findFreeObject ( void )
{
	int found = 0;

	found = g.importermenuobjectoffset;
	while (  ObjectExist ( found )  ==  1 )
	{
		++found;
	}
	return found;
}

float GetDistance ( float x1, float  y1, float  z1, float  x2, float  y2, float  z2 )
{
	float result_f = 0;
	float tXd_f = 0;
	float tYd_f = 0;
	float yZd_f = 0;

	tXd_f = x2 - x1;
	tYd_f = y2 - y1;
	yZd_f = z2 - z1;

	result_f = Sqrt ( tXd_f * tXd_f + tYd_f * tYd_f + yZd_f * yZd_f );

	return result_f;
}

int importer_check_if_protected ( char* timporterfilecheck_s )
{
	int timporterprotectedcheck = 0;
	cstr tStippedFileName_s =  "";
	cstr tStippedString_s =  "";
	int tArrayMarker = 0;
	cstr tempLine_s =  "";
	cstr tstring_s =  "";
	cstr tToken2_s =  "";
	cstr tToken_s =  "";
	int tCount = 0;

	timporterprotectedcheck = 0;

	//  Ensure it has the dbo extension
	Dim (  t.tArray,400  );
	tArrayMarker = 0;
	tstring_s=timporterfilecheck_s;
	tToken_s=FirstToken(tstring_s.Get(),".");
	if (  tToken_s  !=  "" ) 
	{
		t.tArray[tArrayMarker] = tToken_s;
		++tArrayMarker;
	}
	do
	{
		tToken_s=NextToken(".");
		if (  tToken_s  !=  "" ) 
		{
			t.tArray[tArrayMarker] = tToken_s;
			++tArrayMarker;
		}
	} while ( !(  tToken_s == "" ) );
	tStippedFileName_s = "";
	for ( tCount = 0 ; tCount<=  tArrayMarker-2; tCount++ )
	{
		tStippedFileName_s = tStippedFileName_s + t.tArray[tCount];
	}
	UnDim (  t.tArray );
	Dim (  t.tArray,400  );

	tStippedFileName_s = timporterfilecheck_s;
	if (  cstr(Mid(tStippedFileName_s.Get(),Len(tStippedFileName_s.Get())-1))  ==  "." ) 
	{
			tStippedFileName_s = Left(tStippedFileName_s.Get(),Len(tStippedFileName_s.Get())-2);
	}
	else
	{
		tStippedFileName_s = Left(tStippedFileName_s.Get(),Len(tStippedFileName_s.Get())-4);
	}
	t.strwork = ""; t.strwork =  tStippedFileName_s + ".fpe";
	strcpy ( timporterfilecheck_s , t.strwork.Get() );

	//  Check if an FPE exists, if so load it in
	if (  FileOpen(1)  )  CloseFile (1) ;
	if (  FileExist (timporterfilecheck_s) ) 
	{
		OpenToRead (  1 , timporterfilecheck_s );
		while (  FileEnd(1)  ==  0 ) 
		{
			t.tstring_s = ReadString (  1 );
			tempLine_s = t.tstring_s;

			tArrayMarker = 0;
			tToken_s=FirstToken(t.tstring_s.Get()," ");
			if (  tToken_s  !=  "" ) 
			{
				t.tArray[tArrayMarker] = tToken_s;
				++tArrayMarker;
			}
			do
			{
				tToken_s=NextToken(" ");
				if (  tToken_s  !=  "" ) 
				{
					t.tArray[tArrayMarker] = tToken_s;
					++tArrayMarker;
				}
			} while ( !(  tToken_s == "" ) );
			tStippedString_s = "";
			for ( tCount = 0 ; tCount<=  tArrayMarker-1; tCount++ )
			{
				if (  tCount < 3 ) 
				{
					tStippedString_s = tStippedString_s + t.tArray[tCount];
				}
				else
				{
					tStippedString_s = tStippedString_s + " " + t.tArray[tCount];
				}
			}
			if (  tStippedString_s  !=  "" && tStippedString_s.Get()[0]  !=  ';' )
			{
				tToken_s=FirstToken(tStippedString_s.Get(),"=");
				tToken2_s=NextToken("=");

				//  Get rid of any tabs that exist and replace with nothing (some files have tabs in sometimes)
				t.tstring_s = tToken_s ; tToken_s = "";
				for ( tCount = 1 ; tCount<=  Len(t.tstring_s.Get()); tCount++ )
				{
					if ( cstr( Mid(t.tstring_s.Get(),tCount))  !=  Chr(9)  )  tToken_s  =  tToken_s + Mid(t.tstring_s.Get(),tCount);
				}

				t.tstring_s = tToken2_s ; tToken2_s = "";
				for ( tCount = 1 ; tCount<=  Len(t.tstring_s.Get()); tCount++ )
				{
					if (  cstr(Mid(t.tstring_s.Get(),tCount))  !=  Chr(9)  )  tToken2_s  =  tToken2_s + Mid(t.tstring_s.Get(),tCount);
				}

				//  find out which token we have and set the importer fpe string
				if ( tToken_s == "protected" ) timporterprotectedcheck  =  ValF(tToken2_s.Get()) ;
			}
		}

	}

	CloseFile (  1 );
	UnDim (  t.tArray );

	if (  timporterprotectedcheck > 1  )  timporterprotectedcheck  =  1;
	if (  timporterprotectedcheck < 0  )  timporterprotectedcheck  =  0;

	return timporterprotectedcheck;
}

void importer_sort_names ( void )
{
	//  Split the filename into tokens to grab the path, object name and create fpe name
	Dim (  t.tArray,400  );
	t.tArrayMarker = 0;
	t.tstring_s=t.timporterfile_s;
	t.tToken_s=FirstToken(t.tstring_s.Get(),"\\");
	if (  t.tToken_s  !=  "" ) 
	{
		t.tArray[t.tArrayMarker] = t.tToken_s;
		++t.tArrayMarker;
	}
	do
	{
		t.tToken_s=NextToken("\\");
		if (  t.tToken_s  !=  "" ) 
		{
			t.tArray[t.tArrayMarker] = t.tToken_s;
			++t.tArrayMarker;
		}
	} while ( !(  t.tToken_s == "" ) );
	t.tStippedFileName_s = "";

	//  Grab path only
	int tCount = 0;
	for ( tCount = 0 ; tCount<=  t.tArrayMarker-2; tCount++ )
	{
		t.tStippedFileName_s = t.tStippedFileName_s + t.tArray[tCount] + "\\";
	}

	//  Store file path
	t.importer.objectFileOriginalPath = t.tStippedFileName_s;
	if (  cstr(Left(t.importer.objectFileOriginalPath.Get(),2))  ==  ".." ) 
	{
		t.tFPSDir_s = t.importer.startDir;
		t.tFPSDir_s = Left(t.tFPSDir_s.Get(),Len(t.tFPSDir_s.Get())-6);
		t.importer.objectFileOriginalPath = t.tFPSDir_s + Right(t.importer.objectFileOriginalPath.Get(),Len(t.importer.objectFileOriginalPath.Get())-2);
	}

	//  Now store just the file names
	t.tStippedFileName_s = t.tArray[t.tArrayMarker-1];
	t.tOriginalName_s = t.tStippedFileName_s;
	t.tArrayMarker = 0;
	t.tstring_s=t.tStippedFileName_s;
	t.tToken_s=FirstToken(t.tstring_s.Get(),".");
	if (  t.tToken_s  !=  "" ) 
	{
		t.tArray[t.tArrayMarker] = t.tToken_s;
		++t.tArrayMarker;
	}
	do
	{
		t.tToken_s=NextToken(".");
		if (  t.tToken_s  !=  "" ) 
		{
			t.tArray[t.tArrayMarker] = t.tToken_s;
			++t.tArrayMarker;
		}
	} while ( !(  t.tToken_s == "" ) );
	t.tStippedFileName_s = "";
	for ( tCount = 0 ; tCount<=  t.tArrayMarker-2; tCount++ )
	{
		t.tStippedFileName_s = t.tStippedFileName_s + t.tArray[tCount];
	}

	t.tStippedFileName_s = t.tOriginalName_s;
	if (  cstr(Mid(t.tStippedFileName_s.Get(),Len(t.tStippedFileName_s.Get())-1))  ==  "." ) 
	{
			t.tStippedFileName_s = Left(t.tStippedFileName_s.Get(),Len(t.tStippedFileName_s.Get())-2);
	}
	else
	{
		t.tStippedFileName_s = Left(t.tStippedFileName_s.Get(),Len(t.tStippedFileName_s.Get())-4);
	}

	//  Store file names
	t.importer.objectFilename = t.tStippedFileName_s + "." + t.tArray[t.tArrayMarker-1];
	t.importer.objectFilenameFPE = t.tStippedFileName_s +  ".fpe";
	t.importer.objectFilenameExtension = cstr(".") + t.tArray[t.tArrayMarker-1];
	UnDim (  t.tArray );
	
	//  check if it is an fpe that has been loaded in, if so we need to change the model name
	if (  cstr(Lower(Right(t.timporterfile_s.Get(),4)))  ==  ".fpe" ) 
	{
		t.importer.fpeIsMainFile = 1;
		importer_find_object_name_from_fpe ( );
	}
	else
	{
		t.importer.fpeIsMainFile = 0;
	}
}

void importer_find_object_name_from_fpe ( void )
{
	t.timporterprotectedcheck = 0;
	Dim (  t.tArray,400  );
	t.timporterfilecheck_s = t.timporterfile_s;

	//  Check if an FPE exists, if so load it in
	if ( FileOpen(1) )  CloseFile (1);
	if (  FileExist (t.timporterfilecheck_s.Get()) ) 
	{
		OpenToRead (  1 , t.timporterfilecheck_s.Get() );
		while (  FileEnd(1)  ==  0 ) 
		{
			t.tstring_s = ReadString (  1 );
			t.tempLine_s = t.tstring_s;

			t.tArrayMarker = 0;
			t.tToken_s=FirstToken(t.tstring_s.Get()," ");
			if (  t.tToken_s  !=  "" ) 
			{
				t.tArray[t.tArrayMarker] = t.tToken_s;
				++t.tArrayMarker;
			}
			do
			{
				t.tToken_s=NextToken(" ");
				if (  t.tToken_s  !=  "" ) 
				{
					t.tArray[t.tArrayMarker] = t.tToken_s;
					++t.tArrayMarker;
				}
			} while ( !(  t.tToken_s == "" ) );
			t.tStippedString_s = "";
			int tCount = 0;
			for ( tCount = 0 ; tCount<=  t.tArrayMarker-1; tCount++ )
			{
				if (  tCount < 3 ) 
				{
					t.tStippedString_s = t.tStippedString_s + t.tArray[tCount];
				}
				else
				{
					t.tStippedString_s = t.tStippedString_s + " " + t.tArray[tCount];
				}
			}
			if (  t.tStippedString_s  !=  "" && t.tStippedString_s.Get()[0]  !=  ';' )
			{
				t.tToken_s=FirstToken(t.tStippedString_s.Get(),"=");
				t.tToken2_s=NextToken("=");

				//  Get rid of any tabs that exist and replace with nothing (some files have tabs in sometimes)
				t.tstring_s = t.tToken_s ; t.tToken_s = "";
				for ( int tCount2 = 1 ; tCount2 <=  Len(t.tstring_s.Get()); tCount2++ )
				{
					if (  cstr(Mid(t.tstring_s.Get(),tCount2))  !=  Chr(9)  )  t.tToken_s  =  t.tToken_s + Mid(t.tstring_s.Get(),tCount2);
				}

				t.tstring_s = t.tToken2_s ; t.tToken2_s = "";
				for ( tCount = 1 ; tCount<=  Len(t.tstring_s.Get()); tCount++ )
				{
					if ( cstr( Mid(t.tstring_s.Get(),tCount) ) !=  Chr(9)  )  t.tToken2_s  =  t.tToken2_s + Mid(t.tstring_s.Get(),tCount);
				}

				if ( t.tToken_s == "model" ) t.importer.objectFilename  =  t.tToken2_s ;
			}
		}

	}

	CloseFile (  1 );
	UnDim (  t.tArray );
	return;
}

void importer_hide_mouse ( void )
{
	HideMouse (  );
}

void importer_show_mouse ( void )
{
	ShowMouse (  );
}

void importer_fade_out ( void )
{
	t.tfound = 0;
	t.twhiteobj = 1;
	while (  t.tfound  ==  0 ) 
	{
		if (  ObjectExist(t.twhiteobj)  ==  1 ) 
		{
			++t.twhiteobj;
		}
		else
		{
			t.tfound = 1;
		}
	}

	MakeObjectBox (  t.twhiteobj,200000,200000,1 );
	SetObjectLight (  t.twhiteobj,1 );
	LockObjectOn (  t.twhiteobj );
	PositionObject (  t.twhiteobj,0,0,4000 );
	DisableObjectZDepth (  t.twhiteobj );
	SetAlphaMappingOn (  t.twhiteobj,0 );
	ColorObject (  t.twhiteobj , Rgb(0,0,0) );
	SetObjectAmbience (  t.twhiteobj,0 );
	SetObjectEmissive (  t.twhiteobj, Rgb(40,104,131) );

	for ( t.tc = 0 ; t.tc <=  100 ; t.tc += 10 )
	{
			SetAlphaMappingOn (  t.twhiteobj,t.tc );
			Sync (  );
	}
	DeleteObject (  t.twhiteobj );
}

void importer_fade_in ( void )
{
	t.tfound = 0;
	t.twhiteobj = 1;
	while (  t.tfound  ==  0 ) 
	{
		if (  ObjectExist(t.twhiteobj)  ==  1 ) 
		{
			++t.twhiteobj;
		}
		else
		{
			t.tfound = 1;
		}
	}

	MakeObjectBox (  t.twhiteobj,200000,200000,1 );
	SetObjectLight (  t.twhiteobj,1 );
	LockObjectOn (  t.twhiteobj );
	PositionObject (  t.twhiteobj,0,0,4000 );
	DisableObjectZDepth (  t.twhiteobj );
	SetAlphaMappingOn (  t.twhiteobj,100 );
	ColorObject (  t.twhiteobj , Rgb(0,0,0) );
	SetObjectAmbience (  t.twhiteobj,0 );
	SetObjectEmissive (  t.twhiteobj, Rgb(40,104,131) );

	for ( t.tc = 100 ; t.tc >=  0 ; t.tc+= -10 )
	{
			SetAlphaMappingOn (  t.twhiteobj,t.tc );
			Sync (  );
	}
	DeleteObject (  t.twhiteobj );
}

void importer_check_script_token_exists ( void )
{
	t.tfound = 0;
	for ( t.tloop = 0 ; t.tloop<=  t.importer.scriptFileCount-1; t.tloop++ )
	{
			if (  t.tToken2_s  ==  t.importerScriptFiles[t.tloop]  )  t.tfound  =  1;
	}

	if (  t.tfound  ==  0  )  t.tToken2_s  =  "default.lua";
}

void importer_update_scale ( void )
{
	if (bRemoveSprites)
		return;

	if (  t.inputsys.mclick  ==  0 ) 
	{
		t.importer.dropDownListNumber = 0;
		t.importer.oldTime = 0;

		if (  t.timporterprevscale  !=  t.slidersmenuvalue[t.importer.properties1Index][1].value ) 
		{
			t.timporterprevscale = t.slidersmenuvalue[t.importer.properties1Index][1].value;
			t.slidersmenuvalue[t.importer.properties1Index][1].value_s=Str(t.slidersmenuvalue[t.importer.properties1Index][1].value);
			t.slidersmenuvalue[t.importer.properties1Index][1].readmodeindex=50;
			t.slidersmenuvalue[t.importer.properties1Index][1].useCustomRange = 1;
			t.slidersmenuvalue[t.importer.properties1Index][1].valueMin = t.slidersmenuvalue[t.importer.properties1Index][1].value-49;
			t.slidersmenuvalue[t.importer.properties1Index][1].valueMax = t.slidersmenuvalue[t.importer.properties1Index][1].value+50;
			if (  t.slidersmenuvalue[t.importer.properties1Index][1].valueMin < 1 ) 
			{
				t.slidersmenuvalue[t.importer.properties1Index][1].valueMin = 1;
				t.slidersmenuvalue[t.importer.properties1Index][1].valueMax = 100;//400; for better lower scale resolution work
			}
		}
	}
}

