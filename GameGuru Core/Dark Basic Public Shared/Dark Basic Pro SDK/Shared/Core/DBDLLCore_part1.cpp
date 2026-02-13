#endif
#endif

#ifdef ENABLEIMGUI
extern bool bImGuiInTestGame;
#endif

LRESULT CALLBACK WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message )
	{
		case WM_SETTEXT:
		{
		}
		case WM_ACTIVATE:
		{
			#ifdef WICKEDENGINE
			//extern bool g_bActiveApp; moved to main.cpp code
			//g_bActiveApp = true;
			//if (wParam == WA_INACTIVE) g_bActiveApp = false;
			#else
			// 20/7/11 - Win7 - ensure we register for TOUCH over GESTURE (also allows LBUTTONDOWN to happen instantly!)
			HWND hwndPrevious = (HWND) lParam;
			if ( bDetectAndActivateWindows7TouchSystem==false )
			{
				bDetectAndActivateWindows7TouchSystem = true;
				OSVERSIONINFO osvi;
				BOOL bIsWindows7orLater;
				ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
				osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
				GetVersionEx(&osvi);
				bIsWindows7orLater = ( (osvi.dwMajorVersion > 6) || ( (osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion >= 1) ));
				if ( bIsWindows7orLater==TRUE )
				{
					// must dynamically find the user32.dll function and call it IF Windows 7 (allows Windows XP to run)
					typedef UINT (CALLBACK* sRegisterTouchWindowFnc)(HWND,ULONG);
					HMODULE hWinUserDLL = LoadLibrary ( "user32.dll" );
					if ( hWinUserDLL )
					{
						sRegisterTouchWindowFnc pRegTouchWin = (sRegisterTouchWindowFnc) GetProcAddress ( hWinUserDLL, "RegisterTouchWindow" );
						if ( pRegTouchWin ) BOOL bRes = pRegTouchWin ( g_pGlob->hWnd, 0 );
						FreeLibrary ( hWinUserDLL );
					}
				}
			}
			#endif
			break;
		}

		case WM_CLOSE:
		{
			#ifdef DARKSDK_COMPILE
			g_iDarkGameSDKQuit = 1;
			#endif
			#ifdef VRTECH
			if (bImGuiInTestGame) return TRUE;
			if (g_pGlob->hWnd == hWnd)
			{
				// only if window being closed is main one, not the VR secondary window!
				PostQuitMessage(0);
			}
			if (g_pGlob->hOriginalhWnd == hWnd)
			{
				// clear out this invalid HWND, allows software to re-init VR!
				g_pGlob->hOriginalhWnd = NULL;
			}
			#else
				PostQuitMessage(0);
			#endif
			return TRUE;
		}

		case WM_DESTROY:
		case WM_NCDESTROY:
		{
			#ifdef ENABLEIMGUI
			if (bImGuiInTestGame) return TRUE;
			#endif
			PostQuitMessage(0);
			break;
		}

		case WM_ERASEBKGND:
			return TRUE;

		case WM_SIZE:
		case WM_SIZING:
		case WM_MOVE:
		case WM_MOVING:
		case WM_PAINT:
			{
				// 180214 - record new size in glob struct
				RECT rc;

				// GDI Paint
				PAINTSTRUCT ps;
				HDC hdcClient = BeginPaint(hWnd, &ps);
				if(hdcClient)
				{
					if(g_hdcDisplay)
					{
						GetClientRect(hWnd, &rc);
						HGDIOBJ hdcOld = SelectObject(g_hdcDisplay, g_hDisplayBitmap);
						BitBlt(hdcClient, rc.left, rc.top, rc.right, rc.bottom, g_hdcDisplay, 0, 0, SRCCOPY);
						SelectObject(g_hdcDisplay, hdcOld);
					}
					else
					{
						// 210203 - if array of protected boxes setup (from controls requiring primary surface)
						if ( g_pGlob->dwSafeRectMax>0 )
						{
							// Clear Device
							GetClientRect(hWnd, &rc);
							HBRUSH bGrey = GetSysColorBrush ( COLOR_3DFACE );
							HBRUSH bOld = (HBRUSH)SelectObject(hdcClient, bGrey ); 
							Rectangle(hdcClient, -5, -5, rc.right+5, rc.bottom+5);
							SelectObject(hdcClient, bOld ); 
						}
					}
					EndPaint(hWnd, &ps);
				}

				// Ensures rendered areas are retained (when moving window or menu refreshing)
				if ( g_pGlob->dwAppDisplayModeUsing==1 )
				{
					// only dwDisplayMode=1 (window) should do this (otherwise render several times!!)
					// ensure refresh is not done in middle of draw-phase
					End(); Render(); Begin();
				}
			}
			return TRUE;

		case WM_MOUSEMOVE:
			{
				// Get Client Raw Mouse Position
				g_pGlob->iWindowsMouseX = LOWORD(lParam);  // horizontal position of cursor 
				g_pGlob->iWindowsMouseY = HIWORD(lParam);  // vertical position of cursor 
				
				// Special Scale for When Windows Stretch Beyond Physical Size of Backbuffer
				RECT rc;
				GetClientRect(hWnd, &rc);
				float xRatio = (float)g_pGlob->dwWindowWidth/(float)rc.right;
				float yRatio = (float)g_pGlob->dwWindowHeight/(float)rc.bottom;
				g_pGlob->iWindowsMouseX = (int)((float)g_pGlob->iWindowsMouseX * xRatio);
				g_pGlob->iWindowsMouseY = (int)((float)g_pGlob->iWindowsMouseY * yRatio);

				// Restore cursor when move mouse
				if ( g_ActiveCursor != NULL )
					SetCursor( g_ActiveCursor );

			}
			break;

		case WM_LBUTTONDOWN:
			g_pGlob->iWindowsMouseClick|=1;
			g_pGlob->dwWindowsMouseLeftTouchPersist=timeGetTime()+250; // U76 - many cycles
			if ( GetFocus()!=hWnd ) 
			{
				SetFocus ( hWnd );
			}
			break;

		case WM_RBUTTONDOWN:
			g_pGlob->iWindowsMouseClick|=2;
			if ( GetFocus()!=hWnd ) SetFocus ( hWnd );
			break;

		// aaron - 20120811 - Potential issues when using xor depending on obscure and rare window interaction
		case WM_LBUTTONUP:
			g_pGlob->iWindowsMouseClick &= ~1UL;
			break;

		case WM_RBUTTONUP:
			g_pGlob->iWindowsMouseClick &= ~2UL;
			break;

		case WM_SYSKEYDOWN:
			g_wWinKey = wParam;
			break;

		case WM_KEYDOWN:
			g_wWinKey = wParam;
			if((int)wParam==VK_ESCAPE)
			{
				if(g_EscapeValue) *(DWORD*)g_EscapeValue=1;
				if(g_pGlob->bEscapeKeyEnabled)
				{
					#ifdef DARKSDK_COMPILE
					g_iDarkGameSDKQuit = 1;
					#endif
					#ifdef ENABLEIMGUI
					if (bImGuiInTestGame) return TRUE;
					#endif
					PostQuitMessage(0);
				}
			}
			return TRUE;

		case WM_SYSKEYUP:
			g_wWinKey=0; 
			return TRUE;

		case WM_KEYUP:
			g_cInkeyCodeKey=0;
			g_wWinKey=0;
			return TRUE;

		case WM_CHAR:

			// If win string cleared externally (InputDLL)
			if(g_pGlob->pWindowsTextEntry)
				if(g_pGlob->pWindowsTextEntry[0]==0)
					g_dwWindowsTextEntryPos=0;

			// Key that was pressed
			g_cKeyPressed = (unsigned char)wParam;
			g_cInkeyCodeKey = g_cKeyPressed;
			return TRUE;

			// windows text entry is handled in Entry() now as it gets the info from IDE
			// Ensure string is always big enough
			if(g_pGlob->pWindowsTextEntry==NULL)
			{
				g_dwWindowsTextEntrySize = 32;
				g_pGlob->pWindowsTextEntry = new char[g_dwWindowsTextEntrySize];
				g_dwWindowsTextEntryPos = 0;
			}
			if(g_dwWindowsTextEntryPos>g_dwWindowsTextEntrySize-4)
			{
				g_dwWindowsTextEntrySize = g_dwWindowsTextEntrySize * 2;
				LPSTR pNewString = new char[g_dwWindowsTextEntrySize];
				strcpy(pNewString, g_pGlob->pWindowsTextEntry);
				delete[] g_pGlob->pWindowsTextEntry;
				g_pGlob->pWindowsTextEntry=pNewString;
			}

			// Add character to entry string
			g_pGlob->pWindowsTextEntry[g_dwWindowsTextEntryPos]=g_cKeyPressed;
			g_dwWindowsTextEntryPos++;
			g_pGlob->pWindowsTextEntry[g_dwWindowsTextEntryPos]=0;

			return TRUE;

		case WM_USER+1: // Show/Hide Cursor
			if(wParam==0) ShowCursor(FALSE);
			if(wParam==1) ShowCursor(TRUE);
			return TRUE;
	}
	
	// Default Action
	return DefWindowProc(hWnd, message, wParam, lParam);
}

DARKSDK void InternalClearWindowsEntry(void)
{
	if(g_pGlob->pWindowsTextEntry)
	{
		strcpy(g_pGlob->pWindowsTextEntry,"");
		g_dwWindowsTextEntryPos		= 0;
		g_cKeyPressed				= 0;
	}
}

DARKSDK DWORD InternalProcessMessages(void)
{
	DWORD dwResult = ProcessMessagesOnly();
	ExternalDisplayUpdate();
	return dwResult;
}

DARKSDK DWORD ProcessMessages(DWORD dwPositionInMachineCode)
{
	// Process Messages from a program in debug mode
	DWORD dwReturnValue=0;

	// When breakout position filled, leave immediately
	if(g_BreakOutPosition)
	{
		// If Exit requested, store position before leave
		if(*(DWORD*)g_BreakOutPosition==1)
		{
			*(DWORD*)g_BreakOutPosition=dwPositionInMachineCode;
			return 1;
		}
	}

	// Process Internal Message Loop
	dwReturnValue = InternalProcessMessages();

	// Return Value
	return dwReturnValue;
}

DARKSDK DWORD ProcessMessages(void)
{
	// Process Messages from a program in fullspeed mode
	return InternalProcessMessages();
}

DARKSDK DWORD Quit(void)
{
	// Initate Cascade Quit
	g_bCascadeQuitFlag=true;
	if(g_EscapeValue) *(DWORD*)g_EscapeValue=2;

	// Process any other tasks during Final QUIT
	if(g_pGlob->pExitPromptString)
	{
		// Produce an Exit Window with Strings
		MessageBox(NULL, g_pGlob->pExitPromptString, g_pGlob->pExitPromptString2, MB_OK);

		// Free Strings
		SAFE_DELETE(g_pGlob->pExitPromptString);
		SAFE_DELETE(g_pGlob->pExitPromptString2);
	}

	// Complete
	return 0;
}

DARKSDK void StackSnapshotStore(DWORD dwStackPositionNow)
{
	// No Stack save - data would be useless when new m/c executed
}

DARKSDK DWORD StackSnapshotRestore(void)
{
	// No Stack save - data would be useless when new m/c executed
	return 0;
}

// aaron - 20120811 - more flexible memory management routine
// p: in pointer (nullptr: alloc; else: realloc)
// n: size to have (0: free)
DB_EXPORT void *ManageMemory(void *p, size_t n) {
	void *q;

	if (n) {
		if (p)
			q = realloc(p, n);
		else
			q = malloc(n);
	} else {
		if (p)
			free(p);

		return nullptr;
	}

	return q;
}

DARKSDK int TestMemory ( int iSizeInBytes )
{
	try
	{
		void* pMem = new char[iSizeInBytes];
		if ( pMem )
		{
			// can still reserve memory chunk
			delete pMem;
			return 1;
		}
		else
			return 0;
	}
	catch(...)
	{
		return 0;
	}
	return 0;
}

/*
DARKSDK void CreateSingleString(DWORD* dwVariableSpaceAddress, DWORD dwSize)
{
	if(dwSize>0)
	{
		// Create a core string
		*dwVariableSpaceAddress = (DWORD)new char[dwSize];
	}
	else
	{
		// Delete a core string
		delete[] (LPSTR)*dwVariableSpaceAddress;
	}
}
*/

DARKSDK void Break(void)
{
	// Set Escape Value to Break Into Debugger
	if(g_EscapeValue) *(DWORD*)g_EscapeValue=1;
}

DARKSDK LRESULT SendDataToDebugger(int iType, LPSTR pData, DWORD dwDataSize)
{
	LRESULT lResult=0;

	// Create Virtual File for Transfer
	HANDLE hFileMap = CreateFileMapping((HANDLE)0xFFFFFFFF,NULL,PAGE_READWRITE,0,dwDataSize,"DBPROEDITORMESSAGE");
	if(hFileMap)
	{
		LPVOID lpVoid = MapViewOfFile(hFileMap,FILE_MAP_WRITE,0,0,dwDataSize+4);
		if(lpVoid)
		{
			// Copy to Virtual File
			*(DWORD*)lpVoid = dwDataSize;
			memcpy((LPSTR)lpVoid+4, pData, dwDataSize);

			// Find Debugger to send to
			HWND hWnd = FindWindow(NULL,"DBProDebugger");
			if(hWnd)
			{
				// Found - transmit
				lResult = SendMessage(hWnd, WM_USER+10, iType, 0);
			}

			// Release virtual file
			UnmapViewOfFile(lpVoid);
		}
		CloseHandle(hFileMap);
	}

	// May have result
	return lResult;
}

DARKSDK void BreakS(DWORD pString)
{
	// Send String to CLI Debug Console
	LPSTR lpReturnError = new char[1024];
	wsprintf(lpReturnError, "%s", pString);
	SendDataToDebugger(31, lpReturnError, strlen(lpReturnError));
	delete[] lpReturnError;
	lpReturnError=NULL;

	// Set Escape Value to Break Into Debugger
	if(g_EscapeValue) *(DWORD*)g_EscapeValue=1;
}

DARKSDK bool COREDoesFileExist(LPSTR Filename)
{
	// success or failure
	bool bSuccess = true;

	// open File To See If Exist
	HANDLE hfile = GG_CreateFile(Filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hfile==INVALID_HANDLE_VALUE)
		bSuccess=false;
	else
		CloseHandle(hfile);

	// return result
	return bSuccess;
}

DARKSDK void UpdateFilenameFromVirtualTable( DWORD dwStringAddress )
{
	#ifdef VRTECH
	#else
	// no longer use embedded virtual media in EXE
	// String is input with external filename
	if(dwStringAddress==0)
		return;
	
	// If Virtual Table area available
	if(g_pGlob->pEXEUnpackDirectory==NULL)
		return;

	// Construct path to virtual file (if it is there or not) leefix - 200704 - can get very big!
	LPSTR pFilename = new char[_MAX_PATH*3];
	strcpy(pFilename, g_pGlob->pEXEUnpackDirectory);
	strcat(pFilename, "\\media\\");
	strcat(pFilename, (LPSTR)dwStringAddress);

	// If File exists, use that instead of external file
	if(COREDoesFileExist(pFilename)==true)
	{
		// Virtual Table File better than local external file
		strcpy((LPSTR)dwStringAddress, pFilename);
	}

	// Free usages
	delete[] pFilename;
	#endif
}

DARKSDK LPSTR ReadFileData(LPSTR FilenameString, DWORD* dwDataSize)
{
	// Read File Data
	HANDLE hreadfile = GG_CreateFile(FilenameString, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hreadfile==INVALID_HANDLE_VALUE)
		return NULL;

	// Read readout file into memory
	DWORD bytesread=0;
	DWORD filebuffersize = GetFileSize(hreadfile, NULL);	
	LPSTR filebuffer = (char*)GlobalAlloc(GMEM_FIXED, filebuffersize);
	ReadFile(hreadfile, filebuffer, filebuffersize, &bytesread, NULL); 
	CloseHandle(hreadfile);

	*dwDataSize = filebuffersize;
	return filebuffer;
}

DARKSDK void WriteFileData(LPSTR pFilename, LPSTR pData, DWORD dwDataSize)
{
	// Delete existing file
	DeleteFile(pFilename);

	// Write New File with new data
	DWORD byteswritten=0;
	HANDLE hfile = GG_CreateFile(pFilename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hfile!=INVALID_HANDLE_VALUE)
	{
		WriteFile(hfile, pData, dwDataSize, &byteswritten, NULL); 
		CloseHandle(hfile);
	}
}

//PE: This one could be a real memory hook, not sure if everything is freed from all calls ?
//PE: Test showed 942915 bytes allocated after a startup and loading a level. so not that bad.
//PE: But list was 78413 long , so ?
#ifdef DOMEMTESTHERE
std::vector<void *> mem_test;
std::vector<int> mem_test_size;
#endif
DARKSDK void CreateDeleteString(char** pMemory, DWORD dwSize)
{
	#ifdef DOMEMTESTHERE
	if (mem_test.size() > 30000)
	{
		int mem_use = 0;
		for (int i = 0; i < mem_test_size.size(); i++)
		{
			mem_use += mem_test_size[i];
		}
		if(mem_use > 1024 * 1024 * 1024)
			printf("1gb. %d", mem_use);
		if (mem_use > 500 * 1024 * 1024)
			printf("500mb. %d", mem_use);
		if (mem_use > 200 * 1024 * 1024)
			printf("200mb. %d", mem_use);
	}
	#endif
	if (dwSize > 0)
	{
		*pMemory = new char[dwSize];
		memset(*pMemory, 0, dwSize);
		#ifdef DOMEMTESTHERE
		mem_test.push_back(*pMemory);
		mem_test_size.push_back(dwSize);
		#endif
	}
	else
	{
		#ifdef DOMEMTESTHERE
		for (int i = 0; i < mem_test.size() ; i++)
		{
			if (mem_test[i] == *pMemory)
			{
				mem_test.erase(mem_test.begin() + i);
				mem_test_size.erase(mem_test_size.begin() + i);
				break;
			}
		}
		#endif
		delete *pMemory;
		*pMemory = NULL;
	}
}

DARKSDK void EncryptDecrypt( char* pStringAddress, bool bEncryptIfTrue, bool bDoNotUseTempFolder )
{
	CallEncryptDecrypt ( pStringAddress, bEncryptIfTrue, bDoNotUseTempFolder );
}

DARKSDK void Decrypt( char* pStringAddress )
{
	EncryptDecrypt ( pStringAddress, false, false );
}

DARKSDK void Encrypt( char* pStringAddress )
{
	EncryptDecrypt ( pStringAddress, true, false );
}

DARKSDK void EncryptDBPro ( DWORD dwStringAddress )
{
	LPSTR pFilename = new char[_MAX_PATH];
	strcpy(pFilename, (LPSTR)dwStringAddress);
	if(!COREDoesFileExist(pFilename))
		return;

	char newFileName[_MAX_PATH];
	sprintf ( newFileName , "_e_%s" , (LPSTR)dwStringAddress );

	char buf[BUFSIZ];
	size_t size;

	FILE* source = GG_fopen( (LPSTR)dwStringAddress , "rb");
	FILE* dest = GG_fopen(newFileName, "wb");

	// clean and more secure
	// feof(FILE* stream) returns non-zero if the end of file indicator for stream is set

	while (size = fread(buf, 1, BUFSIZ, source))
	{
		fwrite(buf, 1, size, dest);
	}

	fclose(source);
	fclose(dest);

	EncryptDecrypt ( newFileName, true, true );
}

DARKSDK void EncryptWorkshopDBPro ( char* dwStringAddress )
{
	LPSTR pFilename = new char[_MAX_PATH];
	strcpy(pFilename, (LPSTR)dwStringAddress);
	if(!COREDoesFileExist(pFilename))
		return;

	char originalPath[MAX_PATH];
	GetCurrentDirectory ( MAX_PATH, originalPath );

	char* pLocalFile = NULL;
	char filePath[MAX_PATH];
	strcpy( filePath, pFilename );
	pLocalFile = strrchr ( pFilename , '\\' );
	if ( pLocalFile )
	{	
		strcpy ( pFilename, pLocalFile+1 );
		pLocalFile = strrchr ( filePath , '\\' );
		pLocalFile[0] = '\0';
		SetCurrentDirectory ( filePath );
	}

	char newFileName[_MAX_PATH];
	sprintf ( newFileName , "_w_%s" , pFilename );

	char buf[BUFSIZ];
	size_t size;

	FILE* source = GG_fopen( pFilename , "rb");
	FILE* dest = GG_fopen(newFileName, "wb");

	// clean and more secure
	while (size = fread(buf, 1, BUFSIZ, source))
	{
		fwrite(buf, 1, size, dest);
	}

	fclose(source);
	fclose(dest);

	EncryptDecrypt ( newFileName, true, true );

	SetCurrentDirectory(originalPath);
}

DARKSDK bool EncryptNewFile ( LPSTR pStringAddress )
{
	// do not encrypt any sky models (as they use internal image loads which are also encrypted 
	// and I cannot load encryped files from within the temp folder where the decrypted.x is)
	LPSTR pScanFilename = pStringAddress;
	char pThisDirAndFile[MAX_PATH];
	GetCurrentDirectory ( MAX_PATH, pThisDirAndFile );
	strcat ( pThisDirAndFile, "\\" );
	strcat ( pThisDirAndFile, pScanFilename );
	int iScanMax = strlen(pThisDirAndFile)-8;
	if ( iScanMax < 0 ) iScanMax = 0;
	if ( strlen ( pThisDirAndFile ) > 8 )
	{
		for ( int n=0; n<iScanMax; n++ )
		{
			if ( strnicmp ( pThisDirAndFile + n, "skybank\\", 8 )==NULL || strnicmp ( pThisDirAndFile + n, "skybank/", 8 )==NULL )
			{
				if ( strnicmp ( pThisDirAndFile + n, "skybank\\", 8 )==NULL || strnicmp ( pThisDirAndFile + n, "skybank/", 8 )==NULL )
				{
					if ( strnicmp ( pThisDirAndFile + strlen(pThisDirAndFile) - 2, ".x", 2 )==NULL )
					{
						return false;
					}
				}
			}
		}
	}

	char newFileName[_MAX_PATH];
	sprintf ( newFileName , "_e_%s" , pStringAddress );

	char buf[BUFSIZ];
	size_t size;

	FILE* source = GG_fopen( pStringAddress , "rb");
	FILE* dest = GG_fopen(newFileName, "wb");

	// clean and more secure
	while (size = fread(buf, 1, BUFSIZ, source))
	{
		fwrite(buf, 1, size, dest);
	}

	fclose(source);
	fclose(dest);

	EncryptDecrypt ( newFileName, true, true );

	return true;
}

// Delete any empty folders
DARKSDK void EncryptAllFiles(char* dwStringAddress)
{
	LPSTR pFilename = new char[_MAX_PATH];
	strcpy(pFilename, dwStringAddress);

	HANDLE			hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA data  = { 0 };
    
	std::stack  < char* > directoryListStack;

	char folderToCheck[MAX_PATH];
	sprintf ( folderToCheck , pFilename );

	// add first directory into the listing
	directoryListStack.push ( folderToCheck );

	// Added Code to pre-compile Lua scripts
	#ifndef NOSTEAMORVIDEO
	LoadLua("ggprecompile.lua");
	#endif

	// keep going until we have emptied the directory stack
	while ( !directoryListStack.empty ( ) )
	{
		// get the first directory
		char  szLocation [ 256 ] = "";
		char* szCurrentDirectory = directoryListStack.top ( );
		
		// now add this to the location to check plus no mask so we search for everything
		sprintf ( szLocation, "%s\\*.*", szCurrentDirectory );

		// pop this directory off the stack
		directoryListStack.pop ( );

		// find the first file in the location
		hFind = FindFirstFile ( szLocation, &data );

		// break if nothing is there
		if ( hFind == INVALID_HANDLE_VALUE )
			break;

		// cycle through all files
		do 
		{
			// only proceed if it's not . or ..
			if ( strcmp ( data.cFileName, "." ) != 0 && strcmp ( data.cFileName, ".." ) != 0 )
			{
				// deal with a directory
				if ( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				{
					// add this directory onto the stack
					char* p = new char [ 256 ];
					
					sprintf ( p, "%s\\%s", szCurrentDirectory, data.cFileName );
					
					directoryListStack.push ( p );
				}
				else
				{
					if ( strstr(data.cFileName, ".fpe") != NULL || 
						 strstr(data.cFileName, ".dds") != NULL ||  
						 strstr(data.cFileName, ".png") != NULL || 
						 strstr(data.cFileName, ".jpg") != NULL ||
						 strstr(data.cFileName, ".x")   != NULL ||
						 strstr(data.cFileName, ".dbo") != NULL ||
						 strstr(data.cFileName, ".wav") != NULL ||
						 strstr(data.cFileName, ".ogg") != NULL ||
						 strstr(data.cFileName, ".mp3") != NULL )
					{
						// dont encrypt a file if it already is
						if ( strstr ( data.cFileName, "_e_" )  !=  data.cFileName )
						{
							// encrypt the file
							char p[ MAX_PATH ];
							char f[ MAX_PATH ];
					
							sprintf ( p, "%s\\", szCurrentDirectory );
							strcpy ( f , data.cFileName );

							// do not encrypt certain folders
							LPSTR pMatch = "";
							bool bDoEnc = true;
							pMatch = "Files\\editors";  if (strstr (szCurrentDirectory, pMatch) != NULL) bDoEnc = false;
							pMatch = "Files\\effectbank";  if (strstr (szCurrentDirectory, pMatch) != NULL) bDoEnc = false;
							pMatch = "Files\\terraintextures";  if (strstr (szCurrentDirectory, pMatch) != NULL) bDoEnc = false;
							pMatch = "Files\\treebank";  if (strstr (szCurrentDirectory, pMatch) != NULL) bDoEnc = false;
							pMatch = "Files\\grassbank";  if (strstr (szCurrentDirectory, pMatch) != NULL) bDoEnc = false;
							//PE: particlesbank use GPUP_LoadTexture that do not support decrypt.
							pMatch = "Files\\particlesbank";  if (strstr(szCurrentDirectory, pMatch) != NULL) bDoEnc = false;

							if (bDoEnc == true)
							{
								char originalFolder[MAX_PATH];
								GetCurrentDirectory (MAX_PATH, originalFolder);
								SetCurrentDirectory (szCurrentDirectory);
								bool bEncryptedOkay = EncryptNewFile(f);
								SetCurrentDirectory (originalFolder);
								UpdateWindow (NULL);
								sprintf (p, "%s\\%s", szCurrentDirectory, f);
								if (bEncryptedOkay == true) DeleteFile (p);
							}
						}
					}
					else 
					{
						#ifndef NOSTEAMORVIDEO
						if ( strstr(data.cFileName, ".lua") != NULL &&
							 strstr(data.cFileName, "multiplayer") == NULL )
						{
							// Precompile lua script, note: overwrites file, in theory if the call
							// fails for any reason the file should remain uncompiled.
							char p[MAX_PATH];

							sprintf(p, "%s\\%s", szCurrentDirectory, data.cFileName);

							LuaSetFunction("ggprecompile", 1, 0);
							LuaPushString(p);
							LuaCall();
						}
						#endif
					}
				}
			}
		}
		while ( FindNextFile ( hFind, &data ) != 0 );

		// now break out if needed
		if ( GetLastError ( ) != ERROR_NO_MORE_FILES )
		{
			FindClose ( hFind );
			break;
		}

		FindClose ( hFind );
		hFind = INVALID_HANDLE_VALUE;
	}
}

DARKSDK void ConstructPostDisplayItems(HINSTANCE hInstance)
{
	SETUPPassCoreData((LPVOID)g_pGlob, 1);
	Basic2DConstructor();
	ImageConstructor();
	SpritesConstructor();
	InfiniteVegetationConstructor();
}

DARKSDK void ConstructPostDLLItems(HINSTANCE hInstance,bool bNeededToCreateExtraWindowForWMRWindow)
{
	InputConstructor(bNeededToCreateExtraWindowForWMRWindow);
	SystemConstructor();
	SoundConstructor();
	FileConstructor();
	FTPConstructor();
	MemblocksConstructor();
	BitmapConstructor();
	CameraConstructor();
	LightConstructor();
	Basic3DConstructor();
	VectorConstructor();
	BULLETReceiveCoreDataPtr();
	OccluderConstructor();
	#ifndef NOSTEAMORVIDEO
	AnimationConstructor();
	LuaConstructor();
	#endif
}

DARKSDK void PassCmdLineHandlerPtr(LPVOID pCmdLinePtr)
{
	// Store pointer to command line string passed into EXE
	g_pCommandLineString = (LPSTR)pCmdLinePtr;
}

DARKSDK void PassErrorHandlerPtr(LPVOID pErrorPtr)
{
	// Store position of runtime error DWORD (held in executable dataspace)
	g_ErrorHandler = pErrorPtr;
	g_pErrorHandler = (CRuntimeErrorHandler*)pErrorPtr;

	// LEEMOD - 150803 - Also store reference in GLOBSTRUCT for ThirdPartyDLLs
	g_pGlob->g_pErrorHandlerRef = pErrorPtr;

	// Clear error clue
	strcpy ( g_strErrorClue, "" );
}

DARKSDK void ChangeMouse( DWORD dwCursorID )
{
	// Set Cursor Shape (0-31)
	if ( dwCursorID==0 ) g_ActiveCursor=g_hUseArrow;
	if ( dwCursorID==1 ) g_ActiveCursor=g_hUseHourglass;
	if ( dwCursorID>=2 && dwCursorID<=31 ) g_ActiveCursor=g_hCustomCursors[dwCursorID-2];
	if ( dwCursorID==32 ) g_ActiveCursor=NULL;
	if ( dwCursorID<=31 )
	{
		// change cursor
		SetCursor( g_ActiveCursor );
	}
}

DARKSDK void InitDisplayAndGlob(HINSTANCE hInst, HWND hMainWnd, LPSTR gUnpackDirectory, int iDisplayWidth, int iDisplayHeight)
{
	// Global Shared Data
	ZeroMemory(g_pGlob, sizeof(GlobStruct));
	g_pGlob->bWindowsMouseVisible		= true;
	g_pGlob->dwForeColor				= -1; // (white)
	g_pGlob->dwBackColor				= 0;
	g_pGlob->bEscapeKeyEnabled			= true;
	g_pGlob->iScreenWidth				= iDisplayWidth;
	g_pGlob->iScreenHeight				= iDisplayHeight;
	g_pGlob->dwWindowWidth				= iDisplayWidth;
	g_pGlob->dwWindowHeight				= iDisplayHeight;

	// this function assumes from of the logic performed in InitDisplayEx (excluding stuff now done by Wicked)
	memset ( g_pGlob->pEXEUnpackDirectory, 0, _MAX_PATH );
	strcpy(g_pGlob->pEXEUnpackDirectory, gUnpackDirectory);
	g_pGlob->CreateDeleteString = CreateDeleteString;
	g_pGlob->hInstance = hInst;
	g_pGlob->hWnd = hMainWnd;
	g_pGlob->Decrypt = Decrypt;
	g_pGlob->Encrypt = Encrypt;

	OverrideHWND(g_pGlob->hWnd);
}

