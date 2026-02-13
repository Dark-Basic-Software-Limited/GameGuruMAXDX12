void LoadColorNormalSpecGloss ( sMesh* pMesh, LPSTR pName, LPSTR TexturePath, int iDBProMode, int iDivideTextureSize, int* piImageDiffuseIndex, int* piImageAOIndex, int* piImageNormalIndex, int* piImageSpecularIndex, int* piImageGlossIndex, int* piImageMaskIndex, int* piImageIlluminationIndex )
{
	// take copy of base texture name
	char pBaseTexName[MAX_STRING];
	strcpy ( pBaseTexName, pName );

	// 270618 - first check if diffuse file exists, if not, try tacking PBR texture naming convention to it
	if ( strlen ( pName ) > 4 )
	{
		*piImageDiffuseIndex = LoadOrFindTextureAsImage ( pBaseTexName, TexturePath, iDivideTextureSize );
		if ( *piImageDiffuseIndex == 0 )
		{
			// convert regular file reference to PBR one
			strcpy ( pBaseTexName, pName );
			pBaseTexName[strlen(pBaseTexName)-4]=0;
			strcat ( pBaseTexName, "_color.png" );
		}
	}

	// load the base color texture 
	*piImageDiffuseIndex = LoadOrFindTextureAsImage ( pBaseTexName, TexturePath, iDivideTextureSize );

	// if success, also check for and record a normal, specular and gloss (Fuse/Sketchfab character)
	*piImageAOIndex = 0;
	*piImageNormalIndex = 0;
	*piImageSpecularIndex = 0;
	*piImageGlossIndex = 0;
	*piImageMaskIndex = 0;
	*piImageIlluminationIndex = 0;
	if ( *piImageDiffuseIndex != 0 && strlen(pBaseTexName) > 6 )
	{
		// free any existing texture data for entries below
		SAFE_DELETE_ARRAY ( pMesh->pTextures );
		pMesh->dwTextureCount = 8;
		sTexture* pTexture = new sTexture[pMesh->dwTextureCount];
		pMesh->pTextures = pTexture;

		// strip file extenion
		char pTextureName[MAX_STRING];
		strcpy ( pTextureName, pBaseTexName );
		pTextureName[strlen(pTextureName)-4] = 0;

		// determine if non-PBR (DNS) or PBR (color,normal,metalness,gloss) naming convention
		int iTextureType = 0;
		LPSTR pAlbedoVariant = "";
		char pTmpName[MAX_STRING];
		if ( strnicmp ( pTextureName + strlen(pTextureName) - 5, "color", 5 ) == NULL ) 
		{
			pAlbedoVariant = "color";
			strcpy ( pTmpName, pTextureName );
			pTmpName[strlen(pTmpName)-5]=0;
			strcpy ( pTextureName, pTmpName );
			iTextureType = 2;
		}
		if ( iTextureType == 0 && strnicmp ( pTextureName + strlen(pTextureName) - 7, "diffuse", 7 ) == NULL ) 
		{
			pAlbedoVariant = "diffuse";
			strcpy ( pTmpName, pTextureName );
			pTmpName[strlen(pTmpName)-7]=0;
			strcpy ( pTextureName, pTmpName );
			iTextureType = 2;
		}
		if ( iTextureType == 0 && strnicmp ( pTextureName + strlen(pTextureName) - 1, "d", 1 ) == NULL ) 
		{
			pAlbedoVariant = "d";
			strcpy ( pTmpName, pTextureName );
			pTmpName[strlen(pTmpName)-1]=0;
			strcpy ( pTextureName, pTmpName );
			iTextureType = 1;
		}

		// act on texture type
		if ( iTextureType == 0 || iTextureType == 1 ) // non-PBR - use DNS or 'diffuse only if not DNS'
		{
			// diffuse
			strcpy ( pTmpName, pTextureName );
			strcat ( pTmpName, pAlbedoVariant );
			strcat ( pTmpName, ".png" );
			strcpy ( pMesh->pTextures [ 0 ].pName, pTmpName );

			// use diffuse only (0) or DNS (1)
			if ( iTextureType == 0 )
			{
				// 151018 - when model has no PBR or DNS namings, just use diffuse for importer to expand on later
				*piImageNormalIndex = LoadOrFindTextureAsImage("effectbank\\reloaded\\media\\blank_N.dds", TexturePath, iDivideTextureSize);
				*piImageSpecularIndex = LoadOrFindTextureAsImage("effectbank\\reloaded\\media\\blank_none_S.dds", TexturePath, iDivideTextureSize);
			}
			else
			{
				// normal
				strcpy ( pTmpName, pTextureName );
				strcat ( pTmpName, "n.png" );
				strcpy ( pMesh->pTextures [ 2 ].pName, pTmpName );
				*piImageNormalIndex = LoadOrFindTextureAsImage ( pTmpName, TexturePath, iDivideTextureSize );
				if ( *piImageNormalIndex == 0 ) *piImageNormalIndex = LoadOrFindTextureAsImage ( "effectbank\\reloaded\\media\\blank_N.dds", TexturePath, iDivideTextureSize );

				// specular
				strcpy ( pTmpName, pTextureName );
				strcat ( pTmpName, "s.png" );
				strcpy ( pMesh->pTextures [ 3 ].pName, pTmpName );
				*piImageSpecularIndex = LoadOrFindTextureAsImage ( pTmpName, TexturePath, iDivideTextureSize );
				if ( *piImageSpecularIndex == 0 ) *piImageSpecularIndex = LoadOrFindTextureAsImage ( "effectbank\\reloaded\\media\\blank_none_S.dds", TexturePath, iDivideTextureSize );
			}

			// no AO or gloss in DNS system
			*piImageAOIndex = LoadOrFindTextureAsImage("effectbank\\reloaded\\media\\blank_O.dds", TexturePath, iDivideTextureSize);
			*piImageGlossIndex = LoadOrFindTextureAsImage("effectbank\\reloaded\\media\\materials\\0_Gloss.dds", TexturePath, iDivideTextureSize);
			*piImageMaskIndex = LoadOrFindTextureAsImage("effectbank\\reloaded\\media\\blank_O.dds", TexturePath, iDivideTextureSize);
			
			//PE: illumination was also missing.
			strcpy(pTmpName, pTextureName);
			strcat(pTmpName, "i.png");
			strcpy(pMesh->pTextures[7].pName, pTmpName);
			*piImageIlluminationIndex = LoadOrFindTextureAsImage(pTmpName, TexturePath, iDivideTextureSize);
			if (*piImageIlluminationIndex == 0) *piImageIlluminationIndex = LoadOrFindTextureAsImage("effectbank\\reloaded\\media\\blank_none_S.dds", TexturePath, iDivideTextureSize);
		}
		if ( iTextureType == 2 ) // PBR - use color,normal,metalness,gloss,illumination
		{
			// color
			strcpy ( pTmpName, pTextureName );
			strcat ( pTmpName, pAlbedoVariant );
			strcat ( pTmpName, ".png" );
			strcpy ( pMesh->pTextures [ 0 ].pName, pTmpName );

			// ao
			strcpy ( pTmpName, pTextureName );
			strcat ( pTmpName, "ao.png" );
			strcpy ( pMesh->pTextures [ 1 ].pName, pTmpName );
			*piImageAOIndex = LoadOrFindTextureAsImage ( pTmpName, TexturePath, iDivideTextureSize );
			if ( *piImageAOIndex == 0 ) *piImageAOIndex = LoadOrFindTextureAsImage ( "effectbank\\reloaded\\media\\blank_O.dds", TexturePath, iDivideTextureSize );

			// normal
			strcpy ( pTmpName, pTextureName );
			strcat ( pTmpName, "normal.png" );
			strcpy ( pMesh->pTextures [ 2 ].pName, pTmpName );
			*piImageNormalIndex = LoadOrFindTextureAsImage ( pTmpName, TexturePath, iDivideTextureSize );
			if ( *piImageNormalIndex == 0 ) *piImageNormalIndex = LoadOrFindTextureAsImage ( "effectbank\\reloaded\\media\\blank_N.dds", TexturePath, iDivideTextureSize );

			// specular
			strcpy ( pTmpName, pTextureName );
			strcat ( pTmpName, "specular.png" );
			strcpy ( pMesh->pTextures [ 3 ].pName, pTmpName );
			*piImageSpecularIndex = LoadOrFindTextureAsImage ( pTmpName, TexturePath, iDivideTextureSize );
			if ( *piImageSpecularIndex == 0 )
			{
				// or metalness
				strcpy ( pTmpName, pTextureName );
				strcat ( pTmpName, "metalness.png" );
				strcpy ( pMesh->pTextures [ 3 ].pName, pTmpName );
				*piImageSpecularIndex = LoadOrFindTextureAsImage ( pTmpName, TexturePath, iDivideTextureSize );
				if ( *piImageSpecularIndex == 0 ) *piImageSpecularIndex = LoadOrFindTextureAsImage ( "effectbank\\reloaded\\media\\materials\\0_Metalness.dds", TexturePath, iDivideTextureSize );
			}

			// gloss
			strcpy ( pTmpName, pTextureName );
			strcat ( pTmpName, "gloss.png" );
			strcpy ( pMesh->pTextures [ 4 ].pName, pTmpName );
			*piImageGlossIndex = LoadOrFindTextureAsImage ( pTmpName, TexturePath, iDivideTextureSize );
			if ( *piImageGlossIndex == 0 ) *piImageGlossIndex = LoadOrFindTextureAsImage ( "effectbank\\reloaded\\media\\materials\\0_Gloss.dds", TexturePath, iDivideTextureSize );

			// mask (or height?)
			strcpy ( pTmpName, pTextureName );
			strcat ( pTmpName, "mask.png" );
			strcpy ( pMesh->pTextures [ 5 ].pName, pTmpName );
			*piImageMaskIndex = LoadOrFindTextureAsImage ( pTmpName, TexturePath, iDivideTextureSize );
			if ( *piImageMaskIndex == 0 ) *piImageMaskIndex = LoadOrFindTextureAsImage ( "effectbank\\reloaded\\media\\blank_O.dds", TexturePath, iDivideTextureSize );

			// illumination
			strcpy ( pTmpName, pTextureName );
			strcat ( pTmpName, "illumination.png" );
			strcpy ( pMesh->pTextures [ 7 ].pName, pTmpName );
			*piImageIlluminationIndex = LoadOrFindTextureAsImage ( pTmpName, TexturePath, iDivideTextureSize );
			if ( *piImageIlluminationIndex == 0 ) *piImageIlluminationIndex = LoadOrFindTextureAsImage ( "effectbank\\reloaded\\media\\blank_none_S.dds", TexturePath, iDivideTextureSize );
		}
	}

	// Populate texture set
	if ( *piImageDiffuseIndex != 0 )
	{
		pMesh->pTextures[0].iImageID = *piImageDiffuseIndex;
		pMesh->pTextures[0].pTexturesRef = GetImagePointer ( pMesh->pTextures[0].iImageID );
		pMesh->pTextures[0].pTexturesRefView = GetImagePointerView ( pMesh->pTextures[0].iImageID );
		pMesh->pTextures[0].dwBlendMode = GGTOP_MODULATE;
		pMesh->pTextures[0].dwBlendArg1 = GGTA_TEXTURE;
		pMesh->pTextures[1].iImageID = *piImageAOIndex;
		pMesh->pTextures[1].pTexturesRef = GetImagePointer ( pMesh->pTextures[1].iImageID );
		pMesh->pTextures[1].pTexturesRefView = GetImagePointerView ( pMesh->pTextures[1].iImageID );
		pMesh->pTextures[1].dwBlendMode = GGTOP_MODULATE;
		pMesh->pTextures[1].dwBlendArg1 = GGTA_TEXTURE;
		pMesh->pTextures[2].iImageID = *piImageNormalIndex;
		pMesh->pTextures[2].pTexturesRef = GetImagePointer ( pMesh->pTextures[2].iImageID );
		pMesh->pTextures[2].pTexturesRefView = GetImagePointerView ( pMesh->pTextures[2].iImageID );
		pMesh->pTextures[2].dwBlendMode = GGTOP_MODULATE;
		pMesh->pTextures[2].dwBlendArg1 = GGTA_TEXTURE;
		pMesh->pTextures[3].iImageID = *piImageSpecularIndex;
		pMesh->pTextures[3].pTexturesRef = GetImagePointer ( pMesh->pTextures[3].iImageID );
		pMesh->pTextures[3].pTexturesRefView = GetImagePointerView ( pMesh->pTextures[3].iImageID );
		pMesh->pTextures[3].dwBlendMode = GGTOP_MODULATE;
		pMesh->pTextures[3].dwBlendArg1 = GGTA_TEXTURE;
		pMesh->pTextures[4].iImageID = *piImageGlossIndex;
		pMesh->pTextures[4].pTexturesRef = GetImagePointer ( pMesh->pTextures[4].iImageID );
		pMesh->pTextures[4].pTexturesRefView = GetImagePointerView ( pMesh->pTextures[4].iImageID );
		pMesh->pTextures[4].dwBlendMode = GGTOP_MODULATE;
		pMesh->pTextures[4].dwBlendArg1 = GGTA_TEXTURE;
		pMesh->pTextures[5].iImageID = *piImageMaskIndex;
		pMesh->pTextures[5].pTexturesRef = GetImagePointer ( pMesh->pTextures[5].iImageID );
		pMesh->pTextures[5].pTexturesRefView = GetImagePointerView ( pMesh->pTextures[5].iImageID );
		pMesh->pTextures[5].dwBlendMode = GGTOP_MODULATE;
		pMesh->pTextures[5].dwBlendArg1 = GGTA_TEXTURE;
		pMesh->pTextures[7].iImageID = *piImageIlluminationIndex;
		pMesh->pTextures[7].pTexturesRef = GetImagePointer ( pMesh->pTextures[7].iImageID );
		pMesh->pTextures[7].pTexturesRefView = GetImagePointerView ( pMesh->pTextures[7].iImageID );
		pMesh->pTextures[7].dwBlendMode = GGTOP_MODULATE;
		pMesh->pTextures[7].dwBlendArg1 = GGTA_TEXTURE;

		// From old DBP legacy behavior
		// mode to blend texture and diffuse at stage zero (load object with material alpha setting)
		if ( iDBProMode==4 )
		{
			// Mode 4 - retains material settings and blends with texture
			pMesh->pTextures[0].dwBlendMode = GGTOP_MODULATE;
			pMesh->pTextures[0].dwBlendArg1 = GGTA_TEXTURE;
			pMesh->pTextures[0].dwBlendArg2 = GGTA_DIFFUSE;
		}
		else
		{
			// Mode 0,1,2,3 - to alter texture behaviour from basic color to texture
			if ( iDBProMode!=2 && iDBProMode!=3 )
			{
				// Force a MODULATE for default behaviours of [0] and [1]
				pMesh->pTextures[0].dwBlendMode = GGTOP_MODULATE;
				pMesh->pTextures[1].dwBlendMode = GGTOP_MODULATE;
				pMesh->pTextures[2].dwBlendMode = GGTOP_MODULATE;
				pMesh->pTextures[3].dwBlendMode = GGTOP_MODULATE;
				pMesh->pTextures[4].dwBlendMode = GGTOP_MODULATE;
				pMesh->pTextures[5].dwBlendMode = GGTOP_MODULATE;
				pMesh->pTextures[6].dwBlendMode = GGTOP_MODULATE;
				pMesh->pTextures[7].dwBlendMode = GGTOP_MODULATE;
			}

			// Only force this for [0] [1] and [3] where we are expecting texture results
			if ( iDBProMode!=2 ) 
			{
				pMesh->pTextures[0].dwBlendArg1 = GGTA_TEXTURE;
				pMesh->pTextures[1].dwBlendArg1 = GGTA_TEXTURE;
				pMesh->pTextures[2].dwBlendArg1 = GGTA_TEXTURE;
				pMesh->pTextures[3].dwBlendArg1 = GGTA_TEXTURE;
				pMesh->pTextures[4].dwBlendArg1 = GGTA_TEXTURE;
				pMesh->pTextures[5].dwBlendArg1 = GGTA_TEXTURE;
				pMesh->pTextures[6].dwBlendArg1 = GGTA_TEXTURE;
				pMesh->pTextures[7].dwBlendArg1 = GGTA_TEXTURE;
			}
		}
	}
	else
	{
		pMesh->pTextures[0].iImageID = 0;
		pMesh->pTextures[0].pTexturesRef = NULL;
	}
}

DARKSDK_DLL void LoadInternalTextures ( sObject* pObject, sMesh* pMesh, LPSTR TexturePath, int iDBProMode, int iDivideTextureSize, LPSTR pOptionalLightmapNoReduce )
{
	#ifdef WICKEDENGINE
	// leelee, simple for now to texture mesh with diffuse, eventually need mutlimaterial handling
	// and clean everything up so it is unified and still works nicely with Wicked
	if (pMesh->pTextures)
	{
		WickedCall_SetTexturePath(TexturePath);
		WickedCall_TextureObject(pObject,pMesh);
	}
	#else
	// iDBProMode : 0-DBV1 / 1-DBPro defaults / 2-leave all states alone (for internal textureloaduse)
	// 0-DBV1 legacy behaviour
	// 1-DBPro : out of the box new pro standard
	// 2-Leave states alone to keep material/diffuse effects
	// 3-Leave states alone to keep material/texture effects
	// 4-Ensure object blends texture and diffuse at stage zero
	// 5-Leave states alone to keep multi-material effects

	// load multimaterial textures from internal name
	if ( pMesh->bUseMultiMaterial==true )
	{
		// Define textures for multi material array
		DWORD dwMultiMatCount = pMesh->dwMultiMaterialCount;
		for ( DWORD m=0; m<dwMultiMatCount; m++ )
		{
			// get multimat at index
			sMultiMaterial* pMultiMat = &(pMesh->pMultiMaterial [ m ]);

			// load diffuse, normal, spec, gloss textures and assign to mesh
			int iImageDiffuseIndex = 0, iImageAOIndex = 0, iImageNormalIndex = 0, iImageSpecularIndex = 0, iImageGlossIndex = 0, iImageMaskIndex = 0, iImageIlluminationIndex = 0;
			LoadColorNormalSpecGloss ( pMesh, pMultiMat->pName, TexturePath, iDBProMode, iDivideTextureSize, &iImageDiffuseIndex, &iImageAOIndex, &iImageNormalIndex, &iImageSpecularIndex, &iImageGlossIndex, &iImageMaskIndex, &iImageIlluminationIndex );

			// store texture ref
			if ( iImageDiffuseIndex != 0 )
			{
				// maximum diffuse with texture (for DBV1 compatibility)
				if ( iDBProMode==0 )
				{
					// 191203 - added flag to LOAD OBJECT (DBV1 by default)
					pMultiMat->mMaterial.Diffuse.r = 1.0f;
					pMultiMat->mMaterial.Diffuse.g = 1.0f;
					pMultiMat->mMaterial.Diffuse.b = 1.0f;
					pMultiMat->mMaterial.Diffuse.a = 1.0f;
				}

				// and the texture itself
#ifdef WICKEDENGINE
				pMultiMat->pTexturesRef = GetImagePointerView ( iImageDiffuseIndex );
				// 090217 - support for normal, specular and gloss
				pMultiMat->pTexturesRefN = GetImagePointerView ( iImageNormalIndex );
				pMultiMat->pTexturesRefS = GetImagePointerView ( iImageSpecularIndex );
				pMultiMat->pTexturesRefG = GetImagePointerView ( iImageGlossIndex );
				pMultiMat->pTexturesRefM = GetImagePointerView ( iImageMaskIndex );
#endif
			}
			else
			{
				// no texture (uses diffuse colour for coloured model)
				pMultiMat->pTexturesRef = NULL;
				pMultiMat->pTexturesRefN = NULL;
				pMultiMat->pTexturesRefS = NULL;
				pMultiMat->pTexturesRefG = NULL;
				pMultiMat->pTexturesRefM = NULL;
			}

			// in order to let users use ambient, force this!
			if ( iDBProMode!=2 && iDBProMode!=3 && iDBProMode!=5 ) pMultiMat->mMaterial.Ambient = pMultiMat->mMaterial.Diffuse;
		}

		// lee - 040306 - u6rc5 - force this mode on load
		if ( iDBProMode==5 ) pMesh->bUsesMaterial = true;
	}
	else
	{
		// load standard textures for internal name
		int iImageDiffuseIndex = 0, iImageAOIndex = 0, iImageNormalIndex = 0, iImageSpecularIndex = 0, iImageGlossIndex = 0, iImageMaskIndex = 0, iImageIlluminationIndex = 0;
		DWORD dwTextureCount = pMesh->dwTextureCount;
		if ( dwTextureCount == 1 )
		{
			// get texture at index
			sTexture* pTexture = &(pMesh->pTextures [ 0 ]);

			// load diffuse, normal, spec, gloss textures and assign to mesh
			LoadColorNormalSpecGloss ( pMesh, pTexture->pName, TexturePath, iDBProMode, iDivideTextureSize, &iImageDiffuseIndex, &iImageAOIndex, &iImageNormalIndex, &iImageSpecularIndex, &iImageGlossIndex, &iImageMaskIndex, &iImageIlluminationIndex );
		}
		else
		{
			// mesh already has texture set specified, so direct load and assign them
			for ( DWORD t=0; t<dwTextureCount; t++ )
			{
				// get texture at index
				sTexture* pTexture = &(pMesh->pTextures [ t ]);

				// divide or not to divide
				int iLocalDivideValueForStage = iDivideTextureSize;
				if ( pOptionalLightmapNoReduce )
					if ( _strnicmp ( pTexture->pName, pOptionalLightmapNoReduce, strlen(pOptionalLightmapNoReduce))==NULL ) 
						iLocalDivideValueForStage=0;

				// load texture
				int iImageIndex = LoadOrFindTextureAsImage ( pTexture->pName, TexturePath, iLocalDivideValueForStage );

				// store image in texture
				if ( iImageIndex != 0 )
				{
					// setup texture settings
					pTexture->iImageID = iImageIndex;
					pTexture->pTexturesRef = GetImagePointer ( iImageIndex );
#ifdef WICKEDENGINE
					pTexture->pTexturesRefView = GetImagePointerView ( iImageIndex );
#endif

					// lee - 200306 - u6b4 - new mode to blend texture and diffuse at stage zero (load object with material alpha setting)
					if ( iDBProMode==4 )
					{
						// Mode 4 - retains material settings and blends with texture
						if ( t==0 )
						{
							pTexture->dwBlendMode = GGTOP_MODULATE;
							pTexture->dwBlendArg1 = GGTA_TEXTURE;
							pTexture->dwBlendArg2 = GGTA_DIFFUSE;
						}
					}
					else
					{
						// Mode 0,1,2,3
						// to alter texture behaviour from basic color to texture
						if ( iDBProMode!=2 && iDBProMode!=3 )
						{
							// Force a MODULATE for default behaviours of [0] and [1]
							pTexture->dwBlendMode = GGTOP_MODULATE;
						}

						// Only force this for [0] [1] and [3] where we are expecting texture results
						if ( iDBProMode!=2 ) pTexture->dwBlendArg1 = GGTA_TEXTURE;

						// maximum diffuse with texture (for DBV1 compatibility)
						if ( iDBProMode!=2 && iDBProMode!=3 )
						{
							pMesh->mMaterial.Diffuse.r = 1.0f;
							pMesh->mMaterial.Diffuse.g = 1.0f;
							pMesh->mMaterial.Diffuse.b = 1.0f;
							pMesh->mMaterial.Diffuse.a = 1.0f;
						}
					}
				}
				else
				{
					pTexture->iImageID = 0;
					pTexture->pTexturesRef = NULL;
				}
			}
		}

		// maximum diffuse with texture (for DBV1 compatibility)
		if ( iImageDiffuseIndex != 0 )
		{
			if ( iDBProMode!=4 )
			{
				if ( iDBProMode!=2 && iDBProMode!=3 )
				{
					pMesh->mMaterial.Diffuse.r = 1.0f;
					pMesh->mMaterial.Diffuse.g = 1.0f;
					pMesh->mMaterial.Diffuse.b = 1.0f;
					pMesh->mMaterial.Diffuse.a = 1.0f;
				}
			}
		}

		// 240203 - added more defaults for better backward compat. with Patch 3 and earlier
		if ( iDBProMode!=2 && iDBProMode!=3 && iDBProMode!=4 ) pMesh->mMaterial.Ambient = pMesh->mMaterial.Diffuse;
	}
	#endif
}

DARKSDK_DLL void LoadInternalTextures ( sObject* pObject, sMesh* pMesh, LPSTR TexturePath, int iDBProMode, int iDivideTextureSize )
{
	LoadInternalTextures ( pObject, pMesh, TexturePath, iDBProMode, iDivideTextureSize, 0 );
}

DARKSDK_DLL void LoadInternalTextures ( sObject* pObject, sMesh* pMesh, LPSTR TexturePath, int iDBProMode )
{
	LoadInternalTextures ( pObject, pMesh, TexturePath, iDBProMode, 0 );
}

DARKSDK_DLL void FreeInternalTextures ( sMesh* pMesh )
{
	if ( pMesh )
	{
		if ( pMesh->bUseMultiMaterial==true )
		{
			if ( pMesh->pTextures )
			{
				int iImageIndex = pMesh->pTextures->iImageID;
				if ( iImageIndex!=0 ) DeleteImage ( iImageIndex );
			}
		}
		else
		{
			DWORD dwTextureCount = pMesh->dwTextureCount;
			for ( DWORD t=0; t<dwTextureCount; t++ )
			{
				sTexture* pTexture = &(pMesh->pTextures [ t ]);
				if ( pTexture )
				{
					int iImageIndex = pTexture->iImageID;
					if ( iImageIndex!=0 )
					{
						// FPSC-RC5 - internal textures are NEGATIVE values only!
						if ( iImageIndex < 0 )
							DeleteImage ( iImageIndex );
					}
				}
			}
		}
	}
}

DARKSDK_DLL void CloneInternalTextures ( sMesh* pMeshDest, sMesh* pMeshSrc )
{
	// copy texture info from src to dest
	if ( pMeshDest && pMeshSrc )
	{
		// get texture lists
		sTexture* pTextureSrc = pMeshSrc->pTextures;
		sTexture* pTextureDest = pMeshDest->pTextures;

		// copy all texture stages data over to dest
		if ( pTextureSrc && pTextureDest && pMeshSrc->dwTextureCount>0 )
		{
			// 110406 - u6rc7 - if src bigger than dst, recreate dst so can use entire src texture data
			if ( pMeshDest->dwTextureCount < pMeshSrc->dwTextureCount )
			{
				// same size as src texture data
				pMeshDest->dwTextureCount = pMeshSrc->dwTextureCount;
				pTextureDest = new sTexture [ pMeshDest->dwTextureCount ];
				pMeshDest->pTextures = pTextureDest;
			}

			DWORD dwTextureCount = pMeshDest->dwTextureCount;
			memcpy ( pTextureDest, pTextureSrc, sizeof(sTexture)*dwTextureCount );
		}
	}
}

DARKSDK_DLL void CopyBaseMaterialToMultiMaterial ( sMesh* pMesh )
{
	// multi-material
	for ( DWORD dwMaterialIndex=0; dwMaterialIndex<pMesh->dwMultiMaterialCount; dwMaterialIndex++ )
		pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial = pMesh->mMaterial;
}

void CopyMeshSettings ( sMesh* pDestMesh, sMesh* pSrcMesh )
{
	// exit if a ptr null
	if ( !pDestMesh || !pSrcMesh )
		return;

	// copy states across
	pDestMesh->bAlphaOverride = pSrcMesh->bAlphaOverride;
	pDestMesh->bAmbient = pSrcMesh->bAmbient;
	pDestMesh->bCull = pSrcMesh->bCull;
	pDestMesh->bFog = pSrcMesh->bFog;
	pDestMesh->bProtectZWriteState = pSrcMesh->bProtectZWriteState;
	pDestMesh->bLight = pSrcMesh->bLight;
	pDestMesh->bOverridePixelShader = pSrcMesh->bOverridePixelShader;
	pDestMesh->bShaderBoneSkinning = pSrcMesh->bShaderBoneSkinning;
	pDestMesh->bTransparency = pSrcMesh->bTransparency;
	pDestMesh->bUseMultiMaterial = pSrcMesh->bUseMultiMaterial;
	pDestMesh->fSpecularOverride = pSrcMesh->fSpecularOverride;
	pDestMesh->bUsesMaterial = pSrcMesh->bUsesMaterial;
	pDestMesh->mMaterial = pSrcMesh->mMaterial;
	pDestMesh->bUseVertexShader = pSrcMesh->bUseVertexShader;
	pDestMesh->bVertexShaderEffectRefOnly = pSrcMesh->bVertexShaderEffectRefOnly;
	pDestMesh->bWireframe = pSrcMesh->bWireframe;
	pDestMesh->bZBiasActive = pSrcMesh->bZBiasActive;
	pDestMesh->bZRead = pSrcMesh->bZRead;
	pDestMesh->bZWrite = pSrcMesh->bZWrite;
	pDestMesh->dwAlphaOverride = pSrcMesh->dwAlphaOverride;
	pDestMesh->dwAlphaTestValue = pSrcMesh->dwAlphaTestValue;
	pDestMesh->dwMultiMaterialCount = pSrcMesh->dwMultiMaterialCount;
	pDestMesh->dwSubMeshListCount = pSrcMesh->dwSubMeshListCount;
	pDestMesh->dwThisTime = pSrcMesh->dwThisTime;
	pDestMesh->fMipMapLODBias = pSrcMesh->fMipMapLODBias;
	pDestMesh->fZBiasDepth = pSrcMesh->fZBiasDepth;
	pDestMesh->fZBiasSlopeScale = pSrcMesh->fZBiasSlopeScale;
	pDestMesh->iCastShadowIfStatic = pSrcMesh->iCastShadowIfStatic;
	pDestMesh->fBoostIntensity = pSrcMesh->fBoostIntensity;
	pDestMesh->iCurrentFrame = pSrcMesh->iCurrentFrame;
	pDestMesh->iSolidForVisibility = pSrcMesh->iSolidForVisibility;
	CopyBaseMaterialToMultiMaterial ( pDestMesh );
}

DARKSDK_DLL void SetBaseTexture ( sMesh* pMesh, int iStage, int iImage )
{
	// means DB=NO STAGE SPECIFIED
	if ( iStage==-1 )
	{
		// reset effects on object
		ClearTextureSettings( pMesh );
		iStage=0;
	}

	// Set base texture at correct stage
	SetBaseTextureStage ( pMesh, iStage, iImage );
}

DARKSDK_DLL void SetBaseTextureStage ( sMesh* pMesh, int iStage, int iImage )
{
	// when specify a stage, assume not to clear texture first
	if ( iStage==-1 ) iStage=0;

	// create texture array if not present
	g_pGlob->dwInternalFunctionCode=11012;
	if ( !EnsureTextureStageValid ( pMesh, iStage ) )
		return;

	// u64 - when set a new texture stage, also set the stage itself (otherwise it stays zero!)
	pMesh->pTextures [ iStage ].dwStage = iStage;

	// set texture stage zero (base texture)
	g_pGlob->dwInternalFunctionCode=11014;
	pMesh->pTextures [ iStage ].iImageID  = iImage;
	pMesh->pTextures [ iStage ].pTexturesRef = GetImagePointer ( iImage );
	pMesh->pTextures [ iStage ].pTexturesRefView = GetImagePointerView ( iImage );
	if ( pMesh->pTextures [ iStage ].pTexturesRef )
	{
		pMesh->pTextures [ iStage ].dwBlendMode = GGTOP_MODULATE;
		pMesh->pTextures [ iStage ].dwBlendArg1 = GGTA_TEXTURE;
		pMesh->pTextures [ iStage ].dwBlendArg2 = GGTA_DIFFUSE;
	}
	else
	{
		pMesh->pTextures [ iStage ].dwBlendMode = GGTOP_SELECTARG1;
		pMesh->pTextures [ iStage ].dwBlendArg1 = GGTA_DIFFUSE;
	}
	pMesh->pTextures [ iStage ].dwTexCoordMode = 0;

	// 250704 - add name if image holds it
	g_pGlob->dwInternalFunctionCode=11015;
	if ( iImage>0 )
	{
		LPSTR pImageFilename = GetImageName ( iImage );
		if ( pImageFilename )
			if ( strlen(pImageFilename) < MAX_STRING )
				strcpy ( pMesh->pTextures [ iStage ].pName, pImageFilename );
	}

	// clear material
	pMesh->bUsesMaterial = false;
	g_pGlob->dwInternalFunctionCode=11016;
}

DARKSDK_DLL void SetBaseTextureStageRef ( sMesh* pMesh, int iStage, LPGGSHADERRESOURCEVIEW pTextureRef )
{
	// force a texyture ref override (animation to object texture image ref)
	pMesh->pTextures [ iStage ].iImageID = -123;
	pMesh->pTextures [ iStage ].pTexturesRefView = pTextureRef;
}

DARKSDK_DLL void SetAlphaOverride ( sMesh* pMesh, float fPercentage )
{
	#ifdef WICKEDENGINE
	if (pMesh->fLastAlphaOverride == fPercentage) return;
	pMesh->fLastAlphaOverride = fPercentage;
	#endif

	if ( fPercentage<100.0f )
	{
		fPercentage/=100.0f;
		DWORD dwAlpha = (DWORD)(fPercentage*255);
		GGCOLOR dwAlphaValueOnly = GGCOLOR_ARGB ( dwAlpha, 0, 0, 0 );
		pMesh->dwAlphaOverride = dwAlphaValueOnly;
		pMesh->bAlphaOverride = true;
		CopyBaseMaterialToMultiMaterial(pMesh);
	}
	else
	{
		// leefix - 041105 - can switch off if 100 percent used
		pMesh->bAlphaOverride = false;
	}

	#ifdef WICKEDENGINE
	WickedCall_SetMeshMaterial(pMesh,false);
	#endif
}

DARKSDK_DLL void SetDiffuseMaterial	( sMesh* pMesh, DWORD dwRGB )
{
	// lee - 040306 - u6rc5 - apply changes to base material OR multi-material!
	pMesh->bUsesMaterial = true;
	if ( pMesh->dwMultiMaterialCount > 0 )
	{
		// multi-material
		for ( DWORD dwMaterialIndex=0; dwMaterialIndex<pMesh->dwMultiMaterialCount; dwMaterialIndex++ )
		{
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Diffuse.r = ((dwRGB & 0x00FF0000)>>16)/255.0f;
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Diffuse.g = ((dwRGB & 0x0000FF00)>>8)/255.0f;
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Diffuse.b = ((dwRGB & 0x000000FF)>>0)/255.0f;
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Diffuse.a = ((dwRGB & 0xFF000000)>>24)/255.0f;
		}
	}
	else
	{
		// base material
		pMesh->mMaterial.Diffuse.r = ((dwRGB & 0x00FF0000) >> 16) / 255.0f;
		pMesh->mMaterial.Diffuse.g = ((dwRGB & 0x0000FF00) >> 8) / 255.0f;
		pMesh->mMaterial.Diffuse.b = ((dwRGB & 0x000000FF) >> 0) / 255.0f;
		pMesh->mMaterial.Diffuse.a = ((dwRGB & 0xFF000000) >> 24) / 255.0f;

		// apply material changes
		#ifdef WICKEDENGINE
		WickedCall_SetMeshMaterial(pMesh,true);
		#endif
	}
}

DARKSDK_DLL void SetAmbienceMaterial ( sMesh* pMesh, DWORD dwRGB )
{
	// lee - 040306 - u6rc5 - apply changes to base material OR multi-material!
	pMesh->bUsesMaterial = true;
	if ( pMesh->dwMultiMaterialCount > 0 )
	{
		// multi-material
		for ( DWORD dwMaterialIndex=0; dwMaterialIndex<pMesh->dwMultiMaterialCount; dwMaterialIndex++ )
		{
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Ambient.r = ((dwRGB & 0x00FF0000)>>16)/255.0f;
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Ambient.g = ((dwRGB & 0x0000FF00)>>8)/255.0f;
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Ambient.b = ((dwRGB & 0x000000FF)>>0)/255.0f;
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Ambient.a = ((dwRGB & 0xFF000000)>>24)/255.0f;
		}
	}
	else
	{
		// base material
		pMesh->mMaterial.Ambient.r = ((dwRGB & 0x00FF0000)>>16)/255.0f;
		pMesh->mMaterial.Ambient.g = ((dwRGB & 0x0000FF00)>>8)/255.0f;
		pMesh->mMaterial.Ambient.b = ((dwRGB & 0x000000FF)>>0)/255.0f;
		pMesh->mMaterial.Ambient.a = ((dwRGB & 0xFF000000)>>24)/255.0f;
		//CopyBaseMaterialToMultiMaterial(pMesh);
	}
}

DARKSDK_DLL void SetSpecularMaterial ( sMesh* pMesh, DWORD dwRGB )
{
	// lee - 040306 - u6rc5 - apply changes to base material OR multi-material!
	pMesh->bUsesMaterial = true;
	if ( pMesh->dwMultiMaterialCount > 0 )
	{
		// multi-material
		for ( DWORD dwMaterialIndex=0; dwMaterialIndex<pMesh->dwMultiMaterialCount; dwMaterialIndex++ )
		{
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Specular.r = ((dwRGB & 0x00FF0000)>>16)/255.0f;
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Specular.g = ((dwRGB & 0x0000FF00)>>8)/255.0f;
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Specular.b = ((dwRGB & 0x000000FF)>>0)/255.0f;
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Specular.a = ((dwRGB & 0xFF000000)>>24)/255.0f;
		}
	}
	else
	{
		// base material
		pMesh->mMaterial.Specular.r = ((dwRGB & 0x00FF0000)>>16)/255.0f;
		pMesh->mMaterial.Specular.g = ((dwRGB & 0x0000FF00)>>8)/255.0f;
		pMesh->mMaterial.Specular.b = ((dwRGB & 0x000000FF)>>0)/255.0f;
		pMesh->mMaterial.Specular.a = ((dwRGB & 0xFF000000)>>24)/255.0f;
		// CopyBaseMaterialToMultiMaterial(pMesh);
	}
}

DARKSDK_DLL void SetEmissiveMaterial ( sMesh* pMesh, DWORD dwRGB )
{
	// lee - 040306 - u6rc5 - apply changes to base material OR multi-material!
	pMesh->bUsesMaterial = true;
	if ( pMesh->dwMultiMaterialCount > 0 )
	{
		// multi-material
		for ( DWORD dwMaterialIndex=0; dwMaterialIndex<pMesh->dwMultiMaterialCount; dwMaterialIndex++ )
		{
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Emissive.r = ((dwRGB & 0x00FF0000)>>16)/255.0f;
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Emissive.g = ((dwRGB & 0x0000FF00)>>8)/255.0f;
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Emissive.b = ((dwRGB & 0x000000FF)>>0)/255.0f;
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Emissive.a = ((dwRGB & 0xFF000000)>>24)/255.0f;
		}
		#ifdef WICKEDENGINE
		//PE: In wicked we also need to set the oerall emissive color.
		pMesh->mMaterial.Emissive.r = ((dwRGB & 0x00FF0000) >> 16) / 255.0f;
		pMesh->mMaterial.Emissive.g = ((dwRGB & 0x0000FF00) >> 8) / 255.0f;
		pMesh->mMaterial.Emissive.b = ((dwRGB & 0x000000FF) >> 0) / 255.0f;
		pMesh->mMaterial.Emissive.a = ((dwRGB & 0xFF000000) >> 24) / 255.0f;
		#endif
	}
	else
	{
		// base material
		pMesh->mMaterial.Emissive.r = ((dwRGB & 0x00FF0000)>>16)/255.0f;
		pMesh->mMaterial.Emissive.g = ((dwRGB & 0x0000FF00)>>8)/255.0f;
		pMesh->mMaterial.Emissive.b = ((dwRGB & 0x000000FF)>>0)/255.0f;
		pMesh->mMaterial.Emissive.a = ((dwRGB & 0xFF000000)>>24)/255.0f;

		// apply material changes
		#ifdef WICKEDENGINE
		WickedCall_SetMeshMaterial(pMesh, false);
		#endif
	}
}

DARKSDK_DLL void SetSpecularPower ( sMesh* pMesh, float fPower )
{
	/* not used in MAX
	// lee - 040306 - u6rc5 - apply changes to base material OR multi-material!
	pMesh->bUsesMaterial = true;
	if ( pMesh->dwMultiMaterialCount > 0 )
	{
		// multi-material
		for ( DWORD dwMaterialIndex=0; dwMaterialIndex<pMesh->dwMultiMaterialCount; dwMaterialIndex++ )
			pMesh->pMultiMaterial [ dwMaterialIndex ].mMaterial.Power = fPower;
	}
	else
	{
		// base material
		pMesh->mMaterial.Power = fPower;
		// CopyBaseMaterialToMultiMaterial(pMesh);
	}

	// lee - 281116 - newer shaders only requires a specular modulator
	pMesh->fSpecularOverride = fPower;
	*/
}

DARKSDK_DLL void RemoveTextureRefFromMesh  ( sMesh* pMesh, LPGGTEXTURE pTextureRef )
{
	DWORD dwTextureCount = pMesh->dwTextureCount;
	for ( DWORD t=0; t<dwTextureCount; t++ )
	{
		sTexture* pTexture = &(pMesh->pTextures [ t ]);
		if (pTexture && pTexture->pTexturesRef==pTextureRef )
		{
			// remove reference to texture
			pTexture->pTexturesRef = NULL;
		}
	}
}

DARKSDK_DLL void SetMultiTexture ( sMesh* pMesh, int iStage, DWORD dwBlendMode, DWORD dwTexCoordMode, int iImage )
{
	// create texture array if not present
	if ( !EnsureTextureStageValid ( pMesh, iStage ) )
		return;

	#ifdef DX11
	#else
	// free any previous cube map
	SAFE_RELEASE ( pMesh->pTextures [ iStage ].pCubeTexture );

	// set texture stage
	pMesh->pTextures [ iStage ].dwStage = iStage;
	pMesh->pTextures [ iStage ].iImageID  = iImage;
	pMesh->pTextures [ iStage ].pTexturesRef = GetImagePointer ( iImage );
	pMesh->pTextures [ iStage ].dwBlendMode = dwBlendMode;
	pMesh->pTextures [ iStage ].dwBlendArg1 = GGTA_TEXTURE;
	pMesh->pTextures [ iStage ].dwBlendArg2 = GGTA_CURRENT;
	pMesh->pTextures [ iStage ].dwTexCoordMode = dwTexCoordMode;
	#endif
}

DARKSDK_DLL void SetCubeTexture ( sMesh* pMesh, int iStage, LPGGCUBETEXTURE pCubeTexture )
{
	// create texture array if not present
	if ( !EnsureTextureStageValid ( pMesh, iStage ) )
		return;

	// free any previous cube map
	#ifdef DX11
	#else
	SAFE_RELEASE ( pMesh->pTextures [ iStage ].pCubeTexture );

	// set cube texture stage
	pMesh->pTextures [ iStage ].dwStage = iStage;
	pMesh->pTextures [ iStage ].iImageID  = 0;
	pMesh->pTextures [ iStage ].pTexturesRef = NULL;
	pMesh->pTextures [ iStage ].pCubeTexture = pCubeTexture;
	pMesh->pTextures [ iStage ].dwBlendMode = GGTOP_MODULATE;
	pMesh->pTextures [ iStage ].dwBlendArg1 = GGTA_TEXTURE;
	pMesh->pTextures [ iStage ].dwBlendArg2 = GGTA_CURRENT;
	pMesh->pTextures [ iStage ].dwTexCoordMode = 2;
	#endif
}

DARKSDK_DLL void SetBaseColor ( sMesh* pMesh, DWORD dwRGB )
{
	// reset effects on object
	ClearTextureSettings( pMesh );

	// additionally clear texture reference
	sTexture* pTexture = &pMesh->pTextures [ 0 ];
	pTexture->iImageID = 0;
	pTexture->pTexturesRef = NULL;
	pTexture->dwBlendMode = GGTOP_SELECTARG1;
	pTexture->dwBlendArg1 = GGTA_DIFFUSE;
	pTexture->dwBlendArg2 = GGTA_DIFFUSE;
	pTexture->dwTexCoordMode = 0;

	// Assign a colour to the material
	pMesh->bUsesMaterial = true;
	ResetMaterial ( &pMesh->mMaterial );
	ColorMaterial ( &pMesh->mMaterial, dwRGB );
}

// Mesh Shader Functions

DARKSDK_DLL bool InitEffectSystem ( sMesh* pMesh, DWORD dwTextureCount, cSpecialEffect* pEffectObj )
{
	// validate for shader
	// leefix - 010204 - some FX files do not require pixel shaders!
	if(!ValidateMeshForShader ( pMesh, dwTextureCount ))
		return false;

	// vertex shader effect
	pMesh->bUseVertexShader = true;
	pMesh->pVertexShaderEffect = pEffectObj;
	if ( pMesh->pVertexShaderEffect==NULL )
		return false;

	// pixel shader effect (completed by Setup) (redundant)
	pMesh->bOverridePixelShader = true;
	pMesh->pPixelShader = NULL;

	// lee - 230306 - u6b4 - ensure the mesh is unaltered by animation (so correct mesh conversion can happen)
	ResetVertexDataInMeshPerMesh ( pMesh );

	// complete
	return true;
}

// Mesh Effect Functions

DARKSDK_DLL bool SetSpecialEffect ( sMesh* pMesh, cSpecialEffect* pEffectObj, bool bChangeMesh )
{
	// On or Off
	if ( pEffectObj )
	{
		// lee - 300914 - early out if effect shader uses BONES but this mesh has NO BONES, do not use this shader!
		if ( pEffectObj->m_bUsesBoneData==TRUE && pMesh->dwBoneCount==0 )
		{
			// silent fail, simply leaves this mesh unaffected by shader
			return false;
		}

		// initialise FX effect
		if ( !InitEffectSystem ( pMesh, pMesh->dwTextureCount, pEffectObj ) )
			return false;

		// give all effects normals (if not got them)
		if ( pMesh->pVertexShaderEffect->m_bDoNotGenerateExtraData==0 )
		{
			// leeadd - 050906 - only auto-generate if not switched off
			pMesh->pVertexShaderEffect->m_bGenerateNormals = true;
		}

		// wipe out original mesh data as mesh may be changed now
		// 220214 - SAFE_DELETE_ARRAY ( pMesh->pOriginalVertexData );
		SAFE_DELETE_ARRAY ( pMesh->pOriginalVertexData );

		// prepare model for effect
		if ( bChangeMesh ) pMesh->pVertexShaderEffect->Mesh ( pMesh );

		// record effect name
		strcpy ( pMesh->pEffectName, pEffectObj->m_pEffectName );
	}
	else
	{
		// Delete any vertex shader being used
		FreeVertexShaderMesh ( pMesh );
	}

	// complete
	return true;
}

DARKSDK_DLL bool SetSpecialEffect ( sMesh* pMesh, cSpecialEffect* pEffectObj )
{
	return SetSpecialEffect ( pMesh, pEffectObj, true );
}

// Mesh Custom Vertex Shader Functions

DARKSDK_DLL void CombineSubsetPolygonsInMesh ( sMesh* pMesh )
{
}

DARKSDK_DLL void SetCustomShader ( sMesh* pMesh, LPGGVERTEXSHADER pVertexShader, LPGGVERTEXLAYOUT pVertexDec, DWORD dwStagesRequired )
{
	// reset effects on object
	ClearTextureSettings( pMesh );

	// validate for shader
	if(!ValidateMeshForShader ( pMesh, dwStagesRequired ))
		return;

	// vertex shader effect active
	pMesh->bUseVertexShader = true;
	pMesh->pVertexShader = pVertexShader;
	pMesh->pVertexDec = pVertexDec;

	// set texture stages for this shader
	for (DWORD dwIndex=0; dwIndex<dwStagesRequired; dwIndex++)
	{
		pMesh->pTextures [ dwIndex ].dwStage = dwIndex;
		pMesh->pTextures [ dwIndex ].dwBlendMode = GGTOP_MODULATE;
		pMesh->pTextures [ dwIndex ].dwBlendArg1 = GGTA_TEXTURE;
		pMesh->pTextures [ dwIndex ].dwTexCoordMode = 0;
		if ( dwIndex==0 )
			pMesh->pTextures [ dwIndex ].dwBlendArg2 = GGTA_DIFFUSE;
		else
			pMesh->pTextures [ dwIndex ].dwBlendArg2 = GGTA_CURRENT;
	}
}

DARKSDK_DLL void SetNoShader ( sMesh* pMesh )
{
	// vertex shader effect deactivate
	pMesh->bUseVertexShader = false;
	if ( pMesh->pVertexShader )
	{
		#ifdef DX11
		#else
		pMesh->pVertexShader->Release();
		#endif
		pMesh->pVertexShader=NULL;
	}
	if ( pMesh->pVertexDec )
	{
		#ifdef DX11
		#else
		pMesh->pVertexDec->Release();
		#endif
		pMesh->pVertexDec=NULL;
	}
}

// Mesh Custom Pixel Shader Functions

DARKSDK_DLL void SetCustomPixelShader ( sMesh* pMesh, LPGGPIXELSHADER pPixelShader )
{
	// pixel shader effect active
	pMesh->bOverridePixelShader = true;
	pMesh->pPixelShader = pPixelShader;
}

DARKSDK_DLL void SetNoPixelShader ( sMesh* pMesh )
{
	// pixel shader effect deactivate
	pMesh->bOverridePixelShader = false;
	pMesh->pPixelShader = NULL;
}

// Mesh Animation Functions

DARKSDK_DLL void VectorTransform ( const GGVECTOR3 in1, const GGMATRIX matrix, GGVECTOR3 &out )
{
	float in2 [ 3 ] [ 4 ];

	memcpy ( &in2, matrix, sizeof ( in2 ) );

	#define DotProduct( x, y ) ( ( x ) [ 0 ] * ( y ) [ 0 ] + ( x ) [ 1 ] * ( y ) [ 1 ] + ( x ) [ 2 ] * ( y ) [ 2 ] )

	out [ 0 ] = DotProduct ( in1, in2 [ 0 ] ) + in2 [ 0 ] [ 3 ];
	out [ 1 ] = DotProduct ( in1, in2 [ 1 ] ) +	in2 [ 1 ] [ 3 ];
	out [ 2 ] = DotProduct ( in1, in2 [ 2 ] ) +	in2 [ 2 ] [ 3 ];
}

DARKSDK_DLL void AnimateBoneMeshMDL ( sObject* pObject, sFrame* pFrame )
{
	// MIKE 240303 - MDL ANIMATION STYLE /////////////////////////////////////////////////////////
	int iFrame = ( int ) pObject->fAnimFrame;
	int iPos = 0;

	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
	{
		sMesh* pMesh = pObject->ppMeshList [ iMesh ];

		// first time around, copy vertex data
		if ( pMesh->pOriginalVertexData==NULL ) CollectOriginalVertexData ( pMesh );

		if ( !pMesh->pOriginalVertexData )
			return;
		// get the offset map for the vertex data
		sOffsetMap offsetMap;
		GetFVFOffsetMap ( pMesh, &offsetMap );

		int iVertexPosition = 0;
		
		for ( int iVertex = 0; iVertex < ( int ) pMesh->dwVertexCount; iVertex++ )
		{
			GGVECTOR3 vecInput = GGVECTOR3 (
													*( ( float* ) pMesh->pOriginalVertexData + offsetMap.dwX + ( offsetMap.dwSize * iVertex ) ),
													*( ( float* ) pMesh->pOriginalVertexData + offsetMap.dwY + ( offsetMap.dwSize * iVertex ) ),
													*( ( float* ) pMesh->pOriginalVertexData + offsetMap.dwZ + ( offsetMap.dwSize * iVertex ) )
											   );

			GGVECTOR3 vecOutput;

			VectorTransform ( 
								vecInput,
								pObject->pAnimationSet->pAnimation->ppBoneFrames
										[ iFrame ]
										[ pObject->pAnimationSet->pAnimation->piBoneOffsetList [ iPos++ ] ],
								vecOutput
						 );

			// copy data across
			*( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * iVertex ) ) = vecOutput.x;
			*( ( float* ) pMesh->pVertexData + offsetMap.dwY + ( offsetMap.dwSize * iVertex ) ) = vecOutput.y;
			*( ( float* ) pMesh->pVertexData + offsetMap.dwZ + ( offsetMap.dwSize * iVertex ) ) = vecOutput.z;
		}

		pMesh->bVBRefreshRequired = true;
#ifndef WICKEDENGINE
		g_vRefreshMeshList.push_back ( pMesh );
#endif
	}
}

DARKSDK_DLL void AnimateBoneMeshMDX ( sObject* pObject, sFrame* pFrame )
{
	// get mesh ptr
	sMesh* pMesh = pFrame->pMesh;
	if ( pFrame->pMesh->dwSubMeshListCount <= 1 )
		return;

	// and object animating
	if ( !pObject->bAnimPlaying && !pObject->bAnimUpdateOnce )
		return;

	// leeadd -240604 - u54 - MD2 only once update required
	pObject->bAnimUpdateOnce = false;

	/* leefix - 240604 - u54 - replaced with below new unified animation data
	// move to the next frame
	pFrame->pMesh->iNextFrame = ( pFrame->pMesh->iCurrentFrame + 1 ) % pFrame->pMesh->dwSubMeshListCount;

	// make sure anim frame is updated
	pObject->fAnimFrame = ( float ) pFrame->pMesh->iNextFrame;

	// interpolation code
	float fTime			  = ( float ) GetTickCount ( );						// get current time in milliseconds
	float fElapsedTime	  = fTime - pMesh->fLastInterp;						// get the elapsed time
	int   iAnimationSpeed = (int)(pObject->fAnimSpeed*10.0f);				// frame rate for animation
	float fInterp		  = fElapsedTime / ( 1000.0f / iAnimationSpeed );	// find out how far we are from the current frame to the next ( between 0 and 1 )

	// if our elapsed time goes over the desired time
	// segment start over and go to the next key frame
	if ( fElapsedTime >= ( 1000.0f / iAnimationSpeed ) )
	{
		// set current frame to the next frame
		pFrame->pMesh->iCurrentFrame = pFrame->pMesh->iNextFrame;

		// store the time
		pMesh->fLastInterp = fTime;
	}
	
	// get the offset map for the vertex data
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// store link to matrix
	pFrame->matOriginal = pMesh->pSubFrameList [ pFrame->pMesh->iNextFrame ].matOriginal;

	// make sure the frame numbers are valid
	if ( pFrame->pMesh->iNextFrame + 1 >= (int)pFrame->pMesh->dwSubMeshListCount )
	{
		// set next and current frame to 0
		pFrame->pMesh->iNextFrame    = 0;
		pFrame->pMesh->iCurrentFrame = 0;
	}
	*/

	// get the offset map for the vertex data
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// store the nearest frame of the animation data
	pFrame->pMesh->iNextFrame = (int)pObject->fAnimFrame;
	float fInterp = pObject->fAnimFrame - (float)pFrame->pMesh->iNextFrame;
	if ( pFrame->pMesh->iNextFrame + 1 >= (int)pFrame->pMesh->dwSubMeshListCount )
	{
		pFrame->pMesh->iNextFrame    = 0;
		fInterp = 0.0f;
	}

	// store link to matrix
	pFrame->matOriginal = pMesh->pSubFrameList [ pFrame->pMesh->iNextFrame ].matOriginal;

	// copy the vertices across
	for ( int iVertex = 0; iVertex < ( int ) pMesh->dwVertexCount; iVertex++ )
	{
		if ( offsetMap.dwZ>0 )
		{
			// find the frame
			sFrame*	pTheFrame     = pMesh->pSubFrameList;
			sFrame* pCurrentFrame = NULL;
			sFrame* pLastFrame    = NULL;
			sFrame* pNextFrame    = NULL;
			int     iFrame        = 0;

			GGVECTOR3 vecNextPos = GGVECTOR3 (
													*( ( float* ) pMesh->pSubFrameList [ pFrame->pMesh->iNextFrame + 1 ].pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * iVertex ) ),
													*( ( float* ) pMesh->pSubFrameList [ pFrame->pMesh->iNextFrame + 1 ].pMesh->pVertexData + offsetMap.dwY + ( offsetMap.dwSize * iVertex ) ),
													*( ( float* ) pMesh->pSubFrameList [ pFrame->pMesh->iNextFrame + 1 ].pMesh->pVertexData + offsetMap.dwZ + ( offsetMap.dwSize * iVertex ) )
												 );

			GGVECTOR3 vecLastPos = GGVECTOR3 (
													*( ( float* ) pMesh->pSubFrameList [ pFrame->pMesh->iNextFrame ].pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * iVertex ) ),
													*( ( float* ) pMesh->pSubFrameList [ pFrame->pMesh->iNextFrame ].pMesh->pVertexData + offsetMap.dwY + ( offsetMap.dwSize * iVertex ) ),
													*( ( float* ) pMesh->pSubFrameList [ pFrame->pMesh->iNextFrame ].pMesh->pVertexData + offsetMap.dwZ + ( offsetMap.dwSize * iVertex ) )
												 );

			GGVECTOR3 vecFinal = vecLastPos + ( vecNextPos - vecLastPos ) * fInterp;

			*( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * iVertex ) ) = vecFinal.x;
			*( ( float* ) pMesh->pVertexData + offsetMap.dwY + ( offsetMap.dwSize * iVertex ) ) = vecFinal.y;
			*( ( float* ) pMesh->pVertexData + offsetMap.dwZ + ( offsetMap.dwSize * iVertex ) ) = vecFinal.z;
		}
	}

	// trigger mesh to VB update
	pMesh->bVBRefreshRequired = true;
#ifndef WICKEDENGINE
	g_vRefreshMeshList.push_back ( pMesh );
#endif
}

void AnimateBoneMeshBONE ( sObject* pObject, sFrame* pFrame, sMesh* pMesh )
{
	// first time around, copy vertex data to original-store
	#ifndef NEVERSTOREORIGINALVERTICES
	//PE: pOriginalVertexData was allocated here.
	if ( pMesh->pOriginalVertexData==NULL )
	{
		// first time around, copy vertex data to original-store
		DWORD dwTotalVertSize = pMesh->dwVertexCount * pMesh->dwFVFSize;
		pMesh->pOriginalVertexData = (BYTE*)new char [ dwTotalVertSize ];
		memcpy ( pMesh->pOriginalVertexData, pMesh->pVertexData, dwTotalVertSize );
	}
	#endif

	// 010303 - new vertex blending for bones (to take advantage of multiple weights)
	GGMATRIX* matrices = new GGMATRIX [ pMesh->dwBoneCount ];
	BYTE* weighttable = new BYTE [ pMesh->dwVertexCount ];
	memset ( weighttable, 0, pMesh->dwVertexCount );

	// update all bone matrices
	for ( DWORD dwMatrixIndex = 0; dwMatrixIndex < pMesh->dwBoneCount; dwMatrixIndex++ )
	{
		if ( pMesh->pFrameMatrices [ dwMatrixIndex ] ) // lee - 180406 - u6rc10 - not all bones connect to animating frame
			GGMatrixMultiply ( &matrices [ dwMatrixIndex ], &pMesh->pBones [ dwMatrixIndex ].matTranslation, pMesh->pFrameMatrices [ dwMatrixIndex ] );
		else
			memcpy ( &matrices [ dwMatrixIndex ], &pMesh->pBones [ dwMatrixIndex ].matTranslation, sizeof(GGMATRIX) );
	}

	#ifdef PRODUCTCLASSIC
	//PE: This is needed in classic or animations like skeleton.dbo ... do not work.
	// run through all bones
	for ( int iBone = 0; iBone < ( int ) pMesh->dwBoneCount; iBone++ )
	{
		// go through all influenced bones
		for ( int iLoop = 0; iLoop < ( int ) pMesh->pBones [ iBone ].dwNumInfluences; iLoop++ )
		{
			// get the correct vertex and weight
			int iOffset = pMesh->pBones [ iBone ].pVertices [ iLoop ];
			float fWeight = pMesh->pBones [ iBone ].pWeights [ iLoop ];

			// Vertex Data Ptrs
			float* pDestVertexBase = (float*)(pMesh->pVertexData + ( pMesh->dwFVFSize * iOffset ));
			float* pVertexBase = (float*)(pMesh->pOriginalVertexData + ( pMesh->dwFVFSize * iOffset ));

			// clear new vertex if changing it
			if ( weighttable [ iOffset ]==0 )
			{
				memset ( pDestVertexBase, 0, 12 );
				if ( pMesh->dwFVF | GGFVF_NORMAL )
					memset ( pDestVertexBase+3, 0, 12 );
			}
			weighttable [ iOffset ] = 1;
			
			// get original vertex position
			GGVECTOR3 vec = GGVECTOR3 ( *(pVertexBase+0), *(pVertexBase+1), *(pVertexBase+2) );

			// multiply the vector and the bone matrix with weight
			GGVECTOR3 newVec = MultiplyVectorAndMatrix ( vec, matrices [ iBone ] ) * fWeight;
			
			// accumilate vertex for final result
			*(pDestVertexBase+0) += newVec.x;
			*(pDestVertexBase+1) += newVec.y;
			*(pDestVertexBase+2) += newVec.z;

			// transform and normalise the normals vector (if present)
			if ( pMesh->dwFVF | GGFVF_NORMAL )
			{
				GGVECTOR3 norm = GGVECTOR3 ( *(pVertexBase+3), *(pVertexBase+4), *(pVertexBase+5) );
				GGVECTOR3 newNorm;
				GGVec3TransformNormal ( &newNorm, &norm, &matrices [ iBone ] );
				newNorm = newNorm * fWeight;
					
				// update the normal with the new normal values
				*(pDestVertexBase+3) += newNorm.x;
				*(pDestVertexBase+4) += newNorm.y;
				*(pDestVertexBase+5) += newNorm.z;
			}
		}
	}
	#endif

	// free matrix bank
	SAFE_DELETE_ARRAY ( matrices );
	SAFE_DELETE_ARRAY ( weighttable );

	// trigger mesh to VB update
	#ifdef PRODUCTCLASSIC
	pMesh->bVBRefreshRequired = true;
	g_vRefreshMeshList.push_back ( pMesh );
	#endif
}

DARKSDK_DLL void AnimateBoneMeshBONE ( sObject* pObject, sFrame* pFrame )
{
	AnimateBoneMeshBONE ( pObject, pFrame, pFrame->pMesh );
}

bool AnimateBoneMesh ( sObject* pObject, sFrame* pFrame, sMesh* pMesh )
{
	// check if mesh valid
	if ( !pMesh ) return true;

	// see if we need to deal with bones
	if ( pMesh->dwBoneCount )
	{
		// leefix - 211103 - Can reject right away if frame unchanged
		// leefix - 101203 - Fixed the logic here
		bool bQuickAccept = false;
		if ( pObject->fAnimFrame != pObject->fAnimLastFrame )
			bQuickAccept = true;

		// leefix - 140504 - once only update passes
		if ( pObject->bAnimUpdateOnce )
			bQuickAccept = true;

		// leeadd - 120204 - additional accept if slerp changed
		if ( pObject->bAnimManualSlerp==true )
			if ( pObject->fAnimSlerpTime != pObject->fAnimSlerpLastTime )
				bQuickAccept = true;

		// Quick reject check
		if ( bQuickAccept )
		{
			// check if MDL
			if ( pObject->pAnimationSet )
			{
				if ( pObject->pAnimationSet->pAnimation )
				{
					if ( !pObject->pAnimationSet->pAnimation->bBoneType )
					{
						// Animate model as MDL animation
						AnimateBoneMeshMDL ( pObject, pFrame );

						// complete early
						return true;
					}
				}
			}

			// failing MDL, Animate model as BONE animation (if not done in HW)
			if ( (pMesh->bShaderBoneSkinning==false && pMesh->dwForceCPUAnimationMode!=3) || pMesh->dwForceCPUAnimationMode==1 )
				AnimateBoneMeshBONE ( pObject, pFrame, pMesh );
		}
	}
	else
	{
		// Animate model as MDX animation
		AnimateBoneMeshMDX ( pObject, pFrame );
	}

	// return always animated
	return true;
}

DARKSDK_DLL bool AnimateBoneMesh ( sObject* pObject, sFrame* pFrame )
{
	// normal object-mesh bone animation
	return AnimateBoneMesh ( pObject, pFrame, pFrame->pMesh );
}

DARKSDK_DLL void ResetVertexDataInMeshPerMesh ( sMesh* pMesh )
{
	// valid mesh
	if ( !pMesh )
		return;

	// if original data available and same size, restore with it
	if ( pMesh->pOriginalVertexData )
	{
		DWORD dwTotalVertSize = pMesh->dwVertexCount * pMesh->dwFVFSize;
		memcpy ( pMesh->pVertexData, pMesh->pOriginalVertexData, dwTotalVertSize );
		pMesh->bVBRefreshRequired = true;
#ifndef WICKEDENGINE
		g_vRefreshMeshList.push_back ( pMesh );
#endif
	}
}

DARKSDK_DLL void CollectOriginalVertexData ( sMesh* pMesh )
{
	#ifndef NEVERSTOREORIGINALVERTICES
	if ( pMesh->pOriginalVertexData==NULL )
	{
		// first time around, copy vertex data to original-store
		DWORD dwTotalVertSize = pMesh->dwVertexCount * pMesh->dwFVFSize;
		pMesh->pOriginalVertexData = (BYTE*)new char [ dwTotalVertSize ];
		memcpy ( pMesh->pOriginalVertexData, pMesh->pVertexData, dwTotalVertSize );
	}
	#endif
}

DARKSDK_DLL void ResetVertexDataInMesh ( sObject* pObject )
{
	#ifdef WICKEDENGINE
	// Not necessary in wicked engine
	#else
	// valid object
	if ( !pObject )
		return;

	// go through all meshes of object
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
	{
		// get mesh ptr
		sMesh* pMesh = pObject->ppMeshList [ iMesh ];
		ResetVertexDataInMeshPerMesh ( pMesh );
	}
	#endif
}

DARKSDK_DLL void UpdateVertexDataInMesh ( sObject* pObject )
{
	// valid object
	if ( !pObject )
		return;

	// go through all meshes of object
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
	{
		// get mesh ptr
		sMesh* pMesh = pObject->ppMeshList [ iMesh ];

		// delete old data and re-capture latest vertex data (new UV data changes mostly)
		SAFE_DELETE_ARRAY ( pMesh->pOriginalVertexData );
		CollectOriginalVertexData ( pMesh );
	}
}

// Mesh Shadow Meshes

//--------------------------------------------------------------------------------------
// Takes an array of CEdgeMapping objects, then returns an index for the edge in the
// table if such entry exists, or returns an index at which a new entry for the edge
// can be written.
// nV1 and nV2 are the vertex indexes for the old edge.
// nCount is the number of elements in the array.
// The function returns -1 if an available entry cannot be found.  In reality,
// this should never happens as we should have allocated enough memory.
int FindEdgeInMappingTable( int nV1, int nV2, CEdgeMapping *pMapping, int nCount )
{
    for( int i = 0; i < nCount; ++i )
    {
        // If both vertex indexes of the old edge in mapping entry are -1, then
        // we have searched every valid entry without finding a match.  Return
        // this index as a newly created entry.
        if( ( pMapping[i].m_anOldEdge[0] == -1 && pMapping[i].m_anOldEdge[1] == -1 ) ||

            // Or if we find a match, return the index.
            ( pMapping[i].m_anOldEdge[1] == nV1 && pMapping[i].m_anOldEdge[0] == nV2 ) )
        {
            return i;
        }
    }

    return -1;  // We should never reach this line
}

// Mesh Construction Functions

DARKSDK_DLL bool MakeMeshPlain ( bool bCreateNew, sMesh* pMesh, float fWidth, float fHeight, DWORD dwFVF, DWORD dwColor )
{
	// create memory
	DWORD dwVertexCount = 6;									// store number of vertices
	DWORD dwIndexCount  = 0;									// store number of indices
	if ( !SetupMeshFVFData ( pMesh, GGFVF_XYZ | GGFVF_NORMAL | GGFVF_TEX1, dwVertexCount, dwIndexCount, false ) )
	{
		RunTimeError ( RUNTIMEERROR_B3DMESHLOADFAILED );
		return false;
	}
	
	// leefix-210703-corrected UV coords of plain ( they where -1.0f, etc?? )
	// and DBPro rotates it Y=180 so it faces the camera (for compatibility and correct plain)
	SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData,  0,  -fWidth,  fHeight, 0.0f,  0.0f,  0.0f,  1.0f, GGCOLOR_ARGB ( 255, 255, 255, 255 ), 1.0f, 0.0f );
	SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData,  1,   fWidth,  fHeight, 0.0f,  0.0f,  0.0f,  1.0f, GGCOLOR_ARGB ( 255, 255, 255, 255 ), 0.0f, 0.0f );
	SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData,  2,  -fWidth, -fHeight, 0.0f,  0.0f,  0.0f,  1.0f, GGCOLOR_ARGB ( 255, 255, 255, 255 ), 1.0f, 1.0f );
	SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData,  3,   fWidth,  fHeight, 0.0f,  0.0f,  0.0f,  1.0f, GGCOLOR_ARGB ( 255, 255, 255, 255 ), 0.0f, 0.0f );
	SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData,  4,   fWidth, -fHeight, 0.0f,  0.0f,  0.0f,  1.0f, GGCOLOR_ARGB ( 255, 255, 255, 255 ), 0.0f, 1.0f );
	SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData,  5,  -fWidth, -fHeight, 0.0f,  0.0f,  0.0f,  1.0f, GGCOLOR_ARGB ( 255, 255, 255, 255 ), 1.0f, 1.0f );

	// setup mesh drawing properties
	pMesh->iPrimitiveType   = GGPT_TRIANGLELIST;
	pMesh->iDrawVertexCount = pMesh->dwVertexCount;
	pMesh->iDrawPrimitives  = 2;

	// okay
	return true;
}

// mike - 021005 - alternative make plane function
DARKSDK_DLL bool MakeMeshPlainEx ( bool bCreateNew, sMesh* pMesh, float fWidth, float fHeight, DWORD dwFVF, DWORD dwColor )
{
	// create memory
	DWORD dwVertexCount = 6;									// store number of vertices
	DWORD dwIndexCount  = 0;									// store number of indices
	if ( !SetupMeshFVFData ( pMesh, GGFVF_XYZ | GGFVF_NORMAL | GGFVF_TEX1, dwVertexCount, dwIndexCount, false ) )
	{
		RunTimeError ( RUNTIMEERROR_B3DMESHLOADFAILED );
		return false;
	}

	float pos_pos_x = ( fWidth );
	float neg_pos_x = 0 - ( fWidth );
	float pos_pos_y = ( fHeight );
	float neg_pos_y = 0 - ( fHeight );
	float normal_f = -1.0f;
	
	#ifdef WICKEDENGINE

	//PE: @Lee Why is this needed ? , particles ... display images inverted.
	// other way for wicked engine
	//pos_pos_x = 0 - ( fWidth );
	//neg_pos_x = ( fWidth );
	//normal_f = 1.0f;
	#endif
	
	//PE: @Lee it do not match Classic , vertex 0,2,5 has -fWidth , so we need to change UV on all the old classic functions ?
	//PE: Not sure why it was made this way ? , i will change classic to use MakeMeshPlain so we can keep this (if it need to be different).

	SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData, 0, neg_pos_x, pos_pos_y, 0.0f, 0.0f, 0.0f, normal_f, GGCOLOR_ARGB ( 255, 255, 255, 255 ), 0.0f, 0.0f );
	SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData, 1, pos_pos_x, pos_pos_y, 0.0f, 0.0f, 0.0f, normal_f, GGCOLOR_ARGB ( 255, 255, 255, 255 ), 1.0f, 0.0f );
	SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData, 2, pos_pos_x, neg_pos_y, 0.0f, 0.0f, 0.0f, normal_f, GGCOLOR_ARGB ( 255, 255, 255, 255 ), 1.0f, 1.0f );
	SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData, 3, pos_pos_x, neg_pos_y, 0.0f, 0.0f, 0.0f, normal_f, GGCOLOR_ARGB ( 255, 255, 255, 255 ), 1.0f, 1.0f );
	SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData, 4, neg_pos_x, neg_pos_y, 0.0f, 0.0f, 0.0f, normal_f, GGCOLOR_ARGB ( 255, 255, 255, 255 ), 0.0f, 1.0f );
	SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData, 5, neg_pos_x, pos_pos_y, 0.0f, 0.0f, 0.0f, normal_f, GGCOLOR_ARGB ( 255, 255, 255, 255 ), 0.0f, 0.0f );

	// setup mesh drawing properties
	pMesh->iPrimitiveType   = GGPT_TRIANGLELIST;
	pMesh->iDrawVertexCount = pMesh->dwVertexCount;
	pMesh->iDrawPrimitives  = 2;

	// okay
	return true;
}

DARKSDK_DLL bool MakeMeshBox ( bool bCreateNew, sMesh* pMesh, float fWidth1, float fHeight1, float fDepth1, float fWidth2, float fHeight2, float fDepth2, DWORD dwFVF, DWORD dwColor )
{
	// create vertex memory for box
	DWORD dwVertexCount = 24;
	DWORD dwIndexCount  = 36;
	if ( bCreateNew )
	{
		if ( !SetupMeshFVFData ( pMesh, dwFVF, dwVertexCount, dwIndexCount, false ) )
		{
			RunTimeError ( RUNTIMEERROR_B3DMESHLOADFAILED );
			return false;
		}
	}

	// leefix - 080403 - adjusted UV data use accurate coordinates (slight texture alignment can now be done usinfg vertexdata commands)
	// setup the vertices, we're using a standard FVF here so call the standard utility function
	SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData,  0, fWidth1, fHeight2, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, 0.00f, 0.00f );	// front
	SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData,  1, fWidth2, fHeight2, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, 1.00f, 0.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData,  2, fWidth2, fHeight1, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, 1.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData,  3, fWidth1, fHeight1, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, 0.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData,  4, fWidth1, fHeight2, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, 1.00f, 0.00f );	// back
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData,  5, fWidth1, fHeight1, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, 1.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData,  6, fWidth2, fHeight1, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, 0.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData,  7, fWidth2, fHeight2, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, 0.00f, 0.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData,  8, fWidth1, fHeight2, fDepth2,	 0.0f,  1.0f,  0.0f, dwColor, 0.00f, 0.00f );	// top
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData,  9, fWidth2, fHeight2, fDepth2,	 0.0f,  1.0f,  0.0f, dwColor, 1.00f, 0.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 10, fWidth2, fHeight2, fDepth1,	 0.0f,  1.0f,  0.0f, dwColor, 1.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 11, fWidth1, fHeight2, fDepth1,	 0.0f,  1.0f,  0.0f, dwColor, 0.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 12, fWidth1, fHeight1, fDepth2,  0.0f, -1.0f,  0.0f, dwColor, 0.00f, 1.00f );	// bottom
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 13, fWidth1, fHeight1, fDepth1,	 0.0f, -1.0f,  0.0f, dwColor, 0.00f, 0.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 14, fWidth2, fHeight1, fDepth1,	 0.0f, -1.0f,  0.0f, dwColor, 1.00f, 0.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 15, fWidth2, fHeight1, fDepth2,	 0.0f, -1.0f,  0.0f, dwColor, 1.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 16, fWidth2, fHeight2, fDepth1,	 1.0f,  0.0f,  0.0f, dwColor, 0.00f, 0.00f );	// right
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 17, fWidth2, fHeight2, fDepth2,	 1.0f,  0.0f,  0.0f, dwColor, 1.00f, 0.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 18, fWidth2, fHeight1, fDepth2,	 1.0f,  0.0f,  0.0f, dwColor, 1.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 19, fWidth2, fHeight1, fDepth1,	 1.0f,  0.0f,  0.0f, dwColor, 0.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 20, fWidth1, fHeight2, fDepth1,	-1.0f,  0.0f,  0.0f, dwColor, 1.00f, 0.00f );	// left
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 21, fWidth1, fHeight1, fDepth1,	-1.0f,  0.0f,  0.0f, dwColor, 1.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 22, fWidth1, fHeight1, fDepth2,	-1.0f,  0.0f,  0.0f, dwColor, 0.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 23, fWidth1, fHeight2, fDepth2,	-1.0f,  0.0f,  0.0f, dwColor, 0.00f, 0.00f );
	
	// and now fill in the index list
	pMesh->pIndices [  0 ] =  0;		pMesh->pIndices [  1 ] =  1;		pMesh->pIndices [  2 ] =  2;
	pMesh->pIndices [  3 ] =  2;		pMesh->pIndices [  4 ] =  3;		pMesh->pIndices [  5 ] =  0;
	pMesh->pIndices [  6 ] =  4;		pMesh->pIndices [  7 ] =  5;		pMesh->pIndices [  8 ] =  6;
	pMesh->pIndices [  9 ] =  6;		pMesh->pIndices [ 10 ] =  7;		pMesh->pIndices [ 11 ] =  4;
	pMesh->pIndices [ 12 ] =  8;		pMesh->pIndices [ 13 ] =  9;		pMesh->pIndices [ 14 ] = 10;
	pMesh->pIndices [ 15 ] = 10;		pMesh->pIndices [ 16 ] = 11;		pMesh->pIndices [ 17 ] =  8;
	pMesh->pIndices [ 18 ] = 12;		pMesh->pIndices [ 19 ] = 13;		pMesh->pIndices [ 20 ] = 14;
	pMesh->pIndices [ 21 ] = 14;		pMesh->pIndices [ 22 ] = 15;		pMesh->pIndices [ 23 ] = 12;
	pMesh->pIndices [ 24 ] = 16;		pMesh->pIndices [ 25 ] = 17;		pMesh->pIndices [ 26 ] = 18;
	pMesh->pIndices [ 27 ] = 18;		pMesh->pIndices [ 28 ] = 19;		pMesh->pIndices [ 29 ] = 16;
	pMesh->pIndices [ 30 ] = 20;		pMesh->pIndices [ 31 ] = 21;		pMesh->pIndices [ 32 ] = 22;
	pMesh->pIndices [ 33 ] = 22;		pMesh->pIndices [ 34 ] = 23;		pMesh->pIndices [ 35 ] = 20;
	
	// setup mesh drawing properties
	pMesh->iPrimitiveType   = GGPT_TRIANGLELIST;
	pMesh->iDrawVertexCount = pMesh->dwVertexCount;
	pMesh->iDrawPrimitives  = pMesh->dwIndexCount  / 3;

	// okay
	return true;
}

DARKSDK_DLL bool MakeMeshPyramid ( bool bCreateNew, sMesh* pMesh, float fSize, DWORD dwFVF, DWORD dwColor )
{
	// create vertex memory for box
	DWORD dwVertexCount = 24;
	DWORD dwIndexCount  = 36;
	if ( bCreateNew )
	{
		if ( !SetupMeshFVFData ( pMesh, dwFVF, dwVertexCount, dwIndexCount, false ) )
		{
			RunTimeError ( RUNTIMEERROR_B3DMESHLOADFAILED );
			return false;
		}
	}

	// calculate some dimensions
	float fWidth1 = -fSize;
	float fWidth2 = fSize;
	float fHeight1 = -fSize;
	float fHeight2 = fSize;
	float fDepth1 = -fSize;
	float fDepth2 = fSize;

	// generate vrtex data
	SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData,  0, 0, fHeight2, 0,  0.0f,  0.0f, -1.0f, dwColor, 0.00f, 0.00f );	// front
	SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData,  1, 0, fHeight2, 0,  0.0f,  0.0f, -1.0f, dwColor, 1.00f, 0.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData,  2, fWidth2, fHeight1, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, 1.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData,  3, fWidth1, fHeight1, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, 0.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData,  4, 0, fHeight2, 0,  0.0f,  0.0f,  1.0f, dwColor, 1.00f, 0.00f );	// back
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData,  5, fWidth1, fHeight1, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, 1.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData,  6, fWidth2, fHeight1, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, 0.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData,  7, 0, fHeight2, 0,  0.0f,  0.0f,  1.0f, dwColor, 0.00f, 0.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData,  8, 0, fHeight2, 0,	 0.0f,  1.0f,  0.0f, dwColor, 0.00f, 0.00f );	// top
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData,  9, 0, fHeight2, 0,	 0.0f,  1.0f,  0.0f, dwColor, 1.00f, 0.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 10, 0, fHeight2, 0,	 0.0f,  1.0f,  0.0f, dwColor, 1.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 11, 0, fHeight2, 0,	 0.0f,  1.0f,  0.0f, dwColor, 0.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 12, fWidth1, fHeight1, fDepth2,  0.0f, -1.0f,  0.0f, dwColor, 0.00f, 1.00f );	// bottom
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 13, fWidth1, fHeight1, fDepth1,	 0.0f, -1.0f,  0.0f, dwColor, 0.00f, 0.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 14, fWidth2, fHeight1, fDepth1,	 0.0f, -1.0f,  0.0f, dwColor, 1.00f, 0.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 15, fWidth2, fHeight1, fDepth2,	 0.0f, -1.0f,  0.0f, dwColor, 1.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 16, 0, fHeight2, 0,	 1.0f,  0.0f,  0.0f, dwColor, 0.00f, 0.00f );	// right
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 17, 0, fHeight2, 0,	 1.0f,  0.0f,  0.0f, dwColor, 1.00f, 0.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 18, fWidth2, fHeight1, fDepth2,	 1.0f,  0.0f,  0.0f, dwColor, 1.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 19, fWidth2, fHeight1, fDepth1,	 1.0f,  0.0f,  0.0f, dwColor, 0.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 20, 0, fHeight2, 0,	-1.0f,  0.0f,  0.0f, dwColor, 1.00f, 0.00f );	// left
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 21, fWidth1, fHeight1, fDepth1,	-1.0f,  0.0f,  0.0f, dwColor, 1.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 22, fWidth1, fHeight1, fDepth2,	-1.0f,  0.0f,  0.0f, dwColor, 0.00f, 1.00f );
	SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, 23, 0, fHeight2, 0,	-1.0f,  0.0f,  0.0f, dwColor, 0.00f, 0.00f );
	
	// and now fill in the index list
	pMesh->pIndices [  0 ] =  0;		pMesh->pIndices [  1 ] =  1;		pMesh->pIndices [  2 ] =  2;
	pMesh->pIndices [  3 ] =  2;		pMesh->pIndices [  4 ] =  3;		pMesh->pIndices [  5 ] =  0;
	pMesh->pIndices [  6 ] =  4;		pMesh->pIndices [  7 ] =  5;		pMesh->pIndices [  8 ] =  6;
	pMesh->pIndices [  9 ] =  6;		pMesh->pIndices [ 10 ] =  7;		pMesh->pIndices [ 11 ] =  4;
	pMesh->pIndices [ 12 ] =  8;		pMesh->pIndices [ 13 ] =  9;		pMesh->pIndices [ 14 ] = 10;
	pMesh->pIndices [ 15 ] = 10;		pMesh->pIndices [ 16 ] = 11;		pMesh->pIndices [ 17 ] =  8;
	pMesh->pIndices [ 18 ] = 12;		pMesh->pIndices [ 19 ] = 13;		pMesh->pIndices [ 20 ] = 14;
	pMesh->pIndices [ 21 ] = 14;		pMesh->pIndices [ 22 ] = 15;		pMesh->pIndices [ 23 ] = 12;
	pMesh->pIndices [ 24 ] = 16;		pMesh->pIndices [ 25 ] = 17;		pMesh->pIndices [ 26 ] = 18;
	pMesh->pIndices [ 27 ] = 18;		pMesh->pIndices [ 28 ] = 19;		pMesh->pIndices [ 29 ] = 16;
	pMesh->pIndices [ 30 ] = 20;		pMesh->pIndices [ 31 ] = 21;		pMesh->pIndices [ 32 ] = 22;
	pMesh->pIndices [ 33 ] = 22;		pMesh->pIndices [ 34 ] = 23;		pMesh->pIndices [ 35 ] = 20;
	
	// setup mesh drawing properties
	pMesh->iPrimitiveType   = GGPT_TRIANGLELIST;
	pMesh->iDrawVertexCount = pMesh->dwVertexCount;
	pMesh->iDrawPrimitives  = pMesh->dwIndexCount  / 3;

	// okay
	return true;
}

DARKSDK_DLL bool MakeMeshSphere ( bool bCreateNew, sMesh* pMesh, GGVECTOR3 vecCentre, float fRadius, int iRings, int iSegments, DWORD dwFVF, DWORD dwColor )
{
	// create sphere mesh
	DWORD dwIndexCount		= 2 * iRings * ( iSegments + 1 );
	DWORD dwVertexCount		= ( iRings + 1 ) * ( iSegments + 1 );
	if ( bCreateNew )
	{
		if ( !SetupMeshFVFData ( pMesh, dwFVF, dwVertexCount, dwIndexCount, false ) )
		{
			RunTimeError ( RUNTIMEERROR_B3DMEMORYERROR );
			return false;
		}
	}

	// now we can fill in the vertex and index data to form the mesh
	float		fDeltaRingAngle		= ( GG_PI / iRings );
	float		fDeltaSegAngle		= ( 2.0f * GG_PI / iSegments );
	int			iVertex				= 0;
	int			iIndex				= 0;
	WORD		wVertexIndex		= 0;
	GGVECTOR3 vNormal;

	// generate the group of rings for the sphere
	for ( int iCurrentRing = 0; iCurrentRing < iRings + 1; iCurrentRing++ )
	{
		float r0 = sinf ( iCurrentRing * fDeltaRingAngle );
		float y0 = cosf ( iCurrentRing * fDeltaRingAngle );

		// generate the group of segments for the current ring
		for ( int iCurrentSegment = 0; iCurrentSegment < iSegments + 1; iCurrentSegment++ )
		{
			float x0 = r0 * sinf ( iCurrentSegment * fDeltaSegAngle );
			float z0 = r0 * cosf ( iCurrentSegment * fDeltaSegAngle );

			vNormal.x = x0;
			vNormal.y = y0;
			vNormal.z = z0;
	
			GGVec3Normalize ( &vNormal, &vNormal );

			// add one vertex to the strip which makes up the sphere
			SetupStandardVertex (	pMesh->dwFVF, pMesh->pVertexData,  iVertex,
									vecCentre.x+(x0*fRadius), vecCentre.y+(y0*fRadius), vecCentre.z+(z0*fRadius), 
									vNormal.x, vNormal.y, vNormal.z,
									dwColor,
									1.0f - ( ( float ) iCurrentSegment / ( float ) iSegments ),
									( float ) iCurrentRing / ( float ) iRings );

			// increment vertex
			iVertex++;
			
			// add two indices except for the last ring 
			if ( iCurrentRing != iRings )
			{
				pMesh->pIndices [ iIndex ] = wVertexIndex;
				iIndex++;
				
				pMesh->pIndices [ iIndex ] = wVertexIndex + ( WORD ) ( iSegments + 1 ); 
				iIndex++;
				
				wVertexIndex++; 
			}
		}
	}

	// setup mesh drawing properties
	pMesh->iPrimitiveType   = GGPT_TRIANGLESTRIP;
	pMesh->iDrawVertexCount = pMesh->dwVertexCount;
	pMesh->iDrawPrimitives  = pMesh->dwIndexCount - 2;

	// Wicked engine requires only triangle lists
	#ifdef WICKEDENGINE
	 ConvertLocalMeshToTriList(pMesh);
	#endif

	// okay
	return true;
}

DARKSDK_DLL bool MakeMeshFromOtherMesh ( bool bCreateNew, sMesh* pMesh, sMesh* pOtherMesh, GGMATRIX* pmatWorld, DWORD dwIndexCount, DWORD dwVertexCount )
{
	// make new mesh from existing other mesh
	MakeLocalMeshFromOtherLocalMesh ( pMesh, pOtherMesh, dwIndexCount, dwVertexCount );

	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// copy vertex data from mesh to single-mesh
	BYTE* pDestVertexData = NULL;
	BYTE* pDestNormalData = NULL;

	// make sure we have data and ptrs
	if ( offsetMap.dwZ>0 )
	{
		pDestVertexData = (BYTE*)((float*)(pMesh->pVertexData));
	}
	if ( offsetMap.dwNZ>0 )
	{
		pDestNormalData = (BYTE*)((float*)pMesh->pVertexData + offsetMap.dwNX);
	}

	// establish world matrix if any
	GGMATRIX matNoTransWorld;
	if ( *pmatWorld )
		matNoTransWorld = *pmatWorld;
	else
		GGMatrixIdentity ( &matNoTransWorld );

	// remove all translation from world matrix
	matNoTransWorld._41 = 0.0f;
	matNoTransWorld._42 = 0.0f;
	matNoTransWorld._43 = 0.0f;

	// transform vertex data by world matrix of frame
	for ( DWORD v=0; v<pOtherMesh->dwVertexCount; v++ )
	{
		// handle vertex data
		if ( pDestVertexData )
		{
			GGVECTOR3* pVertex = (GGVECTOR3*)(pDestVertexData+(v*pMesh->dwFVFSize));
			GGVec3TransformCoord ( pVertex, pVertex, &matNoTransWorld );
		}

		// handle normals data
		if ( pDestNormalData )
		{
			GGVECTOR3* pNormal = (GGVECTOR3*)(pDestNormalData+(v*pMesh->dwFVFSize));
			GGVec3TransformNormal ( pNormal, pNormal, &matNoTransWorld );
			GGVec3Normalize ( pNormal, pNormal );
		}
	}

	// 151003 - add computation of bound box in collision structure (for limb collision)
	CalculateMeshBounds ( pMesh );

	// okay
	return true;
}

DARKSDK_DLL bool MakeMeshFromOtherMesh ( bool bCreateNew, sMesh* pMesh, sMesh* pOtherMesh, GGMATRIX* pmatWorld )
{
	DWORD dwIndexCount = pOtherMesh->dwIndexCount;
	DWORD dwVertexCount = pOtherMesh->dwVertexCount;
	return MakeMeshFromOtherMesh ( bCreateNew, pMesh, pOtherMesh, pmatWorld, dwIndexCount, dwVertexCount );
}
