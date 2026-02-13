void mapfile_convertCLASSICtoMAX(LPSTR pFPMLoaded)
{
	//  Store and switch folders
	cstr pOldDir = GetDir();

	// check if in mapbank of GAMEGURU CLASSIC
	bool bAllowOneWayConversion = false;
	char pReconstructGameGuruRootFiles[MAX_PATH];
	strcpy(pReconstructGameGuruRootFiles, "");
	char pReconstructGameGuruFolder[MAX_PATH];
	strcpy(pReconstructGameGuruFolder, "");
	char pReconstructGameGuruEXE[MAX_PATH];
	strcpy(pReconstructGameGuruEXE, pFPMLoaded);
	char* pFindClassicFolder = (char*)pestrcasestr(pReconstructGameGuruEXE, "Game Guru\\Files\\mapbank\\");
	if (pFindClassicFolder != NULL)
	{
		*pFindClassicFolder = 0;
		strcpy(pReconstructGameGuruRootFiles, pReconstructGameGuruEXE);
		strcat(pReconstructGameGuruRootFiles, "Game Guru\\Files\\");
		strcpy(pReconstructGameGuruFolder, pReconstructGameGuruEXE);
		strcat(pReconstructGameGuruFolder, "Game Guru\\Files\\entitybank\\");
		strcat(pReconstructGameGuruEXE, "Game Guru\\GameGuru.exe");
		if ( FileExist(pReconstructGameGuruEXE)==1 )
		{
			bAllowOneWayConversion = true;
		}
	}
	if (bAllowOneWayConversion == false)
		return;

	// go into levelbank\testmap
	SetDir ( g.mysystem.levelBankTestMap_s.Get() ); // "levelbank\\testmap\\"

	// debug log for conversion
	timestampactivity(0, "mapfile_convertCLASSICtoMAX:" );

	// original terrain heights are 1024x1024 (stretched over 51200x51200 units = spacing of 50.0f (vs new terrain spacing of 10.0f))
	int iSrcHeightWidth = 1024;
	int iSrcHeightHeight = 1024;

#ifdef FULLTERRAINEDITING
	// load in old terrain heights
	LPSTR pTerrainHeightFile = "m.dat";
	if (FileExist(pTerrainHeightFile) == 1)
	{
		if (1)
		{
			uint32_t size1 = 4096 * 4096 * sizeof(uint8_t);
			uint32_t size2 = 4096 * 4096 * sizeof(float);
			uint32_t terrain_sculpt_size = GGTerrain::GGTerrain_GetSculptDataSize();
			float fHeightAdjust = 600.0f; //600.0f;
			float fHeightDivider = 1.0f/5000.0f;
			char *data = new char[terrain_sculpt_size];
			if (data)
			{
				GGTerrain::GGTerrain_GetSculptData((uint8_t*)data);
				float* pHeightMapEdit = (float*)(data + size1);
				uint8_t* pHeightMapEditType = (uint8_t*)data;

				if (FileOpen(1) == 1) CloseFile(1);
				OpenToRead(1, pTerrainHeightFile);
				if (MemblockExist(1) == 1) DeleteMemblock(1);
				if (FileOpen(1) == 1)
				{
					ReadMemblock(1, 1);
					int mi = 0;
					for (int z = 0; z <= 1023; z++)
					{
						for (int x = 0; x <= 1023; x++)
						{
							float scale = 2.0;
							int offsetz = 1024; //1536
							int offsetx = 1024; //1536
							//float fHeight = (ReadMemblockFloat(1, mi) - fHeightAdjust) * 2.0; //Test 5000.0 look OK.
							float fHeight = (ReadMemblockFloat(1, mi) - fHeightAdjust); //Test 5000.0 look OK.
							float fHeight1= fHeight, fHeight2= fHeight, fHeight3= fHeight;
							fHeight1 = fHeight2 = fHeight3 = fHeight;
							if (z < 1023 && x < 1023)
							{
								fHeight1 = (ReadMemblockFloat(1, mi + 4) - fHeightAdjust);
								fHeight2 = (ReadMemblockFloat(1, mi + 4 + (1024 * 4)) - fHeightAdjust);
								fHeight3 = (ReadMemblockFloat(1, mi + (1024 * 4)) - fHeightAdjust);
							}
							
							if (x > 0 && x < 1023 && z > 0 && z < 1023)
							{

								x++;
								z++;

								uint32_t newindex = (4096 - 1 - (offsetz + (z * scale + 0))) * 4096 + (offsetx + (x * scale + 0));
								pHeightMapEditType[newindex] = 1; //Replace.
								pHeightMapEdit[newindex] = (fHeight) *fHeightDivider;


								newindex = (4096 - 1 - (offsetz + (z * scale + 0))) * 4096 + (offsetx + (x * scale + 1));
								pHeightMapEditType[newindex] = 1; //Replace.
								pHeightMapEdit[newindex] = ((fHeight + fHeight1) * 0.5) * fHeightDivider;

								newindex = (4096 - 1 - (offsetz + (z * scale + 1))) * 4096 + (offsetx + (x * scale + 1));
								pHeightMapEditType[newindex] = 1; //Replace.
								pHeightMapEdit[newindex] = ((fHeight + fHeight2) * 0.5) * fHeightDivider;

								newindex = (4096 - 1 - (offsetz + (z * scale + 1))) * 4096 + (offsetx + (x * scale + 0));
								pHeightMapEditType[newindex] = 1; //Replace.
								pHeightMapEdit[newindex] = ((fHeight + fHeight3) * 0.5) * fHeightDivider;

								x--;
								z--;

							}
							else
							{
								for (int zz = 0; zz <= 1; zz++)
								{
									for (int xx = 0; xx <= 1; xx++)
									{
										uint32_t newindex = (4096 - 1 - (offsetz + (z * scale + zz))) * 4096 + (offsetx + (x * scale + xx));
										pHeightMapEditType[newindex] = 0;
									}
								}
							}
							mi += 4;
						}
					}
					DeleteMemblock(1);
					CloseFile(1);

					void procedural_set_heightmap_level(void);
					procedural_set_heightmap_level();
					GGTerrain::GGTerrain_SetSculptData(terrain_sculpt_size, (uint8_t*)data);
					void check_new_terrain_parameters(void);
					check_new_terrain_parameters();
				}
				delete(data);

			}
		}
	}

#else

	float* fSrcHeightData = new float[iSrcHeightWidth*iSrcHeightHeight];

	// load in old terrain heights
	LPSTR pTerrainHeightFile = "m.dat";
	if ( FileExist(pTerrainHeightFile) == 1 ) 
	{
		OpenToRead ( 1, pTerrainHeightFile );
		if (MemblockExist(1) == 1) DeleteMemblock(1);
		ReadMemblock ( 1, 1 );
		int mi = 0;
		for ( int z = 0; z <= 1023; z++ )
		{
			for (int x = 0; x <= 1023; x++)
			{
				float fHeight = ReadMemblockFloat(1, mi);
				if (x > 0 && x < 1023 && z > 0 && z < 1023)
					fSrcHeightData[x + (z * iSrcHeightWidth)] = fHeight;
				else
					fSrcHeightData[x + (z * iSrcHeightWidth)] = 0.0f;
				mi += 4;
			}
		}
		DeleteMemblock(1);
		CloseFile(1);
	}

	// old terrain and new terrain are at different scales, so sample from an offset and scale factor
	// that lines up center of old terrain with center of terrain node zero (TTR0XR0)
	float fActualCoverage = (1024.0f / 51200.0f) * 10000.0f;
	float fSampleStartCorner = 512.0f - (fActualCoverage / 2.0f);
	int iNearestSrcXZ = (int)fSampleStartCorner;

	// leelee, hack!
	iNearestSrcXZ += 3;

	float fScaleToSrc = 10000.0f / 51200.0f;

	// save out new terrain node height files
	LPSTR pTerrainNodeFolder = "TTR0XR0";
	if (PathExist(pTerrainNodeFolder) == 0) MakeDirectory(pTerrainNodeFolder);
	SetDir(pTerrainNodeFolder);
	unsigned int iLOD = 0;
	for (unsigned int iIndexX = 0; iIndexX < 10; iIndexX++)
	{
		for (unsigned int iIndexZ = 0; iIndexZ < 10; iIndexZ++)
		{
			std::string sBaseFile = "terrain_data_" + std::to_string(iIndexX) + "_" + std::to_string(iIndexZ) + "_level_" + std::to_string(iLOD);
			std::string sHeightFile = sBaseFile + ".dat";
			FILE* fp = NULL;
			fopen_s(&fp, sHeightFile.c_str(), "wb");
			if (fp)
			{
				// dimenions of height field
				unsigned int iFileWidth = 101;
				unsigned int iFileHeight = 101;
				fwrite(&iFileWidth, sizeof(unsigned int), 1, fp);
				fwrite(&iFileHeight, sizeof(unsigned int), 1, fp);

				// write out as geometry
				float fTerrainNewGap = 10.0f;
				int iX = 0;
				int iZ = 0;
				float fX = 0.0f;
				float fZ = 0.0f;
				for (int i = 0; i < iFileWidth * iFileHeight; i++)
				{
					// get source coordinate to old terrain data using the scaled coordinate 
					float fWorldX = (iIndexX * 100 * fTerrainNewGap) + fX;
					float fWorldZ = (iIndexZ * 100 * fTerrainNewGap) + fZ;
					fWorldX *= fScaleToSrc;
					fWorldZ *= fScaleToSrc;
					float fScaledOffsetX = fWorldX / fTerrainNewGap;
					float fScaledOffsetZ = fWorldZ / fTerrainNewGap;
					float fSrcX = (float)iNearestSrcXZ + fScaledOffsetX;
					float fSrcZ = (float)iNearestSrcXZ + fScaledOffsetZ;

					// calculate surface height between four points
					// takes surface position height between four points
					float fHeight = 0.0f;
					int iSrcX = (int)(fSrcX);
					int iSrcZ = (int)(fSrcZ);
					int iSrcX2 = iSrcX + 1;
					int iSrcZ2 = iSrcZ + 1;
					float fMidX = (fSrcX - (float)iSrcX);
					float fMidZ = (fSrcZ - (float)iSrcZ);
					float fHeightX1 = fSrcHeightData[iSrcX + (iSrcZ * iSrcHeightWidth)];
					float fHeightX2 = fSrcHeightData[iSrcX2 + (iSrcZ * iSrcHeightWidth)];
					float fHeightA = fHeightX1 + (fHeightX2 - fHeightX1) * fMidX;
					fHeightX1 = fSrcHeightData[iSrcX + (iSrcZ2 * iSrcHeightWidth)];
					fHeightX2 = fSrcHeightData[iSrcX2 + (iSrcZ2 * iSrcHeightWidth)];
					float fHeightB = fHeightX1 + (fHeightX2 - fHeightX1) * fMidX;
					fHeight = fHeightA + (fHeightB - fHeightA) * fMidZ;

					// new terrain nodes raise objects by 600, so deduct this affect
					if (fHeight > 0.0f) fHeight -= 600.0f;

					// calculate vert
					XMFLOAT3 pos;
					pos.x = fX;
					pos.y = fHeight;
					pos.z = fZ;
					fX += fTerrainNewGap; iX++;
					if (iX == iFileWidth)
					{
						fX = 0.0f;
						iX = 0;
						fZ += fTerrainNewGap;
						iZ++;
					}

					// identity normal
					XMFLOAT3 normal = XMFLOAT3(0, 1, 0);

					// position
					fwrite(&pos.x, sizeof(float), 1, fp);
					fwrite(&pos.y, sizeof(float), 1, fp);
					fwrite(&pos.z, sizeof(float), 1, fp);

					// normals
					fwrite(&normal.x, sizeof(float), 1, fp);
					fwrite(&normal.y, sizeof(float), 1, fp);
					fwrite(&normal.z, sizeof(float), 1, fp);
				}

				// file complete
				fclose(fp);
			}
		}
	}
#endif

	// save grass map into terrain node files
	t.tfileveggrass_s = "vegmaskgrass.dat";

	if (FileExist(t.tfileveggrass_s.Get()) == 1)
	{
		OpenToRead(3, t.tfileveggrass_s.Get());
		if (MemblockExist(t.terrain.grassmemblock)) DeleteMemblock(t.terrain.grassmemblock);
		ReadMemblock(3, t.terrain.grassmemblock);
		CloseFile(3);
	}
	else
	{
		if (MemblockExist(t.terrain.grassmemblock) == 0)
		{
			MakeMemblock(t.terrain.grassmemblock, 4 + 4 + 4 + ((MAXTEXTURESIZE * MAXTEXTURESIZE) * 4));
		}
		WriteMemblockDWord(t.terrain.grassmemblock, 0, MAXTEXTURESIZE);
		WriteMemblockDWord(t.terrain.grassmemblock, 4, MAXTEXTURESIZE);
		WriteMemblockDWord(t.terrain.grassmemblock, 8, 32);
		t.tPindex = 4 + 4 + 4;
		for (t.tP = 0; t.tP <= MAXTEXTURESIZE * MAXTEXTURESIZE - 1; t.tP++)
		{
			WriteMemblockByte(t.terrain.grassmemblock, t.tPindex + 2, 0);
			t.tPindex += 4;
		}
	}


	//PE: Restore grass Data.
	uint32_t grass_data_size = GGGrass::GGGrass_GetDataSize();
	char* data = new char[grass_data_size];
	memset(data, 0, grass_data_size);

	if (data)
	{
		t.tPindex = 4 + 4 + 4;
		int iGrassMemblockThreshhold = 74; // Old classic grass not rendered below this value
		int mi = 0;
		int iScale = 2;
		for (int z = 0; z < 2048; z++)
		{
			for (int x = 0; x < 2048; x++)
			{
				float scale = 1.0;
				int offset = 1024;

				if (x > 0 && x < 2047 && z > 0 && z < 2047)
				{

					uint32_t newindex = ((offset + (z * scale + 0))) * 4096 + (offset + (x * scale + 0));
					if (newindex > 0 && newindex < grass_data_size)
					{
						if (ReadMemblockByte(t.terrain.grassmemblock, t.tPindex + mi + 2) >= iGrassMemblockThreshhold)
						{
							data[newindex] = 2; // realIndex = selected+2
							if (z > 0 && x > 0 && z < 2047 && x < 2047)
							{
								data[newindex + 1] = 2;
								data[newindex - 1] = 2;
								newindex = ((offset + (z * scale + 1))) * 4096 + (offset + (x * scale + 0));
								if (newindex > 0 && newindex < grass_data_size)
									data[newindex] = 2;
								newindex = ((offset + (z * scale - 1))) * 4096 + (offset + (x * scale + 0));
								if (newindex > 0 && newindex < grass_data_size)
									data[newindex] = 2;
							}

						}
						else
						{
							data[newindex] = 0;
						}
					}
				}

				mi += 4;
			}
		}
		GGGrass::GGGrass_SetData(grass_data_size, (uint8_t*)data);
		delete(data);
		gggrass_global_params.draw_enabled = true;
		t.showeditorveg = true;
		t.gamevisuals.bEndableGrassDrawing = t.visuals.bEndableGrassDrawing = true;

	}
	if (MemblockExist(t.terrain.grassmemblock)) DeleteMemblock(t.terrain.grassmemblock);

	// and delete old grass file from testmap
	t.tfileveggrass_s = "vegmaskgrass.dat";
	if (FileExist(t.tfileveggrass_s.Get()) == 1) DeleteFileA(t.tfileveggrass_s.Get());

	if (FileExist("vegmask.dds") == 1)
	{
		uint32_t terrain_paint_size = GGTerrain::GGTerrain_GetPaintDataSize();
		data = new char[terrain_paint_size];
		memset(data, 0, terrain_paint_size);
		if (data)
		{

			image_setlegacyimageloading(true);
			LoadImage("vegmask.dds", t.terrain.imagestartindex + 2);
			image_setlegacyimageloading(false);
			if (ImageExist(t.terrain.imagestartindex + 2))
			{
				CreateMemblockFromImage(t.terrain.grassmemblock, t.terrain.imagestartindex + 2);
				if (MemblockExist(t.terrain.grassmemblock))
				{
					t.tPindex = 4 + 4 + 4;
					int mi = 0;
					int iScale = 2;
					for (int z = 0; z < 2048; z++)
					{
						for (int x = 0; x < 2048; x++)
						{
							float scale = 1.0;

							int green = ReadMemblockByte(t.terrain.grassmemblock, t.tPindex + mi + 1);

							if (x > 0 && x < 2047 && z > 0 && z < 2047)
							{

								int offset = 1024;
								uint32_t newindex = ((offset + (z * scale + 0))) * 4096 + (offset + (x * scale + 0));

								if (newindex > 0 && newindex < terrain_paint_size && green > 1)
								{
									data[newindex] = 6;
									
									if (z > 0 && x > 0 && z < 2047 && x < 2047)
									{
										data[newindex + 1] = 6;
										data[newindex - 1] = 6;
										newindex = ((offset + (z * scale + 1))) * 4096 + (offset + (x * scale + 0));
										if (newindex > 0 && newindex < terrain_paint_size)
										{
											data[newindex] = 6;
										}
										newindex = ((offset + (z * scale - 1))) * 4096 + (offset + (x * scale + 0));
										if (newindex > 0 && newindex < terrain_paint_size)
										{
											data[newindex] = 6;
										}
									}
									
								}
							}
							mi += 4;
						}
					}
				}
			}

			GGTerrain::GGTerrain_SetPaintData(terrain_paint_size, (uint8_t*)data);
			delete(data);
		}
	}

	// and finally leave node zero folder
	SetDir("..");

	// Restore folder
	SetDir(pOldDir.Get());

	// load all entity parents so we can scan the associated files
	int entidmaster = 0;
	std::vector<std::string> pEntBank;
	pEntBank.clear();
	cstr filename_s = t.levelmapptah_s+"map.ent";
	if ( FileExist(filename_s.Get()) == 1 )
	{
		OpenToRead(1, cstr(t.levelmapptah_s + "map.ent").Get());
		entidmaster = ReadLong(1);
		if (entidmaster > 0)
		{
			for ( int entid = 1; entid <= entidmaster; entid++)
			{
				std::string pEntName = ReadString(1);
				pEntBank.push_back(pEntName);
			}
		}
		CloseFile(1);
	}
	// scan all entities and copy over assets not present
	for ( int entlistindex = 0; entlistindex < pEntBank.size(); entlistindex++)
	{
		// for each entity parent, copy all associated files over (fpe, bmp, model, textures)
		LPSTR pEntityName = (LPSTR)pEntBank[entlistindex].c_str();
		if (pEntityName && strlen(pEntityName) > 0)
		{
			// src points to classic folder, dest points to new location in writable area
			char pSrcFile[MAX_PATH];
			char pDestFile[MAX_PATH];

			// find just the entity folder
			char pEntityFolder[MAX_PATH];
			strcpy(pEntityFolder, pEntityName);
			for (int n = strlen(pEntityFolder); n > 0; n--)
			{
				if (pEntityFolder[n] == '\\' || pEntityFolder[n] == '/')
				{
					pEntityFolder[n+1] = 0;
					break;
				}
			}

			// skip if write into certain protected folders
			if (stricmp(pEntityFolder, "_markers\\") == NULL)
				continue;

			// copy FPE
			strcpy(pSrcFile, pReconstructGameGuruFolder);
			strcat(pSrcFile, pEntityName);
			strcpy(pDestFile, "entitybank\\");
			strcat(pDestFile, pEntityName);
			GG_GetRealPath(pDestFile, 1);
			CopyFileA(pSrcFile, pDestFile, TRUE);

			// load in FPE to obtain fields
			char pModelFile[MAX_PATH];
			char pTexturedFile[MAX_PATH];
			strcpy(pModelFile, "");
			strcpy(pTexturedFile, "");
			bool bCanBeIgnored = false;
			if (!pestrcasestr(pEntityName, "\\"))
			{
				//PE: If this is a EBE structure it will have no '\' so check if we can ignore it without generating errors.
				if(!FileExist(pSrcFile))
					bCanBeIgnored = true;
			}

			//PE: Some models ruin wicked , ignore those.
			{
				if(pestrcasestr(pSrcFile,"leafy bush (dense)"))
					bCanBeIgnored = true;
			}

			if (!bCanBeIgnored)
			{
				OpenToRead(1, pSrcFile);
				while (FileEnd(1) == 0)
				{
					LPSTR pLine = ReadString(1);
					LPSTR pToken = "model";
					LPSTR pModelFileSearch = strstr(pLine, pToken);
					if (pModelFileSearch != NULL)
					{
						LPSTR pModelFileEqual = strstr(pModelFileSearch, "=");
						if (pModelFileEqual != NULL)
						{
							pModelFileEqual++;
							while (*pModelFileEqual == 32 || *pModelFileEqual == 9) pModelFileEqual++;
							strcpy(pModelFile, pModelFileEqual);
						}
					}
					pToken = "textured";
					LPSTR pTexturedFileSearch = strstr(pLine, pToken);
					if (pTexturedFileSearch != NULL)
					{
						LPSTR pTexturedFileEqual = strstr(pTexturedFileSearch, "=");
						if (pTexturedFileEqual != NULL)
						{
							pTexturedFileEqual++;
							while (*pTexturedFileEqual == 32 || *pTexturedFileEqual == 9) pTexturedFileEqual++;
							strcpy(pTexturedFile, pTexturedFileEqual);
						}
					}
				}
				CloseFile(1);

				// copy BMP
				char pBMP[MAX_PATH];
				strcpy(pBMP, pEntityName);
				pBMP[strlen(pBMP) - 4] = 0;
				strcat(pBMP, ".bmp");
				strcpy(pSrcFile, pReconstructGameGuruFolder);
				strcat(pSrcFile, pBMP);
				strcpy(pDestFile, "entitybank\\");
				strcat(pDestFile, pBMP);
				GG_GetRealPath(pDestFile, 1);
				CopyFileA(pSrcFile, pDestFile, TRUE);
			}

			// model and textured can point to gamecore
			char pGameCoreAsset[MAX_PATH];
			LPSTR pGameCorePrefix = "gamecore\\";
			strcpy(pGameCoreAsset, "");

			// if model is from gamecore, copy those assets instead
			if (strlen(pModelFile) > 0)
			{
				if (strnicmp(pModelFile, pGameCorePrefix, strlen(pGameCorePrefix)) == NULL)
				{
					// copy gamecore asset 
					strcpy(pGameCoreAsset, pModelFile+strlen(pGameCorePrefix));
				}
				else
				{
					// model stored in FPE
					strcpy(pSrcFile, pReconstructGameGuruFolder);
					strcat(pSrcFile, pEntityFolder);
					strcat(pSrcFile, pModelFile);
					strcpy(pDestFile, "entitybank\\");
					strcat(pDestFile, pEntityFolder);
					strcat(pDestFile, pModelFile);
					GG_GetRealPath(pDestFile, 1);
					CopyFileA(pSrcFile, pDestFile, TRUE);
					//PE: If .x also copy .dbo
					if (pSrcFile[strlen(pSrcFile) - 1] == 'x' || pSrcFile[strlen(pSrcFile) - 1] == 'X')
					{
						if (pSrcFile[strlen(pSrcFile) - 2] == '.')
						{
							pSrcFile[strlen(pSrcFile) - 2] = 0;
							strcat(pSrcFile, ".dbo");
							pDestFile[strlen(pDestFile) - 2] = 0;
							strcat(pDestFile, ".dbo");
							CopyFileA(pSrcFile, pDestFile, TRUE);
						}
					}
				}
			}

			// texture refs stored in FPE
			if (strlen(pTexturedFile) > 0)
			{
				// if model is from gamecore, copy those assets instead
				if (strnicmp(pTexturedFile, pGameCorePrefix, strlen(pGameCorePrefix))== NULL)
				{
					// copy gamecore asset 
					strcpy(pGameCoreAsset, pTexturedFile+strlen(pGameCorePrefix));
				}
				else
				{
					// single texture
					strcpy(pSrcFile, pReconstructGameGuruFolder);
					strcat(pSrcFile, pEntityFolder);
					strcat(pSrcFile, pTexturedFile);
					strcpy(pDestFile, "entitybank\\");
					strcat(pDestFile, pEntityFolder);
					strcat(pDestFile, pTexturedFile);
					GG_GetRealPath(pDestFile, 1);
					CopyFileA(pSrcFile, pDestFile, TRUE);

					// if a PBR texture name, expand to other known formats
					char pExt[MAX_PATH];
					char pBaseName[MAX_PATH];
					strcpy(pBaseName, pTexturedFile);
					if (strlen(pBaseName) > 4)
					{
						// retain extension 
						strcpy(pExt, pBaseName + strlen(pBaseName) - 4);

						// find base filename
						bool bFBF = false;
						pBaseName[strlen(pBaseName) - 4] = 0;
						LPSTR pFM = "_color"; if (strnicmp(pBaseName + strlen(pBaseName) - strlen(pFM), pFM, strlen(pFM)) == NULL) { pBaseName[strlen(pBaseName) - strlen(pFM)] = 0; bFBF = true; }
						pFM = "_D"; if (strnicmp(pBaseName + strlen(pBaseName) - strlen(pFM), pFM, strlen(pFM)) == NULL) { pBaseName[strlen(pBaseName) - strlen(pFM)] = 0; bFBF = true; }
						if (bFBF == true)
						{
							// go through all possible PBR extras
							for (int iPBR = 0; iPBR < 9; iPBR++)
							{
								LPSTR pType = "";
								if (iPBR == 0) pType = "_normal";
								if (iPBR == 1) pType = "_metalness";
								if (iPBR == 2) pType = "_gloss";
								if (iPBR == 3) pType = "_ao";
								if (iPBR == 4) pType = "_n";
								if (iPBR == 5) pType = "_s";
								if (iPBR == 6) pType = "_i";
								if (iPBR == 7) pType = "_d2";
								if (iPBR == 8) pType = "_illum";

								// attempt to copy this type
								strcpy(pSrcFile, pReconstructGameGuruFolder);
								strcat(pSrcFile, pEntityFolder);
								strcat(pSrcFile, pBaseName);
								strcat(pSrcFile, pType);
								strcat(pSrcFile, pExt);
								strcpy(pDestFile, "entitybank\\");
								strcat(pDestFile, pEntityFolder);
								strcat(pDestFile, pBaseName);
								strcat(pDestFile, pType);
								strcat(pDestFile, pExt);
								GG_GetRealPath(pDestFile, 1);
								CopyFileA(pSrcFile, pDestFile, TRUE);
							}
						}
						else
						{
							// just the texture name then
							strcpy(pSrcFile, pReconstructGameGuruFolder);
							strcat(pSrcFile, pEntityFolder);
							strcat(pSrcFile, pTexturedFile);
							strcpy(pDestFile, "entitybank\\");
							strcat(pDestFile, pEntityFolder);
							strcat(pDestFile, pTexturedFile);
							GG_GetRealPath(pDestFile, 1);
							CopyFileA(pSrcFile, pDestFile, TRUE);
						}
					}
				}
			}
			else
			{
				// multi-textured model - load in object and interogate
			}

			// if gamecore asset found, copy all of it
			if (strlen(pGameCoreAsset) > 0)
			{
				// find just the gamecore folder
				char pGameCoreFolder[MAX_PATH];
				strcpy(pGameCoreFolder, pGameCoreAsset);
				for (int n = strlen(pGameCoreFolder); n > 0; n--)
				{
					if (pGameCoreFolder[n] == '\\' || pGameCoreFolder[n] == '/')
					{
						pGameCoreFolder[n+1] = 0;
						break;
					}
				}

				// enter gamecore folder, and copy all files over
				char pSrcFolder[MAX_PATH];
				strcpy(pSrcFolder, pReconstructGameGuruRootFiles);
				strcat(pSrcFolder, "gamecore\\");
				strcat(pSrcFolder, pGameCoreFolder);
				SetDir(pSrcFolder);
				ChecklistForFiles();
				SetDir(pOldDir.Get());
				for (int c = 1; c <= ChecklistQuantity(); c++)
				{
					LPSTR pFileName = ChecklistString(c);
					if (strcmp(pFileName, ".") != NULL && strcmp(pFileName, "..") != NULL)
					{
						strcpy(pSrcFile, pSrcFolder);
						strcat(pSrcFile, pFileName);
						strcpy(pDestFile, "gamecore\\");
						strcat(pDestFile, pGameCoreFolder);
						strcat(pDestFile, pFileName);
						if (!FileExist(pDestFile))
						{
							GG_GetRealPath(pDestFile, 1);
							CopyFileA(pSrcFile, pDestFile, TRUE);
						}
					}
				}
			}
		}
	}

	#ifndef FULLTERRAINEDITING
	// free usages
	if (fSrcHeightData)
	{
		delete fSrcHeightData;
		fSrcHeightData = NULL;
	}
	#endif

}

void AddWPETextures(char* filename)
{
	char cPE[MAX_PATH];
	strcpy(cPE, filename);
	char* find = (char*)pestrcasestr(cPE, ".pe");
	if (find)
	{
		*find = '\0';
		char finalname[MAX_PATH];
		strcpy(finalname, cPE);
		strcat(finalname, "0_color.png");
		addtocollection(finalname);
		strcpy(finalname, cPE);
		strcat(finalname, "1_color.png");
		addtocollection(finalname);
		strcpy(finalname, cPE);
		strcat(finalname, "2_color.png");
		addtocollection(finalname);
		strcpy(finalname, cPE);
		strcat(finalname, "3_color.png");
		addtocollection(finalname);
		strcpy(finalname, cPE);
		strcat(finalname, "4_color.png");
		addtocollection(finalname);
		strcpy(finalname, cPE);
		strcat(finalname, "5_color.png");
		addtocollection(finalname);
		strcpy(finalname, cPE);
		strcat(finalname, "6_color.png");
		addtocollection(finalname);
	}
	else
	{
		char* find = (char*)pestrcasestr(cPE, ".arx");
		if (find)
		{
			*find = '\0';
			char finalname[MAX_PATH];
			strcpy(finalname, cPE);
			strcat(finalname, "_g.png");
			addtocollection(finalname);
			strcpy(finalname, cPE);
			strcat(finalname, "_r.png");
			addtocollection(finalname);
			strcpy(finalname, cPE);
			strcat(finalname, "_s1.png");
			addtocollection(finalname);
			strcpy(finalname, cPE);
			strcat(finalname, "_sx.png");
			addtocollection(finalname);
		}
	}
}
