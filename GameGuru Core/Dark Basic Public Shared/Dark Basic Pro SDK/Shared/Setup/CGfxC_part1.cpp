#define DEPTH_BIAS_D32_FLOAT(d) (d/(1/pow(2,23)))

DARKSDK bool SetupDX11 ( void )
{
	#ifdef DX11

	// use default adapter, unless force another one
	m_uAdapterChoice = GGADAPTER_DEFAULT;
	if ( m_iForceAdapterOrdinal>0 )
	{
		m_uAdapterChoice = m_iForceAdapterOrdinal;
	}
	IDXGIAdapter* pAdapter = NULL;
	strcpy ( m_pAdapterName, "Default Adapter" );
	D3D_DRIVER_TYPE adapterType = D3D_DRIVER_TYPE_HARDWARE;
	std::vector <IDXGIAdapter*> vAdapters;
	IDXGIFactory* pFactory = NULL;
	//if(SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory) ,(void**)&pFactory))) GGVR needs this!
	if(SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory) ,(void**)&pFactory)))
	{
		// special mode to search for a non-Intel GPU adapter (typically a dedicated higher powered one)
		if ( m_uAdapterChoice == 99 )
		{
			m_uAdapterChoice = 0; // in any event, use default adapter if cannot find a better adapter
			for ( int iDedicatedThenIntel = 0; iDedicatedThenIntel < 2; iDedicatedThenIntel++ )
			{
				bool bFoundAGoodAdapter = false;
				for ( int iAdapterIndex = 0; iAdapterIndex < 10; iAdapterIndex++ )
				{
					if ( pFactory->EnumAdapters(iAdapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND )
					{
						DXGI_ADAPTER_DESC adapterDesc;
						pAdapter->GetDesc(&adapterDesc);
						memset ( m_pAdapterName, 0, sizeof ( m_pAdapterName ) );
						const int size = ::WideCharToMultiByte( CP_UTF8, 0, adapterDesc.Description, -1, NULL, 0, 0, NULL );
						::WideCharToMultiByte( CP_UTF8, 0, adapterDesc.Description, -1, m_pAdapterName, size, 0, NULL );
						strlwr ( m_pAdapterName );

						// in first pass, ignore any INTEL or MICROSOFT hardware, second pass allows INTEL
						if ( iDedicatedThenIntel == 0 )
						{
							if ( strstr ( m_pAdapterName, "intel" ) != NULL || strstr ( m_pAdapterName, "microsoft" ) != NULL )
							{
								// ignore this - this adapter is likely integrated and slower than dedicated
							}
							else
							{
								// a found non-Intel adapter
								bFoundAGoodAdapter = true;
							}
						}
						if ( iDedicatedThenIntel == 1 )
						{
							if ( strstr ( m_pAdapterName, "microsoft" ) != NULL )
							{
								// ignore this - this adapter will pick Intel over Microsoft 'reference' hardware
							}
							else
							{
								bFoundAGoodAdapter = true;
							}
						}
						if ( bFoundAGoodAdapter == true )
						{
							m_uAdapterChoice = iAdapterIndex;
							break;
						}
					}
				}
				if ( bFoundAGoodAdapter == true )
				{
					// no need to go to next pass
					break;
				}
			}
		}
		if ( pFactory->EnumAdapters(m_uAdapterChoice, &pAdapter) != DXGI_ERROR_NOT_FOUND )
		{
			DXGI_ADAPTER_DESC adapterDesc;
			pAdapter->GetDesc(&adapterDesc);
			memset ( m_pAdapterName, 0, sizeof ( m_pAdapterName ) );
			const int size = ::WideCharToMultiByte( CP_UTF8, 0, adapterDesc.Description, -1, NULL, 0, 0, NULL );
			::WideCharToMultiByte( CP_UTF8, 0, adapterDesc.Description, -1, m_pAdapterName, size, 0, NULL );
			adapterType = D3D_DRIVER_TYPE_UNKNOWN;
		}
		else
		{
			// cannot get adapter
			strcpy ( m_pAdapterName, "Invalid Adapter Chosen" );
			pAdapter = NULL;
		}
		pFactory->Release();
	}

	int _nTargetFrameRate = 0;
	bool bWindowed = true;
	int numerator = 0;
	int denominator = 0;
	D3D_FEATURE_LEVEL featureLevelsD3D11ONLY[] =
	{
		D3D_FEATURE_LEVEL_11_1,
	};
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	g_featureLevel = D3D_FEATURE_LEVEL_11_0;
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory ( &sd, sizeof( sd ) );
	sd.BufferCount							= 1;
	sd.BufferDesc.Width						= m_iWidth;
	sd.BufferDesc.Height					= m_iHeight;
	sd.BufferDesc.Format					= GGFMT_A8R8G8B8;
	if (_nTargetFrameRate <= 0)
	{
		sd.BufferDesc.RefreshRate.Numerator		= 0;
		sd.BufferDesc.RefreshRate.Denominator	= 1;
		sd.SwapEffect							= DXGI_SWAP_EFFECT_DISCARD;
	}
	else
	{
		sd.BufferDesc.RefreshRate.Numerator		= numerator;
		sd.BufferDesc.RefreshRate.Denominator	= denominator;
		sd.SwapEffect							= DXGI_SWAP_EFFECT_DISCARD;
	}
	if ( bWindowed )
		sd.Flags = 0 ;
	else
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage							= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow							= m_hWnd;
	sd.SampleDesc.Count						= 1;
	sd.SampleDesc.Quality					= 0;
	sd.Windowed								= bWindowed;
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif
	HRESULT hr;
	m_pSwapChain[0] = NULL;
	if ( m_iForceAdapterD3D11ONLY == 1 )
	{
		hr = D3D11CreateDeviceAndSwapChain(	pAdapter, adapterType, NULL, creationFlags, featureLevelsD3D11ONLY, ARRAYSIZE(featureLevelsD3D11ONLY),
												D3D11_SDK_VERSION, &sd, &m_pSwapChain[0], &m_pD3D,
												&g_featureLevel, &m_pImmediateContext );
		if( FAILED( hr ) )
		{
			// if it fails, revert to more common device feature level (some DX12 PCs don't have D3D_FEATURE_LEVEL_11_1)
			hr = D3D11CreateDeviceAndSwapChain(	pAdapter, adapterType, NULL, creationFlags, featureLevels, ARRAYSIZE(featureLevels),
													D3D11_SDK_VERSION, &sd, &m_pSwapChain[0], &m_pD3D,
													&g_featureLevel, &m_pImmediateContext );
		}
	}
	else
	{
		hr = D3D11CreateDeviceAndSwapChain(	pAdapter, adapterType, NULL, creationFlags, featureLevels, ARRAYSIZE(featureLevels),
												D3D11_SDK_VERSION, &sd, &m_pSwapChain[0], &m_pD3D,
												&g_featureLevel, &m_pImmediateContext );
	}
	if( FAILED( hr ) )
	{
		if ( hr == D3D11_ERROR_FILE_NOT_FOUND ) Error1 ( "D3D11CreateDeviceAndSwapChain = D3D11_ERROR_FILE_NOT_FOUND\n" );
		else if ( hr == D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS ) Error1 ( "D3D11CreateDeviceAndSwapChain = D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS\n" );
		else if ( hr == D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS ) Error1 ( "D3D11CreateDeviceAndSwapChain = D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS\n" );
		else if ( hr == D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD ) Error1 ( "D3D11CreateDeviceAndSwapChain = D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD\n" );
		else if ( hr == D3DERR_WASSTILLDRAWING ) Error1 ( "D3D11CreateDeviceAndSwapChain = D3DERR_WASSTILLDRAWING\n" );
		else if ( hr == D3DERR_INVALIDCALL ) Error1 ( "D3D11CreateDeviceAndSwapChain = D3DERR_INVALIDCALL\n" );
		else if ( hr == E_FAIL ) Error1 ( "D3D11CreateDeviceAndSwapChain = E_FAIL\n" );
		else if ( hr == E_INVALIDARG ) Error1 ( "D3D11CreateDeviceAndSwapChain = E_INVALIDARG\n" );
		else if ( hr == E_OUTOFMEMORY ) Error1 ( "D3D11CreateDeviceAndSwapChain = E_OUTOFMEMORY\n" );
		else if ( hr == S_FALSE ) Error1 ( "D3D11CreateDeviceAndSwapChain = S_FALSE\n" );
		else
		{
			char szOut [ 256 ] = "";
			sprintf ( szOut, "Cannot initialize DirectX 11" );
			Error1 ( szOut );
		}
		Error1 ( "Failed to D3D11CreateDeviceAndSwapChain\n" );
		return false;
	}

	// Set Full screen state (or not)
	if ( m_pSwapChain[0] )
	{
		m_pSwapChain[0]->SetFullscreenState ( !bWindowed, NULL );
		// Create back buffer and depth buffer (with views) from swapchain
		if ( GetBackBufferAndDepthBuffer() == false )
			return false;
	}

	// Create the depth stencil STATE for 3D rendering
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	//depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS; // when did this, terrain in cubemap stopped working (suggests no depth values being used!) // PE: works now.
	depthStencilDesc.StencilEnable = false;//true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	hr = m_pD3D->CreateDepthStencilState(&depthStencilDesc, &m_pDepthStencilState);
	if( FAILED( hr ) )
	{
		Error1 ( "Failed to CreateDepthStencilState\n" );
		return false;
	}

	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = false;//true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	hr = m_pD3D->CreateDepthStencilState(&depthStencilDesc, &m_pDepthNoWriteStencilState);
	if( FAILED( hr ) )
	{
		Error1 ( "Failed to CreateDepthStencilState\n" );
		return false;
	}

	// Create the depth stencil STATE for 2D rendering
	D3D11_DEPTH_STENCIL_DESC depthDisabledStencilDesc;
	ZeroMemory(&depthDisabledStencilDesc, sizeof(depthDisabledStencilDesc));
	depthDisabledStencilDesc.DepthEnable = false;
	depthDisabledStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDisabledStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthDisabledStencilDesc.StencilEnable = false;//true;
	depthDisabledStencilDesc.StencilReadMask = 0xFF;
	depthDisabledStencilDesc.StencilWriteMask = 0xFF;
	depthDisabledStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthDisabledStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthDisabledStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthDisabledStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	hr = m_pD3D->CreateDepthStencilState(&depthDisabledStencilDesc, &m_pDepthDisabledStencilState);

	// Setup the raster description which will determine how and what polygons will be drawn
	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	hr = m_pD3D->CreateRasterizerState(&rasterDesc, &m_pRasterState);
	if( FAILED( hr ) )
	{
		Error1 ( "Failed to m_pRasterState\n" );
		return false;
	}

	// Setup the raster description which will determine how and what polygons will be drawn
	rasterDesc.CullMode = D3D11_CULL_FRONT;
	rasterDesc.DepthBias = -50; //-100;
	rasterDesc.SlopeScaledDepthBias = 0.65f;
	rasterDesc.FrontCounterClockwise = false;
	hr = m_pD3D->CreateRasterizerState(&rasterDesc, &m_pRasterStateTerrainShadow);
	if (FAILED(hr))
	{
		Error1("Failed to m_pRasterStateTerrainShadow\n");
		return false;
	}

	rasterDesc.DepthBias = 2000; //-100;
	rasterDesc.SlopeScaledDepthBias = 1.25f;
	rasterDesc.FrontCounterClockwise = true;
	hr = m_pD3D->CreateRasterizerState(&rasterDesc, &m_pRasterStateShadow);
	if (FAILED(hr))
	{
		Error1("Failed to m_pRasterStateShadow\n");
		return false;
	}

	//As we have a bias for backfaces we need to adjust the bias on object that include frontfaces.
	rasterDesc.DepthBias = 1200; //-100;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.FrontCounterClockwise = false;
	hr = m_pD3D->CreateRasterizerState(&rasterDesc, &m_pRasterStateNoCullShadow);
	if (FAILED(hr))
	{
		Error1("Failed to m_pRasterStateNoCullShadow\n");
		return false;
	}

	rasterDesc.DepthBias = 0;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	rasterDesc.CullMode = D3D11_CULL_NONE;
	hr = m_pD3D->CreateRasterizerState(&rasterDesc, &m_pRasterStateNoCull);
	if( FAILED( hr ) )
	{
		Error1 ( "Failed to m_pRasterStateNoCull\n" );
		return false;
	}
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = (int)DEPTH_BIAS_D32_FLOAT(-0.00005); //(-0.00001);//-100.0f;//-0.6f;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.SlopeScaledDepthBias = 0.0f;//-0.0000005f;
	hr = m_pD3D->CreateRasterizerState(&rasterDesc, &m_pRasterStateDepthBias);
	if( FAILED( hr ) )
	{
		Error1 ( "Failed to m_pRasterStateDepthBias\n" );
		return false;
	}

	// Setup the viewport for rendering
	D3D11_VIEWPORT viewport;
	viewport.Width = (float)m_iWidth;
	viewport.Height =(float)m_iHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	//m_pImmediateContext->RSSetViewports(1, &viewport);
	SetupSetViewport ( 0, &viewport, NULL );

	// Create blend state
	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = m_pD3D->CreateBlendState(&blendDesc, &m_pBlendStateAlpha);
	if( FAILED( hr ) )
	{
		Error1 ( "Failed to m_pBlendStateAlpha\n" );
		return false;
	}
	blendDesc.RenderTarget[0].BlendEnable = false;
	hr = m_pD3D->CreateBlendState(&blendDesc, &m_pBlendStateNoAlpha);
	if( FAILED( hr ) )
	{
		Error1 ( "Failed to m_pBlendStateNoAlpha\n" );
		return false;
	}
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
	hr = m_pD3D->CreateBlendState(&blendDesc, &m_pBlendStateShadowBlend);
	if( FAILED( hr ) )
	{
		Error1 ( "Failed to m_pBlendStateShadowBlend\n" );
		return false;
	}

	#endif
	return true;
}

DARKSDK void SetupSetViewport ( int iCameraID, D3D11_VIEWPORT* originalvp, LPGGSURFACE pSurface )
{
	D3D11_VIEWPORT vp = *originalvp;
	if ( pSurface ) 
	{
		D3D11_TEXTURE2D_DESC ddsd;
		pSurface->GetDesc ( &ddsd );
		vp.Width=ddsd.Width;
		vp.Height=ddsd.Height;
	}
    m_pImmediateContext->RSSetViewports( 1, &vp );
}

DARKSDK bool SetupDX9 ( void )
{
	#ifndef DX11
	// flag for backbuffer
	m_dwFlags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	// setup the display mode using the preset values, this function is only called internally
	if ( !m_pDX )
	{
		Error ( "Invalid D3D pointer for Setup ( )" );
		RunTimeError(RUNTIMEERROR_NOTSUPPORTDISPLAYNODX);
		return false;
	}

	// check for other adapters (perhaps use later)
	UINT uNumberOfAdapters = m_pDX->GetAdapterCount();
	for ( UINT uI=0; uI<uNumberOfAdapters; uI++ )
	{
		D3DADAPTER_IDENTIFIER9 identifier;
		HRESULT hRes = m_pDX->GetAdapterIdentifier ( uI, 0, &identifier );
	}
	m_uAdapterChoice = GGADAPTER_DEFAULT;

	// Reduce overhead on system by discarding buffer after use (must refresh each SYNC)
	if ( m_iMultisamplingFactor > 0 )
	{
		// multisampling requires a DISCARDable backbuffer
		m_SwapMode = D3DSWAPEFFECT_DISCARD;

		// Cannot lock backbuffer for multisampled devices so switch D3DPRESENTFLAG_LOCKABLE_BACKBUFFER ''off''
		m_dwFlags = 0;
	}
	else
	{
		// default for non 3D/sprite/grabbing backbuffer
		m_SwapMode = D3DSWAPEFFECT_COPY;
	}

	// test for flip approach
	if ( g_bWindowOverride )
		m_SwapMode = D3DSWAPEFFECT_COPY;
	else
		m_SwapMode = D3DSWAPEFFECT_FLIP;
	
	// this will get switched off if we can't find a stencil buffer
	m_bZBuffer = true;

	// Free before setting properties
	if ( m_D3DPP )
	{
		delete m_D3DPP;
		m_D3DPP=NULL;
	}

	if ( !m_D3DPP ) 
	{
		m_D3DPP = new GGPRESENT_PARAMETERS;
		if ( !m_D3DPP ) Error ( "Failed to allocate display mode parameters" );
		memset ( m_D3DPP, 0, sizeof ( GGPRESENT_PARAMETERS ) );
		m_D3DPP->SwapEffect				     = m_SwapMode;
		m_D3DPP->BackBufferCount			 = m_iBackBufferCount;
		m_D3DPP->EnableAutoDepthStencil		 = m_bZBuffer;
		m_D3DPP->FullScreen_RefreshRateInHz  = D3DPRESENT_RATE_DEFAULT;
		m_D3DPP->Flags                       = m_dwFlags;

		// can have different backbuffer size if flagged
		if ( m_iModBackbufferWidth != 0 )
		{
			// use this instead of '799 m_bUseDeskTopBB resolution trick'
			m_D3DPP->BackBufferWidth             = m_iModBackbufferWidth;
			m_D3DPP->BackBufferHeight            = m_iModBackbufferHeight;
		}
		else
		{
			// default is backbuffer matches resolution
			m_D3DPP->BackBufferWidth             = m_iWidth;
			m_D3DPP->BackBufferHeight            = m_iHeight;
		}
		
		// if VSYNC flagged, allow antialiasing of device up-to device max.
		if ( m_iMultisamplingFactor > 0 )
		{
			// allow anti-aliasing of device (if supported)
			DWORD dwQualityLevels = 0;
			m_D3DPP->MultiSampleType = (D3DMULTISAMPLE_TYPE)m_iMultisamplingFactor;

			// check if multisampling is available for device
			if( SUCCEEDED(m_pDX->CheckDeviceMultiSampleType(	m_uAdapterChoice, D3DDEVTYPE_HAL, m_WindowsD3DMODE.Format,
																TRUE, m_D3DPP->MultiSampleType, &dwQualityLevels ) ) )
			{
				// set quality level of multisampling we found
				m_D3DPP->MultiSampleQuality			= dwQualityLevels-1;
			}
			else
			{
				// fall back to no multisampling
				m_D3DPP->MultiSampleType			= D3DMULTISAMPLE_NONE;
				m_D3DPP->MultiSampleQuality			= 0;
			}
		}
		else
		{
			// default behaviour
			m_D3DPP->MultiSampleType			= D3DMULTISAMPLE_NONE;
			m_D3DPP->MultiSampleQuality			= 0;
		}
		
		if ( m_iDisplayType == FULLSCREEN )
		{
			GetValidBackBufferFormat ( );
			GetStencilDepth ( );
			m_D3DPP->AutoDepthStencilFormat		= m_StencilDepth;
			m_D3DPP->Windowed					= false;
			m_D3DPP->BackBufferFormat			= m_Depth;
			m_D3DPP->FullScreen_RefreshRateInHz = 0;
			m_D3DPP->BackBufferFormat           = m_Depth;
		}
		else
		{
			m_Depth									 = m_WindowsD3DMODE.Format;
			m_D3DPP->Windowed                        = true;
			m_D3DPP->BackBufferFormat                = m_WindowsD3DMODE.Format;
			m_D3DPP->PresentationInterval			 = D3DPRESENT_INTERVAL_DEFAULT;
			GetStencilDepth ( );
			m_D3DPP->AutoDepthStencilFormat = m_StencilDepth;
			if ( m_StencilDepth == GGFMT_UNKNOWN )
				Error ( "Failed to find valid stencil buffer" );
		}

		// unified VSYNC handler
		if ( m_bVSync )
		{
			// CAP TO MONITOR REFRESH RATE - NO TEARING
			m_D3DPP->PresentationInterval = D3DPRESENT_INTERVAL_ONE;
			if ( m_iVSyncInterval==2 ) m_D3DPP->PresentationInterval = D3DPRESENT_INTERVAL_TWO;
			if ( m_iVSyncInterval==3 ) m_D3DPP->PresentationInterval = D3DPRESENT_INTERVAL_THREE;
		}
		else
		{
			// FAST AS YOU CAN - HAS HORIZ TEARING
			m_D3DPP->PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		}
	}

	// MAIN CREATE FUNCTION
	if ( CreateDX9 ( m_hWnd, m_D3DPP )!=1 )
	{
		// Runtime errors generated witin Create function
		return false;
	}

	// Fail if no device created
	if(m_pD3D==NULL)
	{
		RunTimeError(RUNTIMEERROR_NOTSUPPORTDISPLAYINVALID);
		return false;
	}
	#endif
	return true;
}

DARKSDK bool Setup ( void )
{
	#ifdef DX11
	return SetupDX11();
	#else
	return SetupDX9();
	#endif
}

DARKSDK GGFORMAT GetDepthFormatFromDisplaySetting ( int Width, int Height, int Depth )
{
	GGFORMAT m_Depth = GGFMT_A8R8G8B8;
	#ifdef DX11
	#else
	for ( int iTemp = 0; iTemp < m_pInfo [ 0 ].iDisplayCount; iTemp++ )
	{
		if	(	m_pInfo [ 0 ].D3DDisplay [ iTemp ].Width  == (DWORD)Width &&
				m_pInfo [ 0 ].D3DDisplay [ iTemp ].Height == (DWORD)Height )
		{
			if ( Depth == GetBitDepthFromFormat ( m_pInfo [ 0 ].D3DDisplay [ iTemp ].Format ) )
			{
				m_Depth = m_pInfo [ 0 ].D3DDisplay [ iTemp ].Format;
				continue;
			}
		}
	}
	#endif
	return m_Depth;
}

DARKSDK void GetValidBackBufferFormat ( )
{
	// find a suitable format for the backbuffer, note
	// that earlier on we stored a list of all the supported
	// display modes, now we're simply running through each
	// one to find a suitable match

	// Get available depth format from settings
	m_Depth = GetDepthFormatFromDisplaySetting ( m_iWidth, m_iHeight, m_iDepth );
}

DARKSDK void GetStencilDepth ( void )
{
	// create the list in order of precedence
	#ifdef DX11
	#else
	GGFORMAT	list [ ] =
							{
								GGFMT_D24S8, //GeForce4 top choice
								GGFMT_R8G8B8,
								GGFMT_A8R8G8B8,
								GGFMT_X8R8G8B8,
								GGFMT_R5G6B5,
								GGFMT_X1R5G5B5,
								GGFMT_A1R5G5B5,
								GGFMT_A4R4G4B4,
								GGFMT_R3G3B2,
								GGFMT_A8,
								GGFMT_A8R3G3B2,
								GGFMT_X4R4G4B4,
								GGFMT_A8P8,
								GGFMT_P8,
								GGFMT_L8,
								GGFMT_A8L8,
								GGFMT_A4L4,
								GGFMT_V8U8,
								GGFMT_L6V5U5,
								GGFMT_X8L8V8U8,
								GGFMT_Q8W8V8U8,
								GGFMT_V16U16,
								GGFMT_D16_LOCKABLE,
								GGFMT_D32,
								GGFMT_D15S1,
								GGFMT_D16,
								GGFMT_D24X8,
								GGFMT_D24X4S4,
								GGFMT_D24FS8,
								GGFMT_D32F_LOCKABLE,
								GGFMT_D32_LOCKABLE,
								GGFMT_S8_LOCKABLE
							};

	for ( int iTemp = 0; iTemp < 32; iTemp++ )
	{
		// Verify that the depth format exists first
		if ( SUCCEEDED ( m_pDX->CheckDeviceFormat( m_uAdapterChoice,
													D3DDEVTYPE_HAL,
													m_Depth,
													D3DUSAGE_DEPTHSTENCIL,
													D3DRTYPE_SURFACE,
													list [ iTemp ]						) ) )
		{
			if ( SUCCEEDED ( m_pDX->CheckDepthStencilMatch	(	m_uAdapterChoice,
																D3DDEVTYPE_HAL,
																m_Depth,
																m_Depth,
																list [ iTemp ]				) ) )
			{
				m_StencilDepth = list [ iTemp ];
				break;
			}
		}
	}
	#endif
}

DARKSDK int CreateDX9 ( HWND hWnd, GGPRESENT_PARAMETERS* d3dpp )
{
	#ifndef DX11
	// 190315 - trace display start-up issues
	char pDisplayErrTrace[2048];
	strcpy ( pDisplayErrTrace, "" );

	HRESULT			hr;
	GGDISPLAYMODE	d3dmode;
	memset ( &d3dmode, 0, sizeof ( d3dmode  ) );
	m_pDX->GetAdapterDisplayMode ( 0, &d3dmode );
	wsprintf ( pDisplayErrTrace, "%sW%dH%dD%dR%d", pDisplayErrTrace, d3dmode.Width, d3dmode.Height, d3dmode.Format, d3dmode.RefreshRate );

	// End scene before any release (just in case)
	End();
	SAFE_RELEASE ( m_pD3D );
	
	// Assign new window handle to D3DPP
	d3dpp->hDeviceWindow = hWnd;

	// can setup app for viewing in PerfHUD
	D3DDEVTYPE pDevType = D3DDEVTYPE_HAL;
	if ( m_bNVPERFHUD==true )
	{
		// U74BETA9 - 010709 - can switch on PERFHUD for next SET DISPLAY MODE call
		m_uAdapterChoice = m_pDX->GetAdapterCount()-1;
		pDevType = D3DDEVTYPE_REF;
	}
	else
	{
		if ( m_iForceAdapterOrdinal>0 )
		{
			m_uAdapterChoice = m_iForceAdapterOrdinal;
		}
	}
	if ( pDevType==D3DDEVTYPE_HAL ) wsprintf ( pDisplayErrTrace, "%s HAL", pDisplayErrTrace );
	if ( pDevType==D3DDEVTYPE_REF ) wsprintf ( pDisplayErrTrace, "%s REF", pDisplayErrTrace );

	// no depth format detected, default to at GGFMT_D24S8 (standard)
	if ( d3dpp->AutoDepthStencilFormat==0 )
	{
		d3dpp->AutoDepthStencilFormat = GGFMT_D24X8;
		wsprintf ( pDisplayErrTrace, "%sGGFMT_D24X8_ADDED", pDisplayErrTrace );
	}

	// create device
	wsprintf ( pDisplayErrTrace, "%s A=%d P=%d D£DPP=%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d", pDisplayErrTrace, m_uAdapterChoice, m_iProcess, d3dpp->BackBufferCount, d3dpp->BackBufferWidth, d3dpp->BackBufferHeight, d3dpp->AutoDepthStencilFormat, d3dpp->BackBufferFormat, d3dpp->EnableAutoDepthStencil, d3dpp->Flags, d3dpp->FullScreen_RefreshRateInHz, d3dpp->hDeviceWindow, d3dpp->MultiSampleQuality, d3dpp->MultiSampleType, d3dpp->PresentationInterval, d3dpp->SwapEffect, d3dpp->Windowed );
	if(g_pGlob) g_pGlob->iSoftwareVP = 0;
	if ( FAILED ( hr = m_pDX->CreateDevice (	m_uAdapterChoice,						// use default adapter
												pDevType,								// hardware mode
												hWnd,									// handle to window
												m_iProcess,
												d3dpp,									// display info
												&m_pD3D							// pointer to device
												) ) )
	{
		// try again to create device (with software processing)
		if ( FAILED ( hr = m_pDX->CreateDevice
										(
											m_uAdapterChoice,						// use default adapter
											D3DDEVTYPE_HAL,							// hardware mode
											hWnd,									// handle to window
											//D3DCREATE_SOFTWARE_VERTEXPROCESSING,	// software processing //040414 - oops!!
											D3DCREATE_HARDWARE_VERTEXPROCESSING,	// software processing
											d3dpp,									// display info
											&m_pD3D							// pointer to device
										) ) )
		{
			if ( hr==D3DERR_INVALIDCALL ) hr=1;
			if ( hr==D3DERR_NOTAVAILABLE  ) hr=2;
			if ( hr==D3DERR_OUTOFVIDEOMEMORY  ) hr=3;
			wsprintf ( pDisplayErrTrace, "%s HR=%d", pDisplayErrTrace, hr );
			MessageBox ( NULL, pDisplayErrTrace, "Display Mode Error", MB_OK | MB_TOPMOST );

			Error ( "Unable to create device" );
			if(hr==D3DERR_INVALIDCALL) RunTimeError(RUNTIMEERROR_NOTSUPPORTDISPLAYINVALID);
			if(hr==D3DERR_NOTAVAILABLE) RunTimeError(RUNTIMEERROR_NOTSUPPORTDISPLAYNOTAVAIL);
			if(hr==D3DERR_OUTOFVIDEOMEMORY) RunTimeError(RUNTIMEERROR_NOTSUPPORTDISPLAYNOVID);
			return 0;
		}
		else
		{
			// Device will use software VP
			if(g_pGlob) g_pGlob->iSoftwareVP = 1;
		}
	}

	// HWND can be overwritten
	if ( g_bWindowOverride ) g_pGlob->hWnd = g_OldHwnd;

	// setup render states for initial device
	m_pD3D->SetSamplerState ( 0, D3DSAMP_MAGFILTER, GGTEXF_LINEAR );	// mip mapping
	m_pD3D->SetSamplerState ( 0, D3DSAMP_MINFILTER, GGTEXF_LINEAR );	// mip mapping
	m_pD3D->SetSamplerState ( 0, D3DSAMP_MIPFILTER, GGTEXF_LINEAR );	// mip mapping
	m_pD3D->SetRenderState ( D3DRS_DITHERENABLE,   TRUE             );		// enable dither
	m_pD3D->SetRenderState ( D3DRS_ZENABLE,        TRUE             );		// enable z buffer
	m_pD3D->SetRenderState ( D3DRS_SHADEMODE,      D3DSHADE_GOURAUD );		// set shade mode to gourad shading
	m_pD3D->SetRenderState ( D3DRS_SPECULARENABLE, FALSE            );		// turn off specular highlights
	m_pD3D->SetRenderState ( D3DRS_LIGHTING,       TRUE             );		// turn lighting on
	m_pD3D->SetRenderState ( D3DRS_CULLMODE,       D3DCULL_NONE     );		// set cull mode to none
	m_pD3D->SetRenderState ( D3DRS_AMBIENT, GGCOLOR_ARGB ( 255, 50, 50, 50 ) );		// set ambient light on

	// CLEAR SCREEN STRAIGHT AWAY SO NO ARTIFACTS
	SETUPClear ( 0, 0, 0 );
	#endif

	// success
	return 1;
}

DARKSDK void SETUPClear ( int iR, int iG, int iB )
{
	#ifdef DX11
	if ( g_pGlob->pCurrentRenderView )
	{
		// clear the screen to the specified colour
		float ClearColor[4];
		ClearColor[0] = iR/255.0f;
		ClearColor[1] = iG/255.0f;
		ClearColor[2] = iB/255.0f;
		ClearColor[3] = 1.0f;
		if(m_bZBuffer==false)
		{
			// clear the full screen
			m_pImmediateContext->ClearRenderTargetView(g_pGlob->pCurrentRenderView, ClearColor);
		}
		else
		{
			// clear the full screen and the zbuffer
			m_pImmediateContext->ClearRenderTargetView(g_pGlob->pCurrentRenderView, ClearColor);
			m_pImmediateContext->ClearDepthStencilView(g_pGlob->pCurrentDepthView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0 );
		}
	}
	#else
	// clear the screen to the specified colour
	if(m_bZBuffer==false)
	{
		// clear the full screen
		m_pD3D->Clear ( 0, NULL, D3DCLEAR_TARGET, GGCOLOR_XRGB ( iR, iG, iB ), 1.0f, 0 );
	}
	else
	{
		// clear the full screen and the zbuffer
		m_pD3D->Clear ( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, GGCOLOR_XRGB ( iR, iG, iB ), 1.0f, 0 );
	}
	#endif
}

DARKSDK void SETUPClearEx (int iR, int iG, int iB, int iA)
{
	if (g_pGlob->pCurrentRenderView)
	{
		// clear the screen to the specified colour
		float ClearColor[4];
		ClearColor[0] = iR / 255.0f;
		ClearColor[1] = iG / 255.0f;
		ClearColor[2] = iB / 255.0f;
		ClearColor[3] = iA / 255.0f;
		if (m_bZBuffer == false)
		{
			// clear the full screen
			m_pImmediateContext->ClearRenderTargetView(g_pGlob->pCurrentRenderView, ClearColor);
		}
		else
		{
			// clear the full screen and the zbuffer
			m_pImmediateContext->ClearRenderTargetView(g_pGlob->pCurrentRenderView, ClearColor);
			m_pImmediateContext->ClearDepthStencilView(g_pGlob->pCurrentDepthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		}
	}
}

DARKSDK void GetGamma ( int* piR, int* piG, int* piB )
{
	// retrieve the current gamma settings

	// check all of the pointers are valid
	if ( !piR || !piG || !piB )
		Error1 ( "Invalid pointers passed to GetGamma" );

	// now assign the gamma values
	*piR = m_iGammaRed;			// copy red
	*piG = m_iGammaGreen;		// copy green
	*piB = m_iGammaBlue;		// copy blue
}

DARKSDK bool CheckDisplayMode ( int iWidth, int iHeight )
{
	#ifdef DX11
	#else
	int iTemp;
	for ( iTemp = 0; iTemp < m_pInfo [ 0 ].iDisplayCount; iTemp++ )
	{
		if ( m_pInfo [ 0 ].D3DDisplay [ iTemp ].Width == (DWORD)iWidth && m_pInfo [ 0 ].D3DDisplay [ iTemp ].Height == (DWORD)iHeight )
			return true;
	}
	#endif
	return false;
}

DARKSDK int CheckDisplayModeANTIALIAS ( int iWidth, int iHeight, int iDepth, int iVSyncOn, int iMultisamplingFactor, int iMultimonitorMode )
{
	// extra check if VSYNC supported, and if MULTISAMPLING supported
	// no check vsync - assume all devices support D3DPRESENT_INTERVAL_ONE and D3DPRESENT_INTERVAL_IMMEDIATE
	// no multimonitor check - assume double wide resolution (typical S3D monitor)
	// check multisampling
	if ( iMultisamplingFactor > 0 )
	{
		#ifdef DX11
		#else
		BOOL bWindowModeFlag = TRUE;
		GGFORMAT d3dDeviceFormat = m_WindowsD3DMODE.Format;
		if ( m_iDisplayType == FULLSCREEN )
		{
			bWindowModeFlag = FALSE;
			d3dDeviceFormat = m_Depth;
		}
		if ( SUCCEEDED(m_pDX->CheckDeviceMultiSampleType(	m_uAdapterChoice, D3DDEVTYPE_HAL, d3dDeviceFormat,
															bWindowModeFlag, (D3DMULTISAMPLE_TYPE)iMultisamplingFactor, NULL ) ) )
			return true;
		else
			return false;
		#endif
	}

	// regular check on W,H,D
	return CheckDisplayMode ( iWidth, iHeight, iDepth );
}

DARKSDK bool CheckDisplayMode ( int iWidth, int iHeight, int iDepth, int iMode )
{
	if ( CheckDisplayMode ( iWidth, iHeight, iDepth ) )
	{
		GetValidBackBufferFormat ( );
		#ifdef DX11
		#else
		if ( m_pDX )
		{
			if ( SUCCEEDED ( m_pDX->CheckDeviceType
														( 
															0, 
															D3DDEVTYPE_HAL,
															m_Depth,
															m_Depth,
															iMode 
														) ) )
				return true;
		}
		#endif
	}

	return false;
}

DARKSDK bool CheckDisplayMode ( int iWidth, int iHeight, int iDepth, int iMode, int iVertexProcessing )
{
	if ( CheckDisplayMode ( iWidth, iHeight, iDepth, iMode ) )
	{
		#ifdef DX11
		#else
		GGCAPS	d3dCaps;
		memset ( &d3dCaps, 0, sizeof ( d3dCaps ) );
		m_pD3D->GetDeviceCaps ( &d3dCaps );
		if ( iVertexProcessing == 0 && ( d3dCaps.Caps & D3DDEVCAPS_TLVERTEXSYSTEMMEMORY ) )
			return true;
		if ( iVertexProcessing == 1 && ( d3dCaps.Caps & D3DDEVCAPS_TLVERTEXVIDEOMEMORY ) )
			return true;
		#endif
	}

	return false;
}

DARKSDK bool GetWindowedMode ( void )
{
	// returns true if the app is running in windowed mode

	// check the display mode we're in
	if ( m_iDisplayType == 1 )
		return true;
	else
		return false;
}

DARKSDK int GetNumberOfDisplayModes ( void )
{
	// return the number of display modes available
	#ifdef DX11
	return 1;
	#else
	return m_pDX->GetAdapterModeCount ( 0, g_GGFORMAT );
	#endif
}

DARKSDK void GetDisplayMode ( int iID, char* szMode )
{
	// variable declarations
	char	szResolution [ 256 ];	// used to store the main bulk of the string
	
	// check the string passed is valid
	if ( !szMode )
		return;

	// clear out the resolution string
	memset ( szResolution, 0, sizeof ( szResolution ) );
	
	// check the format of the mode
	#ifdef DX11
	#else
	int		iDepth;					// stores the resolution depth
	if ( 
			m_pInfo [ 0 ].D3DDisplay [ iID ].Format == GGFMT_X8R8G8B8 ||
			m_pInfo [ 0 ].D3DDisplay [ iID ].Format == GGFMT_A8R8G8B8 ||
			m_pInfo [ 0 ].D3DDisplay [ iID ].Format == GGFMT_R8G8B8
	   )
	{
		// we're dealing with a 32 bit mode
		iDepth = 32;
	}
	else
	{
		// we're dealing with a 16 bit mode
		iDepth = 16;
	}

	// finally build up the string
	sprintf ( szResolution, ("%ld x %ld x %ld"), m_pInfo [ 0 ].D3DDisplay [ iID ].Width, m_pInfo [ 0 ].D3DDisplay [ iID ].Height, iDepth );

	// and now copy the resolution information across
	strcpy ( szMode, szResolution );
	#endif
}

DARKSDK int GetNumberOfDisplayDevices ( void )
{
	#ifdef DX11
	return 1;
	#else
	return m_pDX->GetAdapterCount ( );
	#endif
}

DARKSDK void SetDisplayDevice ( int iID )
{
}

DARKSDK void GetDeviceName ( int iID, char* szDevice )
{
	// gets the name of the selected device
	#ifdef DX11
	#else
	// declare a structure which will contain device information
	D3DADAPTER_IDENTIFIER9 identifier;

	// check that D3D has been initialised and that the pointer
	// passed to the function is valid
	if ( !m_pDX || !szDevice )
		return;

	// clear out the structure
	memset ( &identifier, 0, sizeof ( identifier ) );

	// get info about the device
	m_pDX->GetAdapterIdentifier ( iID, 0, &identifier );

	// now check that the call succeeded, if it did
	// copy the name across to the pointer
	if ( identifier.Description )
		strcpy ( szDevice, identifier.Description );
	#endif
}

DARKSDK void GetDeviceDriverName ( int iID, char* szDriver )
{
	// gets the name of the driver for the selected device
	#ifdef DX11
	#else
	// declare a structure which will contain device information
	D3DADAPTER_IDENTIFIER9 identifier;

	// check that D3D has been initialised and that the pointer
	// passed to the function is valid
	if ( !m_pDX || !szDriver )
		return;

	// clear out the structure
	memset ( &identifier, 0, sizeof ( identifier ) );

	// now get info about the device
	m_pDX->GetAdapterIdentifier ( iID, 0, &identifier );

	// now check that the call succeeded, if it did
	// copy the driver name across to the pointer
	if ( identifier.Driver )
		strcpy ( szDriver, identifier.Driver );
	#endif
}

DARKSDK void SetDitherMode ( int iMode )
{
	#ifdef DX11
	#else
	m_pD3D->SetRenderState ( D3DRS_DITHERENABLE, iMode );
	#endif
}

DARKSDK void SetShadeMode ( int iMode )
{
	#ifdef DX11
	#else
	m_pD3D->SetRenderState ( D3DRS_SHADEMODE, iMode );
	#endif
}

DARKSDK void SetLightMode ( int iMode )
{
	#ifdef DX11
	#else
	m_pD3D->SetRenderState ( D3DRS_LIGHTING, iMode );
	#endif
}

DARKSDK void SetCullMode ( int iMode )
{
	#ifdef DX11
	#else
	m_pD3D->SetRenderState ( D3DRS_CULLMODE, iMode );
	#endif
}

DARKSDK void SetSpecularMode ( int iMode )
{
	#ifdef DX11
	#else
	m_pD3D->SetRenderState ( D3DRS_SPECULARENABLE, iMode );
	#endif
}

DARKSDK void SetRenderState ( int iState, int iValue )
{
	#ifdef DX11
	#else
	m_pD3D->SetRenderState ( ( D3DRENDERSTATETYPE ) iState, iValue );
	#endif
}

DARKSDK LPGG GetDirect3D ( void )
{
	return m_pDX;
}

DARKSDK float __stdcall Timer ( TIMER_COMMAND command )
{
	// sets up a timer, flags can be used to perform the
	// following operations -
	//          TIMER_RESET           - to reset the timer
	//          TIMER_START           - to start the timer
	//          TIMER_STOP            - to stop ( or pause ) the timer
	//          TIMER_ADVANCE         - to advance the timer by 0.1 seconds
	//          TIMER_GETABSOLUTETIME - to get the absolute system time
	//          TIMER_GETAPPTIME      - to get the current time
	//          TIMER_GETELAPSEDTIME  - to get the time that elapsed between 
	//                                  TIMER_GETELAPSEDTIME calls

    static BOOL     m_bTimerInitialized = FALSE;
    static BOOL     m_bUsingQPF         = FALSE;
    static LONGLONG m_llQPFTicksPerSec  = 0;

    // initialize the timer
    if ( FALSE == m_bTimerInitialized )
    {
        m_bTimerInitialized = TRUE;

        // use QueryPerformanceFrequency ( ) to get frequency of timer. If QPF is
        // not supported, we will timeGetTime ( ) which returns milliseconds.
        LARGE_INTEGER qwTicksPerSec;

        m_bUsingQPF = QueryPerformanceFrequency ( &qwTicksPerSec );

        if ( m_bUsingQPF )
            m_llQPFTicksPerSec = qwTicksPerSec.QuadPart;
    }

    if ( m_bUsingQPF )
    {
        static LONGLONG m_llStopTime        = 0;
        static LONGLONG m_llLastElapsedTime = 0;
        static LONGLONG m_llBaseTime        = 0;
        double fTime;
        double fElapsedTime;
        LARGE_INTEGER qwTime;
        
        // get either the current time or the stop time, depending
        // on whether we're stopped and what command was sent
        if ( m_llStopTime != 0 && command != TIMER_START && command != TIMER_GETABSOLUTETIME )
            qwTime.QuadPart = m_llStopTime;
        else
            QueryPerformanceCounter ( &qwTime );

        // return the elapsed time
        if ( command == TIMER_GETELAPSEDTIME )
        {
            fElapsedTime = ( double ) ( qwTime.QuadPart - m_llLastElapsedTime ) / ( double ) m_llQPFTicksPerSec;
            m_llLastElapsedTime = qwTime.QuadPart;
            return ( FLOAT ) fElapsedTime;
        }
    
        // return the current time
        if ( command == TIMER_GETAPPTIME )
        {
            double fAppTime = ( double ) ( qwTime.QuadPart - m_llBaseTime ) / ( double ) m_llQPFTicksPerSec;
            return ( FLOAT ) fAppTime;
        }
    
        // reset the timer
        if ( command == TIMER_RESET )
        {
            m_llBaseTime        = qwTime.QuadPart;
            m_llLastElapsedTime = qwTime.QuadPart;
            return 0.0f;
        }
    
        // start the timer
        if ( command == TIMER_START )
        {
            m_llBaseTime       += qwTime.QuadPart - m_llStopTime;
            m_llStopTime        = 0;
            m_llLastElapsedTime = qwTime.QuadPart;
            return 0.0f;
        }
    
        // stop the timer
        if ( command == TIMER_STOP )
        {
            m_llStopTime        = qwTime.QuadPart;
            m_llLastElapsedTime = qwTime.QuadPart;
            return 0.0f;
        }
    
        // advance the timer by 1/10th second
        if ( command == TIMER_ADVANCE )
        {
            m_llStopTime += m_llQPFTicksPerSec / 10;
            return 0.0f;
        }

        if ( command == TIMER_GETABSOLUTETIME )
        {
            fTime = qwTime.QuadPart / ( double ) m_llQPFTicksPerSec;
            return ( FLOAT ) fTime;
        }

		// invalid command specified
        return -1.0f;
    }
    else
    {
        // get the time using timeGetTime()
        static double m_fLastElapsedTime  = 0.0;
        static double m_fBaseTime         = 0.0;
        static double m_fStopTime         = 0.0;
        double fTime;
        double fElapsedTime;
        
        // get either the current time or the stop time, depending
        // on whether we're stopped and what command was sent
        if ( m_fStopTime != 0.0 && command != TIMER_START && command != TIMER_GETABSOLUTETIME )
            fTime = m_fStopTime;
        else
            fTime = timeGetTime ( ) * 0.001;
    
        // return the elapsed time
        if ( command == TIMER_GETELAPSEDTIME )
        {   
            fElapsedTime = ( double ) ( fTime - m_fLastElapsedTime );
            m_fLastElapsedTime = fTime;
            return ( FLOAT ) fElapsedTime;
        }
    
        // return the current time
        if ( command == TIMER_GETAPPTIME )
        {
            return ( FLOAT ) ( fTime - m_fBaseTime );
        }
    
        // reset the timer
        if ( command == TIMER_RESET )
        {
            m_fBaseTime         = fTime;
            m_fLastElapsedTime  = fTime;
            return 0.0f;
        }
    
        // start the timer
        if ( command == TIMER_START )
        {
            m_fBaseTime        += fTime - m_fStopTime;
            m_fStopTime         = 0.0f;
            m_fLastElapsedTime  = fTime;
            return 0.0f;
        }
    
        // stop the timer
        if ( command == TIMER_STOP )
        {
            m_fStopTime = fTime;
            return 0.0f;
        }
    
        // advance the timer by 1/10th second
        if ( command == TIMER_ADVANCE )
        {
            m_fStopTime += 0.1f;
            return 0.0f;
        }

		// get absolute time
        if ( command == TIMER_GETABSOLUTETIME )
        {
            return ( FLOAT ) fTime;
        }

		// invalid command specified
        return -1.0f;
    }
}

//
// Display Command Functions
//

DARKSDK int CheckDisplayMode ( int iWidth, int iHeight, int iDepth )
{
	#ifdef DX11
	#else
	for ( int iTemp = 0; iTemp < m_pInfo [ 0 ].iDisplayCount; iTemp++ )
	{
		if ( m_pInfo [ 0 ].D3DDisplay [ iTemp ].Width == (DWORD)iWidth && m_pInfo [ 0 ].D3DDisplay [ iTemp ].Height == (DWORD)iHeight )
		{
			GGFORMAT GGFORMAT = m_pInfo [ 0 ].D3DDisplay [ iTemp ].Format;
			if ( iDepth == 16 )
			{
				 if (	GGFORMAT == GGFMT_R5G6B5   || 
						GGFORMAT == GGFMT_X1R5G5B5 ||
						GGFORMAT == GGFMT_A1R5G5B5  )
						return 1;
			}

			if ( iDepth == 32 )
			{
				 if (	GGFORMAT == GGFMT_X8R8G8B8 || 
						GGFORMAT == GGFMT_A8R8G8B8 ||
						GGFORMAT == GGFMT_R8G8B8    )
						return 1;
			}
		}
	}
	#endif
	return 0;
}

DARKSDK LPSTR CurrentGraphicsCard ( void )
{
	// Work string
	strcpy(m_pWorkString, m_pAdapterName);

	// Create and return string
	LPSTR pReturnString=GetReturnStringFromWorkString();
	return pReturnString;
}

DARKSDK int EmulationMode ( void )
{
	// Not Implemented in DBPRO V1 RELEASE
	RunTimeError(RUNTIMEERROR_COMMANDNOWOBSOLETE);
	return 0;
}

DARKSDK void PerformChecklistForDisplayModes ( void )
{
	#ifdef DX11
	#else
	GGDISPLAYMODE* pMode = new GGDISPLAYMODE [ m_pInfo [ m_iAdapterUsed ].iDisplayCount ];
	memcpy ( pMode, m_pInfo [ m_iAdapterUsed ].D3DDisplay, sizeof ( GGDISPLAYMODE ) * m_pInfo [ m_iAdapterUsed ].iDisplayCount );
	for ( int iA = 0; iA < m_pInfo [ m_iAdapterUsed ].iDisplayCount; iA++ )
	{
		for ( int iB = 0; iB < m_pInfo [ m_iAdapterUsed ].iDisplayCount; iB++ )
		{
			if ( iA == iB )
				continue;

			int iWidthA   = pMode [ iA ].Width;
			int iHeightA  = pMode [ iA ].Height;
			int iRefreshA = pMode [ iA ].RefreshRate;

			int iWidthB   = pMode [ iB ].Width;
			int iHeightB  = pMode [ iB ].Height;
			int iRefreshB = pMode [ iB ].RefreshRate;

			if ( iWidthA == iWidthB && iHeightA == iHeightB )
			{
				DWORD BitDepthA = GetBitDepthFromFormat ( pMode [ iA ].Format );
				DWORD BitDepthB = GetBitDepthFromFormat ( pMode [ iB ].Format );

				if ( BitDepthA == BitDepthB )
				{
					pMode [ iA ].Width  = 0;
					pMode [ iA ].Height = 0;
				}
			}
		}
	}

	g_pGlob->checklisthasvalues=true;
	g_pGlob->checklisthasstrings=true;

	g_dwMaxStringSizeInEnum=0;
	g_bCreateChecklistNow=false;
	for(int pass=0; pass<2; pass++)
	{
		if(pass==1)
		{
			// Ensure checklist is large enough
			g_bCreateChecklistNow=true;
			for(int c=0; c<g_pGlob->checklistqty; c++)
				GlobExpandChecklist(c, g_dwMaxStringSizeInEnum);
		}

		// Run through total list of enumerated display modes
		g_pGlob->checklistqty=0;
		for ( int iTemp = 0; iTemp < m_pInfo [ m_iAdapterUsed ].iDisplayCount; iTemp++ )
		{
			int BitDepth=0;
			GGDISPLAYMODE displaymode = pMode [ iTemp ];

			// mike - 230604 - skip invalid modes
			if ( displaymode.Width == 0 && displaymode.Height == 0 )
				continue;

			// These are the only valid render depths
			BitDepth=GetBitDepthFromFormat(displaymode.Format);

			// Only list 16bit+ modes
			if(BitDepth>=16)
			{
				if(g_bCreateChecklistNow)
				{
					g_pGlob->checklist[g_pGlob->checklistqty].valuea = displaymode.Width;
					g_pGlob->checklist[g_pGlob->checklistqty].valueb = displaymode.Height;
					g_pGlob->checklist[g_pGlob->checklistqty].valuec = BitDepth;
					wsprintf(g_pGlob->checklist[g_pGlob->checklistqty].string, "%dx%dx%d", displaymode.Width, displaymode.Height, BitDepth);
				}
				else
				{
					DWORD dwLength=32;//00x00x00
					if(dwLength>g_dwMaxStringSizeInEnum)
						g_dwMaxStringSizeInEnum=dwLength;
				}
				g_pGlob->checklistqty++;
			}
		}
	}
 
	// Determine if checklist has any contents
	if(g_pGlob->checklistqty>0)
		g_pGlob->checklistexists=true;
	else
		g_pGlob->checklistexists=false;
	
	SAFE_DELETE_ARRAY ( pMode );
	#endif
}

DARKSDK void PerformChecklistForGraphicsCards ( void )
{
	// Generate Checklist
	g_pGlob->checklisthasvalues=false;
	g_pGlob->checklisthasstrings=true;

	g_dwMaxStringSizeInEnum=0;
	g_bCreateChecklistNow=false;
	for(int pass=0; pass<2; pass++)
	{
		if(pass==1)
		{
			// Ensure checklist is large enough
			g_bCreateChecklistNow=true;
			for(int c=0; c<g_pGlob->checklistqty; c++)
				GlobExpandChecklist(c, g_dwMaxStringSizeInEnum);
		}

		// Run through total list of enumerated adapter(card) names
		g_pGlob->checklistqty=0;
		for ( int iTemp = 0; iTemp < m_iAdapterCount; iTemp++ )
		{
			if(g_bCreateChecklistNow)
			{
				wsprintf(g_pGlob->checklist[g_pGlob->checklistqty].string, "%s", m_pInfo [ iTemp ].szName);
			}
			else
			{
				DWORD dwLength=strlen(m_pInfo [ iTemp ].szName);
				if(dwLength>g_dwMaxStringSizeInEnum)
					g_dwMaxStringSizeInEnum=dwLength;
			}
			g_pGlob->checklistqty++;
		}
	}
 
	// Determine if checklist has any contents
	if(g_pGlob->checklistqty>0)
		g_pGlob->checklistexists=true;
	else
		g_pGlob->checklistexists=false;
}

DARKSDK int GetDisplayType ( void )
{
	// find out what type of device we're dealing with
	// return true if it's a hardware device

	// variable declarations
	GGCAPS	d3dCaps;	// D3D capabilities structure

	// clear out the structure
	memset ( &d3dCaps, 0, sizeof ( d3dCaps ) );

	// get the device caps, first check the pointer is valid
	if ( !m_pD3D )
		return 0;

	#ifdef DX11
	#else
	m_pD3D->GetDeviceCaps ( &d3dCaps );
	if ( d3dCaps.DeviceType == D3DDEVTYPE_HAL )
		return 1;
	#endif
	return 0;
}

DARKSDK int GetDisplayWidth ( void )
{
#ifdef WICKEDENGINE
	extern bool bImGuiInTestGame;
	if (bImGuiInTestGame)
		return (int)GetSystemMetrics(SM_CXSCREEN);
	else
#endif
		// always return screen width
		return g_pGlob->iScreenWidth;

	// get the width of the display, first off check what mode
	// we're running in as we have to deal with it a bit differently

	if ( m_iDisplayType == 0 )
	{
		// when we're running in fullscreen just return
		// the width we have stored
		return m_iWidth;
	}
	else
	{
		// when we're in windowed mode return the
		// size of the window
		RECT rect;

		GetWindowRect ( m_hWnd, &rect );
		
		return rect.right - rect.left;
	}
}

DARKSDK int GetDisplayHeight ( void )
{
#ifdef WICKEDENGINE
	extern bool bImGuiInTestGame;
	if (bImGuiInTestGame)
		return (int)GetSystemMetrics(SM_CYSCREEN);
	else
#endif

	// always return screen height
	return g_pGlob->iScreenHeight;

	// get the height of the display, first off check what mode
	// we're running in as we have to deal with it a bit differently

	if ( m_iDisplayType == 0 )
	{
		// when we're running in fullscreen just return
		// the width we have stored
		return m_iWidth;
	}
	else
	{
		// when we're in windowed mode return the
		// size of the window
		RECT rect;

		GetWindowRect ( m_hWnd, &rect );

		return rect.bottom - rect.top;
	}
}

DARKSDK int GetWindowWidth ( void )
{
	return g_pGlob->dwWindowWidth;
}

DARKSDK int GetWindowHeight ( void )
{
	return g_pGlob->dwWindowHeight;
}

DARKSDK int GetDesktopWidth ( void )
{
	return (int)GetSystemMetrics(SM_CXSCREEN);
}

DARKSDK int GetDesktopHeight ( void )
{
	return (int)GetSystemMetrics(SM_CYSCREEN);
}

DARKSDK int GetDisplayDepth ( void )
{
	return m_iDepth;
}

DBPRO_GLOBAL FLOAT fLastTime = 0.0f;
DBPRO_GLOBAL DWORD dwFrames  = 0L;
DBPRO_GLOBAL float m_fFPS    = 0.0f;
DBPRO_GLOBAL char FrameStats [ 256 ];
DBPRO_GLOBAL float fTime;

DARKSDK int GetDisplayFPS ( void )
{
	// get the rate at which tha app is running, must be called once
	// each frame to work properly

	// mike - 250604 - only do fps once per frame
	if ( g_bValidFPS )
	{

		fTime = Timer ( TIMER_GETABSOLUTETIME );
		++dwFrames;

		if ( fTime - fLastTime > 1.0f )
		{
			m_fFPS    = dwFrames / ( fTime - fLastTime );
			fLastTime = fTime;
			dwFrames  = 0L;
		}

		g_bValidFPS = false;
	}

	return ( int ) m_fFPS;
}

DARKSDK int GetDisplayInvalid ( void )
{
	// is set to 1 when the focus has been remove from the window
	int iInvalidSet=0;
	if(g_pGlob->bInvalidFlag) iInvalidSet=1;
	g_pGlob->bInvalidFlag=false;
	return iInvalidSet;
}

DARKSDK void SetGamma ( int iR, int iG, int iB )
{
	// set the gamma for the screen
	#ifdef DX11
	#else
	// variable declarations
	D3DGAMMARAMP	ddgr;			// gamma structure
    
	WORD			wRed   = 0;		// red value
	WORD			wGreen = 0;		// green value
	WORD			wBlue  = 0;		// blue value

	int				iColour;		// used for loops

	// clear out the gamma structu
    memset ( &ddgr, 0, sizeof ( ddgr ) );

	// store the gamma values in case we need
	// to retrieve them later on
	m_iGammaRed   = iR;		// store red
	m_iGammaGreen = iG;		// store green
	m_iGammaBlue  = iB;		// store blue
        
	// run through all of the array and setup the colours
    for ( iColour = 0; iColour < 256; iColour++ )
    {
		// setup colours
		ddgr.red   [ iColour ] = wRed;		// set red
        ddgr.green [ iColour ] = wGreen;	// set green
        ddgr.blue  [ iColour ] = wBlue;		// set blue

		// increment colours
		wRed   += ( WORD ) iR;	// add red component
		wGreen += ( WORD ) iG;	// add green component
		wBlue  += ( WORD ) iB;	// add blue component
    }

	// now that we have setup the gamma structure we can
	// pass the info across to D3D and apply the changes
	m_pD3D->SetGammaRamp ( 0, D3DSGR_NO_CALIBRATION, &ddgr );        
	#endif
}

DARKSDK void SetDisplayModeEx ( int iWidth, int iHeight, int iDepth )
{
}

DARKSDK void SetDisplayModeVSYNC ( int iWidth, int iHeight, int iDepth, int iVSyncOn )
{
	// choose new global vsync state
	if ( iVSyncOn>0 )
	{
		if ( iVSyncOn==1 ) m_iVSyncInterval=1;
		if ( iVSyncOn==2 ) m_iVSyncInterval=2;
		if ( iVSyncOn==3 ) m_iVSyncInterval=3;
		m_bVSync=true;
	}
	else
	{
		m_bVSync=false;
	}

	// call regular set display mode
	SetDisplayModeEx ( iWidth, iHeight, iDepth );
}

DARKSDK void SetDisplayModeANTIALIAS ( int iWidth, int iHeight, int iDepth, int iVSyncOn, int iMultisamplingFactor, int iMultimonitorMode )
{
	// set new multisampling factor
	m_iMultisamplingFactor = iMultisamplingFactor;

	// set new multimpnitor mode value
	m_iMultimonitorMode = iMultimonitorMode;

	// call regular set display mode
	SetDisplayModeVSYNC ( iWidth, iHeight, iDepth, iVSyncOn );
}

DARKSDK void SetDisplayModeMODBACKBUFFER ( int iWidth, int iHeight, int iDepth, int iVSyncOn, int iMultisamplingFactor, int iMultimonitorMode, int iBackbufferWidth, int iBackbufferHeight )
{
	// adjust real backbuffer to differ form size of resolution (render 1-2-1 pixels in window of screen)
	m_iModBackbufferWidth = iBackbufferWidth;
	m_iModBackbufferHeight = iBackbufferHeight;

	// call regular set display mode
	SetDisplayModeANTIALIAS ( iWidth, iHeight, iDepth, iVSyncOn, iMultisamplingFactor, iMultimonitorMode );
}

DARKSDK void SetDisplayModeVR ( int iWidth, int iHeight, int iDepth, int iVSyncOn, int iMultisamplingFactor, int iMultimonitorMode, int iBackbufferWidth, int iBackbufferHeight, int iActivateVRMode )
{
	// switch on special left/right present code for iActivateVRMode if flagged
	// mode 1 : VR920

	// call regular set display mode
	SetDisplayModeMODBACKBUFFER ( iWidth, iHeight, iDepth, iVSyncOn, iMultisamplingFactor, iMultimonitorMode, iBackbufferWidth, iBackbufferHeight );
}


DARKSDK void RestoreLostDevice ( void )
{
	#ifdef DX11
	#else
	HRESULT hRes = m_pD3D->TestCooperativeLevel();
	if(hRes!=GG_OK)
	{
		// Fullscreen lost focus (maybe ALT+TAB)
		if(hRes==GGERR_DEVICELOST)
		{
			// leeadd - 020308 - signal to all TPC DLLs that device has been lost
			InformDLLsOfDeviceLostOrNotReset ( 1 );
			return;
		}

		// Attempt to restore device by testing for not reset state
		if(hRes==D3DERR_DEVICENOTRESET)
		{
			// leeadd - 020308 - signal to all TPC DLLs that device has not been reset (but is no longer lost)
			InformDLLsOfDeviceLostOrNotReset ( 2 );

			// Recreates device to restore application
			SetDisplayModeEx ( g_pGlob->iScreenWidth, g_pGlob->iScreenHeight, g_pGlob->iScreenDepth );

			// leeadd - 070308 - added an extra callback to signal when successfully recreated device
			hRes = m_pD3D->TestCooperativeLevel();
			if(hRes==GG_OK)
			{
				// allows TPC DLLs to know when the device has been reset, and objects can be recreated 
				InformDLLsOfDeviceLostOrNotReset ( 3 );
			}
		}
	}
	#endif
}

DARKSDK void SetEmulationOn   ( void )
{
	// Not Implemented in DBPRO V1 RELEASE
	RunTimeError(RUNTIMEERROR_COMMANDNOWOBSOLETE);
}

DARKSDK void SetEmulationOff   ( void )
{
	// Not Implemented in DBPRO V1 RELEASE
	RunTimeError(RUNTIMEERROR_COMMANDNOWOBSOLETE);
}

DARKSDK void SetGraphicsCard ( DWORD dwCardname )
{
	// lee 100206 - Not Implemented in DBPRO U6 RELEASE (was never implemented)
	RunTimeError(RUNTIMEERROR_COMMANDNOWOBSOLETE);
}


//
// Window Command Functions
//

bool g_bDetachFromPreviousWindow = false;

DARKSDK void SetWindowModeOn(void)
{
	//PE: IMGUI we need support here , called before editor , called after test game.
	if ( g_bWindowOverride==false )
	{
		// Switch to window
		if ( m_iDisplayType==FULLSCREEN )
		{
			// lee - 230306 - u6b4 - only if currently fullscreen
			m_iDisplayType = WINDOWED;
			SetDisplayModeEx ( g_pGlob->iScreenWidth, g_pGlob->iScreenHeight, g_pGlob->iScreenDepth );
		}

		// lee - 290306 - u6rc3 - keep legacy behaviour by switching mode to regular window
		g_pGlob->dwAppDisplayModeUsing=1;

		gbWindowMode=true;
		gbWindowBorderActive=false;
		gWindowVisible=SW_SHOWDEFAULT;
		gWindowSizeX = g_pGlob->iScreenWidth;
		gWindowSizeY = g_pGlob->iScreenHeight;
		gWindowExtraX = gWindowExtraXForOverlap;
		gWindowExtraY = gWindowExtraYForOverlap;
		gWindowStyle = WS_OVERLAPPEDWINDOW;
		DB_UpdateEntireWindow(true,true);
	}

	// Focus on getting input from keyboard and mouse
	if ( g_bWindowOverride )
	{
		if ( g_bDetachFromPreviousWindow==false )
		{
			// direct input
			g_bDetachFromPreviousWindow = true;
			SetupKeyboardEx(1);
			SetupMouseEx(1);
		}
		else
		{
			// input via parent window
			g_bDetachFromPreviousWindow = false;
			SetupKeyboardEx(0);
			SetupMouseEx(0);
		}
	}
}

DARKSDK void SetWindowModeOff(void)
{
	gbWindowMode=false;
	gbWindowBorderActive=true;
	gWindowVisible=SW_SHOWDEFAULT;
	gWindowSizeX = GetSystemMetrics(SM_CXFULLSCREEN);
	gWindowSizeY = GetSystemMetrics(SM_CYFULLSCREEN);
	gWindowIconHandle = gOriginalIcon;
	gWindowExtraX = 0;
	gWindowExtraY = 0;
	gWindowStyle = WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU;
	DB_UpdateEntireWindow(true,true);

	// Switch to fullscreen
	if ( m_iDisplayType==WINDOWED )
	{
		// lee - 230306 - u6b4 - only if currently window
		m_iDisplayType = FULLSCREEN;
		g_pGlob->dwAppDisplayModeUsing=3;
		SetDisplayModeEx ( g_pGlob->iScreenWidth, g_pGlob->iScreenHeight, g_pGlob->iScreenDepth );
	}
}

DARKSDK void SetWindowSettings( int iStyle, int iCaption, int iIcon )
{
	gWindowStyle=0;
	if(iStyle==1 || iStyle == 5)
	{
		gWindowExtraX=gWindowExtraXForOverlap;
		gWindowExtraY=gWindowExtraYForOverlap;
		gWindowStyle |= WS_OVERLAPPEDWINDOW;
	}
	else
	{
		gWindowExtraX=0;
		gWindowExtraY=0;
		gWindowStyle |= WS_POPUP; 
	}

	// lee - 200306 - u6b4 - added extra layout codes
	if ( iStyle==2 ) gWindowStyle |= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX;
	if ( iStyle==3 ) gWindowStyle |= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	if ( iStyle==4 ) gWindowStyle |= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX;

	//PE: added for imgui.
	if (iStyle == 5) gWindowStyle |= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

	if(iCaption==1)
	{
		gWindowExtraX=gWindowExtraXForOverlap-2;
		gWindowExtraY=gWindowExtraYForOverlap-2;
		gWindowStyle |= WS_CAPTION;
	}

	//if (iIcon == 1)
	gWindowIconHandle = g_pGlob->hAppIcon;// gOriginalIcon;
	//else
	//	gWindowIconHandle=NULL;

	// update window
	DB_UpdateEntireWindow(true,true);
}

DARKSDK void SetWindowPosition( int posx, int posy )
{
	g_pGlob->dwWindowX=posx;
	g_pGlob->dwWindowY=posy;
	DB_UpdateEntireWindow(true,true);
}

DARKSDK void SetWindowSize( int sizex, int sizey )
{
	gWindowSizeX=sizex;
	gWindowSizeY=sizey;
	DB_UpdateEntireWindow(true,true);
}

DARKSDK void HideWindow(void)
{
	gWindowVisible=SW_HIDE;
	DB_UpdateEntireWindow(false,true);
}

DARKSDK void ShowWindow(void)
{
	gWindowVisible=SW_SHOW;
	DB_UpdateEntireWindow(false,true);
}

DARKSDK void MinimiseWindow(void)
{
	gWindowVisible=SW_MINIMIZE;
	DB_UpdateEntireWindow(false,true);
}

DARKSDK void MaximiseWindow(void)
{
	gWindowVisible=SW_MAXIMIZE;
	DB_UpdateEntireWindow(false,true);
}

DARKSDK void RestoreWindow(void)
{
	gWindowVisible=SW_SHOWNORMAL;
	DB_UpdateEntireWindow(false,true);
}

DARKSDK void SetWindowTitle( LPSTR pTitleString )
{
	strcpy(gWindowName, pTitleString);
	#ifdef WICKEDENGINE
	wchar_t title[MAX_PATH];
	MultiByteToWideChar(CP_UTF8, 0, gWindowName, -1, title, MAX_PATH);
	SetWindowTextW(m_hWnd, title);
	#else
	SetWindowText(m_hWnd, gWindowName);
	#endif
}

DARKSDK int WindowExist( LPSTR pTitleString )
{
	if ( FindWindow ( NULL, pTitleString )!=NULL )
		return 1;
	else
		return 0;
}

DARKSDK void WindowToBack(void)
{
	SetWindowPos(m_hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

DARKSDK void WindowToFront(void)
{
	// U75 - 080909 - previously commented out - SetWindowPos(m_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	SetWindowPos(m_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	SetForegroundWindow(m_hWnd);
}

