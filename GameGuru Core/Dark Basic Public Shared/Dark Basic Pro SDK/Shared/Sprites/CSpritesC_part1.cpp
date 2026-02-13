DARKSDK void CreateAnimatedSpriteCore ( int iID, char* szImage, int iWidth, int iHeight, int iImageID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return;
	}

	if(UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITEALREADYTEXISTS);
		return;
	}

	if(iWidth<=0 || iWidth>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEWIDTHILLEGAL);
		return;
	}

	if(iHeight<=0 || iHeight>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEHEIGHTILLEGAL);
		return;
	}

	if(iImageID<0 || iImageID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEHANIMCOUNTILLEGAL);
		return;
	}

	// create an animated sprite using one large image
	// which contains all of the frames

	// check if the image in this ID already
	// exists, if so delete it
	if ( ImageExist ( iImageID ) )
		DeleteImage ( iImageID );

	// now load the new image
	//if( LoadImage ( szImage, iImageID )==false )
	{
		//RunTimeError(RUNTIMEERROR_INVALIDFILE);
		//return;
	}

	//DARKSDK void 				LoadEx						( LPSTR szFilename, int iID, int TextureFlag );							// load an image specifying the filename

	//LoadImageEx ( szImage, iImageID );

	// mike - 011105 - reference 1 to 1 for animated sprites
	LoadImage ( szImage, iImageID, 1 );

	//DARKSDK bool Load	( char* szFilename, int iID, int TextureFlag, bool bIgnoreNegLimit );			// load an image specifying the filename
	//LOAD IMAGE%SLL%?LoadEx@@YAXPADHH@Z%Filename, Image Number, Texture Flag

	//LoadImage ( szImage, iImageID, 1, 0 );

	
	// create a sprite offscreen
	Sprite ( iID, -1000000, -1000000, iImageID );

	// mike - 011005 - check the ptr is valid, it may fail due to invalid image
	if ( !m_ptr )
	{
		RunTimeError(RUNTIMEERROR_SPRITEERROR);
		return;
	}

	// now setup anim properties
	int iCount = iWidth * iHeight;
	m_ptr->eAnimType    = ONE_IMAGE;							// notify that we're using frames within an image
	m_ptr->iImage = iImageID;
	m_ptr->iFrameWidth  = ImageWidth  ( iImageID ) / iWidth;								// width of a frame
	m_ptr->iFrameHeight = ImageHeight ( iImageID ) / iHeight;								// height of a frame
	m_ptr->fClipU		= ImageUMax ( iImageID );
	m_ptr->fClipV		= ImageVMax ( iImageID );
	m_ptr->iFrameCount  = iCount;								// number of frames
	m_ptr->iFrameAcross = iWidth;	// get the number of frames across
	m_ptr->iFrameDown   = iHeight;	// get the number of frames down
	m_ptr->iFrame       = 0;									// starting frame


	// mike - 220604
	m_ptr->iLastFrame = 0;

	// Set first frame to hide all images
	SetSpriteFrameEx ( iID, 1 );

	// 270305 - mike - fixes distortion of sprite image
	SizeSprite ( iID, m_ptr->iFrameWidth, m_ptr->iFrameHeight );
}

DARKSDK void CreateAnimatedSprite ( int iID, char* szImage, int iWidth, int iHeight, int iImageID )
{
	// Uses actual or virtual file..
	char VirtualFilename[_MAX_PATH];
	strcpy(VirtualFilename, szImage);
	//g_pGlob->UpdateFilenameFromVirtualTable( VirtualFilename);

	CheckForWorkshopFile (VirtualFilename);

	// Decrypt and use media, re-encrypt
	g_pGlob->Decrypt( VirtualFilename );
	CreateAnimatedSpriteCore ( iID, VirtualFilename, iWidth, iHeight, iImageID );
	g_pGlob->Encrypt( VirtualFilename );
}

DARKSDK void SetSpritePriority ( int iID, int iPriority )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return;
	}

	// Set Priority level
	m_ptr->iPriority = iPriority;
}

//
// COMMAND SET EXPRESSIONS
//

DARKSDK int SpritePriority ( int iID )
{
	// mike - 041005 - return the priority of the sprite

	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return 0;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return 0;
	}

	return m_ptr->iPriority;
}

DARKSDK int SpriteExist ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return 0;
	}

	// check if a sprite exists
	if ( UpdateSpritesPtr ( iID ) )
		return 1;

	// return false if it doesn't exist
	return 0;
}

DARKSDK int SpriteX ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return -1;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return -1;
	}

	// return the x pos
	return (int)m_ptr->fX;
}

DARKSDK int SpriteY ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return -1;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return -1;
	}

	// return the y pos
	return (int)m_ptr->fY;
}

DARKSDK int GetSpriteImage ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return -1;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return -1;
	}

	// return the image number
	return m_ptr->iImage;
}

DARKSDK int SpriteWidth ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return -1;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return -1;
	}

	if ( m_ptr->iXSize==0 )
		return (int)(m_ptr->iWidth * m_ptr->fXScale);
	else
		return m_ptr->iXSize;
}

DARKSDK int SpriteHeight ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return -1;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return -1;
	}

	if ( m_ptr->iYSize==0 )
		return (int)(m_ptr->iHeight * m_ptr->fYScale);
	else
		return m_ptr->iYSize;
}

DARKSDK int SpriteScaleX ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return -1;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return -1;
	}

	// return the size
	return (int)(m_ptr->fXScale*100.0f);
}

DARKSDK int SpriteScaleY ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return -1;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return -1;
	}

	// return the size
	return (int)(m_ptr->fYScale*100.0f);
}

DARKSDK int SpriteMirrored ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return 0;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return 0;
	}

	// is the sprite mirrored
	if(m_ptr->bMirrored)
		return 1;
	else
		return 0;
}

DARKSDK int SpriteFlipped ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return 0;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return 0;
	}

	// is the sprite flipped
	if(m_ptr->bFlipped)
		return 1;
	else
		return 0;
}

DARKSDK int SpriteOffsetX ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return -1;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return -1;
	}

	// get the x offset
	return m_ptr->iXOffset;
}

DARKSDK int SpriteOffsetY ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return -1;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return -1;
	}

	// get the y offset
	return m_ptr->iYOffset;
}

DARKSDK int SpriteHit ( int iID, int iTarget )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return 0;
	}

	if(iTarget<0 || iTarget>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return 0;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return 0;
	}

	// leefix - 020308 - retain ptr to THIS sprite for hit control (hit was constantly fed back)
	tagSpriteData* m_ptrThis = m_ptr;

	int overlap = CheckSpriteCollision( iID, iTarget );

	int returnhit=0;
	if(overlap>0 && m_ptrThis->iHitoverlapstore==0)
		returnhit = overlap;
	else
		returnhit = 0;

	m_ptrThis->iHitoverlapstore = overlap;

	return returnhit;
}

DARKSDK int SpriteCollision ( int iID, int iTarget )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return 0;
	}

	if(iTarget<0 || iTarget>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return 0;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return 0;
	}

	// mike - 021005 - ensure target does not exist
	// mike - 111005 - modify so only check if target is not 0
	if ( iTarget != 0 )
	{
		if(!UpdateSpritesPtr(iTarget))
		{
			RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
			return 0;
		}
	}

	return CheckSpriteCollision( iID, iTarget );
}

DARKSDK float SpriteAngle ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return -1;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return -1;
	}

	// get the angle of a sprite
	return m_ptr->fAngle;
}

DARKSDK int SpriteAlpha ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return -1;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return -1;
	}

	// get the alpha of a sprite
	return m_ptr->iAlpha;
}

DARKSDK int SpriteRed ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return -1;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return -1;
	}

	// return the red colour
	return m_ptr->iRed;
}

DARKSDK int SpriteGreen ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return -1;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return -1;
	}

	// return the green colour
	return m_ptr->iGreen;
}

DARKSDK int SpriteBlue ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return -1;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return -1;
	}

	// return the blue colour
	return m_ptr->iBlue;
}

DARKSDK int SpriteFrame ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return -1;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return -1;
	}

	// get the frame of animation
	if ( m_ptr->eAnimType == ONE_IMAGE )
	{
		// mike - 220604 - return correct frame
		// leefix - 020308 - added anim detection and regular manual frame value return (for non playing sprites)
		if ( m_ptr->bIsAnim==true )
	 		return m_ptr->iLastFrame + 1;
		else
	 		return m_ptr->iFrame + 1;
	}
	else
		return m_ptr->iImage;
	
}

DARKSDK int SpriteVisible   ( int iID )
{
	if(iID<=0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_SPRITEILLEGALNUMBER);
		return -1;
	}

	if(!UpdateSpritesPtr(iID))
	{
		RunTimeError(RUNTIMEERROR_SPRITENOTEXIST);
		return -1;
	}

	// get the alpha of a sprite
	if(m_ptr->bVisible==true)
		return 1;
	else
		return 0;
}

DARKSDK void SetSpriteFilterMode ( int iMode )
{
	// mike - 071005 - filter mode in case you want to use linear
	// lee - 090910 - also controls whether WRAP(0-default) or 1(CLAMP)
    m_SpriteManager.SetFilterMode( iMode );
}

//////////////////////////////////////////////////////////////////////////////////
// DARK SDK SECTION //////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

#ifdef DARKSDK_COMPILE

void ConstructorSprites ( HINSTANCE hSetup, HINSTANCE hImage )
{
	Constructor ( hSetup, hImage );
}

void DestructorSprites ( void )
{
	Destructor ( );
}

void SetErrorHandlerSprites	( LPVOID pErrorHandlerPtr )
{
	SetErrorHandler ( pErrorHandlerPtr );
}

void PassCoreDataSprites ( LPVOID pGlobPtr )
{
	PassCoreData ( pGlobPtr );
}

void RefreshGRAFIXSprites ( int iMode )
{
	RefreshGRAFIX ( iMode );
}

void UpdateSprites ( void )
{
	Update ( );
}

void UpdateAllSpritesSprites (void)
{
	UpdateAllSprites ( );
}

void dbSetSprite ( int iID, int iBacksave, int iTransparent )
{
	SetSprite ( iID, iBacksave, iTransparent );
}

void dbSprite ( int iID, int iX, int iY, int iImage )
{
	Sprite (  iID,  iX,  iY,  iImage );
}

void dbPasteSprite ( int iID, int iX, int iY )
{
	Paste (  iID, iX,  iY );
}
	
void dbSizeSprite ( int iID, int iXSize, int iYSize )
{
	Size (  iID,  iXSize,  iYSize );
}

void dbScaleSprite ( int iID, float fScale )
{
	Scale (  iID,  fScale );
}
 
void dbStretchSprite ( int iID, int iXStretch, int iYStretch )
{
	Stretch (  iID,  iXStretch,  iYStretch );
}

void dbMirrorSprite ( int iID )
{
	Mirror (  iID );
}
 
void dbFlipSprite ( int iID )
{
	Flip ( iID );
}

void dbOffsetSprite ( int iID, int iXOffset, int iYOffset )
{
	Offset (  iID,  iXOffset,  iYOffset );
}
	
void dbHideSprite ( int iID )
{
	Hide ( iID );
}
 
void dbShowSprite ( int iID )
{
	Show ( iID );
}
 
void dbHideAllSprites ( void )
{
	HideAllSprites ( );
}
 
void dbShowAllSprites ( void )
{
	ShowAllSprites ( );
}
 
void dbDeleteSprite ( int iID )
{
	Delete ( iID );
}

void dbMoveSprite ( int iID, float velocity )
{
	Move (  iID,  velocity );
}
 
void dbRotateSprite ( int iID, float fRotate )
{
	Rotate (  iID,  fRotate );
}
 
void dbSetSpriteImage ( int iID, int iImage )
{
	SetImage (  iID,  iImage );
}
 
void dbSetSpriteAlpha ( int iID, int iValue )
{
	SetAlpha (  iID,  iValue );
}
 
void dbSetSpriteDiffuse ( int iID, int iR, int iG, int iB )
{
	SetDiffuse (  iID,  iR,  iG,  iB );
}

void dbPlaySprite ( int iID, int iStart, int iEnd, int iDelay )
{
	Play (  iID,  iStart,  iEnd,  iDelay );
}
 
void dbSetSpriteFrame ( int iID, int iFrame )
{
	SetFrame (  iID,  iFrame );
}
 
void dbSetSpriteTextureCoordinates ( int iID, int iVertex, float tu, float tv )
{
	SetTextureCoordinates (  iID,  iVertex,  tu,  tv );
}
 
void dbCreateAnimatedSprite ( int iID, char* szImage, int iWidth, int iHeight, int iImageID )
{
	CreateAnimatedSprite (  iID,  szImage,  iWidth,  iHeight, iImageID );
}

void dbCloneSprite ( int iID, int iDestinationID )
{
	Clone (  iID,  iDestinationID );
}
 
void dbSetSpritePriority ( int iID, int iPriority )
{
	SetPriority (  iID,  iPriority );
}

int dbSpriteExist ( int iID )
{
	return GetExist ( iID );
}
 
int dbSpriteX ( int iID )
{
	return GetX ( iID );
}
 
int dbSpriteY ( int iID )
{
	return GetY ( iID );
}
 
int dbSpriteImage ( int iID )
{
	return GetImage ( iID );
}
 
int dbSpriteWidth ( int iID )
{
	return GetWidth ( iID );
}
 
int dbSpriteHeight ( int iID )
{
	return GetHeight ( iID );
}
 
int dbSpriteScaleX ( int iID )
{
	return GetXScale ( iID );
}
 
int dbSpriteScaleY ( int iID )
{
	return GetYScale ( iID );
}
 
int dbSpriteMirrored ( int iID )
{
	return GetMirrored ( iID );
}
 
int dbSpriteFlipped ( int iID )
{
	return GetFlipped ( iID );
}
 
int dbSpriteOffsetX ( int iID )
{
	return GetXOffset ( iID );
}
 
int dbSpriteOffsetY ( int iID )
{
	return GetYOffset ( iID );
}
 
int dbSpriteHit ( int iID, int iTarget )
{
	return GetHit (  iID,  iTarget );
}
 
int dbSpriteCollision ( int iID, int iTarget )
{
	return GetCollision (  iID,  iTarget );
}

float dbSpriteAngle ( int iID )
{
	DWORD dwReturn = GetAngle ( iID );
	
	return *( float* ) &dwReturn;
}
 
int dbSpriteAlpha ( int iID )
{
	return GetAlpha ( iID );
}
 
int dbSpriteRed ( int iID )
{
	return GetRed ( iID );
}
 
int dbSpriteGreen ( int iID )
{
	return GetGreen ( iID );
}
 
int dbSpriteBlue ( int iID )
{
	return GetBlue ( iID );
}
 
int dbSpriteFrame ( int iID )
{
	return GetFrame ( iID );
}
 
int dbSpriteVisible ( int iID )
{
	return GetVisible ( iID );
}

void dbPasteImage ( int iImageID, int iX, int iY, float fU, float fV )
{
	PasteImage ( iImageID, iX, iY, fU, fV );
}

void dbPasteImageEx ( int iImageID, int iX, int iY, float fU, float fV, int iTransparent )
{
	PasteImageEx ( iImageID, iX, iY, fU, fV, iTransparent );
}

void dbPasteTextureToRect ( LPGGTEXTURE pTexture, float fU, float fV, RECT Rect )
{
	PasteTextureToRect ( pTexture, fU, fV, Rect );
}

void dbSaveSpriteBack ( void )
{
	SaveBack ( );
}

void dbRestoreSpriteBack ( void )
{
	RestoreBack ( );
}

// lee - 300706 - GDK fixes
void dbScaleSprite ( int iID, int iScale ) { dbScaleSprite ( iID, (float)iScale ); }
void dbSetSpriteTextureCoord ( int iID, int iVertex, float tu, float tv ) { dbSetSpriteTextureCoordinates ( iID, iVertex, tu, tv ); }

#endif

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
