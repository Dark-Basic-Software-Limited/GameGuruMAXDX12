void grass_clearregion ( void )
{
	//  clear the grass memblock in the described terrain region
	t.tvegareax1=((t.terrain.terrainregionx1+0.0)/5)-1;
	t.tvegareax2=((t.terrain.terrainregionx2+0.0)/5)+1;
	t.tvegareaz1=((t.terrain.terrainregionz1+0.0)/5)-1;
	t.tvegareaz2=((t.terrain.terrainregionz2+0.0)/5)+1;
	if (  t.tvegareax1<0  )  t.tvegareax1 = 0;
	if (  t.tvegareax2>MAXTEXTURESIZE-1  )  t.tvegareax2 = MAXTEXTURESIZE - 1;
	if (  t.tvegareaz1<0  )  t.tvegareaz1 = 0;
	if (  t.tvegareaz2>MAXTEXTURESIZE-1  )  t.tvegareaz2 = MAXTEXTURESIZE - 1;
	for ( t.tvz = t.tvegareaz1 ; t.tvz<=  t.tvegareaz2; t.tvz++ )
	{
		t.tZStep = t.tvz*MAXTEXTURESIZE;
		for ( t.tvx = t.tvegareax1 ; t.tvx<=  t.tvegareax2; t.tvx++ )
		{
			WriteMemblockByte (  t.terrain.grassmemblock,(4+4+4+((t.tvx+t.tZStep)*4))+2,0 );
		}
	}
}

void grass_updatedirtyregionfast ( void )
{
	//  scan vegmap image and extract grass values into veggrass memblock
	SetCurrentBitmap (  g.terrainworkbitmapindex );
//  `paste image terrain.imagestartindex+1,0,0

	PasteImage (  t.terrain.imagestartindex+2,0,0 );
	LockPixels (  );
	//  *2 as the veggrass aray is 2048 vs 1024 terraingrid coords
	t.tValue = t.terrain.grassregionz1*2 ; grass_clamptomemblockres() ; t.tBack = t.tValue;
	t.tValue = t.terrain.grassregionz2*2 ; grass_clamptomemblockres() ; t.tFront = t.tValue;
	t.tValue = t.terrain.grassregionx1*2 ; grass_clamptomemblockres() ; t.tLeft = t.tValue;
	t.tValue = t.terrain.grassregionx2*2 ; grass_clamptomemblockres() ; t.tRight = t.tValue;
	for ( t.tvz = t.tBack ; t.tvz<=  t.tFront; t.tvz++ )
	{
		t.tZStep = t.tvz*MAXTEXTURESIZE;
		for ( t.tvx = t.tLeft ; t.tvx<=  t.tRight; t.tvx++ )
		{
			WriteMemblockByte (  t.terrain.grassmemblock,(4+4+4+((t.tvx+t.tZStep)*4))+2,RgbR(GetPoint(t.tvx,t.tvz)) );
		}
	}
	UnlockPixels (  );
	SetCurrentBitmap (  0 );
	t.tRegionX1 = t.terrain.grassregionx1 * 50;
	t.tRegionX2 = t.terrain.grassregionx2 * 50;
	t.tRegionZ1 = t.terrain.grassregionz1 * 50;
	t.tRegionZ2 = t.terrain.grassregionz2 * 50;
	if (  t.tRegionX1  !=  t.terrain.lastgrassupdatex1 || t.tRegionX2  !=  t.terrain.lastgrassupdatex2 || t.tRegionZ1  !=  t.terrain.lastgrassupdatez1 || t.tRegionZ2  !=  t.terrain.lastgrassupdatez2 ) 
	{
		if (  t.terrain.superflat  ==  1 ) 
		{
			UpdateVegZoneSuperFlat (  t.tRegionX1,t.tRegionZ1,t.tRegionX2,t.tRegionZ2,TERRAIN_SUPERFLAT_HEIGHT );
		}
		else
		{
			UpdateVegZoneBlitzTerrain (  t.tRegionX1,t.tRegionZ1,t.tRegionX2,t.tRegionZ2,t.terrain.TerrainID );
		}
		t.terrain.lastgrassupdatex1 = t.tRegionX1;
		t.terrain.lastgrassupdatex2 = t.tRegionX2;
		t.terrain.lastgrassupdatez1 = t.tRegionZ1;
		t.terrain.lastgrassupdatez2 = t.tRegionZ2;
	}
}

void grass_clamptomemblockres ( void )
{
	//  clamps input value tValue to the memblock resolution
	if (  t.tValue < 0  )  t.tValue  =  0;
	if (  t.tValue  >=  MAXTEXTURESIZE  )  t.tValue  =  MAXTEXTURESIZE - 1;
}

void grass_updategrassfrombitmap ( void )
{
	// if no image, must have had a VIDMEM reset, leave quietly
	if ( ImageExist(t.terrain.imagestartindex+2) == 0  )  return;

	// uses raw image data inside VEG module
	if ( MemblockExist(t.terrain.grassmemblock) == 1  )  DeleteMemblock (  t.terrain.grassmemblock );
	CreateMemblockFromImage ( t.terrain.grassmemblock, t.terrain.imagestartindex+2 );
	
	//this was only required because images are loading in RGBA, when they should be BGRA (as it was with DX9!!)
	//ConvertVegMemblock ( t.terrain.grassmemblock );

	// slopes and water can't exist in superflat mode, so only delete invalid grass in terrain mode
	if ( t.terrain.superflat == 0  )  DeleteInvalidGrass (  t.terrain.TerrainID,t.terrain.waterliney_f,1.0 );
}

void grass_loadgrass ( void )
{
	//  load grass data memblock
	if (  FileExist(t.tfileveggrass_s.Get()) == 1 ) 
	{
		OpenToRead (  3,t.tfileveggrass_s.Get() );
		if (  MemblockExist(t.terrain.grassmemblock)  )  DeleteMemblock (  t.terrain.grassmemblock );
		ReadMemblock (  3,t.terrain.grassmemblock );
		CloseFile (  3 );

		//  151214 - if old grass memblock, reconstruct as new raw image grass memblock
		if (  ReadMemblockDWord(t.terrain.grassmemblock,8) != 32 ) 
		{
			DeleteMemblock (  t.terrain.grassmemblock );
			MakeMemblock (  t.terrain.grassmemblock,4+4+4+((MAXTEXTURESIZE*MAXTEXTURESIZE)*4) );
			WriteMemblockDWord (  t.terrain.grassmemblock,0,MAXTEXTURESIZE );
			WriteMemblockDWord (  t.terrain.grassmemblock,4,MAXTEXTURESIZE );
			WriteMemblockDWord (  t.terrain.grassmemblock,8,32 );
		}

		//  This helper call removes all grass entries in the grass memblock where the grass exists on a steep slope or under water.
		//  It can be called at any time when slope grass needs to be removed, but SetResourceValues (  must have been called earlier. )
		//  It is quite slow because it cycles through several million memblock entries and makes ground height calcs for each!
		//  Params; terrain ID, waterheight, max height difference over 1 unit
		if (  t.terrain.superflat == 0  )  DeleteInvalidGrass (  t.terrain.TerrainID,t.terrain.waterliney_f,1.0 );
	}
	else
	{
		grass_buildblankgrass ( );
	}
}

void grass_savegrass ( void )
{
	// regenerate the memblock from the vegmask bitmap for consistency
	t.terrain.grassregionupdate=0;
	grass_updategrassfrombitmap ( );

	// save grass memblock to disk
	if ( FileExist(t.tfileveggrass_s.Get()) == 1 ) DeleteAFile ( t.tfileveggrass_s.Get() );
	if ( OpenToWriteEx ( 3, t.tfileveggrass_s.Get() ) == true )
	{
		WriteMemblock (  3,t.terrain.grassmemblock );
		CloseFile (  3 );
	}
}

void grass_buildblankgrass ( void )
{
	//  make a blank grass data memblock, or clear the one that already exists
	//  151214 - wrong format but VEH module deals with nonraw-image memblocks as ZERO
	if (  MemblockExist(t.terrain.grassmemblock)  ==  0 ) 
	{
		MakeMemblock (  t.terrain.grassmemblock,4+4+4+((MAXTEXTURESIZE*MAXTEXTURESIZE)*4) );
	}
	WriteMemblockDWord (  t.terrain.grassmemblock,0,MAXTEXTURESIZE );
	WriteMemblockDWord (  t.terrain.grassmemblock,4,MAXTEXTURESIZE );
	WriteMemblockDWord (  t.terrain.grassmemblock,8,32 );
	t.tPindex=4+4+4;
	for ( t.tP = 0 ; t.tP<=  MAXTEXTURESIZE*MAXTEXTURESIZE - 1; t.tP++ )
	{
		WriteMemblockByte (  t.terrain.grassmemblock,t.tPindex+2,0 );
		t.tPindex += 4;
	}
	grass_savegrass ( );
}

void grass_buildblankgrass_fornew ( void )
{

	//  Create new memblock for grass
	if (  MemblockExist(t.terrain.grassmemblock) == 1  )  DeleteMemblock (  t.terrain.grassmemblock );
	if (  MemblockExist(t.terrain.grassmemblock) == 0 ) 
	{
		MakeMemblock (  t.terrain.grassmemblock,4+4+4+((MAXTEXTURESIZE*MAXTEXTURESIZE)*4) );
		WriteMemblockDWord (  t.terrain.grassmemblock,0,MAXTEXTURESIZE );
		WriteMemblockDWord (  t.terrain.grassmemblock,4,MAXTEXTURESIZE );
		WriteMemblockDWord (  t.terrain.grassmemblock,8,32 );
	}

	// save grass memblock to disk
	// 151214 - wrong format but VEH module deals with nonraw-image memblocks as ZERO
	if ( FileExist(t.tfileveggrass_s.Get()) == 1 ) DeleteAFile ( t.tfileveggrass_s.Get() );
	if ( OpenToWriteEx ( 3, t.tfileveggrass_s.Get() ) == true )
	{
		WriteMemblock ( 3, t.terrain.grassmemblock );
		CloseFile ( 3 );
	}
}

void grass_free ( void )
{
	//  We used to DeleteVegetationGrid (  here to clear away veg. Now to save time during testing we HideVegetationGrid (  instead. ) )
	HideVegetationGrid (  );
}

//PE: Missing Classic functions.
#ifdef PRODUCTCLASSIC
void grass_initstyles(void)
{
	// Collect vegetation styles
	g.vegstylemax = 0;
#ifdef WICKEDENGINE
#else
	SetDir("vegbank");
	ChecklistForFiles();
	for (t.c = 1; t.c <= ChecklistQuantity(); t.c++)
	{
		t.tfile_s = ChecklistString(t.c);
		if (ChecklistValueA(t.c) == 1)
		{
			if (t.tfile_s.Get()[0] != '.')
			{
				++g.vegstylemax;
				Dim(t.vegstylebank_s, g.vegstylemax);
				t.vegstylebank_s[g.vegstylemax] = Lower(t.tfile_s.Get());
			}
		}
	}
	SetDir("..");
#endif

	// and init grass choice
	grass_initstyles_reset();
}

void grass_initstyles_reset(void)
{
#ifdef WICKEDENGINE
	// when init grass plate, copy it from original (typically only done once for each new level)
	g.vegstyle_s = "";
	g.vegstyleindex = 0;
	//for (int iGrassTexSet = 0; iGrassTexSet < 3; iGrassTexSet++)
	for (int iGrassTexSet = 0; iGrassTexSet < 1; iGrassTexSet++)
	{
		char pParentOriginal[MAX_PATH];
		strcpy(pParentOriginal, g.fpscrootdir_s.Get());
		strcat(pParentOriginal, "\\Files\\vegbank\\original");
		if (iGrassTexSet == 0) strcat(pParentOriginal, "_coloronly.dds");
		//if (iGrassTexSet == 1) strcat(pParentOriginal, "_normal.dds");
		//if (iGrassTexSet == 2) strcat(pParentOriginal, "_surface.dds");
		char pRealDestFile[MAX_PATH];
		strcpy(pRealDestFile, "levelbank\\testmap\\grass");
		if (iGrassTexSet == 0) strcat(pRealDestFile, "_coloronly.dds");
		//if (iGrassTexSet == 1) strcat(pRealDestFile, "_normal.dds");
		//if (iGrassTexSet == 2) strcat(pRealDestFile, "_surface.dds");
		GG_GetRealPath(pRealDestFile, 1);
		if (FileExist(pRealDestFile) == 1) DeleteFileA(pRealDestFile);
		CopyFileA(pParentOriginal, pRealDestFile, FALSE);
	}
#else
	g.vegstyle_s = "lushy";
	if (PathExist(cstr(cstr("vegbank\\") + g.vegstyle_s).Get()) == 0)
	{
		g.vegstyle_s = t.vegstylebank_s[1];
	}
	//  find vegstyle index
	for (g.vegstyleindex = 1; g.vegstyleindex <= g.vegstylemax; g.vegstyleindex++)
	{
		if (cstr(Lower(g.vegstyle_s.Get())) == t.vegstylebank_s[g.vegstyleindex])
		{
			break;
		}
	}
	if (g.vegstyleindex > g.vegstylemax)
	{
		g.vegstyleindex = g.vegstylemax;
		g.vegstyle_s = t.vegstylebank_s[g.vegstyleindex];
	}
#endif
}

void grass_changevegstyle(void)
{
#ifdef WICKEDENGINE
	grass_setgrassimage();
#else
	// replace vegetaion mesh and texture
	g.vegstyle_s = t.vegstylebank_s[g.vegstyleindex];
	if (PathExist(cstr(cstr("vegbank\\") + g.vegstyle_s).Get()) == 1)
	{
		grass_setgrassimage();
	}
#endif
}
#endif

*/
