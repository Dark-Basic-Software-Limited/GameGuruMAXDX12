DARKSDK DWORD InitDisplayEx(DWORD dwDisplayType, DWORD dwWidth, DWORD dwHeight, DWORD dwDepth, HINSTANCE hInstance, LPSTR pApplicationName, HWND pParentHWND, DWORD dwInExStyle, DWORD dwInStyle)
{
	// dwDisplayType
	// =============
	// 0=Hidden Mode
	// 1=Window Mode
	// 2=Desktop Fullscreen Mode
	// 3=Exclusive Fullscreen Mode
	// 4=Desktop Fullscreen Mode (No Taskbar)
	// 5=Use EX and STYLE from values passed in

	// System Settings
	g_dwScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	g_dwScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	// Window Default Settings
	bool bWindowIsDisplayable=true;
	g_pGlob->dwWindowX = 0;
	g_pGlob->dwWindowY = 0;

	// Apply size of screen to global data
	g_pGlob->dwWindowWidth = dwWidth;
	g_pGlob->dwWindowHeight = dwHeight;
	g_pGlob->iScreenWidth = dwWidth;
	g_pGlob->iScreenHeight = dwHeight;
	g_pGlob->iScreenDepth = dwDepth;

	if(g_pGlob->g_GFX==NULL)
	{
		// Using GDI for Display
		if(dwDisplayType==2)
		{
			// Fullscreen Mode - With Taskbar
			RECT rc;
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
			g_pGlob->dwWindowWidth = rc.right-rc.left;
			g_pGlob->dwWindowHeight = rc.bottom-rc.top;
		}
		if(dwDisplayType>=3)
		{
			// Fullscreen Mode - Simply Resize Window
			g_pGlob->dwWindowWidth = g_dwScreenWidth;
			g_pGlob->dwWindowHeight = g_dwScreenHeight;
		}
	}
	
	// Window Settings
	DWORD dwWindowStyle=0;
	DWORD dwWindowExStyle=0;
	switch(dwDisplayType)
	{
		// leechange - 101004 - should be FULL DESKTOP FULLSCREEN if made visible
		case 0 :	dwWindowStyle = WS_POPUP; // HIDDEN APP   // was WS_MINIMIZE;
					bWindowIsDisplayable=false;
					break;

		case 1 :	dwWindowStyle = WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU; // WINDOW APP
					break;

		case 2 :	dwWindowStyle = WS_POPUP; // DESKTOP FULLSCREEN (see taskbar)
					break;

		case 3 :	dwWindowStyle = WS_POPUP; // EXCLUSIVE FULLSCREEN
					break;

		case 4 :	dwWindowStyle = WS_POPUP; // FULL DESKTOP FULLSCREEN (no taskbar)
					break;

		case 5 :	dwWindowStyle = dwInStyle;	// PASSED IN USING DARKGDKINIT
					dwWindowExStyle = dwInExStyle;
					break;

		case 6 :	dwWindowStyle = dwInStyle;	// PASSED IN USING DARKGDKINIT (HIDDEN)
					dwWindowExStyle = dwInExStyle;
					bWindowIsDisplayable=false;
					break;
	}

	// Icons and Cursors (loaded earlier when still in root folder)
	g_hUseIcon = g_pGlob->hAppIcon;// (HICON)LoadImageA(hInstance, pIconFromRootFolder, IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
	
	// Load Custom Cursors (first slot is ARROW, second is WAIT and third onwards is own)
	HCURSOR hCursor = NULL;
	hCursor = (HCURSOR)LoadImageA(hInstance, "arrow.cur", IMAGE_CURSOR, 32, 32, LR_LOADFROMFILE);
	if (hCursor) g_hUseArrow=hCursor;
	hCursor = (HCURSOR)LoadImageA(hInstance, "hourglass.cur", IMAGE_CURSOR, 32, 32, LR_LOADFROMFILE);
	if (hCursor) g_hUseHourglass=hCursor;
	for ( DWORD c=2; c<32; c++)
	{
		char str[_MAX_PATH];
		wsprintf(str, "pointer%d.cur", c);
		hCursor = (HCURSOR)LoadImageA(hInstance, str, IMAGE_CURSOR, 32, 32, LR_LOADFROMFILE);
		g_hCustomCursors[c-2]=hCursor;
	}

	// Use Default Cursor otherwise
	if(g_hUseArrow==NULL) g_hUseArrow = LoadCursor(NULL, IDC_ARROW);
	if(g_hUseHourglass==NULL) g_hUseHourglass = LoadCursor(NULL, IDC_WAIT);

	// Vars
	WNDCLASS wc;

	// Appname
	char pAppName[256];
	char pAppNameUnique[256];

	//PE: Use the exe filenane as the title in the game.
	//PE: So if you use Test-my-Game_name.exe as the standalone
	//PE: the windows title will be "Test my Game name".
	char workstring[1024];
	GetModuleFileName(NULL, workstring, 1024);
	bool bWeAreEditor = false;
	#ifdef VRTECH
	if (strcmp(Lower(Right(workstring, 16)), "vr quest app.exe") == 0 
	||  strcmp(Lower(Right(workstring, 15)), "gamegurumax.exe") == 0 )
	{
		if ( strcmp(Lower(Right(workstring, 16)), "vr quest app.exe") == 0 )
			strcpy(pAppName, "VR Quest");
		else
			strcpy(pAppName, "GameGuru MAX");
		bWeAreEditor = true;
	#else
	if (strcmp(Lower(Right(workstring, 18)), "guru-mapeditor.exe") == 0)
	{
		strcpy(pAppName, "Game Guru");
		bWeAreEditor = true;
	#endif
	}
	else 
	{
		strcpy(pAppName, "GameGuru");
		TCHAR * out;
		out = PathFindFileName(workstring);
		if (out != NULL) 
		{
			*(PathFindExtension(out)) = 0;
			for (int i = strlen(out); i > 0; i--) {
				if (out[i] == '-') out[i] = ' ';
				if (out[i] == '_') out[i] = ' ';
			}
			if (strlen(out) > 0)
				strcpy(pAppName, out);
		}
	}

	// Extract path from ModuleFileName so can locate any DLLs in GG Root Folder
	char pRootPath[1024];
	strcpy ( pRootPath, workstring );
	for ( int n = strlen(pRootPath); n > 0; n-- )
	{
		if ( pRootPath[n] == '\\' || pRootPath[n] == '/' )
		{
			pRootPath[n] = 0;
			break;
		}
	}

	// this ensures no conflict between window class name and application class name
	#ifdef VRTECH
	strcpy(pAppNameUnique, "GameGuru"); //PE: This is just for the window class name. (so mutex can find it).
	strcat ( pAppNameUnique, "12345" );
	#else
	strcpy ( pAppNameUnique, pAppName );
	strcat ( pAppNameUnique, "12345" );
	#endif

	// Register window
	if ( g_pGlob->hWnd==NULL )
	{
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = g_hUseIcon;
		wc.hCursor = NULL;
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = pAppNameUnique;
		RegisterClass( &wc );
	}

	// Icon Set Manually (also in winproc too - for cursor restore control)
	g_ActiveCursor = g_hUseArrow;
	g_OldCursor = SetCursor ( g_ActiveCursor );

	// If running in window mode, start in center of screen
	if ( dwDisplayType==1 )
	{
		g_pGlob->dwWindowX=(GetSystemMetrics(SM_CXSCREEN)-g_pGlob->dwWindowWidth)/2;
		g_pGlob->dwWindowY=(GetSystemMetrics(SM_CYSCREEN)-g_pGlob->dwWindowHeight)/2;
	}

	// Create Window (if one not already created)
	g_pGlob->hInstance = hInstance;
	if ( g_pGlob->hWnd )
	{
		// override window handle with new winproc
		#ifdef WICKEDENGINE
		SetWindowLong ( g_pGlob->hWnd, GWLP_WNDPROC, (LONG)WindowProc );
		#else
		SetWindowLong ( g_pGlob->hWnd, GWL_WNDPROC, (LONG)WindowProc );
		#endif
	}
	else
	{
		// hidden window
		g_pGlob->hWnd = CreateWindow( pAppNameUnique, pAppName, dwWindowStyle, g_pGlob->dwWindowX, g_pGlob->dwWindowY, g_pGlob->dwWindowWidth, g_pGlob->dwWindowHeight, NULL, NULL, hInstance, NULL);
	}

#if defined(ENABLEIMGUI) && !defined(USEOLDIDE)
	bool bNeededToCreateExtraWindowForWMRWindow = false;
	if ( bWeAreEditor == true )
	{
		// Keep current windows and use for imgui interface.
		//g_pGlob->hOriginalhWnd = g_pGlob->hWnd; // this hOriginalhWnd used for VR!

		// Regular call to setup rendering system
		SETUPConstructor();

		// PE: Imgui register ImguiWindowProc so it can be used for IMGUI input in editor.
		#ifdef WICKEDENGINE
		///SetWindowLong(g_pGlob->hWnd, GWLP_WNDPROC, (LONG)ImguiWindowProc); moved to main.cpp
		#else
		SetWindowLong(g_pGlob->hWnd, GWL_WNDPROC, (LONG)ImguiWindowProc);
		#endif

		// Initialise DisplayDLL
		OverrideHWND(g_pGlob->hWnd);
		/*
		// Editor still needs secondary window if VR is being used
		WNDCLASS wc2;
		wc2.style = CS_HREDRAW | CS_VREDRAW;
		wc2.lpfnWndProc = WindowProc;
		wc2.cbClsExtra = 0;
		wc2.cbWndExtra = 0;
		wc2.hInstance = hInstance;
		wc2.hIcon = g_hUseIcon;
		wc2.hCursor = NULL;
		wc2.hbrBackground = NULL;
		wc2.lpszMenuName = NULL;
		wc2.lpszClassName = "TheGameWindowClass";
		RegisterClass(&wc2);
		g_pGlob->hOriginalhWnd = CreateWindow ( // was g_pGlob->hWnd
			"TheGameWindowClass",
			"TheGameWindow",
			0,//WS_VISIBLE,
			CW_USEDEFAULT,
			0,
			CW_USEDEFAULT,
			0,
			nullptr,
			nullptr,
			hInstance,
			nullptr);

		// trigger a new window to be created for VR
		bNeededToCreateExtraWindowForWMRWindow = true;
		*/
	}
	else 
	{
		// Main Setup init
		g_pGlob->hOriginalhWnd = g_pGlob->hWnd;
		bool bDXFailed = false;
		if (SETUPConstructor() == true)
		{
			SETUPPassCoreData(g_pGlob, 0);
			if (m_pDX == NULL) bDXFailed = true;
		}
		else
			bDXFailed = true;
		/*
		// Need window for game so original window can stay hidden until VR activates (used by standalone game exe)
		if (g_pGlob->hOriginalhWnd == g_pGlob->hWnd)
		{
			WNDCLASS wc2;
			wc2.style = CS_HREDRAW | CS_VREDRAW;
			wc2.lpfnWndProc = WindowProc;
			wc2.cbClsExtra = 0;
			wc2.cbWndExtra = 0;
			wc2.hInstance = hInstance;
			wc2.hIcon = g_hUseIcon;
			wc2.hCursor = NULL;
			wc2.hbrBackground = NULL;
			wc2.lpszMenuName = NULL;
			wc2.lpszClassName = "TheGameWindowClass";
			RegisterClass(&wc2);
			g_pGlob->hWnd = CreateWindow(
				"TheGameWindowClass",
				"TheGameWindow",
				WS_VISIBLE,
				CW_USEDEFAULT,
				0,
				CW_USEDEFAULT,
				0,
				nullptr,
				nullptr,
				hInstance,
				nullptr);
			bNeededToCreateExtraWindowForWMRWindow = true;
		}
		*/
		// Show Window
		ShowWindow(g_pGlob->hWnd, SW_SHOW);

		// Initialise DisplayDLL
		OverrideHWND(g_pGlob->hWnd);
	}
#else
	// initialise multiplayer early
	#ifndef NOSTEAMORVIDEO
	SteamInit();
	#endif

	// Main Setup init
	bool bDXFailed=false;
	if ( SETUPConstructor() == true)
	{
		SETUPPassCoreData(g_pGlob, 0);
		if(m_pDX==NULL) bDXFailed=true;
	}
	else
		bDXFailed=true;

	// Initialise DisplayDLL
	OverrideHWND(g_pGlob->hWnd);

	// Activate COM
	CoInitialize(NULL);

#endif
#ifdef ENABLEIMGUI
#ifndef USEOLDIDE
	if (bWeAreEditor) {
		//PE: Setup the window here. pos size. Docking ?
		SetWindowSettings(5, 1, 1);
		SetForegroundWindow(g_pGlob->hWnd);

		SetWindowSize(pref.vStartResolution.x, pref.vStartResolution.y);

		float centerx = (GetDesktopWidth()*0.5) - (pref.vStartResolution.x*0.5);
		float centery = ((float)(GetDesktopHeight()*0.5) - (float)(pref.vStartResolution.y*0.5)) * 0.5f;
		if (centerx < 0)
			centerx = 0;
		if (centery < 0)
			centery = 0;
		SetWindowPosition(centerx, centery);
		ShowWindow();

		extern DWORD gWindowSizeAddY;
		extern DWORD gWindowSizeAddX;
		RECT clientrc;
		GetClientRect(g_pGlob->hWnd, &clientrc);
		gWindowSizeAddY = pref.vStartResolution.y - clientrc.bottom;
		gWindowSizeAddX = pref.vStartResolution.x - clientrc.right;

		if(pref.iMaximized > 0)
			MaximiseWindow();
		else
			RestoreWindow();

		//We need something like g_bWindowOverride = true;
		extern bool g_bWindowOverride;
		g_bWindowOverride = true;
	}
#endif
#endif

	// Create Display (dwAppDisplayModeUsing controls window handler)
	g_pGlob->dwAppDisplayModeUsing=dwDisplayType;
	CreateDisplay(dwDisplayType);

	// Can fail to create starter resolution
	if(*(DWORD*)g_ErrorHandler>0) return 1;

	// Assign Function Ptrs to Glob (for other DLLs to use)
	g_pGlob->CreateDeleteString = CreateDeleteString;
	g_pGlob->ProcessMessageFunction = ProcessMessagesOnly;
	g_pGlob->PrintStringFunction = NULL;
	//g_pGlob->UpdateFilenameFromVirtualTable = UpdateFilenameFromVirtualTable;
	g_pGlob->Decrypt = Decrypt;
	g_pGlob->Encrypt = Encrypt;
	g_pGlob->ChangeMouseFunction = ChangeMouse;

	// Load External DLL Displayer
	ConstructPostDisplayItems(hInstance);

	// Prepare Other DLLs
	ConstructPostDLLItems(hInstance, false);// , bNeededToCreateExtraWindowForWMRWindow);

	// Visible Window
	if(bWindowIsDisplayable)
	{
		// Clear Screen
		InvalidateRect(g_pGlob->hWnd, NULL, TRUE);
		UpdateWindow(g_pGlob->hWnd);

		// Reveal Window
		ShowWindow(g_pGlob->hWnd, SW_SHOW);
	}

	// Set Any After Display Properties (ink, font, etc)
	SetDefaultDisplayProperties();

	// Process any messages prior to program start (also for begin scene call)
	InternalProcessMessages();

	// complete
	return 0;
}

DARKSDK DWORD InitDisplay(DWORD dwDisplayType, DWORD dwWidth, DWORD dwHeight, DWORD dwDepth, HINSTANCE hInstance, LPSTR pApplicationName)
{
	return InitDisplayEx(dwDisplayType, dwWidth, dwHeight, dwDepth, hInstance, pApplicationName, NULL, 0, 0);
}

DARKSDK void SetRenderOrderList(void)
{
	CreateRenderOrderList();
}

DARKSDK void ConstructDLLs(void)
{
	// Prepare Other DLLs
	ConstructPostDisplayItems(g_pGlob->hInstance);
	ConstructPostDLLItems(g_pGlob->hInstance,false);
}

DARKSDK int GetSecurityCode(void)
{	
	// gewnerate once
	srand((int)timeGetTime());
	if ( g_iSecurityCode!=-1 )
	{
		int iSecurityCode = rand()%1000000;
		if ( g_iSecurityCode==0 ) g_iSecurityCode = iSecurityCode;
	}
	return g_iSecurityCode;
}

DARKSDK void WipeSecurityCode(void)
{
	// clear forever
	g_iSecurityCode=-1;
}

DARKSDK DWORD GetGlobPtr(void)
{
	return (DWORD)g_pGlob;
}

DARKSDK void FreeChecklistStrings(void)
{
	// Free checklist strings
	for(DWORD c=0; c<g_pGlob->dwChecklistArraySize; c++)
		if(g_pGlob->checklist[c].string)
			SAFE_DELETE(g_pGlob->checklist[c].string);

	// Free main block
	if(g_pGlob->checklist)
	{
		g_pGlob->CreateDeleteString((char**)&g_pGlob->checklist, 0);
		g_pGlob->checklist=NULL;
	}
}

DARKSDK DWORD CloseDisplay(void)
{
	// Free checklist strings
	FreeChecklistStrings();

	// Restore Display to Windowed Mode
	DeleteDisplay();

	// Free default input resources
	if(g_pGlob->pWindowsTextEntry)
	{
		delete[] g_pGlob->pWindowsTextEntry;
		g_pGlob->pWindowsTextEntry=NULL;
	}

	// Free safe rects arrays
	SAFE_DELETE ( g_pGlob->pSafeRects );

	// Close Window
	if(g_pGlob->hWnd)
	{
		ShowWindow ( g_pGlob->hWnd, SW_HIDE );
		CloseWindow(g_pGlob->hWnd);
		g_pGlob->hWnd=NULL;
	}

	// Free Cursors and Icons
	if(g_hUseIcon) DestroyIcon(g_hUseIcon);
	if(g_hUseArrow) DestroyCursor(g_hUseArrow);
	if(g_hUseHourglass) DestroyCursor(g_hUseHourglass);

	// Free COM
	CoUninitialize();

	// Complete
	return 0;
}

// ABSOLUTE BASIC COMMANDS (PRINT and INPUT)

DARKSDK void Cls(void)
{
}

DARKSDK LONGLONG PerformanceTimer ( void )
{
	LARGE_INTEGER large;
	if (!QueryPerformanceCounter ( &large ))
	{
		large.QuadPart = 0;
	};
	return large.QuadPart;
}

DARKSDK LONGLONG PerformanceFrequency ( void )
{
	LARGE_INTEGER large;
	if (! QueryPerformanceFrequency( &large ))
	{
		large.QuadPart = 0;
	}
	return large.QuadPart;
}

DARKSDK float timeGetSecond(void)
{
	LARGE_INTEGER large;
	if (!QueryPerformanceCounter ( &large ))
	{
		large.QuadPart = 0;
	};
	if ( g_lFirstPerfTime == 0 ) g_lFirstPerfTime = large.QuadPart;
	LONGLONG lTimer = large.QuadPart;
	if (! QueryPerformanceFrequency( &large ))
	{
		large.QuadPart = 0;
	}
	float fTime = (float)(lTimer-g_lFirstPerfTime) / large.QuadPart;
	return fTime;
}

DARKSDK LONGLONG InputR(void)
{
	LONGLONG lValue;
	InputInteger(&lValue);
	return lValue;
}
DARKSDK double InputO(void)
{
	double dValue;
	InputFloat(&dValue);
	return dValue;
}
DARKSDK char* InputS(char* pDestStr)
{
	//if(pDestStr) delete (LPSTR)pDestStr;
	if(pDestStr) g_pGlob->CreateDeleteString((char**)&pDestStr, 0);

	char* pString=NULL;
	InputString(&pString);
	return pString;
}

// MEMORY MANAGEMENT FUNCTIONS

DARKSDK DWORD CreateVariableSpace(DWORD VariableSpaceSize)
{
	// Create Variable Space
	g_pVarSpace = (LPSTR)GlobalAlloc(GMEM_FIXED, VariableSpaceSize);
	g_pGlob->g_pVariableSpace = (LPVOID)g_pVarSpace;
	return (DWORD)g_pVarSpace;
}

DARKSDK DWORD CreateDataSpace(DWORD DataSpaceSize)
{
	// Create Data Space
	g_pDataSpace = (LPSTR)GlobalAlloc(GMEM_FIXED, DataSpaceSize);
	return (DWORD)g_pDataSpace;
}

DARKSDK void DeleteVariableSpace(void)
{
	// Delete Variable Space Itself
	SAFE_FREE(g_pVarSpace);
}

DARKSDK void DeleteDataSpace(void)
{
	// Delete Data Space Itself
	SAFE_FREE(g_pDataSpace);
}

DARKSDK void DeleteSingleVariableAllocation(DWORD* dwVariableSpaceAddress)
{
	// Delete Actual Allocation within Variable Space (no need to clear)
	if(dwVariableSpaceAddress)
	{
		delete dwVariableSpaceAddress;
	}
}

DARKSDK DWORD CreateArray(DWORD dwSizeOfArray, DWORD dwSizeOfOneDataItem, DWORD dwTypeValueOfOneDataItem)
{
	// Calculate Total Size of Array
	DWORD dwHeaderSizeInBytes = HEADERSIZEINBYTES;
	DWORD dwDimSizeBytes = 40;
	DWORD dwRefSizeInBytes = dwSizeOfArray * 4;
	DWORD dwFlagSizeInBytes = dwSizeOfArray * 1;
	DWORD dwDataSizeInBytes = dwSizeOfArray * dwSizeOfOneDataItem;

	// Total Size
	DWORD dwTotalSize = dwHeaderSizeInBytes + dwRefSizeInBytes + dwFlagSizeInBytes + dwDataSizeInBytes;

	// Error Trap for debug to discover larger chunk allocations mid-app activity (fragmentation danger)
	if ( dwTotalSize > 1024*1000*4 )
	{
		// can put breakpoint here when checking for large allocations mid-flow
		int iMB = dwTotalSize / 1024 / 1000;
		int stopwhen4megallocated = 42;
	}

	// Create Array Memory
	LPSTR pArrayPtr = new char[dwTotalSize];
	memset(pArrayPtr, 0, sizeof(pArrayPtr));

	// Derive Pointers into Array
	DWORD* pHeader	= (DWORD*)(pArrayPtr);
	DWORD* pRef		= (DWORD*)(pArrayPtr+dwHeaderSizeInBytes);
	LPSTR  pFlag	= (LPSTR )(pArrayPtr+dwHeaderSizeInBytes+dwRefSizeInBytes);
	LPSTR  pData	= (LPSTR )(pArrayPtr+dwHeaderSizeInBytes+dwRefSizeInBytes+dwFlagSizeInBytes);

	// Create Header
	for(DWORD d=0; d<=9; d++) pHeader[0]=0;
	pHeader[10]=dwSizeOfArray;
	pHeader[11]=dwSizeOfOneDataItem;
	pHeader[12]=dwTypeValueOfOneDataItem;
	pHeader[13]=0;

	// Create Ref Table
	LPSTR pDataPointer = pData;
	for(DWORD r=0; r<dwSizeOfArray; r++)
	{
		pRef[r] = (DWORD)pDataPointer;
		pDataPointer+=dwSizeOfOneDataItem;
	}

	// Create DataBlockFlag Table (all flags to 1)
	memset(pFlag, 1, dwSizeOfArray);

	// Clear DataBlock Memory
	DWORD dwTotalDataSize = dwSizeOfArray * dwSizeOfOneDataItem;
	memset(pData, 0, dwTotalDataSize);

	// Advance ArrayPtr to First Byte in RefTable
	pArrayPtr+=dwHeaderSizeInBytes;

	// Return ArrayPtr
	return (DWORD)pArrayPtr;
}

DARKSDK void FreeStringsFromArray(DWORD dwArrayPtr)
{
	// Get Array Information
	if ( dwArrayPtr )
	{
		DWORD dwTypeValueOfOneDataItem = *((DWORD*)dwArrayPtr-2);
		if ( dwTypeValueOfOneDataItem == 2 ) 
		{
			// only free strings if array holds string items
			DWORD dwSizeOfTable = *((DWORD*)dwArrayPtr-4);
			DWORD dwDataItemSize = *((DWORD*)dwArrayPtr-3);
			DWORD dwRefSizeInBytes = dwSizeOfTable * 4;
			DWORD dwFlagSizeInBytes = dwSizeOfTable * 1;
			LPSTR* pData = (LPSTR*)(((LPSTR)dwArrayPtr)+dwRefSizeInBytes+dwFlagSizeInBytes);
			for ( DWORD dwDataOffset=0; dwDataOffset<dwSizeOfTable; dwDataOffset++)
			{
				if ( pData [ dwDataOffset ] )
				{
					delete[] pData [ dwDataOffset ];
				}
			}
		}
		// Clear strings from UDT's
		else if (dwTypeValueOfOneDataItem >= 9)
		{
			// Grab a copy of the arrays format string
			LPSTR UdtFormat = GetTypePatternCore( NULL, dwTypeValueOfOneDataItem );

			// Search the format string to see if the UDT contains any strings
			bool ContainsString = false;
			for ( LPSTR CurrentItem = UdtFormat; *CurrentItem; ++CurrentItem )
			{
				if (*CurrentItem == 'S')
				{
					ContainsString = true;
					break;
				}
			}

			// If it does, loop through every UDT and release those strings
			if (ContainsString)
			{
				DWORD* ArrayPtr = (DWORD*)dwArrayPtr;
				DWORD  ArraySize = ArrayPtr[-4];

				for ( DWORD Position = 0; Position < ArraySize; ++Position )
				{
					DWORD ItemOffset = 0;
					for ( LPSTR CurrentItem = UdtFormat; *CurrentItem; ++CurrentItem )
					{
						if (*CurrentItem == 'S')
						{
							DWORD P = ArrayPtr[ Position ] + ItemOffset;
							delete[] *(LPSTR*)P;
							ItemOffset += 4;            // Strings are 4 bytes
						}
						else if (*CurrentItem == 'O' || *CurrentItem == 'R')
						{
							ItemOffset += 8;            // Double float/integer are 8 bytes
						}
						else
						{
							ItemOffset += 4;            // Everything else is 4 bytes
						}
					}
				}
			}

			// Release the copy of the arrays format string
			delete[] UdtFormat;
		}
	}
}

DARKSDK void DeleteArray(DWORD dwArrayPtr)
{
	// If Array exists
	if(dwArrayPtr)
	{
		// Array Ptr Skips Header
		dwArrayPtr-=HEADERSIZEINBYTES;

		// Delete Array Memory
		delete[] (DWORD*)dwArrayPtr;
	}
}

DARKSDK DWORD ExpandArray(DWORD dwOldArrayPtr, DWORD dwAddElements)
{
	// Get Old ArrayPtr
	LPSTR pOldArrayPtr = ((LPSTR)dwOldArrayPtr)-HEADERSIZEINBYTES;

	// Old Array Pointers and Data
	DWORD* pHeader	= (DWORD*)(pOldArrayPtr);
	DWORD dwHeaderSizeInBytes = HEADERSIZEINBYTES;

	// Extract header info
	DWORD dwOldSizeOfArray = pHeader[10];
	DWORD dwOldSizeOfOneDataItem = pHeader[11];
	DWORD dwOldTypeValueOfOneDataItem = pHeader[12];
	DWORD dwOldInternalIndex = pHeader[13];

	DWORD dwOldRefSizeInBytes = dwOldSizeOfArray * 4;
	DWORD dwOldFlagSizeInBytes = dwOldSizeOfArray * 1;
	DWORD dwOldDataSizeInBytes = dwOldSizeOfArray * dwOldSizeOfOneDataItem;
	DWORD* pOldRef = (DWORD*)(pOldArrayPtr+dwHeaderSizeInBytes);
	LPSTR pOldFlag = (LPSTR)(pOldArrayPtr+dwHeaderSizeInBytes+dwOldRefSizeInBytes);
	LPSTR pOldData = (LPSTR)(pOldArrayPtr+dwHeaderSizeInBytes+dwOldRefSizeInBytes+dwOldFlagSizeInBytes);

	// Create New Size of Array
	DWORD dwSizeOfArray = dwOldSizeOfArray + dwAddElements;
	LPSTR pArrayPtr = (LPSTR)CreateArray(dwSizeOfArray, dwOldSizeOfOneDataItem, dwOldTypeValueOfOneDataItem);

	// Return ptr to beginning of memory
	pArrayPtr = pArrayPtr - HEADERSIZEINBYTES;

	// Copy dimension-size block over (10xDWORD values)
	memcpy(pArrayPtr, pOldArrayPtr, 40);

	// Calculate Sizes of New Array
	DWORD dwRefSizeInBytes = dwSizeOfArray * 4;
	DWORD dwFlagSizeInBytes = dwSizeOfArray * 1;
	DWORD dwDataSizeInBytes = dwSizeOfArray * dwOldSizeOfOneDataItem;

	// Derive Pointers into New Array
	DWORD* pNewRef		= (DWORD*)(pArrayPtr+dwHeaderSizeInBytes);
	LPSTR  pNewFlag		= (LPSTR )(pArrayPtr+dwHeaderSizeInBytes+dwRefSizeInBytes);
	LPSTR  pNewData		= (LPSTR )(pArrayPtr+dwHeaderSizeInBytes+dwRefSizeInBytes+dwFlagSizeInBytes);

	// Clear new data and copy old data to it
	memset(pNewData, 0, dwDataSizeInBytes);
	memcpy(pNewData, pOldData, dwOldDataSizeInBytes);

	// Update New Array Refs from Old Array Refs
	for(DWORD i=0; i<dwOldSizeOfArray; i++)
	{
		DWORD dwOffset = (DWORD)(pOldRef[i]) - (DWORD)pOldData;
		pNewRef[i] = (DWORD)(pNewData + dwOffset);
	}

	// Copy flag states from old to new
	memcpy(pNewFlag, pOldFlag, dwOldFlagSizeInBytes);

	// Create flags for new part of array
	memset(pNewFlag+dwOldFlagSizeInBytes, 1, dwFlagSizeInBytes-dwOldFlagSizeInBytes);

	// Destroy old array
	DeleteArray(dwOldArrayPtr);

	// Advance ArrayPtr to First Byte in RefTable
	pArrayPtr+=dwHeaderSizeInBytes;

	// Return ArrayPtr
	return (DWORD)pArrayPtr;
}

DARKSDK void ClearDataBlock(DWORD dwArrayPtr, DWORD dwIndex, DWORD dwQuantity)
{
	DWORD dwSizeOfTable = *((DWORD*)dwArrayPtr-4);
	DWORD dwDataItemSize = *((DWORD*)dwArrayPtr-3);
	DWORD dwRefSizeInBytes = dwSizeOfTable * 4;
	DWORD dwFlagSizeInBytes = dwSizeOfTable * 1;
	LPSTR pData = (LPSTR)(((LPSTR)dwArrayPtr)+dwRefSizeInBytes+dwFlagSizeInBytes);
	DWORD dwDataOffset = dwIndex * dwDataItemSize;
	memset(pData+dwDataOffset, 0, dwQuantity * dwDataItemSize);
}

// ARRAY COMMANDS

DARKSDK DWORD DimCore(DWORD dwOldArrayPtr, DWORD dwTypeAndSizeOfElement, DWORD dwD1, DWORD dwD2, DWORD dwD3, DWORD dwD4, DWORD dwD5, DWORD dwD6, DWORD dwD7, DWORD dwD8, DWORD dwD9)
{
	// Increment all DBPro dimensions (+1 based)
	dwD1+=1;
	if(dwD2>0) dwD2+=1;
	if(dwD3>0) dwD3+=1;
	if(dwD4>0) dwD4+=1;
	if(dwD5>0) dwD5+=1;
	if(dwD6>0) dwD6+=1;
	if(dwD7>0) dwD7+=1;
	if(dwD8>0) dwD8+=1;
	if(dwD9>0) dwD9+=1;

	// Work out array size (can be no bigger than DWORD)
	__int64 iiSize = dwD1;
	if(dwD2>0) iiSize *= dwD2;
	if(dwD3>0) iiSize *= dwD3;
	if(dwD4>0) iiSize *= dwD4;
	if(dwD5>0) iiSize *= dwD5;
	if(dwD6>0) iiSize *= dwD6;
	if(dwD7>0) iiSize *= dwD7;
	if(dwD8>0) iiSize *= dwD8;
	if(dwD9>0) iiSize *= dwD9;
	DWORD dwSizeOfArray = (DWORD)iiSize;
	if(dwSizeOfArray!=iiSize)
		return NULL;

	// new idea for dwTypeAndSizeOfElement
	// where the first 0-4095 specify a type index (>9 = user types)
	// and then a multiple of 4096 controls the size of the datatype
	// Type Value and Size as one DWORD value
	DWORD dwSizeOfOneDataItem = dwTypeAndSizeOfElement;
	dwSizeOfOneDataItem = dwSizeOfOneDataItem/4096;
	DWORD dwTypeValueOfOneDataItem = dwTypeAndSizeOfElement-(dwSizeOfOneDataItem*4096);

	// Create New Array
	DWORD dwArrayPtr =  CreateArray(dwSizeOfArray, dwSizeOfOneDataItem, dwTypeValueOfOneDataItem);

	// Fill array with dimension size data (D1-D9)
	DWORD* pHeader = (DWORD*)(((LPSTR)dwArrayPtr)-HEADERSIZEINBYTES);
	DWORD dwDimOverallSize=dwD1;
	for(DWORD h=0; h<=8; h++)
	{
		pHeader[h]=dwDimOverallSize;
		if(h==0) dwDimOverallSize=dwDimOverallSize*dwD2;
		if(h==1) dwDimOverallSize=dwDimOverallSize*dwD3;
		if(h==2) dwDimOverallSize=dwDimOverallSize*dwD4;
		if(h==3) dwDimOverallSize=dwDimOverallSize*dwD5;
		if(h==4) dwDimOverallSize=dwDimOverallSize*dwD6;
		if(h==5) dwDimOverallSize=dwDimOverallSize*dwD7;
		if(h==6) dwDimOverallSize=dwDimOverallSize*dwD8;
		if(h==7) dwDimOverallSize=dwDimOverallSize*dwD9;
	}

	// Return ArrayPtr
	return dwArrayPtr;
}

// ADDED DUE TO POPULAR DEMAND ON 040304

DARKSDK DWORD ReDimCore(DWORD dwOldArrayPtr, DWORD dwNewTypeAndSizeOfElement, DWORD dwOD1, DWORD dwOD2, DWORD dwOD3, DWORD dwOD4, DWORD dwOD5, DWORD dwOD6, DWORD dwOD7, DWORD dwOD8, DWORD dwOD9)
{
	// Increment all DBPro dimensions (+1 based) (as done is DimCore)
	DWORD dwD1=dwOD1, dwD2=dwOD2, dwD3=dwOD3, dwD4=dwOD4, dwD5=dwOD5, dwD6=dwOD6, dwD7=dwOD7, dwD8=dwOD8, dwD9=dwOD9;
	dwD1+=1;
	if(dwD2>0) dwD2+=1;
	if(dwD3>0) dwD3+=1;
	if(dwD4>0) dwD4+=1;
	if(dwD5>0) dwD5+=1;
	if(dwD6>0) dwD6+=1;
	if(dwD7>0) dwD7+=1;
	if(dwD8>0) dwD8+=1;
	if(dwD9>0) dwD9+=1;

	// Old Header Info
	DWORD dwHeaderSizeInBytes = HEADERSIZEINBYTES;
	DWORD* pOldHeader = (DWORD*)(((LPSTR)dwOldArrayPtr)-HEADERSIZEINBYTES);
	DWORD dwSizeOfOneDataItem = pOldHeader[11];

	// lee - 130206 - can detect if LOCAL DIM ARRAY attempte a REDIM with corrupt data
	// prevent bug by ignoring a REDIM and starting with a brand new Array
	if ( dwSizeOfOneDataItem > 1024000 ) // a data item over 1MB is a little extreme
		return NULL;

	// continue as the REDIM appears to be valid..
	DWORD dwTypeValueOfOneDataItem = pOldHeader[12];

	// Work out size and type of new array early
	DWORD dwNewSizeOfOneDataItem = dwNewTypeAndSizeOfElement;
	dwNewSizeOfOneDataItem = dwNewSizeOfOneDataItem/4096;
	DWORD dwNewTypeValueOfOneDataItem = dwNewTypeAndSizeOfElement-(dwNewSizeOfOneDataItem*4096);

	// Leave if core datachunk size (type) different
	if ( dwSizeOfOneDataItem!=dwNewSizeOfOneDataItem
	||   dwTypeValueOfOneDataItem!=dwNewTypeValueOfOneDataItem )
		return dwOldArrayPtr;

	// Create a New Array of new size
	DWORD dwNewArrayPtr = DimCore ( dwOldArrayPtr, dwNewTypeAndSizeOfElement, dwOD1, dwOD2, dwOD3, dwOD4, dwOD5, dwOD6, dwOD7, dwOD8, dwOD9 );
	DWORD* pNewHeader = (DWORD*)(((LPSTR)dwNewArrayPtr)-HEADERSIZEINBYTES);

	// Old Array Offsets
	DWORD dwOld[9];	for ( int i=0; i<9; i++ ) dwOld[i] = pOldHeader[i];

	// New Array Offsets
	DWORD dwNew[9];	for ( int i=0; i<9; i++ ) dwNew[i] = pNewHeader[i];

	// Find old and new ptrs to reference tables of the arrays
	DWORD* pOldRef = (DWORD*)dwOldArrayPtr;
	DWORD* pNewRef = (DWORD*)dwNewArrayPtr;

	// Work out old dim values from data chunk sizes
	DWORD dwOldDims [ 9 ];
	for(DWORD h=0; h<=8; h++)
	{
		DWORD dwDataChunkSize;
		if(h==0) dwDataChunkSize=1;
		if(h==1) dwDataChunkSize=dwOld[0];
		if(h==2) dwDataChunkSize=dwOld[0]*dwOld[1];
		if(h==3) dwDataChunkSize=dwOld[0]*dwOld[1];
		if(h==4) dwDataChunkSize=dwOld[0]*dwOld[1]*dwOld[2];
		if(h==5) dwDataChunkSize=dwOld[0]*dwOld[1]*dwOld[2]*dwOld[3];
		if(h==6) dwDataChunkSize=dwOld[0]*dwOld[1]*dwOld[2]*dwOld[3]*dwOld[4];
		if(h==7) dwDataChunkSize=dwOld[0]*dwOld[1]*dwOld[2]*dwOld[3]*dwOld[4]*dwOld[5];
		if(h==8) dwDataChunkSize=dwOld[0]*dwOld[1]*dwOld[2]*dwOld[3]*dwOld[4]*dwOld[5]*dwOld[6];
		DWORD dwActualDimValue=0;
		if ( dwDataChunkSize>0 ) dwActualDimValue=dwOld[h]/dwDataChunkSize;
		dwOldDims[h]=dwActualDimValue;
	}

	// Trim if new array is smaller than old array (odd redim but possible)
	if ( dwOldDims[0] > dwD1 ) dwOldDims[0]=dwD1;
	if ( dwOldDims[1] > dwD2 ) dwOldDims[1]=dwD2;
	if ( dwOldDims[2] > dwD3 ) dwOldDims[2]=dwD3;
	if ( dwOldDims[3] > dwD4 ) dwOldDims[3]=dwD4;
	if ( dwOldDims[4] > dwD5 ) dwOldDims[4]=dwD5;
	if ( dwOldDims[5] > dwD6 ) dwOldDims[5]=dwD6;
	if ( dwOldDims[6] > dwD7 ) dwOldDims[6]=dwD7;
	if ( dwOldDims[7] > dwD8 ) dwOldDims[7]=dwD8;
	if ( dwOldDims[8] > dwD9 ) dwOldDims[8]=dwD9;

	// make sure can get through all fornext conditions at least once (to get to code)
	for(int h=0; h<=8; h++)
	{
		DWORD dwActualDimValue=dwOldDims[h];
		if ( dwActualDimValue==0 ) dwActualDimValue=1;
		dwOldDims[h]=dwActualDimValue;
	}

	// u55 - 080704 - do not copy if old or new array is EMPTY
	if ( pOldHeader[10]>0 && pNewHeader[10]>0 )
	{
		// Copy Old array data to new array
		for ( DWORD dwI1=0; dwI1<dwOldDims[0]; dwI1++ )
		for ( DWORD dwI2=0; dwI2<dwOldDims[1]; dwI2++ )
		for ( DWORD dwI3=0; dwI3<dwOldDims[2]; dwI3++ )
		for ( DWORD dwI4=0; dwI4<dwOldDims[3]; dwI4++ )
		for ( DWORD dwI5=0; dwI5<dwOldDims[4]; dwI5++ )
		for ( DWORD dwI6=0; dwI6<dwOldDims[5]; dwI6++ )
		for ( DWORD dwI7=0; dwI7<dwOldDims[6]; dwI7++ )
		for ( DWORD dwI8=0; dwI8<dwOldDims[7]; dwI8++ )
		for ( DWORD dwI9=0; dwI9<dwOldDims[8]; dwI9++ )
		{
			// copy old block of data to new array 
			DWORD dwOldIndex = (dwI1)+(dwI2*dwOld[0])+(dwI3*dwOld[1])+(dwI4*dwOld[2])+(dwI5*dwOld[3])+(dwI6*dwOld[4])+(dwI7*dwOld[5])+(dwI8*dwOld[6])+(dwI9*dwOld[7]);
			DWORD dwNewIndex = (dwI1)+(dwI2*dwNew[0])+(dwI3*dwNew[1])+(dwI4*dwNew[2])+(dwI5*dwNew[3])+(dwI6*dwNew[4])+(dwI7*dwNew[5])+(dwI8*dwNew[6])+(dwI9*dwNew[7]);
			LPSTR pOldPtr = (LPSTR)pOldRef[dwOldIndex];
			LPSTR pNewPtr = (LPSTR)pNewRef[dwNewIndex];
			memcpy ( pNewPtr, pOldPtr, dwSizeOfOneDataItem );
		}
	}

	// Free Old Array (if any)
	DeleteArray ( dwOldArrayPtr );

	// return new sized arrau
	return dwNewArrayPtr;
}

DARKSDK DWORD DimDDD(DWORD dwOldArrayPtr, DWORD dwTypeAndSizeOfElement, DWORD dwD1, DWORD dwD2, DWORD dwD3, DWORD dwD4, DWORD dwD5, DWORD dwD6, DWORD dwD7, DWORD dwD8, DWORD dwD9)
{
	try
	{
		// leechange - 050304 - now REDIMs if array already exists
		if ( dwOldArrayPtr )
		{
			// Change Size Of Array (and retain contents)
			DWORD dwNewArrPtr = ReDimCore ( dwOldArrayPtr, dwTypeAndSizeOfElement, dwD1, dwD2, dwD3, dwD4, dwD5, dwD6, dwD7, dwD8, dwD9 );

			// If corruption detected, can resort to a new array as follows..
			if ( dwNewArrPtr!=NULL ) return dwNewArrPtr;
		}

		// Create a New Array
		return DimCore ( dwOldArrayPtr, dwTypeAndSizeOfElement, dwD1, dwD2, dwD3, dwD4, dwD5, dwD6, dwD7, dwD8, dwD9 );
	}
	catch (...)
	{
		RunTimeError(RUNTIMEERROR_NOTENOUGHMEMORY);
		return dwOldArrayPtr;
	}
}

DARKSDK DWORD UnDimDD(DWORD dwAllocation)
{
	// leefix - 070308 - U6.7 - will free strings if string array (fixes string leak)
	FreeStringsFromArray(dwAllocation);

	DeleteArray(dwAllocation);
	return NULL;
}

// ADVANCED UNIFIED ARRAY HANLDING

DARKSDK void ArrayIndexToBottom(DWORD dwArrayPtr)
{
	// set index to last item in array
	if(dwArrayPtr) *((DWORD*)dwArrayPtr-1) = *((DWORD*)dwArrayPtr-4)-1;
}
DARKSDK void ArrayIndexToTop(DWORD dwArrayPtr)
{
	// set index to first item in array
	if(dwArrayPtr) *((DWORD*)dwArrayPtr-1) = 0;
}
DARKSDK void NextArrayIndex(DWORD dwArrayPtr)
{
	// inc array index
	if(dwArrayPtr)
	{
		*((DWORD*)dwArrayPtr-1) = *((DWORD*)dwArrayPtr-1) + 1;
		if(*((DWORD*)dwArrayPtr-1) > *((DWORD*)dwArrayPtr-4))
		{
			// Last index reachable just just outside range >N
			*((DWORD*)dwArrayPtr-1) = *((DWORD*)dwArrayPtr-4);
		}
	}
}
DARKSDK void PreviousArrayIndex(DWORD dwArrayPtr)
{
	// dec array index
	if(dwArrayPtr)
	{
		if(*((DWORD*)dwArrayPtr-1)>0)
		{
			*((DWORD*)dwArrayPtr-1)=(*((DWORD*)dwArrayPtr-1))-1;
		}
		else
		{
			// First index reachable just just outside range <0
			*((DWORD*)dwArrayPtr-1)=(DWORD)-1;
		}
	}
}
DARKSDK DWORD ArrayIndexValid(DWORD dwArrayPtr)
{
	// check if index is valid (pointing to valid item)
	if(dwArrayPtr)
	{
		if(*((DWORD*)dwArrayPtr-1)<*((DWORD*)dwArrayPtr-4))
			return 1;
		else
			return 0;
	}
	else
		return 0;
}
DARKSDK DWORD ArrayCount(DWORD dwArrayPtr)
{
	// return array size
	if(dwArrayPtr) 
		return (*((DWORD*)dwArrayPtr-4))-1;
	else
		return -1;
}
DARKSDK DWORD ArrayInsertAtBottom(DWORD dwArrayPtr)
{
	try
	{
		// If no array, leave now
		if(dwArrayPtr==NULL) return dwArrayPtr;

		// lee - 140306 - u60b3 - Do not allow multi-dimensional arrays
		if ( IsArraySingleDim ( dwArrayPtr )==false )
		{
			RunTimeError(RUNTIMEERROR_ARRAYMUSTBESINGLEDIM);
			return dwArrayPtr;
		}

		// Adjust Size Of Entire Array
		DWORD dwAllocation = ExpandArray(dwArrayPtr, 1);

		// Determine index
		int iIndex = (*((DWORD*)dwAllocation-4)) - 1;
		if(iIndex<0) iIndex=0;

		// Update array index to last in list
		*((DWORD*)dwAllocation-1) = iIndex;

		// Overwrites current array ptr
		return dwAllocation;
	}
	catch (...)
	{
		RunTimeError(RUNTIMEERROR_NOTENOUGHMEMORY);
		return dwArrayPtr;
	}
}
DARKSDK DWORD ArrayInsertAtBottom(DWORD dwArrayPtr, int iQuantity)
{
	try
	{
		// If no array, leave now
		if(dwArrayPtr==NULL) return dwArrayPtr;

		// lee - 140306 - u60b3 - Do not allow multi-dimensional arrays
		if ( IsArraySingleDim ( dwArrayPtr )==false ) { RunTimeError(RUNTIMEERROR_ARRAYMUSTBESINGLEDIM); return dwArrayPtr; }

		// Autohandler
		if(iQuantity<1) iQuantity=1;

		// Adjust Size Of Entire Array
		DWORD dwAllocation = ExpandArray(dwArrayPtr, iQuantity);

		// Determine index
		int iIndex = (*((DWORD*)dwAllocation-4)) - iQuantity;
		if(iIndex<0) iIndex=0;

		// Update array index to fisrt new item at end of list
		*((DWORD*)dwAllocation-1) = iIndex;

		// Overwrites current array ptr
		return dwAllocation;
	}
	catch (...)
	{
		RunTimeError(RUNTIMEERROR_NOTENOUGHMEMORY);
		return dwArrayPtr;
	}
}

DARKSDK DWORD ArrayInsertAtTop(DWORD dwArrayPtr)
{
	try
	{
		// If no array, leave now
		if(dwArrayPtr==NULL) return dwArrayPtr;

		// lee - 140306 - u60b3 - Do not allow multi-dimensional arrays
		if ( IsArraySingleDim ( dwArrayPtr )==false ) { RunTimeError(RUNTIMEERROR_ARRAYMUSTBESINGLEDIM); return dwArrayPtr; }

		// Adjust Size Of Entire Array
		DWORD dwAllocation = ExpandArray(dwArrayPtr, 1);

		// Store Ref located at end of list
		DWORD dwSizeOfTable = *((DWORD*)dwAllocation-4);
		DWORD dwIndex = dwSizeOfTable - 1;
		DWORD* pRef = (DWORD*)dwAllocation;
		DWORD dwRefItem = pRef[dwIndex];

		// Shuffle ref table to make space at top
		if(dwSizeOfTable>0) memcpy(pRef+1, pRef, (dwSizeOfTable-1)*4);

		// Copy refitem to top position
		pRef[0] = dwRefItem;

		// Update array index to new item
		*((DWORD*)dwAllocation-1) = 0;

		// Overwrites current array ptr
		return dwAllocation;
	}
	catch (...)
	{
		RunTimeError(RUNTIMEERROR_NOTENOUGHMEMORY);
		return dwArrayPtr;
	}
}

DARKSDK DWORD ArrayInsertAtTop(DWORD dwArrayPtr, int iQuantity)
{
	try
	{
		// If no array, leave now
		if(dwArrayPtr==NULL) return dwArrayPtr;

		// lee - 140306 - u60b3 - Do not allow multi-dimensional arrays
		if ( IsArraySingleDim ( dwArrayPtr )==false ) { RunTimeError(RUNTIMEERROR_ARRAYMUSTBESINGLEDIM); return dwArrayPtr; }

		// Autohandler
		if(iQuantity<1) iQuantity=1;

		// Adjust Size Of Entire Array
		DWORD dwAllocation = ExpandArray(dwArrayPtr, iQuantity);

		// Store RefItems(iQuantity) located at end of list
		DWORD* pStoreRefs = (DWORD*)new DWORD[iQuantity];
		DWORD dwSizeOfTable = *((DWORD*)dwAllocation-4);
		DWORD dwIndexOfFirstRef = dwSizeOfTable-iQuantity;
		DWORD* pRef = (DWORD*)dwAllocation;
		memcpy(pStoreRefs, (DWORD*)pRef + dwIndexOfFirstRef, iQuantity*4);

		// Shuffle ref table to make space at top
		DWORD dwAmountToShuffle = 0;
		if(dwSizeOfTable>(DWORD)iQuantity) dwAmountToShuffle=(dwSizeOfTable-iQuantity)*4;
		if(dwAmountToShuffle>0) memcpy(pRef+iQuantity, pRef, dwAmountToShuffle);

		// Copy refitem to top position
		memcpy(pRef, pStoreRefs, iQuantity*4);

		// Update array index to new item
		*((DWORD*)dwAllocation-1) = 0;

		// Overwrites current array ptr
		return dwAllocation;
	}
	catch (...)
	{
		RunTimeError(RUNTIMEERROR_NOTENOUGHMEMORY);
		return dwArrayPtr;
	}
}
DARKSDK DWORD ArrayInsertAtElement(DWORD dwArrayPtr, int iIndex)
{
	try
	{
		// If no array, leave now
		if(dwArrayPtr==NULL) return dwArrayPtr;

		// lee - 140306 - u60b3 - Do not allow multi-dimensional arrays
		if ( IsArraySingleDim ( dwArrayPtr )==false ) { RunTimeError(RUNTIMEERROR_ARRAYMUSTBESINGLEDIM); return dwArrayPtr; }

		DWORD dwSizeOfTable = *((DWORD*)dwArrayPtr-4);
		if(iIndex<0 || iIndex>=(int)dwSizeOfTable)
		{
			RunTimeError(RUNTIMEERROR_ARRAYINDEXINVALID);
			return dwArrayPtr;
		}
		
		// Size of insert
		int iQuantity=1;

		// Adjust Size Of Entire Array
		DWORD dwAllocation = ExpandArray(dwArrayPtr, iQuantity);

		// Store RefItems(iQuantity) located at end of list
		DWORD* pStoreRefs = (DWORD*)new DWORD[iQuantity];
		DWORD dwIndexOfFirstRef = dwSizeOfTable-(iQuantity-1);  //leefix-230603-corrected ptr to new item-ref in expanded array
		DWORD* pRef = (DWORD*)dwAllocation;
		memcpy(pStoreRefs, (DWORD*)pRef + dwIndexOfFirstRef, iQuantity*4);

		// Shuffle iIndex to End onwards
		DWORD dwSizeOfLaterChunk = 0;
		if(dwSizeOfTable>(DWORD)iIndex) dwSizeOfLaterChunk = dwSizeOfTable-iIndex;
		if(dwSizeOfLaterChunk>0) memcpy(pRef+iIndex+iQuantity, pRef+iIndex, dwSizeOfLaterChunk*4);

		// Copy RefItems into space created inside table
		memcpy(pRef+iIndex, pStoreRefs, iQuantity*4);
		delete[] pStoreRefs;    // Remove memory leak

		// Update array index to new item
		*((DWORD*)dwAllocation-1) = iIndex;

		// Overwrites current array ptr
		return dwAllocation;
	}
	catch (...)
	{
		RunTimeError(RUNTIMEERROR_NOTENOUGHMEMORY);
		return dwArrayPtr;
	}
}
DARKSDK void ArrayDeleteElement(DWORD dwArrayPtr, int iIndex)
{
	// If no array, leave now
	if(dwArrayPtr==NULL) return;

	// lee - 140306 - u60b3 - Do not allow multi-dimensional arrays
	if ( IsArraySingleDim ( dwArrayPtr )==false ) { RunTimeError(RUNTIMEERROR_ARRAYMUSTBESINGLEDIM); return; }

	DWORD dwSizeOfTable = *((DWORD*)dwArrayPtr-4);
	if(dwSizeOfTable==0)
	{
		// already empty - silent failure
		return;
	}
	if(iIndex<0 || iIndex>=(int)dwSizeOfTable)
	{
		RunTimeError(RUNTIMEERROR_ARRAYINDEXINVALID);
		return;
	}

	// Prepare pointers
	DWORD dwDataItemSize = *((DWORD*)dwArrayPtr-3);
	DWORD dwRefSizeInBytes = dwSizeOfTable * 4;
	DWORD dwFlagSizeInBytes = dwSizeOfTable * 1;
	DWORD* pRef = (DWORD*)dwArrayPtr;
	LPSTR pFlag = (LPSTR)(((LPSTR)dwArrayPtr)+dwRefSizeInBytes);
	LPSTR pData = (LPSTR)(((LPSTR)dwArrayPtr)+dwRefSizeInBytes+dwFlagSizeInBytes);
	DWORD dwOffset = ((DWORD)pRef[iIndex] - (DWORD)pData);// / dwDataItemSize;

	// leeadd - 211008 - u71 - check for strings before remove this element
	DWORD dwInternalTypeIndex = *((DWORD*)dwArrayPtr-2);
	LPSTR pPattern = GetTypePatternCore ( NULL, dwInternalTypeIndex );
	if ( pPattern )
	{
		// go through pattern which matches basic types
		DWORD dwTypeInternalOffset = 0;
		for ( DWORD n=0; n<strlen(pPattern); n++ )
		{
			// delete any strings in the user type
			if ( pPattern[n]=='S' )
			{
				// U74 - 050509 - delete CORRECT part of block!
				LPSTR* pStringData = (LPSTR*)(pData+dwOffset+dwTypeInternalOffset);
				if ( *pStringData )
				{
					delete[] *pStringData;
					*pStringData=NULL;
				}
			}

			// next one..
			switch ( pPattern[n] )
			{
				case 'B'	: dwTypeInternalOffset+=4; break;//1
				case 'Y'	: dwTypeInternalOffset+=4; break;//1
				case 'W'	: dwTypeInternalOffset+=4; break;//2
				case 'O'	: dwTypeInternalOffset+=8; break;
				case 'R'	: dwTypeInternalOffset+=8; break;
				default		: dwTypeInternalOffset+=4; break;
			}
		}
		delete[] pPattern;
	}

	// Shuffle to remove item from ref table
	DWORD dwAmountToShuffle = 0;
	if((dwSizeOfTable-iIndex-1)>0) dwAmountToShuffle = (dwSizeOfTable-iIndex-1)*4;
	if(dwAmountToShuffle>0) memcpy(pRef+iIndex, pRef+iIndex+1, dwAmountToShuffle);

	// Store ref data
	DWORD** pStoreRef = new DWORD* [ dwSizeOfTable ];
	memcpy ( pStoreRef, pRef, dwSizeOfTable * sizeof(DWORD*) );

	// First shuffle out deleted data
	dwAmountToShuffle = 0;
	DWORD dwTotalSizeOfData = dwSizeOfTable * dwDataItemSize;
	if((dwTotalSizeOfData-dwOffset-dwDataItemSize)>0) dwAmountToShuffle = (dwTotalSizeOfData-dwOffset-dwDataItemSize);
	if(dwAmountToShuffle>0) memcpy(pData+dwOffset, pData+dwOffset+dwDataItemSize, dwAmountToShuffle);

	// Reduce size of array
	dwSizeOfTable = dwSizeOfTable - 1;
	*((DWORD*)dwArrayPtr-4) = dwSizeOfTable;

	// Get new sizes and pointers
	DWORD dwNewRefSizeInBytes = dwSizeOfTable * 4;
	DWORD dwNewFlagSizeInBytes = dwSizeOfTable * 1;
	LPSTR pNewFlag = (LPSTR)(((LPSTR)dwArrayPtr)+dwNewRefSizeInBytes);
	LPSTR pNewData = (LPSTR)(((LPSTR)dwArrayPtr)+dwNewRefSizeInBytes+dwNewFlagSizeInBytes);

	// Generate new ref data
	for(DWORD i=0; i<dwSizeOfTable; i++)
	{
		// leefix - 210604 - retain pattern of ref data / 260604-u54-dwDataItemSize not 1
		DWORD dwRedirectOffset = (DWORD)pStoreRef[i] - (DWORD)pData;
		if ( dwRedirectOffset >= dwOffset ) dwRedirectOffset-=dwDataItemSize;
		pRef[i] = (DWORD)(pNewData + dwRedirectOffset);
	}

	// free stored ref data
	delete[] pStoreRef;

	// Set Flag Data with ones
	memset(pNewFlag, 1, dwNewFlagSizeInBytes);

	// Then shuffle all data to new position
	DWORD dwNewTotalSizeOfData = dwSizeOfTable * dwDataItemSize;
	memcpy(pNewData, pData, dwNewTotalSizeOfData);

	// Ensure internal index is still valid
	if( *((DWORD*)dwArrayPtr-1) >= dwSizeOfTable )
		*((DWORD*)dwArrayPtr-1) = dwSizeOfTable - 1;
}
DARKSDK void ArrayDeleteElement(DWORD dwArrayPtr)
{
	// lee - 140306 - u60b3 - Do not allow multi-dimensional arrays
	if ( IsArraySingleDim ( dwArrayPtr )==false ) { RunTimeError(RUNTIMEERROR_ARRAYMUSTBESINGLEDIM); return; }

	DWORD dwSizeOfTable = *((DWORD*)dwArrayPtr-4);
	if(dwSizeOfTable==0)
	{
		// already empty - silent failure
		return;
	}

	int iCurrentIndex = *((DWORD*)dwArrayPtr-1);
	ArrayDeleteElement ( dwArrayPtr, iCurrentIndex );
}
DARKSDK void EmptyArray(DWORD dwAllocation)
{
	// If no array, leave now
	if(dwAllocation==NULL) return;

	DWORD dwSizeOfTable = *((DWORD*)dwAllocation-4);
	if(dwSizeOfTable==0)
	{
		// already empty - silent failure
		return;
	}

	// Get Array Information
	LPSTR pArrayPtr = ((LPSTR)dwAllocation)-HEADERSIZEINBYTES;
	DWORD* pHeader	= (DWORD*)(pArrayPtr);
	DWORD dwHeaderSizeInBytes = HEADERSIZEINBYTES;
	// should this not be 10,11,12,13??  - 140104
	DWORD dwSizeOfArray = pHeader[0];
	DWORD dwSizeOfOneDataItem = pHeader[1];
	DWORD dwTypeValueOfOneDataItem = pHeader[2];
	DWORD dwInternalIndex = pHeader[3];
	DWORD dwRefSizeInBytes = dwSizeOfArray * 4;
	DWORD dwFlagSizeInBytes = dwSizeOfArray * 1;
	DWORD dwDataSizeInBytes = dwSizeOfArray * dwSizeOfOneDataItem;
	DWORD* pRef = (DWORD*)(pArrayPtr+dwHeaderSizeInBytes);
	LPSTR pFlag = (LPSTR)(pArrayPtr+dwHeaderSizeInBytes+dwRefSizeInBytes);
	LPSTR pData = (LPSTR)(pArrayPtr+dwHeaderSizeInBytes+dwRefSizeInBytes+dwFlagSizeInBytes);

	// Clear all data from array
	FreeStringsFromArray( dwAllocation );
	memset(pRef, 0, dwRefSizeInBytes);
	memset(pFlag, 0, dwFlagSizeInBytes);
	memset(pData, 0, dwDataSizeInBytes);

	// Reset size of array to empty
	*((DWORD*)dwAllocation-4) = 0;
	*((DWORD*)dwAllocation-1) = 0;
}
DARKSDK DWORD PushToStack(DWORD dwArrayPtr)
{
	// lee - 140306 - u60b3 - Do not allow multi-dimensional arrays
	if ( IsArraySingleDim ( dwArrayPtr )==false ) { RunTimeError(RUNTIMEERROR_ARRAYMUSTBESINGLEDIM); return dwArrayPtr; }

	// add item to bottom of list
	dwArrayPtr = ArrayInsertAtBottom(dwArrayPtr);

	// place index to end
	int iIndexAtEnd = *((DWORD*)dwArrayPtr-4) - 1;
	*((DWORD*)dwArrayPtr-1) = iIndexAtEnd;

	// return array ptr
	return dwArrayPtr;
}
DARKSDK void PopFromStack(DWORD dwArrayPtr)
{
	// lee - 140306 - u60b3 - Do not allow multi-dimensional arrays
	if ( IsArraySingleDim ( dwArrayPtr )==false ) { RunTimeError(RUNTIMEERROR_ARRAYMUSTBESINGLEDIM); return; }

	// remove from bottom if list
	int iIndexAtEnd = *((DWORD*)dwArrayPtr-4) - 1;
	ArrayDeleteElement(dwArrayPtr, iIndexAtEnd);

	// place index to end
	iIndexAtEnd = *((DWORD*)dwArrayPtr-4) - 1;
	*((DWORD*)dwArrayPtr-1) = iIndexAtEnd;
}
DARKSDK DWORD AddToQueue(DWORD dwArrayPtr)
{
	// lee - 140306 - u60b3 - Do not allow multi-dimensional arrays
	if ( IsArraySingleDim ( dwArrayPtr )==false ) { RunTimeError(RUNTIMEERROR_ARRAYMUSTBESINGLEDIM); return dwArrayPtr; }

	// add to top of list
	dwArrayPtr = ArrayInsertAtBottom(dwArrayPtr);

	// place index to end
	int iIndexAtEnd = *((DWORD*)dwArrayPtr-4) - 1;
	*((DWORD*)dwArrayPtr-1) = iIndexAtEnd;

	// return array ptr
	return dwArrayPtr;
}
DARKSDK void RemoveFromQueue(DWORD dwArrayPtr)
{
	// lee - 140306 - u60b3 - Do not allow multi-dimensional arrays
	if ( IsArraySingleDim ( dwArrayPtr )==false ) { RunTimeError(RUNTIMEERROR_ARRAYMUSTBESINGLEDIM); return; }

	// remove from top of list
	ArrayDeleteElement(dwArrayPtr, 0);

	// place index to zero
	*((DWORD*)dwArrayPtr-1) = 0;
}
DARKSDK void ArrayIndexToStack(DWORD dwArrayPtr)
{
	// set index to last item in array
	if(dwArrayPtr) *((DWORD*)dwArrayPtr-1) = *((DWORD*)dwArrayPtr-4)-1;
}
DARKSDK void ArrayIndexToQueue(DWORD dwArrayPtr)
{
	// set index to first item in array
	if(dwArrayPtr) *((DWORD*)dwArrayPtr-1) = 0;
}

// HARDCODE COMMANDS

DARKSDK LPSTR MakeMemory(int iSize)
{
	LPSTR pMem = new char[iSize];
	return pMem;
}

DARKSDK void DeleteByteMemory(LPSTR dwMem)
{
	if(dwMem) delete[] (LPSTR)dwMem;
}

DARKSDK void FillByteMemory(LPSTR dwDest, int iValue, int iSize)
{
	memset(dwDest, iValue, iSize);
}

DARKSDK void CopyMemory(LPSTR dwDest, LPSTR dwSrc, int iSize)
{
	if ( iSize > 0 )
		memcpy(dwDest, dwSrc, iSize);
}

// DATA STATEMENT COMMAND FUNCTIONS
DARKSDK void Restore(void)
{
	g_pDataLabelPtr = g_pDataLabelStart;		
}
DARKSDK void RestoreD(DWORD dwDataLabel)
{
	if ( dwDataLabel==0 )
		g_pDataLabelPtr = g_pDataLabelStart;
	else
		g_pDataLabelPtr = (LPSTR)dwDataLabel;
		
}
DARKSDK DWORD ReadL(void)
{
	double dData=0;
	if(g_pDataLabelPtr && g_pDataLabelPtr<g_pDataLabelEnd)
	{
		if(*(g_pDataLabelPtr+0)==1)
			dData = *(double*)(g_pDataLabelPtr+2);

		// Advance After Read, but only to end of data
		g_pDataLabelPtr+=10;
	}

	int iValue = (int)dData;

	return *(DWORD*)&iValue;
}
DARKSDK DWORD ReadF(void)
{
	double dData=0;
	if(g_pDataLabelPtr && g_pDataLabelPtr<g_pDataLabelEnd)
	{
		if(*(g_pDataLabelPtr+0)==1)
			dData = *(double*)(g_pDataLabelPtr+2);

		// Advance After Read, but only to end of data
		g_pDataLabelPtr+=10;
	}

	float fValue = (float)dData;

	return *(DWORD*)&fValue;
}
DARKSDK DWORD ReadS(DWORD pDestStr)
{
	//if(pDestStr) delete[] (LPSTR)pDestStr;
	if(pDestStr) g_pGlob->CreateDeleteString((char**)&pDestStr, 0);

	LPSTR pDatStr = NULL;
	if(g_pDataLabelPtr && g_pDataLabelPtr<g_pDataLabelEnd)
	{
		if(*(g_pDataLabelPtr+0)==2)
			pDatStr = (LPSTR)*(DWORD*)(g_pDataLabelPtr+2);

		// Advance After Read, but only to end of data
		g_pDataLabelPtr+=10;
	}

	LPSTR pString;
	if (pDatStr)
	{
		DWORD dwLength = strlen(pDatStr);
		pString=new char[dwLength+1];
		strcpy(pString, pDatStr);
	}
	else
	{
		pString = new char[1];
		*pString = 0;
	}

	return (DWORD)pString;
}
DARKSDK BYTE ReadB(void)
{
	double dData=0;
	if(g_pDataLabelPtr && g_pDataLabelPtr<g_pDataLabelEnd)
	{
		if(*(g_pDataLabelPtr+0)==1)
			dData = *(double*)(g_pDataLabelPtr+2);

		// Advance After Read, but only to end of data
		g_pDataLabelPtr+=10;
	}

	BYTE dwValue = (BYTE)dData;

	return dwValue;
}
DARKSDK WORD ReadW(void)
{
	double dData=0;
	if(g_pDataLabelPtr && g_pDataLabelPtr<g_pDataLabelEnd)
	{
		if(*(g_pDataLabelPtr+0)==1)
			dData = *(double*)(g_pDataLabelPtr+2);

		// Advance After Read, but only to end of data
		g_pDataLabelPtr+=10;
	}

	WORD dwValue = (WORD)dData;

	return dwValue;
}
DARKSDK DWORD ReadD(void)
{
	double dData=0;
	if(g_pDataLabelPtr && g_pDataLabelPtr<g_pDataLabelEnd)
	{
		if(*(g_pDataLabelPtr+0)==1)
			dData = *(double*)(g_pDataLabelPtr+2);

		// Advance After Read, but only to end of data
		g_pDataLabelPtr+=10;
	}

	DWORD dwValue = (DWORD)dData;

	return dwValue;
}
DARKSDK LONGLONG ReadR(void)
{
	double dData=0;
	if(g_pDataLabelPtr && g_pDataLabelPtr<g_pDataLabelEnd)
	{
		if(*(g_pDataLabelPtr+0)==1)
			dData = *(double*)(g_pDataLabelPtr+2);

		// Advance After Read, but only to end of data
		g_pDataLabelPtr+=10;
	}

	LONGLONG lValue = (LONGLONG)dData;

	return lValue;
}
DARKSDK double ReadO(void)
{
	double dValue=0;
	if(g_pDataLabelPtr && g_pDataLabelPtr<g_pDataLabelEnd)
	{
		if(*(g_pDataLabelPtr+0)==1)
			dValue = *(double*)(g_pDataLabelPtr+2);

		// Advance After Read, but only to end of data
		g_pDataLabelPtr+=10;
	}

	return dValue;
}

DARKSDK DWORD EqualDDD(DWORD dwValueA, DWORD dwValueB)
{
	int result = dwValueA==dwValueB;
	return result;
}
DARKSDK DWORD GreaterDDD(DWORD dwValueA, DWORD dwValueB)
{
	int result = dwValueA>dwValueB;
	return result;
}
DARKSDK DWORD LessDDD(DWORD dwValueA, DWORD dwValueB)
{
	int result = dwValueA<dwValueB;
	return result;
}
DARKSDK DWORD NotEqualDDD(DWORD dwValueA, DWORD dwValueB)
{
	int result = dwValueA!=dwValueB;
	return result;
}
DARKSDK DWORD GreaterEqualDDD(DWORD dwValueA, DWORD dwValueB)
{
	int result = dwValueA>=dwValueB;
	return result;
}
DARKSDK DWORD LessEqualDDD(DWORD dwValueA, DWORD dwValueB)
{
	int result = dwValueA<=dwValueB;
	return result;
}

// EXTERNAL SUPPORT MATHS

DARKSDK DWORD PowerLLL(int iValueA, int iValueB)
{
	// do not know the ASM version of this
	int result = (int)pow((long double)iValueA,(long double)iValueB);
	return *((DWORD*)&result);
}
DARKSDK DWORD PowerBBB(DWORD dwValueA, DWORD dwValueB)
{
	DWORD result = (unsigned char)pow((long double)dwValueA,(long double)dwValueB);
	return result;
}
DARKSDK DWORD PowerWWW(DWORD dwValueA, DWORD dwValueB)
{
	DWORD result = (WORD)pow((long double)dwValueA,(long double)dwValueB);
	return result;
}
DARKSDK DWORD PowerDDD(DWORD dwValueA, DWORD dwValueB)
{
	DWORD result = (DWORD)pow(( double ) dwValueA,( double ) dwValueB);
	return result;
}

// FLOAT MATHS

DARKSDK DWORD PowerFFF(float fValueA, float fValueB)
{
	float result = (float)pow(fValueA,fValueB);
	return *((DWORD*)&result);
}
DARKSDK DWORD MulFFF(float fValueA, float fValueB)
{
	float result = fValueA*fValueB;
	return *((DWORD*)&result);
}
DARKSDK DWORD DivFFF(float fValueA, float fValueB)
{
	if(fValueB==0) return 0;
	float result = fValueA/fValueB;
	return *((DWORD*)&result);
}
DARKSDK DWORD AddFFF(float fValueA, float fValueB)
{
	float result = fValueA+fValueB;
	return *((DWORD*)&result);
}
DARKSDK DWORD SubFFF(float fValueA, float fValueB)
{
	float result = fValueA-fValueB;
	return *((DWORD*)&result);
}
DARKSDK DWORD ModFFF(float fValueA, float fValueB)
{
	// lee - 150206 - u60 -floating point MOD added
	if(fValueB==0) return 0;
	double w = (double)fValueA;
	double x = (double)fValueB;
	double z = fmod( w, x );
	float result = (float)z;
	return *((DWORD*)&result);
}

// FLOAT COMPARE MATHS

DARKSDK DWORD EqualLFF(float fValueA, float fValueB)
{
	return fValueA==fValueB;
}
DARKSDK DWORD GreaterLFF(float fValueA, float fValueB)
{
	return fValueA>fValueB;
}
DARKSDK DWORD LessLFF(float fValueA, float fValueB)
{
	return fValueA<fValueB;
}
DARKSDK DWORD NotEqualLFF(float fValueA, float fValueB)
{
	return fValueA!=fValueB;
}
DARKSDK DWORD GreaterEqualLFF(float fValueA, float fValueB)
{
	return fValueA>=fValueB;
}
DARKSDK DWORD LessEqualLFF(float fValueA, float fValueB)
{
	return fValueA<=fValueB;
}

// STRING COMPARE MATHS

DARKSDK DWORD EqualLSS(DWORD dwSrcStr,DWORD dwDestStr)
{
	if(dwSrcStr && dwDestStr)
	{
		if(strcmp((LPSTR)dwSrcStr, (LPSTR)dwDestStr)==NULL)
			return 1;
		else
			return 0;
	}
	else
	{
		if(dwSrcStr==dwDestStr)
		{
			return 1;
		}
		else
		{
			if(dwSrcStr && dwDestStr==NULL)
			{
				if(strcmp((LPSTR)dwSrcStr, "")==NULL)
					return 1;
				else
					return 0;
			}
			if(dwDestStr && dwSrcStr==NULL)
			{
				if(strcmp((LPSTR)dwDestStr, "")==NULL)
					return 1;
				else
					return 0;
			}
			return 0;
		}
	}
}
DARKSDK DWORD GreaterLSS(DWORD dwSrcStr,DWORD dwDestStr)
{
	if(dwSrcStr && dwDestStr)
	{
		if(strcmp((LPSTR)dwSrcStr, (LPSTR)dwDestStr)>0)
			return 1;
		else
			return 0;
	}
	else
	{
		if(dwSrcStr==dwDestStr)
			return 1;
		else
			return 0;
	}
}
DARKSDK DWORD LessLSS(DWORD dwSrcStr,DWORD dwDestStr)
{
	if(dwSrcStr && dwDestStr)
	{
		if(strcmp((LPSTR)dwSrcStr, (LPSTR)dwDestStr)<0)
			return 1;
		else
			return 0;
	}
	else
	{
		if(dwSrcStr==dwDestStr)
			return 1;
		else
			return 0;
	}
}
DARKSDK DWORD NotEqualLSS(DWORD dwSrcStr,DWORD dwDestStr)
{
	if(dwSrcStr && dwDestStr)
	{
		if(strcmp((LPSTR)dwSrcStr, (LPSTR)dwDestStr)!=0)
			return 1;
		else
			return 0;
	}
	else
	{
		// leefix - 060405 - inverse of equal
		if(dwSrcStr==dwDestStr)
		{
			return 0;
		}
		else
		{
			if(dwSrcStr && dwDestStr==NULL)
			{
				if(strcmp((LPSTR)dwSrcStr, "")==NULL)
					return 0;
				else
					return 1;
			}
			if(dwDestStr && dwSrcStr==NULL)
			{
				if(strcmp((LPSTR)dwDestStr, "")==NULL)
					return 0;
				else
					return 1;
			}
			return 1;
		}
	}
}
DARKSDK DWORD GreaterEqualLSS(DWORD dwSrcStr,DWORD dwDestStr)
{
	if(dwSrcStr && dwDestStr)
	{
		if(strcmp((LPSTR)dwSrcStr, (LPSTR)dwDestStr)>=0)
			return 1;
		else
			return 0;
	}
	else
	{
		if(dwSrcStr==dwDestStr)
			return 1;
		else
			return 0;
	}
}
DARKSDK DWORD LessEqualLSS(DWORD dwSrcStr,DWORD dwDestStr)
{
	if(dwSrcStr && dwDestStr)
	{
		if(strcmp((LPSTR)dwSrcStr, (LPSTR)dwDestStr)<=0)
			return 1;
		else
			return 0;
	}
	else
	{
		if(dwSrcStr==dwDestStr)
			return 1;
		else
			return 0;
	}
}


// STRING MATHS

DARKSDK DWORD AddSSS(DWORD dwRetStr, DWORD dwSrcStrA, DWORD dwSrcStrB)
{
	DWORD length=1;
	if((DWORD*)dwSrcStrA) length=strlen((LPSTR)(DWORD*)dwSrcStrA);
	if((DWORD*)dwSrcStrB) length+=strlen((LPSTR)(DWORD*)dwSrcStrB);
	LPSTR lpNewStr = new char[length+1];
	strcpy(lpNewStr,"");
	if((DWORD*)dwSrcStrA) strcat(lpNewStr, (LPSTR)dwSrcStrA);
	if((DWORD*)dwSrcStrB) strcat(lpNewStr, (LPSTR)dwSrcStrB);
	if((DWORD*)dwRetStr) delete[] (DWORD*)dwRetStr;
	return (DWORD)lpNewStr;
}
DARKSDK DWORD EquateSS(DWORD dwDestStr,DWORD dwSrcStr)
{
	DWORD length=1;
	if((DWORD*)dwSrcStr) length=strlen((LPSTR)(DWORD*)dwSrcStr);
	LPSTR lpNewStr = NULL;
	if ( dwSrcStr!=0 )
	{
		lpNewStr = new char[length+1];
		if((DWORD*)dwSrcStr) strcpy(lpNewStr, (LPSTR)dwSrcStr); else strcpy(lpNewStr,"");
	}
	if((DWORD*)dwDestStr) delete[] (DWORD*)dwDestStr;
	return (DWORD)lpNewStr;
}
DARKSDK DWORD FreeSS(DWORD dwDestStr)
{
	if(dwDestStr) delete[] (DWORD*)dwDestStr;
	return 0;
}
DARKSDK DWORD FreeStringSS(DWORD dwDestStr)
{
	if(dwDestStr) delete[] (DWORD*)dwDestStr;
	return 0;
}

// DOUBLE FLOAT MATHS

DARKSDK double PowerOOO(double dValueA, double dValueB)
{
	double result = (float)pow(dValueA,dValueB);
	return result;
}
DARKSDK double MulOOO(double dValueA, double dValueB)
{
	double result = dValueA*dValueB;
	return result;
}
DARKSDK double DivOOO(double dValueA, double dValueB)
{
	if(dValueB==0) return 0;
	double result = dValueA/dValueB;
	return result;
}
DARKSDK double AddOOO(double dValueA, double dValueB)
{
	double result = dValueA+dValueB;
	return result;
}
DARKSDK double SubOOO(double dValueA, double dValueB)
{
	double result = dValueA-dValueB;
	return result;
}

// DOUBLE FLOAT COMPARISONS

DARKSDK DWORD EqualLOO(double dValueA, double dValueB)
{
	int result = dValueA==dValueB;
	return *((DWORD*)&result);
}
DARKSDK DWORD GreaterLOO(double dValueA, double dValueB)
{
	int result = dValueA>dValueB;
	return *((DWORD*)&result);
}
DARKSDK DWORD LessLOO(double dValueA, double dValueB)
{
	int result = dValueA<dValueB;
	return *((DWORD*)&result);
}
DARKSDK DWORD NotEqualLOO(double dValueA, double dValueB)
{
	int result = dValueA!=dValueB;
	return *((DWORD*)&result);
}
DARKSDK DWORD GreaterEqualLOO(double dValueA, double dValueB)
{
	int result = dValueA>=dValueB;
	return *((DWORD*)&result);
}
DARKSDK DWORD LessEqualLOO(double dValueA, double dValueB)
{
	int result = dValueA<=dValueB;
	return *((DWORD*)&result);
}

// DOUBLE INTEGER MATHS

DARKSDK LONGLONG PowerRRR(LONGLONG dValueA, LONGLONG dValueB)
{
	LONGLONG result = (LONGLONG)pow((double)dValueA,(double)dValueB);
	return result;
}
DARKSDK LONGLONG MulRRR(LONGLONG dValueA, LONGLONG dValueB)
{
	LONGLONG result = dValueA*dValueB;
	return result;
}
DARKSDK LONGLONG DivRRR(LONGLONG dValueA, LONGLONG dValueB)
{
	if(dValueB==0) return 0;
	LONGLONG result = dValueA/dValueB;
	return result;
}
DARKSDK LONGLONG AddRRR(LONGLONG dValueA, LONGLONG dValueB)
{
	LONGLONG result = dValueA+dValueB;
	return result;
}
DARKSDK LONGLONG SubRRR(LONGLONG dValueA, LONGLONG dValueB)
{
	LONGLONG result = dValueA-dValueB;
	return result;
}

// DOUBLE INTEGER COMPARISONS

DARKSDK DWORD EqualLRR(LONGLONG lValueA, LONGLONG lValueB)
{
	int result = lValueA==lValueB;
	return *((DWORD*)&result);
}
DARKSDK DWORD GreaterLRR(LONGLONG lValueA, LONGLONG lValueB)
{
	int result = lValueA>lValueB;
	return *((DWORD*)&result);
}
DARKSDK DWORD LessLRR(LONGLONG lValueA, LONGLONG lValueB)
{
	int result = lValueA<lValueB;
	return *((DWORD*)&result);
}
DARKSDK DWORD NotEqualLRR(LONGLONG lValueA, LONGLONG lValueB)
{
	int result = lValueA!=lValueB;
	return *((DWORD*)&result);
}
DARKSDK DWORD GreaterEqualLRR(LONGLONG lValueA, LONGLONG lValueB)
{
	int result = lValueA>=lValueB;
	return *((DWORD*)&result);
}
DARKSDK DWORD LessEqualLRR(LONGLONG lValueA, LONGLONG lValueB)
{
	int result = lValueA<=lValueB;
	return *((DWORD*)&result);
}

// CASTING MATHS

DARKSDK DWORD CastLtoF(int iValue)
{
	float result = (float)iValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastLtoB(int iValue)
{
	unsigned char result = (unsigned char)iValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastLtoY(int iValue)
{
	unsigned char result = (unsigned char)iValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastLtoW(int iValue)
{
	WORD result = (WORD)iValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastLtoD(int iValue)
{
	DWORD result = (DWORD)iValue;
	return *((DWORD*)&result);
}
DARKSDK double CastLtoO(int iValue)
{
	return (double)iValue;
}
DARKSDK LONGLONG CastLtoR(int iValue)
{
	return (LONGLONG)iValue;
}
DARKSDK DWORD CastFtoL(float fValue)
{
	int result = (int)fValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastFtoB(float fValue)
{
	unsigned char result = (unsigned char)fValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastFtoW(float fValue)
{
	WORD result = (WORD)fValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastFtoD(float fValue)
{
	// a maxed out DWORD produces wrong float, so keep it within 4bytes 
	LONGLONG Long = (LONGLONG)fValue;
	if(Long>4294967295) Long=4294967295;

	DWORD result = (DWORD)Long;
	return *((DWORD*)&result);
}
DARKSDK double CastFtoO(float fValue)
{
	// LEEFIX - 141102 - FLD is different on AMD processprs, so truncate..
	return (double)fValue;
}
DARKSDK LONGLONG CastFtoR(float fValue)
{
	return (LONGLONG)fValue;
}
DARKSDK DWORD CastBtoL(unsigned char cValue)
{
	int result = (int)cValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastBtoF(unsigned char cValue)
{
	float result = (float)cValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastBtoW(unsigned char cValue)
{
	WORD result = (WORD)cValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastBtoD(unsigned char cValue)
{
	DWORD result = (DWORD)cValue;
	return *((DWORD*)&result);
}
DARKSDK double CastBtoO(unsigned char cValue)
{
	return (double)cValue;
}
DARKSDK LONGLONG CastBtoR(unsigned char cValue)
{
	return (LONGLONG)cValue;
}
DARKSDK DWORD CastWtoL(WORD wValue)
{
	int result = (int)wValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastWtoF(WORD wValue)
{
	float result = (float)wValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastWtoB(WORD wValue)
{
	unsigned char result = (unsigned char)wValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastWtoD(WORD wValue)
{
	DWORD result = (DWORD)wValue;
	return *((DWORD*)&result);
}
DARKSDK double CastWtoO(WORD wValue)
{
	return (double)wValue;
}
DARKSDK LONGLONG CastWtoR(WORD wValue)
{
	return (LONGLONG)wValue;
}
DARKSDK DWORD CastDtoL(DWORD dwValue)
{
	int result = (int)dwValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastDtoF(DWORD dwValue)
{
	float result = (float)dwValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastDtoB(DWORD dwValue)
{
	unsigned char result = (unsigned char)dwValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastDtoW(DWORD dwValue)
{
	WORD result = (WORD)dwValue;
	return *((DWORD*)&result);
}
DARKSDK double CastDtoO(DWORD dwValue)
{
	return (double)dwValue;
}
DARKSDK LONGLONG CastDtoR(DWORD dwValue)
{
	return (LONGLONG)dwValue;
}
DARKSDK DWORD CastOtoL(double dValue)
{
	int result = (int)dValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastOtoF(double dValue)
{
	float result = (float)dValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastOtoB(double dValue)
{
	unsigned char result = (unsigned char)dValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastOtoW(double dValue)
{
	WORD result = (WORD)dValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastOtoD(double dValue)
{
	DWORD result = (DWORD)dValue;
	return *((DWORD*)&result);
}
DARKSDK LONGLONG CastOtoR(double dValue)
{
	LONGLONG result = (LONGLONG)dValue;
	return result;
}
DARKSDK DWORD CastRtoL(LONGLONG lValue)
{
	int result = (int)lValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastRtoF(LONGLONG lValue)
{
	float result = (float)lValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastRtoB(LONGLONG lValue)
{
	unsigned char result = (unsigned char)lValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastRtoW(LONGLONG lValue)
{
	WORD result = (WORD)lValue;
	return *((DWORD*)&result);
}
DARKSDK DWORD CastRtoD(LONGLONG lValue)
{
	DWORD result = (DWORD)lValue;
	return *((DWORD*)&result);
}
DARKSDK double CastRtoO(LONGLONG lValue)
{
	return (double)lValue;
}

// MATHEMATICAL COMMANDS
DBPRO_GLOBAL double gDegToRad = 3.141592654f/180.0f;
DBPRO_GLOBAL double gRadToDeg = 180.0f/3.141592654f;

DB_EXPORT dbReturnFloat_t AbsFF(float fValue)
{
	return dbReturnFloat( db3::Abs( fValue ) );
}

DARKSDK DWORD IntLF(float fValue)
{
	int result = (int)fValue;
	return *((DWORD*)&result);
}

DARKSDK DWORD AcosFF(float fValue)
{
	float result = (float)(acos(fValue)*gRadToDeg);
	return *((DWORD*)&result);
}

DARKSDK float Asin(float fValue)
{
	float result = (float)(asin(fValue)*gRadToDeg);
	return result;
}

DARKSDK DWORD AtanFF(float fValue)
{
	float result = (float)(atan(fValue)*gRadToDeg);
	return *((DWORD*)&result);
}

DARKSDK float Atan2(float fA, float fB)
{
	float result = (float)(atan2(fA, fB)*gRadToDeg);
	return result;
}

DARKSDK float Cos(float fAngle)
{
	return db3::Cos(fAngle);
}

DARKSDK float Sin(float fAngle)
{
	return db3::Sin(fAngle);
}

DARKSDK float Tan(float fAngle)
{
	return db3::Tan(fAngle);
}

DARKSDK float HcosFF(float fAngle)
{
	float result = (float)cosh(fAngle*gDegToRad);
	return result;
}

DARKSDK float HsinFF(float fAngle)
{
	float result = (float)sinh(fAngle*gDegToRad);
	return result;
}

DARKSDK float HtanFF(float fAngle)
{
	float result = (float)tanh(fAngle*gDegToRad);
	return result;
}

DARKSDK float Sqrt(float fValue)
{
	float result = (float)sqrt(fValue);
	return result;
}

DARKSDK float ExpFF(float fExp)
{
	float result = (float)exp(fExp);
	return result;
}

DB_EXPORT float SignF(float a)
{
	return db3::Sign( a );
}
DB_EXPORT float CopySign(float a, float b)
{
	return db3::CopySign( a, b );
}

DB_EXPORT int FloatToIntFast(float x) {
	return db3::FloatToIntFast( x );
}
DB_EXPORT DWORD FloatToDwordFast(float x) {
	return db3::FloatToUIntFast( x );
}

DB_EXPORT dbReturnFloat_t SqrtFast(float x) {
	return dbReturnFloat( db3::SqrtFast( x ) );
}
DB_EXPORT dbReturnFloat_t InvSqrtFast(float x) {
	return dbReturnFloat( db3::InvSqrtFast( x ) );
}

DB_EXPORT dbReturnFloat_t Lerp(float x, float y, float t) {
	return dbReturnFloat( db3::Lerp(x, y, t) );
}
	
DARKSDK void Randomize(int iSeed)
{
	srand(iSeed);
}

DARKSDK int Rnd(int r)
{
	int result=0;
	if(r>0)
	{
		// leefix - 250604 - u54 - 0 to 22 million now
		if ( r>1000 )  result += (rand()*1000);
		if ( r>100 ) result += (rand()*100);
		if ( r>10 ) result += (rand()*10);
		result += rand();
		result %= r+1;
	}
	return result;
}

// New MATH FUNCTIONS

DARKSDK float Ceil(float x)
{
	float value = ceil ( x );
	return value;
}

DARKSDK float Floor(float x)
{
	float value = floor ( x );
	return value;
}

// 3D MATH EXPRESSIONS

DARKSDK float wrapangleoffset(float da)
{
	// aaron - 20120811 - Faster version from NormalizeAngle360
	return db3::NormalizeAngle360(da);
}

DARKSDK float CurveValue(float a, float da, float sp)
{
	if(sp<1.0f) sp=1.0f;
	float diff = a-da;
	da=da+(diff/sp);
	return da;
}

DARKSDK float WrapValue(float da)
{
	return wrapangleoffset(da);
}

DARKSDK float NewXValue(float x, float a, float b)
{
	float da = x + ((float)sin(GGToRadian(a))*b);
	return da;
}

DARKSDK float NewZValue(float z, float a, float b)
{
	float da = z + ((float)cos(GGToRadian(a))*b);
	return da;
}

DARKSDK float NewYValueFFFF(float y, float a, float b)
{
	float da = y - ((float)sin(GGToRadian(a))*b);
	return da;
}

DARKSDK float CurveAngle(float a, float da, float sp)
{
	if(sp<1.0f) sp=1.0f;
	a = wrapangleoffset(a);
	da = wrapangleoffset(da);
	float diff = a-da;
	if(diff<-180.0f) diff=(a+360.0f)-da;
	if(diff>180.0f) diff=a-(da+360.0f);
	da=da+(diff/sp);
	da = wrapangleoffset(da);
	return da;
}

DB_EXPORT int NextPowerOfTwo1(int x) {
	return db3::NextPowerOfTwo(x);
}
DB_EXPORT int NextPowerOfTwo2(int x, int y) {
	return db3::NextSquarePowerOfTwo(x, y);
}

DB_EXPORT dbReturnFloat_t Clamp(float x, float l, float h) {
	return dbReturnFloat( db3::Clamp(x, l,h) );
}
DB_EXPORT dbReturnFloat_t ClampSNorm(float x) {
	return dbReturnFloat( db3::ClampSNorm(x) );
}
DB_EXPORT dbReturnFloat_t ClampUNorm(float x) {
	return dbReturnFloat( db3::ClampUNorm(x) );
}

DB_EXPORT dbReturnFloat_t Min(float x, float y) {
	return dbReturnFloat( db3::Min(x, y) );
}
DB_EXPORT dbReturnFloat_t Max(float x, float y) {
	return dbReturnFloat( db3::Max(x, y) );
}

// MISCLANIOUS CORE COMMANDS

DWORD g_dwAppLocalTimeStart = 0;

DARKSDK void SetLocalTimerReset(void)
{
	g_dwAppLocalTimeStart = timeGetTime();
}

DARKSDK int Timer(void)
{
	// leefix - 230606 - u62 - timeBeginPeriod/timeEndPeriod added
	timeBeginPeriod(1);
	DWORD dwTimer = timeGetTime() - g_dwAppLocalTimeStart;
	timeEndPeriod(1);
	int iTimer = (int)dwTimer;
	if ( iTimer < 0 ) 
	{
		// if computer running over 24 days solid, need to restart counter to keep inside INT bounds
		g_dwAppLocalTimeStart = timeGetTime();
		iTimer = 0;
	}
	return iTimer;
}

DARKSDK int MAXTimer(void)
{
	return Timer();
}

DARKSDK void SleepNow(int iDelay)
{
	#ifdef WICKEDENGINE
	#else
	DWORD dwTimeNow=timeGetTime();
	while(timeGetTime()<=dwTimeNow+iDelay)
	{
		if(InternalProcessMessages()==1) break;
	}
	#endif
}

DARKSDK void WaitL(int iDelay)
{
	DWORD dwTimeNow=timeGetTime();
	while(timeGetTime()<=dwTimeNow+iDelay)
	{
		if(InternalProcessMessages()==1) break;
	}
}

DARKSDK void MemorySnapshot(int iMode)
{
	// also go through all DLLs in DBP and ask them to make a snapshot prior to this report
	for ( int iDLL=0; iDLL<1; iDLL++ )
	{
		HINSTANCE hThis = NULL;
		if ( iDLL==0 ) hThis = g_pGlob->g_Basic3D;
		if ( hThis )
		{
			typedef void ( *MM_SNAPSHOT ) ( void );
			MM_SNAPSHOT pSnapShotFunc;
			pSnapShotFunc = ( MM_SNAPSHOT ) GetProcAddress ( hThis, "?mm_SnapShot@@YAXXZ" );
			if ( pSnapShotFunc ) pSnapShotFunc();
		}
	}
}

DARKSDK void WaitForKey(void)
{
	while(g_wWinKey!=0)
	{
		if(InternalProcessMessages()==1) break;
	}
	while(g_wWinKey==0)
	{
		if(InternalProcessMessages()==1) break;
	}
}

DARKSDK void WaitForMouse(void)
{
	while(g_pGlob->iWindowsMouseClick!=0)
	{
		if(InternalProcessMessages()==1) break;
	}
	while(g_pGlob->iWindowsMouseClick==0)
	{
		if(InternalProcessMessages()==1) break;
	}
}

DARKSDK LPSTR Cl(void)
{
	// get command line from main program...
	LPSTR lpNewStr = NULL;
	if(g_pCommandLineString)
	{
		lpNewStr = new char[strlen(g_pCommandLineString)+1];
		strcpy(lpNewStr, g_pCommandLineString);
	}
	else
	{
		lpNewStr = new char[2];
		strcpy(lpNewStr, "");
	}
	return lpNewStr;
}

DARKSDK DWORD GetDate$(DWORD dwDestStr)
{
	char buf[256];
	_strdate(buf);
	return reinterpret_cast<DWORD>(dbReturnString(reinterpret_cast<char *>(dwDestStr), buf));
}

DARKSDK DWORD GetTime$(DWORD dwDestStr)
{
	char buf[256];
	_strtime(buf);
	return reinterpret_cast<DWORD>(dbReturnString(reinterpret_cast<char *>(dwDestStr), buf));
}

//#include "..\..\..\..\GameGuru\Include\gameguru.h"
char g_pInkeyString[2];
DARKSDK LPSTR Inkey(void)
{
	// Changed to use game guru t
	//t.pInkeyString[0] = g_cInkeyCodeKey;
	//t.pInkeyString[1] = 0;
	//return t.pInkeyString;
	g_pInkeyString[0] = g_cInkeyCodeKey;
	g_pInkeyString[1] = 0;
	return g_pInkeyString;
}

DARKSDK void SyncOn(void)
{
	g_bSyncOff = false;
	g_bProcessorFriendly = false;
	g_bCanRenderNow = false;
}

DARKSDK void SyncOff(void)
{
	g_bSyncOff = true;
	g_bProcessorFriendly = true;
	g_bCanRenderNow = true;
}

DARKSDK void Sync(void)
{
	#ifdef WICKEDENGINE
	// Handled by Wicked - render and present elsewhere
	extern bool g_bValidFPS;
	g_bValidFPS = true;
	#else
	ExternalDisplaySync(0);
	ProcessMessagesOnly();
	ConstantNonDisplayUpdate();// this is DUPLICATED in the ExternalDisplaySync(0) call
	g_bCanRenderNow = true;
	#endif
}

DARKSDK void Sync(int iProcessMessages)
{
	#ifdef WICKEDENGINE
	// Handled by Wicked - render and present elsewhere
	#else
	ExternalDisplaySync(0);
#ifdef ENABLEIMGUI
#ifndef USEOLDGUI
	//PE: we need input for fake GetFileMapDWORD
	if (iProcessMessages == 0)
		ProcessMessagesOnly();
#endif
#endif

	if ( iProcessMessages==1 ) ProcessMessagesOnly();
	ConstantNonDisplayUpdate();
	g_bCanRenderNow = true;
	#endif
}

DARKSDK void FastSync(void)
{
	#ifdef WICKEDENGINE
	// Handled by Wicked - render and present elsewhere
	#else
	#ifdef ENABLEIMGUI
	#ifndef USEOLDGUI
	//PE: we need input for fake GetFileMapDWORD
	ProcessMessagesOnly();
	//FastSync dont work if we do not have a imgui frame so:
	#endif
	#endif
	ExternalDisplaySync(1);
	g_bCanRenderNow = true;
	#endif
}

DARKSDK void FastSyncInputOnly(void)
{
	#ifdef WICKEDENGINE
	// Handled by Wicked
	#else
	ProcessMessagesOnly();
	#endif
}

DARKSDK void FastSync ( int iNonDisplayUpdates )
{
	#ifdef WICKEDENGINE
	// Handled by Wicked - render and present elsewhere
	#else
	#ifdef ENABLEIMGUI
	#ifndef USEOLDGUI
	//PE: we need input for fake GetFileMapDWORD
	ProcessMessagesOnly();
	#endif
	#endif
	ExternalDisplaySync(1);
	if ( iNonDisplayUpdates==1 )
	{
		// leeadd - 061108 - reinstated for U71 by request under parameter
		ConstantNonDisplayUpdate();
	}
	g_bCanRenderNow = true;
	#endif
}

DARKSDK void SyncRate(int iRate)
{
	// Reset everything to run full speed
	SAFE_DELETE_ARRAY( g_pdwSyncRateSetting);
	g_dwSyncRateSettingSize = 0;
	g_dwManualSuperStepSetting = 0;

	// Zero is full speed
	// Anything over 1000 can't be measured, so treat that as full speed too
	if (iRate == 0 || iRate > 1000)
		return;

	// Negative is super stepping mode
	if (iRate < 0)
	{
		g_dwManualSuperStepSetting = abs(iRate);
		return;
	}

	// What's left can be dealt with.
	// Generate a table that covers 1 second of frames and fill it out with
	// the basic MS-per-frame value. Any milliseconds dropped using the calculation
	// will be evenly distributed within the table.
	g_dwSyncRateSettingSize = iRate;
	g_pdwSyncRateSetting = new DWORD[ iRate ];

	DWORD RoundedMS                 =   1000 / iRate;
	DWORD DroppedTotalMS            =   1000 - (RoundedMS * iRate);
	float DroppedPerFrameMS         =   (float)(DroppedTotalMS) / iRate;
	float AccumulatedDroppedMS      =   0.0;

	for (int i = 0; i < iRate; ++i)
	{
		if (AccumulatedDroppedMS >= 1.0)
		{
			g_pdwSyncRateSetting[i] = RoundedMS + 1;
			AccumulatedDroppedMS -= 1.0;
			--DroppedTotalMS;
		}
		else
		{
			g_pdwSyncRateSetting[i] = RoundedMS;
		}
		AccumulatedDroppedMS += DroppedPerFrameMS;
	}

	// Any further dropped milliseconds, just use them against entries that haven't
	// already had them added previously until they are all used up.
	// This needed because of float (in)accuracy.
	for (int i = 0; i < iRate && DroppedTotalMS > 0; ++i)
	{
		if (g_pdwSyncRateSetting[i] == RoundedMS)
		{
			++g_pdwSyncRateSetting[i];
			--DroppedTotalMS;
		}
	}
}

DWORD GetNextSyncDelay()
{
	// If there is no table, then run return a 'no delay'
	if (g_dwSyncRateSettingSize == 0)
		return 0;

	// Advance the index, reset to start if gone beyond the end of the table
	++g_dwSyncRateCurrent;
	if (g_dwSyncRateCurrent >= g_dwSyncRateSettingSize)
		g_dwSyncRateCurrent = 0;

	// Return the current delay
	return g_pdwSyncRateSetting[ g_dwSyncRateCurrent ];
}

DARKSDK void DrawToBack(void)
{
	g_bDrawAutoStuffFirst = false;
}

DARKSDK void DrawToFront(void)
{
	g_bDrawAutoStuffFirst = true;
}

DARKSDK void DrawToCamera(void)
{
	g_bDrawEntirelyToCamera = true;
}

DARKSDK void DrawToScreen(void)
{
	g_bDrawEntirelyToCamera = false;
}

DARKSDK void DrawSpritesFirst(void)
{
	g_bDrawSpritesFirst=true;
}

DARKSDK void DrawSpritesLast(void)
{
	g_bDrawSpritesFirst=false;
}

DARKSDK void SaveArray(LPSTR pFilename, DWORD dwAllocation)
{
	// Temp vars
	DWORD written;

	// If Array Exists
	if(dwAllocation)
	{
		// Header Info
		DWORD dwHeaderSizeInBytes = HEADERSIZEINBYTES;
		DWORD dwSizeOfArray = *((DWORD*)dwAllocation-4);
		DWORD dwElementSize = *((DWORD*)dwAllocation-3);
		DWORD dwExistingElementType = *((DWORD*)dwAllocation-2);
		DWORD dwTableSizeInBytes = dwSizeOfArray * 4;

		// Can only save pure types
		if(dwExistingElementType<9)
		{
			// Create File for array
			HANDLE hFile = GG_CreateFile(pFilename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if(hFile!=INVALID_HANDLE_VALUE)
			{
				// String arrays can be text file dumps
				if(dwExistingElementType==2)
				{
					// Save Out Array (of x size)
					DWORD dwDataPointer=dwAllocation+dwTableSizeInBytes;
					for(DWORD n=0; n<dwSizeOfArray; n++)
					{
						DWORD* pEntry = *((DWORD**)dwAllocation+n);
						if(pEntry)
						{
							// String data
							DWORD dwStringSize=0;
							LPSTR pStr = (LPSTR)*pEntry;
							if(*pEntry) dwStringSize = strlen(pStr);
							if(dwStringSize>0) WriteFile(hFile, pStr, dwStringSize, &written, FALSE);

							// carriage return
							char CR[2]; CR[0]=13; CR[1]=10;
							WriteFile(hFile, &CR, 2, &written, FALSE);
						}
					}
				}
				else
				{
					// Write Type of Array (element type 2=string)
					WriteFile(hFile, &dwExistingElementType, 4, &written, FALSE);

					// Write Size of Array (elements)
					WriteFile(hFile, &dwSizeOfArray, 4, &written, FALSE);

					// Save Out Array (of x size)
					DWORD dwDataPointer=dwAllocation+dwTableSizeInBytes;
					for(DWORD n=0; n<dwSizeOfArray; n++)
					{
						DWORD* pEntry = *((DWORD**)dwAllocation+n);
						if(pEntry)
						{
							// Write Index + Datablock
							int indexn=(int)n;
							WriteFile(hFile, &indexn, 4, &written, FALSE);

							// Value
							WriteFile(hFile, pEntry, dwElementSize, &written, FALSE);
						}
					}

					// Write Index of -1 to end
					int endn=-1;
					WriteFile(hFile, &endn, 4, &written, FALSE);
				}

				// Close file
				CloseHandle(hFile);
			}
			else
			{
				// runtime - could not create array file
				char pErrStr[1024];
				sprintf ( pErrStr, "Failed to CreateFile with: %s", pFilename );
				Message ( 0, pErrStr, "" );
				RunTimeError(RUNTIMEERROR_INVALIDFILE);
			}
		}
		else
		{
			//runtime not right type
			RunTimeError(RUNTIMEERROR_ARRAYTYPEINVALID);
		}
	}
}

DARKSDK void LoadArrayCore(LPSTR pFilename, DWORD dwAllocation)
{
	// Temp vars
	DWORD readen;

	// If Array Exists
	if(dwAllocation)
	{
		// Header Info
		DWORD dwHeaderSizeInBytes = HEADERSIZEINBYTES;
		DWORD dwExistingSizeOfArray = *((DWORD*)dwAllocation-4);
		DWORD dwElementSize = *((DWORD*)dwAllocation-3);
		DWORD dwExistingElementType = *((DWORD*)dwAllocation-2);
		DWORD dwTableSizeInBytes = dwExistingSizeOfArray * 4;

		// Can only save pure types
		if(dwExistingElementType<9)
		{
			// Load File for array
			HANDLE hFile = GG_CreateFile(pFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if(hFile!=INVALID_HANDLE_VALUE)
			{
				// If array string, load in as string table
				if(dwExistingElementType==2)
				{
					// Read Type of Array (element type 2=string)
					DWORD dwDataSize=GetFileSize(hFile, 0);
					LPSTR pData=new char[dwDataSize+2];
					ReadFile(hFile, pData, dwDataSize, &readen, FALSE);
					pData[dwDataSize]=0;
					pData[dwDataSize+1]=0;

					// Scan all lines into array
					int arrindex = 0;
					LPSTR pPtr = pData;
					LPSTR pStart = pPtr;
					LPSTR pPtrEnd = pData + dwDataSize;
					while(pPtr<=pPtrEnd && arrindex<(int)dwExistingSizeOfArray)
					{
						if( (*(pPtr+0)==13 && *(pPtr+1)==10) || *(pPtr+0)==0 )
						{
							DWORD* pEntry = *((DWORD**)dwAllocation+arrindex);
							if(pEntry)
							{
								// Free any existing string
								if(*pEntry) delete[] (LPSTR)(*pEntry);

								// Make string
								LPSTR pNewStr = NULL;
								DWORD dwStringSize=pPtr-pStart;
								pNewStr = new char[dwStringSize+1];
								memcpy(pNewStr, pStart, dwStringSize);
								pNewStr[dwStringSize]=0;

								// New string
								*pEntry = (DWORD)pNewStr;
								arrindex++;
							}

							// Next line
							pStart = pPtr+2;
							pPtr++;
						}
						pPtr++;
					}

					// Free data
					delete[] pData;
				}
				else
				{
					// Read Type of Array (element type 2=string)
					DWORD dwElementType=0;
					ReadFile(hFile, &dwElementType, 4, &readen, FALSE);

					// Read Size of Array (elements)
					DWORD dwSizeOfArray=0;
					ReadFile(hFile, &dwSizeOfArray, 4, &readen, FALSE);

					// Verify corect array loaded into
					if(dwElementType==dwExistingElementType && dwSizeOfArray==dwExistingSizeOfArray)
					{
						// Clear Array of old data
						DWORD dwDataPointer=dwAllocation+dwTableSizeInBytes;
						DWORD dwDataBlockSizeInBytes = dwSizeOfArray * dwElementSize;
						ZeroMemory((LPSTR)dwDataPointer, dwDataBlockSizeInBytes);

						// Load In Array (of x size)
						int arrindex = 0;
						ReadFile(hFile, &arrindex, 4, &readen, FALSE);
						while(arrindex!=-1)
						{
							DWORD* pEntry = *((DWORD**)dwAllocation+arrindex);
							if(pEntry)
							{
								// Value
								ReadFile(hFile, pEntry, dwElementSize, &readen, FALSE);
							}

							// Read ext index
							ReadFile(hFile, &arrindex, 4, &readen, FALSE);
						}
					}
					else
					{
						// runtime not same aray
					}
				}
			
				// Close file
				CloseHandle(hFile);
			}
			else
			{
				// runtime - could not read array file
				RunTimeError(RUNTIMEERROR_FILENOTEXIST,pFilename);
			}
		}
		else
		{
			//runtime not right type
			RunTimeError(RUNTIMEERROR_ARRAYTYPEINVALID);
		}
	}
}

DARKSDK void LoadArray( LPSTR szFilename, DWORD dwAllocation )
{
	// Uses actual or virtual file..
	char VirtualFilename[_MAX_PATH];
	strcpy(VirtualFilename, szFilename);
	//g_pGlob->UpdateFilenameFromVirtualTable( VirtualFilename);

	CheckForWorkshopFile ( VirtualFilename );

	// Decrypt and use media, re-encrypt
	g_pGlob->Decrypt( VirtualFilename );
	LoadArrayCore ( VirtualFilename, dwAllocation );
	g_pGlob->Encrypt( VirtualFilename );
}

//
// DX Detect Check (from globstruct filled in DarkEXE)
//

DARKSDK DWORD GetDXVer$(DWORD dwDestStr)
{
	char buf[256];
	buf[0] = '\0';
	return reinterpret_cast<DWORD>(dbReturnString(reinterpret_cast<char *>(dwDestStr), buf));
}

//
// Suspend App - used when multiple apps running, want to shut one down
//

DARKSDK void AlwaysActiveOff ( void )
{
	// Will shut down all 3D, sound and music processing (plus any secondary monitoring)
	// Will keep input and general program execution naturally
	g_bAlwaysActiveOff = true;
	g_bAlwaysActiveOneOff = false;
}

DARKSDK void AlwaysActiveOn ( void )
{
	// Restores systems previously shutdown with AlwaysActiveOff
	g_bAlwaysActiveOff = false;
}

DARKSDK void EarlyEnd ( void )
{
	// Report an error
	MessageBox ( NULL, "You have hit a FUNCTION declaration mid-program!", "Early Exit Error", MB_OK );
}

DARKSDK void SyncSleep ( int iFlag )
{
	// controls process friendly flag
	if ( iFlag==1 ) 
		g_bProcessorFriendly = true;
	else
		g_bProcessorFriendly = false;
}

DARKSDK void SyncMask ( DWORD dwMask )
{
	// copy to master sync mask
	g_dwSyncMask = dwMask;
}

DARKSDK void SyncMaskOverride ( DWORD dwMask )
{
	// used to override ALL camera rendering (for a loading sequence)
	g_dwSyncMaskOverride = dwMask;
}

DARKSDK DWORD GetArrayType(DWORD dwArrayPtr)
{
	// return array size
	if(dwArrayPtr) 
	{
		DWORD dwTypeIndex = (*((DWORD*)dwArrayPtr-2));
		return dwTypeIndex;
	}
	else
		return 0;
}

LPSTR GetTypePatternCore ( LPSTR dwTypeName, DWORD dwTypeIndex )
{
	// U73 - 210309 - if basic string, return simple STRING pattern
	if ( dwTypeIndex==2 )
	{
		LPSTR pSimplePattern = new char[2];
		strcpy ( pSimplePattern, "S" );
		return pSimplePattern;
	}

	// U73 - 210309 - if no structures, exit now as rest is structure type stuff only
	if ( g_dwStructPatternQty==0 )
		return NULL;

	// look for type that matches name
	DWORD dwPatternDataBeginsAt = 0;
	if ( g_pStructPatternsPtr )
	{
		if ( dwTypeName )
		{
			LPSTR pFindName = new char[strlen((LPSTR)dwTypeName)+2];
			strcpy ( pFindName, (LPSTR)dwTypeName );
			strcat ( pFindName, ":" );
			DWORD dwFindLength = strlen ( pFindName );
			for ( DWORD dwI=0; dwI<g_dwStructPatternQty-dwFindLength; dwI++ )
			{
				if ( strnicmp ( g_pStructPatternsPtr+dwI, pFindName, dwFindLength )==NULL )
				{
					dwPatternDataBeginsAt = dwI+dwFindLength;
					break;
				}
			}
			delete[] pFindName;
		}
		if ( dwTypeIndex>0 )
		{
			LPSTR pFindName = new char[g_dwStructPatternQty+1];
			wsprintf ( pFindName, ":%d:", dwTypeIndex );
			DWORD dwFindLength = strlen ( pFindName );
			for ( DWORD dwI=0; dwI<g_dwStructPatternQty-dwFindLength; dwI++ )
			{
				if ( strnicmp ( g_pStructPatternsPtr+dwI, pFindName, dwFindLength )==NULL )
				{
					dwPatternDataBeginsAt = dwI+dwFindLength;
					break;
				}
			}
			delete[] pFindName;
		}
	}

	// copy pattern to return string, or null
	LPSTR lpNewStr = new char[(strlen(g_pStructPatternsPtr)-dwPatternDataBeginsAt)+1];
	if ( dwPatternDataBeginsAt > 0 )
	{
		// get type index, then go to get pattern
		if ( dwTypeName )
		{
			LPSTR lpNum = new char[(strlen(g_pStructPatternsPtr)-dwPatternDataBeginsAt)+1];
			LPSTR pSourceStr = g_pStructPatternsPtr + dwPatternDataBeginsAt;
			strcpy ( lpNum, pSourceStr );
			DWORD dwI = 0;
			for (; dwI<strlen(pSourceStr); dwI++ )
			{
				if ( lpNum[dwI]==':' )
				{
					lpNum[dwI]=0;
					break;
				}
			}
			delete[] lpNum;
			dwPatternDataBeginsAt += dwI + 1;
		}

		// get pattern, then cut off at : colon
		LPSTR pSourceStr = g_pStructPatternsPtr + dwPatternDataBeginsAt;
		strcpy ( lpNewStr, pSourceStr );
		for ( DWORD dwI=0; dwI<strlen(pSourceStr); dwI++ )
		{
			if ( lpNewStr[dwI]==':' )
			{
				lpNewStr[dwI]=0;
				break;
			}
		}
	}
	else
		strcpy ( lpNewStr, "" );

	// return pattern from type found
	return lpNewStr;
}

DARKSDK DWORD GetTypePattern$(DWORD dwDestStr,DWORD dwTypeName,DWORD dwTypeIndex)
{
	DWORD r;

	// determine if type name string passed in has contents
	LPSTR pTypeName = NULL;
	if ( dwTypeName )
		if ( strlen ( (LPSTR)dwTypeName ) > 0 )
			pTypeName = (LPSTR)dwTypeName;

	// get pattern from type name
	LPSTR lpNewStr = GetTypePatternCore( pTypeName, dwTypeIndex );

	r = reinterpret_cast<DWORD>(dbReturnString(reinterpret_cast<char *>(dwDestStr), lpNewStr));
	delete [] lpNewStr;

	return r;
}

// Get/Set data pointers
DARKSDK void GetDataPointers(LPSTR* Start, LPSTR* End, LPSTR* Current)
{
	if (Start)      *Start      = g_pDataLabelStart;
	if (End)        *End        = g_pDataLabelEnd;
	if (Current)    *Current    = g_pDataLabelPtr;
}

DARKSDK void SetDataPointer(LPSTR Current)
{
	if (Current < g_pDataLabelStart)
		Current = g_pDataLabelStart;
	if (Current > g_pDataLabelEnd)
		Current = g_pDataLabelEnd;
	g_pDataLabelPtr = Current;
}


#ifdef INCLUDEVRAM
#include <dxgi1_4.h>
IDXGIFactory4* pFactory = nullptr;
IDXGIAdapter3* adapter = nullptr;
float GetVramUsage(void)
{
	extern uint32_t g_iActiveAdapterNumber;
	if(!pFactory)
		CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&pFactory);
	if(!adapter)
		pFactory->EnumAdapters(0, reinterpret_cast<IDXGIAdapter**>(&adapter));
	DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
	adapter->QueryVideoMemoryInfo(g_iActiveAdapterNumber, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo);
	return (float)videoMemoryInfo.CurrentUsage / 1024.0f / 1024.0f;
}

float GetTotalVramUsage(void)
{
	extern uint32_t g_iActiveAdapterNumber;
	if (!pFactory)
		CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&pFactory);
	if (!adapter)
		pFactory->EnumAdapters(0, reinterpret_cast<IDXGIAdapter**>(&adapter));
	DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo = {};
	adapter->QueryVideoMemoryInfo(g_iActiveAdapterNumber, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo);
	float TotalVRam = (float)videoMemoryInfo.CurrentUsage / 1024.0f / 1024.0f;

	DXGI_QUERY_VIDEO_MEMORY_INFO nonLocalVideoMemoryInfo = {};
	HRESULT hrNonLocal = adapter->QueryVideoMemoryInfo(g_iActiveAdapterNumber, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonLocalVideoMemoryInfo);
	TotalVRam += (float)nonLocalVideoMemoryInfo.CurrentUsage / 1024.0f / 1024.0f;

	//usageInfo.nonLocalUsedMB = (float)nonLocalVideoMemoryInfo.CurrentUsage / 1024.0f / 1024.0f;
	//usageInfo.nonLocalBudgetMB = (float)nonLocalVideoMemoryInfo.Budget / 1024.0f / 1024.0f;
	//usageInfo.nonLocalSwappedOutMB = (float)(nonLocalVideoMemoryInfo.Budget - nonLocalVideoMemoryInfo.CurrentUsage) / 1024.0f / 1024.0f;
	//if (usageInfo.nonLocalSwappedOutMB < 0) usageInfo.nonLocalSwappedOutMB = 0; // Ensure non-negative

	return(TotalVRam);
}
#endif
