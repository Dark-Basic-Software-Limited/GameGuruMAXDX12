int ImageCreateTexturePlate(LPSTR pDestTerrainTextureFile, int iWhichTextureOver, LPSTR pTexFileToLoad, int iSeamlessMode, int iCompressIt)
{
	// check if texture to load exists
	GGIMAGE_INFO finfo;
	HRESULT hr = D3DX11GetImageInfoFromFileA( pTexFileToLoad, NULL, &finfo, NULL );

	// real destination for image creation
	char pRealDestination[MAX_PATH];
	strcpy(pRealDestination, pDestTerrainTextureFile);
	GG_GetRealPath(pRealDestination, 1);

	// filenames to WCHAR
	wchar_t wFilenameInsert[512];
	wchar_t wFilenamePlate[512];
	std::string pPlateFilename = pRealDestination;
	MultiByteToWideChar(CP_ACP, 0, pTexFileToLoad, -1, wFilenameInsert, sizeof(wFilenameInsert));
	MultiByteToWideChar(CP_ACP, 0, pPlateFilename.c_str(), -1, wFilenamePlate, sizeof(wFilenamePlate));

	// is insert a DDS or other
	bool bInsertTextureIsDDS = false;
	if ( strnicmp ( pTexFileToLoad + strlen(pTexFileToLoad) - 4, ".dds", 4 ) == NULL )
		bInsertTextureIsDDS = true;

	// create and load the texture selected
	DirectX::ScratchImage imageTextureToInsert;
	DirectX::ScratchImage imageTexturePlate;
	DirectX::ScratchImage convertedTextureToInsert;
	DirectX::ScratchImage convertedTexturePlate;
	LPGGTEXTURE pLoadedTexSurface1024 = NULL;
	LPGGTEXTURE pLoadedTexSurface512 = NULL;
	LPGGTEXTURE pLoadedTexSurface256512 = NULL;
	LPGGTEXTURE pLoadedTexSurface512256 = NULL;
	LPGGTEXTURE pPlateSurface = NULL;
	if ( hr == S_OK )
	{
		// create/load texture to be inserted
		DirectX::TexMetadata insertdata;
		if ( bInsertTextureIsDDS == true )
		{
			hr = GetMetadataFromDDSFile( wFilenameInsert, DirectX::DDS_FLAGS_NONE, insertdata );			
			hr = LoadFromDDSFile( wFilenameInsert, DirectX::DDS_FLAGS_NONE, &insertdata, imageTextureToInsert );
		}
		else
		{
			hr = GetMetadataFromWICFile( wFilenameInsert, DirectX::DDS_FLAGS_NONE, insertdata );			
			hr = LoadFromWICFile( wFilenameInsert, DirectX::DDS_FLAGS_NONE, &insertdata, imageTextureToInsert );
		}
		if ( SUCCEEDED(hr) )
		{
			// create/load texture of plate
			DirectX::TexMetadata platedata;
			hr = GetMetadataFromDDSFile( wFilenamePlate, DirectX::DDS_FLAGS_NONE, platedata );			
			if ( hr != S_OK )
			{
				// if plate file not exist, create one and provide dimensions
				finfo.Width = 4096;
				finfo.Height = 4096;

				// create the plate from fresh
				imageTexturePlate.Initialize2D (DXGIFORMATR8G8B8A8UNORM, finfo.Width, finfo.Height, 1, 1, 0 );
				platedata = imageTexturePlate.GetMetadata();
			}
			else
			{
				// existing plate exists, load it in
				hr = LoadFromDDSFile( wFilenamePlate, DirectX::DDS_FLAGS_NONE, &platedata, imageTexturePlate );
				if ( FAILED(hr) ) platedata.width = 0;
			}
			if ( platedata.width > 0 )
			{
				// ensure we convert compressed textures to uncompressed one, store as pWrkImage
				DirectX::ScratchImage* pWrkImage = &imageTextureToInsert;
				if ( imageTextureToInsert.GetMetadata().format >= DXGI_FORMAT_BC1_TYPELESS && imageTextureToInsert.GetMetadata().format <= DXGI_FORMAT_BC5_SNORM )
				{
					hr = Decompress(imageTextureToInsert.GetImages(), imageTextureToInsert.GetImageCount(), imageTextureToInsert.GetMetadata(),
						DXGI_FORMAT_B8G8R8A8_UNORM, convertedTextureToInsert);
					pWrkImage = &convertedTextureToInsert;
				}

				// resize
				DirectX::ScratchImage InsertImage1024;
				DirectX::ScratchImage InsertImage512;
				DirectX::ScratchImage InsertImage256512;
				DirectX::ScratchImage InsertImage512256;
				hr = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), 1024, 1024, DirectX::TEX_FILTER_SEPARATE_ALPHA, InsertImage1024 );
				CreateTexture(m_pD3D, InsertImage1024.GetImages(), InsertImage1024.GetImageCount(), InsertImage1024.GetMetadata(), &pLoadedTexSurface1024 );
				if ( iSeamlessMode == 0 )
				{
					// 512x512 at center
					hr = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), 512, 512, DirectX::TEX_FILTER_SEPARATE_ALPHA, InsertImage512 );
					CreateTexture(m_pD3D, InsertImage512.GetImages(), InsertImage512.GetImageCount(), InsertImage512.GetMetadata(), &pLoadedTexSurface512 );
					hr = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), 256, 512, DirectX::TEX_FILTER_SEPARATE_ALPHA, InsertImage256512 );
					CreateTexture(m_pD3D, InsertImage256512.GetImages(), InsertImage256512.GetImageCount(), InsertImage256512.GetMetadata(), &pLoadedTexSurface256512 );
					hr = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), 512, 256, DirectX::TEX_FILTER_SEPARATE_ALPHA, InsertImage512256 );
					CreateTexture(m_pD3D, InsertImage512256.GetImages(), InsertImage512256.GetImageCount(), InsertImage512256.GetMetadata(), &pLoadedTexSurface512256 );
				}
				else
				{
					// 1022x1022 at center
					hr = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), 1022, 1022, DirectX::TEX_FILTER_SEPARATE_ALPHA, InsertImage512 );
					hr = CreateTexture(m_pD3D, InsertImage512.GetImages(), InsertImage512.GetImageCount(), InsertImage512.GetMetadata(), &pLoadedTexSurface512 );
				}
				// texture plate always compressed
				hr = Decompress( imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), imageTexturePlate.GetMetadata(),
					DXGI_FORMAT_B8G8R8A8_UNORM, convertedTexturePlate );
				if ( convertedTexturePlate.GetImageCount() == 0 )
				{
					CreateTexture(m_pD3D, imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), platedata, &pPlateSurface );
				}
				else
				{
					platedata.format = convertedTexturePlate.GetImages()->format;
					CreateTexture(m_pD3D, convertedTexturePlate.GetImages(), convertedTexturePlate.GetImageCount(), platedata, &pPlateSurface );
				}
			}
		}

		// paste loaded texture into plate (60 in center of atlas texture area)
		if ( pLoadedTexSurface1024 && pPlateSurface ) 
		{
			// work out exact offset to slot position
			int iRow = iWhichTextureOver / 4;
			int iCol = iWhichTextureOver - (iRow*4);
			int iTexSlotOffsetX = iCol * 1024;
			int iTexSlotOffsetY = iRow * 1024;
			RECT rcPlate = RECT();

			// paste to fill 1024x1024 initially (to get at corners)
			rcPlate.left = iTexSlotOffsetX;
			rcPlate.top = iTexSlotOffsetY;
			rcPlate.right = iTexSlotOffsetX+1024;
			rcPlate.bottom = iTexSlotOffsetY+1024;
			m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface1024, 0, NULL );
			if ( iSeamlessMode == 0 )
			{
				// paste squashed 256x512 borders to help seamlessness
				// left
				rcPlate.left = iTexSlotOffsetX+0; rcPlate.top = iTexSlotOffsetY+256; rcPlate.right = iTexSlotOffsetX+256; rcPlate.bottom = iTexSlotOffsetY+256+512;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface256512, 0, NULL );
				// right
				rcPlate.left = iTexSlotOffsetX+256+512; rcPlate.top = iTexSlotOffsetY+256; rcPlate.right = iTexSlotOffsetX+1024; rcPlate.bottom = iTexSlotOffsetY+256+512;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface256512, 0, NULL );
				// top
				rcPlate.left = iTexSlotOffsetX+256; rcPlate.top = iTexSlotOffsetY+0; rcPlate.right = iTexSlotOffsetX+256+512; rcPlate.bottom = iTexSlotOffsetY+256;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512256, 0, NULL );
				// bottom
				rcPlate.left = iTexSlotOffsetX+256; rcPlate.top = iTexSlotOffsetY+256+512; rcPlate.right = iTexSlotOffsetX+256+512; rcPlate.bottom = iTexSlotOffsetY+1024;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512256, 0, NULL );
				// paste insert so smaller 512x512 terrain texture atlas can be seamless
				rcPlate.left = iTexSlotOffsetX+256; rcPlate.top = iTexSlotOffsetY+256; rcPlate.right = iTexSlotOffsetX+256+512; rcPlate.bottom = iTexSlotOffsetY+256+512;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
			}
			else
			{
				// paste 1022x1022 on borders for seamlessness
				int iX = 1, iY = 0;
				rcPlate.left = iTexSlotOffsetX+iX; rcPlate.top = iTexSlotOffsetY+iY; rcPlate.right = iTexSlotOffsetX+iX+1022; rcPlate.bottom = iTexSlotOffsetY+iY+1022;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
				iX = 1, iY = 2;
				rcPlate.left = iTexSlotOffsetX+iX; rcPlate.top = iTexSlotOffsetY+iY; rcPlate.right = iTexSlotOffsetX+iX+1022; rcPlate.bottom = iTexSlotOffsetY+iY+1022;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
				iX = 0, iY = 1;
				rcPlate.left = iTexSlotOffsetX+iX; rcPlate.top = iTexSlotOffsetY+iY; rcPlate.right = iTexSlotOffsetX+iX+1022; rcPlate.bottom = iTexSlotOffsetY+iY+1022;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
				iX = 2, iY = 1;
				rcPlate.left = iTexSlotOffsetX+iX; rcPlate.top = iTexSlotOffsetY+iY; rcPlate.right = iTexSlotOffsetX+iX+1022; rcPlate.bottom = iTexSlotOffsetY+iY+1022;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
				// then paste into surface at 1022x1022 (so can have seamless textures within atlas)
				rcPlate.left = iTexSlotOffsetX+1; rcPlate.top = iTexSlotOffsetY+1;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
			}

			// replace imageTexturePlate with contents of pPlateSurface
			hr = CaptureTexture( m_pD3D, m_pImmediateContext, pPlateSurface, imageTexturePlate );
			if ( SUCCEEDED(hr) )
			{
				// create mipmaps
				DirectX::ScratchImage mipChain;
				hr = GenerateMipMaps( imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), imageTexturePlate.GetMetadata(), DirectX::TEX_FILTER_SEPARATE_ALPHA, 0, mipChain );

				// compressed or not
				if ( iCompressIt == 0 )
				{
					// save new UNCOMPRESSED texture surface out
					const DirectX::Image* img = mipChain.GetImages();
					hr = SaveToDDSFile( img, mipChain.GetImageCount(), mipChain.GetMetadata(), DirectX::DDS_FLAGS_NONE, wFilenamePlate );
				}
				else
				{
					// compress to a DXT5 (BC3) texture
					hr = Compress( mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(), 
						DXGI_FORMAT_BC3_UNORM, DirectX::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, convertedTexturePlate );

					// save new texture surface out
					const DirectX::Image* img = convertedTexturePlate.GetImages();
					hr = SaveToDDSFile( img, convertedTexturePlate.GetImageCount(), convertedTexturePlate.GetMetadata(), DirectX::DDS_FLAGS_NONE, wFilenamePlate );
				}
			}
		}

		// free temp surface captures
		SAFE_RELEASE(pPlateSurface);
		SAFE_RELEASE(pLoadedTexSurface1024);
		SAFE_RELEASE(pLoadedTexSurface512);
		SAFE_RELEASE(pLoadedTexSurface256512);
		SAFE_RELEASE(pLoadedTexSurface512256);
	}

	// success
	return 1;
}

LPGGTEXTURE ConvertBackBufferToNewFormat ( LPGGSURFACE pBackBuffer, GGFORMAT NewFormat)
{
	// read data of backbuffer, copy into known format (as cannot use CopySubResource to do conversions)
	LPGGTEXTURE pCorrectBackBufferTexture = NULL;

	// read backbuffer surface into scratch image
	DirectX::ScratchImage imageBBSurface;
	HRESULT hr = DirectX::CaptureTexture( m_pD3D, m_pImmediateContext, pBackBuffer, imageBBSurface );
	if (SUCCEEDED(hr))
	{
		// convert to desired format
		DirectX::ScratchImage imageBBSurfaceConverted;
		
		hr = DirectX::Convert(imageBBSurface.GetImages(), imageBBSurface.GetImageCount(), imageBBSurface.GetMetadata(), NewFormat, DirectX::TEX_FILTER_DITHER, DirectX::TEX_THRESHOLD_DEFAULT, imageBBSurfaceConverted);
		if (SUCCEEDED(hr))
		{
			//if (0)
			//{
			//	// save something out as a test - yes, the backbuffer was captured fine :)
			//	wchar_t wFilenamePlate[512];
			//	MultiByteToWideChar(CP_ACP, 0, "leeleetestbackbufferconvert.png", -1, wFilenamePlate, sizeof(wFilenamePlate));
			//	const DirectX::Image* img = imageBBSurfaceConverted.GetImages();
			//	hr = SaveToWICFile(*img, DirectX::DDS_FLAGS_NONE, GUID_ContainerFormatPng, wFilenamePlate, NULL);
			//}
			// create new surface texture from new image
			hr = DirectX::CreateTexture(m_pD3D, imageBBSurfaceConverted.GetImages(), imageBBSurfaceConverted.GetImageCount(), imageBBSurfaceConverted.GetMetadata(), &pCorrectBackBufferTexture);
		}
	}

	// return newly minted texture (from which surface can be read later on)
	return pCorrectBackBufferTexture;
}

LPGGTEXTURE CreateCroppedTexture(LPGGTEXTURE pSourceTexture, D3D11_BOX rc)
{
	// take a texture, take a crop from it and return that
	LPGGTEXTURE pCroppedTexture = NULL;

	// read backbuffer surface into scratch image
	DirectX::ScratchImage imageOrigTexture;
	HRESULT hr = DirectX::CaptureTexture( m_pD3D, m_pImmediateContext, pSourceTexture, imageOrigTexture );
	if (SUCCEEDED(hr))
	{
		// original image
		const DirectX::Image* img = imageOrigTexture.GetImages();

		// crop into new scratchimage
		size_t width = rc.right - rc.left;
		size_t height = rc.bottom - rc.top;
		DirectX::ScratchImage imageCroppedTexture;
		imageCroppedTexture.Initialize2D(img->format, width, height, 1, 1);
		const DirectX::Image* imgdest = imageCroppedTexture.GetImages();
		if (imgdest)
		{
			DirectX::Rect srcRect = { rc.left, rc.top, width, height };
			hr = DirectX::CopyRectangle(*img, srcRect, *imgdest, DirectX::TEX_FILTER_DEFAULT, 0, 0);

			// create new surface texture from new image
			hr = DirectX::CreateTexture(m_pD3D, imageCroppedTexture.GetImages(), imageCroppedTexture.GetImageCount(), imageCroppedTexture.GetMetadata(), &pCroppedTexture);
		}
	}

	// return newly cropped texture
	return pCroppedTexture;
}

DARKSDK unsigned char* DecompressImage ( LPGGSURFACE pTexSurface )
{
	unsigned char* pixels = NULL;

	GGSURFACE_DESC srcddsd;
	if ( pTexSurface )
	{
		// determine size of surface
		HRESULT hRes;
		pTexSurface->GetDesc ( &srcddsd );

		// create and load the texture selected
		LPGGTEXTURE pPlateSurface = NULL;

		// load compressed texture to a plate
		DirectX::ScratchImage imageTexturePlate;
		HRESULT hr = CaptureTexture ( m_pD3D, m_pImmediateContext, pTexSurface, imageTexturePlate );
		if ( SUCCEEDED ( hr ) )
		{
			// decompress the plate
			DirectX::ScratchImage uncompressedTexturePlate;
			hr = Decompress ( imageTexturePlate.GetImages ( ), imageTexturePlate.GetImageCount ( ), imageTexturePlate.GetMetadata ( ),
				DXGIFORMATR8G8B8A8UNORM, uncompressedTexturePlate );
			if (FAILED(hr))
			{
				unsigned int iSize = imageTexturePlate.GetMetadata().width * imageTexturePlate.GetMetadata().height * 4;
				pixels = new unsigned char[iSize];
				memcpy(&pixels[0], imageTexturePlate.GetPixels(), iSize);
			}
			else
			{
				unsigned int iSize = uncompressedTexturePlate.GetMetadata().width * uncompressedTexturePlate.GetMetadata().height * 4;
				pixels = new unsigned char[iSize];
				memcpy(&pixels[0], uncompressedTexturePlate.GetPixels(), iSize);
			}

			// free surface and leave, completed save with DirectXTex functions (as needed format conversion)
			SAFE_RELEASE ( pTexSurface );
			return pixels;
		}
	}
	
	return NULL;
}

DARKSDK int ImageFormat(int iID)
{
	// get the width of an image
	if (iID<1 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
		return false;
	}

	// update internal data
	if (!UpdatePtrImage(iID))
		return -1;

	LPGGSURFACE pTexSurface = NULL;
	GGSURFACE_DESC imgdesc;
	m_imgptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTexSurface);
	if (!pTexSurface) return -1;
	pTexSurface->GetDesc(&imgdesc);
	SAFE_RELEASE(pTexSurface);
	return imgdesc.Format;
}

// Load dds located at sourceFile and convert to newFormat with newWidth and newHeight
DARKSDK int ConvertDDSCompressedFormat(ID3D11Device* device, char* sourceFile, GGFORMAT newFormat, int newWidth, int newHeight, char* outputFile)
{
	if (strcmp(sourceFile + strlen(sourceFile) - 4, ".dds") != 0)
	{
		// Can only operate on DDS textures
		return 0;
	}

	// Load the dds texture that is going to be converted
	DirectX::TexMetadata info;
	DirectX::ScratchImage originalImage;
	wchar_t wTexFilename[512];
	MultiByteToWideChar(CP_ACP, 0, sourceFile, -1, wTexFilename, sizeof(wTexFilename));
	HRESULT hr = LoadFromDDSFile(wTexFilename, DirectX::DDS_FLAGS_NONE, &info, originalImage);
	if (FAILED(hr))
	{
		return 0;
	}

	if (info.format == newFormat && newWidth == info.width && newHeight == info.height)
	{
		// No changes needed.
		return 2;
	}

	DirectX::ScratchImage* pImageToEdit = &originalImage;
	DirectX::ScratchImage decompressedImage;

	// Decompress texture if required
	if ((info.format >= DXGI_FORMAT_BC1_TYPELESS && info.format <= DXGI_FORMAT_BC5_SNORM) || (info.format >= DXGI_FORMAT_BC6H_TYPELESS && info.format <= DXGI_FORMAT_BC7_UNORM_SRGB))
	{
		hr = DirectX::Decompress(pImageToEdit->GetImages(), pImageToEdit->GetImageCount(), info, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, decompressedImage);
		pImageToEdit = &decompressedImage;
		if (FAILED(hr))
		{
			return 0;
		}
	}

	// Convert pixel format of the texture
	DirectX::ScratchImage convertedImage;
	if (pImageToEdit->GetMetadata().format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
	{
		hr = DirectX::Convert(pImageToEdit->GetImages(), pImageToEdit->GetImageCount(), pImageToEdit->GetMetadata(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, convertedImage);
		if (FAILED(hr))
		{
			return 0;
		}
		pImageToEdit = &convertedImage;
	}

	// Resize the texture
	DirectX::ScratchImage resizedImage;
	if (info.width != newWidth || info.height != newHeight)
	{
		hr = DirectX::Resize(pImageToEdit->GetImages(), pImageToEdit->GetImageCount(), pImageToEdit->GetMetadata(), newWidth, newHeight, DirectX::TEX_FILTER_DEFAULT, resizedImage);
		if (FAILED(hr))
		{
			return 0;
		}
		pImageToEdit = &resizedImage;
	}

	// Generate Mip maps
	DirectX::ScratchImage mipChain;
	if (info.mipLevels <= 1)
	{
		hr = DirectX::GenerateMipMaps(pImageToEdit->GetImages(), pImageToEdit->GetImageCount(), pImageToEdit->GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipChain);
		if (FAILED(hr))
		{
			return 0;
		}
		pImageToEdit = &mipChain;
	}
	
	// Compress the final resulting texture into the desired format
	DirectX::ScratchImage compressedImage;
	// Getting a freeze when using BC7 compression, but not for DirectCompute-based compression (BC6H/BC7 only)
	if (newFormat >= DXGI_FORMAT_BC6H_TYPELESS && newFormat <= DXGI_FORMAT_BC7_UNORM_SRGB)
	{
#ifdef WICKEDENGINE
		hr = DirectX::Compress(device, pImageToEdit->GetImages(), pImageToEdit->GetImageCount(), pImageToEdit->GetMetadata(), newFormat, DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT, compressedImage);
#endif
	}
	else
	{
		hr = DirectX::Compress(pImageToEdit->GetImages(), pImageToEdit->GetImageCount(), pImageToEdit->GetMetadata(), newFormat, DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT, compressedImage);
	}
	pImageToEdit = &compressedImage;
	if (FAILED(hr))
	{
		return 0;
	}

	hr = DirectX::SaveToDDSFile(pImageToEdit->GetImages(), pImageToEdit->GetImageCount(), pImageToEdit->GetMetadata(), DirectX::DDS_FLAGS_NONE, wTexFilename);
	if (FAILED(hr))
	{
		return 0;
	}

	return 1;
}

