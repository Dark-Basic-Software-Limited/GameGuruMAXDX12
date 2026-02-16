//PE: 50000+ to be used for internal images inside dbo's.

DARKSDK bool FindInternalImage ( char* szFilename, int* pImageID )
{
	if ( szFilename && szFilename[0] )
	{
    	int iFindFilenameLength = strlen(szFilename);

		ImagePtr pCheck = m_List.begin();
		while ( pCheck != m_List.end() ) //PE: scan everything. && pCheck->first < 0
		{
			//PE: We cant reuse IDs from CCP as they can change, so always create new id's.
			//g. undefined: if (!(pCheck->first > g.charactercreatorEditorImageoffset && pCheck->first < g.charactercreatorEditorImageoffset + 500))
			if (!(pCheck->first > 95000 && pCheck->first < 95500))
			{
				// get a pointer to the actual data
				tagImgData* ptr = pCheck->second;
				if (ptr && ptr->szShortFilename && ptr->lpTexture)
				{
					if (_stricmp(ptr->szShortFilename, szFilename) == 0)
					{
						*pImageID = pCheck->first;
						return true;
					}
				}
			}
            ++pCheck;
		}
	}

	//PE: We need + imageID if we are going to reuse them there is a lot of ID > 0 around the code.

	static int iTry = 50000;
	ImagePtr pCheck = m_List.find(iTry);
	while (pCheck != m_List.end())
	{
		pCheck = m_List.find(iTry++);
		if (iTry > 58000) break;
	}

	if (iTry > 58000) {

		//PE: None positive found or MAX , use the old way.

		// Static, so not reset between function calls - almost guaranteed to find the
		// next free image that way.
		static int iTry = -1;

		// Check to see if the image id is in use
		ImagePtr pCheck = m_List.find(iTry);
		while (pCheck != m_List.end())
		{
			// Is in use, check for the next number, but make sure that underflow is dealt with correctly
			// ie, iTry MUST stay negative. Note that this is not too likely to happen anyway.
			--iTry;
			if (iTry > 0)
				iTry = -1;

			pCheck = m_List.find(iTry);
		}
	}
	// this image can be used = new slot
	*pImageID = iTry;

	// not found existing image..
	return false;
}

#ifndef NOSTEAMORVIDEO
extern int addinternaltexture(char* tfile_s);
extern int findinternaltexture(char* tfile_s);
extern void removeinternaltexture(int teximg);
extern void deleteinternaltexture(char* tfile_s);
#endif

DARKSDK void ClearAnyLightMapInternalTextures ( void )
{
	ImagePtr pCheck = m_List.begin();
	while ( pCheck != m_List.end() ) //PE: removed scan everything. && pCheck->first < 0
	{
        // get a pointer to the actual data
		tagImgData* ptr = pCheck->second;
		int iFoundID = 0;
		char *shortname;
        if ( ptr && ptr->szShortFilename && ptr->lpTexture )
		{
			if ( ptr->szShortFilename!=NULL )
			{
				if ( strlen(ptr->szShortFilename)>11 )
				{
					for (int n = 0; n < (int)strlen(ptr->szShortFilename) - 11; n++) {
						if (strnicmp(ptr->szShortFilename + n, "lightmaps\\", 10) == NULL) {
							shortname = ptr->szShortFilename;
							iFoundID = pCheck->first;
							break;
						}
					}
				}
			}
        }
		if ( iFoundID!=0 ) 
		{
			//PE: Also need to be removed from internal list.
			#ifndef NOSTEAMORVIDEO
			deleteinternaltexture(shortname);
			#endif
			RemoveImage( iFoundID );
			pCheck = m_List.begin();
		}
		else
		{
	        ++pCheck;
		}
	}
}
//PE:
DARKSDK void ClearAnyEntitybankInternalTextures(void)
{
	ImagePtr pCheck = m_List.begin();
	while (pCheck != m_List.end()) //PE: removed scan everything. && pCheck->first < 0
	{
		// get a pointer to the actual data
		tagImgData* ptr = pCheck->second;
		int iFoundID = 0;
		char *shortname;
		if (ptr && ptr->szShortFilename && ptr->lpTexture)
		{
			if (ptr->szShortFilename != NULL)
			{
				if (strlen(ptr->szShortFilename)>11)
				{
					for (int n = 0; n < (int)strlen(ptr->szShortFilename) - 11; n++) {
						if (strnicmp(ptr->szShortFilename + n, "entitybank\\", 10) == NULL) {
							shortname = ptr->szShortFilename;
							iFoundID = pCheck->first;
							break;
						}
					}
				}
			}
		}
		if (iFoundID != 0)
		{
			//PE: Also need to be removed from internal list.
			#ifndef NOSTEAMORVIDEO
			deleteinternaltexture(shortname);
			#endif
			RemoveImage(iFoundID);
			pCheck = m_List.begin();
		}
		else
		{
			++pCheck;
		}
	}
}

// This load is NOT the main DBPro image loader - it is used here though (Load(x,x,x,x))
DARKSDK int LoadImageCoreInternal ( char* szFilename, int iDivideTextureSize )
{
	// does image already exist?
	int iImageID = 0;
	if ( !FindInternalImage ( szFilename, &iImageID ) )
	{
		//PE: Check if we have it in the internal list.
		#ifndef NOSTEAMORVIDEO
		int tmpid = findinternaltexture(szFilename);
		if (tmpid > 0) {
			//char mdebug[2048];
			//sprintf(mdebug, "findinternaltexture: %s, return: %d ", szFilename, tmpid );
			//timestampactivity(0, mdebug);
			return(tmpid);
		}
		#endif
		// copy of filename to attempt to load
		char VirtualFilename[MAX_PATH];
		strcpy ( VirtualFilename, szFilename );

		CheckForWorkshopFile ( VirtualFilename );

		// no, use standard loader
		g_pGlob->Decrypt( VirtualFilename );

		//PE: Add to list so it can be reused.
		#ifndef NOSTEAMORVIDEO
		tmpid = addinternaltexture(szFilename);
		if ( tmpid > 0 ) {
			iImageID = tmpid;
		}
		#endif

		if ( LoadImageCoreRetainName ( szFilename, (LPSTR)VirtualFilename, iImageID, 0, true, iDivideTextureSize ) )
		{
			// new image returned
		}
		else
		{
			// load failed
			//PE: Remove entry again.
			#ifndef NOSTEAMORVIDEO
			removeinternaltexture(iImageID);
			#endif
			iImageID=0;
		}
		g_pGlob->Encrypt( VirtualFilename );
	}
	return iImageID;
}

DARKSDK int LoadImageInternalEx ( char* szFilename, int iDivideTextureSize )
{
	return LoadImageCoreInternal ( szFilename, iDivideTextureSize );
}

DARKSDK int LoadImageInternal ( char* szFilename )
{
	return LoadImageCoreInternal ( szFilename, 0 );
}

DARKSDK LPGGTEXTURE GetTexture ( char* szFilename, GGIMAGE_INFO* info, int iOneToOnePixels )
{
	// Uses actual or virtual file..
	char VirtualFilename[_MAX_PATH];
	strcpy(VirtualFilename, szFilename);
	//g_pGlob->UpdateFilenameFromVirtualTable( VirtualFilename);

	CheckForWorkshopFile ( VirtualFilename );

	// Decrypt and use media, re-encrypt
	g_pGlob->Decrypt( VirtualFilename );
	LPGGTEXTURE Res = GetTextureCore ( VirtualFilename, info, iOneToOnePixels, 0, 0 );
	g_pGlob->Encrypt( VirtualFilename );
	return Res;
}

DARKSDK LPGGTEXTURE GetImagePointer ( int iID )
{
	// update internal data
	if ( !UpdatePtrImage ( iID ) )
		return NULL;

	if (m_imgptr->lpTexture) {

#ifdef PETESTIMAGEUSAGE
		m_imgptr->AccessCountCPU++;
#endif
		return m_imgptr->lpTexture;
	}
	else
		return NULL;
}

DARKSDK LPGGTEXTUREREF GetImagePointerView ( int iID )
{
	#ifdef DX11
	// update internal data
	if ( !UpdatePtrImage ( iID ) )
		return NULL;

	// DX12 path: lazy-load texture via the DX12 bridge
	extern bool ImGui_DX12_IsInitialized();
	if (ImGui_DX12_IsInitialized() && m_imgptr->lpTextureView == NULL)
	{
		extern void* ImGui_DX12_GetOrLoadTexture(int imageId, const char* filepath);
		// Try long filename first, fall back to short filename
		void* texId = ImGui_DX12_GetOrLoadTexture(iID, m_imgptr->szLongFilename);
		if (!texId && m_imgptr->szShortFilename[0])
			texId = ImGui_DX12_GetOrLoadTexture(iID, m_imgptr->szShortFilename);
		return (LPGGTEXTUREREF)texId;
	}

	if (m_imgptr->lpTextureView) {
#ifdef PETESTIMAGEUSAGE
		m_imgptr->AccessCountGPU++;
#endif
		return m_imgptr->lpTextureView;
	}
	else
		return NULL;
	#else
	return GetImagePointer(iID);
	#endif
}

DARKSDK void SetImagePointerView ( int iID, LPGGRENDERTARGETVIEW pView )
{
	#ifdef DX11
	//hmm, maybe render target view is different from shader resource view
	//if ( !UpdatePtrImage ( iID ) ) return;
	//m_imgptr->lpTextureView = (ID3D11ShaderResourceView*)pView;
	#endif
}

DARKSDK int ImageWidth ( int iID )
{
	// get the width of an image

	// update internal data
	if ( !UpdatePtrImage ( iID ) )
		return -1;

	// return the width
	return m_imgptr->iWidth;
}

DARKSDK int ImageHeight ( int iID )
{
	// get the height of an image

	// update internal data
	if ( !UpdatePtrImage ( iID ) )
		return -1;

	// return the height
	return m_imgptr->iHeight;
}

DARKSDK float ImageUMax ( int iID )
{
	// get the height of an image

	// update internal data
	if ( !UpdatePtrImage ( iID ) )
		return 1.0f;

	// return the fTexUMax
	return m_imgptr->fTexUMax;
}

DARKSDK float ImageVMax ( int iID )
{
	// get the width of an image

	// update internal data
	if ( !UpdatePtrImage ( iID ) )
		return 1.0f;

	// return the fTexVMax
	return m_imgptr->fTexVMax;
}

DARKSDK void SetImageSharing ( bool bMode )
{
	// sets up the sharing mode, this helps to conserve
	// memory as it won't load in the same image more
	// than once

	// save mode
	m_bSharing = bMode;
}

DARKSDK void SetImageMemory ( int iMode )
{
	// sets the memory mode for the images

	// iMode possible values
	// 0 - default
	// 1 - managed ( must use this to lock textures )
	// 2 - system

	// check that the mode is valid
	if ( iMode < 0 || iMode > 2 )
		Error1 ( "Invalid memory mode specified" );

	// save the mode
	m_iMemory = iMode;
}

DARKSDK void LockImage ( int iID )
{
	// lock the specified image, this allows you to perform
	// actions like plotting pixels etc
	if ( !UpdatePtrImage ( iID ) )
		return;

	#ifdef DX11
	#else
	HRESULT hr;
	if ( !m_imgptr->bLocked )
	{
		memset ( &m_imgptr->d3dlr, 0, sizeof ( GGLOCKED_RECT ) );
		if ( FAILED ( hr = m_imgptr->lpTexture->LockRect ( 0, &m_imgptr->d3dlr, 0, 0 ) ) )
			Error ( "Failed to lock image for image library" );
		m_imgptr->bLocked = true;
	}
	else
		Error ( "Failed to lock image for image library as it's already locked" );
	#endif
}

DARKSDK void UnlockImage ( int iID )
{
	if ( !UpdatePtrImage ( iID ) )
		return;

	#ifdef DX11
	#else
	m_imgptr->lpTexture->UnlockRect ( NULL );
	m_imgptr->bLocked = false;
	#endif
}

DARKSDK void WriteImage ( int iID, int iX, int iY, int iA, int iR, int iG, int iB )
{
	if ( !UpdatePtrImage ( iID ) )
		return;

	if ( !m_imgptr->bLocked )
		Error1 ( "Unable to modify texture data for image library as it isn't locked" );
	
	#ifdef DX11
	#else
	DWORD* pPix = ( DWORD* ) m_imgptr->d3dlr.pBits;
	pPix [ ( ( m_imgptr->d3dlr.Pitch >> 4 ) * iX ) + iY ] = GGCOLOR_ARGB ( iA, iR, iG, iB );
	#endif
}

DARKSDK void GetImage ( int iID, int iX, int iY, int* piR, int* piG, int* piB )
{
	// get a pixel at x, y

	// check pointers are valid
	if ( !piR || !piG || !piB )
		return;

	// update the internal data
	if ( !UpdatePtrImage ( iID ) )
		return;

	// check the image is locked
	if ( !m_imgptr->bLocked )
		Error1 ( "Unable to get texture data for image library as it isn't locked" );

	#ifdef DX11
	#else
	// get a pointer to the data
	DWORD* pPix = ( DWORD* ) m_imgptr->d3dlr.pBits;
	DWORD  pGet;

	// get offset in file
	pGet = pPix [ ( ( m_imgptr->d3dlr.Pitch >> 4 ) * iX ) + iY ];

	// break value down
	DWORD dwAlpha = pGet >> 24;						// get alpha
	DWORD dwRed   = ((( pGet ) >> 16 ) & 0xff );	// get red
	DWORD dwGreen = ((( pGet ) >>  8 ) & 0xff );	// get green
	DWORD dwBlue  = ((( pGet ) )       & 0xff );	// get blue

	// save values
	*piR = dwRed;
	*piG = dwGreen;
	*piB = dwBlue;
	#endif
}

DARKSDK LPGGTEXTURE MakeFormat ( int iID, int iWidth, int iHeight, GGFORMAT format, DWORD dwUsageRenderTarget )
{
	// make a new image
	// mike : can specify iID of -1 which means image is added to list (internal use only)

	// variable declarations
	tagImgData*	test = NULL;		// pointer to data structure
	
	// create a new block of memory
	test = new tagImgData;
	memset(test, 0, sizeof(tagImgData));

	// check the memory was created
	if ( test == NULL )
		return NULL;

	// clear out the memory
	memset ( test, 0, sizeof ( tagImgData ) );

	#ifdef DX11
	// all images created need to be shader resources!
	//D3D11_USAGE dwPoolStaging = D3D11_USAGE_STAGING;
	//UINT dwCPUAccess = D3D11_CPU_ACCESS_READ;
	//D3D11_BIND_FLAG dwBind = (D3D11_BIND_FLAG)0;
	//if ( dwUsage!=0 ) 
	//{
	//	dwPoolStaging = D3D11_USAGE_DEFAULT;
	//	dwCPUAccess = 0;
	//	dwBind = D3D11_BIND_SHADER_RESOURCE;
	//}
	D3D11_USAGE dwPoolStaging = D3D11_USAGE_DEFAULT;
	UINT dwCPUAccess = 0;
	DWORD dwBind = D3D11_BIND_SHADER_RESOURCE;
	if ( dwUsageRenderTarget != 0 ) dwBind |= D3D11_BIND_RENDER_TARGET;

	// create a new texture
	GGSURFACE_DESC StagedDesc = { iWidth, iHeight, 1, 1, format, 1, 0, dwPoolStaging, (D3D11_BIND_FLAG)dwBind, dwCPUAccess, 0 };
	HRESULT hr = m_pD3D->CreateTexture2D( &StagedDesc, NULL, (ID3D11Texture2D**)&test->lpTexture );
	if ( FAILED ( hr ) )
	{
		Error1 ( "Failed to create new image" );
		return NULL;
	}

	// setup properties
	test->iHeight = iHeight;		// store the width
	test->iWidth  = iWidth;			// store the height
	test->iDepth  = ImageGetBitDepthFromFormat ( format );
	test->bLocked = false;			// set locked to false

	// get actual dimensions of texture/image
	GGSURFACE_DESC imageddsd;
	LPGGSURFACE pTextureInterface = NULL;
	test->lpTexture->QueryInterface<ID3D11Texture2D>(&pTextureInterface);
	pTextureInterface->GetDesc(&imageddsd);
	SAFE_RELEASE ( pTextureInterface );
	test->fTexUMax=(float)test->iWidth/(float)imageddsd.Width;
	test->fTexVMax=(float)test->iHeight/(float)imageddsd.Height;

	// for DX11, create resource view from texture and store in lpTextureRef
	test->lpTextureView = NULL;
	CreateShaderResourceViewFor ( test, 0, format );

	#else
	D3DPOOL dwPool = D3DPOOL_MANAGED;
	if ( dwUsage!=0 ) dwPool=D3DPOOL_DEFAULT;

	// create a new texture
	HRESULT		hr;					// used for error checking
	hr = D3DXCreateTexture ( 
							  m_pD3D,
							  iWidth,
							  iHeight,
							  1,
							  dwUsage,
							  format,
							  dwPool,
							  &test->lpTexture
					       );

	if ( FAILED ( hr ) )
	{
		Error ( "Failed to create new image" );
		return NULL;
	}

	// setup properties
	test->iHeight = iHeight;		// store the width
	test->iWidth  = iWidth;		// store the height
	test->iDepth  = ImageGetBitDepthFromFormat ( format );
	test->bLocked = false;			// set locked to false

	// get actual dimensions of texture/image
	D3DSURFACE_DESC imageddsd;
	test->lpTexture->GetLevelDesc(0, &imageddsd);
	test->fTexUMax=(float)test->iWidth/(float)imageddsd.Width;
	test->fTexVMax=(float)test->iHeight/(float)imageddsd.Height;
	#endif

	// Ensure smalltextres are handled
	if(test->fTexUMax>1.0f) test->fTexUMax=1.0f;
	if(test->fTexVMax>1.0f) test->fTexVMax=1.0f;

	// add to the list
    m_List.insert( std::make_pair(iID, test) );

	// ensure sprites all updated
	UpdateAllSprites();

	return test->lpTexture;
}

DARKSDK LPGGTEXTURE MakeImage ( int iID, int iWidth, int iHeight )
{
	return MakeFormat ( iID, iWidth, iHeight, g_DefaultGGFORMAT, 0 );
}

DARKSDK LPGGTEXTURE MakeImageUsage ( int iID, int iWidth, int iHeight, DWORD dwUsage )
{
	return MakeFormat ( iID, iWidth, iHeight, g_DefaultGGFORMAT, dwUsage );
}

DARKSDK LPGGTEXTURE MakeImageJustFormat ( int iID, int iWidth, int iHeight, GGFORMAT Format )
{
	return MakeFormat ( iID, iWidth, iHeight, Format, 0 );
}

DARKSDK LPGGTEXTURE MakeImageRenderTarget ( int iID, int iWidth, int iHeight, GGFORMAT Format )
{
	return MakeFormat ( iID, iWidth, iHeight, Format, GGUSAGE_RENDERTARGET );
}

DARKSDK void SetCubeFace ( int iID, LPGGCUBETEXTURE pCubeMap, int iFace )
{
	// get ptr to image
	if ( !UpdatePtrImage ( iID ) )
		return;

	// set cube reference details
	m_imgptr->pCubeMapRef = pCubeMap;
	m_imgptr->iCubeMapFace = iFace;
}

DARKSDK void GetCubeFace ( int iID, LPGGCUBETEXTURE* ppCubeMap, int* piFace )
{
	// get ptr to image
	if ( !UpdatePtrImage ( iID ) )
		return;

	// return cube reference details
	if ( ppCubeMap ) *ppCubeMap = m_imgptr->pCubeMapRef;
	if ( piFace ) *piFace = m_imgptr->iCubeMapFace;
}

DARKSDK void SetMipmapMode ( bool bMode )
{
	// switch mip mapping on or off
	#ifdef DX11
	#else
	if ( !bMode )
	{
		m_pD3D->SetSamplerState ( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
		m_pD3D->SetSamplerState ( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
		m_pD3D->SetSamplerState ( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT );
	}
	else
	{
		m_pD3D->SetSamplerState ( 0, D3DSAMP_MAGFILTER, GGTEXF_LINEAR );
		m_pD3D->SetSamplerState ( 0, D3DSAMP_MINFILTER, GGTEXF_LINEAR );
		m_pD3D->SetSamplerState ( 0, D3DSAMP_MIPFILTER, GGTEXF_LINEAR );
	}
	#endif
}

DARKSDK void SetMipmapType ( int iType )
{
	// set the type of mip mapping used
	// iType possible values
	// 0 - none
	// 1 - point
	// 2 - linear ( default )
	if ( !m_pD3D )
		return;

	#ifdef DX11
	#else
	switch ( iType )
	{
		case 0:
		{
			// use no mip mapping
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );//GGTEXF_NONE ); // may not exist on HW driver any more!
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );//GGTEXF_NONE );
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT );//GGTEXF_NONE );
		}
		break;

		case 1:
		{
			// set up point mip mapping
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT );
		}
		break;

		case 2:
		{
			// set up linear mip mapping
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MAGFILTER, GGTEXF_LINEAR );
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MINFILTER, GGTEXF_LINEAR );
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MIPFILTER, GGTEXF_LINEAR );	
		}
		break;
	}
	#endif
}
	
DARKSDK void SetMipmapBias ( float fBias )
{
	// set the bias for the mip mapping, this allows you
	// to set the distance at which point the mip mapping
	// is brought into action
	#ifdef DX11
	#else
	m_pD3D->SetSamplerState ( 0, D3DSAMP_MIPMAPLODBIAS, *( ( LPDWORD ) ( &fBias ) ) );
	#endif
}

DARKSDK void SetMipmapNum ( int iNum )
{
	// set the number of mip maps used, remember that if
	// you increase this value it will take up a lot more
	// video memory - turn this down if you don't need it
	m_iMipMapNum = iNum;
}

DARKSDK void SetImageTranslucency ( int iID, int iPercent )
{
	// set the translucency for an image
	if ( !UpdatePtrImage ( iID ) )
		return;

	// now lock the surface
	#ifdef DX11
	#else
	HRESULT hr;
	if ( FAILED ( hr = m_imgptr->lpTexture->LockRect ( 0, &m_imgptr->d3dlr, 0, 0 ) ) )
		Error ( "Failed to lock texture in translucency for image library" );
	{
		// get a pointer to surface data
		DWORD* pPix = ( DWORD* ) m_imgptr->d3dlr.pBits;
		DWORD  pGet;

		for ( DWORD y = 0; y < (DWORD)m_imgptr->iHeight; y++ )
		{
			for ( DWORD x = 0; x < (DWORD)m_imgptr->iWidth; x++ )
			{
				pGet = *pPix;

				DWORD dwAlpha = pGet >> 24;
				DWORD dwRed   = ((( pGet ) >> 16 ) & 0xff );
				DWORD dwGreen = ((( pGet ) >>  8 ) & 0xff );
				DWORD dwBlue  = ((( pGet ) )       & 0xff );
				
				if ( *pPix != 0 )
					*pPix++ = GGCOLOR_ARGB ( iPercent, dwRed, dwGreen, dwBlue );
				else
					*pPix++;
			}
		}
	}
	m_imgptr->lpTexture->UnlockRect ( NULL );
	#endif
}

DARKSDK bool ImageExist ( int iID )
{
	// returns true if the image exists
	if ( !UpdatePtrImage ( iID ) )
		return false;

	// return true
	return true;
}

int LoadImageCoreRetainNameTime = 0;

// This load is the MAIN IMAGE LOADER
DARKSDK bool LoadImageCoreRetainName ( char* szRealName, char* szFilename, int iID, int iTextureFlag, bool bIgnoreNegLimit, int iDivideTextureSize )
{
	// iTextureFlag (0-default/1-sectionofplate/2-cube)
	// iDivideTextureSize (0-leave size,0>divide by)
	if ( bIgnoreNegLimit==false )
	{
		if( iID<1 || iID>MAXIMUMVALUE )
		{
			RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER, szFilename);
			return false;
		}
	}

	DARKSDK int Timer(void);
	int ts = MAXTimer();

	//PE: Debug , show list of combination checked.,
	//char mdebug[2048];
	//sprintf(mdebug, "TEST: %s (%s), %d ", szFilename, szRealName, iID);
	//timestampactivity(0, mdebug);

	//PE: Check if same image is already loaded into this ID.
	int			iLen1;				// used for checking length of strings
	LPSTR pNameForInternalList = szFilename;
	if (strlen(szRealName) > 1) pNameForInternalList = szRealName;
	// get length of filename and copy to shortfilename
	iLen1 = lstrlen(pNameForInternalList);											// get length

	//Disabled for now, need a way to force reload.
//	ImagePtr pImage = m_List.find(iID);
//	if (pImage != m_List.end())
//	{
//		if( _strnicmp(pNameForInternalList, pImage->second->szShortFilename, iLen1) == 0 ) {
////			char mdebug[2048];
////			sprintf(mdebug, "CACHE HIT: %s ", pNameForInternalList);
////			timestampactivity(0, mdebug);
//			return true;
//		}
//	}

	
    RemoveImage( iID );

	// loads in image into the bank
	tagImgData*	test = NULL;		// pointer to data structure
	int			iLen2;				// used for checking length of strings
	char		szTest [ 256 ];		// text buffer
	bool		bFound = false;		// flag which checks if we need to load the texture

	// create a new block of memory
	test = new tagImgData;

	// check the memory was created
	if ( test == NULL )
		return false;

	// clear out the memory
	memset(test, 0, sizeof(tagImgData));

	// clear out text buffers
	memset ( test->szLongFilename,  0, sizeof ( char ) * 256 );		// clear out long filename
	memset ( test->szShortFilename, 0, sizeof ( char ) * 256 );		// clear out short filename
	memset ( szTest,                0, sizeof ( char ) * 256 );		// clear out test buffer

	memcpy ( test->szShortFilename, pNameForInternalList, sizeof ( char ) * iLen1 );	// copy to short filename

	// sort out full filename
	GetCurrentDirectory ( 256, szTest );	// get the apps full directory e.g. "c:\test\"
	iLen1 = lstrlen ( szTest );				// get the length of the full directory
	iLen2 = lstrlen ( pNameForInternalList );			// get the length of the passed filename
	
	// copy memory
	memcpy ( test->szLongFilename,         szTest,         sizeof ( char ) * iLen1 );	// copy the full dir to the long filename

	// U74 BETA9 - 030709 - fix longfilename if full dir has no \ and filename does not start with
	// memcpy ( test->szLongFilename + iLen1, szFilename + 1, sizeof ( char ) * iLen2 );	// append the filename onto the long file name
	if ( pNameForInternalList[0]!='\\' )
	{
		if ( szTest[iLen1]!='\\' )
		{
			test->szLongFilename[iLen1]='\\';
			memcpy ( test->szLongFilename + iLen1 + 1, pNameForInternalList, sizeof ( char ) * iLen2 );
		}
		else
			memcpy ( test->szLongFilename + iLen1, pNameForInternalList, sizeof ( char ) * iLen2 );
	}
	else
	{
		// legacy support
		memcpy ( test->szLongFilename + iLen1, pNameForInternalList + 1, sizeof ( char ) * iLen2 );
	}

	// DX12 mode: skip DX11 texture creation, store entry with filenames for lazy DX12 loading
	extern bool ImGui_DX12_IsInitialized();
	if (ImGui_DX12_IsInitialized())
	{
		// Get image dimensions from file header without loading full pixel data
		extern bool ImGui_DX12_GetFileDimensions(const char* filepath, int* outWidth, int* outHeight);
		int imgW = 0, imgH = 0;
		ImGui_DX12_GetFileDimensions(test->szLongFilename, &imgW, &imgH);
		if (imgW == 0 || imgH == 0)
			ImGui_DX12_GetFileDimensions(szFilename, &imgW, &imgH);
		if (imgW == 0 || imgH == 0)
		{
			// File not found or unreadable - return false so caller can try fallbacks
			SAFE_DELETE(test);
			return false;
		}
		test->iWidth = imgW;
		test->iHeight = imgH;
		test->iDepth = 32;
		test->lpTexture = NULL;
		test->lpTextureView = NULL;
		test->fTexUMax = 1.0f;
		test->fTexVMax = 1.0f;
		test->bLocked = false;
		m_List.insert(std::make_pair(iID, test));
		return true;
	}

	// The default is a setting of zero (0)
	GGIMAGE_INFO info;
	if( iTextureFlag==1 )
	{
		// loads image into a section of the texture plate
		if ( m_bSharing && !bFound )
			test->lpTexture = GetTexture ( szFilename, &info, 1 );	// load the perfect texture
		else
			test->lpTexture = GetTexture ( szFilename, &info, 1 );	// load the perfect texture

		// load failed
		if ( test->lpTexture==NULL )
		{
			SAFE_DELETE(test);
			return false;
		}

		// Set image settings
		test->iHeight = m_iHeight;					// store the width
		test->iWidth  = m_iWidth;					// store the height

		// get actual dimensions of texture/image
		#ifdef DX11
		GGSURFACE_DESC imageddsd;
		LPGGSURFACE pTextureInterface = NULL;
		test->lpTexture->QueryInterface<ID3D11Texture2D>(&pTextureInterface);
		pTextureInterface->GetDesc(&imageddsd);
		SAFE_RELEASE ( pTextureInterface );
		test->fTexUMax=(float)test->iWidth/(float)imageddsd.Width;
		test->fTexVMax=(float)test->iHeight/(float)imageddsd.Height;
		#else
		D3DSURFACE_DESC imageddsd;
		test->lpTexture->GetLevelDesc(0, &imageddsd);
		test->fTexUMax=(float)test->iWidth/(float)imageddsd.Width;
		test->fTexVMax=(float)test->iHeight/(float)imageddsd.Height;
		#endif

		// Ensure smalltextres are handled
		if(test->fTexUMax>1.0f) test->fTexUMax=1.0f;
		if(test->fTexVMax>1.0f) test->fTexVMax=1.0f;
	}
	else
	{
		// Load Image Into Whole Texture Plate
		LoadImageCoreFullTex ( szFilename, &test->lpTexture, &info, iTextureFlag, iDivideTextureSize );	// loads into whole texture

		// load failed
		if ( test->lpTexture==NULL )
		{
			SAFE_DELETE(test);
			return false;
		}

		// get file image info
		test->iHeight = info.Height;
		test->iWidth  = info.Width;

		// Entire texture used
		test->fTexUMax=1.0f;
		test->fTexVMax=1.0f;
	}

	// Get depth of texture
	#ifdef DX11

	// for DX11, create resource view from texture and store in lpTextureRef
	CreateShaderResourceViewFor ( test, iTextureFlag, info.Format );

	// Get depth from format
	test->iDepth = ImageGetBitDepthFromFormat(info.Format);

	#else
	D3DSURFACE_DESC desc;
	test->lpTexture->GetLevelDesc(0, &desc);
	test->iDepth  = ImageGetBitDepthFromFormat(desc.Format);
	#endif

	// load failed
	if ( test->lpTexture==NULL )
	{
		SAFE_DELETE(test);
		return false;
	}

	// fill out rest of structure
	test->bLocked = false;

#ifdef PETESTIMAGEUSAGE
	test->AccessCountGPU = 0;
	test->AccessCountCPU = 0;
	test->iImageLoadTime = MAXTimer() - ts;
	LoadImageCoreRetainNameTime += MAXTimer() - ts;
#endif

	// add to the list
    m_List.insert( std::make_pair(iID, test) );

	// ensure sprites all updated
	// V109 BETA3 - only need to update image/textures, not internal textures (negatives)
	if ( iID>0 ) UpdateAllSprites();

	return true;
}

DARKSDK bool LoadImageCore ( char* szFilename, int iID, int iTextureFlag, bool bIgnoreNegLimit, int iDivideTextureSize )
{
	return LoadImageCoreRetainName ( "", szFilename, iID, iTextureFlag, bIgnoreNegLimit, iDivideTextureSize );
}

DARKSDK bool LoadImageCore ( char* szFilename, int iID )
{
	return LoadImageCore ( szFilename, iID, 1, false, 0 );
}

DARKSDK bool SaveImageCoreAsTexSurface(char* szFilename, LPGGSURFACE* pTexSurface, int iCompressionMode)
{
	D3DX11_IMAGE_FILE_FORMAT DestFormat = D3DX11_IFF_BMP;
	LPSTR szFilenameExt = szFilename + strlen(szFilename)-4;
	if ( _strnicmp ( szFilenameExt, ".bmp", 4 )==NULL ) DestFormat = D3DX11_IFF_BMP;
	if ( _strnicmp ( szFilenameExt, ".dds", 4 )==NULL ) DestFormat = D3DX11_IFF_DDS;
	if ( _strnicmp ( szFilenameExt, ".jpg", 4 )==NULL ) DestFormat = D3DX11_IFF_JPG;
	if ( _strnicmp ( szFilenameExt, ".png", 4 )==NULL ) DestFormat = D3DX11_IFF_PNG;

	GGSURFACE_DESC srcddsd;
	if (*pTexSurface)
	{
		// determine size of surface
		HRESULT hRes;
		(*pTexSurface)->GetDesc(&srcddsd);

		// use automatic compression
		GGFORMAT dwD3DSurfaceFormat = DXGIFORMATR8G8B8A8UNORM;
		switch (iCompressionMode)
		{
		case 1: dwD3DSurfaceFormat = GGFMT_DXT1; break;
		case 2: dwD3DSurfaceFormat = GGFMT_DXT2; break;
		case 3: dwD3DSurfaceFormat = GGFMT_DXT3; break;
		case 4: dwD3DSurfaceFormat = GGFMT_DXT4; break;
		case 5: dwD3DSurfaceFormat = GGFMT_DXT5; break;
		}

		// final format to save the image file
		DXGI_FORMAT pFinalSaveFormat = srcddsd.Format;

		// if not DDS saving, and format a BC3/DXT, change to DXGI_FORMAT_R8G8B8A8_UNORM
#ifdef WICKEDENGINE
		// need to select allowable conversions
		//PE: Support saving BC7 as png, ccp had some BC7
		if (DestFormat != D3DX11_IFF_DDS && ((srcddsd.Format >= DXGI_FORMAT_BC1_TYPELESS && srcddsd.Format <= DXGI_FORMAT_BC5_SNORM) || srcddsd.Format == DXGI_FORMAT_BC7_UNORM))
		{
			// filenames to WCHAR
			wchar_t wFilenamePlate[512];
			MultiByteToWideChar(CP_ACP, 0, szFilename, -1, wFilenamePlate, sizeof(wFilenamePlate));

			// create and load the texture selected
			LPGGTEXTURE pPlateSurface = NULL;

			// load compressed texture to a plate
			DirectX::ScratchImage imageTexturePlate;
			HRESULT hr = CaptureTexture(m_pD3D, m_pImmediateContext, *pTexSurface, imageTexturePlate);
			if (SUCCEEDED(hr))
			{
				// decompress the plate
				DirectX::ScratchImage uncompressedTexturePlate;
				hr = Decompress(imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), imageTexturePlate.GetMetadata(),
					DXGI_FORMAT_B8G8R8A8_UNORM, uncompressedTexturePlate);

				// save as an uncompressed file
				const DirectX::Image* img = uncompressedTexturePlate.GetImages();
				hr = SaveToWICFile(*img, DirectX::DDS_FLAGS_NONE, GUID_ContainerFormatPng, wFilenamePlate, NULL);

				// create mipmaps for final image and save
				//DirectX::ScratchImage mipChain;
				//hr = GenerateMipMaps( uncompressedTexturePlate.GetImages(), uncompressedTexturePlate.GetImageCount(), uncompressedTexturePlate.GetMetadata(), DirectX::TEX_FILTER_SEPARATE_ALPHA, 0, mipChain );
				//const DirectX::Image* img = mipChain.GetImages();
				//hr = SaveToWICFile( img, mipChain.GetImageCount(), mipChain.GetMetadata(), DirectX::DDS_FLAGS_NONE, wFilenamePlate );
				if (FAILED(hr))
				{
					char pStrClue[512];
					wsprintf(pStrClue, "Failed to save texture:%s", szFilename);
					RunTimeError(RUNTIMEERROR_IMAGEERROR, pStrClue);
					SAFE_RELEASE(*pTexSurface);
					return false;
				}

				// free surface and leave, completed save with DirectXTex functions (as needed format conversion)
				SAFE_RELEASE(*pTexSurface);
				return true;
			}
		}
		#else
		//if (DestFormat != D3DX11_IFF_DDS) //messed up terrian texture saving!!
		//{
		//	pFinalSaveFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		//}
		#endif

		LPGGSURFACE pSourceDDS = NULL;
		GGSURFACE_DESC StagedDesc = { srcddsd.Width, srcddsd.Height, srcddsd.MipLevels, srcddsd.ArraySize, pFinalSaveFormat, 1, 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0 };
		HRESULT hr = m_pD3D->CreateTexture2D(&StagedDesc, NULL, (ID3D11Texture2D**)&pSourceDDS);
		if (pSourceDDS)
		{
			// first copy texture to a stage ready for reading (BGRA)
			m_pImmediateContext->CopyResource(pSourceDDS, *pTexSurface);

			// Copy Resource cannot convert pixel formats, so do it manually to get (RGBA)
			if ( srcddsd.Format == DXGI_FORMAT_B8G8R8A8_UNORM && dwD3DSurfaceFormat == DXGIFORMATR8G8B8A8UNORM)
			{
				LPGGSURFACE pDestDDS = NULL;
				GGSURFACE_DESC DestDesc = { srcddsd.Width, srcddsd.Height, 1, 1, dwD3DSurfaceFormat, 1, 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE, 0 };
				HRESULT hr = m_pD3D->CreateTexture2D(&DestDesc, NULL, (ID3D11Texture2D**)&pDestDDS);
				if (pDestDDS)
				{
					D3D11_MAPPED_SUBRESOURCE srcMapped;
					HRESULT hr = m_pImmediateContext->Map(pSourceDDS, 0, D3D11_MAP_READ, 0, &srcMapped);
					if (SUCCEEDED(hr))
					{
						D3D11_MAPPED_SUBRESOURCE destMapped;
						//HRESULT hr = m_pImmediateContext->Map( pDestDDS, 0, D3D11_MAP_READ, 0, &destMapped );
						HRESULT hr = m_pImmediateContext->Map(pDestDDS, 0, D3D11_MAP_WRITE, 0, &destMapped);
						if (SUCCEEDED(hr))
						{
							const size_t size = srcddsd.Width*srcddsd.Height * 4;
							unsigned char* pSrc = static_cast<unsigned char*>(srcMapped.pData);
							unsigned char* pDest = static_cast<unsigned char*>(destMapped.pData);
							int offsetSrc = 0;
							int offsetDst = 0;
							int rowOffset = srcMapped.RowPitch % srcddsd.Width;
							//PE: Saveimage did not work on NPO2 images, we need the destMapped.RowPitch.
							int rowDestOffset = destMapped.RowPitch % DestDesc.Width;
							for (int row = 0; row < srcddsd.Height; ++row)
							{
								for (int col = 0; col < srcddsd.Width; ++col)
								{
									pDest[offsetDst] = pSrc[offsetSrc + 2];
									pDest[offsetDst + 1] = pSrc[offsetSrc + 1];
									pDest[offsetDst + 2] = pSrc[offsetSrc];
									if(g_bDontUseImageAlpha)
										pDest[offsetDst + 3] = 255;
									else
										pDest[offsetDst + 3] = pSrc[offsetSrc + 3];
									offsetSrc += 4;
									offsetDst += 4;
								}
								offsetSrc += rowOffset;
								offsetDst += rowDestOffset;
							}
							m_pImmediateContext->Unmap(pDestDDS, 0);
						}
						m_pImmediateContext->Unmap(pSourceDDS, 0);
					}
				}
				SAFE_RELEASE(pSourceDDS);
				SAFE_RELEASE(*pTexSurface);
				*pTexSurface = pDestDDS;
			}
			else
			{
				SAFE_RELEASE(*pTexSurface);
				*pTexSurface = pSourceDDS;
			}
		}

		// save surface of image to file
		try
		{
			#ifdef WICKEDENGINE
			//PE: Test save. to remove DX10. cant convert a cubemap.
			//LB: Reactivated this approach to saving DDS files (no need to save cube maps right now that I recall)
			DirectX::ScratchImage imageTexturePlate;
			HRESULT hr = CaptureTexture(m_pD3D, m_pImmediateContext, *pTexSurface, imageTexturePlate);
			//std::unique_ptr<DirectX::ScratchImage> convertedImage(new (std::nothrow)DirectX::ScratchImage);
			//hr = DirectX::Convert(imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), imageTexturePlate.GetMetadata(), dwD3DSurfaceFormat, DirectX::TEX_FILTER_DITHER, DirectX::TEX_THRESHOLD_DEFAULT, *convertedImage);
			wchar_t wTexSaveFilename[512];
			MultiByteToWideChar(CP_ACP, 0, szFilename, -1, wTexSaveFilename, sizeof(wTexSaveFilename));
			//hRes = DirectX::SaveToDDSFile(convertedImage->GetImages(), convertedImage->GetImageCount(), convertedImage->GetMetadata(), DirectX::DDS_FLAGS_NONE, wTexSaveFilename);
			if (DestFormat == D3DX11_IFF_PNG)
			{
				// save as PNG for the ICON images at least :)
				if (hRes = DirectX::SaveToWICFile(imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), DirectX::DDS_FLAGS_NONE, GUID_ContainerFormatPng, wTexSaveFilename, NULL) != S_OK)
				{
					// fall back is only save one image from what is probably a texture of mipmaps
					hRes = DirectX::SaveToWICFile(imageTexturePlate.GetImages(), 1, DirectX::DDS_FLAGS_NONE, GUID_ContainerFormatPng, wTexSaveFilename, NULL);
				}
			}
			else
			{
				// oops, it seems even JPG is being saved as DDS (which is why the thumbbank JPGs cannot be opened in PSP)
				hRes = DirectX::SaveToDDSFile(imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), imageTexturePlate.GetMetadata(), DirectX::DDS_FLAGS_NONE, wTexSaveFilename);
			}
			#else
			hRes = D3DX11SaveTextureToFile(m_pImmediateContext, *pTexSurface, DestFormat, szFilename);
			if (FAILED(hRes))
			{
				char pStrClue[512];
				wsprintf(pStrClue, "tex:%d filename:%s", (int)*pTexSurface, szFilename);
				RunTimeError(RUNTIMEERROR_IMAGEERROR, pStrClue);
				SAFE_RELEASE(*pTexSurface);
				return false;
			}
			#endif
		}
		catch (...)
		{
			// this can fail when file is locked, so try waiting 3 seconds, then try again, else silent fail so no crash!
			Sleep(3000);
			hRes = D3DX11SaveTextureToFile(m_pImmediateContext, *pTexSurface, DestFormat, szFilename);
			if (FAILED(hRes))
			{
				// so joy, silent fail!
				SAFE_RELEASE(*pTexSurface);
				return false;
			}
		}
	}
	return true;
}

DARKSDK bool SaveImageCore ( char* szFilename, int iID, int iCompressionMode )
{
	if(iID<1 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
		return false;
	}
	if ( !UpdatePtrImage ( iID ) )
	{
		#ifdef WICKEDENGINE
		//Silent error now log instead. So we know exactly where the error is.
		if (iMaxPasteImageLogs > 0)
		{
			char pErr[512]; sprintf(pErr, "Error Image Not Found: SaveImageCore(%s,%d,%d) .", szFilename , iID, iCompressionMode );
			timestampactivity(0, pErr);
			iMaxPasteImageLogs--;
		}
		#else
		RunTimeError(RUNTIMEERROR_IMAGENOTEXIST);
		#endif
		return false;
	}
	if ( szFilename==NULL )
	{
		RunTimeError(RUNTIMEERROR_FILENOTEXIST);
		return false;
	}

	// determine format from extension
	#ifdef DX11

	// determine region of surface to save
	LPGGSURFACE pTexSurface = NULL;
	m_imgptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTexSurface);
	SaveImageCoreAsTexSurface(szFilename, &pTexSurface, iCompressionMode);
	/*
	if ( pTexSurface )
	{
		// determine size of surface
		HRESULT hRes;
		pTexSurface->GetDesc(&srcddsd);

		// use automatic compression
		GGFORMAT dwD3DSurfaceFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		switch ( iCompressionMode )
		{
			case 1 : dwD3DSurfaceFormat = GGFMT_DXT1; break;
			case 2 : dwD3DSurfaceFormat = GGFMT_DXT2; break;
			case 3 : dwD3DSurfaceFormat = GGFMT_DXT3; break;
			case 4 : dwD3DSurfaceFormat = GGFMT_DXT4; break;
			case 5 : dwD3DSurfaceFormat = GGFMT_DXT5; break;
		}

		// final format to save the image file
		DXGI_FORMAT pFinalSaveFormat = srcddsd.Format;

		// if not DDS saving, and format a BC3/DXT, change to DXGI_FORMAT_R8G8B8A8_UNORM
		#ifdef WICKEDENGINE
		// need to select allowable conversions
		if ( DestFormat != D3DX11_IFF_DDS && srcddsd.Format >= DXGI_FORMAT_BC1_TYPELESS && srcddsd.Format <= DXGI_FORMAT_BC5_SNORM )
		{
			// filenames to WCHAR
			wchar_t wFilenamePlate[512];
			MultiByteToWideChar(CP_ACP, 0, szFilename, -1, wFilenamePlate, sizeof(wFilenamePlate));

			// create and load the texture selected
			LPGGTEXTURE pPlateSurface = NULL;

			// load compressed texture to a plate
			DirectX::ScratchImage imageTexturePlate;
			HRESULT hr = CaptureTexture( m_pD3D, m_pImmediateContext, pTexSurface, imageTexturePlate );
			if (SUCCEEDED(hr))
			{
				// decompress the plate
				DirectX::ScratchImage uncompressedTexturePlate;
				hr = Decompress(imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), imageTexturePlate.GetMetadata(),
					DXGI_FORMAT_B8G8R8A8_UNORM, uncompressedTexturePlate);

				// save as an uncompressed file
				const DirectX::Image* img = uncompressedTexturePlate.GetImages();
				hr = SaveToWICFile ( *img, DirectX::DDS_FLAGS_NONE, GUID_ContainerFormatPng, wFilenamePlate, NULL );

				// create mipmaps for final image and save
				//DirectX::ScratchImage mipChain;
				//hr = GenerateMipMaps( uncompressedTexturePlate.GetImages(), uncompressedTexturePlate.GetImageCount(), uncompressedTexturePlate.GetMetadata(), DirectX::TEX_FILTER_SEPARATE_ALPHA, 0, mipChain );
				//const DirectX::Image* img = mipChain.GetImages();
				//hr = SaveToWICFile( img, mipChain.GetImageCount(), mipChain.GetMetadata(), DirectX::DDS_FLAGS_NONE, wFilenamePlate );
				if ( FAILED ( hr ) )
				{
					char pStrClue[512];
					wsprintf ( pStrClue, "Failed to save texture:%s", szFilename );
					RunTimeError(RUNTIMEERROR_IMAGEERROR,pStrClue);
					SAFE_RELEASE(pTexSurface);
					return false;
				}

				// free surface and leave, completed save with DirectXTex functions (as needed format conversion)
				SAFE_RELEASE(pTexSurface);
				return true;
			}
		}
		#else
		//if (DestFormat != D3DX11_IFF_DDS) //messed up terrian texture saving!!
		//{
		//	pFinalSaveFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		//}
		#endif

		LPGGSURFACE pSourceDDS = NULL;
		GGSURFACE_DESC StagedDesc = { srcddsd.Width, srcddsd.Height, srcddsd.MipLevels, srcddsd.ArraySize, pFinalSaveFormat, 1, 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0 };
		HRESULT hr = m_pD3D->CreateTexture2D( &StagedDesc, NULL, (ID3D11Texture2D**)&pSourceDDS );
		if ( pSourceDDS )
		{
			// first copy texture to a stage ready for reading (BGRA)
			m_pImmediateContext->CopyResource ( pSourceDDS, pTexSurface );

			// Copy Resource cannot convert pixel formats, so do it manually to get (RGBA)
			if (srcddsd.Format == DXGI_FORMAT_B8G8R8A8_UNORM && dwD3DSurfaceFormat == DXGI_FORMAT_R8G8B8A8_UNORM)
			{
				LPGGSURFACE pDestDDS = NULL;
				GGSURFACE_DESC DestDesc = { srcddsd.Width, srcddsd.Height, 1, 1, dwD3DSurfaceFormat, 1, 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE, 0 };
				HRESULT hr = m_pD3D->CreateTexture2D(&DestDesc, NULL, (ID3D11Texture2D**)&pDestDDS);
				if (pDestDDS)
				{
					D3D11_MAPPED_SUBRESOURCE srcMapped;
					HRESULT hr = m_pImmediateContext->Map(pSourceDDS, 0, D3D11_MAP_READ, 0, &srcMapped);
					if (SUCCEEDED(hr))
					{
						D3D11_MAPPED_SUBRESOURCE destMapped;
						//HRESULT hr = m_pImmediateContext->Map( pDestDDS, 0, D3D11_MAP_READ, 0, &destMapped );
						HRESULT hr = m_pImmediateContext->Map(pDestDDS, 0, D3D11_MAP_WRITE, 0, &destMapped);
						if (SUCCEEDED(hr))
						{
							const size_t size = srcddsd.Width*srcddsd.Height * 4;
							unsigned char* pSrc = static_cast<unsigned char*>(srcMapped.pData);
							unsigned char* pDest = static_cast<unsigned char*>(destMapped.pData);
							int offsetSrc = 0;
							int offsetDst = 0;
							int rowOffset = srcMapped.RowPitch % srcddsd.Width;
							for (int row = 0; row < srcddsd.Height; ++row)
							{
								for (int col = 0; col < srcddsd.Width; ++col)
								{
									pDest[offsetDst] = pSrc[offsetSrc + 2];
									pDest[offsetDst + 1] = pSrc[offsetSrc + 1];
									pDest[offsetDst + 2] = pSrc[offsetSrc];
									pDest[offsetDst + 3] = pSrc[offsetSrc + 3];
									offsetSrc += 4;
									offsetDst += 4;
								}
								offsetSrc += rowOffset;
							}
							m_pImmediateContext->Unmap(pDestDDS, 0);
						}
						m_pImmediateContext->Unmap(pSourceDDS, 0);
					}
				}
				SAFE_RELEASE(pSourceDDS);
				SAFE_RELEASE(pTexSurface);
				pTexSurface = pDestDDS;
			}
			else
			{
				SAFE_RELEASE(pTexSurface);
				pTexSurface = pSourceDDS;
			}
		}

		// save surface of image to file
		try
		{
			hRes = D3DX11SaveTextureToFile( m_pImmediateContext, pTexSurface, DestFormat, szFilename );
			if ( FAILED ( hRes ) )
			{
				char pStrClue[512];
				wsprintf ( pStrClue, "tex:%d filename:%s", (int)pTexSurface, szFilename );
				RunTimeError(RUNTIMEERROR_IMAGEERROR,pStrClue);
				SAFE_RELEASE(pTexSurface);
				return false;
			}
		}
		catch (...)
		{
			// this can fail when file is locked, so try waiting 3 seconds, then try again, else silent fail so no crash!
			Sleep(3000);
			hRes = D3DX11SaveTextureToFile( m_pImmediateContext, pTexSurface, DestFormat, szFilename );
			if ( FAILED ( hRes ) )
			{
				// so joy, silent fail!
				SAFE_RELEASE(pTexSurface);
				return false;
			}
		}
	}
	*/

	// free surface
	SAFE_RELEASE(pTexSurface);
	#else
	D3DXIMAGE_FILEFORMAT DestFormat = D3DXIFF_BMP;
	LPSTR szFilenameExt = szFilename + strlen(szFilename)-4;
	if ( _strnicmp ( szFilenameExt, ".bmp", 4 )==NULL ) DestFormat=D3DXIFF_BMP ;
	if ( _strnicmp ( szFilenameExt, ".dds", 4 )==NULL ) DestFormat=D3DXIFF_DDS ;
	if ( _strnicmp ( szFilenameExt, ".dib", 4 )==NULL ) DestFormat=D3DXIFF_DIB ;
	if ( _strnicmp ( szFilenameExt, ".jpg", 4 )==NULL ) DestFormat=D3DXIFF_JPG ;
	if ( _strnicmp ( szFilenameExt, ".png", 4 )==NULL ) DestFormat=D3DXIFF_PNG ;

	// determine region of surface to save
	LPGGSURFACE pTexSurface = NULL;
	m_imgptr->lpTexture->GetSurfaceLevel(0, &pTexSurface);
	if ( pTexSurface )
	{
		// determine size of surface
		D3DSURFACE_DESC srcddsd;
		HRESULT hRes = pTexSurface->GetDesc(&srcddsd);
		RECT rc = { 0, 0, (int)((float)srcddsd.Width*m_imgptr->fTexUMax), (int)((float)srcddsd.Height*m_imgptr->fTexVMax) };

		// If DDS, use automatic compression
		LPGGTEXTURE pTextureDDS = NULL;
		LPGGSURFACE pDDSSurface = NULL;
		if ( DestFormat==D3DXIFF_DDS )
		{
			// and only if source NOT already compressed
			if ( srcddsd.Format==GGFMT_DXT1 || srcddsd.Format==GGFMT_DXT2 || srcddsd.Format==GGFMT_DXT3 || srcddsd.Format==GGFMT_DXT4 || srcddsd.Format==GGFMT_DXT5 )
			{
				// source already compressed
			}
			else
			{
				// create our DDS MASTER texture (the final one to be used)
				GGFORMAT dwD3DSurfaceFormat = GGFMT_A8R8G8B8;
				switch ( iCompressionMode )
				{
					case 1 : dwD3DSurfaceFormat = GGFMT_DXT1; break;
					case 2 : dwD3DSurfaceFormat = GGFMT_DXT2; break;
					case 3 : dwD3DSurfaceFormat = GGFMT_DXT3; break;
					case 4 : dwD3DSurfaceFormat = GGFMT_DXT4; break;
					case 5 : dwD3DSurfaceFormat = GGFMT_DXT5; break;
				}
				m_pD3D->CreateTexture ( srcddsd.Width, srcddsd.Height, 1, 0, dwD3DSurfaceFormat, D3DPOOL_MANAGED, &pTextureDDS, NULL );
				if ( pTextureDDS )
				{
					// copy texture to DDS compressed texture
					pTextureDDS->GetSurfaceLevel ( 0, &pDDSSurface );
					if ( pDDSSurface )
					{
						D3DSURFACE_DESC dstddsd;
						HRESULT hRes = pDDSSurface->GetDesc(&dstddsd);
						if ( pTexSurface )
						{
							hRes = D3DXLoadSurfaceFromSurface ( pDDSSurface, NULL, NULL, pTexSurface, NULL, NULL, D3DX_FILTER_NONE, 0 );
							if ( FAILED ( hRes ) )
							{
								char pStrClue[512];
								wsprintf ( pStrClue, "D3DXLoadSurfaceFromSurface failure" );
								RunTimeError(RUNTIMEERROR_IMAGEERROR,pStrClue);
								SAFE_RELEASE(pDDSSurface);
								SAFE_RELEASE(pTextureDDS);
								SAFE_RELEASE(pTexSurface);
								return false;
							}
						}
					}

					// Copy over DDS to regular surface
					SAFE_RELEASE ( pTexSurface );
					pTexSurface = pDDSSurface;
					pDDSSurface = NULL;
				}
			}
		}

		// save surface of image to file
		hRes = D3DXSaveSurfaceToFile( szFilename, DestFormat, pTexSurface, NULL, &rc );
		if ( FAILED ( hRes ) )
		{
			char pStrClue[512];
			wsprintf ( pStrClue, "tex:%d filename:%s region:%d %d %d %d", (int)pTexSurface, szFilename, rc.left, rc.top, rc.right, rc.bottom );
			RunTimeError(RUNTIMEERROR_IMAGEERROR,pStrClue);
			SAFE_RELEASE(pTexSurface);
			return false;
		}

		SAFE_RELEASE(pDDSSurface);
		SAFE_RELEASE(pTextureDDS);
	}

	// free surface
	SAFE_RELEASE(pTexSurface);
	#endif

	// success
	return true;
}

DARKSDK bool SaveImageCore ( char* szFilename, int iID )
{
	// default behaviour
	return SaveImageCore ( szFilename, iID, 0 );
}

DARKSDK void CreateReplaceImage ( int iID, int iTexSize, ID3D11Texture2D* pTex, ID3D11ShaderResourceView* pView )
{
	// establish if image exists or not
	if( iID < 1 || iID > MAXIMUMVALUE )
		return;

	if ( !UpdatePtrImage ( iID ) )
	{
		// create a new image
		MakeFormat ( iID, iTexSize, iTexSize, DXGIFORMATR8G8B8A8UNORM, 0 );
		UpdatePtrImage ( iID );
	}

	// valid image can be be overridden with new image details
	m_imgptr->lpTexture = pTex;
	m_imgptr->lpTextureView = pView;
	m_imgptr->iWidth = iTexSize;
	m_imgptr->iHeight = iTexSize;
	SAFE_DELETE ( m_imgptr->lpName );
	strcpy ( m_imgptr->szLongFilename, "" );
	strcpy ( m_imgptr->szShortFilename, "" );
	m_imgptr->fTexUMax = 1.0f;
	m_imgptr->fTexVMax = 1.0f;
}
#ifdef WICKEDENGINE
//PE: GameGuru IMGUI.
#include "..\..\..\..\GameGuru\Imgui\imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "..\..\..\..\GameGuru\Imgui\imgui_internal.h"
#include "..\..\..\..\GameGuru\Imgui\imgui_impl_win32.h"
#include "..\..\..\..\GameGuru\Imgui\imgui_gg_dx11.h"
#include ".\..\..\Guru-WickedMAX\wickedcalls.h"

int g_iSpecialGrabImageMode = 0;

DARKSDK void SetGrabImageMode(int iMode)
{
	// set this mode to 1 when you want to use DirectXTex method to convert and grab from the wicked backbuffer
	g_iSpecialGrabImageMode = iMode;
}

DARKSDK bool GrabImageCore(int iID, int iX1, int iY1, int iX2, int iY2, int iTextureFlagForGrab)
{
	//PE: Perhaps wait for GPU to finish its threads ?

	if (iID<1 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
		return false;
	}
	if (iX1 >= iX2 || iY1 >= iY2)
	{
		RunTimeError(RUNTIMEERROR_IMAGEAREAILLEGAL);
		return false;
	}

	//Finish any imgui stuff.
	extern LPGGDEVICE				m_pD3D;
	extern LPGGIMMEDIATECONTEXT		m_pImmediateContext;
	extern bool bRenderTabTab;
	extern bool bRenderNextFrame;
	if (bRenderTabTab) {
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui::Render();
		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
		bRenderTabTab = false;
		bRenderNextFrame = true;
		
		//DARKSDK void SetCurrentBitmap(int iID);
		//SetCurrentBitmap(2);
		//CLS(Rgb(64, 64, 64));

		//PE: Draw now.
		WickedCall_DrawImguiNow();
	}

	if (!g_pGlob->pCurrentBitmapSurface)
	{
		return false;
	}
	if (!m_pD3D)
		return false;

	// Size of grab
	GGFORMAT backFormat;
	GGSURFACE_DESC ddsd;
	LPGGSURFACE pBackBuffer = g_pGlob->pCurrentBitmapSurface;
	if (pBackBuffer)
	{
		// get format of backbuffer
		pBackBuffer->GetDesc(&ddsd);
		backFormat = ddsd.Format;
		if (iX2 > (int)ddsd.Width || iY2 > (int)ddsd.Height)
		{
			iX2 = ddsd.Width;
			iY2 = ddsd.Height;
			if (iX1 >= iX2 || iY1 >= iY2) return false;
			//RunTimeError(RUNTIMEERROR_IMAGEAREAILLEGAL);
			//return false;
		}
	}

	// Image size
	int iImageWidth = iX2 - iX1;
	int iImageHeight = iY2 - iY1;

	// Get current render target surface
	if (pBackBuffer)
	{
		// check if image already exists of same size and type
		if (UpdatePtrImage(iID))
		{
			// check against existing
			if (m_imgptr->iWidth != iImageWidth || m_imgptr->iHeight != iImageHeight)
			{
				// existing image and new image are different sizes - so delete any existing image
				RemoveImage(iID);
			}
		}

		// if image format not internal texture format, delete so can be recreated
		if (m_imgptr)
		{
			GGSURFACE_DESC imgddsd;
			LPGGSURFACE pTextureInterface = NULL;
			m_imgptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTextureInterface);
			pTextureInterface->GetDesc(&imgddsd);
			SAFE_RELEASE(pTextureInterface);
			if (imgddsd.Format != g_DefaultGGFORMAT)
			{
				RemoveImage(iID);
			}
		}

		// determine grabimage mode
		if ( g_iSpecialGrabImageMode == 1 )
		{
			// special case for creating thumbnails, grabbed from wicked backbuffer
			// if backbuffer is strange format, need to convert it to compatible one
			//PE: Wicked backbuffer is DXGI_FORMAT_R10G10B10A2_UNORM . GG use DXGI_FORMAT_B8G8R8A8_UNORM.
			LPGGTEXTURE pNewCroppedTexture = NULL;
			LPGGTEXTURE pNewCorrectBBTexture = NULL;
			// then crop it to required new texture
			if (backFormat == g_DefaultGGFORMAT)
			{
				//PE: Backbuffer already in correct format. no need to convert. way faster.
				D3D11_BOX rc = { iX1, iY1, 0, (LONG)(iX1 + iImageWidth), (LONG)(iY1 + iImageHeight), 1 };
				pNewCroppedTexture = CreateCroppedTexture(pBackBuffer, rc);
			}
			else if (iX1 == 0 && iY1 == 0 && ddsd.Width == iX2 && ddsd.Height == iY2)
			{
				//PE: No need to crop.
				pNewCroppedTexture = ConvertBackBufferToNewFormat(pBackBuffer, g_DefaultGGFORMAT);;
			}
			else
			{
				pNewCorrectBBTexture = ConvertBackBufferToNewFormat(pBackBuffer, g_DefaultGGFORMAT);
				if (pNewCorrectBBTexture)
				{
					D3D11_BOX rc = { iX1, iY1, 0, (LONG)(iX1 + iImageWidth), (LONG)(iY1 + iImageHeight), 1 };
					pNewCroppedTexture = CreateCroppedTexture(pNewCorrectBBTexture, rc);
					// free corrected backbuffer we created to do the copy
					SAFE_RELEASE(pNewCorrectBBTexture);
				}

			}
			if (!pNewCroppedTexture)
			{
				return(false);
			}
			// create image to store cropped texture
			if (m_imgptr == NULL)
			{
				MakeFormat(iID, iImageWidth, iImageHeight, g_DefaultGGFORMAT, 0);
			}
			if (UpdatePtrImage(iID))
			{
				
				// put cropped texture in image
				SAFE_RELEASE(m_imgptr->lpTexture);
				m_imgptr->lpTexture = pNewCroppedTexture;

				if(m_imgptr->lpTextureView) //PE: Fix memory leak.
					SAFE_RELEASE(m_imgptr->lpTextureView);

				m_imgptr->lpTextureView = NULL;
				CreateShaderResourceViewFor(m_imgptr, 0, g_DefaultGGFORMAT);
				if (!m_imgptr->lpTextureView || !m_imgptr->lpTexture)
				{
					RemoveImage(iID);
					return false;
				}
				
				// get desc of image
				GGSURFACE_DESC srcddsd;
				LPGGSURFACE pTextureInterface = NULL;
				m_imgptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTextureInterface);
				pTextureInterface->GetDesc(&srcddsd);
				SAFE_RELEASE(pTextureInterface);

				// get actual dimensions of texture/image
				m_imgptr->fTexUMax = (float)m_imgptr->iWidth / (float)srcddsd.Width;
				m_imgptr->fTexVMax = (float)m_imgptr->iHeight / (float)srcddsd.Height;

				// Ensure smalltextres are handled
				if (m_imgptr->fTexUMax > 1.0f) m_imgptr->fTexUMax = 1.0f;
				if (m_imgptr->fTexVMax > 1.0f) m_imgptr->fTexVMax = 1.0f;
				
			}
			else
			{
				RunTimeError(RUNTIMEERROR_IMAGEERROR);
				return false;
			}

			// free corrected backbuffer we created to do the copy
			//SAFE_RELEASE(pNewCorrectBBTexture);
		}
		else
		{
			// normal grabimage, grabbing from old graphics engine surface (prompt3d text creation)
			// create temp image texture to copy backbuffer to
			LPGGSURFACE pTempTexture = NULL;
			GGSURFACE_DESC TempTextureDesc = { iImageWidth, iImageHeight, 1, 1, backFormat, 1, 0, D3D11_USAGE_DEFAULT, 0, 0, 0 };
			m_pD3D->CreateTexture2D(&TempTextureDesc, NULL, &pTempTexture);
			if (pTempTexture)
			{
				// copy backbuffer to temp texture
				D3D11_BOX rc = { iX1, iY1, 0, (LONG)(iX1 + iImageWidth), (LONG)(iY1 + iImageHeight), 1 };
				m_pImmediateContext->CopySubresourceRegion(pTempTexture, 0, 0, 0, 0, pBackBuffer, 0, &rc);

				// create image
				if (m_imgptr == NULL) MakeFormat(iID, iImageWidth, iImageHeight, g_DefaultGGFORMAT, 0);
				if (UpdatePtrImage(iID))
				{
					// get desc of destination texture
					GGSURFACE_DESC srcddsd;
					LPGGSURFACE pTextureInterface = NULL;
					m_imgptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTextureInterface);
					pTextureInterface->GetDesc(&srcddsd);
					SAFE_RELEASE(pTextureInterface);

					// load grabbed surface data into destination texture
					D3D11_BOX rc = { 0, 0, 0, (LONG)(iImageWidth), (LONG)(iImageHeight), 1 };
					m_pImmediateContext->CopySubresourceRegion(m_imgptr->lpTexture, 0, 0, 0, 0, pTempTexture, 0, &rc);

					// get actual dimensions of texture/image
					m_imgptr->fTexUMax = (float)m_imgptr->iWidth / (float)srcddsd.Width;
					m_imgptr->fTexVMax = (float)m_imgptr->iHeight / (float)srcddsd.Height;

					// Ensure smalltextres are handled
					if (m_imgptr->fTexUMax > 1.0f) m_imgptr->fTexUMax = 1.0f;
					if (m_imgptr->fTexVMax > 1.0f) m_imgptr->fTexVMax = 1.0f;
				}
				else
				{
					RunTimeError(RUNTIMEERROR_IMAGEERROR);
					return false;
				}

				// free work-newsurface
				SAFE_RELEASE(pTempTexture);
			}
		}
	}
	else
	{
		RunTimeError(RUNTIMEERROR_IMAGEERROR);
		return false;
	}
	return true;
}

#endif // WICKEDENGINE - continued in part2
