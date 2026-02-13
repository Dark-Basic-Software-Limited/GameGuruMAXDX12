void charactercreatorplus_refreshtype(void)
{
	// need legacy loading for image reskinning work
	image_setlegacyimageloading(true);

	// ensure we have all textures of character creator init before we start
	image_preload_files_wait();
	object_preload_files_wait();

	// reset character base mesh and textures based on type stored in CCP_Type
	CharacterCreatorHeadGear_s.clear();
	CharacterCreatorHair_s.clear();
	CharacterCreatorHead_s.clear();
	CharacterCreatorEyeglasses_s.clear();
	CharacterCreatorFacialHair_s.clear();
	CharacterCreatorBody_s.clear();
	CharacterCreatorLegs_s.clear();
	CharacterCreatorFeet_s.clear();
	CharacterCreatorAccessory1_s.clear();
	CharacterCreatorAccessory2_s.clear();

	CharacterCreatorAnnotatedHeadGear_s.clear();
	CharacterCreatorAnnotatedHair_s.clear();
	CharacterCreatorAnnotatedHead_s.clear();
	CharacterCreatorAnnotatedEyeglasses_s.clear();
	CharacterCreatorAnnotatedFacialHair_s.clear();
	CharacterCreatorAnnotatedBody_s.clear();
	CharacterCreatorAnnotatedLegs_s.clear();
	CharacterCreatorAnnotatedFeet_s.clear();
	CharacterCreatorAnnotatedAccessory1_s.clear();
	CharacterCreatorAnnotatedAccessory2_s.clear();


	CharacterCreatorAnnotatedTagHeadGear_s.clear();
	CharacterCreatorAnnotatedTagHair_s.clear();
	CharacterCreatorAnnotatedTagHead_s.clear();
	CharacterCreatorAnnotatedTagEyeglasses_s.clear();
	CharacterCreatorAnnotatedTagFacialHair_s.clear();
	CharacterCreatorAnnotatedTagBody_s.clear();
	CharacterCreatorAnnotatedTagLegs_s.clear();
	CharacterCreatorAnnotatedTagFeet_s.clear();
	CharacterCreatorAnnotatedTagAccessory1_s.clear();
	CharacterCreatorAnnotatedTagAccessory2_s.clear();

	// choose voice based on CCP type
	pCCPVoiceSet = "";
	if (stricmp (CCP_Type, "adult male") == NULL) pCCPVoiceSet = "David";
	if (stricmp (CCP_Type, "adult female") == NULL) pCCPVoiceSet = "Hazel";
	for (int i = 0; i < g_voiceList_s.size(); i++)
	{
		if (stricmp (CCP_Type, "adult male") == NULL && strstr (g_voiceList_s[i].Get(), "David") != NULL)
		{
			pCCPVoiceSet = g_voiceList_s[i].Get();
			break;
		}
		if (stricmp (CCP_Type, "adult female") == NULL && strstr (g_voiceList_s[i].Get(), "Hazel") != NULL)
		{
			pCCPVoiceSet = g_voiceList_s[i].Get();
			break;
		}
	}

	// work out path to parts
	char pPartsPath[260];
	strcpy(pPartsPath, "charactercreatorplus\\parts\\");
	strcat(pPartsPath, CCP_Type);
	strcat(pPartsPath, "\\");

	// some lists can have a NONE value, which does not need a part model
	CharacterCreatorHeadGear_s.insert(std::make_pair("None", pPartsPath));
	CharacterCreatorHair_s.insert(std::make_pair("None", pPartsPath));
	CharacterCreatorEyeglasses_s.insert(std::make_pair("None", pPartsPath));
	CharacterCreatorFacialHair_s.insert(std::make_pair("None", pPartsPath));
	CharacterCreatorAccessory1_s.insert(std::make_pair("None", pPartsPath));
	CharacterCreatorAccessory2_s.insert(std::make_pair("None", pPartsPath));

	CharacterCreatorAnnotatedHeadGear_s.insert(std::make_pair("", "None"));
	CharacterCreatorAnnotatedHair_s.insert(std::make_pair("", "None"));
	CharacterCreatorAnnotatedEyeglasses_s.insert(std::make_pair("", "None"));
	CharacterCreatorAnnotatedFacialHair_s.insert(std::make_pair("", "None"));
	CharacterCreatorAnnotatedAccessory1_s.insert(std::make_pair("", "None"));
	CharacterCreatorAnnotatedAccessory2_s.insert(std::make_pair("", "None"));

	// ensure annotates and tags size matches
	CharacterCreatorAnnotatedTagHeadGear_s.insert(std::make_pair("", ""));
	CharacterCreatorAnnotatedTagHair_s.insert(std::make_pair("", ""));
	CharacterCreatorAnnotatedTagEyeglasses_s.insert(std::make_pair("", ""));
	CharacterCreatorAnnotatedTagFacialHair_s.insert(std::make_pair("", ""));
	CharacterCreatorAnnotatedTagAccessory1_s.insert(std::make_pair("", ""));
	CharacterCreatorAnnotatedTagAccessory2_s.insert(std::make_pair("", ""));

	// free any old character objects
	iCharObj = g.characterkitobjectoffset + 1;
	iCharObjHeadGear = g.characterkitobjectoffset + 2;
	iCharObjHair = g.characterkitobjectoffset + 3;
	iCharObjHead = g.characterkitobjectoffset + 4;
	iCharObjEyeglasses = g.characterkitobjectoffset + 5;
	iCharObjFacialHair = g.characterkitobjectoffset + 6;
	iCharObjLegs = g.characterkitobjectoffset + 7;
	iCharObjFeet = g.characterkitobjectoffset + 8;
	iCharObjAccessory1 = g.characterkitobjectoffset + 9;
	iCharObjAccessory2 = g.characterkitobjectoffset + 10;

	if (ObjectExist(iCharObj) == 1) DeleteObject(iCharObj);
	if (ObjectExist(iCharObjHeadGear) == 1) DeleteObject(iCharObjHeadGear);
	if (ObjectExist(iCharObjHair) == 1) DeleteObject(iCharObjHair);
	if (ObjectExist(iCharObjHead) == 1) DeleteObject(iCharObjHead);
	if (ObjectExist(iCharObjEyeglasses) == 1) DeleteObject(iCharObjEyeglasses);
	if (ObjectExist(iCharObjFacialHair) == 1) DeleteObject(iCharObjFacialHair);
	if (ObjectExist(iCharObjLegs) == 1) DeleteObject(iCharObjLegs);
	if (ObjectExist(iCharObjFeet) == 1) DeleteObject(iCharObjFeet);
	if (ObjectExist(iCharObjAccessory1) == 1) DeleteObject(iCharObjAccessory1);
	if (ObjectExist(iCharObjAccessory2) == 1) DeleteObject(iCharObjAccessory2);

	// default body part choices
	strcpy(cSelectedFeetFilter, "");
	strcpy(cSelectedICCode, "IC1a");
	strcpy(cSelectedHeadGear, "None");
	strcpy(cSelectedHair, "");
	strcpy(cSelectedHead, "");
	strcpy(cSelectedEyeglasses, "None");
	strcpy(cSelectedFacialHair, "None");
	strcpy(cSelectedBody, "");
	strcpy(cSelectedLegs, "");
	strcpy(cSelectedFeet, "");
	strcpy(cSelectedAccessory1, "None");
	strcpy(cSelectedAccessory2, "None");

	// which legs part to use
	strcpy(cSelectedLegsFilter, "");
	LPSTR pOptionalLegsChoice = "02"; 

	// scan for all character parts
	cstr olddir_s = GetDir();
	char pTempStr[1024];

	//PE: Load parts avaiable from document folder.
	for (int i = 0; i < 2; i++)
	{
		bool bActive = false;
		if (i == 0)
		{
			strcpy(pTempStr, "charactercreatorplus\\parts\\");
			strcat(pTempStr, CCP_Type);
			SetDir(pTempStr);
			bActive = true;
		}
		else
		{
			extern char szWriteDir[MAX_PATH];
			extern char szBeforeChangeWriteDir[MAX_PATH];
			//PE: WriteDir contain ducument folder and or remoteproject folder.
			if (strlen(szWriteDir) > 0)
			{
				strcpy(pTempStr, szWriteDir);
				strcat(pTempStr, "Files\\");
				strcat(pTempStr, "charactercreatorplus\\parts\\");
				strcat(pTempStr, CCP_Type);
				if (PathExist(pTempStr))
				{
					SetDir(pTempStr);
					bActive = true;
				}
				else
					bActive = false;
			}
		}
		if (bActive)
		{
			ChecklistForFiles();
			charactercreatorplus_loadannotationlist();
			for (int c = 1; c <= ChecklistQuantity(); c++)
			{
				cStr tfile_s = Lower(ChecklistString(c));
				if (tfile_s != "." && tfile_s != "..")
				{
					char* find = NULL;
					if (strcmp(Right(tfile_s.Get(), 10), "_color.dds") == 0)
					{
						// base filename
						char tmp[260];
						strcpy(tmp, tfile_s.Get());
						tmp[strlen(tmp) - 10] = 0; // remove _color.dds

						// determine which list it goes into
						if (pestrcasestr(tfile_s.Get(), " body "))
						{
							CharacterCreatorBody_s.insert(std::make_pair(tmp, pPartsPath));
							LPSTR pAnnotatedLabel = charactercreatorplus_findannotation(tmp);
							if (pAnnotatedLabel)
								CharacterCreatorAnnotatedBody_s.insert(std::make_pair(tmp, pAnnotatedLabel));
							else
								CharacterCreatorAnnotatedBody_s.insert(std::make_pair(tmp, tmp));
							LPSTR pAnnotatedTagLabel = charactercreatorplus_findannotationtag(tmp);
							if (pAnnotatedTagLabel)
								CharacterCreatorAnnotatedTagBody_s.insert(std::make_pair(tmp, pAnnotatedTagLabel));
							else
								CharacterCreatorAnnotatedTagBody_s.insert(std::make_pair(tmp, ""));

							bool bBodyFilterThisOut = false;
							if (stricmp(CCP_Type, "zombie male") != NULL && stricmp(CCP_Type, "zombie female") != NULL)
							{
								if (tmp[strlen(tmp) - 1] != '1' && tmp[strlen(tmp) - 1] != 'a') bBodyFilterThisOut = true;
							}
							if (strlen(cSelectedBody) == 0 && bBodyFilterThisOut == false)
							{
								// for body, do we need no legs?
								if (pAnnotatedTagLabel)
								{
									if (strstr(pAnnotatedTagLabel, "No Legs") != NULL)
									{
										strcpy(cSelectedLegsFilter, "No Legs");
										pOptionalLegsChoice = "01";
									}
								}

								// and this is the default body
								strcpy(cSelectedBody, tmp);
							}
						}
						else if (pestrcasestr(tfile_s.Get(), " feet "))
						{
							CharacterCreatorFeet_s.insert(std::make_pair(tmp, pPartsPath));
							LPSTR pAnnotatedLabel = charactercreatorplus_findannotation(tmp);
							if (pAnnotatedLabel)
								CharacterCreatorAnnotatedFeet_s.insert(std::make_pair(tmp, pAnnotatedLabel));
							else
								CharacterCreatorAnnotatedFeet_s.insert(std::make_pair(tmp, tmp));
							LPSTR pAnnotatedTagLabel = charactercreatorplus_findannotationtag(tmp);
							if (pAnnotatedTagLabel)
								CharacterCreatorAnnotatedTagFeet_s.insert(std::make_pair(tmp, pAnnotatedTagLabel));
							else
								CharacterCreatorAnnotatedTagFeet_s.insert(std::make_pair(tmp, ""));
							if (strlen(cSelectedFeet) == 0) strcpy(cSelectedFeet, tmp);
						}
						else if (pestrcasestr(tfile_s.Get(), " hair "))
						{
							CharacterCreatorHair_s.insert(std::make_pair(tmp, pPartsPath));
							LPSTR pAnnotatedLabel = charactercreatorplus_findannotation(tmp);
							if (pAnnotatedLabel)
								CharacterCreatorAnnotatedHair_s.insert(std::make_pair(tmp, pAnnotatedLabel));
							else
								CharacterCreatorAnnotatedHair_s.insert(std::make_pair(tmp, tmp));
							LPSTR pAnnotatedTagLabel = charactercreatorplus_findannotationtag(tmp);
							if (pAnnotatedTagLabel)
								CharacterCreatorAnnotatedTagHair_s.insert(std::make_pair(tmp, pAnnotatedTagLabel));
							else
								CharacterCreatorAnnotatedTagHair_s.insert(std::make_pair(tmp, ""));
							if (strlen(cSelectedHair) == 0) strcpy(cSelectedHair, tmp);
						}
						else if (pestrcasestr(tfile_s.Get(), " head "))
						{
							CharacterCreatorHead_s.insert(std::make_pair(tmp, pPartsPath));
							LPSTR pAnnotatedLabel = charactercreatorplus_findannotation(tmp);
							if (pAnnotatedLabel)
								CharacterCreatorAnnotatedHead_s.insert(std::make_pair(tmp, pAnnotatedLabel));
							else
								CharacterCreatorAnnotatedHead_s.insert(std::make_pair(tmp, tmp));
							LPSTR pAnnotatedTagLabel = charactercreatorplus_findannotationtag(tmp);
							if (pAnnotatedTagLabel)
								CharacterCreatorAnnotatedTagHead_s.insert(std::make_pair(tmp, pAnnotatedTagLabel));
							else
								CharacterCreatorAnnotatedTagHead_s.insert(std::make_pair(tmp, ""));
							if (strlen(cSelectedHead) == 0) strcpy(cSelectedHead, tmp);
						}
						else if (pestrcasestr(tfile_s.Get(), " legs "))
						{
							CharacterCreatorLegs_s.insert(std::make_pair(tmp, pPartsPath));
							LPSTR pAnnotatedLabel = charactercreatorplus_findannotation(tmp);
							if (pAnnotatedLabel)
								CharacterCreatorAnnotatedLegs_s.insert(std::make_pair(tmp, pAnnotatedLabel));
							else
								CharacterCreatorAnnotatedLegs_s.insert(std::make_pair(tmp, tmp));
							LPSTR pAnnotatedTagLabel = charactercreatorplus_findannotationtag(tmp);
							if (pAnnotatedTagLabel)
								CharacterCreatorAnnotatedTagLegs_s.insert(std::make_pair(tmp, pAnnotatedTagLabel));
							else
								CharacterCreatorAnnotatedTagLegs_s.insert(std::make_pair(tmp, ""));

							// if require no legs, always choose 01 for default
							if (strlen(cSelectedLegs) == 0)
							{
								bool bLegsFilterThisOut = false;
								if (stricmp(CCP_Type, "zombie male") != NULL && stricmp(CCP_Type, "zombie female") != NULL)
								{
									bLegsFilterThisOut = true;
									if (strstr(pOptionalLegsChoice, "01") != NULL)
									{
										if (strlen(cSelectedLegs) == 0 && tmp[strlen(tmp) - 1] == '1') bLegsFilterThisOut = false;
									}
									else
									{
										if (strlen(cSelectedLegs) == 0 && tmp[strlen(tmp) - 1] != '1') bLegsFilterThisOut = false;
									}
								}
								if (bLegsFilterThisOut == false)
								{
									strcpy(cSelectedLegs, tmp);
								}
							}
						}
						else if (pestrcasestr(tfile_s.Get(), " headgear "))
						{
							CharacterCreatorHeadGear_s.insert(std::make_pair(tmp, pPartsPath));
							LPSTR pAnnotatedLabel = charactercreatorplus_findannotation(tmp);
							if (pAnnotatedLabel)
								CharacterCreatorAnnotatedHeadGear_s.insert(std::make_pair(tmp, pAnnotatedLabel));
							else
								CharacterCreatorAnnotatedHeadGear_s.insert(std::make_pair(tmp, tmp));
							LPSTR pAnnotatedTagLabel = charactercreatorplus_findannotationtag(tmp);
							if (pAnnotatedTagLabel)
								CharacterCreatorAnnotatedTagHeadGear_s.insert(std::make_pair(tmp, pAnnotatedTagLabel));
							else
								CharacterCreatorAnnotatedTagHeadGear_s.insert(std::make_pair(tmp, ""));
							if (strlen(cSelectedHeadGear) == 0) strcpy(cSelectedHeadGear, tmp);
						}
						else if (pestrcasestr(tfile_s.Get(), " facialhair "))
						{
							CharacterCreatorFacialHair_s.insert(std::make_pair(tmp, pPartsPath));
							LPSTR pAnnotatedLabel = charactercreatorplus_findannotation(tmp);
							if (pAnnotatedLabel)
								CharacterCreatorAnnotatedFacialHair_s.insert(std::make_pair(tmp, pAnnotatedLabel));
							else
								CharacterCreatorAnnotatedFacialHair_s.insert(std::make_pair(tmp, tmp));
							LPSTR pAnnotatedTagLabel = charactercreatorplus_findannotationtag(tmp);
							if (pAnnotatedTagLabel)
								CharacterCreatorAnnotatedTagFacialHair_s.insert(std::make_pair(tmp, pAnnotatedTagLabel));
							else
								CharacterCreatorAnnotatedTagFacialHair_s.insert(std::make_pair(tmp, ""));
							if (strlen(cSelectedFacialHair) == 0) strcpy(cSelectedFacialHair, tmp);
						}
						else if (pestrcasestr(tfile_s.Get(), " eyeglasses "))
						{
							CharacterCreatorEyeglasses_s.insert(std::make_pair(tmp, pPartsPath));
							LPSTR pAnnotatedLabel = charactercreatorplus_findannotation(tmp);
							if (pAnnotatedLabel)
								CharacterCreatorAnnotatedEyeglasses_s.insert(std::make_pair(tmp, pAnnotatedLabel));
							else
								CharacterCreatorAnnotatedEyeglasses_s.insert(std::make_pair(tmp, tmp));
							LPSTR pAnnotatedTagLabel = charactercreatorplus_findannotationtag(tmp);
							if (pAnnotatedTagLabel)
								CharacterCreatorAnnotatedTagEyeglasses_s.insert(std::make_pair(tmp, pAnnotatedTagLabel));
							else
								CharacterCreatorAnnotatedTagEyeglasses_s.insert(std::make_pair(tmp, ""));
							if (strlen(cSelectedEyeglasses) == 0) strcpy(cSelectedEyeglasses, tmp);
						}
						else if (pestrcasestr(tfile_s.Get(), " accessory1 "))
						{
							CharacterCreatorAccessory1_s.insert(std::make_pair(tmp, pPartsPath));
							LPSTR pAnnotatedLabel = charactercreatorplus_findannotation(tmp);
							if (pAnnotatedLabel)
								CharacterCreatorAnnotatedAccessory1_s.insert(std::make_pair(tmp, pAnnotatedLabel));
							else
								CharacterCreatorAnnotatedAccessory1_s.insert(std::make_pair(tmp, tmp));
							LPSTR pAnnotatedTagLabel = charactercreatorplus_findannotationtag(tmp);
							if (pAnnotatedTagLabel)
								CharacterCreatorAnnotatedTagAccessory1_s.insert(std::make_pair(tmp, pAnnotatedTagLabel));
							else
								CharacterCreatorAnnotatedTagAccessory1_s.insert(std::make_pair(tmp, ""));
							if (strlen(cSelectedAccessory1) == 0) strcpy(cSelectedAccessory1, tmp);
						}
						else if (pestrcasestr(tfile_s.Get(), " accessory2 "))
						{
							CharacterCreatorAccessory2_s.insert(std::make_pair(tmp, pPartsPath));
							LPSTR pAnnotatedLabel = charactercreatorplus_findannotation(tmp);
							if (pAnnotatedLabel)
								CharacterCreatorAnnotatedAccessory2_s.insert(std::make_pair(tmp, pAnnotatedLabel));
							else
								CharacterCreatorAnnotatedAccessory2_s.insert(std::make_pair(tmp, tmp));
							LPSTR pAnnotatedTagLabel = charactercreatorplus_findannotationtag(tmp);
							if (pAnnotatedTagLabel)
								CharacterCreatorAnnotatedTagAccessory2_s.insert(std::make_pair(tmp, pAnnotatedTagLabel));
							else
								CharacterCreatorAnnotatedTagAccessory2_s.insert(std::make_pair(tmp, ""));
							if (strlen(cSelectedAccessory2) == 0) strcpy(cSelectedAccessory2, tmp);
						}
					}
				}
			}
		}
	}
	SetDir(olddir_s.Get());

	// if have default selection, set the choices to those
	int iBase = 0;
	std::array<std::string, 10>* storage = nullptr;
	if (stricmp(CCP_Type, "adult male") == NULL)
	{
		iBase = 1;
		if(g_maleStorage[0].length() > 0)
			storage = &g_maleStorage;
		g_fCCPZoom = 73.0f;
	}
	if (stricmp(CCP_Type, "adult female") == NULL) 
	{
		iBase = 2;
		if (g_femaleStorage[0].length() > 0)
			storage = &g_femaleStorage;
		g_fCCPZoom = 73.0f;
	}
	if (stricmp(CCP_Type, "zombie male") == NULL)
	{
		iBase = 3;
		if (g_zombieStorage[0].length() > 0)
			storage = &g_zombieStorage;
		g_fCCPZoom = 67.0f;
	}
	if (stricmp(CCP_Type, "zombie female") == NULL)
	{
		iBase = 4;
		if (g_zombieStorage[0].length() > 0)
			storage = &g_zombieStorage;
		g_fCCPZoom = 67.0f;
	}
	if (iBase == 0)
	{
		// search for custom char type
		iBase = GetBaseValueFromCCPType(CCP_Type);
		if (iBase > 0)
		{
			if (g_genericStorage[0].length() > 0)
				storage = &g_genericStorage;
			g_fCCPZoom = 67.0f;
		}
		else
		{
			iBase = 1;
		}
	}
	if (!storage)
	{
		char pDefault[32];
		char pDefaultVariant[32];
		strcpy(pDefaultVariant, "");
		charactercreatorplus_GetDefaultCharacterPartNum(iBase, 1, pDefault, pDefaultVariant);
		sprintf(cSelectedBody, "%s body %s", CCP_Type, pDefaultVariant);
		charactercreatorplus_GetDefaultCharacterPartNum(iBase, 2, pDefault, pDefaultVariant);
		sprintf(cSelectedHead, "%s head %s", CCP_Type, pDefaultVariant);
		charactercreatorplus_GetDefaultCharacterPartNum(iBase, 3, pDefault, pDefaultVariant);
		sprintf(cSelectedLegs, "%s legs %s", CCP_Type, pDefaultVariant);
		charactercreatorplus_GetDefaultCharacterPartNum(iBase, 4, pDefault, pDefaultVariant);
		sprintf(cSelectedFeet, "%s feet %s", CCP_Type, pDefaultVariant);
		charactercreatorplus_GetDefaultCharacterPartNum(iBase, 5, pDefault, pDefaultVariant);
		if(strlen(pDefaultVariant)>0)
			sprintf(cSelectedHair, "%s hair %s", CCP_Type, pDefaultVariant);
		else
			strcpy(cSelectedHair, "None");
	}
	else
	{
		// 0: Head Gear
		// 1: Hair
		// 2: Head
		// 3: Eye Glasses
		// 4: Facial Hair
		// 5: Body
		// 6: Legs
		// 7: Feet
		std::array<std::string, 10>& parts = *storage;
		strcpy(cSelectedHeadGear, parts[0].c_str());
		strcpy(cSelectedHair, parts[1].c_str());
		strcpy(cSelectedHead, parts[2].c_str());
		strcpy(cSelectedEyeglasses, parts[3].c_str());
		strcpy(cSelectedFacialHair, parts[4].c_str());
		strcpy(cSelectedBody, parts[5].c_str());
		strcpy(cSelectedLegs, parts[6].c_str());
		strcpy(cSelectedFeet, parts[7].c_str());
		strcpy(cSelectedAccessory1, parts[8].c_str());
		strcpy(cSelectedAccessory2, parts[9].c_str());
	
	}

	charactercreatorplus_initautoswaps();
	//g_pLastKnownTransition = &g_HeadTransition;
	if (iBase == 3 || iBase == 4)
	{
		g_pLastKnownTransition = &g_ZombieBodyTransition;
	}
	else
	{
		g_pLastKnownTransition = &g_UpperBodyTransition;
	}

	// configure default weapon choices
	g_grideleprof_holdchoices.hasweapon_s = "enhanced\\Mk19T";
	g_grideleprof_holdchoices.cantakeweapon = 0;
	g_grideleprof_holdchoices.quantity = 8;

	// output location for character
	LPSTR pCharacterFinal = "entitybank\\user\\charactercreatorplus\\character.dbo";

	// reset color selections
	for (int a = 0; a < 5; a++) vColorSelected[a] = ImVec4(0.0, 0.0, 0.0, 1.0);

	// generic textures
	int iCharTextureWhite = g.charactercreatorEditorImageoffset + 0;
	LoadImage("effectbank\\reloaded\\media\\blank_O.DDS", iCharTextureWhite);
	int iCharTextureBlack = g.charactercreatorEditorImageoffset + 1;
	if (!GetImageExistEx(iCharTextureBlack)) LoadImage("effectbank\\reloaded\\media\\blank_black.dds", iCharTextureBlack);

	// work out path to part files
	strcpy(pTempStr, "charactercreatorplus\\parts\\");
	strcat(pTempStr, CCP_Type);
	strcat(pTempStr, "\\");
	strcat(pTempStr, CCP_Type);
	strcat(pTempStr, " ");

	// texture index references
	int iCharTexture = g.charactercreatorEditorImageoffset + 1;
	int iCharHeadGearTexture = g.charactercreatorEditorImageoffset + 11;
	int iCharHairTexture = g.charactercreatorEditorImageoffset + 21;
	int iCharHeadTexture = g.charactercreatorEditorImageoffset + 31;
	int iCharEyeglassesTexture = g.charactercreatorEditorImageoffset + 41;
	int iCharFacialHairTexture = g.charactercreatorEditorImageoffset + 51;
	int iCharLegsTexture = g.charactercreatorEditorImageoffset + 61;
	int iCharFeetTexture = g.charactercreatorEditorImageoffset + 71;
	int iCharAccessory1Texture = g.charactercreatorEditorImageoffset + 81;
	int iCharAccessory2Texture = g.charactercreatorEditorImageoffset + 91;

	// load default skin type texture IC1a
	std::map<std::string, std::string>::iterator it = CharacterCreatorAnnotatedTagHead_s.begin();
	LPSTR pCorrectICCode = "IC1c"; // default
	for (it = CharacterCreatorAnnotatedTagHead_s.begin(); it != CharacterCreatorAnnotatedTagHead_s.end(); ++it)
	{
		std::string thistag = it->first;
		if (strnicmp (thistag.c_str(), cSelectedHead, strlen(cSelectedHead)) == NULL)
		{
			// found correct skin for this head
			LPSTR pNewSkinCode = (LPSTR)it->second.c_str();
			if (strlen(pNewSkinCode) > 0) pCorrectICCode = pNewSkinCode;
			break;
		}
	}
	int iCharSkinTexture = g.charactercreatorEditorImageoffset + 101;
	if (GetImageExistEx(iCharSkinTexture) == 1) DeleteImage(iCharSkinTexture);
	char pRelPathToSkinTexture[MAX_PATH];
	sprintf(pRelPathToSkinTexture, "charactercreatorplus\\skins\\%s.png", pCorrectICCode);
	LoadImage(pRelPathToSkinTexture, iCharSkinTexture);

	// Default body
	bool bNonStandardCharacter = false;
	char pPartNumStr[32];
	char pPartNumVariantStr[32];
	char pDefaultBody[32];
	char pDefaultBodyVariant[32];
	char pDefaultHair[32];
	char pDefaultHairVariant[32];
	char pDefaultHead[32];
	char pDefaultHeadVariant[32];
	char pDefaultLegs[32];
	char pDefaultLegsVariant[32];
	char pDefaultFeet[32];
	char pDefaultFeetVariant[32];

	char pDefaultHeadgear[32] = { 0 };
	char pDefaultHeadgearVariant[32] = { 0 };
	char pDefaultGlasses[32] = { 0 };
	char pDefaultGlassesVariant[32] = { 0 };
	char pDefaultFacialHair[32] = { 0 };
	char pDefaultFacialHairVariant[32] = { 0 };

	char pDefaultAccessory1[32] = { 0 };
	char pDefaultAccessory2[32] = { 0 };
	char pDefaultAccessory1Variant[32] = { 0 };
	char pDefaultAccessory2Variant[32] = { 0 };

	// Need to add default headgear, glasses and facial hair.
	iBase = 0;
	if (stricmp(CCP_Type, "adult male") == NULL) iBase = 1;
	if (stricmp(CCP_Type, "adult female") == NULL) iBase = 2;
	if (stricmp(CCP_Type, "zombie male") == NULL) iBase = 3;
	if (stricmp(CCP_Type, "zombie female") == NULL) iBase = 4;
	if (iBase == 0)	iBase = GetBaseValueFromCCPType(CCP_Type);
	if (!storage)
	{
		charactercreatorplus_GetDefaultCharacterPartNum(iBase, 1, pPartNumStr, pPartNumVariantStr);
		strcpy(pDefaultBody, pPartNumStr);
		strcpy(pDefaultBodyVariant, pPartNumVariantStr);

		charactercreatorplus_GetDefaultCharacterPartNum(iBase, 5, pPartNumStr, pPartNumVariantStr);
		strcpy(pDefaultHair, pPartNumStr);
		strcpy(pDefaultHairVariant, pPartNumVariantStr);

		charactercreatorplus_GetDefaultCharacterPartNum(iBase, 2, pPartNumStr, pPartNumVariantStr);
		strcpy(pDefaultHead, pPartNumStr);
		strcpy(pDefaultHeadVariant, pPartNumVariantStr);

		charactercreatorplus_GetDefaultCharacterPartNum(iBase, 3, pPartNumStr, pPartNumVariantStr);
		strcpy(pDefaultLegs, pPartNumStr);
		strcpy(pDefaultLegsVariant, pPartNumVariantStr);

		charactercreatorplus_GetDefaultCharacterPartNum(iBase, 4, pPartNumStr, pPartNumVariantStr);
		strcpy(pDefaultFeet, pPartNumStr);
		strcpy(pDefaultFeetVariant, pPartNumVariantStr);
	}
	else
	{
		// Instead of using defaults, use the users previous choice for this base type.
		std::array<std::string, 10>& parts = *storage;
		charactercreatorplus_extractpartnumberandvariation(parts[0].c_str(), pDefaultHeadgear, pDefaultHeadgearVariant);
		charactercreatorplus_extractpartnumberandvariation(parts[1].c_str(), pDefaultHair, pDefaultHairVariant);
		charactercreatorplus_extractpartnumberandvariation(parts[2].c_str(), pDefaultHead, pDefaultHeadVariant);
		charactercreatorplus_extractpartnumberandvariation(parts[3].c_str(), pDefaultGlasses, pDefaultGlassesVariant);
		charactercreatorplus_extractpartnumberandvariation(parts[4].c_str(), pDefaultFacialHair, pDefaultFacialHairVariant);
		charactercreatorplus_extractpartnumberandvariation(parts[5].c_str(), pDefaultBody, pDefaultBodyVariant);
		charactercreatorplus_extractpartnumberandvariation(parts[6].c_str(), pDefaultLegs, pDefaultLegsVariant);
		charactercreatorplus_extractpartnumberandvariation(parts[7].c_str(), pDefaultFeet, pDefaultFeetVariant);
		charactercreatorplus_extractpartnumberandvariation(parts[8].c_str(), pDefaultAccessory1, pDefaultAccessory1Variant);
		charactercreatorplus_extractpartnumberandvariation(parts[9].c_str(), pDefaultAccessory2, pDefaultAccessory2Variant);
	}

	// load all body parts
	for (int iPartID = 0; iPartID < 10; iPartID++)
	{
		// work out which part
		int iThisObj = 0;
		int iThisTexture = 0;
		LPSTR pPartName = "";
		LPSTR pPartNum = "";
		LPSTR pPartNumVariant = "";
		if (iPartID == 0) { iThisObj = iCharObj; iThisTexture = iCharTexture; pPartName = "body"; pPartNum = pDefaultBody; pPartNumVariant = pDefaultBodyVariant; }
		if (iPartID == 1) 
		{ 
			if (storage)
			{
				char* pHeadgear = "";
				if (strlen(pDefaultHeadgear) > 0)
					pHeadgear = "headgear";
				iThisObj = iCharObjHeadGear; iThisTexture = iCharHeadGearTexture; pPartName = pHeadgear; pPartNum = pDefaultHeadgear; pPartNumVariant = pDefaultHeadgearVariant;
			}
			else
			{
				iThisObj = iCharObjHeadGear; iThisTexture = iCharHeadGearTexture; pPartName = ""; pPartNum = ""; pPartNumVariant = "";
			}
		}
		if (iPartID == 2) 
		{ 
			iThisObj = iCharObjHair; 
			iThisTexture = iCharHairTexture; 
			pPartName = "hair"; pPartNum = pDefaultHair; 
			pPartNumVariant = pDefaultHairVariant; 
			if (strlen(pPartNum) == 0)
				pPartName = "";

		}
		if (iPartID == 3) { iThisObj = iCharObjHead; iThisTexture = iCharHeadTexture; pPartName = "head"; pPartNum = pDefaultHead; pPartNumVariant = pDefaultHeadVariant; }
		if (iPartID == 4) 
		{ 
			if (storage)
			{
				char* pGlasses = "";
				if (strlen(pDefaultGlasses) > 0)
					pGlasses = "eyeglasses";
				iThisObj = iCharObjEyeglasses; iThisTexture = iCharEyeglassesTexture; pPartName = pGlasses; pPartNum = pDefaultGlasses; pPartNumVariant = pDefaultGlassesVariant;
			}
			else
			{
				iThisObj = iCharObjEyeglasses; iThisTexture = iCharEyeglassesTexture; pPartName = ""; pPartNum = ""; pPartNumVariant = "";
			}
		}
		if (iPartID == 8)
		{
			if (storage)
			{
				char* pAccessory = "";
				if (strlen(pDefaultAccessory1) > 0)
					pAccessory = "accessory1";
				iThisObj = iCharObjAccessory1; iThisTexture = iCharAccessory1Texture; pPartName = pAccessory; pPartNum = pDefaultAccessory1; pPartNumVariant = pDefaultAccessory1Variant;
			}
			else
			{
				iThisObj = iCharObjAccessory1; iThisTexture = iCharAccessory1Texture; pPartName = ""; pPartNum = ""; pPartNumVariant = "";
			}
		}
		if (iPartID == 9)
		{
			if (storage)
			{
				char* pAccessory = "";
				if (strlen(pDefaultAccessory2) > 0)
					pAccessory = "accessory2";
				iThisObj = iCharObjAccessory1; iThisTexture = iCharAccessory2Texture; pPartName = pAccessory; pPartNum = pDefaultAccessory2; pPartNumVariant = pDefaultAccessory2Variant;
			}
			else
			{
				iThisObj = iCharObjAccessory2; iThisTexture = iCharAccessory2Texture; pPartName = ""; pPartNum = ""; pPartNumVariant = "";
			}
		}
		if (iPartID == 5) 
		{ 
			if (storage)
			{
				char* pFacialHair = "";
				if (strlen(pDefaultFacialHair) > 0)
					pFacialHair = "facialhair";
				iThisObj = iCharObjFacialHair; iThisTexture = iCharFacialHairTexture; pPartName = pFacialHair; pPartNum = pDefaultFacialHair; pPartNumVariant = pDefaultFacialHairVariant;
			}
			else
			{
				iThisObj = iCharObjFacialHair; iThisTexture = iCharFacialHairTexture; pPartName = ""; pPartNum = ""; pPartNumVariant = "";
			}
		}
		if (iPartID == 6) 
		{ 
			iThisObj = iCharObjLegs; 
			iThisTexture = iCharLegsTexture; 
			pPartName = "legs"; 
			pPartNum = pDefaultLegs; 
			pPartNumVariant = pDefaultLegsVariant; 	
			std::map<std::string, std::string>::iterator it = CharacterCreatorAnnotatedTagBody_s.begin();
			LPSTR pCorrpBodyNoLegsTag = (LPSTR)it->second.c_str();
			if ( strstr ( pCorrpBodyNoLegsTag, "No Legs" ) != NULL )
				pPartNum = "01"; 
		}
		if (iPartID == 7) { iThisObj = iCharObjFeet; iThisTexture = iCharFeetTexture; pPartName = "feet"; pPartNum = pDefaultFeet; pPartNumVariant = pDefaultFeetVariant; }

		// load part object, textures and apply effect
		if (strlen(pPartName) > 0)
		{
			char pPartFile[260];
			strcpy(pPartFile, pPartName);
			strcat(pPartFile, " ");
			strcat(pPartFile, pPartNum); // i.e. 01
			LoadObject(cstr(cstr(pTempStr) + pPartFile + CCPMODELEXT).Get(), iThisObj);
			char pVariantPartFile[260];
			strcpy(pVariantPartFile, pPartName);
			strcat(pVariantPartFile, " ");
			strcat(pVariantPartFile, pPartNumVariant); // i.e. 01c
			cstr pPathVariant = cstr(pTempStr) + pVariantPartFile;
			cstr pPath = cstr(pTempStr) + pPartFile;
			charactercreatorplus_loadccimages(pPathVariant.Get(), pPath.Get(), iThisTexture);
			charactercreatorplus_textureccimages(iThisObj, iThisTexture);
			// and need mask for reskin
			if (ImageExist(iThisTexture + 1) == 1) DeleteImage(iThisTexture + 1);
			LoadImage(cstr(cstr(pTempStr) + pPartFile + "_mask.dds").Get(), iThisTexture + 1);
			if (ImageExist(iThisTexture + 1) == 0)
			{
				// remove color-specific from part and look for base mask for this body part
				char pCropPartFile[MAX_PATH];
				strcpy(pCropPartFile, pPartFile);
				LPSTR pLastChar = pCropPartFile + strlen(pCropPartFile) - 1;
				if (*pLastChar >= 'a' && *pLastChar <= 'z') *pLastChar = 0;
				LoadImage(cstr(cstr(pTempStr) + pCropPartFile + "_mask.dds").Get(), iThisTexture + 1);
			}
		}
	}

	// wicked not using custom character shader, so create a new albedo/color texture
	// using template mask and skin color texture to produce correct albedo representing skin
	// only needed to do for body, legs and feet
	if (bNonStandardCharacter == false)
	{
		charactercreatorplus_refreshskincolor();
	}

	// remove wicked resources before modifying objects below
	if ( ObjectExist(iCharObj) ) WickedCall_RemoveObject( GetObjectData(iCharObj) );
	if ( ObjectExist(iCharObjHeadGear) ) WickedCall_RemoveObject( GetObjectData(iCharObjHeadGear) );
	if ( ObjectExist(iCharObjHead) ) WickedCall_RemoveObject( GetObjectData(iCharObjHead) );
	if ( ObjectExist(iCharObjLegs) ) WickedCall_RemoveObject( GetObjectData(iCharObjLegs) );
	if ( ObjectExist(iCharObjFeet) ) WickedCall_RemoveObject( GetObjectData(iCharObjFeet) );
	if ( ObjectExist(iCharObjHair) ) WickedCall_RemoveObject( GetObjectData(iCharObjHair) );
	if ( ObjectExist(iCharObjEyeglasses) ) WickedCall_RemoveObject( GetObjectData(iCharObjEyeglasses) );
	if ( ObjectExist(iCharObjFacialHair) ) WickedCall_RemoveObject( GetObjectData(iCharObjFacialHair) );
	if (ObjectExist(iCharObjAccessory1)) WickedCall_RemoveObject(GetObjectData(iCharObjAccessory1));
	if (ObjectExist(iCharObjAccessory2)) WickedCall_RemoveObject(GetObjectData(iCharObjAccessory2));

	// meshes are useless once they have been stolen from (preload system allows fresh loading to be near instant however)
	if (ObjectExist(iCharObjHeadGear) == 1)
	{
		StealMeshesFromObject(iCharObj, iCharObjHeadGear);
		DeleteObject(iCharObjHeadGear);
	}
	StealMeshesFromObject(iCharObj, iCharObjHead);
	DeleteObject(iCharObjHead);
	StealMeshesFromObject(iCharObj, iCharObjLegs);
	DeleteObject(iCharObjLegs);
	StealMeshesFromObject(iCharObj, iCharObjFeet);
	DeleteObject(iCharObjFeet);
	if (ObjectExist(iCharObjHair) == 1)
	{
		// ensure hair has no culling and semi-transparent
		SetObjectTransparency(iCharObjHair, 2);
		SetObjectCull(iCharObjHair, 0);
		// wicked handles this differently
		StealMeshesFromObject(iCharObj, iCharObjHair);
		DeleteObject(iCharObjHair);
	}
	if (ObjectExist(iCharObjEyeglasses) == 1)
	{
		SetObjectCull(iCharObjEyeglasses, 0);
		DisableObjectZWriteEx(iCharObjEyeglasses, true);
		SetObjectTransparency(iCharObjEyeglasses, 2);
		StealMeshesFromObject(iCharObj, iCharObjEyeglasses);
		DeleteObject(iCharObjEyeglasses);
	}
	if (ObjectExist(iCharObjFacialHair) == 1)
	{
		SetObjectTransparency(iCharObjFacialHair, 2);
		SetObjectCull(iCharObjFacialHair, 0);
		StealMeshesFromObject(iCharObj, iCharObjFacialHair);
		DeleteObject(iCharObjFacialHair);
	}

	if (ObjectExist(iCharObjAccessory1) == 1)
	{
		StealMeshesFromObject(iCharObj, iCharObjAccessory1);
		DeleteObject(iCharObjAccessory1);
	}
	if (ObjectExist(iCharObjAccessory2) == 1)
	{
		StealMeshesFromObject(iCharObj, iCharObjAccessory2);
		DeleteObject(iCharObjAccessory2);
	}

	// as character parts have no animations, wipe out ones they do have
	// and replace with the latest animation set for this base mesh
	cstr final_name = "charactercreatorplus\\animations\\sets\\";
	final_name = final_name + CCP_Type;
	final_name = final_name +"\\default animations" + CCPMODELEXT;
	if (FileExist(final_name.Get()))
	{
		AppendObject(final_name.Get(), iCharObj, 0);
	}
	// and trigger animation to be prepped
	g_bCharacterCreatorPrepAnims = true;

	// some DBOs are created with BLACK base color, so for character creator
	// objects set them always to WHITE with full ALPHA
	SetObjectDiffuseEx(iCharObj, 0xFFFFFFFF, 0);

	// character gone through extensive changes, ensure wicked is updated
	sObject* pObjectToRecreateInWicked = GetObjectData(iCharObj);
	WickedCall_AddObject( pObjectToRecreateInWicked );
	WickedCall_UpdateObject ( pObjectToRecreateInWicked );
	WickedCall_TextureObject ( pObjectToRecreateInWicked, NULL);
	// and set character to use full reflectance (as all body parts have alpha data in surface texture)
	// PE: reduced from 1.0 to 0.002 as it gives a bluish shine on the edges of the ccp.
	for (int iMeshIndex = 0; iMeshIndex < pObjectToRecreateInWicked->iMeshCount; iMeshIndex++)
	{
		//LB: Changed back to 0.04 reflectance to solve washed out look on surfaces with no/minimal normal map
		//WickedCall_SetReflectance(pObjectToRecreateInWicked->ppMeshList[iMeshIndex], 1.0f);
		char * pNameFromTexture = pObjectToRecreateInWicked->ppMeshList[iMeshIndex]->pTextures[0].pName;
		bool bNeedReflectance = false;
		if (pNameFromTexture && pestrcasestr(pNameFromTexture, "glasses")) bNeedReflectance = true;
		if (bNeedReflectance)
			WickedCall_SetReflectance(pObjectToRecreateInWicked->ppMeshList[iMeshIndex], 1.0f);
		else
			WickedCall_SetReflectance(pObjectToRecreateInWicked->ppMeshList[iMeshIndex], 0.04f);
	}

	// place character in scene
	float terrain_height = BT_GetGroundHeight(t.terrain.TerrainID, GGORIGIN_X, GGORIGIN_Z, 1);
	fCharObjectY = terrain_height;
	PositionObject(iCharObj, ccpObjTargetX, ccpObjTargetY, ccpObjTargetZ);
	RotateObject(iCharObj, ccpObjTargetAX, ccpObjTargetAY, ccpObjTargetAZ);
	SetObjectArtFlags(iCharObj, (1 << 1) + (0), 0);
	LoopObject(iCharObj, 15, 55);

	// set default object animation speed
	SetObjectSpeed(iCharObj, 100);

	// if it highly likely new users will go from male, female, girl and boy,
	// so anticipate this by preloading the default sets of each just after a type selected
	charactercreatorplus_preloadallcharacterpartchoices(); 

	// finished legacy loading
	image_setlegacyimageloading(false);

	g_MeshesThatNeedDoubleSided.clear();
	if (stricmp(CCP_Type, "adult male") == 0)
	{
		g_MeshesThatNeedDoubleSided.push_back("adult male headgear 18");
		g_MeshesThatNeedDoubleSided.push_back("adult male body 20");
		g_MeshesThatNeedDoubleSided.push_back("adult male legs 16");
		g_MeshesThatNeedDoubleSided.push_back("adult male legs 17");
		g_MeshesThatNeedDoubleSided.push_back("adult male legs 18");
		g_MeshesThatNeedDoubleSided.push_back("adult male headgear 20");
		g_MeshesThatNeedDoubleSided.push_back("adult male headgear 19");
		g_MeshesThatNeedDoubleSided.push_back("adult male body 21");
	}
	else if (stricmp(CCP_Type, "adult female") == 0)
	{
		g_MeshesThatNeedDoubleSided.push_back("adult female headgear 11");
		g_MeshesThatNeedDoubleSided.push_back("adult female body 14");
	}

	// Set default character behavior
	char script[260];
	switch (iBase)
	{
		case 1: strcpy(script, "character_attack.lua");
			break;
		case 2: strcpy(script, "character_attack.lua");
			break;
		case 3: strcpy(script, "zombie_attack.lua");
			break;
		case 4: strcpy(script, "zombie_attack.lua");
			break;
		default: strcpy(script, "patrol.lua");
			break;
	}
	strcpy(CCP_Script, script);
}

void charactercreatorplus_init(void)
{
	// Initialisation prompt 
	timestampactivity ( 0, "Start character creator plus initialisation" );
	g_CharacterCreatorPlus.bInitialised = true;

	// create voice list for choices
	pCCPVoiceSet = "";
	CCP_SelectedToken = 0;
	if (CreateListOfVoices() > 0)
	{
		pCCPVoiceSet = g_voiceList_s[0].Get();
		CCP_SelectedToken = g_voicetoken[0];
	}

	// hide other editor stuff
	terrain_paintselector_hide();
	waypoint_hideall ( );

	// load in base mesh list, objects and texture for initial character
	charactercreatorplus_refreshtype();

	// finished
	timestampactivity ( 0, "Finished character creator plus initialisation" );
}

void charactercreatorplus_free(void)
{
	// show editor stuff
	waypoint_showall();

	// deactivate character creator plus
	g_bCharacterCreatorPlusActivated = false;
}

bool charactercreatorplus_savecharacterentity ( int iCharObj, LPSTR pOptionalDBOSaveFile, int iThumbnailImage )
{
	// saves character FPE, DBO and BMP from current character
	char pEntityName[1024];
	cstr FPEFile_s, BMPFile_s, DBOFile_s;
	strcpy ( pEntityName, "" );
	if ( pOptionalDBOSaveFile != NULL )
	{
		strcpy ( pEntityName, pOptionalDBOSaveFile );
		pEntityName[strlen(pEntityName)-4] = 0;
		FPEFile_s = cstr(pEntityName)+".fpe";
		BMPFile_s = cstr(pEntityName)+".bmp";
		DBOFile_s = cstr(pEntityName)+".dbo";
	}

	// store old dir
	cstr olddir_s = GetDir();

	// check if user folder exists, if not create it
	if ( PathExist( cstr( g.fpscrootdir_s+"\\Files\\entitybank\\user").Get() )  ==  0 ) 
	{
		MakeDirectory ( cstr(g.fpscrootdir_s+"\\Files\\entitybank\\user").Get() );
	}
	if ( PathExist( cstr(g.fpscrootdir_s+"\\Files\\entitybank\\user\\charactercreatorplus").Get() )  ==  0 ) 
	{
		MakeDirectory ( cstr(g.fpscrootdir_s+"\\Files\\entitybank\\user\\charactercreatorplus").Get() );
	}

	// allow mouse for file dialog
	ShowMouse (  );

	// save dialog
	cstr SaveFile_s = "";
	if ( pOptionalDBOSaveFile == NULL )
	{
		int iInSaveDialog = 1;
		while ( iInSaveDialog == 1 ) 
		{
			SaveFile_s = openFileBox("FPSC Entity (.fpe)|*.fpe|All Files|*.*|", cstr(g.fpscrootdir_s+"\\Files\\entitybank\\user\\charactercreatorplus\\").Get(), "Save Character", ".fpe", CHARACTERKITSAVEFILE);
			if ( SaveFile_s == "Error" ) return false;
			iInSaveDialog = 0;
		}
		strcpy ( pEntityName, SaveFile_s.Get() );
		pEntityName[strlen(pEntityName)-4] = 0;
		FPEFile_s = cstr(pEntityName)+".fpe";
		BMPFile_s = cstr(pEntityName)+".bmp";
		DBOFile_s = cstr(pEntityName)+".dbo";
	}

	// get character name
	cstr CharacterName_s = "";
	for ( int n = strlen(pEntityName); n >= 1; n-- )
	{
		if ( pEntityName[n] == '\\' || pEntityName[n] == '/' ) 
		{
			CharacterName_s = pEntityName + n + 1;
			break;
		}
	}

	// wicked uses non-cusotm shader for character skin, so copy the prebaked skin to the export area as the required
	// amended textures for the character (better as it does not put extra burden on the shader!)
	// go through all meshes
	sObject* pSaveObject = GetObjectData(iCharObj);
	for ( int iM = 0; iM<pSaveObject->iMeshCount; iM++)
	{
		// for each valid mesh
		sMesh* pMesh = pSaveObject->ppMeshList[iM];
		if (pMesh)
		{
			// scan to see if a modified albedo is being used
			LPSTR pMeshAlbedoTexture = pMesh->pTextures[0].pName;
			if (pMeshAlbedoTexture && strstr(pMeshAlbedoTexture,"charactercreatorplus\\skins")!=NULL)
			{
				// if so, copy it to the export area under the same name as the new entity name
				char pExt[MAX_PATH];
				strcpy(pExt, pMeshAlbedoTexture);
				strrev(pExt);
				pExt[5] = 0;
				strrev(pExt);
				cstr sColorFile = cstr(pEntityName) + pExt;

				// but to real path
				char pRealColorFile[MAX_PATH];
				strcpy(pRealColorFile, sColorFile.Get());
				GG_GetRealPath(pRealColorFile, 1);
				CopyFileA(pMeshAlbedoTexture, pRealColorFile, FALSE);

				// and then redirect the object to that texture, so when it is saved, it points
				// to the local texture, and not the temp one created inside "charactercreatorplus\skins"
				strcpy ( pMesh->pTextures[0].pName, sColorFile.Get() );
			}
		}
	}

	// before save, match created animsets to actual DBO structure
	extern void UpdateObjectWithAnimSlotList (sObject* pObject);
	UpdateObjectWithAnimSlotList(pSaveObject);

	// save DBO at specified location
	if ( FileExist(DBOFile_s.Get()) == 1 ) DeleteAFile ( DBOFile_s.Get() );
	SaveObject ( DBOFile_s.Get(), iCharObj );

	// character template
	cstr copyFrom_s = g.fpscrootdir_s+"\\Files\\charactercreatorplus\\Uber Character.fpe";

	// prepare destination file
    cstr copyTo_s = FPEFile_s;
	if ( FileExist(FPEFile_s.Get()) == 1 ) DeleteAFile ( FPEFile_s.Get() );
	if ( FileOpen(1) == 1 ) CloseFile(1);
	if ( FileOpen(2) == 1 ) CloseFile(2);
	OpenToRead ( 1, copyFrom_s.Get() );
	OpenToWrite ( 2, copyTo_s.Get() );

	// go through all source FPE
	int iCount = 0;
	while ( FileEnd(1) == 0 ) 
	{
		// get line by line
		bool bSkipWritingReadLine = false;
		cstr line_s = ReadString ( 1 );

		// update description
		if ( cstr(Lower(Left(line_s.Get(),4))) == "desc" ) line_s = cstr("desc             = ") + CharacterName_s;

		// write out how this character was made up		
		if ( cstr(Lower(Left(line_s.Get(),11))) == "ccpassembly" )
		{
			cstr pCCPAssemblyString = "";
			for (int partscan = 0; partscan < 10; partscan++)
			{
				char pTrunc[260];
				if (partscan == 0) strcpy(pTrunc, cSelectedHeadGear);
				if (partscan == 1) strcpy(pTrunc, cSelectedHair);
				if (partscan == 2)
				{
					strcpy(pTrunc, cSelectedHead);
					strcat(pTrunc, " [");
					strcat(pTrunc, cSelectedICCode);
					strcat(pTrunc, "]");
				}
				if (partscan == 3) strcpy(pTrunc, cSelectedEyeglasses);
				if (partscan == 4) strcpy(pTrunc, cSelectedFacialHair);
				if (partscan == 5) strcpy(pTrunc, cSelectedBody);
				if (partscan == 6) strcpy(pTrunc, cSelectedLegs);
				if (partscan == 7) strcpy(pTrunc, cSelectedFeet);
				if (partscan == 8) strcpy(pTrunc, cSelectedAccessory1);
				if (partscan == 9) strcpy(pTrunc, cSelectedAccessory2);

				if (stricmp(pTrunc, "none") != NULL)
				{
					pCCPAssemblyString += pTrunc;
					if (partscan < 9) pCCPAssemblyString += ",";
				}
			}
			WriteString ( 2, cstr(cstr("ccpassembly      = ") + pCCPAssemblyString).Get() );
			bSkipWritingReadLine = true;
		}

		// replace some fields for destination
		if ( cstr(Lower(Left(line_s.Get(),6))) == "aimain" )
		{
			// main behavior
			WriteString ( 2, cstr(cstr("aimain           = ") + g_CharacterCreatorPlus.obj.settings.script_s).Get() );
			WriteString ( 2, cstr(cstr("voice            = ") + g_CharacterCreatorPlus.obj.settings.voice_s).Get() );
			WriteString ( 2, cstr(cstr("speakrate        = ") + cstr(g_CharacterCreatorPlus.obj.settings.iSpeakRate)).Get() );

			// behavior
			if (stricmp (g_CharacterCreatorPlus.obj.settings.script_s.Get(), "people\\zombie_attack.lua") == NULL)
			{
				// sounds
				WriteString (2, "soundset         = audiobank\\characters\\zombie\\ZombieMunchRatLoop.wav");
				WriteString (2, "soundset1        = audiobank\\characters\\zombie\\ZombieRaiseHead.wav");
				WriteString (2, "soundset2        = audiobank\\characters\\zombie\\ZombieLungeAlt.wav");
				WriteString (2, "soundset3        = audiobank\\characters\\zombie\\ZombieDyingAlt.wav");
			}
			else
			{
				if (strstr (g_CharacterCreatorPlus.obj.settings.script_s.Get(), "attack"))
				{
					if (stricmp (g_CharacterCreatorPlus.obj.settings.voice_s.Get(), "Microsoft David Desktop - English (United States)") == NULL || strstr(g_CharacterCreatorPlus.obj.settings.voice_s.Get(), "David"))
					{
						WriteString (2, "soundset         = audiobank\\characters\\human\\male\\enemies_heard_something2.wav");
						WriteString (2, "soundset1        = audiobank\\characters\\human\\male\\enemyseesplayer1.wav");
						WriteString (2, "soundset2        = audiobank\\characters\\human\\male\\enemy_lost_sight1.wav");
						WriteString (2, "soundset3        = audiobank\\characters\\human\\male\\enemy_kills_player1.wav");
					}
					else
					{
						WriteString (2, "soundset         = audiobank\\characters\\human\\female\\enemies_heard_something3.wav");
						WriteString (2, "soundset1        = audiobank\\characters\\human\\female\\enemyseesplayer5.wav");
						WriteString (2, "soundset2        = audiobank\\characters\\human\\female\\enemy_lost_sight2.wav");
						WriteString (2, "soundset3        = audiobank\\characters\\human\\female\\enemy_kills_player1.wav");
					}
					// weapon details
					char pWeaponData[MAX_PATH];
					sprintf(pWeaponData, "hasweapon        = %s", g_grideleprof_holdchoices.hasweapon_s.Get()); WriteString (2, pWeaponData);
					sprintf(pWeaponData, "cantakeweapon    = %d", g_grideleprof_holdchoices.cantakeweapon); WriteString (2, pWeaponData);
					sprintf(pWeaponData, "quantity         = %d", g_grideleprof_holdchoices.quantity); WriteString (2, pWeaponData);
				}
				else
				{
					WriteString (2, "hasweapon        =");
					WriteString (2, "cantakeweapon    = 0");
					WriteString (2, "quantity         = 0");
				}
			}
			WriteString (2, "ragdoll          = 1");
			WriteString (2, "");
			WriteString (2, "; flesh");
			WriteString (2, "decalmax         = 1");
			WriteString (2, "decal0           = blood");
			WriteString (2, "strength         = 500");
			WriteString (2, "materialindex    = 6");
			bSkipWritingReadLine = true;
		}
		if ( cstr(Lower(Left(line_s.Get(),5))) == "model" )
		{
			WriteString ( 2, cstr(cstr("model            = ") + CharacterName_s + ".dbo").Get() );
			bSkipWritingReadLine = true;
		}
		if ( cstr(Lower(Left(line_s.Get(),8))) == "textured" )
		{
			WriteString ( 2, cstr(cstr("textured         = ") + "").Get() );
			bSkipWritingReadLine = true;
			//PE: Missing "textureref1 = " for save standalone.
		}
		if (cstr(Lower(Left(line_s.Get(), 5))) == "anim0")
		{
			int iIdleStart = 0;
			int iIdleFinish = 0;
			extern std::vector<sAnimSlotStruct> g_pAnimSlotList;
			for (int slot = 0; slot < (int)g_pAnimSlotList.size(); slot++)
			{
				char pLwr[MAX_PATH];
				strcpy (pLwr, g_pAnimSlotList[slot].pName);
				strlwr(pLwr);
				if (strstr (pLwr, "idle") != NULL)
				{
					iIdleStart = g_pAnimSlotList[slot].fStart;
					iIdleFinish = g_pAnimSlotList[slot].fFinish;
					break;
				}
			}
			char pCorrectIdleAnim[MAX_PATH];
			sprintf(pCorrectIdleAnim, "anim0            = %d,%d", iIdleStart, iIdleFinish);
			WriteString (2, pCorrectIdleAnim);
			bSkipWritingReadLine = true;
		}
		
		// write line (changed or not) to the destination FPE
		if ( bSkipWritingReadLine == false )
		{
			WriteString ( 2, line_s.Get() );
		}
	}

	// Prevent the light above the character from influencing the thumbnail.
	if (iLightIndex >= 0)
	{
		WickedCall_DeleteLight(iLightIndex);
		iLightIndex = -1;
	}

	// additionally, to retain material settings per mesh, save out mesh settings
	extern void imporer_save_multimeshsection(sObject* pObject, int iFileIndex);
	imporer_save_multimeshsection(pSaveObject, 2);

	// close file handling
	CloseFile ( 1 );
	CloseFile ( 2 );
	
	// save thumbnail file
	if (iThumbnailImage > 0)
	{
		if (ImageExist(iThumbnailImage) == 0)
		{
			image_setlegacyimageloading(true);
			LoadImage("editors\\uiv3\\ThumbnailTemplate.png", iThumbnailImage);
			image_setlegacyimageloading(false);
		}
		char pRealBMPFile[MAX_PATH];
		strcpy(pRealBMPFile, BMPFile_s.Get());
		GG_GetRealPath(pRealBMPFile, 1);
		if (FileExist(pRealBMPFile) == 1) DeleteAFile(pRealBMPFile);
		SaveImage(pRealBMPFile, iThumbnailImage);
	}

	// restore old dir
	SetDir ( olddir_s.Get() );

	// success
	return true;
}

void charactercreatorplus_loop(void)
{
	if ( g_CharacterCreatorPlus.bInitialised == false )
	{
		// set up for character editing
		charactercreatorplus_init();
	}
	else
	{
		if (t.inputsys.wheelmousemove != 0.0f)
		{
			g_fCCPZoom += (t.inputsys.wheelmousemove * ImGui::GetIO().DeltaTime * 250.0f);

			if (g_fCCPZoom > 100.0f)
				g_fCCPZoom = 100.0f;
			else if (g_fCCPZoom < 0.0f)
				g_fCCPZoom = 0.0f;

			charactercreatorplus_dozoom();
		}
	}
}

// Character Creator for Multiplayer

void characterkitplus_chooseOnlineAvatar ( void )
{
	// new 'simple' method, select from file requester 
	char pOldDir[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, pOldDir);
	SetDir(g.fpscrootdir_s.Get());
	SetDir("Files\\entitybank\\user\\charactercreatorplus");
	char pCCPDir[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, pCCPDir);
	t.tpickedAvatar_s = openFileBox("Character Files|*.bmp|", pCCPDir, "Select Character", ".bmp", 1);
	if (t.tpickedAvatar_s == "Error")
	{
		SetDir(pOldDir);
		return;
	}
	if (FileExist(t.tpickedAvatar_s.Get()) == 1)
	{
		t.tpickedAvatar_s = Right(t.tpickedAvatar_s.Get(), (Len(t.tpickedAvatar_s.Get()) - Len(pCCPDir))-1 );
		t.tpickedAvatar_s = Left(t.tpickedAvatar_s.Get(), Len(t.tpickedAvatar_s.Get()) - 4);
		SetDir(pOldDir);
		characterkitplus_saveAvatarInfo();
	}
	SetDir(pOldDir);
}

void characterkitplus_saveAvatarInfo ( void )
{
	t.tavatarstring_s = "";
	if ( FileExist( cstr(cstr("entitybank\\user\\charactercreatorplus\\")+t.tpickedAvatar_s+".fpe").Get() )  ==  1 ) 
	{
		// grab the character creator string
		t.tavatarstring_s = "";
		if ( FileOpen(1) == 1 ) CloseFile ( 1 );
		OpenToRead ( 1, cstr(cstr("entitybank\\user\\charactercreatorplus\\")+t.tpickedAvatar_s+".fpe").Get() );
		while ( FileEnd(1) == 0 ) 
		{
			t.tline_s = ReadString ( 1 );
			t.tcciStat_s = Lower(FirstToken( t.tline_s.Get(), " "));
			if ( t.tcciStat_s == "ccpassembly" ) 
			{
				LPSTR pStr = t.tline_s.Get();
				for (int n = 0; n < strlen(pStr); n++)
				{
					if (pStr[n] == '=')
					{
						n++; if (pStr[n] == ' ') n++;
						t.tavatarstring_s = pStr + n;
					}
				}
				break;
			}
		}
		CloseFile (  1 );

		// write out multiplayeravatar.dat file
		if ( FileExist( cstr(g.fpscrootdir_s + "\\multiplayeravatar.dat").Get() ) == 1 ) DeleteAFile ( cstr(g.fpscrootdir_s + "\\multiplayeravatar.dat").Get() );
		OpenToWrite ( 1, cstr(g.fpscrootdir_s + "\\multiplayeravatar.dat").Get() );
		WriteString ( 1,t.tavatarstring_s.Get() );
		WriteString ( 1,t.tpickedAvatar_s.Get() );
		CloseFile ( 1 );

		// load in the avatar for multiplayer
		characterkitplus_loadMyAvatarInfo();
	}
}

void characterkitplus_checkAvatarExists ( void )
{
	if ( FileOpen(1) == 1 ) CloseFile ( 1 );
	if ( FileExist( cstr(g.fpscrootdir_s + "\\multiplayeravatar.dat").Get() ) == 1 ) 
	{
		OpenToRead ( 1, cstr (g.fpscrootdir_s + "\\multiplayeravatar.dat").Get() );
		g.mp.myAvatar_s = ReadString ( 1 );
		g.mp.myAvatarHeadTexture_s = ReadString ( 1 );
		g.mp.myAvatarName_s = g.mp.myAvatarHeadTexture_s;
		CloseFile (  1 );
	}
}

void characterkitplus_loadMyAvatarInfo ( void )
{
	// blank out the data first
	g.mp.myAvatar_s = "";
	g.mp.myAvatarName_s = "";
	g.mp.myAvatarHeadTexture_s = "";
	g.mp.haveSentMyAvatar = 0;
	t.bTriggerAvatarRescanAndLoad = false;
	for ( t.c = 0 ; t.c <= MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{
		t.mp_playerAvatars_s[t.c] = "";
		t.mp_playerAvatarOwners_s[t.c] = "";
		t.mp_playerAvatarLoaded[t.c] = false;
	}

	// open multiplayer avatar file
	if ( FileExist( cstr(g.fpscrootdir_s + "\\multiplayeravatar.dat").Get() ) == 1 ) 
	{
		OpenToRead ( 1, cstr (g.fpscrootdir_s + "\\multiplayeravatar.dat").Get() );
		g.mp.myAvatar_s = ReadString ( 1 );
		g.mp.myAvatarHeadTexture_s = ReadString ( 1 );
		g.mp.myAvatarName_s = g.mp.myAvatarHeadTexture_s;
		CloseFile ( 1 );

		if ( t.tShowAvatarSprite == 1 ) 
		{
			t.tShowAvatarSprite = 0;
			if ( g.charactercreatorEditorImageoffset > 1 ) 
			{
				if ( ImageExist(g.charactercreatorEditorImageoffset) == 1 ) DeleteImage ( g.charactercreatorEditorImageoffset );
				if ( FileExist( cstr(g.fpscrootdir_s+"\\Files\\entitybank\\user\\charactercreatorplus\\"+g.mp.myAvatarName_s+".bmp").Get() ) == 1 ) 
				{
					LoadImage ( cstr(g.fpscrootdir_s+"\\Files\\entitybank\\user\\charactercreatorplus\\"+g.mp.myAvatarName_s+".bmp").Get(), g.charactercreatorEditorImageoffset );
				}
			}
		}
	}
}

void characterkitplus_makeMultiplayerCharacterCreatorAvatar ( void )
{
	// delete it if it exists (it shouldn't, but just in case)
	if ( FileExist(t.avatarFile_s.Get()) == 1 ) DeleteAFile ( t.avatarFile_s.Get() );

	// Store old dir
	t.tolddir_s=GetDir();

	// Check if user folder exists, if not create it
	if ( PathExist( cstr(g.fpscrootdir_s+"\\Files\\entitybank\\user").Get() ) == 0 ) 
	{
		MakeDirectory ( cstr(g.fpscrootdir_s+"\\Files\\entitybank\\user").Get() );
	}
	if ( PathExist( cstr(g.fpscrootdir_s+"\\Files\\entitybank\\user\\charactercreatorplus").Get() ) == 0 ) 
	{
		MakeDirectory ( cstr(g.fpscrootdir_s+"\\Files\\entitybank\\user\\charactercreatorplus").Get() );
	}

	// create correct name
	t.tSaveFile_s = t.avatarFile_s;
	t.tname_s = t.tSaveFile_s;
	if ( cstr(Lower(Right(t.tname_s.Get(),4))) == ".fpe"  )  t.tname_s = Left(t.tname_s.Get(),Len(t.tname_s.Get())-4);
	for ( t.tloop = Len(t.tname_s.Get()) ; t.tloop >= 1 ; t.tloop+= -1 )
	{
		if ( cstr(Mid(t.tname_s.Get(),t.tloop)) == "\\" || cstr(Mid(t.tname_s.Get(),t.tloop)) == "/" ) 
		{
			t.tname_s = Right(t.tname_s.Get(),Len(t.tname_s.Get())- t.tloop);
			break;
		}
	}

	// template reference
	t.tcopyfrom_s = g.fpscrootdir_s+"\\Files\\charactercreatorplus\\Uber Character.fpe";
	t.tcopyto_s = t.tSaveFile_s;
	if ( cstr(Lower(Right(t.tcopyto_s.Get(),4))) != ".fpe" ) t.tcopyto_s = t.tcopyto_s + ".fpe";

	// now modify the copy
	if ( FileOpen(1) ==  1 ) CloseFile ( 1 );
	if ( FileOpen(2) ==  1 ) CloseFile ( 2 );
	OpenToRead ( 1, t.tcopyfrom_s.Get() );
	OpenToWrite ( 2, t.tcopyto_s.Get() );
	while ( FileEnd(1) == 0 ) 
	{
		// line by line
		t.ts_s = ReadString ( 1 );
		if ( cstr(Lower(Left(t.ts_s.Get(),4))) == "desc" ) t.ts_s = cstr("desc           =  ") + t.tname_s;

		// replace model specified 
		if ( cstr(Lower(Left(t.ts_s.Get(),5))) == "model" ) t.ts_s = cstr("model          =  ") + t.tname_s + ".dbo";

		// replace ccpassembly field
		if ( cstr(Lower(Left(t.ts_s.Get(),11))) == "ccpassembly" ) t.ts_s = cstr(cstr("ccpassembly      = ") + t.avatarString_s).Get();

		// write back out
		WriteString ( 2, t.ts_s.Get() );
	}
	CloseFile ( 1 );
	CloseFile ( 2 );

	// created FPE, but have no DBO, so need to create one here
	char pTmpDBO[MAX_PATH];
	strcpy(pTmpDBO, t.tcopyto_s.Get());
	pTmpDBO[strlen(pTmpDBO)-4] = 0;
	strcat(pTmpDBO, ".dbo");

	// setup character parts, then trigger its creation (adult male hair 11,adult male head 01,adult male body 03,adult male legs 04e,adult male feet 04)
	iCharObj = g.characterkitobjectoffset + 1;
	iCharObjHeadGear = g.characterkitobjectoffset + 2;
	iCharObjHair = g.characterkitobjectoffset + 3;
	iCharObjHead = g.characterkitobjectoffset + 4;
	iCharObjEyeglasses = g.characterkitobjectoffset + 5;
	iCharObjFacialHair = g.characterkitobjectoffset + 6;
	iCharObjLegs = g.characterkitobjectoffset + 7;
	iCharObjFeet = g.characterkitobjectoffset + 8;
	iCharObjAccessory1 = g.characterkitobjectoffset + 9;
	iCharObjAccessory2 = g.characterkitobjectoffset + 10;


	strcpy(cSelectedLegsFilter, "");
	strcpy(cSelectedFeetFilter, "");
	strcpy(cSelectedICCode, "IC1a");
	strcpy(cSelectedHeadGear, "None");
	strcpy(cSelectedHair, "None");
	strcpy(cSelectedHead, "None");
	strcpy(cSelectedEyeglasses, "None");
	strcpy(cSelectedFacialHair, "None");
	strcpy(cSelectedBody, "None");
	strcpy(cSelectedLegs, "None");
	strcpy(cSelectedFeet, "None");
	strcpy(cSelectedAccessory1, "None");
	strcpy(cSelectedAccessory2, "None");
	
	char pICTag[MAX_PATH];
	strcpy(pICTag, "IC1a");
	char pBasePath[MAX_PATH];
	strcpy(pBasePath, "");
	char pAvatarAssembly[MAX_PATH];
	strcpy(pAvatarAssembly, t.avatarString_s.Get());
	int n = 0;
	while ( n <= strlen(pAvatarAssembly) )
	{
		bool bLastItem = false;
		if (n == strlen(pAvatarAssembly)) bLastItem = true;
		if ((n < strlen(pAvatarAssembly) && pAvatarAssembly[n] == ',') || bLastItem == true)
		{
			// get each part name
			char pPartName[MAX_PATH];
			strcpy(pPartName, pAvatarAssembly);
			pPartName[n] = 0;

			// find the part type
			if (strstr(pPartName, "headgear") != 0) strcpy(cSelectedHeadGear, pPartName);
			if (strstr(pPartName, "hair") != 0) strcpy(cSelectedHair, pPartName);
			if (strstr(pPartName, "head") != 0)
			{
				// head assembly name can include IC code (for avatar recreation)
				// get base
				strcpy(pBasePath, "charactercreatorplus\\parts\\");
				strcat(pBasePath, pPartName);
				LPSTR pHeadToken = strstr(pBasePath, " head");
				if (pHeadToken) *pHeadToken = 0;
				strcat(pBasePath, "\\");

				// find IC part
				LPSTR pICToken = strstr(pPartName, "[");
				if (pICToken)
				{
					// cut off IC part for selectedhead string
					strcpy(cSelectedHead, pPartName);
					LPSTR pICCutOff = strstr(cSelectedHead, " [");
					if (pICCutOff) *pICCutOff = 0;

					// and keep the IC part for the tag
					strcpy(pICTag, pICToken + 1);
					if (pICTag[strlen(pICTag) - 1] == ']')
						pICTag[strlen(pICTag) - 1] = 0;
				}
				else
				{
					strcpy(cSelectedHead, pPartName);
				}
			}
			if (strstr(pPartName, "eyeglasses") != 0) strcpy(cSelectedEyeglasses, pPartName);
			if (strstr(pPartName, "facialhair") != 0) strcpy(cSelectedFacialHair, pPartName);
			if (strstr(pPartName, "body") != 0) strcpy(cSelectedBody, pPartName);
			if (strstr(pPartName, "legs") != 0) strcpy(cSelectedLegs, pPartName);
			if (strstr(pPartName, "feet") != 0) strcpy(cSelectedFeet, pPartName);
			if (strstr(pPartName, "accessory1") != 0) strcpy(cSelectedAccessory1, pPartName);
			if (strstr(pPartName, "accessory2") != 0) strcpy(cSelectedAccessory2, pPartName);

			// prepare to get next one
			if (bLastItem == false)
			{
				strcpy(pAvatarAssembly, pAvatarAssembly + n + 1);
				n = 0;
			}
			else
			{
				strcpy(pAvatarAssembly, "");
				n = 1;
			}
		}
		else
		{
			n++;
		}
	}
	// now make a character object from parts
	image_preload_files_wait();
	object_preload_files_wait();
	charactercreatorplus_change(pBasePath, -1, pICTag);
	if (ObjectExist(iCharObj) == 1)
	{
		// save DBO at specified location
		if (FileExist(pTmpDBO) == 1) DeleteAFile(pTmpDBO);
		SaveObject(pTmpDBO, iCharObj);

		// and finally delete the unneeded object
		DeleteObject(iCharObj);
	}

	// Restore old dir
	SetDir(t.tolddir_s.Get());
}

void characterkitplus_removeMultiplayerCharacterCreatorAvatar(void)
{
	// remove temp FPE file
	if (FileExist(t.avatarFile_s.Get()) == 1) DeleteAFile(t.avatarFile_s.Get());

	// remove temp DBO file
	char pTmpDBO[MAX_PATH];
	strcpy(pTmpDBO, t.avatarFile_s.Get());
	pTmpDBO[strlen(pTmpDBO) - 4] = 0;
	strcat(pTmpDBO, ".dbo");
	if (FileExist(pTmpDBO) == 1) DeleteAFile(pTmpDBO);
}

void charactercreatorplus_initautoswaps()
{
	// Clear any existing auto swap data.
	g_headGearMandatorySwaps.clear();
	g_previousAutoSwap = nullptr;
	g_SkinTextureStorage[0] = 0;

	std::vector<std::string> autoSwapFiles;

	// Determine the directory to search for auto swap files, based on the currently selected base type.
	cstr olddir_s = GetDir();
	char newDirectory[260] = { 0 };

	if (stricmp(CCP_Type, "adult male") == NULL)
	{
		strcpy(newDirectory, "charactercreatorplus\\parts\\adult male");
	}
	else if (stricmp(CCP_Type, "adult female") == NULL)
	{
		strcpy(newDirectory, "charactercreatorplus\\parts\\adult female");
	}
	else if (stricmp(CCP_Type, "zombie male") == NULL)
	{
		strcpy(newDirectory, "charactercreatorplus\\parts\\zombie male");
	}
	else if (stricmp(CCP_Type, "zombie female") == NULL)
	{
		strcpy(newDirectory, "charactercreatorplus\\parts\\zombie female");
	}
	else
	{
		int iBaseValue = GetBaseValueFromCCPType(CCP_Type);
		if (iBaseValue > 1)
		{
			strcpy(newDirectory, "charactercreatorplus\\parts\\");
			strcat(newDirectory, g_CharacterType[iBaseValue-1].pPartsFolder);
		}
	}
	if (strlen(newDirectory) > 0)
	{
		SetDir(newDirectory);

		// Find all of the <abc>autoswaps.txt files.
		for (int c = 1; c <= ChecklistQuantity(); c++)
		{
			cStr tfile_s = Lower(ChecklistString(c));
			if (tfile_s != "." && tfile_s != "..")
			{
				if (strstr(tfile_s.Get(), "autoswaps"))
				{
					if (strcmp(Right(tfile_s.Get(), 4), ".txt") == 0)
					{
						std::string file(tfile_s.Get());
						autoSwapFiles.push_back(file);
					}
				}
			}
		}

		for (int i = 0; i < autoSwapFiles.size(); i++)
		{
			charactercreatorplus_getautoswapdata((char*)autoSwapFiles[i].c_str());
		}

		SetDir(olddir_s.Get());

	}
}

int current_dress_room = -999;
void change_dress_room(int charactertypeindex)
{
	// if charactertypeindex is negative, specifying roomID!
	int room = fabs(charactertypeindex);
	if (charactertypeindex>=0) room = g_CharacterTypeRoomPref[charactertypeindex];
	if (room != current_dress_room)
	{
		current_dress_room = room;
		strcpy(CCP_Room, g_RoomType[current_dress_room].pPartsFolder);

		iDressRoom = g.characterkitobjectoffset + 16;
		int iDressRoomImage = g.charactercreatorEditorImageoffset + 121;
		if (ObjectExist(iDressRoom)) DeleteObject(iDressRoom);
		if (ImageExist(iDressRoomImage)) DeleteImage(iDressRoomImage);

		// Choose the visual settings based on the dress room.
		visualsdatastoragetype newVisuals;
		if (iLightIndex >= 0)
		{
			WickedCall_DeleteLight(iLightIndex);
			iLightIndex = -1;
		}
		
		wiScene::MaterialComponent::SHADERTYPE shadertype = wiScene::MaterialComponent::SHADERTYPE_PBR;

		// switch to new room
		LPSTR pRomFolder = g_RoomType[current_dress_room].pPartsFolder;
		char pRoomFilePath[MAX_PATH];
		sprintf(pRoomFilePath, "charactercreatorplus\\rooms\\%s\\room.dbo", pRomFolder); LoadObject(pRoomFilePath, iDressRoom);
		sprintf(pRoomFilePath, "charactercreatorplus\\rooms\\%s\\room_color.dds", pRomFolder); LoadImage(pRoomFilePath, iDressRoomImage);
		newVisuals.SunIntensity_f = 0.0f;
		wiScene::Scene& scene = wiScene::GetScene();
		iLightIndex = scene.Entity_CreateLight("CCPLight", XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), 7, 550, wiScene::LightComponent::LightType::POINT);
		WickedCall_UpdateLight(iLightIndex, g_HeadTransition.to[0] + 100, g_HeadTransition.to[1] + 40, g_HeadTransition.to[2] - 110, 0, 0, 0, 550, 0, 255, 255, 255, true);

		// The current settings will not be stored, as they are already stored in t.visualsStorage, so create throwaway object.
		visualsdatastoragetype throwaway;
		set_temp_visuals(t.visuals, throwaway, newVisuals);
		visuals_loop();
		TextureObject(iDressRoom, 0, iDressRoomImage);

		sObject* pObject = GetObjectData(iDressRoom);
		if (pObject)
		{
			WickedCall_SetObjectCastShadows(pObject, false);
			for (int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++)
			{
				sMesh* pMesh = pObject->ppMeshList[iMesh];
				if (pMesh)
				{
					//SHADERTYPE_UNLIT need higher diffuse values ?
					pMesh->mMaterial.Diffuse.r = 1.0f;
					pMesh->mMaterial.Diffuse.g = 1.0f;
					pMesh->mMaterial.Diffuse.b = 1.0f;
					pMesh->mMaterial.Diffuse.a = 1.0f;
					wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pMesh->wickedmeshindex);
					if (mesh)
					{
						uint64_t materialEntity = mesh->subsets[0].materialID;
						wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
						if (pObjectMaterial)
						{
							pObjectMaterial->SetReflectance(0.0f);
							pObjectMaterial->shaderType = shadertype;
							
							//PE: Also ignoes all other material settings , so its perfect.
							pObjectMaterial->SetDirty(true);
						}
					}
					WickedCall_SetMeshMaterial(pMesh,true);
				}
			}
			WickedCall_SetObjectMetalness(pObject, 0.0f);
			WickedCall_SetObjectRoughness(pObject, 0.0f);
		}
	}
}

