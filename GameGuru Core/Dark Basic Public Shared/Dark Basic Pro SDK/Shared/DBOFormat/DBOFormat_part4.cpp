DARKSDK_DLL bool LoadDBO ( LPSTR pPassedInFilename, sObject** ppObject )
{
	// DBOBlock pointer
	DWORD dwBlockSize = 0;
	void* pDBOBlock = NULL;
	char pFilename[MAX_PATH];
	strcpy(pFilename, pPassedInFilename);

	// No object to start with
	*ppObject = NULL;

	// switch to use new AssImp Importer if not a DBO file
	// Obtain extension
	char pExtension[256];
	strcpy(pExtension, "");
	for ( int n=strlen(pFilename); n>0; n-- ) { if ( pFilename[n]=='.' ) { strcpy(pExtension, pFilename+n+1); break; } }

	#ifdef WICKEDENGINE
	if ( _stricmp ( pExtension, "DBO" ) != NULL )
	{
		extern char sLoadAssImpObjectError[1024];
		strcpy(sLoadAssImpObjectError, "");
		//PE: If this file is in the docwrite folder we need to resolve filename or assimp cant find it.
		char cAssimpFilename[MAX_PATH];
		strcpy(cAssimpFilename, pFilename);
		GG_GetRealPath(cAssimpFilename, 0);
		//PE: If the file is a path alone , assImp will crash so make sure its a file.
		if (FileExist(cAssimpFilename) == 0) return false;
		if ( LoadAssImpObject (cAssimpFilename, ppObject, g_eLoadScalingMode ) == false )
		{
			// before give up, use converter to see if can create a DBO fron the X we can load
			char pOldDir[MAX_PATH];
			GetCurrentDirectoryA(MAX_PATH, pOldDir);
			char pRealLocationOfModelFile[MAX_PATH];
			strcpy(pRealLocationOfModelFile, pOldDir);
			pRealLocationOfModelFile[strlen(pRealLocationOfModelFile) - strlen("Files")] = 0;
			strcat(pRealLocationOfModelFile, "\\");
			GG_GetRealPath(pRealLocationOfModelFile, 1);
			SetDir(pRealLocationOfModelFile);
			extern char g_pAbsPathToConverter[MAX_PATH];
			HANDLE g_hConvertWAVtoLIPProcess = NULL;
			DB_ExecuteFile(&g_hConvertWAVtoLIPProcess, "open", g_pAbsPathToConverter, "", "", true);
			int iRunning = 1;
			while (iRunning == 1) 
			{
				iRunning = 0;
				DWORD dwStatus;
				if ( GetExitCodeProcess ( g_hConvertWAVtoLIPProcess, &dwStatus )==TRUE )
					if(dwStatus==STILL_ACTIVE)
						iRunning = 1;
			}
			CloseHandle ( g_hConvertWAVtoLIPProcess );
			SetDir(pOldDir);
			char pDBOVersionOfFile[MAX_PATH];
			strcpy(pDBOVersionOfFile, pFilename);
			pDBOVersionOfFile[strlen(pDBOVersionOfFile) - strlen(pExtension)] = 0;
			strcat(pDBOVersionOfFile, "DBO");
			GG_GetRealPath(pDBOVersionOfFile, 0);
			if (FileExist(pDBOVersionOfFile))
			{
				strcpy(pFilename, pDBOVersionOfFile);
				strcpy ( pExtension, "DBO" );
			}
			else
			{
				if (g_bGracefulWarningAboutOldXFiles == false)
				{
					char pRealFilename[MAX_PATH];
					strcpy(pRealFilename, pFilename);
					GG_GetRealPath(pRealFilename, 0);
					char pFriendlyMessage[1024+MAX_PATH];
					sprintf(pFriendlyMessage, "The model file (%s) is not supported in MAX. Suggest converting the model file to the latest export version of the format.\n\nError: %s", pRealFilename, sLoadAssImpObjectError);
					MessageBox(NULL, pFriendlyMessage, "AssImp Load Failure", MB_OK);
					// keep messages coming so we know ALL models that fail during a session.
					//g_bGracefulWarningAboutOldXFiles = true;
				}
				return false;
			}
		}
	}
	#endif
	if ( _stricmp ( pExtension, "DBO" ) == NULL )
	{
		// load data from DBO file or multi-threaded-pre-loaded DBO data
		// and we have access to the loaded textures via g_object_outputv
		DWORD* pDataBlockFromPreload = NULL;
		for (int n = 0; n < g_object_outputv.size(); n++)
		{
			int iSearchStrLen = strlen(pFilename);
			if (strnicmp(g_object_outputv[n].pFilename + strlen(g_object_outputv[n].pFilename) - iSearchStrLen, pFilename, iSearchStrLen) == NULL)
			{
				if (g_object_outputv[n].pData)
				{
					pDataBlockFromPreload = g_object_outputv[n].pData;
					dwBlockSize = g_object_outputv[n].dwDataSize;
					g_object_outputv[n].pData = NULL;
					g_object_outputv[n].dwDataSize = 0;
					pDBOBlock = pDataBlockFromPreload;
					break;
				}
			}
		}
		if (pDataBlockFromPreload == NULL)
		{
			if (LoadDBODataBlock(pFilename, &dwBlockSize, &pDBOBlock) == false)
				return false;
		}

		// construct the object
		if (!DBOConvertBlockToObject((void*)pDBOBlock, dwBlockSize, ppObject))
		{
			char str[1024];
			sprintf_s(str, "Failed to load object: %s", pFilename);
			Message(0, str, "Error");
			RunTimeError(RUNTIMEERROR_B3DOBJECTLOADFAILED);
			return false;
		}

		// free block when done
		SAFE_DELETE_ARRAY(pDBOBlock);
	}
	#ifndef WICKEDENGINE
	else
	{
		// call converter DLL (ConvX.dll)
		if ( !ConvertToDBOBlock ( pFilename, pExtension, &pDBOBlock, &dwBlockSize ) )
		{
			RunTimeError ( RUNTIMEERROR_B3DOBJECTLOADFAILED );
			return false;
		}
		// construct the object
		if (!DBOConvertBlockToObject((void*)pDBOBlock, dwBlockSize, ppObject))
		{
			RunTimeError(RUNTIMEERROR_B3DOBJECTLOADFAILED);
			return false;
		}

		// free block when done
		SAFE_DELETE_ARRAY(pDBOBlock);

	}
	#endif

	// okay
	return true;
}

DARKSDK_DLL bool SaveDBO ( LPSTR pFilename, sObject* pObject )
{
	// DBOBlock ptr
	DWORD dwBlockSize = 0;
	void* pDBOBlock = NULL;

	// does file exist
	if ( DBOFileExist ( pFilename ) )
	{
		RunTimeError ( RUNTIMEERROR_FILEEXISTS, pFilename );
		return false;
	}

	// leefix - 171203 - before save a DBO, ensure vertex data is original (and not modified by animation activity)
	ResetVertexDataInMesh ( pObject );

	// convert pObject to DBOBlock
	if ( !DBOConvertObjectToBlock ( pObject, (void**)&pDBOBlock, &dwBlockSize ) )
		return false;
		
	// save DBOBlock to file
	if ( !DBOSaveBlockFile ( pFilename, (void*)pDBOBlock, dwBlockSize ) )
		return false;

	// free block when done
	SAFE_DELETE(pDBOBlock);

	// okay
	return true;
}

DARKSDK_DLL bool CloneDBO ( sObject** ppDestObject, sObject* pSrcObject )
{
	// DBOBlock ptr
	DWORD dwBlockSize = 0;
	void* pDBOBlock = NULL; // was DWORD*

	// convert pObject to DBOBlock
	if ( !DBOConvertObjectToBlock ( pSrcObject, (void**)&pDBOBlock, &dwBlockSize ) )
		return false;
		
	// construct the new destination object
	if ( !DBOConvertBlockToObject ( pDBOBlock, dwBlockSize, ppDestObject ) )
		return false;

	// free block when done
	SAFE_DELETE_ARRAY(pDBOBlock);

	// okay
	return true;
}

// 310305 - mike - new function for allocating custom memory
DARKSDK_DLL void CreateCustomDataArrayForObject	( sObject* pObject, int iSize )
{
	pObject->pCustomData = new BYTE [ iSize ];
}

