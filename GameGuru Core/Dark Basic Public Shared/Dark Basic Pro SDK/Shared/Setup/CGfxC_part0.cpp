#define _CRT_SECURE_NO_DEPRECATE
#define _USING_V110_SDK71_

// GG flag header for preprocessor defines
#include "..\..\..\..\GameGuru\Include\preprocessor-flags.h"

#include "cGFXc.h"	// include GFX header
#include "..\..\DarkSDK\Setup\resource.h"
#include <delayimp.h>
#include <iostream> // can use STD:: with this include
#include <vector>
#include <DXGI.h>

#include "CTextC.h"
#include "CBasic2DC.h"
#include "CSpritesC.h"
#include "CImageC.h"
#include "CInputC.h"
#include "CSystemC.h"
#include "CMemblocks.h"
#include "CBitmapC.h"
#include "CAnimation.h"
#include "CObjectsC.h"
#include "CCameraC.h"
#include "CLightC.h"
#include "CParticleC.h"
#include "cVectorC.h"
#include "ConvX.h"
#include "CSoundC.h"
#include "BlitzTerrain.h"

#include "CFileC.h"

#ifdef ENABLEIMGUI
#include "..\..\..\..\GameGuru\Imgui\imgui.h"
#include "..\..\..\..\GameGuru\Imgui\imgui_impl_win32.h"
#include "..\..\..\..\GameGuru\Imgui\imgui_gg_dx11.h"
#endif

DBPRO_GLOBAL tagInfo*						m_pInfo;			// gfx card information
DBPRO_GLOBAL int							m_iAdapterCount;	// number of graphic cars
DBPRO_GLOBAL int							m_iAdapterUsed;		// graphics card being used by number
DBPRO_GLOBAL char							m_pAdapterName[_MAX_PATH];// graphica card being used by name 
DBPRO_GLOBAL bool							m_bDraw;			// flag to start a scene
DBPRO_GLOBAL bool							m_iDisplayChange;	// set when display has been switched
DBPRO_GLOBAL HWND							m_hWnd;				// handle to main VISUAL window(Editor window in IDE eventually)

DBPRO_GLOBAL bool							m_bOverrideHWND;	// should we override the window
DBPRO_GLOBAL bool							m_bResizeWindow;	// should we resize the window

DBPRO_GLOBAL GGPRESENT_PARAMETERS*			m_D3DPP				= NULL;
DBPRO_GLOBAL int							m_iWidth;			// width of display mode
DBPRO_GLOBAL int							m_iHeight;			// height of display mode
DBPRO_GLOBAL int							m_iChopWidth;		// chop width of display mode (for nonstandard setdisplaymode sizes)
DBPRO_GLOBAL int							m_iChopHeight;		// chop height of display mode
DBPRO_GLOBAL int							m_iDisplayType;		// windowed / fullscreen
DBPRO_GLOBAL int							m_iProcess;			// vertex processing mode
DBPRO_GLOBAL int							m_iDepth;			// depth of display
DBPRO_GLOBAL int							m_iBackBufferCount;	// back buffer count
DBPRO_GLOBAL bool							m_bLockable;		// lockable mode
DBPRO_GLOBAL bool							m_bVSync = true;
DBPRO_GLOBAL int							m_iVSyncInterval = 1;
DBPRO_GLOBAL bool							m_bVSyncInitialDefault = false;
DBPRO_GLOBAL int							m_iMultisamplingFactor = 0;
DBPRO_GLOBAL int							m_iMultimonitorMode = 0;
DBPRO_GLOBAL int							m_iModBackbufferWidth = 0;
DBPRO_GLOBAL int							m_iModBackbufferHeight = 0;
DBPRO_GLOBAL UINT							m_uAdapterChoice = GGADAPTER_DEFAULT;
DBPRO_GLOBAL bool							m_bNVPERFHUD = false;
DBPRO_GLOBAL int							m_iForceAdapterOrdinal = 0;
DBPRO_GLOBAL int							m_iForceAdapterD3D11ONLY = 0;

DBPRO_GLOBAL GGFORMAT						m_Depth;			// final back bufferformat
DBPRO_GLOBAL GGFORMAT						m_StencilDepth;		// final stencil buffer format
DBPRO_GLOBAL DWORD							m_dwFlags;			// flags
DBPRO_GLOBAL GGDISPLAYMODE					m_WindowsD3DMODE;
DBPRO_GLOBAL GGFORMAT						g_GGFORMAT;		// dx9 adapter format of choice

DBPRO_GLOBAL bool							m_bZBuffer;			// ZBuffer present
DBPRO_GLOBAL GGSWAPEFFECT					m_SwapMode;			// Whether in COPY or FLIP Mode

DBPRO_GLOBAL int							m_iWindowWidth;		// width of window
DBPRO_GLOBAL int							m_iWindowHeight;	// window height

DBPRO_GLOBAL LPGGSWAPCHAIN					m_pSwapChain [ MAX_SWAP_CHAINS ];
DBPRO_GLOBAL int							m_iSwapChainCount;

DBPRO_GLOBAL int							m_iGammaRed;
DBPRO_GLOBAL int							m_iGammaBlue;
DBPRO_GLOBAL int							m_iGammaGreen;

DBPRO_GLOBAL LPGGSURFACE					g_pBackBuffer = NULL;	//globals for locking backbuffer (commands)
DBPRO_GLOBAL LPGGRENDERTARGETVIEW			m_pRenderTargetView = NULL;
DBPRO_GLOBAL LPGGSURFACE					m_pDepthStencil = NULL;
DBPRO_GLOBAL LPGGDEPTHSTENCILVIEW			m_pDepthStencilView = NULL;
DBPRO_GLOBAL LPGGSHADERRESOURCEVIEW			m_pDepthStencilResourceView = NULL;

DBPRO_GLOBAL DWORD							g_dwSurfacePtr = 0;
DBPRO_GLOBAL DWORD							g_dwSurfaceWidth = 0;
DBPRO_GLOBAL DWORD							g_dwSurfaceHeight = 0;
DBPRO_GLOBAL DWORD							g_dwSurfaceDepth = 0;
DBPRO_GLOBAL DWORD							g_dwSurfacePitch = 0;

DBPRO_GLOBAL bool							gbScreenBecomeInvalid=false;	// globals for fine windows control (commands)
DBPRO_GLOBAL bool							gbWindowMode=false;
DBPRO_GLOBAL bool							gbWindowBorderActive=true;
DBPRO_GLOBAL DWORD							gWindowSizeX=0;
DBPRO_GLOBAL DWORD							gWindowSizeY=0;
DBPRO_GLOBAL DWORD							gWindowExtraX=0;
DBPRO_GLOBAL DWORD							gWindowExtraY=0;
DBPRO_GLOBAL DWORD							gWindowExtraXForOverlap=0;
DBPRO_GLOBAL DWORD							gWindowExtraYForOverlap=0;
DBPRO_GLOBAL DWORD							gWindowVisible=SW_SHOWDEFAULT;
DBPRO_GLOBAL char							gWindowName[256];
DBPRO_GLOBAL DWORD							gWindowStyle=WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU;
DBPRO_GLOBAL HICON							gOriginalIcon=NULL;
DBPRO_GLOBAL HICON							gWindowIconHandle=NULL;
DBPRO_GLOBAL bool							gbFirstInitOfDisplayOnly=true;

// to allow true pixel representation when rendering to a child window.
DBPRO_GLOBAL DWORD							g_dwChildWindowTruePixel = 0;

static bool									g_bCreateChecklistNow=false;
static DWORD								g_dwMaxStringSizeInEnum=0;
static char									m_pWorkString[_MAX_PATH];

DBPRO_GLOBAL bool							g_bValidFPS = true;
DBPRO_GLOBAL bool							g_bSceneBegun = false;
DBPRO_GLOBAL LPGGSURFACE					g_pGFXBackBufferRenderTarget = NULL;

extern LPGG									m_pDX;				// interface to D3D
extern LPGGDEVICE							m_pD3D;				// D3D device
extern GlobStruct*							g_pGlob;
///extern PTR_FuncCreateStr					g_pCreateDeleteStringFunction;
DBPRO_GLOBAL D3D_FEATURE_LEVEL				g_featureLevel;

DBPRO_GLOBAL HWND							g_OldHwnd						= NULL;
DBPRO_GLOBAL bool							g_bWindowOverride				= false;
DBPRO_GLOBAL char							g_szMainWindow  [ MAX_PATH ];
DBPRO_GLOBAL char							g_szChildWindow [ MAX_PATH ];

bool										gbAlwaysIgnoreShaderBlobFile = false;

#ifdef DX11
sShaderGlobalsType							g_sShaders [ SHADERSARRAYMAX ];
int											g_iShowDetailedShaderErrorMessage = 0;
#endif

bool										g_bNoSwapchainPresent = false;
bool										g_bNo2DRender = false;
bool										g_bNoTerrainRender = false;
bool										g_bDelayedShadows = true;
bool										g_bDelayedShadowsLaptop = false;
bool										g_bNoVSync = false;
float										maxApparentSize = 0.000008f; // 0.000002f;// 0.000007f; 0.000008; // 0.000002f;

#if DEBUG_MODE
	DBPRO_GLOBAL FILE*	m_fp;
#endif

// mike - 070207 - need a way of overriding the present mode to draw to custom
//			     - areas, the globals are placed in a namespace as a simple way
//			     - of avoiding conflicts between the libraries
namespace DisplayLibrary
{
	bool					g_bCustomPresentMode;
	std::vector < RECT >	g_CustomPresentRectangles;
};

DARKSDK GlobStruct* GetGlobalData ( void )
{
	return g_pGlob;
}

void GetD3DExtraInfo ( int *piAdapterOrdinal, LPSTR pAdapterName, int* piFeatureLevel )
{
	*piAdapterOrdinal = m_uAdapterChoice;
	strcpy ( pAdapterName, m_pAdapterName );
	*piFeatureLevel = g_featureLevel;
}

static LONG WINAPI DelayLoadDllExceptionFilter(PEXCEPTION_POINTERS pep, std::string& strError)
{
	// Structured Exception Handler for delay loaded DLLs
	// If this is a Delay-load problem, ExceptionInformation[0] points 
	// to a DelayLoadInfo structure that has detailed error info
	//PDelayLoadInfo pdli = PDelayLoadInfo(pep->ExceptionRecord->ExceptionInformation[0]);
	//switch(pep->ExceptionRecord->ExceptionCode)
	//{
	//default:
	//	// We don't recognize this exception
  	//	return EXCEPTION_CONTINUE_SEARCH;
	//	break;
	//}
	return EXCEPTION_CONTINUE_SEARCH;
}

std::string strInitD3DXError;
static bool InitD3DX ( void )
{
	return true;
	/*
	bool bRet = true;
	__try
	{
		// Load D3DX DLL by calling one of its functions
		GGMATRIXA16 m1, m2, m3;
		memset(&m1, 0, sizeof(m1));
		memset(&m2, 0, sizeof(m2));
		GGMatrixMultiply(&m3, &m1, &m2);
	}
	__except (DelayLoadDllExceptionFilter(GetExceptionInformation(), strInitD3DXError))
	{
		bRet = false;
	}
	return bRet;
	*/
}

DARKSDK bool SETUPConstructorDX9 ( void )
{
	// Direct X 9 Creation
	#ifndef DX11
	// Load D3DX (need to provoke D3DX DLL to load in, so we call a D3DX command that does not need m_pD3D)
	bool bD3DXAvailable = false;
	if( InitD3DX() ) bD3DXAvailable = true;
	if ( FAILED ( m_pDX = Direct3DCreate9 ( D3D_SDK_VERSION ) ) || bD3DXAvailable==false )
	{
		Error ( "Unable to create Direct3D interface" );
		return false;
	}

	// obtain hardware information
	m_iAdapterCount = m_pDX->GetAdapterCount ( );

	// setup adapter storage
	m_pInfo = new tagInfo [ m_iAdapterCount ];
	memset ( m_pInfo, 0, sizeof ( m_pInfo ) * m_iAdapterCount );

	// find out graphics card information e.g.
	int	iTemp = 0;
	for ( UINT iAdapter = 0; iAdapter < (DWORD)m_iAdapterCount; iAdapter++ )
	{
		GGDISPLAYMODE			d3dmode;
		GGCAPS				d3dcaps;
		D3DADAPTER_IDENTIFIER9	d3dAdapter;
		HRESULT					hr;

		// clear out any structures
		memset ( &d3dAdapter, 0, sizeof ( d3dAdapter ) );
		memset ( &d3dmode,    0, sizeof ( d3dmode    ) );
		memset ( &d3dcaps,    0, sizeof ( d3dcaps    ) );

		// get current display mode
		if ( FAILED ( hr = m_pDX->GetAdapterDisplayMode ( iAdapter, &d3dmode ) ) )
			Error ( "Failed to get adapter display mode" );

		// get device caps
		if ( FAILED ( hr = m_pDX->GetDeviceCaps ( iAdapter, D3DDEVTYPE_HAL, &d3dcaps ) ) )
			Error ( "Failed to get device caps" );

		// get adapter info
		if ( FAILED ( hr = m_pDX->GetAdapterIdentifier ( iAdapter, 0, &d3dAdapter ) ) )
			Error ( "Failed to get adapter identifier" );

		// get the name of the device
		m_pInfo [ iAdapter ].szName = new char [ 256 ];

		if ( !m_pInfo [ iAdapter ].szName )
			Error ( "Failed to allocate adapter name storage" );

		memset ( m_pInfo [ iAdapter ].szName, 0, sizeof ( char ) * 256 );
		memcpy ( m_pInfo [ iAdapter ].szName, d3dAdapter.Description, sizeof ( char ) * 256 );

		// copy adapter info
		memset ( &m_pInfo [ iAdapter ].D3DAdapter, 0, sizeof ( m_pInfo [ iAdapter ].D3DAdapter ) );
		memcpy ( &m_pInfo [ iAdapter ].D3DAdapter, &d3dAdapter, sizeof ( d3dAdapter ) );

		// copy current display mode
		memset ( &m_pInfo [ iAdapter ].D3DMode, 0, sizeof ( m_pInfo [ iAdapter ].D3DMode ) );
		memcpy ( &m_pInfo [ iAdapter ].D3DMode, &d3dmode, sizeof ( d3dmode ) );

		// copy the device caps
		memset ( &m_pInfo [ iAdapter ].D3DCaps, 0, sizeof ( m_pInfo [ iAdapter ].D3DCaps ) );
		memcpy ( &m_pInfo [ iAdapter ].D3DCaps, &d3dcaps, sizeof ( d3dcaps ) );

		// dx8->dx9 get all available display modes
		int iModeCount=0;
		m_pInfo [ iAdapter ].iDisplayCount = 0;
		while (iModeCount>=0 && iModeCount<=5)
		{
			if ( iModeCount==0 ) g_GGFORMAT = GGFMT_X8R8G8B8;
			if ( iModeCount==1 ) g_GGFORMAT = GGFMT_A8R8G8B8;
			if ( iModeCount==2 ) g_GGFORMAT = GGFMT_X1R5G5B5;
			if ( iModeCount==3 ) g_GGFORMAT = GGFMT_A1R5G5B5;
			if ( iModeCount==4 ) g_GGFORMAT = GGFMT_R5G6B5;
			if ( iModeCount==5 ) g_GGFORMAT = GGFMT_A2R10G10B10;
			m_pInfo [ iAdapter ].iDisplayCount += m_pDX->GetAdapterModeCount ( iAdapter, g_GGFORMAT );
			iModeCount++;
		}

		// create display mode array
		m_pInfo [ iAdapter ].D3DDisplay    = new GGDISPLAYMODE [ m_pInfo [ iAdapter ].iDisplayCount ];
		if ( !m_pInfo [ iAdapter ].D3DDisplay )
			Error ( "Failed to allocate dispay list" );

		// clear display mode array
		memset ( m_pInfo [ iAdapter ].D3DDisplay, 0, sizeof ( m_pInfo [ iAdapter ].D3DDisplay ) * m_pInfo [ iAdapter ].iDisplayCount );

		// enumerate display modes
		iModeCount=0;
		int iArrayCount=0;
		GGDISPLAYMODE DisplayMode, LastDsiplayMode;
		while (iModeCount>=0 && iModeCount<=5)
		{
			// formats
			if ( iModeCount==0 ) g_GGFORMAT = GGFMT_X8R8G8B8;
			if ( iModeCount==1 ) g_GGFORMAT = GGFMT_A8R8G8B8;
			if ( iModeCount==2 ) g_GGFORMAT = GGFMT_X1R5G5B5;
			if ( iModeCount==3 ) g_GGFORMAT = GGFMT_A1R5G5B5;
			if ( iModeCount==4 ) g_GGFORMAT = GGFMT_R5G6B5;
			if ( iModeCount==5 ) g_GGFORMAT = GGFMT_A2R10G10B10;

			// get number of them
			int iLocalCount = m_pDX->GetAdapterModeCount ( iAdapter, g_GGFORMAT );

			// enumerate them into list
			LastDsiplayMode.Width=0;
			LastDsiplayMode.Height=0;
			for ( iTemp=0; iTemp<iLocalCount; iTemp++ )
			{
				// enum for display mode data
				if ( FAILED ( hr = m_pDX->EnumAdapterModes ( iAdapter, g_GGFORMAT, iTemp, &m_pInfo [ iAdapter ].D3DDisplay [ iArrayCount ] ) ) )
					Error ( "Failed to enumerate adapter display modes" );

				// leefix - 200603 - most modes repeated for refresh rate, ignore refresh differences
				DisplayMode = m_pInfo [ iAdapter ].D3DDisplay [ iArrayCount ];
				if( DisplayMode.Width==LastDsiplayMode.Width && DisplayMode.Height==LastDsiplayMode.Height
				&& GetBitDepthFromFormat(DisplayMode.Format)==GetBitDepthFromFormat(LastDsiplayMode.Format) )
				{
					// do not increment array, so we overwrite with next one..
				}
				else
				{
					// next in array
					iArrayCount++;
				}
				LastDsiplayMode = DisplayMode;
			}

			// next mode
			iModeCount++;
		}

		// adjust size to actual display modes to use
		m_pInfo [ iAdapter ].iDisplayCount = iArrayCount;
	}

	// graphics card being used by number
	m_iAdapterUsed=0;		
	strcpy(m_pAdapterName, m_pInfo [ m_iAdapterUsed ].szName);
	#endif

	// success
	return true;
}

DARKSDK bool SETUPConstructorDX11 ( void )
{
	#ifdef DX11
	// Direct X 11 Creation

	// create m_pDX using equiv. Direct3DCreate9 ( D3D_SDK_VERSION )

	// fill m_pInfo with all adapters (needed?)
	// m_iAdapterCount = m_pDX->GetAdapterCount ( );
	//m_pInfo = new tagInfo [ m_iAdapterCount ];
	//memset ( m_pInfo, 0, sizeof ( m_pInfo ) * m_iAdapterCount );
	//for ( UINT iAdapter = 0; iAdapter < (DWORD)m_iAdapterCount; iAdapter++ )
	//if ( FAILED ( hr = m_pDX->GetAdapterDisplayMode ( iAdapter, &d3dmode ) ) )
	//if ( FAILED ( hr = m_pDX->GetDeviceCaps ( iAdapter, D3DDEVTYPE_HAL, &d3dcaps ) ) )
	//if ( FAILED ( hr = m_pDX->GetAdapterIdentifier ( iAdapter, 0, &d3dAdapter ) ) )
	//m_pInfo [ iAdapter ].szName = new char [ 256 ];
	//memset ( m_pInfo [ iAdapter ].szName, 0, sizeof ( char ) * 256 );
	//memcpy ( m_pInfo [ iAdapter ].szName, d3dAdapter.Description, sizeof ( char ) * 256 );
	//memset ( &m_pInfo [ iAdapter ].D3DAdapter, 0, sizeof ( m_pInfo [ iAdapter ].D3DAdapter ) );
	//memcpy ( &m_pInfo [ iAdapter ].D3DAdapter, &d3dAdapter, sizeof ( d3dAdapter ) );
	//memset ( &m_pInfo [ iAdapter ].D3DMode, 0, sizeof ( m_pInfo [ iAdapter ].D3DMode ) );
	//memcpy ( &m_pInfo [ iAdapter ].D3DMode, &d3dmode, sizeof ( d3dmode ) );
	//memset ( &m_pInfo [ iAdapter ].D3DCaps, 0, sizeof ( m_pInfo [ iAdapter ].D3DCaps ) );
	//memcpy ( &m_pInfo [ iAdapter ].D3DCaps, &d3dcaps, sizeof ( d3dcaps ) );
	//m_pInfo [ iAdapter ].iDisplayCount = 0;
	//while (iModeCount>=0 && iModeCount<=5) {
	//if ( iModeCount==0 ) g_GGFORMAT = GGFMT_X8R8G8B8;
	//if ( iModeCount==1 ) g_GGFORMAT = GGFMT_A8R8G8B8;
	//m_pInfo [ iAdapter ].iDisplayCount += m_pDX->GetAdapterModeCount ( iAdapter, g_GGFORMAT );
	//iModeCount++;}
	//m_pInfo [ iAdapter ].D3DDisplay    = new GGDISPLAYMODE [ m_pInfo [ iAdapter ].iDisplayCount ];
	//memset ( m_pInfo [ iAdapter ].D3DDisplay, 0, sizeof ( m_pInfo [ iAdapter ].D3DDisplay ) * m_pInfo [ iAdapter ].iDisplayCount );
	//if ( iModeCount==0 ) g_GGFORMAT = GGFMT_X8R8G8B8;
	//if ( iModeCount==1 ) g_GGFORMAT = GGFMT_A8R8G8B8;
	//int iLocalCount = m_pDX->GetAdapterModeCount ( iAdapter, g_GGFORMAT );
	//for ( iTemp=0; iTemp<iLocalCount; iTemp++ )
	//if ( FAILED ( hr = m_pDX->EnumAdapterModes ( iAdapter, g_GGFORMAT, iTemp, &m_pInfo [ iAdapter ].D3DDisplay [ iArrayCount ] ) ) )
	//DisplayMode = m_pInfo [ iAdapter ].D3DDisplay [ iArrayCount ];
	//iArrayCount++;
	//m_pInfo [ iAdapter ].iDisplayCount = iArrayCount;
	//m_iAdapterUsed=0;		
	//strcpy(m_pAdapterName, m_pInfo [ m_iAdapterUsed ].szName);
	#endif

	// success
	return true;
}

DARKSDK bool SETUPConstructor ( void )
{
	// setup default values
	m_iDisplayType   = 1;
	m_iDisplayChange = false;
	m_bOverrideHWND  = false;
	m_bResizeWindow  = true;
	m_Depth = GGFMT_UNKNOWN;
	m_StencilDepth = GGFMT_UNKNOWN;
	m_hWnd = GetForegroundWindow(); // redundant line?
	m_iSwapChainCount = 0;
	m_iGammaRed   = 255;
	m_iGammaGreen = 255;
	m_iGammaBlue  = 255;

	// ensure this is false by default
	DisplayLibrary::g_bCustomPresentMode = false;

	#ifdef DX11
	 SETUPConstructorDX11();
	#else
	 SETUPConstructorDX9();
	#endif

	// Default Window Settings for Windows Control
	gbWindowMode=true;
	gbWindowBorderActive=false;
	strcpy(gWindowName,"");
	gWindowVisible=SW_SHOWDEFAULT;
	gWindowSizeX = m_iWidth;
	gWindowSizeY = m_iHeight;
	gWindowIconHandle = g_pGlob->hAppIcon;// NULL;
	gWindowExtraX = gWindowExtraXForOverlap;
	gWindowExtraY = gWindowExtraYForOverlap;
	gWindowStyle = WS_OVERLAPPEDWINDOW;
	gbFirstInitOfDisplayOnly=true;

	// Get Window Display Mode (for later use for windows-modes)
	#ifdef DX11
	#else
	m_pDX->GetAdapterDisplayMode ( 0, &m_WindowsD3DMODE );
	#endif

	// disable screen saver at start of program
	SystemParametersInfo ( SPI_SETSCREENSAVEACTIVE, FALSE, 0, SPIF_SENDCHANGE );

	// Success
	return true;
}

#ifdef DX11
DARKSDK HRESULT SwapChainPresent ( int iID )
{
	if ( m_pSwapChain [ iID ] )
	{
		int iSyncInterval = 0;
		if ( m_bVSync )
		{
			// CAP TO MONITOR REFRESH RATE - NO TEARING
			iSyncInterval = m_iVSyncInterval;
		}
		else
		{
			// FAST AS YOU CAN - HAS HORIZ TEARING
			iSyncInterval = 0;
		}
		
		if(g_bNoSwapchainPresent)
			return m_pSwapChain[iID]->Present(iSyncInterval, DXGI_PRESENT_TEST);
		else
			return m_pSwapChain [ iID ]->Present( iSyncInterval, 0 );
	}
	else
		return 0;
}
#endif

DARKSDK void GetBackBufferPointersDX9(void)
{
	#ifndef DX11
	if(g_pGlob)
	{
		if(g_pGlob->pHoldBackBufferPtr) g_pGlob->pHoldBackBufferPtr->Release();
		if(g_pGlob->pHoldDepthBufferPtr) g_pGlob->pHoldDepthBufferPtr->Release();
		m_pD3D->GetRenderTarget(0, &g_pGlob->pHoldBackBufferPtr);
		m_pD3D->GetDepthStencilSurface(&g_pGlob->pHoldDepthBufferPtr);
		m_pD3D->SetRenderTarget(0, g_pGlob->pHoldBackBufferPtr);
		m_pD3D->SetDepthStencilSurface ( g_pGlob->pHoldDepthBufferPtr );
		g_pGlob->pCurrentBitmapSurface=g_pGlob->pHoldBackBufferPtr;
		g_pGlob->iCurrentBitmapNumber=0;
	}
	#endif
}

DARKSDK void GetBackBufferPointersDX11(void)
{
	#ifdef DX11
	if(g_pGlob)
	{
		g_pGlob->pHoldBackBufferPtr = m_pRenderTargetView;
		g_pGlob->pHoldDepthBufferPtr = m_pDepthStencilView;
		g_pGlob->pCurrentBitmapSurface = g_pBackBuffer;
		g_pGlob->pCurrentBitmapSurfaceView = m_pRenderTargetView;
		g_pGlob->pCurrentBitmapDepthView = m_pDepthStencilView;
		g_pGlob->pCurrentRenderView = m_pRenderTargetView;
		g_pGlob->pCurrentDepthView = m_pDepthStencilView;
		g_pGlob->iCurrentBitmapNumber = 0;
		g_pGlob->dwClientRegionWidth = 0; // can be updated after present when scan client rect size
		g_pGlob->dwClientRegionHeight = 0;
	}
	#endif
}

DARKSDK void GetBackBufferPointers(void)
{
	#ifdef DX11
	GetBackBufferPointersDX11();
	#else
	GetBackBufferPointersDX9();
	#endif
}

DARKSDK void ReleaseBackBufferPointers(void)
{
	// Free refs
	#ifdef DX11
	#else
	if(g_pGlob)
	{
		if(g_pGlob->pHoldBackBufferPtr)
		{
			g_pGlob->pHoldBackBufferPtr->Release();
			g_pGlob->pHoldBackBufferPtr=NULL;
		}
		if(g_pGlob->pHoldDepthBufferPtr)
		{
			g_pGlob->pHoldDepthBufferPtr->Release();
			g_pGlob->pHoldDepthBufferPtr=NULL;
		}
	}

	// Free locked surface
	if(g_dwSurfacePtr)
	{
		g_pBackBuffer->UnlockRect();
		g_dwSurfacePtr=NULL;
	}

	// Release backbuffer
	SAFE_RELEASE(g_pBackBuffer);
	#endif
}

DARKSDK void SETUPDestructor ( void ) 
{
	// Free ref to backbuffer
	ReleaseBackBufferPointers();

	// clear up D3D
	#ifdef DX11
	SAFE_RELEASE ( m_pRenderTargetView );
	SAFE_RELEASE ( g_pBackBuffer );
	SAFE_RELEASE ( m_pDepthStencilView );
	SAFE_RELEASE ( m_pDepthStencilResourceView );
	SAFE_RELEASE ( m_pDepthStencil );
	SAFE_RELEASE ( m_pSwapChain[0] );
	SAFE_RELEASE ( m_pImmediateContext );
	SAFE_RELEASE ( m_pD3D );
	#else
	// Delete global property-desc of device
	if ( m_D3DPP )
	{
		delete m_D3DPP;
		m_D3DPP=NULL;
	}
	SAFE_RELEASE ( m_pD3D );	// release the device
	SAFE_RELEASE ( m_pDX );	// release the interface
	#endif

	// remove any adapter information
	for ( int iTemp = 0; iTemp < m_iAdapterCount; iTemp++ )
	{
		SAFE_DELETE_ARRAY ( m_pInfo [ iTemp ].szName     );	// clear out name
		SAFE_DELETE_ARRAY ( m_pInfo [ iTemp ].D3DDisplay );	// clear out display structure
	}

	// finally get rid of the adapter info array
	SAFE_DELETE_ARRAY ( m_pInfo );

	// enable screen saver at end of program
	SystemParametersInfo ( SPI_SETSCREENSAVEACTIVE, TRUE, 0, SPIF_SENDCHANGE );
}

DARKSDK void SETUPSetErrorHandler ( LPVOID pErrorHandlerPtr ) 
{
	// Update error handler pointer
	g_pErrorHandler = (CRuntimeErrorHandler*)pErrorHandlerPtr;
}

DARKSDK BOOL CALLBACK EnumChildProc ( HWND hwnd, LPARAM lParam )
{
	char szBuffer [ MAX_PATH ];
	GetWindowText ( hwnd, szBuffer, MAX_PATH );
	if ( strcmp ( szBuffer, g_szChildWindow ) == 0 )
	{
		g_pGlob->hWnd     = hwnd;
		m_hWnd            = hwnd;
		g_bWindowOverride = true;
	}
	return TRUE;
}

DARKSDK BOOL CALLBACK EnumWindowsProc ( HWND hwnd, LPARAM lParam )
{
	char szBuffer [ MAX_PATH ];
	GetWindowText ( hwnd, szBuffer, MAX_PATH );
	int iResult = strspn ( szBuffer, g_szMainWindow );
	if ( iResult )
		EnumChildWindows ( hwnd, EnumChildProc, 0 );
	
	return TRUE;
}

DARKSDK void AttachWindowToChildOfAnother ( LPSTR pAbsoluteAppFilename )
{
	if ( !pAbsoluteAppFilename )
		return;

	// find external settings file (230105 - added >4 detect as it corrupted stack before in release mode)
	char pAppExtFile[_MAX_PATH];
	if ( strlen ( pAbsoluteAppFilename ) > 4 )
	{
		// myprog.exe becomes myprog.ini in pAppExtFile
		strcpy ( pAppExtFile, pAbsoluteAppFilename );
		strcpy ( pAppExtFile + strlen(pAppExtFile) - 4, ".ini" );
	}
	else
		return;

	// clear strings
	strcpy ( g_szMainWindow, "" );
	strcpy ( g_szChildWindow, "" );

	// read data from settings file
	#ifdef VRTECH
	 #ifdef PRODUCTV3
	  strcpy ( g_szMainWindow, "VR Quest" );
	 #else
	  strcpy ( g_szMainWindow, "GameGuru MAX" );
	 #endif
	 strcpy ( g_szChildWindow, "Editor" );
	#else
	 GetPrivateProfileString ( "External", "Main Window", "", g_szMainWindow, MAX_PATH, pAppExtFile );
	 GetPrivateProfileString ( "External", "Child Window", "", g_szChildWindow, MAX_PATH, pAppExtFile );
	#endif

	// determine if window should be attached to child of another
	if ( strlen ( g_szMainWindow ) > 1 )
		EnumWindows ( EnumWindowsProc, 0 );

	// if not overridden, try again after delay
	if ( g_bWindowOverride == false )
	{
		//Sleep(2000); remove this 2s delay to speed up initial IDE loading
		Sleep(200);
		if ( strlen ( g_szMainWindow ) > 1 )
			EnumWindows ( EnumWindowsProc, 0 );

		if ( g_bWindowOverride == false )
		{
			Sleep(3000);
			if ( strlen ( g_szMainWindow ) > 1 )
				EnumWindows ( EnumWindowsProc, 0 );
		}
	}
}

DARKSDK void SETUPPassCoreData ( LPVOID pGlobPtr, int iStage ) 
{
	switch(iStage)
	{
		case 0 :	// Constructor Phase
					g_OldHwnd = g_pGlob->hWnd;
					// Make this window child of another (if applicable)
					AttachWindowToChildOfAnother ( (LPSTR)g_pGlob->ppEXEAbsFilename );
					// done
					break;
		case 1 :	// Post-Device-Creation Phase
					g_pGlob = (GlobStruct*)pGlobPtr;
					///g_pCreateDeleteStringFunction = g_pGlob->CreateDeleteString;
					#ifdef DX11
					#else
					SETUPClear(0,0,0);
					GetBackBufferPointers();
					#endif
					break;
	}
}

DARKSDK void BuildFunctionsForDLLRefresh(void)
{
}

DARKSDK void InformDLLsOfDeviceLostOrNotReset ( int iDeviceLost )
{
	// leeadd - 020308 - inform all TPC DLLs that Device Has Been Lost
	if ( g_pGlob->pDynMemPtr )
	{
		// get local copy of DLLs
		HINSTANCE	hDLLMod[256];
		bool		bDLLTPC[256];
		memcpy ( hDLLMod, g_pGlob->pDynMemPtr+0, (sizeof(HINSTANCE)*256) );
		memcpy ( bDLLTPC, g_pGlob->pDynMemPtr+(sizeof(HINSTANCE)*256), (sizeof(bool)*256) );
	}
}

DARKSDK void InformDLLsOfD3DChange(int iMode)
{
	// Make sure altest DLLs have links to their refresh functions
	//
	// COOL TRICK : REM out refresh lines to find resource leak when SETDSPLAYMODE
	//
	BuildFunctionsForDLLRefresh();

	// D3D Change release/recreate
	if(g_pGlob)
	{
		// Release/Regrab backbuffer pointers
		if(iMode==0) ReleaseBackBufferPointers();
		if(iMode==1) GetBackBufferPointers();

		// Call Refresh Function of Any Active DLLS
		Basic2DRefreshGRAFIX(iMode);
		SpritesRefreshGRAFIX(iMode);
		ImageRefreshGRAFIX(iMode);
		InputRefreshGRAFIX(iMode);
		SystemRefreshGRAFIX(iMode); 
		MemblocksRefreshGRAFIX(iMode);
		BitmapRefreshGRAFIX(iMode);
		#ifndef NOSTEAMORVIDEO
		AnimationRefreshGRAFIX(iMode);
		#endif
		Basic3DRefreshGRAFIX(iMode);
		CameraRefreshGRAFIX(iMode);
		LightRefreshGRAFIX(iMode);
		VectorRefreshGRAFIX(iMode);
		SoundRefreshGRAFIX(iMode);

		// leeadd - 280305 - must also refresh all TPC DLls (u58)
		if ( g_pGlob->pDynMemPtr )
		{
			// get local copy of DLLs
			HINSTANCE	hDLLMod[256];
			bool		bDLLTPC[256];
			memcpy ( hDLLMod, g_pGlob->pDynMemPtr+0, (sizeof(HINSTANCE)*256) );
			memcpy ( bDLLTPC, g_pGlob->pDynMemPtr+(sizeof(HINSTANCE)*256), (sizeof(bool)*256) );
		}
	}
}

static LPSTR GetReturnStringFromWorkString(void)
{
	LPSTR pReturnString=NULL;
	if(m_pWorkString)
	{
		DWORD dwSize=strlen(m_pWorkString);
		g_pGlob->CreateDeleteString((char**)&pReturnString, dwSize+1);
		strcpy(pReturnString, m_pWorkString);
	}
	return pReturnString;
}

static int GetBitDepthFromFormatDX9(GGFORMAT Format)
{
	#ifndef DX11
	switch(Format)
	{
		case GGFMT_R8G8B8 :		return 24;	break;
		case GGFMT_A8R8G8B8 :		return 32;	break;
		case GGFMT_X8R8G8B8 :		return 32;	break;
		case GGFMT_R5G6B5 :		return 16;	break;
		case GGFMT_X1R5G5B5 :		return 16;	break;
		case GGFMT_A1R5G5B5 :		return 16;	break;
		case GGFMT_A4R4G4B4 :		return 16;	break;
		case GGFMT_A8	:			return 8;	break;
		case GGFMT_R3G3B2 :		return 8;	break;
		case GGFMT_A8R3G3B2 :		return 16;	break;
		case GGFMT_X4R4G4B4 :		return 16;	break;
		case GGFMT_A2B10G10R10 :	return 32;	break;
		case GGFMT_G16R16 :		return 32;	break;
		case GGFMT_A8P8 :			return 8;	break;
		case GGFMT_P8 :			return 8;	break;
		case GGFMT_L8 :			return 8;	break;
		case GGFMT_A8L8 :			return 16;	break;
		case GGFMT_A4L4 :			return 8;	break;
	}
	#endif
	return 0;
}

static int GetBitDepthFromFormatDX11(GGFORMAT Format)
{
	#ifdef DX11
	#endif
	return 32;
}

static int GetBitDepthFromFormat(GGFORMAT Format)
{
	#ifdef DX11
		return GetBitDepthFromFormatDX11(Format);
	#else
		return GetBitDepthFromFormatDX9(Format);
	#endif
}

DARKSDK void DB_UpdateEntireWindow(bool bFullUpdate, bool bMovement)
{
	// regular DBP window
	if(bFullUpdate==false)
	{
		ShowWindow(m_hWnd, gWindowVisible);
	}
	else
	{
		if(strlen(gWindowName)>0) SetWindowText(m_hWnd, gWindowName);
		SetWindowLong(m_hWnd, GWL_STYLE, gWindowStyle);
		DWORD dwActualWindowWidth = gWindowSizeX+gWindowExtraX;
		DWORD dwActualWindowHeight = gWindowSizeY+gWindowExtraY;
		SetWindowPos(m_hWnd, HWND_TOP, g_pGlob->dwWindowX, g_pGlob->dwWindowY, dwActualWindowWidth, dwActualWindowHeight, SWP_SHOWWINDOW);
		ShowWindow(m_hWnd, gWindowVisible);
		#ifdef WICKEDENGINE
		SetClassLong(m_hWnd, GCLP_HICON, (LONG)gWindowIconHandle);
		#else
		SetClassLong(m_hWnd, GCL_HICON, (LONG)gWindowIconHandle);
		#endif
	}

	// Paint after window switch (fixed issue of splash being whited out)
	//GGREDUCED
	//if(bMovement)
	//	InvalidateRect(NULL, NULL, FALSE);
	//else
	//	InvalidateRect(m_hWnd, NULL, FALSE);

	UpdateWindow(m_hWnd);
}

DARKSDK void DB_EnsureWindowRestored(void)
{
	if(gbWindowMode)
	{
		gbWindowMode=false;
		gbWindowBorderActive=true;
		gWindowVisible=SW_SHOWDEFAULT;
		gWindowSizeX = GetSystemMetrics(SM_CXFULLSCREEN);
		gWindowSizeY = GetSystemMetrics(SM_CYFULLSCREEN);
		gWindowStyle = WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU;
		gWindowIconHandle = gOriginalIcon;
		DB_UpdateEntireWindow(true, true);
	}
}

DARKSDK void UpdateWindowSize ( int iWidth, int iHeight )
{
	// updates the window size, remember that in windowed
	// mode the window could be resized at any stage, if this
	// happens we need to inform the setup library

	// we could always detect the size automatically but then
	// it's waste because we would be checking it every frame
	// and this would be inefficient, by doing it this way we
	// call this function when the window has been resized

	// store the new window size
	m_iWindowWidth  = iWidth;
	m_iWindowHeight = iHeight;
}

DARKSDK void GetWindowSize ( int* piWidth, int* piHeight )
{
	// retrieve the size of the window, several other DLL's
	// need to be able to do this, for example when clicking
	// on objects we need to determine the size of the window

	// check for valid pointers
	if ( !piWidth || !piHeight )
		Error1 ( "Invalid pointers passed to GetWindowSize for setup library" );

	// assign the pointers the saved window size
	*piWidth  = m_iWindowWidth;
	*piHeight = m_iWindowHeight;
}

DARKSDK void OverrideHWND ( HWND hWnd )
{
	// use an external window instead of the default
	// check the window handle is valid
	if ( !hWnd ) Error1 ( "Invalid window handle passed to OverrideHWND" );
	m_bOverrideHWND = true;
	m_hWnd          = hWnd;
}

DARKSDK void DisableWindowResize ( void )
{
	// disable window resizing
	m_bResizeWindow = false;
}

DARKSDK void EnableWindowResize ( void )
{
	// enable window resizing
	m_bResizeWindow = true;
}

DARKSDK void AddSwapChain ( HWND hwnd )
{
	// add to the swap chain

	// variable declarations
	GGPRESENT_PARAMETERS	d3dpp;	// setup structure
	GGDISPLAYMODE			mode;	// display mode

	// check the window handle is valid
	if ( !hwnd )
		Error1 ( "Invalid window handle for AddSwapChain" );

	// clear out structures
	memset ( &d3dpp, 0, sizeof ( d3dpp ) );
	memset ( &mode,  0, sizeof ( mode  ) );

	#ifdef DX11
	#else
	// get the current display mode
	m_pDX->GetAdapterDisplayMode ( m_uAdapterChoice, &mode );
	
	// setup some fields
	d3dpp.Windowed         = true;					// windowed mode
	d3dpp.SwapEffect       = D3DSWAPEFFECT_COPY;	// use copy
	d3dpp.BackBufferFormat = mode.Format;			// back buffer format is same as main mode
	d3dpp.hDeviceWindow    = hwnd;					// handle to device window

	// finally add the new swap chain
	HRESULT					hr;		// used for error checking
	if ( FAILED ( hr = m_pD3D->CreateAdditionalSwapChain ( &d3dpp, &m_pSwapChain [ m_iSwapChainCount++ ] ) ) )
	{
		// check the type of error, while DXTrace would do a better job than this
		// we can't use it as it's only provided for debug use
		switch ( hr )
		{
			case D3DERR_INVALIDCALL:
				Error ( "Invalid parameters when creating additional swap chain" );
			break;

			case D3DERR_OUTOFVIDEOMEMORY:
				Error ( "Unable to create additional swap chain - out of video memory" );
			break;
		}
	}
	#endif
}

DARKSDK void StartSwapChain ( int iID )
{
	// start rendering a swap chain
	#ifdef DX11
	#else
	// declare some surfaces
	LPGGSURFACE	pBack    = NULL;	// back buffer
	LPGGSURFACE	pStencil = NULL;	// stencil buffer

	// check that the device is valid
	if ( !m_pD3D )
		return;

	// check the ID is valid
	if ( iID > MAX_SWAP_CHAINS )
		Error ( "Specified invalid swap chain - overrun maximum limit" );

	// check that we could begin a scene
	if ( SUCCEEDED ( m_pD3D->BeginScene ( ) ) )
		m_bDraw = true;

	// get the backbuffer
	m_pSwapChain [ iID ]->GetBackBuffer ( 0, D3DBACKBUFFER_TYPE_MONO, &pBack );

	// now get the stencil buffer
	m_pD3D->GetDepthStencilSurface ( &pStencil );

	// check we got the buffer
	if ( !pBack || !pStencil )
		Error ( "Unable to access back buffer / stencil buffer for swap chain" );

	// now setup the new render target
	m_pD3D->SetRenderTarget ( 0, pBack );
	m_pD3D->SetDepthStencilSurface ( pStencil );

	// due to the way COM works we need to release
	// these interfaces otherwise we would end up 
	// with some resource leaks
	pBack->Release    ( );
	pStencil->Release ( );
	#endif
}

DARKSDK void EndSwapChain ( int iID )
{
	// finish rendering a swap chain
	// check the device is valid
	if ( !m_pD3D )
		return;

	// check we are ok to draw
	if ( m_bDraw )
	{
		#ifdef DX11
		#else
		m_pD3D->EndScene ( );	// finish the scene drawing
		#endif
		m_bDraw = false;		// report that we've finished drawing
	}
}

DARKSDK void UpdateSwapChain ( int iID )
{
	// update the swap chain and draw it's contents on screen

	// check the ID is valid
	if ( iID > MAX_SWAP_CHAINS )
		Error1 ( "Specified invalid swap chain - overrun maximum limit" );

	// check the swap chain is valid
	if ( !m_pSwapChain [ iID ] )
		Error1 ( "Swap chain pointer not setup correctly" );

	// update full screen
	#ifdef DX11
	SwapChainPresent(iID);
	#else
	m_pSwapChain [ iID ]->Present ( NULL, NULL, NULL, NULL, 0 );
	#endif
}

DARKSDK void Begin ( void )
{
	// being a typical rendering session
	g_bValidFPS = true;
	//if ( g_bWindowOverride ) ShowWindow ( g_OldHwnd, SW_HIDE );
	if ( !m_pD3D ) return;

	// now begin scene drawing
	if ( g_bSceneBegun==false )
	{
		g_bSceneBegun = true;
		#ifdef DX11
		#else
		if ( SUCCEEDED ( m_pD3D->BeginScene ( ) ) )
			m_bDraw = true;
		#endif
	}
}

DARKSDK void End ( void )
{
	// end the rendering session
	if ( !m_pD3D ) return;
	if ( m_bDraw && m_pD3D )
	{
		if ( g_bSceneBegun==true ) 
		{
			#ifdef DX11
			#else
			m_pD3D->EndScene ( );
			#endif
		}
		m_bDraw = false;
		g_bSceneBegun = false;
	}
}

DARKSDK HRESULT PresentRect ( RECT* pArea, RECT* pClientrc, float fX, float fY )
{
	// result
	HRESULT hRes = 0;

	// assign src and dest rects
	RECT src = { 0, 0, g_pGlob->iScreenWidth, g_pGlob->iScreenHeight };
	for(DWORD s=0; s<4; s++)
	{
		if(s==0)
		{
			src.bottom=pArea->top;
		}
		if(s==1)
		{
			src.top=pArea->top;
			src.bottom=pArea->bottom;
			src.right=pArea->left;
		}
		if(s==2)
		{
			src.left=pArea->right;
			src.right=pClientrc->right;
		}
		if(s==3)
		{
			src.top=pArea->bottom;
			src.left=0;
			src.bottom=pClientrc->bottom;
		}
		RECT dest = { (int)(src.left*fX), (int)(src.top*fY), (int)(src.right*fX), (int)(src.bottom*fY) };
		#ifdef DX11
		#else
		hRes = m_pD3D->Present ( &src, &dest, NULL, NULL );
		#endif
	}
	return hRes;
}

DARKSDK void DivideAreaByRect ( RECT* pArea, RECT* pSafeBoxes, DWORD* pdwDrawToBoxes, RECT** ppDrawBoxes )
{
	// Determine if area is clear of safe boxes
	bool bAreaIsClearOfSafeBoxes=true;
	for ( DWORD si=0; si<g_pGlob->dwSafeRectMax; si++ )
	{
		// check whether safe box intersects area
		if ( pSafeBoxes[si].right!=0 )
		{
			if( pArea->right > pSafeBoxes[si].left
			&&	pArea->bottom > pSafeBoxes[si].top
			&&	pArea->left < pSafeBoxes[si].right
			&&	pArea->top < pSafeBoxes[si].bottom	)	
			{
				// found safe box cutting into area
				bAreaIsClearOfSafeBoxes=false;

				// Store original rect
				RECT rectOrig = pSafeBoxes[si];

				// divide area into four smaller areas (all sides of safe box)
				RECT src = *pArea;
				for(DWORD s=0; s<4; s++)
				{
					if(s==0)
					{
						src.bottom=rectOrig.top;
					}
					if(s==1)
					{
						src.top=rectOrig.top;
						src.bottom=rectOrig.bottom;
						src.right=rectOrig.left;
					}
					if(s==2)
					{
						src.left=rectOrig.right;
						src.right=pArea->right;
					}
					if(s==3)
					{
						src.top=rectOrig.bottom;
						src.left=pArea->left;
						src.bottom=pArea->bottom;
					}

					// further divide this area by remaining safe boxes
					DivideAreaByRect ( &src, pSafeBoxes, pdwDrawToBoxes, ppDrawBoxes );
				}
			}
		}
	}

	// if area clear of safe boxes, add to draw box array
	if ( bAreaIsClearOfSafeBoxes )
	{
		// check if box is a valid one
		bool bBoxIsQualified=false;
		if(	pArea->right > pArea->left && pArea->bottom > pArea->top
		&&	pArea->left >= 0 && pArea->top >= 0
		&&	pArea->right <= g_pGlob->iScreenWidth
		&&	pArea->bottom <= g_pGlob->iScreenHeight	)
			bBoxIsQualified=true;

		// only add qualified boxes
		if (bBoxIsQualified )
		{
			if ( (*ppDrawBoxes) ) (*ppDrawBoxes) [ (*pdwDrawToBoxes) ] = *pArea;
			*pdwDrawToBoxes = *pdwDrawToBoxes + 1;
		}
	}
}

DARKSDK void CreateDrawBoxes ( DWORD* pdwDrawToBoxes, RECT** ppDrawBoxes )
{
	// Go through safe boxes
	if ( g_pGlob->dwSafeRectMax > 0 )
	{
		// Wipe out any boxes entirely inside other boxes
		for ( DWORD iSR=0; iSR<g_pGlob->dwSafeRectMax; iSR++ )
		{
			for ( DWORD iSR2=0; iSR2<g_pGlob->dwSafeRectMax; iSR2++ )
			{
				if ( iSR != iSR2 )
				{
					if(	g_pGlob->pSafeRects [iSR].left >= g_pGlob->pSafeRects [iSR2].left
					&&	g_pGlob->pSafeRects [iSR].right <= g_pGlob->pSafeRects [iSR2].right
					&&	g_pGlob->pSafeRects [iSR].top >= g_pGlob->pSafeRects [iSR2].top
					&&	g_pGlob->pSafeRects [iSR].bottom <= g_pGlob->pSafeRects [iSR2].bottom )
					{
						g_pGlob->pSafeRects [iSR].right=0;
					}
				}
			}
		}

		// Create a copy of the safe boxes
		RECT* pSafeBoxesCopy = new RECT [ g_pGlob->dwSafeRectMax ];

		// Initial area is entire screen
		RECT area = { 0, 0, g_pGlob->iScreenWidth, g_pGlob->iScreenHeight };

		// Count number of draw boxes
		memcpy ( pSafeBoxesCopy, g_pGlob->pSafeRects, sizeof(RECT) * g_pGlob->dwSafeRectMax );
		DivideAreaByRect ( &area, pSafeBoxesCopy, pdwDrawToBoxes, ppDrawBoxes );

		// Create draw array
		*ppDrawBoxes = new RECT [ *pdwDrawToBoxes ];
		*pdwDrawToBoxes = 0;

		// Create draw boxes
		memcpy ( pSafeBoxesCopy, g_pGlob->pSafeRects, sizeof(RECT) * g_pGlob->dwSafeRectMax );
		DivideAreaByRect ( &area, pSafeBoxesCopy, pdwDrawToBoxes, ppDrawBoxes );

		// Free copy of safe boxes
		SAFE_DELETE(pSafeBoxesCopy);
	}
}

// mike - 070207 - switch on custom present mode
void dbSetCustomPresentMode ( bool bMode )
{
	DisplayLibrary::g_bCustomPresentMode = bMode;
}

// mike - 070207 - add a rectangle region
void dbAddCustomPresentRectangle ( RECT rect )
{
	DisplayLibrary::g_CustomPresentRectangles.push_back ( rect );
}

// mike - 070207 - clear the draw list
void dbClearCustomPresentList ( void )
{
	DisplayLibrary::g_CustomPresentRectangles.clear ( );
}

// mike - 070207 - render all regions
HRESULT dbRenderUsingCustomPresentList ( void )
{
	HRESULT hr = 0;
	for ( int i = 0; i < (int)DisplayLibrary::g_CustomPresentRectangles.size ( ); i++ )
	{
		// dest is primary stretched maybe
		RECT clientrc = { 0,0,0,0 };
		GetClientRect(g_pGlob->hWnd, &clientrc);

		// calculate scaling between src and dest
		float fX = (float)clientrc.right / (float)g_pGlob->iScreenWidth;
		float fY = (float)clientrc.bottom / (float)g_pGlob->iScreenHeight;
		RECT src = DisplayLibrary::g_CustomPresentRectangles [ i ];
		RECT dest = { (int)(src.left*fX), (int)(src.top*fY), (int)(src.right*fX), (int)(src.bottom*fY) };
		#ifdef DX11
		#else
		hr = m_pD3D->Present ( &src, &dest, NULL, NULL );
		#endif
	}
	return hr;
}

int g_iTriggerResize = 0;

HRESULT StandardPresent ( void )
{
	// result
	HRESULT hRes  S_OK;

	// Standard stretched present to visual surface
	#ifdef DX11
	// No VR, just a simple swap chain pesent
	SwapChainPresent(0);

	if ( g_bWindowOverride && g_dwChildWindowTruePixel && m_pSwapChain[0] )
	{
		// no stretch present equivilant in DX11, so resize backbuffer instead
		// resize swapchain to suit correct child window size

		RECT src = { 0, 0, 0, 0 };
		GetClientRect ( g_pGlob->hWnd, &src );

		if ( src.right != g_pGlob->dwClientRegionWidth || src.bottom != g_pGlob->dwClientRegionHeight )
		{
			m_pImmediateContext->ClearState();
			SAFE_RELEASE(m_pRenderTargetView);
			SAFE_RELEASE(g_pBackBuffer);
			SAFE_RELEASE(m_pDepthStencilView);
			SAFE_RELEASE(m_pDepthStencilResourceView);
			SAFE_RELEASE(m_pDepthStencil);
			HRESULT hRes = m_pSwapChain[0]->ResizeBuffers(0, src.right, src.bottom, DXGI_FORMAT_UNKNOWN, 0);

			GetBackBufferAndDepthBuffer();
			GetBackBufferPointers();

			if (hRes == S_OK)
			{
				g_pGlob->dwClientRegionWidth = src.right;
				g_pGlob->dwClientRegionHeight = src.bottom;
				g_iTriggerResize = 0;
			}
		}

	}
	#else
	if ( g_bWindowOverride && g_dwChildWindowTruePixel )
	{
		RECT src = { 0, 0, 0, 0 };
		GetClientRect ( m_hWnd , &src );
		hRes = m_pD3D->Present ( &src , NULL, NULL, NULL );
	}
	else
		hRes = m_pD3D->Present ( NULL, NULL, NULL, NULL );
	#endif

	// return if device lost during present
	return hRes;
}

DARKSDK void Render ( void )
{
	// result
	HRESULT hRes;

	// copy the buffers and show the contents
	if ( !m_pD3D ) return;

	// dest is primary stretched maybe
	RECT clientrc = { 0,0,0,0 };
	GetClientRect(g_pGlob->hWnd, &clientrc);

	// calculate scaling between src and dest
	float fX = (float)clientrc.right / (float)g_pGlob->iScreenWidth;
	float fY = (float)clientrc.bottom / (float)g_pGlob->iScreenHeight;

	// if array of protected boxes setup (from controls requiring primary surface)
	if ( g_pGlob->dwSafeRectMax>0 )
	{
		// create draw boxes from protected boxes
		DWORD dwDrawToBoxes = 0;
		RECT* pDrawBoxes = NULL;
		CreateDrawBoxes ( &dwDrawToBoxes, &pDrawBoxes );

		// go through boxes
		if ( dwDrawToBoxes > 0 )
		{
			for ( DWORD boxindex=0; boxindex<dwDrawToBoxes; boxindex++ )
			{
				// present each one to copy to primary
				RECT src = pDrawBoxes [ boxindex ];
				RECT dest = { (int)(src.left*fX), (int)(src.top*fY), (int)(src.right*fX), (int)(src.bottom*fY) };
				#ifdef DX11
				#else
				hRes = m_pD3D->Present ( &src, &dest, NULL, NULL );
				#endif
			}
		}

		// Free usages
		SAFE_DELETE(pDrawBoxes);
	}
	else
	{
		// if area is for direct primary-surface drawing (such as a DVD video window)
		if ( g_pGlob->iNoDrawRight!=0 )
		{
			// area as src-scaled rect
			RECT area = { (LONG)g_pGlob->iNoDrawLeft, (LONG)g_pGlob->iNoDrawTop, (LONG)g_pGlob->iNoDrawRight, (LONG)g_pGlob->iNoDrawBottom };

			// Present 
			hRes = PresentRect ( &area, &clientrc, fX, fY );
		}
		else
		{
			// Standard stretched present to visual surface
			hRes = StandardPresent();
		}
	}

	// Catch if present fail, device may be lost
	if(hRes==GGERR_DEVICELOST)
	{
		// Attempt to restore device
		RestoreLostDevice();
	}
}

DARKSDK bool SetDisplayMode ( int iWidth, int iHeight )
{
	// sets the display mode to the specified
	// width and height, default values are provided
	// when this function is called the program will
	// default to setting 16 bit color depth and will
	// also default to full screen mode and attempt
	// to select hardware vertex processing
	if ( !SetDisplayMode ( iWidth, iHeight, 16, FULLSCREEN, HARDWARE ) )
		return false;

	return true;
}

DARKSDK bool SetDisplayMode ( int iWidth, int iHeight, int iDepth )
{
	// sets the display mode to the specified
	// width and height, default values are provided
	// when this function is called the program will
	// default to full screen mode and attempt to
	// select hardware vertex processing, lockable
	// backbuffers are switched off
	if(!SetDisplayMode ( iWidth, iHeight, iDepth, FULLSCREEN, HARDWARE, 0 ) )
		return false;

	return true;
}

DARKSDK bool SetDisplayMode ( int iWidth, int iHeight, int iDepth, int iMode )
{
	// sets the display mode to the specified
	// width, height, depth and mode default values
	// are provided when this function is called 
	// the program will attempt to select hardware
	// vertex processing, lockable backbuffers are
	// switched off
	if ( !SetDisplayMode ( iWidth, iHeight, iDepth, iMode, HARDWARE, 0 ) )
		return false;

	return true;
}

DARKSDK bool SetDisplayMode ( int iWidth, int iHeight, int iDepth, int iMode, int iVertexProcessing )
{
	// sets the display mode
	if ( !SetDisplayMode ( iWidth, iHeight, iDepth, iMode, iVertexProcessing, 0 ) )
		return false;

	return true;
}

DARKSDK bool SetDisplayMode ( int iWidth, int iHeight, int iDepth, int iMode, int iVertexProcessing, int iLockable )
{
	#ifdef DX11
	#else
	int  iTemp;
	bool bValid = false;
	
	// check the width and height
	if ( iWidth <= 0 || iHeight <= 0 )
	{
		Error ( "Invalid display resolution" );
		RunTimeError(RUNTIMEERROR_SCREENSIZEILLEGAL);
		return false;
	}

	// if depth is zero, fill with current windows depth
	if ( iDepth == 0 )
	{
		// get windows natural depth
		GGDISPLAYMODE d3dmode;
		m_pDX->GetAdapterDisplayMode ( 0, &d3dmode );
		iDepth = GetBitDepthFromFormat(d3dmode.Format);
		g_pGlob->iScreenDepth = iDepth;
	}

	// 24bit not supported
	if ( iDepth == 24 )
	{
		Error ( "Invalid bit depth specified" );
		RunTimeError(RUNTIMEERROR_24BITNOTSUPPORTED);
		return false;
	}

	// check the depth
	if ( iDepth != 16 && iDepth != 32 && iDepth != 24 )
	{
		Error ( "Invalid bit depth specified" );
		RunTimeError(RUNTIMEERROR_SCREENDEPTHILLEGAL);
		return false;
	}

	// mode
	if ( iMode < 0 || iMode > 1 )
	{
		Error ( "Invalid display mode specified" );
		RunTimeError(RUNTIMEERROR_SCREENMODEINVALID);
		return false;
	}

	// just check that the mode does exist
	for ( iTemp = 0; iTemp < m_pInfo [ 0 ].iDisplayCount; iTemp++ )
	{
		// find the mode which which matches the selected width and height of the new display settings
		if (	m_pInfo [ 0 ].D3DDisplay [ iTemp ].Width  == (DWORD)iWidth &&
				m_pInfo [ 0 ].D3DDisplay [ iTemp ].Height  == (DWORD)iHeight &&
				GetBitDepthFromFormat(m_pInfo [ 0 ].D3DDisplay [ iTemp ].Format) == iDepth )
		{
			bValid = true;
			break;
		}
	}

	// if non standard size required, submit a valid size and setup clip data
	if ( !bValid )
	{
		// Generate a standard width
		int iStWidth=iWidth, iStHeight=iHeight;
		for ( iTemp = 0; iTemp < m_pInfo [ 0 ].iDisplayCount; iTemp++ )
		{
			if (	m_pInfo [ 0 ].D3DDisplay [ iTemp ].Width  > (DWORD)iStWidth &&
					m_pInfo [ 0 ].D3DDisplay [ iTemp ].Height  > (DWORD)iStHeight &&
					GetBitDepthFromFormat(m_pInfo [ 0 ].D3DDisplay [ iTemp ].Format) == iDepth )
			{
				// Nearest valid display mode
				iStWidth = m_pInfo [ 0 ].D3DDisplay [ iTemp ].Width;
				iStHeight = m_pInfo [ 0 ].D3DDisplay [ iTemp ].Height;
				bValid = true;
				break;
			}
		}

		// Create clip data
		m_iChopWidth = iStWidth - iWidth;
		m_iChopHeight = iStHeight - iHeight;

		// Assign new width and height
		iWidth = iStWidth;
		iHeight = iStHeight;
	}
	else
	{
		// No chopping required
		m_iChopWidth = 0;
		m_iChopHeight = 0;
	}

	// ensure that the mode exists
	if ( !bValid )
	{
		Error ( "Display mode not supported" );
		if(iDepth==16) RunTimeError(RUNTIMEERROR_NOTSUPPORTDISPLAY16B);
		if(iDepth==24) RunTimeError(RUNTIMEERROR_NOTSUPPORTDISPLAY24B);
		if(iDepth==32) RunTimeError(RUNTIMEERROR_NOTSUPPORTDISPLAY32B);
		return false;
	}
	
	// now check that the processing mode is valid
	if ( iVertexProcessing != HARDWARE && iVertexProcessing != SOFTWARE )
	{
		if ( iVertexProcessing < 0 || iVertexProcessing > 2 )
		{
			Error ( "Unknown vertex processing mode" );
			RunTimeError(RUNTIMEERROR_NOTSUPPORTDISPLAYVB);
			return false;
		}
	}

	// setup the correct vertex processing mode
	if ( iVertexProcessing == 0 )
		iVertexProcessing = HARDWARE;

	if ( iVertexProcessing == 1 )
		iVertexProcessing = SOFTWARE;

	if ( iVertexProcessing == 2 )
		iVertexProcessing = SOFTWARE;

	// last thing to do is check that the lockable flag is valid
	if ( iLockable < 0 || iLockable > 1 )
	{
		Error ( "Invalid lockable flag" );
		RunTimeError(RUNTIMEERROR_NOTSUPPORTDISPLAYLOCK);
		return false;
	}

	// Delete global property-desc of device
	if ( m_D3DPP )
	{
		delete m_D3DPP;
		m_D3DPP=NULL;
	}
	#endif

	// U69 - 180508 - multimonitor mode doubles the width
	if ( m_iMultimonitorMode==1 )
	{
		// to fill primary (first half) and second monitor (second half)
		iWidth = iWidth * 2;
	}

	// setup values
	m_D3DPP			    = NULL;					// don't use debug mode
	m_iWidth		    = iWidth;				// set width
	m_iHeight		    = iHeight;				// set height
	m_iDepth		    = iDepth;				// set default color depth
	m_iDisplayType	    = iMode;				// windowed or fullscreen
	m_iProcess		    = iVertexProcessing;	// vertex processing mode
	m_bLockable		    = true;					// default to a lockable backbuffer
	m_iBackBufferCount  = 1;					// back buffer count

	// now setup the device
	if ( Setup ( ) )
	{
		// Adjust window size if desktop fullscreen mode in effect
		if(g_pGlob)
		{
			// Get Default Icon
			gOriginalIcon = g_pGlob->hAppIcon;// (HICON)GetClassLong(g_pGlob->hWnd, GCL_HICON);
			gWindowIconHandle=gOriginalIcon;

			// 0=hidden
			if(g_pGlob->dwAppDisplayModeUsing==1)
			{
				// 1=window
				RECT clientrc;
				GetClientRect(g_pGlob->hWnd, &clientrc);
				gWindowSizeX = g_pGlob->iScreenWidth;
				gWindowSizeY = g_pGlob->iScreenHeight;
				if(gWindowExtraXForOverlap==0 && gbFirstInitOfDisplayOnly==true)
				{
					// Determine Extra for Overlapped Window Border
					gWindowExtraXForOverlap = g_pGlob->iScreenWidth-clientrc.right;
					gWindowExtraYForOverlap = g_pGlob->iScreenHeight-clientrc.bottom;
				}

				// U75 - 070909 - AA must have actual correct client size matching screen size, so ensure this here
				DWORD dwWindowStyle = GetWindowLong(g_pGlob->hWnd, GWL_STYLE);
				if ( (dwWindowStyle&WS_CAPTION) && (dwWindowStyle&WS_THICKFRAME) )
				{
					gWindowExtraX = gWindowExtraXForOverlap;
					gWindowExtraY = gWindowExtraYForOverlap;
				}
				else
				{
					gWindowExtraX = 0;
					gWindowExtraY = 0;
				}

				DWORD dwActualWindowWidth = gWindowSizeX+gWindowExtraX;
				DWORD dwActualWindowHeight = gWindowSizeY+gWindowExtraY;
				SetWindowPos ( g_pGlob->hWnd, HWND_TOP, 0, 0, dwActualWindowWidth, dwActualWindowHeight, SWP_NOMOVE | SWP_SHOWWINDOW );
			}
			if(g_pGlob->dwAppDisplayModeUsing==2)
			{
				// 2=taskbar
				RECT rc;
				SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
				gWindowSizeX = rc.right-rc.left;
				gWindowSizeY = rc.bottom-rc.top;
				gWindowExtraXForOverlap=0;
				gWindowExtraYForOverlap=0;
				gWindowExtraX = gWindowExtraXForOverlap;
				gWindowExtraY = gWindowExtraYForOverlap;
				SetWindowPos(g_pGlob->hWnd, HWND_TOP, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOMOVE );
			}
			if(g_pGlob->dwAppDisplayModeUsing==3)
			{
				// 3=exclusive
				gWindowSizeX = iWidth;
				gWindowSizeY = iHeight;
				gWindowExtraXForOverlap=0;
				gWindowExtraYForOverlap=0;
				gWindowExtraX = gWindowExtraXForOverlap;
				gWindowExtraY = gWindowExtraYForOverlap;
				SetWindowPos(g_pGlob->hWnd, HWND_TOP, 0, 0, iWidth, iHeight, SWP_NOMOVE );
			}
			if(g_pGlob->dwAppDisplayModeUsing==0
			|| g_pGlob->dwAppDisplayModeUsing==4)
			{
				// 0=hidden (added 101004 so hidden window can become mode4)
				// 4=notaskbar
				DWORD dwWidth=GetSystemMetrics(SM_CXSCREEN);
				DWORD dwHeight=GetSystemMetrics(SM_CYSCREEN);
				gWindowSizeX = dwWidth;
				gWindowSizeY = dwHeight;
				gWindowExtraXForOverlap=0;
				gWindowExtraYForOverlap=0;
				gWindowExtraX = gWindowExtraXForOverlap;
				gWindowExtraY = gWindowExtraYForOverlap;
				// leefix - 200906 - u63 - removed to not interfere with parallel DBP related tasks
				if ( g_pGlob->dwAppDisplayModeUsing==4 )
				{
					// non-hidden windows still must set this though
					SetWindowPos(g_pGlob->hWnd, HWND_TOP, 0, 0, dwWidth, dwHeight, SWP_NOMOVE);
				}
			}

			// Set only when CreateWindow first creates window (used to calculate extra window borders)
			gbFirstInitOfDisplayOnly=false;
		}

		// in case drawing right away, open scene
		Begin();

		// display mode successful
		return true;
	}
	else
	{
		Error1 ( "Unable to setup 3D device" );
	}

	// Runs off the end as unknown direct x, else runtime picked up along the way
	return false;
}

DARKSDK bool SetDisplayDebugMode ( void )
{
	Error1 ( "SetDisplayDebug mode disabled" );
	return true;
}

DARKSDK void RestoreDisplayMode  ( void )
{
	// Restore display mode (called after a set display mode when releasing)
	#ifdef DX11
	#else
	if(m_D3DPP && m_pD3D)
	{
		if(m_D3DPP->Windowed==FALSE)
		{
			m_D3DPP->Windowed = TRUE;
			ReleaseBackBufferPointers();
			m_pD3D->Reset ( m_D3DPP );
			GetBackBufferPointers();
		}
	}
	#endif
}

BOOL Is_Win_Vista_Or_Later () 
{
   OSVERSIONINFOEX osvi;
   DWORDLONG dwlConditionMask = 0;
   int op=VER_GREATER_EQUAL;

   // Initialize the OSVERSIONINFOEX structure.

   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   osvi.dwMajorVersion = 6;
   osvi.dwMinorVersion = 0;
   osvi.wServicePackMajor = 0;
   osvi.wServicePackMinor = 0;

   // Initialize the condition mask.

   VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, op );
   VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, op );
   VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, op );
   VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMINOR, op );

   // Perform the test.

   return VerifyVersionInfo(
      &osvi, 
      VER_MAJORVERSION | VER_MINORVERSION | 
      VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
      dwlConditionMask);
}

DARKSDK bool GetBackBufferAndDepthBuffer ( void )
{
	#ifdef DX11
	// Create backbuffer from swap chain
    HRESULT hr = m_pSwapChain[0]->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&g_pBackBuffer );
    if( FAILED( hr ) )
	{
		//Error( "Failed to GetBuffer\n" );
        return false;
	}

	// Create render target view from backbuffer
    hr = m_pD3D->CreateRenderTargetView( g_pBackBuffer, NULL, &m_pRenderTargetView );
    if( FAILED( hr ) )
    {
		Error1 ( "Failed to CreateRenderTargetView\n" );
        return false;
	}

	// Get render target size so can match with depth buffer size
	D3D11_TEXTURE2D_DESC ddsd;
	g_pBackBuffer->GetDesc ( &ddsd );

	// Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory( &descDepth, sizeof(descDepth) );
    descDepth.Width = ddsd.Width;
    descDepth.Height = ddsd.Height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_R32_TYPELESS;//DXGI_FORMAT_R24G8_TYPELESS;//DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;    
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = m_pD3D->CreateTexture2D( &descDepth, NULL, &m_pDepthStencil );
    if( FAILED( hr ) )
	{
		Error1 ( "Failed to CreateTexture2D\n" );
        return false;
	}

	// Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory( &descDSV, sizeof(descDSV) );
    descDSV.Format = DXGI_FORMAT_D32_FLOAT;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = m_pD3D->CreateDepthStencilView( m_pDepthStencil, &descDSV, &m_pDepthStencilView );
    if( FAILED( hr ) )
	{
		Error1 ( "Failed to CreateDepthStencilView\n" );
        return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	ZeroMemory(&shaderResourceViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;//DXGI_FORMAT_R24_UNORM_X8_TYPELESS; (DXGI_FORMAT_R32_FLOAT)
	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	hr = m_pD3D->CreateShaderResourceView ( m_pDepthStencil, &shaderResourceViewDesc, &m_pDepthStencilResourceView );

	// success
	return true;
	#endif
}

