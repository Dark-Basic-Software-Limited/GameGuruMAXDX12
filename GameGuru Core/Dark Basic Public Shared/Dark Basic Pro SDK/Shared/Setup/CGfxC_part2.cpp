DARKSDK void WindowToFront(LPSTR pTitleString)
{
	LPSTR lpstrTitle = pTitleString;
	if ( strnicmp ( lpstrTitle, "__topmost__", 11 )==NULL )
	{
		// U75 - 051109 - special name to send window to topmost
		// makes sense in test game for FPS Creator as mouse pointer is hijacked anyway!
		SetForegroundWindow(m_hWnd);
		SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		// U&5 - 070510 - Force a click on the screen to give topmost window the focus
		MOUSEINPUT Mouse;
		memset ( &Mouse, 0, sizeof(MOUSEINPUT) );
		Mouse.dx=320;
		Mouse.dy=240;
		INPUT Input;
		memset ( &Input, 0, sizeof(INPUT) );
		Input.type = INPUT_MOUSE;
		Input.mi = Mouse;
		Mouse.dwFlags = MOUSEEVENTF_LEFTDOWN;
		SendInput ( 1, &Input, sizeof(INPUT) );
		Mouse.dwFlags = MOUSEEVENTF_LEFTUP;
		SendInput ( 1, &Input, sizeof(INPUT) );
		Mouse.dwFlags = MOUSEEVENTF_LEFTDOWN;
		SendInput ( 1, &Input, sizeof(INPUT) );
		MOUSEINPUT Mouse2;
		memset ( &Mouse2, 0, sizeof(MOUSEINPUT) );
		Mouse2.dx=320;
		Mouse2.dy=240;
		INPUT Input2;
		memset ( &Input2, 0, sizeof(INPUT) );
		Input2.type = INPUT_MOUSE;
		Input2.mi = Mouse2;
		Mouse2.dwFlags = MOUSEEVENTF_LEFTUP;
		SendInput ( 1, &Input2, sizeof(INPUT) );
	}
	else
	{
		HWND hWnd = FindWindow ( NULL, lpstrTitle );
		SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		SetForegroundWindow(hWnd);
	}
}

DARKSDK void WindowToBack(LPSTR pTitleString)
{
	HWND hWnd = FindWindow ( NULL, pTitleString );
	SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

HWND GetWindowHandle ( void )
{
	return m_hWnd;
}

//
// Backbuffer Command Functions
//

DARKSDK void LockBackbuffer(void)
{
	#ifdef DX11
	#else
	if(g_pBackBuffer==NULL)
	{
		HRESULT hRes = m_pD3D->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &g_pBackBuffer);
		if(g_pBackBuffer)
		{
			// get format of backbuffer
			D3DSURFACE_DESC ddsd;
			hRes = g_pBackBuffer->GetDesc(&ddsd);

			GGLOCKED_RECT d3dlockedrect;
			hRes = g_pBackBuffer->LockRect(&d3dlockedrect, NULL, D3DLOCK_NOSYSLOCK);
			if(hRes==GG_OK)
			{
				int BitDepth=GetBitDepthFromFormat(ddsd.Format);
				g_dwSurfacePtr=(DWORD)d3dlockedrect.pBits;
				g_dwSurfaceWidth=ddsd.Width;
				g_dwSurfaceHeight=ddsd.Height;
				g_dwSurfaceDepth=BitDepth;
				g_dwSurfacePitch=d3dlockedrect.Pitch;
			}
		}
	}
	#endif
}

DARKSDK void UnlockBackbuffer(void)
{
	#ifdef DX11
	#else
	if(g_pBackBuffer)
	{
		// Free locked surface
		g_pBackBuffer->UnlockRect();
		g_dwSurfacePtr=NULL;
		g_dwSurfaceWidth=0;
		g_dwSurfaceHeight=0;
		g_dwSurfaceDepth=0;
		g_dwSurfacePitch=0;

		// Release backbuffer
		SAFE_RELEASE(g_pBackBuffer);
	}
	#endif
}

DARKSDK DWORD GetBackbufferPtr()
{
	DWORD pReturnValue=-1;
	if(g_dwSurfacePtr)
		pReturnValue = g_dwSurfacePtr;

	return pReturnValue;
}

DARKSDK int GetBackbufferWidth()
{
	int pReturnValue=-1;
	if(g_dwSurfacePtr)
		pReturnValue = g_dwSurfaceWidth;

	return pReturnValue;
}

DARKSDK int GetBackbufferHeight()
{
	int pReturnValue=-1;
	if(g_dwSurfacePtr)
		pReturnValue = g_dwSurfaceHeight;

	return pReturnValue;
}

DARKSDK int GetBackbufferDepth()
{
	int pReturnValue=-1;
	if(g_dwSurfacePtr)
		pReturnValue = g_dwSurfaceDepth;

	return pReturnValue;
}

DARKSDK int GetBackbufferPitch()
{
	int pReturnValue=-1;
	if(g_dwSurfacePtr)
		pReturnValue = g_dwSurfacePitch;

	return pReturnValue;
}

// lee - 130108 - added for X10 compat.
DARKSDK void				SetNvPerfHUD						( int iUsePerfHUD )
{
	if ( iUsePerfHUD==1 )
		m_bNVPERFHUD = true;
	else
		m_bNVPERFHUD = false;
}

DARKSDK void				ForceAdapterOrdinal ( int iForceOrdinal )
{
	m_iForceAdapterOrdinal = iForceOrdinal;
}

DARKSDK void				ForceAdapterD3D11ONLY ( int iForceD3D11ONLY )
{
	m_iForceAdapterD3D11ONLY = iForceD3D11ONLY;
}

DARKSDK void				SetCaptureName						( DWORD pFilename )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void				SetCaptureCodec						( DWORD pFilename )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void				SetCaptureSettings					( int iCompressed, int iFPS, int iWidth, int iHeight, int iThreaded, int iWait )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void				SetCaptureMode						( int iRecordVideo )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void				SaveScreenshot						( DWORD pFilename )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void				StartPlayback						( DWORD pFilename, float fSpeed )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void				UpdatePlayback						( void )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void				StopPlayback						( void )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK int					PlaybackPlaying						( void )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
	return 0;
}

DARKSDK int					GetSessionTerminate					( void )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
	return 0;
}

bool g_bGrabTrackingDataOncePerTrio = false;
float g_fYaw, g_fPitch, g_fRoll;

DARKSDK int ResetLeftEye ( void )
{
	// U75 - 150310 - extra command to detect VR920 AND reset to left eye (otherwise user cross-eyed)
	return 0;
}

DARKSDK void ResetTracking ( void )
{
}

DARKSDK LPSTR GetDXName ( void )
{
	#ifdef DX11
	 wsprintf ( m_pWorkString, "DX11" );
	#else
	 wsprintf ( m_pWorkString, "Classic" );
	#endif
	LPSTR pReturnString=GetReturnStringFromWorkString();
	return pReturnString;
}

DARKSDK LPSTR GetDirectRefreshRate ( void )
{
	// Create and return string
	#ifdef DX11
	wsprintf ( m_pWorkString, "DirectX 11" );
	#else
	wsprintf ( m_pWorkString, "%d", (int)m_WindowsD3DMODE.RefreshRate );
	#endif
	LPSTR pReturnString=GetReturnStringFromWorkString();
	return pReturnString;
}

//Dave - new routines to work with true pixels in client windows
DARKSDK void SetChildWindowTruePixel ( int mode )
{
	g_dwChildWindowTruePixel = mode;
}

DARKSDK int GetChildWindowX()
{
	RECT src;
	GetClientRect ( m_hWnd , &src );
	return (int)src.left;
}

DARKSDK int GetChildWindowY()
{
	RECT src;
	GetClientRect ( m_hWnd , &src );
	return (int)src.top;
}

DARKSDK int GetChildWindowWidth(int iDPIAware)
{
#ifdef ENABLEIMGUI
#ifndef USEOLDGUI
#ifdef USERENDERTARGET
	extern bool bImGuiInTestGame;
	extern bool bImGuiInitDone;
	if (bImGuiInitDone && !bImGuiInTestGame) {

		if (iDPIAware == -1) {
			RECT src;
			GetClientRect(m_hWnd, &src);
			return (int)src.right;
		}

		//PE: Calculate the postion on our zoomed render target.
		extern ImVec2 OldrenderTargetSize;

		float fPreviewWidth = (int) OldrenderTargetSize.x; // -iBorderSize;
		float fPreviewHeight = (int)OldrenderTargetSize.y; // -iBorderSize;

		int iImgW = GetDisplayWidth();
		int iImgH = GetDisplayHeight();
		float fRatio;

		fRatio = fPreviewHeight / iImgH;
		if (iImgW*fRatio < fPreviewWidth)
			fRatio = fPreviewWidth / iImgW;

		//PE: Take our ratio and invert it for zoom factor.
		return fPreviewWidth * (1.0f/ fRatio);
	}
#endif
#endif
#endif

	RECT src;
	GetClientRect ( m_hWnd , &src );
	int iWidth=(int)src.right;
	if ( iDPIAware==1 )
	{
		HDC hdc = GetDC(NULL);
		int LogicalScreenHeight = GetDeviceCaps(hdc, (int)VERTRES);
		int PhysicalScreenHeight = GetDeviceCaps(hdc, (int)DESKTOPVERTRES); 
		float ScreenScalingFactor = (float)PhysicalScreenHeight / (float)LogicalScreenHeight;
		iWidth = (int)g_pGlob->dwWindowWidth;
		if ( ScreenScalingFactor==1.0f || ScreenScalingFactor==1.25f )
		{
			// 100% or 125% font size
			iWidth = (int)src.right; // ALL
		}
		if ( ScreenScalingFactor==1.5f )
		{
			// 150% font size
			if ( g_pGlob->dwWindowWidth==1280 ) iWidth = (int)src.right; // 1920x900
			if ( g_pGlob->dwWindowWidth==2560 ) iWidth = (int)src.right; // 3840x2160
		}
		ReleaseDC(NULL, hdc);
	}
	return iWidth;
}

DARKSDK int GetChildWindowHeight(int iDPIAware)
{
#ifdef ENABLEIMGUI
#ifndef USEOLDGUI
#ifdef USERENDERTARGET
	extern bool bImGuiInTestGame;
	extern bool bImGuiInitDone;
	if (bImGuiInitDone && !bImGuiInTestGame) {
		if (iDPIAware == -1) {
			RECT src;
			GetClientRect(m_hWnd, &src);
			return (int)src.bottom;
		}

		//PE: Calculate the postion on our zoomed render target.
		extern ImVec2 OldrenderTargetSize;

		float fPreviewWidth = OldrenderTargetSize.x; // -iBorderSize;
		float fPreviewHeight = OldrenderTargetSize.y; // -iBorderSize;

		int iImgW = GetDisplayWidth();
		int iImgH = GetDisplayHeight();
		
		float fRatio;
		fRatio = fPreviewHeight / (float) iImgH;
		if ((float)iImgW*fRatio < fPreviewWidth)
			fRatio = fPreviewWidth / (float) iImgW;

		//float fCenterY = (fPreviewHeight - iImgH*fRatio) * 0.5;

		extern int ImGuiStatusBar_Size;
		//PE: Take our ratio and invert it for zoom factor.
		if (iDPIAware == -2) {
			return (fPreviewHeight * (1.0f / fRatio) );
		}
		return (fPreviewHeight * (1.0f / fRatio) - (ImGuiStatusBar_Size * (1.0f / fRatio) ));

	}
#endif
#endif
#endif

	RECT src;
	GetClientRect ( m_hWnd , &src );
	int iHeight=(int)src.bottom;
	if ( iDPIAware==1 )
	{
		HDC hdc = GetDC(NULL);
		int LogicalScreenHeight = GetDeviceCaps(hdc, (int)VERTRES);
		int PhysicalScreenHeight = GetDeviceCaps(hdc, (int)DESKTOPVERTRES); 
		float ScreenScalingFactor = (float)PhysicalScreenHeight / (float)LogicalScreenHeight;
		iHeight = (int)g_pGlob->dwWindowHeight;
		if ( ScreenScalingFactor==1.0f || ScreenScalingFactor==1.25f )
		{
			// 100% or 125% font size
			iHeight = (int)src.bottom; // ALL
		}
		if ( ScreenScalingFactor==1.5f )
		{
			// 150% font size
			if ( g_pGlob->dwWindowWidth==1280 ) iHeight = (int)src.bottom; // 1920x900
			if ( g_pGlob->dwWindowWidth==2560 ) iHeight = (int)src.bottom; // 3840x2160
		}
		ReleaseDC(NULL, hdc);
	}
	return iHeight;
}
DARKSDK int GetChildWindowWidth()
{
	return GetChildWindowWidth(0);
}
DARKSDK int GetChildWindowHeight()
{
	return GetChildWindowHeight(0);
}

#ifdef DX11
DARKSDK ID3DX11Effect* SETUPLoadShader ( LPSTR szFile, LPSTR szBlobFile, int iShaderIndex )
{
	#ifdef WICKEDENGINE
	return NULL;
	#else
	// check if this index already has a shader inside it (zero just loads the shader to nowhere to create blob)
	if ( iShaderIndex > 0 )
	{
		if ( g_sShaders[iShaderIndex].pEffect )
		{
			// return existing shader (likely when LoadEffect command used)
			return g_sShaders[iShaderIndex].pEffect;
		}
	}

	// get wchar of blobfilename
	size_t origsize = strlen(szBlobFile) + 1;
	const size_t newsize = 1024;
	size_t convertedChars = 0;
	wchar_t wcstringBlobFilename[newsize];
	mbstowcs_s(&convertedChars, wcstringBlobFilename, origsize, szBlobFile, _TRUNCATE);

	// if special flag, delete any blob file (editing shaders mode)
	if ( gbAlwaysIgnoreShaderBlobFile == true || iShaderIndex == 0 )
		if ( DoesFileExist ( szBlobFile ) == true ) 
			DeleteFile ( szBlobFile );

	// compile if blob file does not exist
	bool bCompiledShader = false;
	ID3DBlob* pErrorBlob = NULL;
	HRESULT hRes = D3DReadFileToBlob ( wcstringBlobFilename, &g_sShaders[iShaderIndex].pBlob);
	if ( hRes != S_OK )
	{
		char szRealFilename[ MAX_PATH ];
		strcpy_s( szRealFilename, MAX_PATH, szFile );
		GG_GetRealPath( szRealFilename, 0 );
		// proceed to compile shader blob
		D3DX11CompileFromFile(szRealFilename, NULL, NULL, NULL, "fx_5_0", D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY, 0, 0, &g_sShaders[iShaderIndex].pBlob, &pErrorBlob, NULL);
		bCompiledShader = true;
	}
	if ( pErrorBlob != NULL )
	{
		if ( g_iShowDetailedShaderErrorMessage == 0 && iShaderIndex > 0 )
		{
			char pErrorDesc[1024];
			sprintf ( pErrorDesc, "Failed to load the shader '%s'. All GameGuru shaders need to be HLSL level 5.0 or above.", szFile ); 
			MessageBox ( NULL, pErrorDesc, "DirectX", MB_OK );
		}
		else
		{
			LPSTR szError = (LPSTR)pErrorBlob->GetBufferPointer ( );
			MessageBox ( NULL, szError, "D3DX11CompileFromFile failure", MB_OK );
		}
		SAFE_RELEASE ( pErrorBlob );
		strcpy ( g_sShaders[iShaderIndex].pName, "" );
		return NULL;
	}
	else
	{
		if ( bCompiledShader == true ) 
		{
			D3DWriteBlobToFile ( g_sShaders[iShaderIndex].pBlob, wcstringBlobFilename, FALSE );
		}
	}

	// only if loading shader for real
	if ( iShaderIndex > 0 )
	{
		ID3DBlob* pFXBlob = g_sShaders[iShaderIndex].pBlob;
		if ( pFXBlob )
		{
			#ifdef WICKEDENGINE
			// Does not use DX11 Effects (uses Wicked Shaders)
			#else
			D3DX11CreateEffectFromMemory(pFXBlob->GetBufferPointer(), pFXBlob->GetBufferSize(), 0, m_pD3D, &g_sShaders[iShaderIndex].pEffect);
			#endif
		}
		else
		{
			MessageBox ( NULL, "Effect file not found!", "D3DX11CreateEffectFromMemory failure", MB_OK );
			strcpy ( g_sShaders[iShaderIndex].pName, "" );
			return NULL;
		}
	}
	else
	{
		// iShaderIndex of zero is just to create a new shader blob file
		g_sShaders[iShaderIndex].pEffect = NULL;
	}

	// copy name into record
	strcpy ( g_sShaders[iShaderIndex].pName, szFile );

	// return effect ptr for when used by LoadEffect command
	return g_sShaders[iShaderIndex].pEffect;
	#endif
}
DARKSDK bool SETUPFreeShader ( int iShaderIndex )
{
	#ifdef WICKEDENGINE
	return true;
	#else
	if ( g_sShaders[iShaderIndex].pEffect )
	{
		SAFE_RELEASE ( g_sShaders[iShaderIndex].pEffect ); 
		SAFE_RELEASE ( g_sShaders[iShaderIndex].pBlob );
		return true;
	}
	else
		return false;
	#endif
}
#endif

DARKSDK void SETUPLoadAllCoreShadersFIRST ( int iShowDetailedShaderErrorMessage )
{
	#ifdef WICKEDENGINE
	// uses different shaders altogether!
	#else
	// For DirectX11, all core shaders loaded early for later referencing
	#ifdef DX11
	// Set Error mode
	g_iShowDetailedShaderErrorMessage = iShowDetailedShaderErrorMessage;
	// Clear shader globals
	for ( int n = 0; n < SHADERSARRAYMAX; n++ ) g_sShaders[n] = { NULL, NULL, NULL };
	// Preload common shaders (for cases where inputlayout needs creating 'prior' to shader being loaded in old system)
	SETUPLoadShader ( "effectbank\\reloaded\\sprite_basic.fx", "effectbank\\reloaded\\sprite_basic.blob", SHADERSSPRITEBASIC );
	#endif
	#endif
}

DARKSDK void SETUPLoadAllCoreShadersREST ( int iShowDetailedShaderErrorMessage, int iPBRMode )
{
	#ifdef WICKEDENGINE
	// uses different shaders altogether!
	#else
	// For DirectX11, all core shaders loaded early for later referencing
	#ifdef DX11
	// Preload common shaders (for cases where inputlayout needs creating 'prior' to shader being loaded in old system)
	if ( iPBRMode == 1 )
		SETUPLoadShader ( "effectbank\\reloaded\\apbr_terrain.fx", "effectbank\\reloaded\\apbr_terrain.blob", SHADERSTERRAINBASIC );
	else
		SETUPLoadShader ( "effectbank\\reloaded\\terrain_basic.fx", "effectbank\\reloaded\\terrain_basic.blob", SHADERSTERRAINBASIC );
	SETUPLoadShader ( "effectbank\\reloaded\\static_basic.fx", "effectbank\\reloaded\\static_basic.blob", SHADERSSTATICBASIC );
	SETUPLoadShader ( "effectbank\\reloaded\\shadow_basic.fx", "effectbank\\reloaded\\shadow_basic.blob", SHADERSSHADOWBASIC );
	#endif
	#endif
}

DARKSDK void SETUPFreeAllCoreShaders ( void )
{
	#ifdef WICKEDENGINE
	#else
	// For DirectX11, all core shaders released here (perhaps redundant as do this end exit app)
	#ifdef DX11
	SETUPFreeShader ( SHADERSTERRAINBASIC );
	SETUPFreeShader ( SHADERSSPRITEBASIC );
	#endif
	#endif
}

void SetRenderAndDepthTarget ( LPGGRENDERTARGETVIEW render, LPGGDEPTHSTENCILVIEW depth )
{
	#ifdef DX11
	if ( render && m_pImmediateContext ) m_pImmediateContext->OMSetRenderTargets ( 1, &render, depth );
	g_pGlob->pCurrentRenderView = render;
	g_pGlob->pCurrentDepthView = depth;
	#endif
}
