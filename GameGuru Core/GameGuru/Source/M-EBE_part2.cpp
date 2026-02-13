int ebe_loadcustomtexture ( int iEntityProfileIndex, int iWhichTextureOver )
{
	// Needed locals
	cstr pOldDir = GetDir();
	HWND hThisWnd = GetForegroundWindow();

	// must have textures image before we can customise it
	if ( ebebuild.iTexPlateImage == 0 )
		return 0;

	// EBE Load Folder
	t.strwork = g.fpscrootdir_s + "\\Files\\texturebank";
	if ( PathExist( t.strwork.Get() ) == 0 ) 
	{
		MessageBoxA ( hThisWnd, "Cannot find textures folder' folder", "Error", MB_OK | MB_TOPMOST );
		return 0;
	}

	//  Ask for save filename
	cStr tLoadFile = "";
	cStr tLoadMessage = "Replace with custom texture";
	tLoadFile = openFileBox("Diffuse File (_D.dds)|*.dds|Texture File (.dds)|*.dds|All Files|*.*|", t.strwork.Get(), tLoadMessage.Get(), ".dds", IMPORTERSAVEFILE);
	if ( tLoadFile == "Error" )
	{
		SetDir(pOldDir.Get());
		return 0;
	}

	// Use large prompt
	//t.statusbar_s = "Generating Building Editor Textures"; 
	//popup_text(t.statusbar_s.Get());

	// preferred format
	GGFORMAT d3dFormat;
	GGFORMAT compressedFormat;
	#ifdef DX11
	#else
	D3DSURFACE_DESC backbufferdesc;
	g_pGlob->pHoldBackBufferPtr->GetDesc ( &backbufferdesc );
	d3dFormat = backbufferdesc.Format;
	compressedFormat = D3DFMT_DXT1;
	#endif

	// work out if _D.dds or not
	int iTexSetsPossible = 1;
	LPSTR pLoadFilename = tLoadFile.Get();
	cstr pFileExt = Right ( pLoadFilename, 10 );
	if ( stricmp ( pFileExt.Get(), "_color.dds" ) == NULL )
		iTexSetsPossible = 3;

	// go through texture subsets (D, N, S )
	for ( int iTexSet = 0; iTexSet < iTexSetsPossible; iTexSet++ )
	{
		// work out texture filename
		cstr pFilename = pLoadFilename;
		cstr pFileExt = cstr("_color.dds");
		if ( iTexSetsPossible == 3 )
		{
			pFilename = Left ( pLoadFilename, strlen(pLoadFilename)-10 );
			if ( iTexSet == 0 ) pFileExt = cstr("_color.dds");
			if ( iTexSet == 1 ) pFileExt = cstr("_normal.dds");
			if ( iTexSet == 2 ) pFileExt = cstr("_surface.dds");
			pFilename = pFilename + pFileExt;
		}


		// final destination of Textures subsets
		cstr sSavePathFileNonAbs = "ebebank\\default\\textures";
		cstr sSavePathFile = g.fpscrootdir_s + "\\Files\\" + sSavePathFileNonAbs;

		// check if texture to load exists
		#ifdef DX11
		// use terrain custom texture maker for inserting texture in textureplate
		cstr pPlateFilename = cstr(sSavePathFile) + pFileExt;
		ebe_createnewstructuretexture ( pPlateFilename.Get(), iWhichTextureOver, pFilename.Get(), 1, 1 );

		// reload image with new file (apply auto-mipmapping when load)
		if ( iTexSet == 0 ) 
		{
			// texture used by textures
			// textures updated (further below) once texture creation finished
		}
		#else
		GGIMAGE_INFO finfo;
		LPDIRECT3DSURFACE9 pLoadedTexSurface = NULL;
		HRESULT hRes = D3DXGetImageInfoFromFile( pFilename.Get(), &finfo );
		if ( hRes == S_OK )
		{
			// file exists, use the provided _N or _S texture file
		}
		else
		{
			// file not exist, if N or S, substitute with blank
			if ( iTexSet == 1 ) pFilename = g.fpscrootdir_s + "\\Files\\effectbank\\reloaded\\media\\blank_N.dds";
			if ( iTexSet == 2 ) pFilename = g.fpscrootdir_s + "\\Files\\effectbank\\reloaded\\media\\blank_black.dds";
			hRes = D3DXGetImageInfoFromFile( pFilename.Get(), &finfo );
		}

		// create and load the texture selected
		if ( hRes == S_OK )
		{
			hRes = m_pD3D->CreateRenderTarget( finfo.Width, finfo.Height, d3dFormat, D3DMULTISAMPLE_NONE, 0, TRUE, &pLoadedTexSurface, NULL);
			hRes = D3DXLoadSurfaceFromFile( pLoadedTexSurface, NULL, NULL, pFilename.Get(), NULL, D3DX_FILTER_POINT, 0, &finfo );

			// create and load the texture plate surface
			LPDIRECT3DTEXTURE9 pTextureDDS;
			LPDIRECT3DSURFACE9 pPlateSurface = NULL;
			cstr pPlateFilename = cstr(sSavePathFile) + pFileExt;
			hRes = D3DXGetImageInfoFromFile( pPlateFilename.Get(), &finfo );
			m_pD3D->CreateTexture ( finfo.Width, finfo.Height, 1, 0, d3dFormat, D3DPOOL_MANAGED, &pTextureDDS, NULL );
			if ( pTextureDDS )
			{
				// copy texture to DDS compressed texture
				pTextureDDS->GetSurfaceLevel ( 0, &pPlateSurface );
				if ( pPlateSurface )
				{
					hRes = D3DXLoadSurfaceFromFile( pPlateSurface, NULL, NULL, pPlateFilename.Get(), NULL, D3DX_FILTER_POINT, 0, &finfo );
				}
			}

			// get surface of current texture plate
			if ( pLoadedTexSurface && pPlateSurface ) 
			{
				// work out exact offset to slot position
				int iRow = iWhichTextureOver / 4;
				int iCol = iWhichTextureOver - (iRow*4);
				int iTexSlotOffsetX = iCol * 1024;
				int iTexSlotOffsetY = iRow * 1024;
				RECT rcPlate = RECT();
				rcPlate.left = iTexSlotOffsetX; rcPlate.top = iTexSlotOffsetY; rcPlate.right = iTexSlotOffsetX+1024; rcPlate.bottom = iTexSlotOffsetY+1024;

				// paste to fill 1024x1024 initially (to get at corners)
				hRes = D3DXLoadSurfaceFromSurface(pPlateSurface, NULL, &rcPlate, pLoadedTexSurface, NULL, NULL, D3DX_DEFAULT, 0);

				// paste surface so smaller 1022x1022 texture can seamlessly wrap
				int iX = 1, iY = 0;
				rcPlate.left = iTexSlotOffsetX+iX; rcPlate.top = iTexSlotOffsetY+iY; rcPlate.right = iTexSlotOffsetX+iX+1022; rcPlate.bottom = iTexSlotOffsetY+iY+1022;
				hRes = D3DXLoadSurfaceFromSurface(pPlateSurface, NULL, &rcPlate, pLoadedTexSurface, NULL, NULL, D3DX_DEFAULT, 0);
				iX = 1, iY = 2;
				rcPlate.left = iTexSlotOffsetX+iX; rcPlate.top = iTexSlotOffsetY+iY; rcPlate.right = iTexSlotOffsetX+iX+1022; rcPlate.bottom = iTexSlotOffsetY+iY+1022;
				hRes = D3DXLoadSurfaceFromSurface(pPlateSurface, NULL, &rcPlate, pLoadedTexSurface, NULL, NULL, D3DX_DEFAULT, 0);
				iX = 0, iY = 1;
				rcPlate.left = iTexSlotOffsetX+iX; rcPlate.top = iTexSlotOffsetY+iY; rcPlate.right = iTexSlotOffsetX+iX+1022; rcPlate.bottom = iTexSlotOffsetY+iY+1022;
				hRes = D3DXLoadSurfaceFromSurface(pPlateSurface, NULL, &rcPlate, pLoadedTexSurface, NULL, NULL, D3DX_DEFAULT, 0);
				iX = 2, iY = 1;
				rcPlate.left = iTexSlotOffsetX+iX; rcPlate.top = iTexSlotOffsetY+iY; rcPlate.right = iTexSlotOffsetX+iX+1022; rcPlate.bottom = iTexSlotOffsetY+iY+1022;
				hRes = D3DXLoadSurfaceFromSurface(pPlateSurface, NULL, &rcPlate, pLoadedTexSurface, NULL, NULL, D3DX_DEFAULT, 0);
	
				// then paste into surface at 1022x1022 (so can have seamless textures within atlas)
				rcPlate.left = iTexSlotOffsetX+1; rcPlate.top = iTexSlotOffsetY+1;
				rcPlate.right = iTexSlotOffsetX+1023; rcPlate.bottom = iTexSlotOffsetY+1023;
				hRes = D3DXLoadSurfaceFromSurface(pPlateSurface, NULL, &rcPlate, pLoadedTexSurface, NULL, NULL, D3DX_DEFAULT, 0);

				// and finally copy back to LoadTexture
				SAFE_RELEASE ( pLoadedTexSurface );
				hRes = m_pD3D->CreateRenderTarget( finfo.Width, finfo.Height, d3dFormat, D3DMULTISAMPLE_NONE, 0, TRUE, &pLoadedTexSurface, NULL);
				hRes = D3DXLoadSurfaceFromSurface(pLoadedTexSurface, NULL, NULL, pPlateSurface, NULL, NULL, D3DX_DEFAULT, 0);

				// now create the compressed surface for the save
				SAFE_RELEASE ( pPlateSurface );
				SAFE_RELEASE ( pTextureDDS );
				D3DFORMAT d3dChoice = compressedFormat; if ( iTexSet == 1 ) d3dChoice = d3dFormat;
				m_pD3D->CreateTexture ( finfo.Width, finfo.Height, 0, D3DUSAGE_AUTOGENMIPMAP, d3dChoice, D3DPOOL_MANAGED, &pTextureDDS, NULL );
				if ( pTextureDDS )
				{
					// copy texture to DDS compressed texture
					pTextureDDS->GetSurfaceLevel ( 0, &pPlateSurface );
					if ( pPlateSurface )
					{
						hRes = D3DXLoadSurfaceFromSurface(pPlateSurface, NULL, NULL, pLoadedTexSurface, NULL, NULL, D3DX_DEFAULT, 0);
					}
					pTextureDDS->GenerateMipSubLevels();
				}

				// save new texture surface out
				D3DXIMAGE_FILEFORMAT DestFormat = D3DXIFF_DDS;
				cstr pSaveLocation = cstr(sSavePathFile) + pFileExt;
				hRes = D3DXSaveSurfaceToFile( pSaveLocation.Get(), DestFormat, pPlateSurface, NULL, NULL );
				if ( FAILED ( hRes ) )
				{
					char pStrClue[512];
					wsprintf ( pStrClue, "Failed to save new custom texture plate: %s", pFilename.Get() );
					RunTimeError(RUNTIMEERROR_IMAGEERROR,pStrClue);
					SAFE_RELEASE(pLoadedTexSurface);
					SAFE_RELEASE(pPlateSurface);
					return 0;
				}

				// and the PNG for debugging
				//if(0)
				//{
				//	DestFormat = D3DXIFF_PNG;
				//	cstr pFilePNGExt;
				//	if ( iTexSet == 0 ) pFilePNGExt = cstr("_D.png");
				//	if ( iTexSet == 1 ) pFilePNGExt = cstr("_N.png");
				//	if ( iTexSet == 2 ) pFilePNGExt = cstr("_S.png");
				//	pSaveLocation = cstr(sSavePathFile) + pFilePNGExt;
				//	hRes = D3DXSaveSurfaceToFile( pSaveLocation.Get(), DestFormat, pPlateSurface, NULL, NULL );
				//}
			}

			// free temp surface captures
			SAFE_RELEASE(pPlateSurface);
			SAFE_RELEASE(pTextureDDS);
			SAFE_RELEASE(pLoadedTexSurface);

			// reload image with new file (apply auto-mipmapping when load)
			cstr pRefreshTexturesFileName = cstr(sSavePathFile) + pFileExt;
			if ( iTexSet == 0 ) 
			{
				// texture used by texture plate
				LoadImage ( pRefreshTexturesFileName.Get(), ebebuild.iTexPlateImage );
				TextureObject ( ebebuild.iBuildObj, 0, ebebuild.iTexPlateImage );
				TextureObject ( ebebuild.iBuildObj, 1, ebebuild.iTexPlateImage );

				// diffuse texture (260317 - this crashed when two EBE entities of same texture in scene)
				//int iTexIndex = t.entityprofile[iEntityProfileIndex].texdid;
				//if ( iTexIndex > 0 ) LoadImage ( pRefreshTexturesFileName.Get(), iTexIndex );
			}
			else
			{
				/*
				// normal and specular (260317 - this crashed when two EBE entities of same texture in scene)
				int iTexIndex = t.entityprofile[iEntityProfileIndex].texnid;
				if ( iTexSet == 2 ) iTexIndex = t.entityprofile[iEntityProfileIndex].texsid;
				if ( iTexIndex > 0 )
				{
					LoadImage ( pRefreshTexturesFileName.Get(), iTexIndex );
					if ( iTexSet == 1 ) TextureObject ( ebebuild.iBuildObj, 2, iTexIndex );
					if ( iTexSet == 2 ) TextureObject ( ebebuild.iBuildObj, 3, iTexIndex );
				}
				*/
			}
			
			//strangely changing diffuse and normal (maybe old code) - moved correct line further up
			//TextureObject ( ebebuild.iBuildObj, 1, ebebuild.iTexPlateImage );
			//TextureObject ( ebebuild.iBuildObj, 2, ebebuild.iTexPlateImage );
		}
		#endif
	}

	// write chosen texture to texture reference
	char pNameOnly[256];
	char pFilenameAndPath[256];
	strcpy ( pFilenameAndPath, tLoadFile.Get() );
	for ( int n = strlen(pFilenameAndPath); n > 0; n-- )
	{
		if ( pFilenameAndPath[n] == '\\' || pFilenameAndPath[n] == '/' )
		{
			strcpy ( pNameOnly, pFilenameAndPath + n + 1 );
			break;
		}
	}
	ebebuild.TXP.sTextureFile[iWhichTextureOver] = pNameOnly;

	// restore current folder
	SetDir(pOldDir.Get());

	// wicked allows new textures to be created before deleting old references and loading new ones (once dir restored)
	// destination of Textures subsets
	cstr sSavePathFileNonAbs = "ebebank\\default\\textures";
	// rare event where a texture file can be different things, so ensure the image manager 
	// deletes the image entry for this image before a new attempt to load it happens
	// no absolute path, just local within Files folder when loading
	char pReusableTextureColor[MAX_PATH];
	strcpy(pReusableTextureColor, sSavePathFileNonAbs.Get());
	strcat(pReusableTextureColor, "_color.dds");
	WickedCall_DeleteImage(pReusableTextureColor);
	char pReusableTextureNormal[MAX_PATH];
	strcpy(pReusableTextureNormal, sSavePathFileNonAbs.Get());
	strcat(pReusableTextureNormal, "_normal.dds");
	WickedCall_DeleteImage(pReusableTextureNormal);
	char pReusableTextureSurface[MAX_PATH];
	strcpy(pReusableTextureSurface, sSavePathFileNonAbs.Get());
	strcat(pReusableTextureSurface, "_surface.dds");
	WickedCall_DeleteImage(pReusableTextureSurface);
	// texture used by textures
	image_setlegacyimageloading(true);
	LoadImage ( pReusableTextureColor, ebebuild.iTexPlateImage, 0, g.gdividetexturesize);
	TextureObject ( ebebuild.iBuildObj, 0, ebebuild.iTexPlateImage );
	image_setlegacyimageloading(false);

	// Clear status Text
	//t.statusbar_s = ""; popup_text_close();

	// success
	return 1;
}

void ebe_finishsite ( void )
{
	// ensure TXP profile is saved
	ebe_savetxp(cstr(cstr("ebebank\\default\\")+cstr("textures_profile.txp")).Get());

	// save data created into entity that holds it (t.ebe.entityelementindex)
	if ( t.ebe.entityelementindex > 0 )
	{
		// latest cube data back into entity element
		int entid = t.entityelement[t.ebe.entityelementindex].bankindex;
		if ( entid > 0 )
		{
			// write ebe structure to the entity profile
			DWORD dwRLESize = t.entityprofile[entid].ebe.dwRLESize;
			ebe_packsite ( &t.entityprofile[entid].ebe.dwRLESize, &t.entityprofile[entid].ebe.pRLEData );

			// write TXP profile to the entity profile
			t.entityprofile[entid].ebe.dwMatRefCount = ebebuild.TXP.iWidth * ebebuild.TXP.iHeight;
			SAFE_DELETE ( t.entityprofile[entid].ebe.iMatRef );
			t.entityprofile[entid].ebe.iMatRef = new int[t.entityprofile[entid].ebe.dwMatRefCount];
			for ( int n = 0; n < t.entityprofile[entid].ebe.dwMatRefCount; n++ )
				t.entityprofile[entid].ebe.iMatRef[n] = ebebuild.TXP.iMaterialRef[n];

			// write texture references to the entity profile
			for ( int n = 0; n < t.entityprofile[entid].ebe.dwTexRefCount; n++ )
				SAFE_DELETE ( t.entityprofile[entid].ebe.pTexRef[n] );
			SAFE_DELETE ( t.entityprofile[entid].ebe.pTexRef );
			t.entityprofile[entid].ebe.dwTexRefCount = ebebuild.TXP.iWidth * ebebuild.TXP.iHeight;
			t.entityprofile[entid].ebe.pTexRef = new LPSTR[t.entityprofile[entid].ebe.dwTexRefCount];
			for ( int n = 0; n < t.entityprofile[entid].ebe.dwTexRefCount; n++ )
			{
				t.entityprofile[entid].ebe.pTexRef[n] = new char[256];
				strcpy ( t.entityprofile[entid].ebe.pTexRef[n], ebebuild.TXP.sTextureFile[n].Get() );
			}

			// write new texture path and name for this entity
			if ( t.entityprofile[entid].texdid > 0 ) removeinternaltexture(t.entityprofile[entid].texdid);
			t.entityprofile[entid].texpath_s = "ebebank\\default\\";
			t.entityprofile[entid].texd_s = ebe_constructlongTXPname("_color.dds");
			image_setlegacyimageloading(true);
			cstr tthistexdir_s = t.entityprofile[entid].texpath_s + t.entityprofile[entid].texd_s;
			if ( t.entityprofile[entid].transparency == 0 ) 
				t.entityprofile[entid].texdid = loadinternaltextureex(tthistexdir_s.Get(),1,0);
			else
				t.entityprofile[entid].texdid = loadinternaltextureex(tthistexdir_s.Get(),5,0);
			image_setlegacyimageloading(false);
			image_setlegacyimageloading(true);
			 tthistexdir_s = t.entityprofile[entid].texpath_s + ebe_constructlongTXPname("_normal.dds");
			 t.entityprofile[entid].texnid = loadinternaltextureex(tthistexdir_s.Get(),5,0);
			 tthistexdir_s = t.entityprofile[entid].texpath_s + ebe_constructlongTXPname("_surface.dds");
			 t.entityprofile[entid].texsid = loadinternaltextureex(tthistexdir_s.Get(),1,0);
			 image_setlegacyimageloading(false);

			// recreate entity using optimized polygons
			ebe_optimize_e();
		}

		// finished with entity
		t.ebe.entityelementindex = 0;
	}
}

void ebe_packsite ( DWORD* pdwRLEPos, DWORD** ppRLEData )
{
	// delete any previous site data
	if ( *ppRLEData != NULL ) { delete *ppRLEData; }

	// use RLE to scan cube site and pack into small memory footprint
	*pdwRLEPos = 0;
	*ppRLEData = NULL;
	for ( int iPass = 0; iPass < 2; iPass++ )
	{
		DWORD dwCount = 0;
		unsigned char lastItem = 0;
		if ( iPass == 1 ) { *ppRLEData = new DWORD[*pdwRLEPos]; }
		*pdwRLEPos = 0;
		for ( int x = 0; x < CUBEAREASIZE; x++ )
		{
			for ( int y = 0; y < CUBEAREASIZE; y++ )
			{
				for ( int z = 0; z < CUBEAREASIZE; z++ )
				{
					if ( pCubes[x][y][z] != lastItem )
					{
						if ( iPass == 1 ) { (*ppRLEData)[*pdwRLEPos] = dwCount; }
						(*pdwRLEPos)++;
						if ( iPass == 1 ) { (*ppRLEData)[*pdwRLEPos] = lastItem; }
						(*pdwRLEPos)++;
						dwCount = 0;
					}
					lastItem = pCubes[x][y][z];
					dwCount++;
				}
			}
		}
	}
}

void ebe_unpacksite ( DWORD dwRLESize, DWORD* pRLEData )
{
	int x = 0, y = 0, z = 0;
	memset ( pCubes, 0, sizeof(pCubes) );
	for ( DWORD dwPos = 0; dwPos < dwRLESize; dwPos+=2 )
	{
		DWORD dwCount = pRLEData[dwPos+0];
		unsigned char bItem = pRLEData[dwPos+1];
		for ( int n = 0; n < dwCount; n++ )
		{
			pCubes[x][y][z] = bItem;
			z++;
			if ( z >= CUBEAREASIZE )
			{
				z = 0;
				y++;
				if ( y >= CUBEAREASIZE )
				{
					y = 0;
					x++;
					if ( x >= CUBEAREASIZE )
					{
						// should reach end of RLE sequence here
						x = 0; // resetting X to prevent overflow!
					}
				}
			}
		}
	}
}

void ebe_loadtxp ( LPSTR pTXPFilename )
{
	// default pattern is single cube
	// by default, wicked will use texture array and five to start with
	ebebuild.TXP.iWidth = 0;
	ebebuild.TXP.iHeight = 0;
	for ( int n = 0; n < 64; n++ )
	{
		ebebuild.TXP.sTextureFile[n] = "";
		ebebuild.TXP.iMaterialRef[n] = 0;
	}

	// if TXP file exists, replace above pattern
	if ( FileExist(pTXPFilename) == 1 ) 
	{
		Dim ( t.data_s, 2000 );
		LoadArray ( pTXPFilename, t.data_s );
		for ( t.l = 0; t.l <= 1999; t.l++ )
		{
			t.line_s=t.data_s[t.l];
			if (  Len(t.line_s.Get())>0 ) 
			{
				if (  t.line_s.Get()[0] != ';' ) 
				{
					//  take fieldname and value
					for ( t.c = 0 ; t.c < Len(t.line_s.Get()); t.c++ )
					{
						if ( t.line_s.Get()[t.c] == '=' ) 
						{ 
							t.mid = t.c+1  ; break;
						}
					}
					t.field_s=cstr(Lower(removeedgespaces(Left(t.line_s.Get(),t.mid-1))));
					t.value_s=cstr(removeedgespaces(Right(t.line_s.Get(),Len(t.line_s.Get())-t.mid)));
					for ( t.c = 0 ; t.c < Len(t.value_s.Get()); t.c++ )
					{
						if (  t.value_s.Get()[t.c] == ',' ) 
						{ 
							t.mid = t.c+1 ; break; 
						}
					}
					t.value1=ValF(removeedgespaces(Left(t.value_s.Get(),t.mid-1)));
					t.value1_f=ValF(removeedgespaces(Left(t.value_s.Get(),t.mid-1)));
					t.value2_s=cstr(removeedgespaces(Right(t.value_s.Get(),Len(t.value_s.Get())-t.mid)));
					if ( Len(t.value2_s.Get())>0  ) t.value2 = ValF(t.value2_s.Get()); else t.value2 = -1;

					// extract field data from file
					t.tryfield_s="width"; if ( t.field_s == t.tryfield_s  )  ebebuild.TXP.iWidth = t.value1;
					t.tryfield_s="height"; if ( t.field_s == t.tryfield_s  ) ebebuild.TXP.iHeight = t.value1;

					// filename strings and material references
					if (ebebuild.TXP.iWidth > 0 && ebebuild.TXP.iHeight > 0)
					{
						int ncount = ebebuild.TXP.iWidth * ebebuild.TXP.iHeight;
						if (ncount > 0)
						{
							for (int n = 0; n < ncount; n++)
							{
								// Texture filename
								cstr sNum = cstr(100 + n);
								t.tryfield_s = cStr("t") + cStr(Right(sNum.Get(), strlen(sNum.Get()) - 1));
								if (t.field_s == t.tryfield_s) ebebuild.TXP.sTextureFile[n] = Lower(t.value_s.Get());

								// Material Index
								t.tryfield_s = cStr("m") + cStr(Right(sNum.Get(), strlen(sNum.Get()) - 1));
								if (t.field_s == t.tryfield_s) ebebuild.TXP.iMaterialRef[n] = t.value1;
							}
						}
					}
				}
			}
		}
		UnDim (  t.data_s );
	}
}

cstr ebe_constructlongTXPname ( LPSTR pExt )
{
	cstr sLongFilename = "";
	int ncount = ebebuild.TXP.iWidth * ebebuild.TXP.iHeight;
	if ( ncount > 0 )
	{
		LONGLONG lValue = 0;
		for ( int n = 0; n < ncount; n++ )
		{
			cstr sEncode = Upper(Left(ebebuild.TXP.sTextureFile[n].Get(),strlen(ebebuild.TXP.sTextureFile[n].Get())-4));
			char pEncodeString[512];
			strcpy ( pEncodeString, sEncode.Get() );
			LONGLONG lScale = 1;
			for ( int n = 0; n < strlen(pEncodeString); n++ )
			{
				int charnum = pEncodeString[n] - ' ';
				lValue = lValue + (charnum*lScale);
				lScale = lScale * 100;
			}
		}
		char num[1024];
		sprintf ( num, "%llu", lValue );
		sLongFilename = sLongFilename + num;
	}
	sLongFilename = sLongFilename + pExt;
	return sLongFilename;
}

void ebe_savetxp ( LPSTR pTXPFilename )
{
	// Calc info
	int ncount = ebebuild.TXP.iWidth * ebebuild.TXP.iHeight;

	// save two files, the editors TXP and the one that represents this arrangement uniquely
	for ( int iFilePass = 0; iFilePass < 2; iFilePass++ )
	{
		char pFileToSave[1024];
		strcpy ( pFileToSave, pTXPFilename );
		if ( iFilePass == 1 ) 
		{
			cstr sLongTXPFilename = cstr("ebebank\\default\\") + ebe_constructlongTXPname(".txp");
			strcpy ( pFileToSave, sLongTXPFilename.Get() ); 
		}

		// Save TXP file out
		int iLine = 0;
		Dim ( t.setuparr_s, 100 );
		t.setuparr_s[iLine]=";Texture Array Dimensions"; iLine++;
		t.setuparr_s[iLine]=cstr("Width = ") + ebebuild.TXP.iWidth; iLine++;
		t.setuparr_s[iLine]=cstr("Height = ") + ebebuild.TXP.iHeight; iLine++;
		t.setuparr_s[iLine]=""; iLine++;
		t.setuparr_s[iLine]=";Texture Array Files"; iLine++;
		for ( int n = 0; n < ncount; n++ ) 
		{
			cstr sNum = cstr(100+n);
			t.setuparr_s[iLine]=cstr("T") + cstr(Right(sNum.Get(),strlen(sNum.Get())-1)) + cstr(" = ") + ebebuild.TXP.sTextureFile[n]; 
			iLine++; 
		}
		t.setuparr_s[iLine]=""; iLine++;
		t.setuparr_s[iLine]=";Texture Array Materials"; iLine++;
		for ( int n = 0; n < ncount; n++ ) 
		{
			cstr sNum = cstr(100+n);
			t.setuparr_s[iLine]=cstr("M") + cstr(Right(sNum.Get(),strlen(sNum.Get())-1)) + cstr(" = ") + ebebuild.TXP.iMaterialRef[n]; 
			iLine++; 
		}
		cstr pTXPFile = cstr(pFileToSave);
		if ( FileExist(pTXPFile.Get()) == 1 ) DeleteAFile ( pTXPFile.Get() );
		SaveArray ( pTXPFile.Get(), t.setuparr_s );
	}
	UnDim ( t.setuparr_s );

	// Also save snapshot of latest textures_DNS
	cstr sLongFilename = cstr("ebebank\\default\\") + ebe_constructlongTXPname("_color.dds");
	char pRealLongFilename[MAX_PATH];
	strcpy(pRealLongFilename, sLongFilename.Get());
	GG_GetRealPath(pRealLongFilename, 1);
	sLongFilename = pRealLongFilename;
	cstr tRawPathAndFile = cstr(Left(sLongFilename.Get(),strlen(sLongFilename.Get())-10));
	cstr tDDSFilename = "ebebank\\default\\textures_color.dds";
	char pRealDDSFilename[MAX_PATH];
	strcpy(pRealDDSFilename, tDDSFilename.Get());
	GG_GetRealPath(pRealDDSFilename, 0);
	tDDSFilename = pRealDDSFilename;
	if ( FileExist(tDDSFilename.Get()) == 1 ) 
	{
		cstr sDDSFile = tRawPathAndFile + cstr("_color.dds");
		if ( FileExist(sDDSFile.Get()) == 1 ) DeleteAFile ( sDDSFile.Get() );
		CopyFileA ( tDDSFilename.Get(), sDDSFile.Get(), FALSE );
		 tDDSFilename = "ebebank\\default\\textures_normal.dds";
		 strcpy(pRealDDSFilename, tDDSFilename.Get());
		 GG_GetRealPath(pRealDDSFilename, 0);
		 tDDSFilename = pRealDDSFilename;
		 sDDSFile = tRawPathAndFile + cstr("_normal.dds");
		 if ( FileExist(sDDSFile.Get()) == 1 ) DeleteAFile ( sDDSFile.Get() );
		 CopyFileA ( tDDSFilename.Get(), sDDSFile.Get(), FALSE );
		 tDDSFilename = "ebebank\\default\\textures_surface.dds";
		 strcpy(pRealDDSFilename, tDDSFilename.Get());
		 GG_GetRealPath(pRealDDSFilename, 0);
		 tDDSFilename = pRealDDSFilename;
		 sDDSFile = tRawPathAndFile + cstr("_surface.dds");
		 if ( FileExist(sDDSFile.Get()) == 1 ) DeleteAFile ( sDDSFile.Get() );
		 CopyFileA ( tDDSFilename.Get(), sDDSFile.Get(), FALSE );
	}
}

void ebe_restoreebedefaulttextures(void)
{
	char pRealDestPath[MAX_PATH];
	char pRealDestPathAndFile[MAX_PATH];
	// get real destination for texture copies
	sprintf(pRealDestPath, "%s\\Files\\ebebank\\default\\", g.fpscrootdir_s.Get());
	GG_GetRealPath(pRealDestPath, 1);

	// ensure when start new level/etc, the old EBE textures are reset!
	sprintf(pRealDestPathAndFile, "%stextures_profile.txp", pRealDestPath);
	CopyFileA("ebebank\\default\\original_profile.txp", pRealDestPathAndFile,FALSE);
	sprintf(pRealDestPathAndFile, "%stextures_color.dds", pRealDestPath);
	CopyFileA("ebebank\\default\\original_color.dds", pRealDestPathAndFile,FALSE);
	sprintf(pRealDestPathAndFile, "%stextures_normal.dds", pRealDestPath);
	CopyFileA("ebebank\\default\\original_normal.dds", pRealDestPathAndFile,FALSE);
	sprintf(pRealDestPathAndFile, "%stextures_surface.dds", pRealDestPath);
	CopyFileA("ebebank\\default\\original_surface.dds", pRealDestPathAndFile,FALSE);

	// also free if EBE already active (to erase old plate texture)
	if (t.ebe.active == 1) ebe_free();
}

int ebe_createnewstructuretexture ( LPSTR pDestTerrainTextureFile, int iWhichTextureOver, LPSTR pTexFileToLoad, int iSeamlessMode, int iCompressIt )
{
	return ImageCreateTexturePlate( pDestTerrainTextureFile, iWhichTextureOver, pTexFileToLoad, iSeamlessMode, iCompressIt);
}
