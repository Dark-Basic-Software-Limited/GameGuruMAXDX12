//
// RENDERING
//

bool CObjectManager::SetVertexShader ( sMesh* pMesh )
{
	// set the vertex shader for a mesh - only change if the FVF is different

	// if VertDec different in any way
	bool bRefresh = false;
	if ( m_bUpdateVertexDecs==true )
	{
		m_bUpdateVertexDecs = false;
		bRefresh = true;
	}

	// check the mesh is okay
	SAFE_MEMORY ( pMesh );

	// regular or custom shader
	if ( pMesh->bUseVertexShader )
		m_dwCurrentShader = (DWORD)pMesh->pVertexShader;
	else
		m_dwCurrentShader = 0;

	// store the current FVF as regular
	m_dwCurrentFVF = pMesh->dwFVF;

	// is the shader different to the previously set shader
	if ( m_dwCurrentShader != m_dwLastShader || bRefresh==true )
	{
		// custom shader or Fixed-Function\FX-Effect 
		#ifdef DX11
		#else
		if ( pMesh->pVertexShader )
		{
			// set the new vertex shader
			if ( FAILED ( m_pD3D->SetVertexShader ( pMesh->pVertexShader ) ) )
				return false;
		}
		else
		{
			// set no vertex shader
			if ( FAILED ( m_pD3D->SetVertexShader ( NULL ) ) )
				return false;
		}
		#endif

		// store the current shader
		m_dwLastShader = m_dwCurrentShader;
	}

	// is the FVF different to the previously set FVF
	#ifdef DX11
	#else
	if ( (m_dwCurrentFVF != m_dwLastFVF) || m_dwCurrentFVF==0 || bRefresh==true )
	{
		// custom low level shader
		if ( pMesh->pVertexShader )
		{
			// vertex dec - usually from low level assembly shader
			if ( FAILED ( m_pD3D->SetVertexDeclaration ( pMesh->pVertexDec ) ) )
				return false;
		}
		else
		{
			// custom FVF or Regular
			if ( m_dwCurrentFVF==0 )
			{
				// custom vertex dec - usually from FX effect
				if ( FAILED ( m_pD3D->SetVertexDeclaration ( pMesh->pVertexDec ) ) )
					return false;

				// regular vertex FVF - standard usage
				if ( FAILED ( m_pD3D->SetFVF ( 0 ) ) )
					return false;
			}
			else
			{
				// regular vertex FVF - standard usage
				if ( FAILED ( m_pD3D->SetFVF ( m_dwCurrentFVF ) ) )
					return false;
			}
		}

		// store the current shader
		m_dwLastFVF = m_dwCurrentFVF;
	}
	#endif

	// return okay
	return true;
}

bool CObjectManager::SetInputStreams ( sMesh* pMesh )
{
	// set the input streams for drawing - only change if different

	// make sure the mesh is valid
	SAFE_MEMORY ( pMesh );

	// store a pointer to the current VB and IB
	m_ppCurrentVBRef = pMesh->pDrawBuffer->pVertexBufferRef;
	m_ppCurrentIBRef = pMesh->pDrawBuffer->pIndexBufferRef;

	// see the difference flag to false
	bool bDifferent = false;
	if ( m_ppCurrentVBRef != m_ppLastVBRef )
		bDifferent = true;

	// when a new frame starts we need to reset the streams
	if ( m_bUpdateStreams ) bDifferent = true;

	// update VB only when necessary
	if ( bDifferent )
	{
		// store the current VB
		m_ppLastVBRef = m_ppCurrentVBRef;

		// set the stream source
		#ifdef DX11
		unsigned int stride;
		unsigned int offset;
		stride = pMesh->pDrawBuffer->dwFVFSize;
		offset = pMesh->pDrawBuffer->dwVertexStart;
		m_pImmediateContext->IASetVertexBuffers ( 0, 1, &pMesh->pDrawBuffer->pVertexBufferRef, &stride, &offset);
		#else
		if ( FAILED ( m_pD3D->SetStreamSource ( 0,
												pMesh->pDrawBuffer->pVertexBufferRef,
												0, 
												pMesh->pDrawBuffer->dwFVFSize				 ) ) )
			return false;
		#endif
	}

	// see the difference flag to false
	bDifferent = false;
	if ( m_ppCurrentIBRef != m_ppLastIBRef )
		bDifferent = true;

	// when a new frame starts we need to reset the streams
	if ( m_bUpdateStreams ) bDifferent = true;

	// update VB only when necessary
	if ( bDifferent )
	{
		// store the current VB
		m_ppLastIBRef = m_ppCurrentIBRef;

		// set the indices (if any)
		#ifdef DX11
		if ( m_ppCurrentIBRef )
			m_pImmediateContext->IASetIndexBuffer ( pMesh->pDrawBuffer->pIndexBufferRef, DXGI_FORMAT_R16_UINT, 0);
		#else
		if ( m_ppCurrentIBRef )
			if ( FAILED ( m_pD3D->SetIndices ( pMesh->pDrawBuffer->pIndexBufferRef ) ) )//, pMesh->pDrawBuffer->dwBaseVertexIndex ) ) )
				return false;
		#endif
	}

	// update refresh used (resets at start of cycle)
	m_bUpdateStreams = false;

	return true;
}

bool CObjectManager::PreSceneSettings ( void )
{
	// cullmode
	m_RenderStates.dwCullDirection				= m_RenderStates.dwGlobalCullDirection;
	m_RenderStates.bCull						= true;
	m_RenderStates.iCullMode					= 0;	
	#ifdef DX11
	#else
	m_pD3D->SetRenderState ( D3DRS_CULLMODE,	m_RenderStates.dwCullDirection );
	#endif

	// allow anistropic filtering to look better when used
	#ifdef DX11
	#else
	if ( g_iAnisotropyLevel==-1 )
	{
		// best card can give
		GGCAPS pCaps;
		m_pD3D->GetDeviceCaps(&pCaps);
		g_iAnisotropyLevel = pCaps.MaxAnisotropy;
	}
	for ( int texturestage=0; texturestage<8; texturestage++)
		m_pD3D->SetSamplerState ( texturestage, D3DSAMP_MAXANISOTROPY, g_iAnisotropyLevel );
	#endif

	// okay
	return true;
}

bool CObjectManager::PreDrawSettings ( void )
{
	#ifdef DX11
	m_RenderStates.bZWrite = true;
	m_pImmediateContext->OMSetDepthStencilState( m_pDepthStencilState, 0 );
	#else
	// obtain external render states not tracked by manager
	m_pD3D->GetRenderState ( D3DRS_AMBIENT, &m_RenderStates.dwAmbientColor );
	m_pD3D->GetRenderState ( D3DRS_FOGCOLOR, &m_RenderStates.dwFogColor );

	// 041013 - set multisampling on for everything (if activated in SET DISPLAY MODE)
	m_pD3D->SetRenderState ( D3DRS_MULTISAMPLEANTIALIAS, FALSE); // 270316 - artifacts in terrain TRUE );

	// wireframe
	m_pD3D->SetRenderState ( D3DRS_FILLMODE, D3DFILL_SOLID );
	m_RenderStates.bWireframe							= false;

	// lighting
	m_pD3D->SetRenderState ( D3DRS_LIGHTING, TRUE );
	m_RenderStates.bLight								= true;

	// fog override starts off disabled
	m_RenderStates.bFogOverride=false;

	// fogenable
	m_pD3D->SetRenderState ( D3DRS_FOGENABLE, FALSE );
	m_RenderStates.bFog									= false;

	// ambient
	m_pD3D->SetRenderState ( D3DRS_AMBIENT, m_RenderStates.dwAmbientColor );
	m_RenderStates.iAmbient								= 1;

	// transparency
	m_pD3D->SetRenderState ( D3DRS_ALPHATESTENABLE,		false );
	m_pD3D->SetRenderState ( D3DRS_ALPHAFUNC,			D3DCMP_ALWAYS );
	m_pD3D->SetRenderState ( D3DRS_DEPTHBIAS,			0 );
	m_RenderStates.bTransparency						= false;
	m_RenderStates.dwAlphaTestValue						= 0;

	// ghost
	m_pD3D->SetRenderState ( D3DRS_ZENABLE,				TRUE );
	m_pD3D->SetRenderState ( D3DRS_ZWRITEENABLE,		TRUE );
	m_pD3D->SetRenderState ( D3DRS_ALPHABLENDENABLE,	FALSE );
	m_RenderStates.bZRead								= true;
	m_RenderStates.bZWrite								= true;
	m_RenderStates.bGhost								= false;
	
	// zbias handling
	m_pD3D->SetRenderState ( D3DRS_DEPTHBIAS,			0 );
	m_pD3D->SetRenderState ( D3DRS_SLOPESCALEDEPTHBIAS,	0 );
	m_RenderStates.bZBiasActive							= false;
	m_RenderStates.fZBiasSlopeScale						= 0.0f;
	m_RenderStates.fZBiasDepth							= 0.0f;

	// default material render states
	m_pD3D->SetRenderState ( D3DRS_COLORVERTEX,					TRUE );
	m_pD3D->SetRenderState ( D3DRS_DIFFUSEMATERIALSOURCE,		D3DMCS_COLOR1 );
	m_pD3D->SetRenderState ( D3DRS_SPECULARMATERIALSOURCE,		D3DMCS_MATERIAL );
	m_pD3D->SetRenderState ( D3DRS_AMBIENTMATERIALSOURCE,		D3DMCS_MATERIAL );
	m_pD3D->SetRenderState ( D3DRS_EMISSIVEMATERIALSOURCE,		D3DMCS_MATERIAL );
	m_pD3D->SetRenderState ( D3DRS_SPECULARENABLE,				TRUE );

	// white default material 'set during init'
	m_RenderStates.gWhiteDefaultMaterial.Diffuse.r		= 1.0f;
	m_RenderStates.gWhiteDefaultMaterial.Diffuse.g		= 1.0f;
	m_RenderStates.gWhiteDefaultMaterial.Diffuse.b		= 1.0f;
	m_RenderStates.gWhiteDefaultMaterial.Diffuse.a		= 1.0f;
	m_RenderStates.gWhiteDefaultMaterial.Ambient.r		= 1.0f;
	m_RenderStates.gWhiteDefaultMaterial.Ambient.g		= 1.0f;
	m_RenderStates.gWhiteDefaultMaterial.Ambient.b		= 1.0f;
	m_RenderStates.gWhiteDefaultMaterial.Ambient.a		= 1.0f;
	m_RenderStates.gWhiteDefaultMaterial.Specular.r		= 0.0f;
	m_RenderStates.gWhiteDefaultMaterial.Specular.g		= 0.0f;
	m_RenderStates.gWhiteDefaultMaterial.Specular.b		= 0.0f;
	m_RenderStates.gWhiteDefaultMaterial.Specular.a		= 0.0f;
	m_RenderStates.gWhiteDefaultMaterial.Emissive.r		= 0.0f;
	m_RenderStates.gWhiteDefaultMaterial.Emissive.g		= 0.0f;
	m_RenderStates.gWhiteDefaultMaterial.Emissive.b		= 0.0f;
	m_RenderStates.gWhiteDefaultMaterial.Emissive.a		= 0.0f;
	m_RenderStates.gWhiteDefaultMaterial.Power			= 10.0f;

	// set default white material (for diffuse, ambience, etc)
	if ( FAILED ( m_pD3D->SetMaterial ( &m_RenderStates.gWhiteDefaultMaterial ) ) )
		return false;

	// fixed function blending stage defaults
	DWORD dwMaxTextureStage = MAXTEXTURECOUNT;
	for ( DWORD dwTextureStage = 0; dwTextureStage < dwMaxTextureStage; dwTextureStage++ )
	{
		// leefix - 180204 - set defaults at start of render phase
		m_RenderStates.dwAddressU[dwTextureStage] = D3DTADDRESS_WRAP;
		m_RenderStates.dwAddressV[dwTextureStage] = D3DTADDRESS_WRAP;
		m_RenderStates.dwMagState[dwTextureStage] = GGTEXF_LINEAR;
		m_RenderStates.dwMinState[dwTextureStage] = GGTEXF_LINEAR;
		m_RenderStates.dwMipState[dwTextureStage] = GGTEXF_LINEAR;

		// texture filter modes
		m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_ADDRESSU, m_RenderStates.dwAddressU[dwTextureStage]==0 ? D3DTADDRESS_WRAP : m_RenderStates.dwAddressU[dwTextureStage] );
		m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_ADDRESSV, m_RenderStates.dwAddressV[dwTextureStage]==0 ? D3DTADDRESS_WRAP : m_RenderStates.dwAddressV[dwTextureStage] );
		m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_MAGFILTER, m_RenderStates.dwMagState[dwTextureStage]==0 ? GGTEXF_LINEAR : m_RenderStates.dwMagState[dwTextureStage] );
		m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_MINFILTER, m_RenderStates.dwMinState[dwTextureStage]==0 ? GGTEXF_LINEAR : m_RenderStates.dwMinState[dwTextureStage] );
		m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_MIPFILTER, m_RenderStates.dwMipState[dwTextureStage]==0 ? GGTEXF_LINEAR : m_RenderStates.dwMipState[dwTextureStage] );

		// texture blending modes
		if(dwTextureStage==0)
		{
			m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_COLOROP, GGTOP_MODULATE );
			m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_COLORARG1, GGTA_TEXTURE );
			m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_COLORARG2, GGTA_DIFFUSE );
		}
		else
		{
			m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_COLOROP, GGTOP_DISABLE );
		}

		// texture coordinate data
		m_pD3D->SetTextureStageState( dwTextureStage, D3DTSS_TEXCOORDINDEX, dwTextureStage );
		m_pD3D->SetTextureStageState( dwTextureStage, D3DTSS_TEXTURETRANSFORMFLAGS, 0 );
	}
	#endif

	// Set default FOV from camera (zero does not change camera FOV!)
	if ( g_pGlob->dwRenderCameraID == 0 )
	{
		if ( m_RenderStates.fObjectFOV != 0.0f )
		{
			// sometimes, objectfov renderstate is not reset, and needed before we start again
			SetCameraFOV ( m_RenderStates.fStoreCameraFOV );
			m_RenderStates.fObjectFOV = 0.0f;
		}
		tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData( 0 );
		m_RenderStates.fStoreCameraFOV = m_Camera_Ptr->fFOV;
		m_RenderStates.fObjectFOV = 0.0f;
		SetCameraFOV ( m_RenderStates.fStoreCameraFOV );
	}

	// success
	return true;
}

bool CObjectManager::SetMeshMaterial ( sMesh* pMesh, D3DMATERIAL9* pMaterial )
{
	#ifdef DX11
	#else
	if ( pMesh->bUsesMaterial )
	{
		// use diffuse from material (if present)
		m_pD3D->SetRenderState ( D3DRS_COLORVERTEX,					FALSE );
		m_pD3D->SetRenderState ( D3DRS_DIFFUSEMATERIALSOURCE,		D3DMCS_MATERIAL );

		// set the material from the mesh
		if ( FAILED ( m_pD3D->SetMaterial ( pMaterial ) ) )
			return false;
	}
	else
	{
		// use diffuse from mesh vertex (if any)
		m_pD3D->SetRenderState ( D3DRS_COLORVERTEX,					TRUE );
		m_pD3D->SetRenderState ( D3DRS_DIFFUSEMATERIALSOURCE,		D3DMCS_COLOR1 );

		// set no material
		if ( FAILED ( m_pD3D->SetMaterial ( &m_RenderStates.gWhiteDefaultMaterial ) ) )
			return false;
	}
	#endif

	// success
	return true;
}

void CObjectManager::SetMeshDepthStates( sMesh* pMesh, bool bForceState )
{
	if ( pMesh->bZWrite != m_RenderStates.bZWrite || pMesh->bZRead != m_RenderStates.bZRead || bForceState == true )
	{
		ID3D11DepthStencilState* pDepthStencilState = NULL;
		if ( pMesh->bZRead == true )
		{
			if ( pMesh->bZWrite )
				pDepthStencilState = m_pDepthStencilState;
			else
				pDepthStencilState = m_pDepthNoWriteStencilState;
		}
		else
		{
			pDepthStencilState = m_pDepthDisabledStencilState;
		}
		m_RenderStates.bZWrite = pMesh->bZWrite;
		m_RenderStates.bZRead = pMesh->bZRead;
		m_pImmediateContext->OMSetDepthStencilState( pDepthStencilState, 0 );
	}
}

bool CObjectManager::SetMeshRenderStates( sMesh* pMesh )
{
	#ifdef DX11

	// standard depth state
	SetMeshDepthStates ( pMesh, false );

	// blend state
	if ( pMesh->bShadowBlend )
		m_pImmediateContext->OMSetBlendState(m_pBlendStateShadowBlend, 0, 0xffffffff);
	else
		m_pImmediateContext->OMSetBlendState(m_pBlendStateAlpha, 0, 0xffffffff);

	// render state
	if ( pMesh->bCull == false )
	{
		m_pImmediateContext->RSSetState(m_pRasterStateNoCull);
	}
	else
	{
		if ( pMesh->bZBiasActive == true )
		{
			m_pImmediateContext->RSSetState(m_pRasterStateDepthBias);
		}
		else
		{
			m_pImmediateContext->RSSetState(m_pRasterState);
		}
	}
	#else
	// wireframe
	if ( pMesh->bWireframe != m_RenderStates.bWireframe )
	{
		if ( pMesh->bWireframe )
			m_pD3D->SetRenderState ( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
		else
			m_pD3D->SetRenderState ( D3DRS_FILLMODE, D3DFILL_SOLID );

		m_RenderStates.bWireframe = pMesh->bWireframe;
	}

	// lighting
	if ( pMesh->bLight != m_RenderStates.bLight )
	{
		if ( pMesh->bLight )
			m_pD3D->SetRenderState ( D3DRS_LIGHTING, TRUE );
		else
			m_pD3D->SetRenderState ( D3DRS_LIGHTING, FALSE );

		m_RenderStates.bLight = pMesh->bLight;
	}

	// cullmode
	if ( pMesh->bCull != m_RenderStates.bCull || pMesh->iCullMode != m_RenderStates.iCullMode )
	{
		// lee - 040306 -u6rc5 - cull mode (direction override)
		if ( pMesh->iCullMode==2 )
		{
			m_pD3D->SetRenderState ( D3DRS_CULLMODE, GGCULL_CCW );
		}
		else
		{
			// lee - 121006 - u63 - cull mode CW (for manual reflection cull toggle)
			if ( pMesh->iCullMode==3 )
			{
				m_pD3D->SetRenderState ( D3DRS_CULLMODE, D3DCULL_CW );
			}
			else
			{
				// on/off
				if ( pMesh->bCull )
					m_pD3D->SetRenderState ( D3DRS_CULLMODE, m_RenderStates.dwCullDirection );
				else	
					m_pD3D->SetRenderState ( D3DRS_CULLMODE, D3DCULL_NONE );
			}
		}
		m_RenderStates.bCull = pMesh->bCull;
		m_RenderStates.iCullMode = pMesh->iCullMode;
	}

	// fog system (from light DLL)
	if(g_pGlob)
	{
		if(g_pGlob->iFogState==1)
		{
			// fogenable
			if ( pMesh->bFog != m_RenderStates.bFog )
			{
				if ( pMesh->bFog )
					m_pD3D->SetRenderState ( D3DRS_FOGENABLE, TRUE );
				else	
					m_pD3D->SetRenderState ( D3DRS_FOGENABLE, FALSE );
	
				m_RenderStates.bFog = pMesh->bFog;
			}

			// ghosts in fog must override fog color part (fog override)
			if ( pMesh->bFog && pMesh->bGhost )
			{
				if ( m_RenderStates.bFogOverride==false )
				{
					m_pD3D->SetRenderState ( D3DRS_FOGCOLOR, GGCOLOR_RGBA ( 0, 0, 0, 0 ) );
					m_RenderStates.bFogOverride=true;
				}
			}
			else
			{
				m_pD3D->SetRenderState ( D3DRS_FOGCOLOR, m_RenderStates.dwFogColor );
				m_RenderStates.bFogOverride=false;
			}
		}
	}

	// ambient - leefix - 230604 - u54 - no ambience can now be 255 or 0 (so need to do code in those cases)
	if ( pMesh->bAmbient==false && m_RenderStates.iAmbient==1
	||	 m_RenderStates.iAmbient==0 || m_RenderStates.iAmbient==2 )
	{
		if ( pMesh->bAmbient )
		{
			m_pD3D->SetRenderState ( D3DRS_AMBIENT, m_RenderStates.dwAmbientColor );
			m_RenderStates.iAmbient = 1;
		}
		else
		{
			// leefix - 210303 - diffuse colour must be maintained over any ambience
			bool bWhite=false;
			if ( pMesh->pTextures )
				if ( pMesh->pTextures[0].iImageID!=0 )
					bWhite=true;

			// leefix - 210303 - white used for no ambient on a texture
			if ( bWhite )
			{
				m_pD3D->SetRenderState ( D3DRS_AMBIENT, GGCOLOR_ARGB(255,255,255,255) );
				m_RenderStates.iAmbient = 2;
			}
			else
			{
				m_pD3D->SetRenderState ( D3DRS_AMBIENT, GGCOLOR_ARGB(0,0,0,0) );
				m_RenderStates.iAmbient = 0;
			}
		}
	}

	// leefix - 070204 - introduced for better Zwrite control
	bool bCorrectZWriteState = pMesh->bZWrite;

	// transparency (leefix - 190303 - added second condition where transparency is reimposed after a ghosted object)
	bool bDoGhostAgain = false;
	if ( pMesh->bTransparency != m_RenderStates.bTransparency
	||	 pMesh->dwAlphaTestValue != m_RenderStates.dwAlphaTestValue
	|| ( pMesh->bTransparency==true && m_RenderStates.bGhost==true	) )
	{
		if ( pMesh->bTransparency )
		{
			m_pD3D->SetRenderState ( D3DRS_ALPHABLENDENABLE,	true );
			m_pD3D->SetRenderState ( D3DRS_SRCBLEND,			D3DBLEND_SRCALPHA );
			m_pD3D->SetRenderState ( D3DRS_DESTBLEND,			D3DBLEND_INVSRCALPHA );
			m_pD3D->SetRenderState ( D3DRS_ALPHATESTENABLE,		true );
			
			// mike - 020904 - use this for alpha testing - do not get edges anymore
			// lee - 240903 - need full range of alpha rendered, not just the upper band
			DWORD dwuseAlphaTestValue = pMesh->dwAlphaTestValue;

			if ( dwuseAlphaTestValue==0 )
			{
				m_pD3D->SetRenderState ( D3DRS_ALPHAFUNC,	D3DCMP_GREATER );
				m_pD3D->SetRenderState ( D3DRS_ALPHAREF,	(DWORD)0x00000000 );
			}
			else
			{
				// leeadd - 131205 - let SetAlphaMappingOn command scale the alpha-test to let semi-transparent pixel through
				if ( pMesh->bAlphaOverride==true )
				{
					// alpha mapping percentage vased alpha test
					DWORD dwPercAlpha = ( (pMesh->dwAlphaOverride & 0xFF000000) >> 24 ) ;
					float perc = (float)dwPercAlpha / 255.0f;
					// alpha test transition not perfect as go from override to 0xCF based alpha, so cap it
					DWORD dwAlphaLevelToDraw = (DWORD)(255 * perc);
					dwuseAlphaTestValue = dwAlphaLevelToDraw;
					if ( dwuseAlphaTestValue > (DWORD)0x000000CF ) dwuseAlphaTestValue=(DWORD)0x000000CF;
					m_pD3D->SetRenderState ( D3DRS_ALPHAFUNC,	D3DCMP_GREATEREQUAL );
				}
				else
				{
					// regular alpha test
					dwuseAlphaTestValue=(DWORD)0x000000CF;
					m_pD3D->SetRenderState ( D3DRS_ALPHAFUNC,	D3DCMP_GREATEREQUAL );
				}
				m_pD3D->SetRenderState ( D3DRS_ALPHAREF,	dwuseAlphaTestValue );
			}
			m_RenderStates.dwAlphaTestValue = dwuseAlphaTestValue;
		}
		else
		{
			m_pD3D->SetRenderState ( D3DRS_ALPHABLENDENABLE,	false );
			m_pD3D->SetRenderState ( D3DRS_ALPHATESTENABLE,		false );
			m_pD3D->SetRenderState ( D3DRS_ALPHAFUNC,			D3DCMP_ALWAYS );
		}
		m_RenderStates.bTransparency = pMesh->bTransparency;

		// now must do ghost again - to combine with blend settings
		bDoGhostAgain = true;
	}

	// ghost
	if ( bDoGhostAgain==true
	||	pMesh->bGhost != m_RenderStates.bGhost
	||  pMesh->iGhostMode != m_RenderStates.iGhostMode )
	{
		if ( pMesh->bGhost )
		{
			m_pD3D->SetRenderState ( D3DRS_ALPHABLENDENABLE, true );
			switch ( pMesh->iGhostMode )
			{
				case 0:
				{
					m_pD3D->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ONE );
					m_pD3D->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCCOLOR );
				}
				break;

				case 1:
				{
					// lee - 220306 - u6b4 - direct from best of DBC (darkghostmode7)
					DWORD dwDarkAlphaSourceBlend = D3DBLEND_ZERO;
					DWORD dwDarkAlphaDestinationBlend = D3DBLEND_SRCCOLOR;
					m_pD3D->SetRenderState( D3DRS_SRCBLEND,  dwDarkAlphaSourceBlend );
					m_pD3D->SetRenderState( D3DRS_DESTBLEND, dwDarkAlphaDestinationBlend );
				}
				break;

				case 2:
				{
					m_pD3D->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCCOLOR );
					m_pD3D->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
				}
				break;

				case 3:
				{
					m_pD3D->SetRenderState ( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
					m_pD3D->SetRenderState ( D3DRS_DESTBLEND, D3DBLEND_SRCALPHA );
				}
				break;

				case 4:
				{
					m_pD3D->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCCOLOR );
					m_pD3D->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_DESTCOLOR );
				}
				break;

				case 5:
				{
					// leeadd - 210806 - replace OLD-MODE-1 (used in FPSC) for Scorch Texture Multiply
					m_pD3D->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_DESTCOLOR );
					m_pD3D->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR );
				}
				break;
			}
			bCorrectZWriteState = false;
		}
		else
		{
			// no ghost and no transparency, end alpha blend effect
			if ( pMesh->bTransparency==false)
			{
				m_pD3D->SetRenderState ( D3DRS_ALPHABLENDENABLE, FALSE );
			}
		}

		m_RenderStates.bGhost = pMesh->bGhost;
		m_RenderStates.iGhostMode = pMesh->iGhostMode;
	}

	// leefix - 070204 - simplified - set zwrite state
	if ( pMesh->bZWrite != m_RenderStates.bZWrite )
	{
		if ( pMesh->bZWrite )
			m_pD3D->SetRenderState ( D3DRS_ZWRITEENABLE,		TRUE );
		else
			m_pD3D->SetRenderState ( D3DRS_ZWRITEENABLE,		FALSE );

		m_RenderStates.bZWrite = pMesh->bZWrite;
	}

	// leeadd - 080604 - ZBIAS handling - always set unless not active, then unset once
	if ( pMesh->bZBiasActive )
	{
		m_pD3D->SetRenderState ( D3DRS_DEPTHBIAS,			
			pMesh->fZBiasDepth );
		m_pD3D->SetRenderState ( D3DRS_SLOPESCALEDEPTHBIAS,	*(DWORD*)&pMesh->fZBiasSlopeScale );
		m_RenderStates.bZBiasActive = true;
	}
	else
	{
		if ( m_RenderStates.bZBiasActive )
		{
			m_pD3D->SetRenderState ( D3DRS_DEPTHBIAS,			0 );
			m_pD3D->SetRenderState ( D3DRS_SLOPESCALEDEPTHBIAS,	0 );
			m_RenderStates.bZBiasActive = false;
		}
	}

	// set zread state
	if ( pMesh->bZRead != m_RenderStates.bZRead )
	{
		if ( pMesh->bZRead )
			m_pD3D->SetRenderState ( D3DRS_ZENABLE,		TRUE );
		else
			m_pD3D->SetRenderState ( D3DRS_ZENABLE,		FALSE );

		m_RenderStates.bZRead = pMesh->bZRead;
	}

	// set the new material and render state
	SetMeshMaterial ( pMesh, &pMesh->mMaterial );

	// need to be able to set mip map LOD bias on a per mesh basis
	m_pD3D->SetSamplerState ( 0, D3DSAMP_MIPMAPLODBIAS, *( ( LPDWORD ) ( &pMesh->fMipMapLODBias ) ) );
	#endif

	// success
	return true;
}

bool CObjectManager::SetMeshTextureStates ( sMesh* pMesh )
{
	#ifdef DX11
	#else
	// close off any stages from previous runs
	if ( m_dwLastTextureCount > pMesh->dwTextureCount )
	{
		DWORD dwTexCountMax = m_dwLastTextureCount;
		if ( dwTexCountMax > 7 ) dwTexCountMax = 7;
		for ( DWORD dwTextureStage = pMesh->dwTextureCount; dwTextureStage < dwTexCountMax; dwTextureStage++ )
		{
			m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_COLOROP, GGTOP_DISABLE );
		}
	}

	// texture filtering and blending
	DWORD dwTextureCountMax = pMesh->dwTextureCount;
	if ( dwTextureCountMax>=MAXTEXTURECOUNT ) dwTextureCountMax=MAXTEXTURECOUNT;
	for ( DWORD dwTextureIndex = 0; dwTextureIndex < pMesh->dwTextureCount; dwTextureIndex++ )
	{
		// Determine texture stage to write to
		DWORD dwTextureStage = pMesh->pTextures [ dwTextureIndex ].dwStage;

		// Determine texture data ptr
		sTexture* pTexture = &pMesh->pTextures [ dwTextureIndex ];

		// texture wrap and filter modes
		m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_ADDRESSU, pTexture->dwAddressU==0 ? D3DTADDRESS_WRAP : pTexture->dwAddressU );
		m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_ADDRESSV, pTexture->dwAddressV==0 ? D3DTADDRESS_WRAP : pTexture->dwAddressV );
		m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_MAGFILTER, pTexture->dwMagState==0 ? GGTEXF_LINEAR : pTexture->dwMagState );
		m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_MINFILTER, pTexture->dwMinState==0 ? GGTEXF_LINEAR : pTexture->dwMinState );
		m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_MIPFILTER, pTexture->dwMipState==0 ? GGTEXF_LINEAR : pTexture->dwMipState );

		// texture blending modes
		if ( pMesh->bOverridePixelShader )
		{
			// use custom pixel shader to replace blending stages
			m_pD3D->SetPixelShader ( pMesh->pPixelShader );
		}
		else
		{
			// fixed function does not use pixel shaders
			m_pD3D->SetPixelShader ( 0 );

			// fixed function blending (leefix-210703-fixed now at source)
			m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_COLOROP, pTexture->dwBlendMode );
			m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_COLORARG1, pTexture->dwBlendArg1 );

			// lee - 240206 - u60 - will use TFACTOR (diffuse replacement) instead of regular DIFFUSE
			if ( dwTextureStage==0 && m_RenderStates.bIgnoreDiffuse==true )
			{
				// TFACTOR set in previous call from the INSTANCE drawmesh call (instance diffuse changes)
				m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_COLORARG2, D3DTA_TFACTOR );
			}
			else
				m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_COLORARG2, pTexture->dwBlendArg2 );

			// U73 - 210309 - apply extra ARG values
			m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_COLORARG0, pTexture->dwBlendArg0 );
			m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_ALPHAARG0, pTexture->dwBlendArg0 );
			m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_RESULTARG, pTexture->dwBlendArgR );

			// last texture stage can override alpha with tfactor
			if ( m_RenderStates.bNoMeshAlphaFactor==false )
			{
				if ( pMesh->bAlphaOverride==true )
				{
					if ( dwTextureStage==pMesh->dwTextureCount-1 )
					{
						// instance overrides alpha value using TFACTOR
						m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_ALPHAOP, pTexture->dwBlendMode );
						m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_ALPHAARG2, pTexture->dwBlendArg1 );
						m_pD3D->SetTextureStageState ( 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR );
						m_pD3D->SetRenderState( D3DRS_TEXTUREFACTOR, pMesh->dwAlphaOverride );
					}
				}
				else
				{
					// regular alpha operations
					m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_ALPHAOP, pTexture->dwBlendMode );
					m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_ALPHAARG1, pTexture->dwBlendArg1 );
					m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_ALPHAARG2, pTexture->dwBlendArg2 );
				}
			}

			// texture coordinate data
			switch ( pTexture->dwTexCoordMode )
			{
				case 0 :{	// Regular UV Stage Match
							m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_TEXCOORDINDEX, dwTextureStage );
							m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_TEXTURETRANSFORMFLAGS, 0 );
						}
						break;

				/*
				case 1 :{	// Sphere Mapping
							GGMATRIX mat;
							mat._11 = 0.5f; mat._12 = 0.0f; mat._13 = 0.0f; mat._14 = 0.0f; 
							mat._21 = 0.0f; mat._22 =-0.5f; mat._23 = 0.0f; mat._24 = 0.0f; 
							mat._31 = 0.0f; mat._32 = 0.0f; mat._33 = 1.0f; mat._34 = 0.0f; 
							mat._41 = 0.5f; mat._42 = 0.5f; mat._43 = 0.0f; mat._44 = 1.0f; 
							D3DTRANSFORMSTATETYPE dwTexTS = D3DTS_TEXTURE1;
							if( dwTextureStage==2 ) dwTexTS=D3DTS_TEXTURE2;
							GGSetTransform ( dwTexTS, &mat );
							m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACENORMAL );
							m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 );
						}
						break;

				case 2 :{	// Cube Mapping (leefix - 190303 - works now as camera moves around)
							GGMATRIX mat;
							GGMATRIX matview;
							GGGetTransform ( GGTS_VIEW, &matview );
							GGMatrixInverse ( &mat, NULL, &matview );
							mat._41 = 0.0f; mat._42 = 0.0f; mat._43 = 0.0f; mat._44 = 1.0f; 
							D3DTRANSFORMSTATETYPE dwTexTS = D3DTS_TEXTURE0;
							if( dwTextureStage==1 ) dwTexTS=D3DTS_TEXTURE1;
							if( dwTextureStage==2 ) dwTexTS=D3DTS_TEXTURE2;
							GGSetTransform ( dwTexTS, &mat );
							m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR );
							m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3 );
						}
						break;
						*/

				case 3 :{	// Steal UV Stage From Zero
							m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_TEXCOORDINDEX, 0 );
							m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_TEXTURETRANSFORMFLAGS, 0 );
						}
						break;

				case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17:
						{	// Set alternate texture bank for UV data
							int iGetFrom = pTexture->dwTexCoordMode-10;
							m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_TEXCOORDINDEX, iGetFrom );
							m_pD3D->SetTextureStageState ( dwTextureStage, D3DTSS_TEXTURETRANSFORMFLAGS, 0 );
						}
						break;
			}
		}
	}
	#endif

	// store number of textures handled for this mesh
	m_dwLastTextureCount = pMesh->dwTextureCount;

	// okay
	return true;
}

bool CObjectManager::ShaderStart ( sMesh* pMesh, LPGGRENDERTARGETVIEW* ppCurrentRenderTarget, LPGGDEPTHSTENCILVIEW* ppCurrentDepthTarget, UINT* puPasses, bool* pbEffectRendering, bool* pbLocalOverrideAllTexturesAndEffects )
{
	// set the vertex shader
	if ( !SetVertexShader ( pMesh ) )
		return false;

	// can switch off fore color wipe if object flags it
	*pbLocalOverrideAllTexturesAndEffects = m_RenderStates.bOverrideAllTexturesAndEffects;
	if ( pMesh->pDrawBuffer )
		if ( pMesh->pDrawBuffer->dwImmuneToForeColorWipe==1 )
			*pbLocalOverrideAllTexturesAndEffects = false;

	// set effect shader
	if ( pMesh->pVertexShaderEffect && *pbLocalOverrideAllTexturesAndEffects==false )
	{
		// use an effect
		GGMATRIX matWorld;
		GGGetTransform ( GGTS_WORLD, &matWorld );
		*puPasses = pMesh->pVertexShaderEffect->Start ( pMesh, matWorld );

		// if FX effect, flag effect code
		if ( pMesh->pVertexShaderEffect->m_pEffect )
		{
			// effect shall be used
			*pbEffectRendering=true;

			// set states prior to shader begin pass
			#ifdef DX11
			#else
			if ( pMesh->pTextures )
			{
				for ( DWORD dwTextureIndex = 0; dwTextureIndex < pMesh->dwTextureCount; dwTextureIndex++ )
				{
					DWORD dwTextureStage = pMesh->pTextures [ dwTextureIndex ].dwStage;
					if ( dwTextureStage < 16 )
					{
						// so the object states to do overwrite any shader states that begin pass will set
						sTexture* pTexture = &pMesh->pTextures [ dwTextureIndex ];
						if ( pTexture )
						{
							m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_ADDRESSU, pTexture->dwAddressU==0 ? D3DTADDRESS_WRAP : pTexture->dwAddressU );
							m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_ADDRESSV, pTexture->dwAddressV==0 ? D3DTADDRESS_WRAP : pTexture->dwAddressV );
							m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_MAGFILTER, pTexture->dwMagState==0 ? GGTEXF_LINEAR : pTexture->dwMagState );
							m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_MINFILTER, pTexture->dwMinState==0 ? GGTEXF_LINEAR : pTexture->dwMinState );
							m_pD3D->SetSamplerState ( dwTextureStage, D3DSAMP_MIPFILTER, pTexture->dwMipState==0 ? GGTEXF_LINEAR : pTexture->dwMipState );
						}
					}
				}
			}
			#endif
		}
	}

	// if using RT, store current render target
	if ( pMesh->pVertexShaderEffect )
	{
		if ( pMesh->pVertexShaderEffect->m_bUsesAtLeastOneRT==true )
		{
			#ifdef DX11
			*ppCurrentRenderTarget = g_pGlob->pCurrentRenderView;
			*ppCurrentDepthTarget = g_pGlob->pCurrentDepthView;
			#else
			m_pD3D->GetRenderTarget( 0, ppCurrentRenderTarget );
			m_pD3D->GetDepthStencilSurface( ppCurrentDepthTarget );
			#endif
		}
	}

	// if rendering with an effect
	#ifdef DX11
	#else
	if ( *pbEffectRendering )
	{
		// FF affects HLSL pipeline (and vice versa), so switch off
		// the automated clipping plane (FF will stop clipping for HLSLs)
		if ( m_RenderStates.bOverriddenClipPlaneforHLSL==false )
		{
			m_pD3D->SetRenderState ( D3DRS_CLIPPLANEENABLE, 0x00 );
			m_RenderStates.bOverriddenClipPlaneforHLSL = true;
		}
	}
	#endif

	// continue
	return true;
}

DWORD g_InstanceAlphaControlValue = 0;
DWORD g_UseOutLine = 0;
DWORD g_UseWireFrame = 0;

bool CObjectManager::ShaderPass ( sMesh* pMesh, UINT uPass, UINT uPasses, bool bEffectRendering, bool bLocalOverrideAllTexturesAndEffects, LPGGRENDERTARGETVIEW pCurrentRenderTarget, LPGGDEPTHSTENCILVIEW pCurrentDepthTarget, sObject* pObject )
{
	// return true else if something like [depth render is skipped]
	bool bResult = true;

	// override every texture and effect with single color
	// useful for advanced post-processing effects like depthoffield/heathaze
	if ( bLocalOverrideAllTexturesAndEffects==true )
	{
		// no texture, no effect, just plane color
		#ifdef DX11
		#else
		m_pD3D->SetTexture ( 0, NULL );
		m_pD3D->SetTextureStageState ( 0, D3DTSS_COLOROP, GGTOP_SELECTARG1 );
		m_pD3D->SetTextureStageState ( 0, D3DTSS_COLORARG1, D3DTA_TFACTOR );
		m_pD3D->SetTextureStageState ( 0, D3DTSS_ALPHAOP,   GGTOP_DISABLE );
		for ( int t=1; t<=7; t++ )
		{
			m_pD3D->SetTexture ( t, NULL );
			m_pD3D->SetTextureStageState ( t, D3DTSS_COLOROP, GGTOP_DISABLE );
			m_pD3D->SetTextureStageState ( t, D3DTSS_ALPHAOP,   GGTOP_DISABLE );
		}
		m_pD3D->SetRenderState ( D3DRS_TEXTUREFACTOR,	m_RenderStates.dwOverrideAllWithColor );
		m_pD3D->SetRenderState ( D3DRS_FOGCOLOR,		m_RenderStates.dwFogColor );
		#endif
	}
	else
	{


		// if using RT, determine if should switch to RT or final render target (current)
		if ( bEffectRendering )
		{
#ifdef DX11
			//PE: Only shader Apply needed.
			if (g_pGlob->dwRenderCameraID >= 31) {
				ID3DX11EffectTechnique* pTech = pMesh->pVertexShaderEffect->m_hCurrentTechnique;//m_pEffect->GetTechniqueByIndex(0);
				ID3DX11EffectPass* pPass = pTech->GetPassByIndex(uPass);
				pPass->Apply(0, m_pImmediateContext);
				return true;
			}
#endif
			// commit var
			LPGGEFFECT pEffect = pMesh->pVertexShaderEffect->m_pEffect;
			bool bMustCommit = false;

			// only if RT flagged (saves performance)
			if ( pMesh->pVertexShaderEffect->m_bUsesAtLeastOneRT==true )
			{
				#ifdef DX11
				// ensure technique exists
				ID3DX11EffectTechnique* hTech = pMesh->pVertexShaderEffect->m_hCurrentTechnique;//pEffect->GetTechniqueByIndex(0);

				// get rendercolortarget string from this pass
				ID3DX11EffectPass* hPass = hTech->GetPassByIndex(uPass);
				GGHANDLE hRT = hPass->GetAnnotationByName( "RenderColorTarget" );
				const char* szRT = 0;
				D3DX11_EFFECT_VARIABLE_DESC varDesc;
				if ( hRT && hRT->IsValid() ) 
				{
					hRT->AsString()->GetString(&szRT);
				}

				// if detect, use special depth texture render target
				/*if ( szRT && strnicmp( szRT, "[depthtexture]", strlen("[depthtexture]") )==NULL ) 
				{
					// NOTE: Optimize here, should not be using strings/compares
					if ( g_pGlob->dwRenderCameraID==0 )
					{
						// only render depths from camera zero
						int iSuccess = SwitchRenderTargetToDepth(0);
					}
					else
					{
						// actually SKIP this pass if any other camera
						return false;
					}
				}
				else
				{*/
					// check if RT string has contents
					if ( hRT && strcmp( szRT, "" ) != 0 )
					{
						// yes, now get handle to texture param we want to re-direct our render to
						GGHANDLE hTexParamWant = pEffect->GetVariableByName( szRT );

						// go through all textures recorded during the shader parse
						for ( DWORD t = 0; t < pMesh->pVertexShaderEffect->m_dwTextureCount; t++ )
						{
							// only consider RT textures that are flagged in bitmask
							DWORD dwThisTextureBit = 1 << t;
							if ( pMesh->pVertexShaderEffect->m_dwCreatedRTTextureMask & dwThisTextureBit )
							{
								// get the param handle of each texture in shader
								int iParam = 0;
								GGHANDLE hTexParam = NULL;
								if ( t<=31 ) 
								{
									iParam = pMesh->pVertexShaderEffect->m_iParamOfTexture [ t ];
									hTexParam = pEffect->GetVariableByIndex( iParam );
								}

								// if it matches the one we want
								if ( hTexParam == hTexParamWant )
								{
									// get texture ptr from effect
									LPGGSHADERRESOURCEVIEW pRTTexView = NULL;
									hTexParam->AsShaderResource()->GetResource(&pRTTexView);
									IGGTexture* pRTTex = NULL;
									pRTTexView->GetResource(&pRTTex);

									// switch render target to internal shader RT
									ID3D11RenderTargetView* pRTTexRenderView = pMesh->pVertexShaderEffect->m_pParamOfTextureRenderView[t];
									SetRenderAndDepthTarget ( pRTTexRenderView, nullptr );

									// for renders to shader off-screen RT textures, disable depth buffer completely
									m_pImmediateContext->OMSetDepthStencilState( m_pDepthDisabledStencilState, 0 );

									// put RT textures width and height into ViewSize
									GGHANDLE hViewSize = pEffect->GetVariableBySemantic( "ViewSize" );
									if ( hViewSize )
									{
										GGSURFACE_DESC desc;
										g_pGlob->pCurrentBitmapSurface->GetDesc(&desc);
										int width = desc.Width, height = desc.Height;
										GGHANDLE hWidth = hTexParam->GetAnnotationByName( "width" );
										GGHANDLE hHeight = hTexParam->GetAnnotationByName( "height" );
										if ( hWidth && hWidth->IsValid() ) hWidth->AsScalar()->GetInt( &width );
										if ( hHeight && hHeight->IsValid() ) hHeight->AsScalar()->GetInt( &height );
										if ( width == -1 ) width = desc.Width;
										if ( height == -1 ) height = desc.Height;
										GGVECTOR4 vec( (float) width, (float) height, 0, 0 );
										hViewSize->AsVector()->SetFloatVector( (float*)&vec );

										// DX11 seems viewport stays at fullscreen size (needs to be set to target size)
										D3D11_VIEWPORT vp = { 0, 0, width, height, 0, 1 };
										//m_pImmediateContext->RSSetViewports( 1, &vp );
										SetupSetViewport ( -1, &vp, NULL );
									}
								}
							}
						}
					}
					else
					{
						// no, we render to current as normal
						SetRenderAndDepthTarget ( pCurrentRenderTarget, pCurrentDepthTarget );

						// restore to standard depth state
						SetMeshDepthStates ( pMesh, true );

						// set the Viewsize to match the width and height of the RT passed in
						GGHANDLE hViewSize = pEffect->GetVariableBySemantic( "ViewSize" );
						if ( hViewSize )
						{
							GGSURFACE_DESC desc;
							g_pGlob->pCurrentBitmapSurface->GetDesc(&desc);
							GGVECTOR4 vec( (float) desc.Width, (float) desc.Height, 0, 0 );
							hViewSize->AsVector()->SetFloatVector( (float*)&vec );

							// DX11 seems viewport stays at fullscreen size (needs to be set to target size)
							D3D11_VIEWPORT vp = { 0, 0, desc.Width, desc.Height, 0, 1 };
							//m_pImmediateContext->RSSetViewports( 1, &vp );
							SetupSetViewport ( g_pGlob->dwRenderCameraID, &vp, NULL );
						}
					}
				//}
				// once textures established, commit effect state changes and begin this pass
				bMustCommit = true;
				#else
				// ensure technique exists
				GGHANDLE hTech = pEffect->GetCurrentTechnique();

				// get rendercolortarget string from this pass
				GGHANDLE hPass = pEffect->GetPass( hTech, uPass );
				GGHANDLE hRT = pEffect->GetAnnotationByName( hPass, "RenderColorTarget" );
				const char* szRT = 0;
				if ( hRT ) pEffect->GetString( hRT, &szRT );

				// if detect, use special depth texture render target
				if ( szRT && strnicmp( szRT, "[depthtexture]", strlen("[depthtexture]") )==NULL ) 
				{
					// NOTE: Optimize here, should not be using strings/compares
					if ( g_pGlob->dwRenderCameraID==0 )
					{
						// only render depths from camera zero
						int iSuccess = SwitchRenderTargetToDepth(0);
					}
					else
					{
						// actually SKIP this pass if any other camera
						return false;
					}
				}
				else
				{
					// check if RT string has contents
					if ( hRT && strcmp( szRT, "" ) != 0 )
					{
						// yes, now get handle to texture param we want to re-direct our render to
						GGHANDLE hTexParamWant = pEffect->GetParameterByName( NULL, szRT );

						// go through all textures recorded during the shader parse
						for ( DWORD t = 0; t < pMesh->pVertexShaderEffect->m_dwTextureCount; t++ )
						{
							// only consider RT textures that are flagged in bitmask
							DWORD dwThisTextureBit = 1 << t;
							if ( pMesh->pVertexShaderEffect->m_dwCreatedRTTextureMask & dwThisTextureBit )
							{
								// get the param handle of each texture in shader
								int iParam = 0;
								GGHANDLE hTexParam = NULL;
								if ( t<=31 ) 
								{
									iParam = pMesh->pVertexShaderEffect->m_iParamOfTexture [ t ];
									hTexParam = pEffect->GetParameter( NULL, iParam );
								}

								// if it matches the one we want
								if ( hTexParam == hTexParamWant )
								{
									// get texture ptr from effect
									IDirect3DBaseTexture9* pRTTex = NULL;
									pEffect->GetTexture( hTexParam, &pRTTex );

									// switch render target to internal shader RT
									IGGSurface *pSurface;
									((IGGTexture*)pRTTex)->GetSurfaceLevel( 0, &pSurface );
									m_pD3D->SetRenderTarget( 0, pSurface );
									if ( pSurface ) pSurface->Release( );

									// put RT textures width and height into ViewSize
									GGHANDLE hViewSize = pEffect->GetParameterBySemantic( NULL, "ViewSize" );
									if ( hViewSize )
									{
										D3DSURFACE_DESC desc;
										pCurrentRenderTarget->GetDesc( &desc );
										int width = desc.Width, height = desc.Height;
										GGHANDLE hWidth = pEffect->GetAnnotationByName( hTexParam, "width" );
										GGHANDLE hHeight = pEffect->GetAnnotationByName( hTexParam, "height" );
										if ( hWidth ) pEffect->GetInt( hWidth, &width );
										if ( hHeight ) pEffect->GetInt( hWidth, &height );
										GGVECTOR4 vec( (float) width, (float) height, 0, 0 );
										pEffect->SetVector( hViewSize, &vec );
									}
								}
							}
						}
					}
					else
					{
						// no, we render to current as normal
						m_pD3D->SetRenderTarget( 0, pCurrentRenderTarget );
						m_pD3D->SetDepthStencilSurface( pCurrentDepthTarget );

						// set the Viewsize to match the width and height of the RT passed in
						GGHANDLE hViewSize = pEffect->GetParameterBySemantic( NULL, "ViewSize" );
						if ( hViewSize )
						{
							D3DSURFACE_DESC desc;
							pCurrentRenderTarget->GetDesc( &desc );
							GGVECTOR4 vec( (float) desc.Width, (float) desc.Height, 0, 0 );
							pEffect->SetVector( hViewSize, &vec );
						}
					}
				}

				// once textures established, commit effect state changes and begin this pass
				bMustCommit = true;
				#endif
			}

			// U77 - 270111 - pass clipping data to shader (automatic) (duplicated in terrain renderer:BT_Intern_RenderTerrain)
			if ( pMesh->pVertexShaderEffect->m_VecClipPlaneEffectHandle )
			{
				GGVECTOR4 vec;
				tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData ( g_pGlob->dwRenderCameraID );
				if ( m_Camera_Ptr )
				{
					if ( m_Camera_Ptr->iClipPlaneOn==1 )
					{
						// special mode which creates plane but does not use RenderState to set clip
						// as you cannot mix FF clip and HLSL clip in same scene (artefacts)
						vec.x = m_Camera_Ptr->planeClip.a;
						vec.y = m_Camera_Ptr->planeClip.b;
						vec.z = m_Camera_Ptr->planeClip.c;
						vec.w = m_Camera_Ptr->planeClip.d;
					}
					else
					{
						// ensure shader stops using clip plane when not being clipped!
						vec = GGVECTOR4( 0.0f, 1.0f, 0.0f, 99999.0f );
					}
				}
				else
				{
					// ensure shader stops using clip plane when not being clipped!
					vec = GGVECTOR4( 0.0f, 1.0f, 0.0f, 99999.0f );
				}
				#ifdef DX11
				pMesh->pVertexShaderEffect->m_VecClipPlaneEffectHandle->AsVector()->SetFloatVector ( (float*)&vec );
				#else
				pEffect->SetVector( pMesh->pVertexShaderEffect->m_VecClipPlaneEffectHandle, &vec );
				#endif
				bMustCommit = true;
			}

			// special effect which can override the texture stage of an instanced object (Guru)
			#ifdef DX11
			if ( g_InstanceAlphaControlValue > 0 )
			{
				// eventually we can use the other RGB components to communicate this highlight info!
				DWORD dwRedPart = (g_InstanceAlphaControlValue >> 16) & 0xFF;
				DWORD dwGreenPart = (g_InstanceAlphaControlValue >> 8) & 0xFF;
				DWORD dwBluePart = (g_InstanceAlphaControlValue) & 0xFF;
				GGHANDLE gGlowIntensity = pMesh->pVertexShaderEffect->m_pEffect->GetVariableByName ( "GlowIntensity" );
				if ( gGlowIntensity )
				{
					pMesh->pVertexShaderEffect->m_GlowIntensityHandle = gGlowIntensity;
					GGVECTOR4 vecHighlight = GGVECTOR4((float)dwRedPart/255.0f,(float)dwGreenPart/255.0f,(float)dwBluePart/255.0f,0);
					gGlowIntensity->AsVector()->SetFloatVector((float*)&vecHighlight);
				}
			}
			else
			{
				GGHANDLE gGlowIntensity = pMesh->pVertexShaderEffect->m_GlowIntensityHandle;
				if ( gGlowIntensity )
				{
					pMesh->pVertexShaderEffect->m_GlowIntensityHandle = NULL;
					GGVECTOR4 vecHighlight = GGVECTOR4(0,0,0,0);
					gGlowIntensity->AsVector()->SetFloatVector((float*)&vecHighlight);
					bMustCommit = true;
				}
			}
			#else
			if ( g_InstanceAlphaControlValue > 0 )
			{
				// eventually we can use the other RGB components to communicate this highlight info!
				DWORD dwRedPart = (g_InstanceAlphaControlValue >> 16) & 0xFF;
				DWORD dwGreenPart = (g_InstanceAlphaControlValue >> 8) & 0xFF;
				DWORD dwBluePart = (g_InstanceAlphaControlValue) & 0xFF;
				GGHANDLE gGlowIntensity = pMesh->pVertexShaderEffect->m_pEffect->GetParameterByName ( NULL, "GlowIntensity" );
				if ( gGlowIntensity )
				{
					pMesh->pVertexShaderEffect->m_GlowIntensityHandle = gGlowIntensity;
					GGVECTOR4 vecHighlight = GGVECTOR4((float)dwRedPart/255.0f,(float)dwGreenPart/255.0f,(float)dwBluePart/255.0f,0);
					pMesh->pVertexShaderEffect->GGSetEffectVector( gGlowIntensity, &vecHighlight );
					bMustCommit = true;
				}
			}
			else
			{
				GGHANDLE gGlowIntensity = pMesh->pVertexShaderEffect->m_GlowIntensityHandle;
				if ( gGlowIntensity )
				{
					pMesh->pVertexShaderEffect->m_GlowIntensityHandle = NULL;
					GGVECTOR4 vecHighlight = GGVECTOR4(0,0,0,0);
					pMesh->pVertexShaderEffect->GGSetEffectVector ( gGlowIntensity, &vecHighlight );
					bMustCommit = true;
				}
			}
			#endif

			// Character Creator Tone Control - Using the object pointer to work out
			if ( pObject && pObject->pCharacterCreator )
			{				
				#ifdef DX11
				if ( pMesh->pVertexShaderEffect->m_ColorTone[0] == NULL )
				{
					pMesh->pVertexShaderEffect->m_ColorTone[0] = pMesh->pVertexShaderEffect->m_pEffect->GetVariableByName ( "ColorTone" );
				}
				if ( pMesh->pVertexShaderEffect->m_ToneMix[0] == NULL )
				{
					pMesh->pVertexShaderEffect->m_ToneMix[0] = pMesh->pVertexShaderEffect->m_pEffect->GetVariableByName ( "ToneMix" );
				}
				if ( pMesh->pVertexShaderEffect->m_ColorTone[0] )
				{
					float fData[16];
					for ( int i = 0 ; i < 4 ; i++ )
					{
						fData[(i*4)+0] = pObject->pCharacterCreator->ColorTone[i][0];
						fData[(i*4)+1] = pObject->pCharacterCreator->ColorTone[i][1];
						fData[(i*4)+2] = pObject->pCharacterCreator->ColorTone[i][2];
						fData[(i*4)+3] = 1.0f;
					}
					pMesh->pVertexShaderEffect->m_ColorTone[0]->AsVector()->SetFloatVectorArray ( fData, 0, 4 );
				}
				if ( pMesh->pVertexShaderEffect->m_ToneMix[0] )
				{
					float fData[4];
					for ( int i = 0 ; i < 4 ; i++ )
					{
						fData[i] = pObject->pCharacterCreator->ToneMix[i];
					}
					pMesh->pVertexShaderEffect->m_ToneMix[0]->AsScalar()->SetFloatArray ( fData, 0, 4 );
				}
				#else
				for ( int i = 0 ; i < 4 ; i++ )
				{
					if ( pMesh->pVertexShaderEffect->m_ColorTone[i] == NULL )
					{
						char s[256];
						sprintf ( s , "ColorTone[%i]" , i );
						#ifdef DX11
						pMesh->pVertexShaderEffect->m_ColorTone[i] = pMesh->pVertexShaderEffect->m_pEffect->GetVariableByName ( s );
						#else
						pMesh->pVertexShaderEffect->m_ColorTone[i] = pMesh->pVertexShaderEffect->m_pEffect->GetParameterByName ( NULL, s );
						#endif
					}
					if ( pMesh->pVertexShaderEffect->m_ToneMix[i] == NULL )
					{
						char s[256];
						sprintf ( s , "ToneMix[%i]" , i );
						#ifdef DX11
						pMesh->pVertexShaderEffect->m_ToneMix[i] = pMesh->pVertexShaderEffect->m_pEffect->GetVariableByName ( s );
						#else
						pMesh->pVertexShaderEffect->m_ToneMix[i] = pMesh->pVertexShaderEffect->m_pEffect->GetParameterByName ( NULL, s );
						#endif
					}

					if ( pMesh->pVertexShaderEffect->m_ColorTone[i] )
					{
						GGVECTOR4 vecColorTone = GGVECTOR4(pObject->pCharacterCreator->ColorTone[i][0],pObject->pCharacterCreator->ColorTone[i][1],pObject->pCharacterCreator->ColorTone[i][2],1.0f);
						#ifdef DX11
						pMesh->pVertexShaderEffect->m_ColorTone[i]->AsVector()->SetFloatVector ( (float*)&vecColorTone );
						#else
						pMesh->pVertexShaderEffect->GGSetEffectVector( pMesh->pVertexShaderEffect->m_ColorTone[i], &vecColorTone );
						#endif
					}
					if ( pMesh->pVertexShaderEffect->m_ToneMix[i] )
					{
						#ifdef DX11
						pMesh->pVertexShaderEffect->m_ToneMix[i]->AsVector()->SetFloatVector ( (float*)&pObject->pCharacterCreator->ToneMix[i] );
						#else
						pMesh->pVertexShaderEffect->GGSetEffectFloat( pMesh->pVertexShaderEffect->m_ToneMix[i], pObject->pCharacterCreator->ToneMix[i] );
						#endif
					}
				}
				#endif
				bMustCommit = true;
				pMesh->pVertexShaderEffect->m_bCharacterCreatorTonesOn = true;
			}
			else
			{		
				if ( pMesh->pVertexShaderEffect->m_bCharacterCreatorTonesOn == true )
				{
					#ifdef DX11
					if ( pMesh->pVertexShaderEffect->m_ColorTone[0] == NULL )
					{
						pMesh->pVertexShaderEffect->m_ColorTone[0] = pMesh->pVertexShaderEffect->m_pEffect->GetVariableByName ( "ColorTone" );
					}
					if ( pMesh->pVertexShaderEffect->m_ColorTone[0] )
					{
						float fData[16];
						for ( int i = 0 ; i < 4 ; i++ )
						{
							fData[(i*4)+0] = -1;
							fData[(i*4)+1] = 0;
							fData[(i*4)+2] = 0;
							fData[(i*4)+3] = 1.0f;
						}
						pMesh->pVertexShaderEffect->m_ColorTone[0]->AsVector()->SetFloatVectorArray ( fData, 0, 4 );
					}
					#else
					for ( int i = 0 ; i < 4 ; i++ )
					{
						if ( pMesh->pVertexShaderEffect->m_ColorTone[i] == NULL )
						{
							char s[256];
							sprintf ( s , "ColorTone[%i]" , i );
							#ifdef DX11
							pMesh->pVertexShaderEffect->m_ColorTone[i] = pMesh->pVertexShaderEffect->m_pEffect->GetVariableByName ( s );
							#else
							pMesh->pVertexShaderEffect->m_ColorTone[i] = pMesh->pVertexShaderEffect->m_pEffect->GetParameterByName ( NULL, s );
							#endif
						}

						if ( pMesh->pVertexShaderEffect->m_ColorTone[i] )
						{
							GGVECTOR4 vecColorTone = GGVECTOR4(-1,0,0,1.0f);
							#ifdef DX11
							pMesh->pVertexShaderEffect->m_ColorTone[i]->AsVector()->SetFloatVector ( (float*)&vecColorTone );
							#else
							pMesh->pVertexShaderEffect->GGSetEffectVector( pMesh->pVertexShaderEffect->m_ColorTone[i], &vecColorTone );
							#endif
						}
					}
					#endif
					bMustCommit = true;
					pMesh->pVertexShaderEffect->m_bCharacterCreatorTonesOn = false;
				}
			}

			// added specular override to per object rendering
			#ifdef DX11
			GGHANDLE pSpecularOverride = pMesh->pVertexShaderEffect->m_pEffect->GetVariableByName ( "SpecularOverride" );
			if ( pSpecularOverride )
			{
				pSpecularOverride->AsScalar()->SetFloat ( pMesh->fSpecularOverride );
				bMustCommit = true;
			}
			#else
			GGHANDLE pSpecularOverride = pMesh->pVertexShaderEffect->m_pEffect->GetParameterByName ( NULL, "SpecularOverride" );
			if ( pSpecularOverride )
			{
				pMesh->pVertexShaderEffect->GGSetEffectFloat( pSpecularOverride, pMesh->fSpecularOverride );
				bMustCommit = true;
			}
			#endif

			// added UV control to per object rendering
			#ifdef DX11
			GGHANDLE pScrollScaleUV = pMesh->pVertexShaderEffect->m_pEffect->GetVariableByName ( "ScrollScaleUV" );
			if ( pScrollScaleUV )
			{
				GGVECTOR4 vec4 = GGVECTOR4 ( pMesh->fScrollOffsetU, pMesh->fScrollOffsetV, pMesh->fScaleOffsetU, pMesh->fScaleOffsetV );
				pScrollScaleUV->AsVector()->SetFloatVector ( (float*)&vec4 );
			}
			#endif

			// added per-object control for additional artist flags
			#ifdef DX11
			GGHANDLE pArtFlags = pMesh->pVertexShaderEffect->m_pEffect->GetVariableByName ( "ArtFlagControl1" );
			if ( pArtFlags )
			{
				float fInvertNormal = 0.0f;
				float fGenerateTangents = 0.0f;
				if ( pMesh->dwArtFlags & 0x1 ) fInvertNormal = 1.0f;
				if ( pMesh->dwArtFlags & 0x2 ) fGenerateTangents = 1.0f;
				float fBoostIntensity = pMesh->fBoostIntensity;
				GGVECTOR4 vec4 = GGVECTOR4 ( fInvertNormal, fGenerateTangents, fBoostIntensity, 0.0f );
				pArtFlags->AsVector()->SetFloatVector ( (float*)&vec4 );
			}
			#endif

			// when flagged, we must update effect with changes we made
			if ( bMustCommit==true )
			{
				// commit effect state changes to begin this pass
				#ifdef DX11
				#else
				pEffect->CommitChanges( );
				#endif
			}
		}

		// FX Effect or Regular
		if ( bEffectRendering )
		{
			// disable fog for shaders and begin effect rendering
			#ifdef DX11

			if (g_UseOutLine) {
				GGTECHNIQUEHANDLE hOldTechnique = pMesh->pVertexShaderEffect->m_hCurrentTechnique;

				LPGGEFFECT pEffect = pMesh->pVertexShaderEffect->m_pEffect;

				pMesh->pVertexShaderEffect->m_hCurrentTechnique = pEffect->GetTechniqueByName("OutLine");
				if (!pMesh->pVertexShaderEffect->m_hCurrentTechnique) {
					pMesh->pVertexShaderEffect->m_hCurrentTechnique = hOldTechnique;
				}
				ID3DX11EffectTechnique* pTech = pMesh->pVertexShaderEffect->m_hCurrentTechnique;//m_pEffect->GetTechniqueByIndex(0);
				ID3DX11EffectPass* pPass = pTech->GetPassByIndex(uPass);
				pPass->Apply(0, m_pImmediateContext);
				pMesh->pVertexShaderEffect->m_hCurrentTechnique = hOldTechnique;
			}
			else if (g_UseWireFrame) {
				GGTECHNIQUEHANDLE hOldTechnique = pMesh->pVertexShaderEffect->m_hCurrentTechnique;

				LPGGEFFECT pEffect = pMesh->pVertexShaderEffect->m_pEffect;

				pMesh->pVertexShaderEffect->m_hCurrentTechnique = pEffect->GetTechniqueByName("WireFrameMode");
				if (!pMesh->pVertexShaderEffect->m_hCurrentTechnique) {
					pMesh->pVertexShaderEffect->m_hCurrentTechnique = hOldTechnique;
				}
				ID3DX11EffectTechnique* pTech = pMesh->pVertexShaderEffect->m_hCurrentTechnique;
				ID3DX11EffectPass* pPass = pTech->GetPassByIndex(uPass);
				pPass->Apply(0, m_pImmediateContext);
				pMesh->pVertexShaderEffect->m_hCurrentTechnique = hOldTechnique;

			}
			else {
				ID3DX11EffectTechnique* pTech = pMesh->pVertexShaderEffect->m_hCurrentTechnique;//m_pEffect->GetTechniqueByIndex(0);
				ID3DX11EffectPass* pPass = pTech->GetPassByIndex(uPass);
				pPass->Apply(0, m_pImmediateContext);
			}
			#else
			m_pD3D->SetRenderState ( D3DRS_FOGENABLE, FALSE );
			pMesh->pVertexShaderEffect->m_pEffect->BeginPass ( uPass );
			#endif
		}

		// old FF texturing code (some effects do not do any texturing stuff)
		// this allowed non PS shader to use DBP textures but it killed shader ability to use DBP textures that HAD PS code!
		if ( bEffectRendering )
		{
			// effects CAN use 'texture object' textures if the effect did not assign a specfic texture to them (paul request for DarkSHADER)
			if ( pMesh->pTextures )
			{
				for ( DWORD dwTextureIndex = 0; dwTextureIndex < pMesh->dwTextureCount; dwTextureIndex++ )
				{
					DWORD dwTextureStage = pMesh->pTextures [ dwTextureIndex ].dwStage;
					if ( dwTextureStage < 16 )
					{
						// get texture ptr
						sTexture* pTexture = &pMesh->pTextures [ dwTextureIndex ];

						// m_dwUseDynamicTextureMask holds a mask of 32 bits, 1=use dynamic texture form texture object command
						int iUseDyntex = ( ( pMesh->pVertexShaderEffect->m_dwUseDynamicTextureMask >> dwTextureStage ) & 1 );
						if ( iUseDyntex==1 )
						{
							// when in effect, only if texture in effect is NULL should this be allowed
							#ifdef DX11
							#else
							if ( pTexture->pTexturesRef )
							{
								m_pD3D->SetTexture ( dwTextureStage, pTexture->pTexturesRef );
							}
							else
							{
								if ( pMesh->pTextures [ dwTextureIndex ].pCubeTexture )
									m_pD3D->SetTexture ( dwTextureStage, pTexture->pCubeTexture );
								else
									m_pD3D->SetTexture ( dwTextureStage, NULL);
							}
							#endif
						}
					}
				}

				// set dynamic depth mapping texture for this effect
				if ( pMesh->pVertexShaderEffect )
				{
					if ( pMesh->pVertexShaderEffect->m_pEffect==g_pMainCameraDepthEffect )
					{
						#ifdef DX11
						tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData ( g_pGlob->dwCurrentSetCameraID );
						if ( m_Camera_Ptr )
						{
							g_pMainCameraDepthHandle->AsShaderResource()->SetResource(m_Camera_Ptr->pImageDepthResourceView);
						}
						#else
						g_pMainCameraDepthEffect->SetTexture ( g_pMainCameraDepthHandle, g_pMainCameraDepthTexture );
						#endif
					}
				}

				// set dynamic shadow mapping texture for this effect
				if ( g_CascadedShadow.m_depthTexture[0] )
				{
					if ( pMesh->pVertexShaderEffect->m_bPrimaryEffectForCascadeShadowMapping==true )
					{
						// depth handles only relate to ONE shader
						#ifdef DX11
#ifdef WICKEDENGINE
						for ( int i = 0; i < 8; i++ )
							if ( g_CascadedShadow.m_depthHandle[i] && g_CascadedShadow.m_depthTexture[i] ) 
								g_CascadedShadow.m_depthHandle[i]->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[i]->getTextureResourceView() );
#endif
						#else
						pMesh->pVertexShaderEffect->m_pEffect->SetTexture ( g_CascadedShadow.m_depthHandle[0], g_CascadedShadow.m_depthTexture[0]->getTexture() );
						pMesh->pVertexShaderEffect->m_pEffect->SetTexture ( g_CascadedShadow.m_depthHandle[1], g_CascadedShadow.m_depthTexture[1]->getTexture() );
						pMesh->pVertexShaderEffect->m_pEffect->SetTexture ( g_CascadedShadow.m_depthHandle[2], g_CascadedShadow.m_depthTexture[2]->getTexture() );
						pMesh->pVertexShaderEffect->m_pEffect->SetTexture ( g_CascadedShadow.m_depthHandle[3], g_CascadedShadow.m_depthTexture[3]->getTexture() );
						#endif
					}
					else
					{
						// any effect that has DepthMapTX4 will be filled with most distant shadow cascade render
						DWORD dwEffectIndex = pMesh->pVertexShaderEffect->m_dwEffectIndex;
						if ( dwEffectIndex < EFFECT_INDEX_SIZE )
						{
							if ( g_CascadedShadow.m_pEffectParam[dwEffectIndex] )
							{
								GGHANDLE hdepthHandle0 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX1;
								GGHANDLE hdepthHandle1 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX2;
								GGHANDLE hdepthHandle2 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX3;
								GGHANDLE hdepthHandle3 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX4;
								GGHANDLE hdepthHandle4 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX5;
								GGHANDLE hdepthHandle5 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX6;
								GGHANDLE hdepthHandle6 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX7;
								GGHANDLE hdepthHandle7 = g_CascadedShadow.m_pEffectParam[dwEffectIndex]->DepthMapTX8;
								#ifdef DX11
#ifdef WICKEDENGINE
								if ( hdepthHandle0 && g_CascadedShadow.m_depthTexture[0] ) hdepthHandle0->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[0]->getTextureResourceView() );
								if ( hdepthHandle1 && g_CascadedShadow.m_depthTexture[1] ) hdepthHandle1->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[1]->getTextureResourceView() );
								if ( hdepthHandle2 && g_CascadedShadow.m_depthTexture[2] ) hdepthHandle2->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[2]->getTextureResourceView() );
								if ( hdepthHandle3 && g_CascadedShadow.m_depthTexture[3] ) hdepthHandle3->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[3]->getTextureResourceView() );
								if ( hdepthHandle4 && g_CascadedShadow.m_depthTexture[4] ) hdepthHandle4->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[4]->getTextureResourceView() );
								if ( hdepthHandle5 && g_CascadedShadow.m_depthTexture[5] ) hdepthHandle5->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[5]->getTextureResourceView() );
								if ( hdepthHandle6 && g_CascadedShadow.m_depthTexture[6] ) hdepthHandle6->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[6]->getTextureResourceView() );
								if ( hdepthHandle7 && g_CascadedShadow.m_depthTexture[7] ) hdepthHandle7->AsShaderResource()->SetResource ( g_CascadedShadow.m_depthTexture[7]->getTextureResourceView() );
#endif
								#else
								if ( hdepthHandle0 ) pMesh->pVertexShaderEffect->m_pEffect->SetTexture ( hdepthHandle0, g_CascadedShadow.m_depthTexture[0]->getTexture() );
								if ( hdepthHandle1 ) pMesh->pVertexShaderEffect->m_pEffect->SetTexture ( hdepthHandle1, g_CascadedShadow.m_depthTexture[1]->getTexture() );
								if ( hdepthHandle2 ) pMesh->pVertexShaderEffect->m_pEffect->SetTexture ( hdepthHandle2, g_CascadedShadow.m_depthTexture[2]->getTexture() );
								if ( hdepthHandle3 ) pMesh->pVertexShaderEffect->m_pEffect->SetTexture ( hdepthHandle3, g_CascadedShadow.m_depthTexture[3]->getTexture() );
								#endif
							}
						}
					}
				}
			}
		}
		else
		{
			// FIXED FUNCTION TEXTURING
			#ifdef DX11
			#else
			// FF affects HLSL pipeline (and vice versa), so switch on
			// the automated clipping plane if end of override
			if ( m_RenderStates.bOverriddenClipPlaneforHLSL==true )
			{
				tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData ( g_pGlob->dwRenderCameraID );
				if ( m_Camera_Ptr )
				{
					if ( m_Camera_Ptr->iClipPlaneOn!=0 )
						m_pD3D->SetRenderState ( D3DRS_CLIPPLANEENABLE, D3DCLIPPLANE0 );
					else
						m_pD3D->SetRenderState ( D3DRS_CLIPPLANEENABLE, 0x00 );
				}
				m_RenderStates.bOverriddenClipPlaneforHLSL = false;
			}

			// call the texturestate function
			if ( !SetMeshTextureStates ( pMesh ) )
				return bResult;

			// is there a texture
			if ( pMesh->pTextures )
			{
				// store the current texture
				m_iCurrentTexture = pMesh->pTextures [ 0 ].iImageID;

				// is it different to the last texture we set (leefix-040803-and only if single texture otherwise lightmaps might be used)
				if ( m_iCurrentTexture != m_iLastTexture || pMesh->dwTextureCount>1 )
				{
					// set the new texture - along with related stage textures
					for ( DWORD dwTextureIndex = 0; dwTextureIndex < pMesh->dwTextureCount; dwTextureIndex++ )
					{
						// Determine texture stage to write to
						DWORD dwTextureStage = pMesh->pTextures [ dwTextureIndex ].dwStage;

						// Determine texture data ptr
						sTexture* pTexture = &pMesh->pTextures [ dwTextureIndex ];

						if ( pTexture->pTexturesRef )
						{
							// set regular texture
							if ( FAILED ( m_pD3D->SetTexture ( dwTextureStage, pTexture->pTexturesRef ) ) )
								break;
						}
						else
						{
							if ( pMesh->pTextures [ dwTextureIndex ].pCubeTexture )
							{
								// set cube texture
								if ( FAILED ( m_pD3D->SetTexture ( dwTextureStage, pTexture->pCubeTexture ) ) )
									break;
							}
							else
							{
								// set no texture
								if ( FAILED ( m_pD3D->SetTexture ( dwTextureStage, NULL) ) )
									break;
							}
						}
					}

					// now store the current texture
					m_iLastTexture = m_iCurrentTexture;
				}
			}
			else
			{
				// default zero texture
				m_pD3D->SetTexture ( 0, NULL );
				m_iLastTexture = 0;
			}
			#endif
		}
	}

	// always success (unless exited early with false from depth render skip)
	return bResult;
}

bool CObjectManager::ShaderPassEnd ( sMesh* pMesh, bool bEffectRendering )
{
	// End FX Effect
	#ifdef DX11
	#else
	if ( bEffectRendering )	pMesh->pVertexShaderEffect->m_pEffect->EndPass();
	#endif

	// continue
	return true;
}

bool CObjectManager::ShaderFinish ( sMesh* pMesh, LPGGRENDERTARGETVIEW pCurrentRenderTarget, LPGGDEPTHSTENCILVIEW pCurrentDepthTarget )
{
	// if using RT, restore current render target
	if ( pCurrentRenderTarget )
	{
		if ( pMesh->pVertexShaderEffect )
		{
			if ( pMesh->pVertexShaderEffect->m_bUsesAtLeastOneRT==true )
			{
				SetRenderAndDepthTarget ( pCurrentRenderTarget, pCurrentDepthTarget );
			}
		}
	}

	// Run any end code for any effect used
	if ( pMesh->pVertexShaderEffect )
	    pMesh->pVertexShaderEffect->End();

	/*
	// free dynamic shadow mapping to release input stage (ready for next output stage)
	if ( g_CascadedShadow.m_depthTexture[0] )
	{
		if ( pMesh->pVertexShaderEffect->m_bPrimaryEffectForCascadeShadowMapping==true )
		{
			for ( int i = 0; i < 8; i++ )
				if ( g_CascadedShadow.m_depthHandle[i] && g_CascadedShadow.m_depthTexture[i] ) 
					g_CascadedShadow.m_depthHandle[i]->AsShaderResource()->SetResource ( NULL );
		}
	}
	*/

	// continue
	return true;
}

