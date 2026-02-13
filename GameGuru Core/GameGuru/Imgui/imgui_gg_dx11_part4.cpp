void RefreshEntityFolder(char* folder_s, void *pFolder)
{
	int tt = 0;
	cstr file_s = "";
	int fin = 0;
	cstr tempcstr;

	//pNewFolder->m_pFirstFile
	cFolderItem *pNewFolder = (cFolderItem *)pFolder;

	cstr OldDir = GetDir();
	if(pNewFolder && PathExist(folder_s) == 1)
	{
		int foldertype = pNewFolder->iType;

		//Delete everything inside pNewFolder->m_pFirstFile
		cFolderItem::sFolderFiles * m_pnextFiles, *m_pFiles = pNewFolder->m_pFirstFile;

		while (m_pFiles) 
		{
			m_pnextFiles = m_pFiles->m_pNext;
			if (m_pFiles->iPreview > 0) 
			{
				//Delete any old thumb images.
				//PE: Bug - tool icons can be used , so never delete m_pFiles->iPreview < UIV3IMAGES
				if ( m_pFiles->iPreview >= 4000 && GetImageExistEx(m_pFiles->iPreview) == 1 && m_pFiles->iPreview < UIV3IMAGES )
				{
					DeleteImage(m_pFiles->iPreview);
					m_pFiles->iPreview = 0;
				}
			}
			delete m_pFiles;
			m_pFiles = m_pnextFiles;
		}

		SetDir(folder_s);

		FindFirst(); fin = 0;

		std::vector<std::string> sorted_files;
		while (GetFileType() > -1)
		{
			file_s = GetFileName();
			if (file_s == "." || file_s == "..")
			{
				//  ignore . and ..
			}
			else
			{
				if (GetFileType() == 1)
				{
					//We only update the folder requested.
				}
				else
				{
					bool bValid = false;
					if (foldertype == 0 && pestrcasestr(file_s.Get(), ".fpe"))
						bValid = true;
					if (foldertype == 1 && pestrcasestr(file_s.Get(), ".wav"))
						bValid = true;
					if (foldertype == 1 && pestrcasestr(file_s.Get(), ".mp3"))
						bValid = true;
					if (foldertype == 1 && pestrcasestr(file_s.Get(), ".ogg"))
						bValid = true;

					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".png"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".dds"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".bmp"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".tif"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".jpg"))
						bValid = true;
					if (foldertype == 2 && pestrcasestr(file_s.Get(), ".gif"))
						bValid = true;

					if (foldertype == 3 && pestrcasestr(file_s.Get(), ".wmv"))
						bValid = true;
					//if (foldertype == 3 && pestrcasestr(file_s.Get(), ".ogv"))
					//	bValid = true;
					if (foldertype == 3 && pestrcasestr(file_s.Get(), ".mp4"))
						bValid = true;

					if (foldertype == 4 && pestrcasestr(file_s.Get(), ".lua"))
						bValid = true;

					if (foldertype == 5 && pestrcasestr(file_s.Get(), ".arx"))
						bValid = true;

					// The animation library was not displaying any animations, sorted_files was not populated with the animation files
					// This ensures that they are included
					//if (foldertype == 6 && pestrcasestr(file_s.Get(), ".dbo"))
					//	bValid = true;

					//PE: Should be .fpe, that also means that it displays the thumbs and you can preview animations :)
					if (foldertype == 6 && pestrcasestr(file_s.Get(), ".fpe"))
						bValid = true;



					if (bValid) {
						sorted_files.push_back(file_s.Get());

						//SAVE: time_create
						time_t ts = GetFileDateLong();
						cstr file = pNewFolder->m_sFolderFullPath;
						file = file + "\\" + file_s;

						//std::vector< std::pair<std::string, time_t> >::iterator its = files_time_stamp.begin();
						//for (; its != files_time_stamp.end(); ++its)
						//{
						//	if (its->first.size() > 0) {
						//		if (strcmp(its->first.c_str(), file.Get()) == 0)
						//		{
						//			files_time_stamp.erase(its);
						//			break;
						//		}
						//	}
						//}

						auto it = files_time_stamp.find(file.Get());
						if (it != files_time_stamp.end()) {
							files_time_stamp.erase(it);
						}

						//files_time_stamp.push_back(std::make_pair(file.Get(), ts));
						files_time_stamp[file.Get()] = ts;

					}
				}
			}
			FindNext();
			fin = fin + 1;
		}

		//sorted_files
		if (!sorted_files.empty()) 
		{
			std::sort(sorted_files.begin(), sorted_files.end(), NoCaseLess);

			std::vector<std::string>::iterator it = sorted_files.begin();

			if (it->size() > 0) 
			{
				cFolderItem::sFolderFiles *pNewItem = new cFolderItem::sFolderFiles;
				pNewItem->m_sName = "...";
				pNewItem->m_sNameFinal = pNewItem->m_sName;
				pNewItem->m_sNameFinalCredit = "";
				pNewItem->m_tFileModify = 0;
				pNewItem->m_Backdrop = "";
				pNewItem->bFavorite = false;
				pNewItem->bAvailableInFreeTrial = false;
				pNewItem->m_sPath = "";
				pNewItem->m_sFolder = "[na]";
				pNewItem->iFlags = 0;
				pNewItem->iPreview = 0;
				pNewItem->iBigPreview = 0;
				pNewItem->id = iTotalFiles++;
				pNewItem->bPreviewProcessed = false;
				pNewItem->m_pNext = NULL;
				pNewItem->m_pNextTime = NULL;
				pNewItem->m_pCustomSort = NULL;

				pNewItem->m_bFPELoaded = false;
				pNewItem->m_sFPEModel = "";
				pNewItem->m_sFPETextured = "";
				pNewItem->m_sFPEKeywords = "";
				pNewItem->m_sFPEDBOFile = "";
				pNewItem->m_sDLuaDescription = "##na##";
				pNewItem->m_fDLuaHeight = 0.0f;
				pNewItem->m_iFPEDBOFileSize = 0;
				pNewItem->m_bIsCharacterCreator = 0;
				pNewItem->iType = foldertype;

				pNewFolder->m_pFirstFile = pNewItem;
				m_pFiles = pNewItem;
			}

			for (; it != sorted_files.end(); ++it) 
			{
				//PE: Here we update , so we only have new files. but need to update duplicate_files_check
				if (it->size() > 0) {
					cStr sName = (char *)it->c_str();
					cStr m_sPath = folder_s;
					cStr sCheck = m_sPath + cstr("\\") + sName;
					char *find = NULL;

					if(foldertype == 0)
						find = (char *)pestrcasestr(sCheck.Get(), "entitybank\\");
					if (foldertype == 1)
						find = (char *)pestrcasestr(sCheck.Get(), "audiobank\\");
					if (foldertype == 2)
						find = (char *)pestrcasestr(sCheck.Get(), "imagebank\\");
					if (foldertype == 3)
						find = (char *)pestrcasestr(sCheck.Get(), "videobank\\");
					if (foldertype == 4)
						find = (char *)pestrcasestr(sCheck.Get(), "scriptbank\\");
					if (foldertype == 5)
						find = (char *)pestrcasestr(sCheck.Get(), "particlesbank\\");
					if (foldertype == 6)
						find = (char *)pestrcasestr(sCheck.Get(), "charactercreatorplus\\animations\\");

					if (find)
					{
						// ZJ: Got a heap corruption error here.
						//sCheck = find;
						cStr sFind(find);
						strcpy(sCheck.Get(), sFind.Get());
						//sCheck = sCheck + cstr("\\") + sName;
					}
					sCheck = sCheck.Lower();
					//auto itr = std::find(duplicate_files_check.begin(), duplicate_files_check.end(), sCheck.Get());
					//if(!(itr != duplicate_files_check.end() && duplicate_files_check.size() > 0))
					//	duplicate_files_check.push_back(sCheck.Get());

					if (duplicate_files_check.count(sCheck.Get()) <= 0)
						duplicate_files_check.insert(sCheck.Get());

				}
				if (it->size() > 0)
				{
					cFolderItem::sFolderFiles *pNewItem = new cFolderItem::sFolderFiles;
					pNewItem->m_sName = it->c_str();
					if ((foldertype == 0 || foldertype == 6) && pNewItem->m_sName.Len() > 4)
						pNewItem->m_sNameFinal = pNewItem->m_sName.Left(pNewItem->m_sName.Len() - 5);
					else
						pNewItem->m_sNameFinal = pNewItem->m_sName;

					// credit if community asset
					pNewItem->m_sNameFinalCredit = GetNameFinalCreditFromAbsPath (pNewFolder->m_sFolderFullPath.Get());

					//Generate a better search string. include category at end.#
					std::string sBetterSearch = pNewItem->m_sNameFinal.Get();
					sBetterSearch = sBetterSearch + " ( " + pNewFolder->m_sFolder.Get() + " )";
					//Remove main folder for better search.
					if (foldertype == 1)
						replaceAll(sBetterSearch, "audiobank", "");
					if (foldertype == 2)
						replaceAll(sBetterSearch, "imagebank", "");
					if (foldertype == 3)
						replaceAll(sBetterSearch, "videobank", "");
					if (foldertype == 4)
						replaceAll(sBetterSearch, "scriptbank", "");
					if (foldertype == 5)
						replaceAll(sBetterSearch, "particlesbank", "");
					if (foldertype == 6)
						replaceAll(sBetterSearch, "charactercreatorplus\\animations", "");

					replaceAll(sBetterSearch, "_", " ");
					replaceAll(sBetterSearch, "-", " ");
					pNewItem->m_sBetterSearch = sBetterSearch.c_str();

					pNewItem->m_tFileModify = 0; //PE: Need timestamp here.
					pNewItem->bFavorite = false;
					pNewItem->bAvailableInFreeTrial = false;

					//PE: This was to slow.
					struct stat sb;
					cstr file = pNewFolder->m_sFolderFullPath;
					file = file + "\\" + pNewItem->m_sName;

					//std::vector< std::pair<std::string, time_t> >::iterator its = files_time_stamp.begin();
					//for (; its != files_time_stamp.end(); ++its)
					//{
					//	if (its->first.size() > 0) 
					//	{
					//		if (strcmp(its->first.c_str(), file.Get()) == 0)
					//		{
					//			pNewItem->m_tFileModify = its->second;
					//			break;
					//		}
					//	}
					//}

					auto it = files_time_stamp.find(file.Get());
					if (it != files_time_stamp.end()) {
						pNewItem->m_tFileModify = it->second;
					}

					std::vector<std::string>::iterator itf = files_favorite.begin();
					for (; itf != files_favorite.end(); ++itf)
					{
						if (itf->size() > 0) 
						{
							if (strnicmp(itf->c_str(), file.Get(), file.Len()) == 0)
							{
								pNewItem->bFavorite = true;
								break;
							}
						}
					}
					extern bool g_bFreeTrialVersion;
					if (g_bFreeTrialVersion == true)
					{
						SetAvailableInFreeTrial(foldertype, pNewItem, file);
						/*
						std::vector<std::string>::iterator itf = files_availableinfreetrial.begin();
						for (; itf != files_availableinfreetrial.end(); ++itf)
						{
							if (itf->size() > 0)
							{
								if (strnicmp(itf->c_str(), file.Get(), file.Len()) == 0)
								{
									pNewItem->bAvailableInFreeTrial = true;
									break;
								}
							}
						}
						*/
					}

					pNewItem->m_sPath = folder_s;
					pNewItem->m_sFolder = folder_s;
					pNewItem->m_Backdrop = "";
					pNewItem->iFlags = 0;
					pNewItem->iPreview = 0;
					pNewItem->iBigPreview = 0;
					pNewItem->id = iTotalFiles++;
					pNewItem->bPreviewProcessed = false;
					pNewItem->m_pNext = NULL;
					pNewItem->m_pNextTime = NULL;
					pNewItem->m_pCustomSort = NULL;

					pNewItem->m_bFPELoaded = false;
					pNewItem->m_sFPEModel = "";
					pNewItem->m_sFPETextured = "";
					pNewItem->m_sFPEKeywords = "";
					pNewItem->m_sFPEDBOFile = "";
					pNewItem->m_sDLuaDescription = "##na##";
					pNewItem->m_fDLuaHeight = 0.0f;
					pNewItem->m_iFPEDBOFileSize = 0;
					pNewItem->m_bIsCharacterCreator = 0;
					pNewItem->iType = foldertype;
					m_pFiles->m_pNext = pNewItem;
					m_pFiles->m_pNextTime = pNewItem;
					m_pFiles->m_pCustomSort = pNewItem;
					m_pFiles = pNewItem;
				}
			}
			sorted_files.clear();

			CustomSortFiles(0, pNewFolder->m_pFirstFile);

		}

		//Update last folder modify date time.
		struct stat sb;
		if (stat(pNewFolder->m_sFolderFullPath.Get(), &sb) == 0) 
		{
			if (sb.st_mtime != pNewFolder->m_tFolderModify) 
			{
				pNewFolder->m_tFolderModify = sb.st_mtime;
			}
		}
	}

	SetDir(OldDir.Get());
}

void RefreshPurchasedFolder ( void )
{
	// First update for any folders
	// for this we need to delete the curretn one
	bool bCannotDeletFirstItIsGlobal = false;
	cFolderItem* pDeleteFolder = &MainEntityList;
	while (pDeleteFolder->m_pNext)
	{
		cFolderItem* pNextOneToDelete = pDeleteFolder->m_pNext;
		pDeleteFolder->m_pNext = NULL;
		if (bCannotDeletFirstItIsGlobal == false)
		{
			// just clear the first instance as it is global
			memset(&pDeleteFolder, 0, sizeof(pDeleteFolder));
			bCannotDeletFirstItIsGlobal = true;
		}
		else
		{
			// rest are created in runtime, so can delete
			delete pDeleteFolder;
		}
		pDeleteFolder = pNextOneToDelete;
	}
	extern bool bExternal_Entities_Init;
	bExternal_Entities_Init = false;

	// collect an entire list of all relevant filders (entitybank, scriptbank, images, particles, etc)
	extern void mapeditorexecutable_full_folder_refresh (void);
	mapeditorexecutable_full_folder_refresh();

	// Second scan purchased folder
	cFolderItem *pNewFolder = &MainEntityList;
	while (pNewFolder->m_pNext)
	{
		if (strnicmp (pNewFolder->m_sFolder.Get(), "purchased", strlen("purchased")) == NULL)
		{
			// force this folder to refresh
			pNewFolder->m_tFolderModify = 0;
			LPSTR pFolderFullPath = pNewFolder->m_sFolderFullPath.Get();
			RefreshEntityFolder(pFolderFullPath, pNewFolder);
			break;
		}
		pNewFolder = pNewFolder->m_pNext;
	}

	// Third scan ALL folders (store can place files and folders most places!)
	pNewFolder = &MainEntityList;
	while (pNewFolder->m_pNext)
	{
		// force this folder to refresh
		pNewFolder->m_tFolderModify = 0;
		LPSTR pFolderFullPath = pNewFolder->m_sFolderFullPath.Get();
		RefreshEntityFolder(pFolderFullPath, pNewFolder);
		pNewFolder = pNewFolder->m_pNext;
	}

	// ensure the sorted list that is static in the main loop is reset to avoid crash if contents changes mid-loop
	extern bool bUpdateSearchSorting;
	bUpdateSearchSorting = true;
}

char defaultWriteFolder[260];

void FindAWriteablefolder( void )
{
	if ( (SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, &defaultWriteFolder[0])) >= 0 ) {

		cstr tmp = defaultWriteFolder;
		tmp += "\\AppData";
		if (PathExist(tmp.Get()) == 1) {
			tmp += "\\Local\\TGC";
			if (PathExist(tmp.Get()) != 1) {
				_mkdir(tmp.Get());
			}
		}
		tmp += "\\";
		strcpy(defaultWriteFolder, tmp.Get());
	}
	else
	{
		//
		cstr tmp = GetDir();
		tmp += "\\";
		strcpy(defaultWriteFolder, tmp.Get());
	}
}

#include "Types.h"
#include <set>

void InitParseLuaScript(entityeleproftype *tmpeleprof)
{
	for (int i = 0; i < MAXPROPERTIESVARIABLES; i++) {
		tmpeleprof->PropertiesVariable.VariableType[i] = 0;
		strcpy(tmpeleprof->PropertiesVariable.Variable[i], "");
		strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], "");
		tmpeleprof->PropertiesVariable.VariableValueFrom[i] = 0.0f;
		tmpeleprof->PropertiesVariable.VariableValueTo[i] = 0.0f;
		//strcpy(tmpeleprof->PropertiesVariable.VariableSectionDescription[i], "");
		tmpeleprof->PropertiesVariable.VariableSectionDescription[i] = "";
		//strcpy(tmpeleprof->PropertiesVariable.VariableSectionEndDescription[i], "");
		tmpeleprof->PropertiesVariable.VariableSectionEndDescription[i] = "";
	}

	luadropdownlabels.clear();
	g_DLuaVariableNames.clear();
}

void ParseLuaScriptWithElementID(entityeleproftype *tmpeleprof, char * script, int iObjID)
{
	char cScriptName[MAX_PATH];
	strcpy(cScriptName, script);
	int pos;
	for (pos = strlen(cScriptName); pos > 0; pos-- ) {
		if (cScriptName[pos] == '\\' || cScriptName[pos] == '/')
			break;
	}
	if (pos > 0) {
		strcpy(cScriptName, &cScriptName[pos+1]);
		if (strlen(cScriptName) > 4)
			if (cScriptName[strlen(cScriptName) - 4] == '.')
				cScriptName[strlen(cScriptName) - 4] = 0;
	}

	strcpy(tmpeleprof->PropertiesVariable.VariableScript, cScriptName);

	InitParseLuaScript(tmpeleprof);

	tmpeleprof->PropertiesVariableActive = 0;
	tmpeleprof->PropertiesVariable.iVariables = 0;
	tmpeleprof->PropertiesVariable.VariableDescription = "";
	//scriptbank\markers\storyinzone.lua fails.

	FILE* fScript = GG_fopen(script, "r");
	if (fScript)
	{
		char ctmp[8192];
		int include_returns = 0;
		int description_lines = 0;
		bool bFirstLine = true;
		while (!feof(fScript))
		{
			fgets(ctmp, 8190 , fScript);
			ctmp[8190] = 0;
			if (strlen(ctmp) > 0 && ctmp[strlen(ctmp) - 1] == '\n')
				ctmp[strlen(ctmp) - 1] = 0;
			int cadd = 0;
			char *find = (char *) pestrcasestr(ctmp, "-- DESCRIPTION:");
			if (!find) {
				find = (char *)pestrcasestr(ctmp, "--DESCRIPTION:");
				cadd = -1;
			}

			//PE: Make a raw description text first, then parse after all descriptions are merged.

			if (find) {
				if(find[15] == ' ')
					strcpy(ctmp, find + 16 + cadd);
				else
					strcpy(ctmp, find + 15 + cadd);

				if(bFirstLine)
					tmpeleprof->PropertiesVariable.VariableDescription = ctmp;
				else {
					tmpeleprof->PropertiesVariable.VariableDescription += " "; 
					tmpeleprof->PropertiesVariable.VariableDescription += ctmp;
				}
				description_lines++;
				//Activate Propertie Variables.
				bFirstLine = false;
			}
		}
		fclose(fScript);

		//PE: Make error for scripts with missing newline.
		if (description_lines <= 1)
		{
			//PE: Check if lua files is missing \n newlines this can really give strange results.
			strcpy(ctmp, tmpeleprof->PropertiesVariable.VariableDescription.Get());
			//PE: Scan for non newline text file.
			for (int i = 0; i < strlen(ctmp); i++)
			{
				if (ctmp[i] == '\r')
				{
					include_returns++;
					break;
				}
			}
			if (include_returns > 0)
			{
				tmpeleprof->PropertiesVariable.VariableDescription = "Lua script is missing 'newlines' and description can't be parsed!";
			}
		}

		//Activate Propertie Variables.
		bFirstLine = false;
		//while [ , collect all variables.
		char *find = tmpeleprof->PropertiesVariable.VariableDescription.Get();
		char *SectionDescription = find;
		while ((find = (char *)pestrcasestr(find, "[")) && tmpeleprof->PropertiesVariable.iVariables < MAXPROPERTIESVARIABLES)
		{
			char *find2 = (char *)pestrcasestr(find, "]");
			if (find2) 
			{
				find2[0] = 0;
				find[0] = 0;
				if (strlen(SectionDescription) >= MAXVARIABLETEXTSIZELARGE)
				{
					//To Large.
					SectionDescription = find2 + 1;
				}
				else
				{
					if (tmpeleprof->PropertiesVariable.iVariables >= 1) 
					{
						//Add space
						//strcpy(tmpeleprof->PropertiesVariable.VariableSectionDescription[tmpeleprof->PropertiesVariable.iVariables], " ");
						//strcat(tmpeleprof->PropertiesVariable.VariableSectionDescription[tmpeleprof->PropertiesVariable.iVariables], SectionDescription);
						tmpeleprof->PropertiesVariable.VariableSectionDescription[tmpeleprof->PropertiesVariable.iVariables] = cStr(" ") + cStr(SectionDescription);
					}
					else 
					{
						//strcpy(tmpeleprof->PropertiesVariable.VariableSectionDescription[tmpeleprof->PropertiesVariable.iVariables], SectionDescription);
						tmpeleprof->PropertiesVariable.VariableSectionDescription[tmpeleprof->PropertiesVariable.iVariables] = SectionDescription;
					}
					//strcpy(tmpeleprof->PropertiesVariable.VariableSectionEndDescription[tmpeleprof->PropertiesVariable.iVariables], "");
					tmpeleprof->PropertiesVariable.VariableSectionEndDescription[tmpeleprof->PropertiesVariable.iVariables] = "";
					SectionDescription = find2 + 1;
					if (!pestrcasestr(SectionDescription, "[")) 
					{
						if (SectionDescription[0] != 0) 
						{
							//strcpy(tmpeleprof->PropertiesVariable.VariableSectionEndDescription[tmpeleprof->PropertiesVariable.iVariables], SectionDescription);
							tmpeleprof->PropertiesVariable.VariableSectionEndDescription[tmpeleprof->PropertiesVariable.iVariables] = SectionDescription;
						}
					}
					tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] = 0;

					//PE: tmpeleprof->PropertiesVariable.Variable can be larger then 100 before '=' is found.
					char tmpVariable[1024];
					strcpy(tmpVariable, find + 1);

					//PE: Scan all lua script for largest [] // [@MULTI_TRIGGER=2(1=Yes, 2=No)]
					//if (strlen(tmpeleprof->PropertiesVariable.Variable[tmpeleprof->PropertiesVariable.iVariables]) > maxvariable)
					//	maxvariable = strlen(tmpeleprof->PropertiesVariable.Variable[tmpeleprof->PropertiesVariable.iVariables]);

					if (pestrcasestr(tmpVariable, "#"))
						tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] = 1; //float
					if (pestrcasestr(tmpVariable, "$"))
						tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] = 2; //string
					if (pestrcasestr(tmpVariable, "!"))
						tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] = 3; //bool
					if (pestrcasestr(tmpVariable, "@"))
						tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] = 4; // labelled int[]
					if (pestrcasestr(tmpVariable, "@@"))
						tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] = 7; // labelled string
					if (pestrcasestr(tmpVariable, "*"))
						tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] = 5; // user specified in seconds
					if (pestrcasestr(tmpVariable, "&"))
						tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] = 6; // should alter eleprof variable

					//Set default values search for =
					char *find3 = (char *)pestrcasestr(tmpVariable, "=");
					if (find3) 
					{
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[tmpeleprof->PropertiesVariable.iVariables], find3 + 1);
						
						find3[0] = 0; // remove = from Variable.
						strcpy(tmpeleprof->PropertiesVariable.Variable[tmpeleprof->PropertiesVariable.iVariables], tmpVariable);

						//check if we got a range.
						char *find4 = (char *)pestrcasestr(tmpeleprof->PropertiesVariable.VariableValue[tmpeleprof->PropertiesVariable.iVariables], "(");
						if (find4) 
						{
							find4[0] = 0;
							char from[1050];
							strcpy(from, find4 + 1);

							// Need to calculate the range differently for the dropdown type.
							if (tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] == 4 || tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] == 7)
							{
								// Count number of = symbols
								int iFirstEqualsIndex = -1;
								int iLastEqualsIndex = -1;
								for (int i = 0; i < strlen(from); i++)
								{
									if (from[i] == '=')
									{
										iLastEqualsIndex = i;
										if (iFirstEqualsIndex < 0)
											iFirstEqualsIndex = i;
									}
								}

								if (iFirstEqualsIndex > -1)
								{
									/* e.g. (1=Slow, 2=Fast, 3=Very Fast)*/
									/* or.  (0=AnimSetList)*/
									/* or.  (0=QuestList)*/

									// Determine range.
									tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables]
										= atof(from + iFirstEqualsIndex - 1);

									tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables]
										= atof(from + iLastEqualsIndex - 1);

									// Store labels.
									int count = tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables]
										- tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables];

									std::vector<std::string> labels;
									// First element is the variable number - needed for when there are more than one dropdown variables in a single script.
									char cVariable[8];
									sprintf(cVariable, "%d", tmpeleprof->PropertiesVariable.iVariables);
									labels.push_back(std::string(cVariable));

									for (int i = iFirstEqualsIndex; i < strlen(from); i++)
									{
										if (from[i] == '=')
										{
											int end = i;
											// Now find the comma or close bracket, to determine where the label starts and ends.
											for (int j = i+1; j < strlen(from); j++)
											{
												if (from[j] == ',' || from[j] == ')')
												{
													end = j;
													break;
												}
											}

											bool bExit = false;
											if (from[end] == ')')
												bExit = true;

											// copy the label into the buffer.
											char buffer[MAX_PATH];
											strcpy(buffer, from + i+1);
											buffer[end - i - 1] = 0;

											i = end;

											labels.push_back(std::string(buffer));

											if (bExit) break;
										}
									}

									// can intercept label list here if special indicator that it is an animset or questlist list
									//PE: Allow more DLUA lists to work on zones.
									if (labels.size()==2 ) // && iObjID > 0
									{
										if(iObjID > 0 && stricmp(labels[1].c_str(),"animsetlist")==NULL)
										{
											labels.clear();
											labels.push_back(cVariable);
											extern sObject* GetObjectData (int);
											sObject* pObject = GetObjectData(iObjID);
											sAnimationSet* pAnimSet = pObject->pAnimationSet;
											int iAnimSetIndex = 0;
											if (pAnimSet)
											{
												if (stricmp(pAnimSet->szName, "all") == NULL)
												{
													pAnimSet = pAnimSet->pNext; // skip first one (base zero all character anims)
												}
											}
											while (pAnimSet)
											{
												if (stricmp(pAnimSet->szName, "mouthshapes") != NULL)
												{
													iAnimSetIndex++;
													labels.push_back(pAnimSet->szName);
												}
												pAnimSet = pAnimSet->pNext;
											}
											if (iAnimSetIndex > 0)
											{
												tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 1;
												tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = iAnimSetIndex;
											}
											else
											{
												tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 0;
												tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = 0;
											}
										}
										if (labels.size() == 2)
										{
											//InterActionWeaponList
											if (stricmp(labels[1].c_str(), "interactionweaponlist") == NULL)
											{
												labels.clear();
												labels.push_back(cVariable);
												int iWeaponListIndex = 1;
												//PE: Add filter.
												int FillWeaponList(std::vector<std::string>& labels, char* filter);
												iWeaponListIndex += FillWeaponList(labels, "Interaction");

												if (iWeaponListIndex > 0)
												{
													tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 1;
													tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = iWeaponListIndex;
												}
												else
												{
													tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 0;
													tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = 0;
												}

											}
											if (stricmp(labels[1].c_str(), "anyweaponlist") == NULL)
											{
												labels.clear();
												labels.push_back(cVariable);
												int iWeaponListIndex = 1;
												int FillWeaponList(std::vector<std::string> &labels, char* filter);
												iWeaponListIndex += FillWeaponList( labels , nullptr);

												if (iWeaponListIndex > 0)
												{
													tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 1;
													tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = iWeaponListIndex;
												}
												else
												{
													tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 0;
													tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = 0;
												}

											}

											if (stricmp(labels[1].c_str(), "questlist") == NULL)
											{
												labels.clear();
												labels.push_back(cVariable);
												int iQuestListIndex = 1;
												labels.push_back("None");
												for (int n = 0; n < g_collectionQuestList.size(); n++)
												{
													iQuestListIndex++;
													labels.push_back(g_collectionQuestList[n].collectionFields[0].Get());
												}
												if (iQuestListIndex > 0)
												{
													tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 1;
													tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = iQuestListIndex;
												}
												else
												{
													tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 0;
													tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = 0;
												}
											}
										}
									}
									//PE: Let some lists work on zones also.
									if (labels.size() == 2)
									{
										if (stricmp(labels[1].c_str(), "armanimsetlist") == NULL)
										{
											labels.clear();
											labels.push_back(cVariable);
											labels.push_back("None");

											char animlist[MAX_PATH];
											//strcpy(animlist, "gamecore\\interactive_anims.txt");
											strcpy(animlist, "gamecore\\guns\\interactive\\interactive_anims.txt");
											GG_GetRealPath(animlist, 0);
											if (FileExist(animlist) == 1)
											{
												FILE* fAnimList = GG_fopen(animlist, "r");
												if (fAnimList)
												{
													char ctmp[MAX_PATH];
													while (!feof(fAnimList))
													{
														strcpy(ctmp, "");
														fgets(ctmp, MAX_PATH - 1, fAnimList);
														if (strlen(ctmp) > 0 && ctmp[strlen(ctmp) - 1] == '\n')
															ctmp[strlen(ctmp) - 1] = 0;
														if (strlen(ctmp) > 0)
															labels.push_back(&ctmp[0]);
													}
													fclose(fAnimList);
												}
											}
											if (labels.size() > 1)
											{
												tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 1;
												tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = labels.size();
											}
											else
											{
												tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 0;
												tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = 0;
											}

										}
										

										if (stricmp(labels[1].c_str(), "effectlist") == NULL)
										{
											labels.clear();
											labels.push_back(cVariable);
											labels.push_back("None");

											char writePath[MAX_PATH];
											extern const char* GG_GetWritePath();
											strcpy(writePath, GG_GetWritePath());

											std::vector<std::string> effectFilesWrite, effectFilesDoc;
											effectFilesWrite.clear();
											effectFilesDoc.clear();
											char writableEffect[MAX_PATH];
											strcpy(writableEffect, writePath);
											strcat(writableEffect, "Files\\particlesbank\\wpe");
											CollectFilesWithExtension(".pe", writableEffect, &effectFilesWrite);

											extern char szRootDir[MAX_PATH];
											char docEffect[MAX_PATH];
											strcpy(docEffect, szRootDir);
											strcat(docEffect, "\\Files\\particlesbank\\wpe");
											CollectFilesWithExtension(".pe", docEffect, &effectFilesDoc);

											std::set<std::string> uniqueEffectFiles;
											for (const std::string& file : effectFilesWrite) {
												uniqueEffectFiles.insert(&file[strlen(writableEffect)-17]);
											}
											for (const std::string& file : effectFilesDoc) {
												uniqueEffectFiles.insert(file);
											}
											for (const std::string& file : uniqueEffectFiles) {
												labels.push_back(&file[strlen(docEffect)-17]);
											}
											uniqueEffectFiles.clear();
											effectFilesWrite.clear();
											effectFilesDoc.clear();

											if (labels.size() > 1)
											{
												tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 1;
												tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = labels.size();
											}
											else
											{
												tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 0;
												tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = 0;
											}
										}
										if (stricmp(labels[1].c_str(), "decallist") == NULL)
										{
											labels.clear();
											labels.push_back(cVariable);
											labels.push_back("None");
											std::vector <cstr> mydecals;
											int fillgloballistwithdecals(std::vector <cstr>& list_s);
											int listmax = fillgloballistwithdecals(mydecals);
											for (int n = 1; n < listmax; n++)
											{
												if (mydecals[n].Len() > 0)
												{
													std::string name = mydecals[n].Get();
													labels.push_back(name);
												}
											}
											if (labels.size() > 1)
											{
												tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 1;
												tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = labels.size();
											}
											else
											{
												tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 0;
												tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = 0;
											}
										}

										if (stricmp(labels[1].c_str(), "hudscreenlist") == NULL)
										{
											static std::vector<std::string> globallist_labels;
											extern StoryboardStruct Storyboard;
											labels.clear();
											labels.push_back(cVariable);
											labels.push_back("None");
											for (int allhudscreensnodeid = 0; allhudscreensnodeid < STORYBOARD_MAXNODES; allhudscreensnodeid++)
											{
												if (Storyboard.Nodes[allhudscreensnodeid].used)
												{
													if (Storyboard.Nodes[allhudscreensnodeid].type == STORYBOARD_TYPE_HUD)
													{
														if (strlen(Storyboard.Nodes[allhudscreensnodeid].title) > 0)
														{
															labels.push_back(Storyboard.Nodes[allhudscreensnodeid].title);

														}
													}
												}
											}
											if (labels.size() > 1)
											{
												tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 1;
												tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = labels.size();
											}
											else
											{
												tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 0;
												tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = 0;
											}

										}
										if (stricmp(labels[1].c_str(), "globallist") == NULL)
										{
											static std::vector<std::string> globallist_labels;
											extern StoryboardStruct Storyboard;
											extern std::vector<int> g_gameGlobalListNodeId;
											extern std::vector<int> g_gameGlobalListIndex;
											extern std::vector<int> g_gameGlobalListValue;

											if (1) //PE: Always update.
											{
												globallist_labels.clear();
												globallist_labels.push_back(cVariable);
												globallist_labels.push_back("None");
												//PE: Need to push these to g_UserGlobal[''].
												globallist_labels.push_back("Gun Ammo Remaining");
												globallist_labels.push_back("Health Remaining");
												globallist_labels.push_back("Lives Remaining");

												for (int allhudscreensnodeid = 0; allhudscreensnodeid < STORYBOARD_MAXNODES; allhudscreensnodeid++)
												{
													//PE: Need custom ? stricmp(readout.c_str(), "User Defined Global Statusbar") == NULL
													if (strlen(Storyboard.Nodes[allhudscreensnodeid].lua_name) > 0 && strnicmp(Storyboard.Nodes[allhudscreensnodeid].lua_name, "hud", 3) == NULL)
													{
														for (int i = STORYBOARD_MAXWIDGETS; i >= 0; i--)
														{
															if (Storyboard.Nodes[allhudscreensnodeid].widget_type[i] == STORYBOARD_WIDGET_TEXT)
															{
																std::string readout = Storyboard.widget_readout[allhudscreensnodeid][i];
																if (stricmp(readout.c_str(), "User Defined Global") == NULL
																	|| stricmp(Storyboard.widget_readout[allhudscreensnodeid][i], "User Defined Global Text") == NULL)
																{
																	//"User Defined Global Statusbar"
																	// only add unique ones to game global list
																	LPSTR pNewName = Storyboard.Nodes[allhudscreensnodeid].widget_label[i];
																	if (strlen(pNewName) > 0)
																	{
																		if (!pestrcasestr(pNewName, ":")) //PE: Do not show : rpginventorykinds.
																		{
																			for (int n = 0; n < globallist_labels.size(); n++)
																			{
																				if (strcmp(pNewName, globallist_labels[n].c_str()) == NULL)
																				{
																					// already exists
																					pNewName = "";
																					break;
																				}
																			}
																			if (strlen(pNewName) > 0)
																				globallist_labels.push_back(pNewName);
																		}
																	}
																}
															}
														}
													}
												}
											}
											labels = globallist_labels;
											if (labels.size() > 1)
											{
												tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 1;
												tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = labels.size();
											}
											else
											{
												tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = 0;
												tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = 0;
											}
										}
									}
								

									// and add to official dropdown lists
									luadropdownlabels.push_back(labels);
								}
							}
							else
							{
								// Extract range normally.
								char *find5 = (char *)pestrcasestr(from, ",");
								if (find5)
								{
									find5[0] = 0;
									tmpeleprof->PropertiesVariable.VariableValueFrom[tmpeleprof->PropertiesVariable.iVariables] = atof(find4 + 1);
									char *find6 = (char *)pestrcasestr(find5 + 1, ")");
									if (find6)
									{
										find6[0] = 0;
										tmpeleprof->PropertiesVariable.VariableValueTo[tmpeleprof->PropertiesVariable.iVariables] = atof(find5 + 1);
									}
								}
							}
						}
					}
					else
					{
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[tmpeleprof->PropertiesVariable.iVariables], "");
						tmpVariable[MAXVARIABLESIZE - 1] = 0;
						strcpy(tmpeleprof->PropertiesVariable.Variable[tmpeleprof->PropertiesVariable.iVariables], tmpVariable);
					}

					//Clean variable name.
					std::string clean_string = tmpeleprof->PropertiesVariable.Variable[tmpeleprof->PropertiesVariable.iVariables];
					replaceAll(clean_string, "#", "");
					replaceAll(clean_string, "$", "");
					replaceAll(clean_string, "!", "");
					replaceAll(clean_string, "@", "");
					replaceAll(clean_string, "*", "");
					replaceAll(clean_string, "&", "");
					if (tmpeleprof->PropertiesVariable.VariableType[tmpeleprof->PropertiesVariable.iVariables] == 6)
					{
						g_DLuaVariableNames.push_back(clean_string);
					}
					strcpy(tmpeleprof->PropertiesVariable.Variable[tmpeleprof->PropertiesVariable.iVariables], clean_string.c_str());

					//Clean variablevalue.
					clean_string = tmpeleprof->PropertiesVariable.VariableValue[tmpeleprof->PropertiesVariable.iVariables];
					replaceAll(clean_string, "\"", ""); //cant use "
					strcpy(tmpeleprof->PropertiesVariable.VariableValue[tmpeleprof->PropertiesVariable.iVariables], clean_string.c_str());

					tmpeleprof->PropertiesVariable.iVariables++;
					tmpeleprof->PropertiesVariableActive = 1;
				}

				find = find2 + 1;
			}
			else find++;
		}


		//[RANGE] integer [SPEED#] float [TEXT$] string
		cstr sLuaScriptName = tmpeleprof->PropertiesVariable.VariableScript;
		sLuaScriptName += "_properties(";
		//Check if we need to update with new default values.
		if (!pestrcasestr(tmpeleprof->soundset4_s.Get(),sLuaScriptName.Get())) {
			if (tmpeleprof->PropertiesVariable.iVariables > 0) {
				tmpeleprof->soundset4_s = sLuaScriptName;
				//Add varables.
				for (int i = 0; i < tmpeleprof->PropertiesVariable.iVariables; i++) {

					char val[3];
					val[0] = tmpeleprof->PropertiesVariable.VariableType[i] + '0';
					val[1] = 0;

					tmpeleprof->soundset4_s += val;
					tmpeleprof->soundset4_s += "\"";
					std::string clean_string = tmpeleprof->PropertiesVariable.VariableValue[i];
					replaceAll(clean_string, "\"", ""); //cant use "
					tmpeleprof->soundset4_s += (char *)clean_string.c_str();
					//tmpeleprof->soundset4_s += tmpeleprof->PropertiesVariable.VariableValue[i];
					tmpeleprof->soundset4_s += "\"";
					if (i < tmpeleprof->PropertiesVariable.iVariables - 1)
						tmpeleprof->soundset4_s += ",";
				}
				tmpeleprof->soundset4_s += ")";
				//fclose(fScript);
				tmpeleprof->PropertiesVariableActive = 1;
				//return;
			}
		}

		//Get old value into arrayes.
		char tmp[4096];
		ZeroMemory(tmp, 4095);
		strcpy(tmp, tmpeleprof->soundset4_s.Get()); //dont change original.
//		char *find;
		int iNextVariable = 0;
		find = (char *)pestrcasestr(tmp, "(");
		if (find) {
			//cmd = &tmp[0];
			find[0] = 0;
			find++;
			while ((find = (char *)pestrcasestr(find, "\"")) && iNextVariable < tmpeleprof->PropertiesVariable.iVariables) {
				char *find2 = (char *)pestrcasestr(find + 1, "\"");
				if (find2) {
					int type = find[-1] - '0';
					find2[0] = 0;

					if (type >= 0 && type <= 9) {
						if (type == 1) {
							strcpy(tmpeleprof->PropertiesVariable.VariableValue[iNextVariable],find + 1);
						}
						else if (type == 2) {
							strcpy(tmpeleprof->PropertiesVariable.VariableValue[iNextVariable], find + 1);
						}
						else {
							strcpy(tmpeleprof->PropertiesVariable.VariableValue[iNextVariable], find + 1);
						}
					}
					find = find2 + 1;
				}
				else {
					find++;
				}

				iNextVariable++;
			}
		}

	}
}

void ParseLuaScript(entityeleproftype *tmpeleprof, char * script)
{
	ParseLuaScriptWithElementID(tmpeleprof, script, -1);
}

cstr DLUAFormatLabel(LPSTR pIn)
{
	// introduce a display space if the first word has a capital in the original variable label
	bool bDetectAnyLowerCase = false;
	for (int i = 0; i < strlen(pIn); i++)
	{
		unsigned char letter = pIn[i];
		if (letter >= 'a' && letter <= 'z')
		{
			bDetectAnyLowerCase = true;
			break;
		}
	}
	int iWordPos = 0;
	char pFullLabel[MAX_PATH];
	memset(pFullLabel, 0, sizeof(pFullLabel));
	if (bDetectAnyLowerCase == true)
	{
		// handle lower case word separating
		for (int i = 0; i < strlen(pIn); i++)
		{
			bool bUpper = false;
			unsigned char letter = pIn[i];
			if (letter >= 'A' && letter <= 'Z') bUpper = true;
			if (i > 0 && bUpper == true) pFullLabel[iWordPos++] = ' ';
			pFullLabel[iWordPos++] = letter;
		}
		pFullLabel[iWordPos] = 0;
	}
	else
	{
		// all or mostly upper case, treat as such
		strcpy (pFullLabel, pIn);
	}
	// format variable labels to convert _ to spaces
	for (int i = 0; i < strlen(pFullLabel); i++)
	{
		unsigned char letter = pFullLabel[i];
		if (letter == '_') pFullLabel[i] = ' ';
	}
	// finally ensure all lower case except for first letter of each word (now separated by space)
	strlwr(pFullLabel);
	bool bMakeUpperCase = true;
	for (int i = 0; i < strlen(pFullLabel); i++)
	{
		unsigned char letter = pFullLabel[i];
		if (bMakeUpperCase == true)
		{
			int iLwrToUpr = 'a' - 'A';
			pFullLabel[i] = pFullLabel[i] - iLwrToUpr;
			bMakeUpperCase = false;
		}
		if (letter == ' ') bMakeUpperCase = true;
	}
	cstr out = pFullLabel;
	return out;
}
