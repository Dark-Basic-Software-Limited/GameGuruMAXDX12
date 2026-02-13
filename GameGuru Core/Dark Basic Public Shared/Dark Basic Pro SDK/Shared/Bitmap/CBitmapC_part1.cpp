#ifndef DX11 // continued from part0
DARKSDK void SetCurrentBitmap ( int iID )
{
	if ( iID>=0 )
	{
		// specifies bitmap number
		if(iID<0 || iID>MAXIMUMVALUE)
		{
			RunTimeError(RUNTIMEERROR_BITMAPILLEGALNUMBER);
			return;
		}
		if ( !BitmapUpdatePtr ( iID ) )
		{
			RunTimeError(RUNTIMEERROR_BITMAPNOTEXIST);
			return;
		}
	}
	else
	{
		// specifies inverse camera number
	}

	// Bitmap Zero is default backbuffer
	if ( iID==0 )
	{
		// render target is backbuffer
		///m_pD3D->SetRenderTarget ( 0, g_pGlob->pHoldBackBufferPtr );
		///m_pD3D->SetDepthStencilSurface ( g_pGlob->pHoldDepthBufferPtr );
		SetRenderTargetAndDepth ( g_pGlob->pHoldBackBufferPtr, g_pGlob->pHoldDepthBufferPtr );
		g_pGlob->pCurrentBitmapSurface=g_pGlob->pHoldBackBufferPtr;
		g_pGlob->iCurrentBitmapNumber=iID;
	}
	else
	{
		if ( iID > 0 )
		{
			// render target is bitmap
			///m_pD3D->SetRenderTarget ( 0, m_camptr->lpSurface );
			///m_pD3D->SetDepthStencilSurface ( m_camptr->lpDepth );
			SetRenderTargetAndDepth ( m_camptr->lpSurface, m_camptr->lpDepth );
			g_pGlob->pCurrentBitmapSurface=m_camptr->lpSurface;
			g_pGlob->iCurrentBitmapNumber=iID;
		}
		else
		{
			// U70 - 190908 - render target is camera surface
			int iRenderBitmapActivityToCameraID = abs(iID);
			if ( iRenderBitmapActivityToCameraID > 0 )
			{
				tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData ( iRenderBitmapActivityToCameraID );
				if ( m_Camera_Ptr )
				{
					///m_pD3D->SetRenderTarget ( 0, m_Camera_Ptr->pCameraToImageSurface );
					///m_pD3D->SetDepthStencilSurface ( NULL );
					SetRenderTargetAndDepth ( m_Camera_Ptr->pCameraToImageSurface, NULL );
					g_pGlob->pCurrentBitmapSurface=m_Camera_Ptr->pCameraToImageSurface;
				}
				else
					iRenderBitmapActivityToCameraID = 0;
			}
			if ( iRenderBitmapActivityToCameraID==0 )
			{
				// DEFAULT back to render target is default backbuffer
				///m_pD3D->SetRenderTarget ( 0, g_pGlob->pHoldBackBufferPtr );
				///m_pD3D->SetDepthStencilSurface ( g_pGlob->pHoldDepthBufferPtr );
				SetRenderTargetAndDepth ( g_pGlob->pHoldBackBufferPtr, g_pGlob->pHoldDepthBufferPtr );
				g_pGlob->pCurrentBitmapSurface=g_pGlob->pHoldBackBufferPtr;
			}
			g_pGlob->iCurrentBitmapNumber=0;
		}
	}
}


//
// Command Expression Functions
//

DARKSDK int CurrentBitmap ( void )
{
	// Current birmap number
	return g_pGlob->iCurrentBitmapNumber;
}

DARKSDK int BitmapExist ( int iID )
{
	if(iID<0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_BITMAPILLEGALNUMBER);
		return 0;
	}

	// returns true if the bitmap exists
	if ( !BitmapUpdatePtr ( iID ) )
		return 0;

	// return true
	return 1;
}

DARKSDK int BitmapWidth ( int iID )
{
	if(iID<0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_BITMAPILLEGALNUMBER);
		return 0;
	}
	if ( !BitmapUpdatePtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_BITMAPNOTEXIST);
		return 0;
	}
	return m_camptr->iWidth;
}

DARKSDK int BitmapHeight ( int iID )
{
	if(iID<0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_BITMAPILLEGALNUMBER);
		return 0;
	}
	if ( !BitmapUpdatePtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_BITMAPNOTEXIST);
		return 0;
	}
	return m_camptr->iHeight;
}

DARKSDK int BitmapDepth ( int iID )
{
	if(iID<0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_BITMAPILLEGALNUMBER);
		return 0;
	}
	if ( !BitmapUpdatePtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_BITMAPNOTEXIST);
		return 0;
	}
	return m_camptr->iDepth;
}

DARKSDK int BitmapMirrored ( int iID )
{
	if(iID<0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_BITMAPILLEGALNUMBER);
		return 0;
	}
	if ( !BitmapUpdatePtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_BITMAPNOTEXIST);
		return 0;
	}
	return m_camptr->iMirrored;
}

DARKSDK int BitmapFlipped ( int iID )
{
	if(iID<0 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_BITMAPILLEGALNUMBER);
		return 0;
	}
	if ( !BitmapUpdatePtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_BITMAPNOTEXIST);
		return 0;
	}
	return m_camptr->iFlipped;
}

DARKSDK int BitmapExist ( void )
{
	return BitmapExist ( g_pGlob->iCurrentBitmapNumber );
}

DARKSDK int BitmapWidth ( void )
{
	return BitmapWidth ( g_pGlob->iCurrentBitmapNumber );
}

DARKSDK int BitmapHeight ( void )
{
	return BitmapHeight ( g_pGlob->iCurrentBitmapNumber );
}

DARKSDK int BitmapDepth ( void )
{
	return BitmapDepth ( g_pGlob->iCurrentBitmapNumber );
}

DARKSDK int BitmapMirrored ( void )
{
	return BitmapMirrored ( g_pGlob->iCurrentBitmapNumber );
}

DARKSDK int BitmapFlipped ( void )
{
	return BitmapFlipped ( g_pGlob->iCurrentBitmapNumber );
}

// Data Access Functions

DARKSDK void GetBitmapData( int iID, DWORD* dwWidth, DWORD* dwHeight, DWORD* dwDepth, LPSTR* pData, DWORD* dwDataSize, bool bLockData )
{
	// Read Data
	if(bLockData==true)
	{
		if ( !BitmapUpdatePtr ( iID ) )
			return;

		// Backbuffer or Bitmap To/Dest Surface
		LPGGSURFACE pToSurface = m_camptr->lpSurface;
		if(iID==0) pToSurface=g_pGlob->pHoldBackBufferPtr;

		// bitmap data
		*dwWidth = m_camptr->iWidth;
		*dwHeight = m_camptr->iHeight;
		*dwDepth = m_camptr->iDepth;

		// lock
		GGLOCKED_RECT d3dlock;
		if(SUCCEEDED(pToSurface->LockRect ( &d3dlock, NULL, 0 ) ) )
		{
			// create memory
			DWORD bpp = m_camptr->iDepth/8;
			DWORD dwSizeOfBitmapData = m_camptr->iWidth*m_camptr->iHeight*bpp;
			*pData = new char[dwSizeOfBitmapData];
			*dwDataSize = dwSizeOfBitmapData;

			// copy from surface
			LPSTR pSrc = (LPSTR)d3dlock.pBits;
			LPSTR pPtr = *pData;
			for(int y=0; y<m_camptr->iHeight; y++)
			{
				memcpy(pPtr, pSrc, m_camptr->iWidth*bpp);
				pPtr+=m_camptr->iWidth*bpp;
				pSrc+=d3dlock.Pitch;
			}
			pToSurface->UnlockRect();
		}
	}
	else
	{
		// free memory
		delete *pData;
	}
}

DARKSDK void SetBitmapData( int iID, DWORD dwWidth, DWORD dwHeight, DWORD dwDepth, LPSTR pData, DWORD dwDataSize )
{
	LPGGSURFACE pToSurface=NULL;
	if ( iID>0 )
	{
		if( BitmapUpdatePtr ( iID ) )
		{
			// Backbuffer or Bitmap To/Dest Surface
			pToSurface = m_camptr->lpSurface;
			if(iID==0) pToSurface=g_pGlob->pHoldBackBufferPtr;

			// Check new bitmap specs with existing one
			if(dwWidth==(DWORD)m_camptr->iWidth && dwHeight==(DWORD)m_camptr->iHeight && dwDepth==(DWORD)m_camptr->iDepth)
			{
				// Same bitmap size
			}
			else
			{
				// Recreate Bitmap
				DeleteBitmapEx ( iID );
				m_camptr=NULL;
			}
		}
		if(m_camptr==NULL)
		{
			GGFORMAT dFormat;
			g_bOffscreenBitmap=false;
			if ( dwDepth==16 ) dFormat=GGFMT_R5G6B5;
			if ( dwDepth==24 ) dFormat=GGFMT_R8G8B8;
			if ( dwDepth==32 ) dFormat=GGFMT_X8R8G8B8;
			MakeFormat ( iID, dwWidth, dwHeight, dFormat );
		}
	}

	// Failed to create bitmap?
	if ( !BitmapUpdatePtr ( iID ) ) return;

	// Bitmap may have changed
	pToSurface = m_camptr->lpSurface;
	if(iID==0) pToSurface=g_pGlob->pHoldBackBufferPtr;

	// Write Data
	GGLOCKED_RECT d3dlock;
	if(SUCCEEDED(pToSurface->LockRect ( &d3dlock, NULL, 0 ) ) )
	{
		// copy from surface
		DWORD databpp = dwDepth/8;
		LPSTR pSrc = (LPSTR)d3dlock.pBits;
		LPSTR pPtr = pData;
		for(DWORD y=0; y<dwHeight; y++)
		{
			memcpy(pSrc, pPtr, dwWidth*databpp);
			pPtr+=dwWidth*databpp;
			pSrc+=d3dlock.Pitch;
		}
		pToSurface->UnlockRect();
	}

}

#endif
