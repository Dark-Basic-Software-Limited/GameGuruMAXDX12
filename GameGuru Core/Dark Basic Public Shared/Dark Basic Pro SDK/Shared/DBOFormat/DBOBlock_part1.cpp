DARKSDK_DLL bool ConstructObject ( sObject** ppObject, LPSTR* ppBlock )
{
	// ensure is DBO block
	char pMagicString [ MAX_STRING ];

	ReadString ( pMagicString, ppBlock );

	if ( _stricmp ( pMagicString, "MAGICDBO" )==NULL )
	{
		// version information
		DWORD dwVersion=0;
		DWORD dwRes1=0, dwRes2=0;
		ReadDWORD	( &dwVersion,	ppBlock );
		ReadDWORD	( &dwRes1,		ppBlock );
		ReadDWORD	( &dwRes2,		ppBlock );
		g_dwVersion = dwVersion;

		// create object
		(*ppObject) = new sObject;

		// construct frame data
		DWORD dwCode = 0, dwCodeSize = 0;
		ReadCODE ( &dwCode, &dwCodeSize, ppBlock );
		if ( !ConstructFrame ( &(*ppObject)->pFrame, ppBlock ) )
		{
			// cannot construct frame
			return false;
		}

		// construct animation data
		ReadCODE ( &dwCode, &dwCodeSize, ppBlock );
		if ( !ConstructAnimationSet ( &(*ppObject)->pAnimationSet, ppBlock ) )
		{
			// cannot construct animation
			return false;
		}

		// 280305 - new custom data
		// lee - 280306 - u6rc2 - ONLY if there is more block data (older DBO files did not have any more data)
		if ( *ppBlock < g_pBlockEnd )
			ConstructCustomData ( ppObject, ppBlock );
	}
	else
	{
		// not a DBO file
		return false;
	}

	// return result
	return true;
}

//////////////////////////////////////////////////////////////////////////////////
// BLOCK FUNCTIONS ///////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

DARKSDK_DLL bool DBOConvertObjectToBlock ( sObject* pObject, void** ppBlock, DWORD* pdwBlockSize )
{
	// determine size of block
	DWORD dwSize = 0;
	if ( !ScanObject ( pObject, NULL, &dwSize ) )
	{
		// could not parse object
		return false;
	}

	// create block memory
	LPSTR pBlockBase = new char[dwSize];

	// generate block data from object
	DWORD dwBytePosition = 0;
	LPSTR pBlock = pBlockBase;
	if ( !ScanObject ( pObject, &pBlock, &dwBytePosition ) )
	{
		// could not parse object
		SAFE_DELETE(pBlock);
		return false;
	}

	// store block size
	*ppBlock = (void*)pBlockBase;
	*pdwBlockSize = dwSize;

	// return okay
	return true;
}

DARKSDK_DLL bool DBOConvertBlockToObject ( void* pBlock, DWORD dwBlockSize, sObject** ppObject )
{
	// U74 - used to track DBO sizes (to find bugs)
	g_pBlockStart = (LPSTR)pBlock;

	// lee - 280306 - u6rc2 - defeat custom-data read crash (from old DBO files), by recording end of block data gloablly
	g_pBlockEnd = (LPSTR)pBlock + dwBlockSize;

	// construct object from block
	if ( !ConstructObject ( ppObject, (LPSTR*)&pBlock ) )
		return false;

	// return okay
	return true;
}

DARKSDK_DLL bool DBOLoadBlockFile ( LPSTR pFilename, void** ppBlock, DWORD* pdwSize )
{
	// load file
	HANDLE hfile = GG_CreateFile ( pFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hfile != INVALID_HANDLE_VALUE )
	{
		DWORD bytesread=0;
		*pdwSize = GetFileSize ( hfile, NULL );
		*ppBlock = (void*)new char[*pdwSize];
		ReadFile( hfile, (LPSTR)(*ppBlock), *pdwSize, &bytesread, NULL ); 
		CloseHandle ( hfile );
	}
	else
	{
		// could not load file
		return false;
	}

	// okay
	return true;
}

DARKSDK_DLL bool DBOSaveBlockFile ( LPSTR pFilename, void* pBlock, DWORD dwSize )
{
	// save new file
	DeleteFile ( pFilename );
	HANDLE hfile = GG_CreateFile ( pFilename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hfile != INVALID_HANDLE_VALUE )
	{
		DWORD byteswritten=0;
		WriteFile( hfile, (LPSTR)pBlock, dwSize, &byteswritten, NULL ); 
		CloseHandle ( hfile );
	}
	else
	{
		// could not create file
		return false;
	}

	// okay
	return true;
}


