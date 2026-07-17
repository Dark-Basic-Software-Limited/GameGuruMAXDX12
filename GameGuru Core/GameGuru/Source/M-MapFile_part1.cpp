void addthisentityprofilesfilestocollection (entityeleproftype* pEleProf)
{
	// takes t.entid and t.e/pEleProf
	if ( t.entid >= t.entityprofile.size() ) return;
	
	// Ensure we also collect any textures for Building Editor entities - they are not included with the export
	entityprofiletype& entProfile = t.entityprofile[t.entid];
	if (strstr(entProfile.model_s.Get(), "smartchild"))
	{
		WickedMaterial& material = entProfile.WEMaterial;
		for (int i = 0; i < MAXMESHMATERIALS; i++)
		{
			if (material.baseColorMapName[i].Len() > 0)
			{
				addtocollection(material.baseColorMapName[i].Get());
				addtocollection(material.normalMapName[i].Get());
				addtocollection(material.emissiveMapName[i].Get());
				addtocollection(material.surfaceMapName[i].Get());
				addtocollection(material.displacementMapName[i].Get());
			}
		}
	}

	// entity profile file
	t.tentityname1_s = cstr("entitybank\\") + t.entitybank_s[t.entid];
	t.tentityname2_s = cstr(Left(t.tentityname1_s.Get(), Len(t.tentityname1_s.Get()) - 4)) + ".bin";
	if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + t.tentityname2_s).Get()) == 1)
	{
		t.tentityname_s = t.tentityname2_s;
	}
	else
	{
		t.tentityname_s = t.tentityname1_s;
	}
	addtocollection(t.tentityname_s.Get());

	//PE: NEWLOD
	std::string lodname = t.tentityname_s.Get();
	replaceAll(lodname, ".fpe", "_lod.dbo");
	replaceAll(lodname, ".bin", "_lod.dbo");
	addtocollection((char*)lodname.c_str());

	// and the BULLET file if exist (caused slowdown of standalone level loads!)
	std::string bulletname = t.tentityname_s.Get();
	replaceAll(bulletname, ".fpe", ".bullet");
	addtocollection((char*)bulletname.c_str());

	// entity files in folder
	t.tentityfolder_s = t.tentityname_s;
	for (t.n = Len(t.tentityname_s.Get()); t.n >= 1; t.n += -1)
	{
		if (cstr(Mid(t.tentityname_s.Get(), t.n)) == "\\" || cstr(Mid(t.tentityname_s.Get(), t.n)) == "/")
		{
			t.tentityfolder_s = Left(t.tentityfolder_s.Get(), t.n);
			break;
		}
	}

	//  model files (main model, final appended model and all other append
	int iModelAppendFileCount = t.entityprofile[t.entid].appendanimmax;
	if (Len (t.entityappendanim[t.entid][0].filename.Get()) > 0) iModelAppendFileCount = 0;
	for (int iModels = -1; iModels <= iModelAppendFileCount; iModels++)
	{
		LPSTR pModelFile = "";
		if (iModels == -1)
		{
			pModelFile = t.entityprofile[t.entid].model_s.Get();
		}
		else
		{
			pModelFile = t.entityappendanim[t.entid][iModels].filename.Get();
		}
		t.tlocaltofpe = 1;
		for (t.n = 1; t.n <= Len(pModelFile); t.n++)
		{
			if (cstr(Mid(pModelFile, t.n)) == "\\" || cstr(Mid(pModelFile, t.n)) == "/")
			{
				t.tlocaltofpe = 0; break;
			}
		}
		if (t.tlocaltofpe == 1)
		{
			t.tfile1_s = t.tentityfolder_s + pModelFile;
		}
		else
		{
			t.tfile1_s = pModelFile;
		}

		t.tfile2_s = cstr(Left(t.tfile1_s.Get(), Len(t.tfile1_s.Get()) - 2)) + ".dbo";
		if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + t.tfile2_s).Get()) == 1)
		{
			t.tfile_s = t.tfile2_s;
		}
		else
		{
			t.tfile_s = t.tfile1_s;
		}
		t.tmodelfile_s = t.tfile_s;
		addtocollection(t.tmodelfile_s.Get());

		//PE: CCP have missing textures the body part, i seen entrys like 'baseColorMap0    = tempfinalalbedo0.dds' , so always copy over main texture.
		if (t.entityprofile[t.entid].ischaracter)
		{
			cstr ccpname = t.tmodelfile_s;
			std::string sParseName = t.tmodelfile_s.Get();
			std::string sParseNext = sParseName;
			replaceAll(sParseName, ".dbo", "0.dds"); //CCP missing main texture (body).
			addtocollection((char *)sParseName.c_str());
			sParseName = sParseNext;
			replaceAll(sParseName, ".dbo", "1.dds"); //PE: Legs with custom skin also need to be added.
			addtocollection((char *)sParseName.c_str());
			sParseName = sParseNext;
			replaceAll(sParseName, ".dbo", "2.dds"); //PE: Feets with custom skin also need to be added.
			addtocollection((char *)sParseName.c_str());
		}

		// if entity did not specify texture it is multi-texture, so interogate model file
		findalltexturesinmodelfile(t.tmodelfile_s.Get(), t.tentityfolder_s.Get(), t.entityprofile[t.entid].texpath_s.Get());
	}

	// Export entity FPE BMP file if flagged
	if (g.gexportassets == 1)
	{
		t.tfile3_s = cstr(Left(t.tentityname_s.Get(), Len(t.tentityname_s.Get()) - 4)) + ".bmp";
		if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + t.tfile3_s).Get()) == 1)
		{
			addtocollection(t.tfile3_s.Get());
		}
	}

	// entity characterpose file (if any)
	t.tfile3_s = cstr(Left(t.tfile1_s.Get(), Len(t.tfile1_s.Get()) - 2)) + ".dat";
	if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + t.tfile3_s).Get()) == 1)
	{
		addtocollection(t.tfile3_s.Get());
	}

	// bullet physics hull decomp file (if any)
	t.tfile3_s = cstr(Left(t.tentityname_s.Get(), Len(t.tentityname_s.Get()) - 4)) + ".bullet";
	if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + t.tfile3_s).Get()) == 1)
	{
		addtocollection(t.tfile3_s.Get());
	}

	// texture files
	int iStartingType = 0;
	if (t.e == 0) iStartingType = 1; // parent profile only
	if (pEleProf) iStartingType = 0; // comes from standalone exporter, using pEleProf as alternative to t.e
	for (int iBothTypes = iStartingType; iBothTypes < 2; iBothTypes++)
	{
		// can be from ELEPROF of entityelement (older maps point to old texture names) or parent ELEPROF original
		cstr pTextureFile = "", pAltTextureFile = "";
		if (iBothTypes == 0)
		{ 
			if (pEleProf)
			{
				pTextureFile = pEleProf->texd_s; pAltTextureFile = pEleProf->texaltd_s;
			}
			else
			{
				pTextureFile = t.entityelement[t.e].eleprof.texd_s; pAltTextureFile = t.entityelement[t.e].eleprof.texaltd_s;
			}
		}
		if (iBothTypes == 1) 
		{ 
			pTextureFile = t.entityprofile[t.entid].texd_s; pAltTextureFile = t.entityprofile[t.entid].texaltd_s; 
		}
		t.tlocaltofpe = 1;
		for (t.n = 1; t.n <= Len(pTextureFile.Get()); t.n++)
		{
			if (cstr(Mid(pTextureFile.Get(), t.n)) == "\\" || cstr(Mid(pTextureFile.Get(), t.n)) == "/")
			{
				t.tlocaltofpe = 0; break;
			}
		}
		if (t.tlocaltofpe == 1)
		{
			t.tfile_s = t.tentityfolder_s + pTextureFile;
		}
		else
		{
			t.tfile_s = pTextureFile;
		}
		addtocollection(t.tfile_s.Get());

		// always allow a DDS texture of same name to be copied over (for test game compatibility)
		for (int iTwoExtensions = 0; iTwoExtensions <= 1; iTwoExtensions++)
		{
			if (iTwoExtensions == 0) t.tfileext_s = Right (t.tfile_s.Get(), 3);
			if (iTwoExtensions == 1) t.tfileext_s = "dds";
			if (cstr(Left(Lower(Right(t.tfile_s.Get(), 6)), 2)) == "_d")
			{
				t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "_n." + t.tfileext_s; addtocollection(t.tfile_s.Get());
				t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "_s." + t.tfileext_s; addtocollection(t.tfile_s.Get());
				t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "_i." + t.tfileext_s; addtocollection(t.tfile_s.Get());
				t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "_o." + t.tfileext_s; addtocollection(t.tfile_s.Get());
				t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "_cube." + t.tfileext_s; addtocollection(t.tfile_s.Get());
			}
			int iNewPBRTextureMode = 0;
			if (cstr(Left(Lower(Right(t.tfile_s.Get(), 10)), 6)) == "_color") iNewPBRTextureMode = 6 + 4;
			if (cstr(Left(Lower(Right(t.tfile_s.Get(), 11)), 7)) == "_albedo") iNewPBRTextureMode = 7 + 4;
			if (iNewPBRTextureMode > 0)
			{
				cstr pToAdd;
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_color." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_albedo." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_normal." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_specular." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_metalness." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_gloss." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_mask." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_ao." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_height." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_detail." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_surface." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_emissive." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_illumination." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_illum." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_i." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_cube." + t.tfileext_s; addtocollection(pToAdd.Get());
			}
		}
		if (t.tlocaltofpe == 1)
		{
			t.tfile_s = t.tentityfolder_s + pAltTextureFile;
		}
		else
		{
			t.tfile_s = pAltTextureFile;
		}
		addtocollection(t.tfile_s.Get());
	}

	// also include textures specified by textureref entries
	// and assemblyccp from new character creator plus
	cstr tFPEFilePath = g.fpscrootdir_s + "\\Files\\";
	tFPEFilePath += t.tentityname1_s;
	FILE* tFPEFile = GG_fopen (tFPEFilePath.Get(), "r");
	if (tFPEFile)
	{
		char tTempLine[2048];
		while (!feof(tFPEFile))
		{
			fgets (tTempLine, 2047, tFPEFile);
			//PE: We need to add all custom mesh textures.
			if (strstr(tTempLine, "baseColorMap"))
			{
				char* pToFilename = strstr(tTempLine, "=");
				if (pToFilename)
				{
					while (*pToFilename == '=' || *pToFilename == 32) pToFilename++;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					//PE: CCP is special as we dont have to add entitybank...
					if (pestrcasestr(pToFilename, "charactercreatorplus\\parts\\"))
					{
						addtocollection(pToFilename);
					}
					else
					{
						cstr tTextureFile = cstr(t.tentityfolder_s + cstr(pToFilename));
						addtocollection(tTextureFile.Get());
					}
				}
			}
			if (strstr(tTempLine, "normalMap"))
			{
				char* pToFilename = strstr(tTempLine, "=");
				if (pToFilename)
				{
					while (*pToFilename == '=' || *pToFilename == 32) pToFilename++;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					//PE: CCP is special as we dont have to add entitybank...
					if (pestrcasestr(pToFilename, "charactercreatorplus\\parts\\"))
					{
						addtocollection(pToFilename);
					}
					else
					{
						cstr tTextureFile = cstr(t.tentityfolder_s + cstr(pToFilename));
						addtocollection(tTextureFile.Get());
					}
				}
			}
			if (strstr(tTempLine, "surfaceMap"))
			{
				char* pToFilename = strstr(tTempLine, "=");
				if (pToFilename)
				{
					while (*pToFilename == '=' || *pToFilename == 32) pToFilename++;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					//PE: CCP is special as we dont have to add entitybank...
					if (pestrcasestr(pToFilename, "charactercreatorplus\\parts\\"))
					{
						addtocollection(pToFilename);
					}
					else
					{
						cstr tTextureFile = cstr(t.tentityfolder_s + cstr(pToFilename));
						addtocollection(tTextureFile.Get());
					}
				}
			}
			if (strstr (tTempLine, "textureref"))
			{
				char* pToFilename = strstr (tTempLine, "=");
				if (pToFilename)
				{
					while (*pToFilename == '=' || *pToFilename == 32) pToFilename++;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					cstr tTextureFile = cstr(t.tentityfolder_s + cstr(pToFilename));
					addtocollection (tTextureFile.Get());
				}
			}
			if (strstr (tTempLine, "ccpassembly"))
			{
				// LB: Copied this so I can work out textures from this field, but may be able to adapt 
				// this "CreateVectorListFromCPPAssembly" call to here as well.
				char* pAssemblyString = strstr (tTempLine, "=");
				if (pAssemblyString)
				{
					// get past equals and any spaces
					while (*pAssemblyString == '=' || *pAssemblyString == 32) pAssemblyString++;

					// now we have the assembly string; adult male hair 01,adult male head 01,adult male body 03,adult male legs 04e,adult male feet 04
					// delimited by a comma, and indicates which parts we used (to specify the textures to copy over)
					char pCustomPathToFolder[MAX_PATH];
					cstr assemblyString_s = FirstToken(pAssemblyString, ",");
					while (assemblyString_s.Len() > 0)
					{
						// work out texture files from this reference, i.e adultmalehair01
						char pAssemblyReference[1024];
						strcpy(pAssemblyReference, assemblyString_s.Get());
						if (pAssemblyReference[strlen(pAssemblyReference) - 1] == '\n') pAssemblyReference[strlen(pAssemblyReference) - 1] = 0;
						strlwr(pAssemblyReference);
						int iBaseCount = g_CharacterType.size();// 4; //(3) PE: Added zombie female
						for (int iBaseIndex = 0; iBaseIndex < iBaseCount; iBaseIndex++)
						{
							LPSTR pBaseName = "";
							if (iBaseIndex == 0) pBaseName = "adult male";
							if (iBaseIndex == 1) pBaseName = "adult female";
							if (iBaseIndex == 2) pBaseName = "zombie male";
							if (iBaseIndex == 3) pBaseName = "zombie female";
							if (iBaseIndex > 3) pBaseName = g_CharacterType[iBaseIndex].pPartsFolder;
							if (strstr(pAssemblyReference, pBaseName) != NULL)
							{
								// found category
								cstr pPartFolder = "";
								if (iBaseIndex == 0) pPartFolder = "charactercreatorplus\\parts\\adult male\\";
								if (iBaseIndex == 1) pPartFolder = "charactercreatorplus\\parts\\adult female\\";
								if (iBaseIndex == 2) pPartFolder = "charactercreatorplus\\parts\\zombie male\\";
								if (iBaseIndex == 3) pPartFolder = "charactercreatorplus\\parts\\zombie female\\";
								if (iBaseIndex > 3)
								{
									sprintf(pCustomPathToFolder, "charactercreatorplus\\parts\\%s\\", g_CharacterType[iBaseIndex].pPartsFolder);
									pPartFolder = pCustomPathToFolder;
								}

								// add final texture files
								cstr pTmpFile = pPartFolder + pAssemblyReference;
								char pRemoveTag[MAX_PATH];
								strcpy(pRemoveTag, pTmpFile.Get());
								for (int nnn = 0; nnn < strlen(pRemoveTag); nnn++)
								{
									if (pRemoveTag[nnn] == '[')
									{
										if (pRemoveTag[nnn - 1] == ' ') nnn--;
										pRemoveTag[nnn] = 0;
										break;
									}
								}

								// need to strip out the tag [xxx] part to find texture proper
								pTmpFile = pRemoveTag;
								addtocollection (cstr(pTmpFile + "_ao.dds").Get());
								addtocollection (cstr(pTmpFile + "_color.dds").Get());
								addtocollection (cstr(pTmpFile + "_gloss.dds").Get());
								addtocollection (cstr(pTmpFile + "_mask.dds").Get());
								addtocollection (cstr(pTmpFile + "_metalness.dds").Get());
								addtocollection(cstr(pTmpFile + "_surface.dds").Get());
								addtocollection (cstr(pTmpFile + "_normal.dds").Get());
							}
						}
						assemblyString_s = NextToken(",");
					}
				}
			}
		}
		fclose (tFPEFile);
	}
}

// we set this during level loading, then action it near the end
bool g_bUsingSomeKindOfTerrain = false;

void mapfile_collectfoldersandfiles (cstr levelpathfolder)
{
	cstr pOldDir = GetDir();

	// Collect ALL files in string array list
	Undim (t.filecollection_s);
	g.filecollectionmax = 0;
	Dim (t.filecollection_s, 500);

	//  Stage 1 - specify all common files
	addtocollection(cstr(cstr("languagebank\\") + g.language_s + "\\textfiles\\guru-wordcount.ini").Get());
	addtocollection(cstr(cstr("languagebank\\") + g.language_s + "\\textfiles\\guru-words.txt").Get());
	addtocollection(cstr(cstr("languagebank\\") + g.language_s + "\\inittext.ssp").Get());
	addtocollection("audiobank\\misc\\silence.wav");
	addtocollection("audiobank\\misc\\explode.wav");
	addtocollection("audiobank\\misc\\ammo.wav");
	addtocollection("audiobank\\misc\\Bullet_FlyBy_01.wav");
	addtocollection("audiobank\\misc\\Bullet_FlyBy_02.wav");
	addtocollection("audiobank\\misc\\Bullet_FlyBy_03.wav");
	addtocollection("audiobank\\misc\\Bullet_FlyBy_04.wav");
	addtocollection("audiobank\\misc\\melee.wav");
	addtocollection("audiobank\\misc\\melee1.wav");
	addtocollection("audiobank\\misc\\melee2.wav");
	addtocollection("audiobank\\misc\\melee3.wav");
	addtocollection("audiobank\\misc\\melee4.wav");
	addtocollection("audiobank\\misc\\melee5.wav");
	addtocollection("audiobank\\misc\\melee6.wav");
	addtocollection("editors\\gfx\\14.png");
	addtocollection("editors\\gfx\\14-white.png");
	addtocollection("editors\\gfx\\14-red.png");
	addtocollection("editors\\gfx\\14-green.png");
	addtocollection("editors\\gfx\\dummy.png");
	addtocollection("editors\\gfx\\notexture.dds");
	addtocollection("editors\\keymap\\default.ini");
	addtocollection("editors\\keymap\\weaponslots.dat");
	addtocollection("editors\\templates\\ScreenEditor\\project.dat");
	addtocollection("scriptbank\\gameloop.lua");
	addtocollection("scriptbank\\gameplayercontrol.lua");
	addtocollection("scriptbank\\gameplayerhealth.lua");
	addtocollection("scriptbank\\global.lua");

	addfoldertocollection(cstr(cstr("languagebank\\") + g.language_s + "\\artwork\\watermark").Get());
	addfoldertocollection("scriptbank\\people\\ai");
	addtocollection("scriptbank\\people\\patrol.byc");
	addtocollection("scriptbank\\people\\patrol.lua");
	addfoldertocollection("scriptbank\\ai");
	addfoldertocollection("scriptbank\\images");
	addfoldertocollection("audiobank\\materials");
	addfoldertocollection("audiobank\\user");

	addtocollection("scriptbank\\perlin_noise.lua");
	addtocollection("scriptbank\\hud0.lua");
	addtocollection("scriptbank\\utillib.lua"); //PE: hud0 use  utillib.lua
	addtocollection("scriptbank\\huds\\cursorcontrol.lua");
	addtocollection("scriptbank\\gameplayerhealth.lua");
	addtocollection("scriptbank\\gameplayerspeed.lua");
	addtocollection("scriptbank\\huds\\cursorcontrol.lua");

	// not all cineguru scripts/associated files cover over, ensure they do
	addfoldertocollection("scriptbank\\Cine Guru MAX");
	addfoldertocollection("scriptbank\\user\\actors");

	//PE: Missing foot step material sounds
	addfoldertocollection("audiobank\\materials\\dirt");
	addfoldertocollection("audiobank\\materials\\grass");
	addfoldertocollection("audiobank\\materials\\grass\\extras");
	addfoldertocollection("audiobank\\materials\\gravel");
	addfoldertocollection("audiobank\\materials\\metal");
	addfoldertocollection("audiobank\\materials\\puddle");
	addfoldertocollection("audiobank\\materials\\sand");
	addfoldertocollection("audiobank\\materials\\snow");
	addfoldertocollection("audiobank\\materials\\tarmac");
	addfoldertocollection("audiobank\\materials\\underwater");
	addfoldertocollection("audiobank\\materials\\wood");

	addfoldertocollection("audiobank\\music\\theescape");
	addfoldertocollection("audiobank\\voices\\player");
	addfoldertocollection("audiobank\\voices\\characters");
	addfoldertocollection("audiobank\\character\\soldier\\onAggro");
	addfoldertocollection("audiobank\\character\\soldier\\onAlert");
	addfoldertocollection("audiobank\\character\\soldier\\onDeath");
	addfoldertocollection("audiobank\\character\\soldier\\onHurt");
	addfoldertocollection("audiobank\\character\\soldier\\onHurtPlayer");
	addfoldertocollection("audiobank\\character\\soldier\\onIdle");
	addfoldertocollection("audiobank\\character\\soldier\\onInteract");
	addfoldertocollection("databank");
	addallinfoldertocollection("titlesbank", "titlesbank"); // need the ENTIRE contents - now includes the root files not just the folders!
	addfoldertocollection("effectbank\\reloaded");
	addfoldertocollection("effectbank\\reloaded\\media");
	addfoldertocollection("effectbank\\reloaded\\media\\materials");
	addfoldertocollection("effectbank\\explosion");
	addfoldertocollection("effectbank\\particles");
	addfoldertocollection("effectbank\\particles\\weather");
	addfoldertocollection("lensflares");
	addfoldertocollection("fontbank");
	addfoldertocollection("languagebank\\neutral\\gamecore\\huds\\ammohealth");
	addfoldertocollection("languagebank\\neutral\\gamecore\\huds\\panels");

	addfoldertocollection("gamecore\\decals\\blood"); //PE: New particle effects.
	addfoldertocollection("gamecore\\decals\\explosion"); //PE: New particle effects.
	//PE: New added effects.
	addfoldertocollection("gamecore\\decals\\explosion huge");
	addfoldertocollection("gamecore\\decals\\explosion large");
	addfoldertocollection("gamecore\\decals\\explosion medium");
	addfoldertocollection("gamecore\\decals\\explosion small");
	addfoldertocollection("gamecore\\decals\\explosion_blood");
	addfoldertocollection("gamecore\\decals\\splat");
	addfoldertocollection("gamecore\\decals\\bloodsplat");
	addfoldertocollection("gamecore\\decals\\impact");
	addfoldertocollection("gamecore\\decals\\gunsmoke");
	addfoldertocollection("gamecore\\decals\\smoke1");
	addfoldertocollection("gamecore\\decals\\muzzleflash4");
	addfoldertocollection("gamecore\\decals\\splash_droplets");
	addfoldertocollection("gamecore\\decals\\splash_foam");
	addfoldertocollection("gamecore\\decals\\splash_large");
	addfoldertocollection("gamecore\\decals\\splash_misty");
	addfoldertocollection("gamecore\\decals\\splash_ripple");
	addfoldertocollection("gamecore\\decals\\splash_small");
	addfoldertocollection("gamecore\\decals\\splinters");
	addfoldertocollection("gamecore\\decals\\sparks");
	addfoldertocollection("gamecore\\decals\\dust");


	addfoldertocollection("gamecore\\vrcontroller");
	addfoldertocollection("gamecore\\vrcontroller\\oculus");
	addfoldertocollection("gamecore\\projectiletypes");
	addfoldertocollection("gamecore\\projectiletypes\\common\\explode");
	addfoldertocollection("gamecore\\projectiletypes\\enhanced\\m67");
	addfoldertocollection("gamecore\\bulletholes");
	addfoldertocollection("editors\\lut");

	// folders related to terrain system
	g_bUsingSomeKindOfTerrain = false;

	//PE: Still need old effects. arx
	addfoldertocollection("particlesbank");

	addtocollection("effectbank\\common\\noise64.png");
	addtocollection("effectbank\\common\\dist2.png");
	addfoldertocollection("effectbank\\common"); //Just in case we get more.

	addtocollection("pinetree_high_color_1024.dds");
	addtocollection("pinetree.dds");
	addtocollection("noise.dds");

	addtocollection("skybank\\clear\\"); //for fallback.

	// add any material decals that are active
	for (t.m = 0; t.m <= g.gmaterialmax; t.m++)
	{
		if (t.material[t.m].usedinlevel == 1)
		{
			cstr decalFolder_s = cstr("gamecore\\decals\\") + t.material[t.m].decal_s;
			addfoldertocollection(decalFolder_s.Get());
		}
	}

	addfoldertocollection("gamecore\\muzzleflash");
	addfoldertocollection("gamecore\\projectiletypes");

	// we will much improve this with the new project system!!
	addfoldertocollection("gamecore\\hands\\Animations");
	addallinfoldertocollection("gamecore\\guns\\interactive", "gamecore\\guns\\interactive");
	//PE: We now have lua script that use this directly, so add by default for now.
	addfoldertocollection("gamecore\\guns\\enhanced\\Gloves_Unarmed");
	//PE: Need to read hudcustom.txt from all used guns folders, for now.
	addfoldertocollection("gamecore\\hands\\Male Light");
	addfoldertocollection("gamecore\\hands\\Male Dark");
	addfoldertocollection("gamecore\\hands\\Female Light");
	addfoldertocollection("gamecore\\hands\\Female Dark");
	addfoldertocollection("gamecore\\hands\\Combat Gloves Light");
	addfoldertocollection("gamecore\\hands\\Combat Gloves Dark");
	addfoldertocollection("gamecore\\hands\\Low Poly");
	addfoldertocollection("gamecore\\hands\\Fantasy Gauntlets");
	addfoldertocollection("gamecore\\hands\\Fantasy Leather Gloves");

	//  Stage 1B - Style dependent files
	titles_getstyle ();
	addtocollection("titlesbank\\style.txt");
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\").Get());
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\1280x720").Get());
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\1280x800").Get());
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\1366x768").Get());
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\1440x900").Get());
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\1600x900").Get());
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\1680x1050").Get());
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\1920x1080").Get());
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\1920x1200").Get());

	// HUD elements
	addfoldertocollection("imagebank\\HUD");
	addfoldertocollection("imagebank\\HUD Library\\MAX");

	// include original FPM
	addtocollection(t.tmasterlevelfile_s.Get());

	//PE: Add .lst version , this is to be used for thread loading of level textures and objects.
	std::string lstfile = t.tmasterlevelfile_s.Get();
	replaceAll(lstfile, ".fpm", ".lst");
	addtocollection((char*)lstfile.c_str());

	// Pre-Stage 2 - clear a list which will collect all folders/files to REMOVE from the final standalone file transfer
	// list, courtesy of the special FPP file which controls the final files to be used for standalone creation
	// (eventually to be controlled from a nice UI)
	std::vector<cstr> fppFoldersToRemoveList;
	std::vector<cstr> fppFilesToRemoveList;
	fppFoldersToRemoveList.clear();
	fppFilesToRemoveList.clear();

	// Stage 2 - collect all files (from all levels)
	t.levelmax = 0;
	Dim (t.levellist_s, 100);
	for (int i = 0; i < 100; i++) t.levellist_s[i] = "";
	addtocollection(t.visuals.sAmbientMusicTrack.Get());
	addtocollection(t.visuals.sCombatMusicTrack.Get());
	t.levelindex = 1;

	//PE: Need adding images from g_collectionQuestList
	for (int n = 0; n < g_collectionQuestList.size(); n++)
	{
		if (g_collectionQuestList[n].collectionFields.size() > 2)
		{
			LPSTR pImageFile = g_collectionQuestList[n].collectionFields[2].Get();
			if (strlen(pImageFile) > 0)
			{
				if (stricmp(pImageFile, "default") != NULL && stricmp(pImageFile, "image") != NULL)
				{
					addtocollection(pImageFile);
				}
			}

		}
	}

	// Add images from collection list (can be stored in thumbbank)
	for (int n = 0; n < g_collectionList.size(); n++)
	{
		if (g_collectionList[n].collectionFields.size() > 2)
		{
			LPSTR pImageFile = g_collectionList[n].collectionFields[2].Get();
			if (strlen(pImageFile) > 0)
			{
				if (stricmp(pImageFile, "default") != NULL && stricmp(pImageFile, "image") != NULL)
				{
					addtocollection(pImageFile);
				}
			}
		}
	}

	//Add all storyboard files to scan list.
	if (g.bUseStoryBoardSetup)
	{
		addfoldertocollection("editors\\templates\\fonts");
		addtocollection("editors\\uiv3\\Roboto-Medium.ttf");

		// go through and add all FPMs to export
		char pIncludeMapFile[MAX_PATH];
		FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm);
		g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
		strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);
		int foundlevel = 1; // FindNextLevel(g_Storyboard_Current_Level, g_Storyboard_Current_fpm); thix would ignore the first level, instead assume first level is good level
		while (foundlevel == 1)
		{
			bool bAlreadyAdded = false;
			for (int n = 0; n < t.levelmax; n++)
			{
				if (stricmp(g_Storyboard_Current_fpm, t.levellist_s[n].Get()) == NULL)
				{
					bAlreadyAdded = true;
					break;
				}
			}
			if (bAlreadyAdded == false)
			{
				++t.levelmax;
				t.levellist_s[t.levelmax] = g_Storyboard_Current_fpm;
				addtocollection(g_Storyboard_Current_fpm);
				if (strlen(g_Storyboard_Current_fpm) > 5)
				{
					strcpy(pIncludeMapFile, g_Storyboard_Current_fpm);
					pIncludeMapFile[strlen(pIncludeMapFile) - 4] = 0;
					strcat(pIncludeMapFile, ".png");
					addtocollection(pIncludeMapFile);
				}
				foundlevel = FindNextLevel(g_Storyboard_Current_Level, g_Storyboard_Current_fpm);
			}
			else
			{
				// level recursed back on itself, end this loop!
				foundlevel = 0;
			}
		}

		// Now find any levels that are on the Storyboard, but have not been marked for collection (not connected to any screens - loaded from Winzone)
		for (int i = 0; i < STORYBOARD_MAXNODES; i++)
		{
			if (Storyboard.Nodes[i].used && Storyboard.Nodes[i].type == STORYBOARD_TYPE_LEVEL)
			{
				// get level name
				cStr levelName = Storyboard.Nodes[i].level_name;
				if (strlen(levelName.Get()) > 0)
				{
					// only valid if have a level name here
					// Check if this Storyboard level has already been marked for collection of its files
					bool bAlreadyCollected = false;
					for (int j = 1; j <= t.levelmax; j++)
					{
						if (strcmp(levelName.Get(), t.levellist_s[j].Get()) == 0)
						{
							bAlreadyCollected = true;
							break;
						}
					}
					if (!bAlreadyCollected)
					{
						// This level has not yet been marked for collection
						++t.levelmax;
						t.levellist_s[t.levelmax] = levelName;
						addtocollection(levelName.Get());
						if (levelName.Len() > 5)
						{
							strcpy(pIncludeMapFile, levelName.Get());
							pIncludeMapFile[strlen(pIncludeMapFile) - 4] = 0;
							strcat(pIncludeMapFile, ".png");
							addtocollection(pIncludeMapFile);
						}
					}
				}
			}
		}

		//Restore to first level.
		FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm);
		g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
		strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);

		//Use project name as exename
		if (strlen(Storyboard.gamename) > 0)
		{
			//Add project.
			char project[MAX_PATH], project_files[MAX_PATH];
			strcpy(project, "projectbank\\");
			strcat(project, Storyboard.gamename);
			addfoldertocollection(project);

			// add loading splash in case of a needed fallback
			addtocollection("editors\\uiv3\\loadingsplash.jpg");

			// add all media used by storyboard.
			for (int nodeid = 0; nodeid < STORYBOARD_MAXNODES; nodeid++)
			{
				if (Storyboard.Nodes[nodeid].used)
				{
					// include splashscreen if specified
					if (Storyboard.Nodes[nodeid].type == STORYBOARD_TYPE_SPLASH)
					{
						if (strlen(Storyboard.Nodes[nodeid].thumb) > 0)
						{
							addtocollection(Storyboard.Nodes[nodeid].thumb);
						}
					}

					if (strlen(Storyboard.Nodes[nodeid].screen_music) > 0)
					{
						addtocollection(Storyboard.Nodes[nodeid].screen_music);
					}
					if (strlen(Storyboard.Nodes[nodeid].screen_backdrop) > 0)
					{
						addtocollection(Storyboard.Nodes[nodeid].screen_backdrop);
					}

					for (int i = 0; i < STORYBOARD_MAXWIDGETS; i++)
					{
						if (Storyboard.Nodes[nodeid].widget_used[i] == 1)
						{
							if (strlen(Storyboard.Nodes[nodeid].widget_normal_thumb[i]) > 0)
							{
								addtocollection(Storyboard.Nodes[nodeid].widget_normal_thumb[i]);
							}
							if (strlen(Storyboard.Nodes[nodeid].widget_highlight_thumb[i]) > 0)
							{
								addtocollection(Storyboard.Nodes[nodeid].widget_highlight_thumb[i]);
							}
							if (strlen(Storyboard.Nodes[nodeid].widget_selected_thumb[i]) > 0)
							{
								addtocollection(Storyboard.Nodes[nodeid].widget_selected_thumb[i]);
							}
							//PE: We are going to reuse for widget_click_sound for default value for "Text Globals"
							if (Storyboard.Nodes[nodeid].widget_type[i] == STORYBOARD_WIDGET_BUTTON
								|| Storyboard.Nodes[nodeid].widget_type[i] == STORYBOARD_WIDGET_RADIOTYPE
								|| Storyboard.Nodes[nodeid].widget_type[i] == STORYBOARD_WIDGET_TICKBOX)
							{

								if (strlen(Storyboard.Nodes[nodeid].widget_click_sound[i]) > 0)
								{
									addtocollection(Storyboard.Nodes[nodeid].widget_click_sound[i]);
								}
							}
						}
					}
				}
			}
		}
	}

	// if remote project, add ALL files in remote project folder to the files to be copied over
	if ( strlen(Storyboard.customprojectfolder) > 0)
	{
		// add all files in the remote project folder
		char pAbsRemoteProjectFolderToScan[MAX_PATH];
		strcpy(pAbsRemoteProjectFolderToScan, Storyboard.customprojectfolder);
		strcat(pAbsRemoteProjectFolderToScan, Storyboard.gamename);
		strcat(pAbsRemoteProjectFolderToScan, "\\Files\\");
		addallinfoldertocollection(pAbsRemoteProjectFolderToScan, "");

		// and them remove any that are not needed in the standalone
		removeanymatchingfromcollection("mapbank\\_automatedbackups");
		removeanymatchingfromcollection("ebebank\\default");
		removeanymatchingfromcollection("levelbank\\testmap");
	}
}

void mapfile_savestandalone_start ( void )
{
	// first grab current folder for later restoring
	t.told_s=GetDir();

	// In Classic, I would run through the load process and collect files as they
	// where loaded in. In Reloaded, the currently loaded level data is scanned
	// to arrive at the required files for the Standalone EXE
	t.interactive.savestandaloneused=1;

	// this flag ensures the loadassets splash does not appear when making standalone
	t.levelsforstandalone = 1;

	// 040316 - v1.13b1 - find the nested folder structure of the level (could be in map bank\Easter\level1.fpm)
	t.told_s=GetDir();
	cstr mapbankpath;
	cstr levelpathfolder;
	if ( g.projectfilename_s.Get()[1] != ':' )
	{
		// relative project path
		g_mapfile_mapbankpath = cstr("mapbank\\");
		g_mapfile_levelpathfolder = Right ( g.projectfilename_s.Get(), strlen(g.projectfilename_s.Get()) - strlen(g_mapfile_mapbankpath.Get()) );
	}
	else
	{
		// absolute project path
		g_mapfile_mapbankpath = g.mysystem.mapbankAbs_s;
		g_mapfile_levelpathfolder = Right ( g.projectfilename_s.Get(), strlen(g.projectfilename_s.Get()) - strlen(g_mapfile_mapbankpath.Get()) );
	}

	bool bGotNestedPath = false;
	for ( int n = Len(g_mapfile_levelpathfolder.Get()) ; n >= 1 ; n+= -1 )
	{
		if ( cstr(Mid(g_mapfile_levelpathfolder.Get(),n)) == "\\" || cstr(Mid(g_mapfile_levelpathfolder.Get(),n)) == "/" ) 
		{
			g_mapfile_levelpathfolder = Left ( g_mapfile_levelpathfolder.Get(), n );
			bGotNestedPath = true;
			break;
		}
	}
	if ( bGotNestedPath==false )
	{
		// 240316 - V1.131v1 - if NO nested folder, string must be empty!
		g_mapfile_levelpathfolder = "";
	}

	//  Name without EXE
	t.exename_s=g.projectfilename_s;
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
	//PE: issue https://github.com/TheGameCreators/GameGuruRepo/issues/444
	if (  Len(t.exename_s.Get())<1  )  t.exename_s = "mylevel";

	//  the level to start off standalone export
	t.tmasterlevelfile_s=cstr("mapbank\\")+g_mapfile_levelpathfolder+t.exename_s+".fpm";

	if (g.bUseStoryBoardSetup)
	{
		//Use project name as exename
		if (strlen(Storyboard.gamename) > 0)
		{
			t.exename_s = Storyboard.gamename;
		}
	}

	timestampactivity(0,cstr(cstr("Saving standalone from ")+t.tmasterlevelfile_s).Get() );

	//  Create MYDOCS folder if not exist
	if ( PathExist(g.myownrootdir_s.Get()) == 0 ) file_createmydocsfolder ( );

	// Create absolute My Games folder (if not exist)
	if ( PathExist ( g.exedir_s.Get() ) == 0 )
	{
		g.exedir_s="?";
		SetDir ( g.myownrootdir_s.Get() );
		t.mygamesfolder_s = "My Games";
		if ( PathExist(t.mygamesfolder_s.Get()) == 0 ) MakeDirectory ( t.mygamesfolder_s.Get() );
		if ( PathExist(t.mygamesfolder_s.Get()) == 1 ) 
		{
			SetDir ( t.mygamesfolder_s.Get() );
			g.exedir_s = GetDir();
		}
	}
	SetDir ( t.told_s.Get() );

	// Path to EXE (for dealing with relative EXE paths later)
	if ( g.exedir_s.Get()[1] == ':' )
	{
		t.exepath_s = g.exedir_s;
	}
	else
	{
		t.exepath_s = g.exedir_s;
	}
	if ( cstr(Right(t.exepath_s.Get(),1)) != "\\"  ) t.exepath_s = t.exepath_s+"\\";

	// ensure filecollection array unmolesterd during level loads (some remote project code would have tried to reuse this array)
	g_bMakingAStandaloneUsingFileCollectionArray = true;

	// Collect all files and folders and store in t.filecollection_s
	mapfile_collectfoldersandfiles ( levelpathfolder );

	// Pre-Stage 2 - clear a list which will collect all folders/files to REMOVE from the final standalone file transfer
	// list, courtesy of the special FPP file which controls the final files to be used for standalone creation
	g_mapfile_fppFoldersToRemoveList.clear();
	g_mapfile_fppFilesToRemoveList.clear();

	// process in stages
	g_mapfile_iStage = 1;
	g_mapfile_fProgress = 0.0f;
}

void mapfile_savestandalone_stage2a ( void )
{
	// Stage 2 - have all level from previous start step
	bool bWeUnloadedTheFirstLevel = false;
	t.levelindex = 1;
	if (g.bUseStoryBoardSetup)
	{
		// restore to first level.
		FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm);
		g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
		strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);
	}
	t.tlevelfile_s="";
	t.tlevelstoprocess = 1;
	g_mapfile_iNumberOfEntitiesAcrossAllLevels = 0;
	while ( t.tlevelstoprocess == 1 ) 
	{
		if ( Len(t.tlevelfile_s.Get())>1 ) 
		{
			g.projectfilename_s=t.tlevelfile_s;
			mapfile_loadproject_fpm ( );
			game_loadinentitiesdatainlevel ( );
			bWeUnloadedTheFirstLevel = true;
			if (t.gamevisuals.bEnableEmptyLevelMode == false)
			{
				// if even one level does NOT use completely-empty mode, we have basic terrain requirements
				g_bUsingSomeKindOfTerrain = true;
			}
		}
		g_mapfile_iNumberOfEntitiesAcrossAllLevels += g.entityelementlist;
		cstr pLevelObjectsCountAllLevels = cstr("g_mapfile_iNumberOfEntitiesAcrossAllLevels==") + cstr(g_mapfile_iNumberOfEntitiesAcrossAllLevels);
		timestampactivity(0, pLevelObjectsCountAllLevels.Get());
		for ( t.e = 1; t.e <= g.entityelementlist; t.e++ )
		{
			t.entid=t.entityelement[t.e].bankindex;
			if ( t.entid>0 ) 
			{
				// suspect old entID references, so produce a log if so
				if (t.entid >= t.entityprofile.size())
				{
					// this is bad
					cstr pOldEntIDReference = cstr("ENTITY REF ERROR:") + cstr(t.e) + cstr(" uses ParentID of ") + cstr(t.entid) + cstr(" >= ") + cstr((int)t.entityprofile.size());
					timestampactivity(0, pOldEntIDReference.Get());
					continue;
				}

				// zone marker can reference other levels to jump to
				if ( t.entityprofile[t.entid].ismarker == 3 ) 
				{
					t.tlevelfile_s=t.entityelement[t.e].eleprof.ifused_s;
					if ( Len(t.tlevelfile_s.Get()) > 1 ) 
					{
						t.tlevelfile_s=cstr("mapbank\\")+g_mapfile_levelpathfolder+t.tlevelfile_s+".fpm";
						timestampactivity(0, t.tlevelfile_s.Get());
						if ( FileExist(cstr(g.fpscrootdir_s+"\\Files\\"+t.tlevelfile_s).Get()) == 1 )
						{
							++t.levelmax;
							t.levellist_s[t.levelmax]=t.tlevelfile_s;
							cstr pLogLevelAdded = cstr("added to list ") + cstr(t.levelmax);
							timestampactivity(0, pLogLevelAdded.Get());
						}
						else
							t.tlevelfile_s="";
					}
				}
			}
		}
		if ( t.levelindex < t.levelmax ) 
		{
			t.tlevelfile_s = "";
			t.tlevelstoprocess = 0;
			while ( t.levelindex < t.levelmax && strcmp ( t.tlevelfile_s.Get(), "" )==NULL ) 
			{
				++t.levelindex;
				t.ttrylevelfile_s=t.levellist_s[t.levelindex];
				for ( t.n = 1; t.n <= t.levelindex-1; t.n++ )
				{
					if ( t.ttrylevelfile_s == t.levellist_s[t.n] ) 
					{
						t.ttrylevelfile_s = "";
						break;
					}
				}
				if ( t.ttrylevelfile_s != "" ) 
				{
					t.tlevelfile_s = t.ttrylevelfile_s;
					t.tlevelstoprocess = 1;
				}
			}
		}
		else
		{
			t.tlevelstoprocess = 0;
		}
	}
	g_mapfile_iNumberOfLevels = 1 + t.levelmax;
	cstr pLogNumberOfLevels = cstr("number of levels==") + cstr(g_mapfile_iNumberOfLevels);
	timestampactivity(0, pLogNumberOfLevels.Get());

	// Stage 2 - collect all files (from all levels)
	t.levelindex=0;
	t.tlevelfile_s="";
	t.tlevelstoprocess = 1;

	if (g.projectfilename_s != t.tmasterlevelfile_s)
		restore_old_map = true;
	g.projectfilename_s = t.tmasterlevelfile_s;
	if ( bWeUnloadedTheFirstLevel == true )
		t.tlevelfile_s = t.tmasterlevelfile_s;
}

int mapfile_savestandalone_stage2b ( void )
{
	int iMoveAlong = 0;
	cstr pProgressMarkerLog = cstr("tlevelstoprocess:") + t.tlevelstoprocess;
	timestampactivity(0, pProgressMarkerLog.Get());
	if ( t.tlevelstoprocess == 1 )
	{
		// load in level FPM
		cstr pProgressLevelLog = cstr("tlevelfile:") + t.tlevelfile_s;
		timestampactivity(0, pProgressLevelLog.Get());
		if ( Len(t.tlevelfile_s.Get())>1 )
		{
			g.projectfilename_s=t.tlevelfile_s;
			mapfile_loadproject_fpm ( );
			game_loadinentitiesdatainlevel ( );
			if (t.gamevisuals.bEnableEmptyLevelMode == false)
			{
				// if even one level does NOT use completely-empty mode, we have basic terrain requirements
				g_bUsingSomeKindOfTerrain = true;
			}
		}

		// 061018 - check if an FPP file exists for this level file
		cstr pFPPFile = cstr(Left(g.projectfilename_s.Get(),strlen(g.projectfilename_s.Get())-4)) + ".fpp";
		cstr pLogFPP = cstr("pFPPFile:") + pFPPFile;
		timestampactivity(0, pLogFPP.Get());
		if ( FileExist( pFPPFile.Get() ) == 1 )
		{
			// used to specify additional files required for standalone executable
			// handy as a workaround until reported issue resolved
			int iFPPStandaloneExtraFilesMode = 0;
			Dim ( t.data_s, 999 );
			LoadArray ( pFPPFile.Get(), t.data_s );
			for ( t.l = 0 ; t.l <= 999; t.l++ )
			{
				t.line_s = t.data_s[t.l];
				LPSTR pLine = t.line_s.Get();
				if ( Len(pLine) > 0 )
				{
					cstr pLofFPPLine = cstr("FPP:") + pLine;
					timestampactivity(0, pLofFPPLine.Get());
					if ( strnicmp ( pLine, "[standalone add files]", 22 ) == NULL )
					{
						// denotes our standalone extra files
						iFPPStandaloneExtraFilesMode = 1;
					}
					else
					{
						if ( strnicmp ( pLine, "[standalone delete files]", 25 ) == NULL )
						{
							// denotes our standalone remove files
							iFPPStandaloneExtraFilesMode = 2;
						}
						else
						{
							// this prevents newer FPP files from getting confused with this original simple method
							if ( iFPPStandaloneExtraFilesMode == 1 )
							{
								// add
								if ( pLine[strlen(pLine)-1] == '\\' )
								{
									// include whole folder
									addfoldertocollection(pLine);
								}
								else
								{
									// include specific file
									addtocollection(pLine);
								}
							}
							if ( iFPPStandaloneExtraFilesMode == 2 )
							{
								// remove
								if ( pLine[strlen(pLine)-1] == '\\' )
								{
									// remove whole folder
									g_mapfile_fppFoldersToRemoveList.push_back(cstr(pLine));
								}
								else
								{
									// remove specific file
									g_mapfile_fppFilesToRemoveList.push_back(cstr(pLine));
								}
							}
						}
					}
				}
			}
			UnDim(t.data_s);
		}	

		//  chosen sky, terrain and veg
		cstr pSkyLog = cstr("adding skybank files:") + t.skybank_s[g.skyindex];
		timestampactivity(0, pSkyLog.Get());
		addfoldertocollection(cstr(cstr("skybank\\")+t.skybank_s[g.skyindex]).Get() );

		// pre-add the skins folder - can optimize later to find only skins we used (118MB)
		timestampactivity(0, "adding charactercreatorplus skin files");
		addfoldertocollection("charactercreatorplus\\skins");

		// start for loop
		t.e = 1;
		cstr pProgressPercLog = cstr("g_mapfile_fProgressSpan:") + g_mapfile_fProgressSpan;
		timestampactivity(0, pProgressPercLog.Get());
		g_mapfile_fProgressSpan = g_mapfile_iNumberOfEntitiesAcrossAllLevels;
	}
	else
	{
		iMoveAlong = 1;
	}
	return iMoveAlong;
}

void mapfile_addallentityrelatedfiles ( int entid, entityeleproftype* pEleProf )
{
	// Store current
	int iStoredEntID = t.entid;
	t.entid = entid;

	if (pEleProf->newparticle.emittername.Len() > 0)
	{
		char effectname[MAX_PATH];
		if (pEleProf->newparticle.emittername != "particlesbank/default")
		{
			AddWPETextures(pEleProf->newparticle.emittername.Get());
			//PE: Looks like some do not have .arx extension.
			strcpy(effectname, pEleProf->newparticle.emittername.Get());
			strcat(effectname, ".arx");
			AddWPETextures(effectname);
		}
	}

	// Check for custom images loaded in lua script
	if (pEleProf->aimain_s != "")
	{
		// Check for files required by the script via DLua
		extern void InitParseLuaScript(entityeleproftype * tmpeleprof);
		extern void ParseLuaScript(entityeleproftype * tmpeleprof, char* script);

		// PropertiesVariables are not filled automatically for t.entityelement[t.e], so create a temp variable and parse the script to fill the values
		// PropertiesVariables may eventually be saved with entityelement data, so we could remove this step in future
		entityeleproftype tempeleprof = *pEleProf;
		InitParseLuaScript(&tempeleprof);
		cstr script_name = "";
		script_name = "scriptbank\\";
		script_name += tempeleprof.aimain_s;
		ParseLuaScript(&tempeleprof, script_name.Get());

		// We now have the properties variables
		for (int i = 0; i < MAXPROPERTIESVARIABLES; i++)
		{
			// Check the lua variable is a string.
			//PE: We can now have variabletype == 7 that contain media.
			if (tempeleprof.PropertiesVariable.VariableType[i] == 2 || tempeleprof.PropertiesVariable.VariableType[i] == 7)
			{
				// Check if the string contains a file.
				int variableLength = strlen(tempeleprof.PropertiesVariable.VariableValue[i]);
				if (variableLength > 4 && ( tempeleprof.PropertiesVariable.VariableValue[i][variableLength - 4] == '.' || tempeleprof.PropertiesVariable.VariableValue[i][variableLength - 3] == '.') )
				{
					// can specify a textfile, but needs to be specified as relative
					char cRel[MAX_PATH];
					LPSTR pStringOrFile = tempeleprof.PropertiesVariable.VariableValue[i];
					if (pStringOrFile[1] == ':')
					{
						// replace absolute paths with relative ones
						char pRelativePathAndFile[MAX_PATH];
						strcpy(pRelativePathAndFile, pStringOrFile);
						GG_GetRealPath(pRelativePathAndFile, 0);
						extern char szWriteDir[MAX_PATH];
						char pRemoveAbsPart[MAX_PATH];
						strcpy(pRemoveAbsPart, szWriteDir);
						strcat(pRemoveAbsPart, "Files\\");
						if (strnicmp(pRelativePathAndFile, pRemoveAbsPart, strlen(pRemoveAbsPart)) == NULL)
						{
							strcpy(pRelativePathAndFile, pStringOrFile + strlen(pRemoveAbsPart));
						}
						addtocollection(pRelativePathAndFile);
						strcpy(cRel, pRelativePathAndFile);
						if (pRelativePathAndFile[1] == ':')
						{
							//PE: If this is a projectfolder above dont work so.
							extern char szBeforeChangeWriteDir[MAX_PATH];
							strcpy(pRemoveAbsPart, szBeforeChangeWriteDir);
							strcat(pRemoveAbsPart, "Files\\");
							if (strnicmp(pRelativePathAndFile, pRemoveAbsPart, strlen(pRemoveAbsPart)) == NULL)
							{
								strcpy(pRelativePathAndFile, pStringOrFile + strlen(pRemoveAbsPart));
							}
							addtocollection(pRelativePathAndFile);
							strcpy(cRel, pRelativePathAndFile);
						}
					}
					else
					{
						addtocollection(pStringOrFile);
						strcpy(cRel, pStringOrFile);
					}

					//PE: Add WPE Textures.
					char cPE[MAX_PATH];
					strcpy(cPE, cRel);
					char* find = (char*)pestrcasestr(cPE, ".pe");
					if (!find) find = (char*)pestrcasestr(cPE, ".arx");;
					if (find)
					{
						AddWPETextures(cPE);
					}

					//PE: if .dds or.png also add - _normal and _emissive and _surface (behavior: Change Texture).
					if (pestrcasestr(tempeleprof.PropertiesVariable.VariableValue[i], ".dds") || pestrcasestr(tempeleprof.PropertiesVariable.VariableValue[i], ".png"))
					{
						if (pestrcasestr(tempeleprof.PropertiesVariable.VariableValue[i], "_color"))
						{
							std::string sParseName = tempeleprof.PropertiesVariable.VariableValue[i];
							replaceAll(sParseName, "_color.", "_normal.");
							addtocollection((char*)sParseName.c_str());
							replaceAll(sParseName, "_normal.", "_surface.");
							addtocollection((char*)sParseName.c_str());
							replaceAll(sParseName, "_surface.", "_emissive.");
							addtocollection((char*)sParseName.c_str());
							replaceAll(sParseName, "_emissive.", "_illumination.");
							addtocollection((char*)sParseName.c_str());
						}
					}
				}
			}
		}

		// Copy any files specified directly in Lua script
		cstr tLuaScript = g.fpscrootdir_s + "\\Files\\scriptbank\\";
		tLuaScript += pEleProf->aimain_s;
		FILE* tLuaScriptFile = GG_fopen (tLuaScript.Get(), "r");
		if (tLuaScriptFile)
		{
			char tTempLine[2048];
			while (!feof(tLuaScriptFile))
			{
				fgets (tTempLine, 2047, tLuaScriptFile);
				if (strstr (tTempLine, "LoadImages"))
				{
					char* pImageFolder = strstr (tTempLine, "\"");
					if (pImageFolder)
					{
						pImageFolder++;
						char* pImageFolderEnd = strstr (pImageFolder, "\"");
						if (pImageFolderEnd)
						{
							*pImageFolderEnd = '\0';
							cstr tFolderToAdd = cstr(cstr("scriptbank\\images\\") + cstr(pImageFolder));
							addfoldertocollection (tFolderToAdd.Get());
						}
					}
				}

				// Handle new load image and sound commands, they can be in nested folders
				if (strstr (tTempLine, "LoadImage ")
				|| strstr (tTempLine, "LoadImage(")
				|| strstr (tTempLine, "LoadGlobalSound ")
				|| strstr (tTempLine, "LoadGlobalSound(")
				|| strstr(tTempLine, "WParticleEffectLoad(")
				|| strstr(tTempLine, "WParticleEffectLoad "))
				{
					char* pImageFolder = strstr (tTempLine, "\"");
					if (pImageFolder)
					{
						pImageFolder++;
						char* pImageFolderEnd = strstr (pImageFolder, "\"");
						if (pImageFolderEnd)
						{
							*pImageFolderEnd = '\0';
							cstr pFile = cstr(pImageFolder);
							addtocollection (pFile.Get());

							//PE: Resolve wpe effects textures.
							char cPE[MAX_PATH];
							strcpy(cPE, pFile.Get());
							char* find = (char*)pestrcasestr(cPE, ".pe");
							if (!find) find = (char*)pestrcasestr(cPE, ".arx");;
							if(find)
							{
								AddWPETextures(cPE);
							}
						}
					}
				}
				if (strstr(tTempLine, "SetSkyTo(")) 
				{
					char* pSkyFolder = strstr(tTempLine, "\"");
					if (pSkyFolder)
					{
						pSkyFolder++;
						char* pSkyFolderEnd = strstr(pSkyFolder, "\"");
						if (pSkyFolderEnd)
						{
							*pSkyFolderEnd = '\0';
							cstr tFolderToAdd = cstr(cstr("skybank\\") + cstr(pSkyFolder));
							addfoldertocollection(tFolderToAdd.Get());
						}
					}
				}
			}
			fclose (tLuaScriptFile);
		}
	}

	// gives "t.entid" and adds ALL entity profile related files to the collection (not t.e(pEleProf specific)
	addthisentityprofilesfilestocollection(pEleProf);

	// shader file
	t.tfile_s = pEleProf->effect_s; addtocollection(t.tfile_s.Get());
	//Try to take the .blob.
	if (cstr(Lower(Right(t.tfile_s.Get(), 3))) == ".fx") 
	{
		t.tfile_s = Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 3);
		t.tfile_s = t.tfile_s + ".blob";
		if (FileExist(t.tfile_s.Get()) == 1)
		{
			addtocollection(t.tfile_s.Get());
		}
	}

	// script files
	cstr script_name = "";
	script_name = "scriptbank\\";
	script_name += pEleProf->aimain_s;
	t.tfile_s = script_name;
	addtocollection(t.tfile_s.Get());
	// Copy .byc too
	std::string sLuaFile = t.tfile_s.Get();
	replaceAll(sLuaFile, ".lua", ".byc");
	addtocollection((char*)sLuaFile.c_str());

	// for the script associated, scan it and include any references to other scripts
	scanscriptfileandaddtocollection(t.tfile_s.Get());

	// sound files
	if (t.entityprofile[t.entid].ismarker == 1 && pEleProf->soundset_s.Len() > 0) 
	{
		//PE: Make sure voiceset from player start marker is added.
		t.tfile_s = pEleProf->soundset_s;
		addfoldertocollection(cstr(cstr("audiobank\\voices\\") + cstr(t.tfile_s.Get())).Get());
	}
	t.tfile_s = pEleProf->soundset_s; addtocollection(t.tfile_s.Get());
	t.tfile_s = pEleProf->soundset1_s; addtocollection(t.tfile_s.Get());
	t.tfile_s = pEleProf->soundset2_s; addtocollection(t.tfile_s.Get());
	t.tfile_s = pEleProf->soundset3_s; addtocollection(t.tfile_s.Get());
	t.tfile_s = pEleProf->soundset5_s; addtocollection(t.tfile_s.Get());
	t.tfile_s = pEleProf->soundset6_s; addtocollection(t.tfile_s.Get());
	t.tfile_s = pEleProf->soundset4a_s; addtocollection(t.tfile_s.Get());
	t.tfile_s = pEleProf->overrideanimset_s; addtocollection(t.tfile_s.Get());

	// lipsync files associated with soundset references
	cstr tmpFile_s = pEleProf->soundset_s; tmpFile_s = cstr(Left(tmpFile_s.Get(), strlen(tmpFile_s.Get()) - 4)) + ".lip"; addtocollection(tmpFile_s.Get());
	tmpFile_s = pEleProf->soundset1_s; tmpFile_s = cstr(Left(tmpFile_s.Get(), strlen(tmpFile_s.Get()) - 4)) + ".lip"; addtocollection(tmpFile_s.Get());
	tmpFile_s = pEleProf->soundset2_s; tmpFile_s = cstr(Left(tmpFile_s.Get(), strlen(tmpFile_s.Get()) - 4)) + ".lip"; addtocollection(tmpFile_s.Get());
	tmpFile_s = pEleProf->soundset3_s; tmpFile_s = cstr(Left(tmpFile_s.Get(), strlen(tmpFile_s.Get()) - 4)) + ".lip"; addtocollection(tmpFile_s.Get());
	tmpFile_s = pEleProf->soundset5_s; tmpFile_s = cstr(Left(tmpFile_s.Get(), strlen(tmpFile_s.Get()) - 4)) + ".lip"; addtocollection(tmpFile_s.Get());
	tmpFile_s = pEleProf->soundset6_s; tmpFile_s = cstr(Left(tmpFile_s.Get(), strlen(tmpFile_s.Get()) - 4)) + ".lip"; addtocollection(tmpFile_s.Get());
	tmpFile_s = pEleProf->soundset4a_s; tmpFile_s = cstr(Left(tmpFile_s.Get(), strlen(tmpFile_s.Get()) - 4)) + ".lip"; addtocollection(tmpFile_s.Get());

	// collectable guns
	cstr pGunPresent = "";
	if (Len(t.entityprofile[t.entid].isweapon_s.Get()) > 1) pGunPresent = t.entityprofile[t.entid].isweapon_s;
	if (t.entityprofile[t.entid].isammo == 0)
	{
		// 270618 - only accept HASWEAPON if NOT ammo, so executables are not bloated with ammo that specifies another weapon type
		if (Len(pEleProf->hasweapon_s.Get()) > 1) pGunPresent = pEleProf->hasweapon_s;
	}
	if (Len(pGunPresent.Get()) > 1)
	{
		t.tfile_s = cstr("gamecore\\guns\\") + pGunPresent; addfoldertocollection(t.tfile_s.Get());
		t.findgun_s = Lower(pGunPresent.Get());
		gun_findweaponindexbyname ();
		if (t.foundgunid > 0)
		{
			// ammo and brass
			for (t.x = 0; t.x <= 1; t.x++)
			{
				// ammo files
				t.tpoolindex = g.firemodes[t.foundgunid][t.x].settings.poolindex;
				if (t.tpoolindex > 0)
				{
					t.tfile_s = cstr("gamecore\\ammo\\") + t.ammopool[t.tpoolindex].name_s;
					if (PathExist (t.tfile_s.Get())) addfoldertocollection(t.tfile_s.Get());
				}

				// brass files
				int iBrassIndex = g.firemodes[t.foundgunid][t.x].settings.brass;
				if (iBrassIndex > 0)
				{
					t.tfile_s = cstr(cstr("gamecore\\brass\\brass") + Str(iBrassIndex));
					if (PathExist (t.tfile_s.Get()))
						addfoldertocollection(t.tfile_s.Get());
				}

				// and any projectile files associated with it
				cstr pProjectilePresent = t.gun[t.foundgunid].projectile_s;
				if (Len(pProjectilePresent.Get()) > 1)
				{
					t.tfile_s = cstr("gamecore\\projectiletypes\\") + pProjectilePresent;
					addfoldertocollection(t.tfile_s.Get());
				}
			}

			// and any projectile files associated with it
			cstr pProjectilePresent = t.gun[t.foundgunid].projectile_s;
			if (Len(pProjectilePresent.Get()) > 1)
			{
				t.tfile_s = cstr("gamecore\\projectiletypes\\") + pProjectilePresent;
				addfoldertocollection(t.tfile_s.Get());
			}
		}
	}

	// player start marler
	if (t.entityprofile[t.entid].ismarker == 1)
	{
		// can specify custom arms for weapons, need the hands
		if (pEleProf->texaltd_s.Len() > 0)
		{
			t.tfile_s = cstr("gamecore\\hands\\") + pEleProf->texaltd_s;
			addfoldertocollection(t.tfile_s.Get());
		}
	}

	// zone marker can reference other levels to jump to
	if (t.entityprofile[t.entid].ismarker == 3)
	{
		t.tlevelfile_s = pEleProf->ifused_s;
		if (Len(t.tlevelfile_s.Get()) > 1)
		{
			t.tlevelfile_s = cstr("mapbank\\") + g_mapfile_levelpathfolder + t.tlevelfile_s + ".fpm";
			if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + t.tlevelfile_s).Get()) == 1)
			{
				addtocollection(t.tlevelfile_s.Get());
			}
			else
			{
				// nope, just a regular string entry in the marker field
				t.tlevelfile_s = "";
			}
		}
	}
	t.entid = iStoredEntID;
}

void mapfile_copyallfilecollectiontopreferredprojectfolder(void)
{
	// before the copy, some files should never move across (core scripts, etc)
	removefromcollection ("scriptbank\\global.lua");
	removefromcollection ("scriptbank\\gameloop.lua");
	removefromcollection ("scriptbank\\masterinterpreter.lua");
	removefromcollection ("scriptbank\\physlib.lua");
	removefromcollection ("scriptbank\\quatlib.lua");
	removefromcollection ("scriptbank\\utillib.lua");
	removefromcollection ("scriptbank\\vectlib.lua");
	removefromcollection ("scriptbank\\hud0.lua");
	removefromcollection ("scriptbank\\module_activationcontrol.lua");
	removefromcollection ("scriptbank\\module_misclib.lua");
	removefromcollection ("scriptbank\\navmeshlib.lua");
	removefromcollection ("scriptbank\\perlin_noise.lua");
	removefromcollection ("scriptbank\\ai\\module_cameraoverride.lua");
	removefromcollection ("scriptbank\\no_behavior_selected.lua");
	
	// do the copy
	char pPreferredProjectEntityFolder[MAX_PATH];
	strcpy(pPreferredProjectEntityFolder, Storyboard.customprojectfolder);
	strcat(pPreferredProjectEntityFolder, Storyboard.gamename);
	for (int files = 1; files <= g.filecollectionmax; files++)
	{
		char pFileToCopy[MAX_PATH];
		strcpy(pFileToCopy, t.filecollection_s[files].Get());
		GG_GetRealPath(pFileToCopy, 0);
		if (strnicmp (pFileToCopy, pPreferredProjectEntityFolder, strlen(pPreferredProjectEntityFolder)) != NULL)
		{
			char pFileToCopyTo[MAX_PATH];
			strcpy(pFileToCopyTo, t.filecollection_s[files].Get());
			GG_GetRealPath(pFileToCopyTo, 1);
			CopyFileA(pFileToCopy, pFileToCopyTo, FALSE);
		}
	}
}

void mapfile_ensurethisfolderexistsinremoteproject(LPSTR pFolderToCopy)
{
	//PE: Bug fix , in standalone skybox got copied to docwrite folder with _e_.
	if (t.game.gameisexe == 1) return;
	extern bool g_bMakingAStandaloneUsingFileCollectionArray;
	if (g_bMakingAStandaloneUsingFileCollectionArray == false)
	{
		// clear filecollection, specify new folder, copy all to preferred project folder
		g.filecollectionmax = 0;
		Undim (t.filecollection_s);
		Dim (t.filecollection_s, 500);
		addfoldertocollection (pFolderToCopy);
		extern void mapfile_copyallfilecollectiontopreferredprojectfolder (void);
		mapfile_copyallfilecollectiontopreferredprojectfolder();
	}
}

int mapfile_savestandalone_stage2c ( void )
{
	// choose all entities and associated files
	int iMoveAlong = 0;
	if ( t.e <= g.entityelementlist )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if ( t.entid>0 ) 
		{
			// most eleprof related files
			mapfile_addallentityrelatedfiles(t.entid, &t.entityelement[t.e].eleprof);

			// and purey entity element related files
			if (!t.entityelement[t.e].eleprof.bUseFPESettings)
			{
				// Also add any custom material textures
				sObject* pObject = GetObjectData(t.entityelement[t.e].obj);
				if (pObject)
				{
					for (int i = 0; i < pObject->iFrameCount; i++)
					{
						sFrame* pFrame = pObject->ppFrameList[i];
						if (pFrame)
						{
							sMesh* pMesh = pFrame->pMesh;
							if (pMesh)
							{
								wiScene::MaterialComponent* pMaterialComponent = wiScene::GetScene().materials.GetComponent(pMesh->wickedmaterialindex);
								if (pMaterialComponent)
								{
									if (pMaterialComponent->textures[0].name.length() > 0)
									{
										addtocollection((char*)pMaterialComponent->textures[0].name.c_str());
										addtocollection((char*)pMaterialComponent->textures[1].name.c_str());
										addtocollection((char*)pMaterialComponent->textures[2].name.c_str());
										addtocollection((char*)pMaterialComponent->textures[3].name.c_str());
									}
								}
							}
						}
					}
				}
				//PE: And add any custom wematerial texture settings.
				if (t.entityelement[t.e].eleprof.WEMaterial.MaterialActive)
				{
					//PE: Need relative path here.
					cstr entpath = cstr("entitybank\\") + t.entitybank_s[t.entid];
					for (t.n = Len(entpath.Get()); t.n >= 1; t.n += -1)
					{
						if (cstr(Mid(entpath.Get(), t.n)) == "\\" || cstr(Mid(entpath.Get(), t.n)) == "/")
						{
							entpath = Left(entpath.Get(), t.n);
							break;
						}
					}

					cstr texture;
					for (int loop = 0; loop < MAXMESHMATERIALS; loop++)
					{
						for (int l = 0; l < 4; l++)
						{
							if (l == 0) texture = t.entityelement[t.e].eleprof.WEMaterial.baseColorMapName[loop];
							if (l == 1) texture = t.entityelement[t.e].eleprof.WEMaterial.normalMapName[loop];
							if (l == 2) texture = t.entityelement[t.e].eleprof.WEMaterial.surfaceMapName[loop];
							if (l == 3) texture = t.entityelement[t.e].eleprof.WEMaterial.emissiveMapName[loop];

							if (texture.Len() > 0)
							{
								if (!pestrcasestr(texture.Get(), "\\") &&
									!pestrcasestr(texture.Get(), "/"))
								{
									cstr finalname = entpath + texture;
									addtocollection((char*)finalname.Get());
								}
								else
									addtocollection((char*)texture.Get());
							}
						}
					}
				}
			}
		}
	}
	else
	{
		if (t.visuals.customTexturesFolder.Len() > 0)
		{
			// Collect all .dds textures in this folder and add them to the standalone file collection
			char customTexturePath[MAX_PATH];
			strcpy(customTexturePath, GG_GetWritePath());
			strcat(customTexturePath, t.visuals.customTexturesFolder.Get());
			std::vector<std::string> filesToCollect;
			CollectFilesWithExtension(".dds", customTexturePath, &filesToCollect);
			for (auto& file : filesToCollect)
			{
				const char* filename = strstr(file.c_str(), "Files\\");
				if (filename)
				{
					const char* finalName = filename + 6;
					addtocollection((char*)finalName);
				}
			}
			// TODO: If we ever release DLC with terrain textures stored in root dir, we would need to do an additional scan here.
		}
		iMoveAlong = 1;
	}
	t.e++;
	return iMoveAlong;
}

void mapfile_savestandalone_stage2d ( void )
{
	// decide if another level needs loading/processing
	if ( t.levelindex < t.levelmax ) 
	{
		t.tlevelfile_s = "";
		t.tlevelstoprocess = 0;
		while ( t.levelindex < t.levelmax && strcmp ( t.tlevelfile_s.Get(), "" )==NULL ) 
		{
			++t.levelindex;
			t.ttrylevelfile_s=t.levellist_s[t.levelindex];
			for ( t.n = 1; t.n <= t.levelindex-1; t.n++ )
			{
				if ( t.ttrylevelfile_s == t.levellist_s[t.n] ) 
				{
					t.ttrylevelfile_s = "";
					break;
				}
			}
			if ( t.ttrylevelfile_s != "" ) 
			{
				t.tlevelfile_s = t.ttrylevelfile_s;
				t.tlevelstoprocess = 1;
			}
		}
	}
	else
	{
		t.tlevelstoprocess = 0;
	}
}

void mapfile_savestandalone_stage2e ( void )
{
	//  if multi-level, do NOT include the levelbank\testmap temp files
	t.tignorelevelbankfiles=0;
	if (  g.projectfilename_s != t.tmasterlevelfile_s ) 
	{
		timestampactivity(0,"Ignoring levelbank testmap folder for multilevel standalone");
		t.tignorelevelbankfiles=1;
	}
	else
	{
		addtocollection("levelbank\\testmap\\header.dat");
	}
}

void mapfile_savestandalone_stage3 ( void )
{
	// if any of the levels used terrain, add the terrain/tree/grass folders
	if (g_bUsingSomeKindOfTerrain == true)
	{
		addfoldertocollection("treebank"); // all but completely empty level (basic flat terrain)
		addfoldertocollection("treebank\\billboards");
		addfoldertocollection("treebank\\textures");
		if (strlen(Storyboard.customprojectfolder) > 0)
		{
			// if remote project being used, it will copy its OWN terraintexture folder over
			// so no need to include the default in case all custom textures used
		}
		else
		{
			//PE: Storyboard - Need standalone / lua menu's working.
			addfoldertocollection("terraintextures");
			for (int i = 0; i < 42; i++)
			{
				char addfolder[MAX_PATH];
				sprintf(addfolder, "terraintextures\\mat%d", i);
				addfoldertocollection(addfolder);
			}
		}
		addfoldertocollection("grassbank");
	}

	//  Create game folder
	SetDir (  t.exepath_s.Get() );
	MakeDirectory (  t.exename_s.Get() );
	SetDir (  t.exename_s.Get() );
	MakeDirectory (  "Files" );
	SetDir (  "Files" );

	//  Ensure gamesaves files are removed (if any)
	if (  PathExist("gamesaves") == 1 ) 
	{
		SetDir (  "gamesaves" );
		ChecklistForFiles (  );
		for ( t.c = 1 ; t.c<=  ChecklistQuantity(); t.c++ )
		{
			t.tfile_s=ChecklistString(t.c);
			if (  Len(t.tfile_s.Get())>2 ) 
			{
				if (  FileExist(t.tfile_s.Get()) == 1  )  DeleteAFile (  t.tfile_s.Get() );
			}
		}
		SetDir (  ".." );
	}

	//  Ensure file path exists (by creating folders)
	createallfoldersincollection();

	// If not copying levelbank files, must still create the folder
	if ( t.tignorelevelbankfiles == 1 ) 
	{
		t.olddir_s=GetDir();
		SetDir (  cstr(t.exepath_s+t.exename_s+"\\Files").Get() );
		if (  PathExist("levelbank") == 0  )  MakeDirectory (  "levelbank" );
		SetDir (  "levelbank" );
		if (  PathExist("testmap") == 0  )  MakeDirectory (  "testmap" );
		SetDir (  "testmap" );
		if (  PathExist("lightmaps") == 0  )  MakeDirectory (  "lightmaps" );
		if (  PathExist("ttsfiles") == 0  )  MakeDirectory (  "ttsfiles" );
		SetDir (  t.olddir_s.Get() );
	}

	// If existing standalone there, ensure lightmaps are removed (as they will be unintentionally encrypted)
	t.destpath_s=t.exepath_s+t.exename_s+"\\Files\\levelbank\\testmap\\lightmaps";
	if (  PathExist(t.destpath_s.Get()) == 1 ) 
	{
		t.olddir_s=GetDir();
		SetDir (  t.destpath_s.Get() );
		ChecklistForFiles (  );
		for ( t.c = 1 ; t.c<=  ChecklistQuantity(); t.c++ )
		{
			t.tfile_s=ChecklistString(t.c);
			if (  t.tfile_s != "." && t.tfile_s != ".." ) 
			{
				if (  FileExist(t.tfile_s.Get()) == 1  )  DeleteAFile (  t.tfile_s.Get() );
			}
		}
		SetDir (  t.olddir_s.Get() );
	}

	// If existing standalone there, ensure ttsfiles are removed
	t.destpath_s=t.exepath_s+t.exename_s+"\\Files\\levelbank\\testmap\\ttsfiles";
	if ( PathExist(t.destpath_s.Get()) == 1 ) 
	{
		t.olddir_s=GetDir();
		SetDir ( t.destpath_s.Get() );
		ChecklistForFiles ( );
		for ( t.c = 1 ; t.c <= ChecklistQuantity(); t.c++ )
		{
			t.tfile_s=ChecklistString(t.c);
			if ( t.tfile_s != "." && t.tfile_s != ".." ) 
			{
				if ( FileExist(t.tfile_s.Get()) == 1  ) DeleteAFile ( t.tfile_s.Get() );
			}
		}
		SetDir (  t.olddir_s.Get() );
	}

	// 010917 - go through and remove any X files that have DBO counterparts
	SetDir ( cstr(g.fpscrootdir_s+"\\Files\\").Get() );
	for ( t.fileindex = 1 ; t.fileindex <= t.filesmax; t.fileindex++ )
	{
		t.src_s=t.filecollection_s[t.fileindex];
		if ( FileExist(t.src_s.Get()) == 1 ) 
		{
			char pSrcFile[1024];
			strcpy ( pSrcFile, t.filecollection_s[t.fileindex].Get() );
			if ( strnicmp ( pSrcFile + strlen(pSrcFile) - 4, ".dbo", 4 ) == NULL )
			{
				cstr dboequiv = cstr(Left(pSrcFile,strlen(pSrcFile)-4))+".x";
				if ( FileExist(dboequiv.Get()) == 1 ) 
				{
					// Found DBO, and an X file sitting alongside it, remove the X from consideration
					removefromcollection ( dboequiv.Get() );
				}
			}
		}
	}

	// also remove folders/files marked by FPP file
	if ( g_mapfile_fppFoldersToRemoveList.size() > 0 || g_mapfile_fppFilesToRemoveList.size() > 0 )
	{
		for ( int n = 0; n < g_mapfile_fppFoldersToRemoveList.size(); n++ )
		{
			cstr pRemoveFolder = g_mapfile_fppFoldersToRemoveList[n];
			removeanymatchingfromcollection ( pRemoveFolder.Get() );
		}
		for ( int n = 0; n < g_mapfile_fppFilesToRemoveList.size(); n++ )
		{
			cstr pRemoveFile = g_mapfile_fppFilesToRemoveList[n];
			removeanymatchingfromcollection ( pRemoveFile.Get() );
		}
	}
}

void removeEmptyFolders(const std::string& directoryPath)
{
	std::string searchPath = directoryPath + "\\*";

	WIN32_FIND_DATAA findFileData;
	HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		return;
	}
	std::vector<std::string> subdirectories;

	do
	{
		if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0)
		{
			continue;
		}
		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			std::string subDirPath = directoryPath + "\\" + findFileData.cFileName;
			subdirectories.push_back(subDirPath);
			removeEmptyFolders(subDirPath);
		}
	} while (FindNextFileA(hFind, &findFileData) != 0);

	FindClose(hFind);

	for (const auto& subDir : subdirectories)
	{
		removeEmptyFolders(subDir);
	}

	//PE: Try removing current folder , if its empty it will work.
	RemoveDirectoryA(directoryPath.c_str());
}

