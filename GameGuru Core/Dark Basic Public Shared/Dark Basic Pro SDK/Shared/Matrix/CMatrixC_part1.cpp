// mike - 230406 - reported as not moving in right direction
//				 - shift down now becomes shift up
//void ShiftDown ( int iID )
DARKSDK void ShiftUp ( int iID )
{
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return;
	}

	// mike : could you wrap the texture UV coords too :)

	// move all of the matrix tiles down by one,
	// and wrap them around e.g.
	
	// using a 5 * 5 grid, before shift down
	
	// 5 = 20 21 22 23 24
	// 4 = 15 16 17 18 19
	// 3 = 10 11 12 13 14
	// 2 =  5  6  7  8  9
	// 1 =  0  1  2  3  4


	// 5 = 15 16 17 18 19
	// 4 = 10 11 12 13 14
	// 3 =  5  6  7  8  9
	// 2 =  0  1  2  3  4
	// 1 = 20 21 22 23 24

	int iX    = 0;
	int iZ	  = 0;
	int iTemp = 0;

	// copy all of the height properties across to the temp buffer
	for ( iTemp = 0; iTemp < m_ptr->iXSegmentedFull * m_ptr->iZSegmentedFull; iTemp++ )
	{
		m_ptr->pfVert [ iTemp ].y = m_ptr->pTiles [ iTemp ].fHeight;
		m_ptr->pfVert [ iTemp ].nx = m_ptr->pTiles [ iTemp ].vNormal.x;
		m_ptr->pfVert [ iTemp ].ny = m_ptr->pTiles [ iTemp ].vNormal.y;
		m_ptr->pfVert [ iTemp ].nz = m_ptr->pTiles [ iTemp ].vNormal.z;
		m_ptr->ptmpTiles [ iTemp ] = m_ptr->pTiles [ iTemp ].iTile;
	}
	
	int iOffset = 0;
	for ( iZ = 1; iZ < m_ptr->iZSegmentedFull; iZ++ )
	{
		for ( iX = 0; iX < m_ptr->iXSegmentedFull; iX++ )	
		{
			SetHeight ( iID, iX, iZ,	m_ptr->pfVert [ iX + ( iOffset * m_ptr->iXSegmentedFull ) ].y );
			SetNormalCore ( iID, iX, iZ,	m_ptr->pfVert [ iX + ( iOffset * m_ptr->iXSegmentedFull ) ].nx,
											m_ptr->pfVert [ iX + ( iOffset * m_ptr->iXSegmentedFull ) ].ny,
											m_ptr->pfVert [ iX + ( iOffset * m_ptr->iXSegmentedFull ) ].nz );

			SetTileCore ( iID, iX, iZ,	m_ptr->ptmpTiles [ iX + ( iOffset * m_ptr->iXSegmentedFull ) ] );
		}
		iOffset++;
	}

	iZ=0;
	iOffset = m_ptr->iZSegmentedFull - 1;
	// leefix - 211211 - u78 - not sure where these extra -1 came from, not needed RE DarkGDK
	// iOffset = iOffset - 1; // leefix - 270206 - u60 - we're copying from unshifted temp data (extra -1!)
	for ( iX = 0; iX < m_ptr->iXSegmentedFull; iX++ )
	{
		SetHeight ( iID, iX, iZ,	m_ptr->pfVert [ iX + ( iOffset * m_ptr->iXSegmentedFull ) ].y );
		SetNormalCore ( iID, iX, iZ,	m_ptr->pfVert [ iX + ( iOffset * m_ptr->iXSegmentedFull ) ].nx,
										m_ptr->pfVert [ iX + ( iOffset * m_ptr->iXSegmentedFull ) ].ny,
										m_ptr->pfVert [ iX + ( iOffset * m_ptr->iXSegmentedFull ) ].nz );

		SetTileCore ( iID, iX, iZ,	m_ptr->ptmpTiles [ iX + ( (iOffset) * m_ptr->iXSegmentedFull ) ] );
	}

}

DARKSDK void ShiftLeft ( int iID )
{
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return;
	}

	int iX    = 0;
	int iZ	  = 0;
	int iTemp = 0;
	int iOffset = 0;

	// copy all of the height properties across to the temp buffer
	for ( iTemp = 0; iTemp < m_ptr->iXSegmentedFull * m_ptr->iZSegmentedFull; iTemp++ )
	{
		m_ptr->pfVert [ iTemp ].y = m_ptr->pTiles [ iTemp ].fHeight;
		m_ptr->pfVert [ iTemp ].nx = m_ptr->pTiles [ iTemp ].vNormal.x;
		m_ptr->pfVert [ iTemp ].ny = m_ptr->pTiles [ iTemp ].vNormal.y;
		m_ptr->pfVert [ iTemp ].nz = m_ptr->pTiles [ iTemp ].vNormal.z;
		m_ptr->ptmpTiles [ iTemp ] = m_ptr->pTiles [ iTemp ].iTile;
	}
	
	/*

	// mike - 220406 - comment out because causes a crash, see below

	for ( iZ = 0; iZ < m_ptr->iZSegmentedFull; iZ++ )
	{
		if ( iZ < m_ptr->iZSegmentedFull - 1 )
			iOffset++;
		else
			iOffset = 0;

		for ( iX = 0; iX < m_ptr->iXSegmentedFull; iX++ )
		{
			SetHeight ( iID, iZ, iX, m_ptr->pfVert [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ].y );
			SetNormalCore ( iID, iZ, iX,	m_ptr->pfVert [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ].nx,
											m_ptr->pfVert [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ].ny,
											m_ptr->pfVert [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ].nz );

			int iTileOffset = iOffset % m_ptr->iZSegmented;
			SetTileCore ( iID, iZ, iX,	m_ptr->ptmpTiles [ iTileOffset + ( iX * m_ptr->iXSegmentedFull ) ] );
		}
	}
	*/
	
	// mike - 220406 - stop crash with matrices of different sizes
	//				 - e.g. 32 x 64

	int iZMax = m_ptr->iZSegmentedFull;
	int iXMax = m_ptr->iXSegmentedFull;

	//if ( m_ptr->iZSegmentedFull > m_ptr->iXSegmentedFull )
	{
		iZMax = m_ptr->iXSegmentedFull;
		iXMax = m_ptr->iZSegmentedFull;
	}
	
	for ( iZ = 0; iZ < iZMax; iZ++ )
	{
		if ( iZ < iZMax - 1 )
			iOffset++;
		else
			// lee - 270206 - u60 - from unshifted buffer data, extra 1
//			iOffset = 0;
//			iOffset = 1;
			// lee - u78 - 211211 - seems the 2006 correction copied the WRONG line to the rightmost column (DarkGDK report)
			iOffset = 0;

		for ( iX = 0; iX < iXMax; iX++ )
		{
			SetHeight ( iID, iZ, iX, m_ptr->pfVert [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ].y );
			SetNormalCore ( iID, iZ, iX,	m_ptr->pfVert [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ].nx,
											m_ptr->pfVert [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ].ny,
											m_ptr->pfVert [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ].nz );

			int iTileOffset = iOffset % m_ptr->iZSegmented;
			SetTileCore ( iID, iZ, iX,	m_ptr->ptmpTiles [ iTileOffset + ( iX * m_ptr->iXSegmentedFull ) ] );
		}
	}
}

DARKSDK void ShiftRight ( int iID )
{
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return;
	}

	int iX    = 0;
	int iZ	  = 0;
	int iTemp = 0;

	// copy all of the height properties across to the temp buffer
	for ( iTemp = 0; iTemp < m_ptr->iXSegmentedFull * m_ptr->iZSegmentedFull; iTemp++ )
	{
		m_ptr->pfVert [ iTemp ].y = m_ptr->pTiles [ iTemp ].fHeight;
		m_ptr->pfVert [ iTemp ].nx = m_ptr->pTiles [ iTemp ].vNormal.x;
		m_ptr->pfVert [ iTemp ].ny = m_ptr->pTiles [ iTemp ].vNormal.y;
		m_ptr->pfVert [ iTemp ].nz = m_ptr->pTiles [ iTemp ].vNormal.z;
		m_ptr->ptmpTiles [ iTemp ] = m_ptr->pTiles [ iTemp ].iTile;
	}
	
	// mike - 220406 - stop crash with matrices of different sizes
	//				 - e.g. 32 x 64

	int iZMax = m_ptr->iZSegmentedFull;
	int iXMax = m_ptr->iXSegmentedFull;

	//if ( m_ptr->iZSegmentedFull > m_ptr->iXSegmentedFull )
	{
		// lee - 270206 - ?!? X becoming Z?
		iZMax = m_ptr->iXSegmentedFull;
		iXMax = m_ptr->iZSegmentedFull;
	}
	
	int iOffset = 0;
	for ( iZ = 1; iZ < iZMax; iZ++ )
	{
		for ( iX = 0; iX < iXMax; iX++ )	
		{
			SetHeight ( iID, iZ, iX, m_ptr->pfVert [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ].y );
			SetNormalCore ( iID, iZ, iX,	m_ptr->pfVert [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ].nx,
											m_ptr->pfVert [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ].ny,
											m_ptr->pfVert [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ].nz );

			SetTileCore ( iID, iZ, iX,	m_ptr->ptmpTiles [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ] );
		}

		iOffset++;
	}

	// copy rightmost data to left side
	iZ = 0;
	//iOffset = iZMax - 1 - 1; // leefix - 270206 - u60 - we're copying from unshifted temp data (extra -1!)
	iOffset = iZMax - 1; // leefix - 211211 - u78 - no need for extra -1 according to DarkGDK
	for ( iX = 0; iX < iXMax; iX++ )
	{
		SetHeight ( iID, iZ, iX, m_ptr->pfVert [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ].y );
		SetNormalCore ( iID, iZ, iX,	m_ptr->pfVert [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ].nx,
										m_ptr->pfVert [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ].ny,
										m_ptr->pfVert [ iOffset + ( iX * m_ptr->iXSegmentedFull ) ].nz );

		SetTileCore ( iID, iZ, iX,	m_ptr->ptmpTiles [ (iOffset) + ( iX * m_ptr->iXSegmentedFull ) ] );
	}
}

// mike - 230406 - reported as not moving in right direction
//				 - shift up now becomes shift down
//void ShiftUp ( int iID )
DARKSDK void ShiftDown ( int iID )
{
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return;
	}

	// move all of the matrix tiles up by one,
	// and wrap them around e.g.
	
	// using a 5 * 5 grid, before shift up
	// 5 = 20 21 22 23 24
	// 4 = 15 16 17 18 19
	// 3 = 10 11 12 13 14
	// 2 =  5  6  7  8  9
	// 1 =  0  1  2  3  4

	// after shift up
	// 5 =  0  1  2  3  4
	// 4 = 20 21 22 23 24
	// 3 = 15 16 17 18 19
	// 2 = 10 11 12 13 14
	// 1 =  5  6  7  8  9

	// shift up again
	// 5 =  5  6  7  8  9
	// 4 =  0  1  2  3  4
	// 3 = 20 21 22 23 24
	// 2 = 15 16 17 18 19
	// 1 = 10 11 12 13 14

	// basically take the bottom line of 
	// the grid and move it to the top

	int iX    = 0;
	int iZ	  = 0;
	int iTemp = 0;
	int iOffset = 0;

	// copy all of the height properties across to the temp buffer
	for ( iTemp = 0; iTemp < m_ptr->iXSegmentedFull * m_ptr->iZSegmentedFull; iTemp++ )
	{
		m_ptr->pfVert [ iTemp ].y = m_ptr->pTiles [ iTemp ].fHeight;
		m_ptr->pfVert [ iTemp ].nx = m_ptr->pTiles [ iTemp ].vNormal.x;
		m_ptr->pfVert [ iTemp ].ny = m_ptr->pTiles [ iTemp ].vNormal.y;
		m_ptr->pfVert [ iTemp ].nz = m_ptr->pTiles [ iTemp ].vNormal.z;
		m_ptr->ptmpTiles [ iTemp ] = m_ptr->pTiles [ iTemp ].iTile;
	}

	for ( iZ = 0; iZ < m_ptr->iZSegmentedFull; iZ++ )
	{
		if ( iZ < m_ptr->iZSegmentedFull - 1 )
			iOffset++;
		else
			// lee - 270206 - u60 - from unshifted buffer data, extra 1
//			iOffset = 0;
//			iOffset = 1;
			// lee - u78 - 211211 - seems the 2006 correction copied the WRONG line to the top row (DarkGDK report)
			iOffset = 0;

		for ( iX = 0; iX < m_ptr->iXSegmentedFull; iX++ )
		{
			SetHeight ( iID, iX, iZ, m_ptr->pfVert [ iX + ( iOffset * m_ptr->iXSegmentedFull ) ].y );
			SetNormalCore ( iID, iX, iZ,	m_ptr->pfVert [ iX + ( iOffset * m_ptr->iXSegmentedFull ) ].nx,
											m_ptr->pfVert [ iX + ( iOffset * m_ptr->iXSegmentedFull ) ].ny,
											m_ptr->pfVert [ iX + ( iOffset * m_ptr->iXSegmentedFull ) ].nz );

			int iTileOffset = iOffset % m_ptr->iZSegmented;
			SetTileCore ( iID, iX, iZ,	m_ptr->ptmpTiles [ iX + ( iTileOffset * m_ptr->iXSegmentedFull ) ] );
		}
	}
}

DARKSDK void Apply ( int iID )
{
	// apply any changes to the matrix

	// update the internal data
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return;
	}

	// variable declarations
	int						iX     = 0;														// x segment
	int						iZ     = 0;														// z segment
	int						iCount = 0;														// counter
	VOID*					pVertices	= NULL;												// pointer for vertices data
	tagMatrixVertexDesc*	pData		= NULL;												// pointer for vertices data
	DWORD					dwSize      = ( m_ptr->iXSegmented * m_ptr->iZSegmented ) *
										  4 * sizeof ( tagMatrixVertexDesc );				// size of matrix in bytes
	
	// lock the vertex buffer so we can get access to it's data
	if ( FAILED ( m_ptr->lpVertexBuffer->Lock ( 0, dwSize, ( VOID** ) &pVertices, 0 ) ) )
		Error ( "Failed to lock vertex buffer for matrix manipulation" );
	
	// cast pointer to our own type
	pData = ( tagMatrixVertexDesc* ) pVertices;

	// run through all segments and apply properties
	for ( iZ = 0; iZ < m_ptr->iZSegmentedFull; iZ++ )
	{
		for ( iX = 0; iX < m_ptr->iXSegmentedFull; iX++ )
		{
			// set the tile height
			SetHeightReal ( iID, pData, iX, iZ, m_ptr->pTiles [ iCount   ].fHeight );

			// set the tile normal
			SetNormalReal ( iID, pData, iX, iZ, m_ptr->pTiles [ iCount   ].vNormal );

			// set the tile pattern (tile based, not corner based)
			if(iX<m_ptr->iXSegmented && iZ<m_ptr->iZSegmented) 
				SetTileReal   ( iID, pData, iX, iZ, m_ptr->pTiles [ iCount ].iTile );

			// next in tiledata
			iCount++;
		}
	}

	// unlock
	if ( FAILED ( m_ptr->lpVertexBuffer->Unlock ( ) ) )
        return;
}

DARKSDK void SetTrim ( int iID, float fTrimX, float fTrimY )
{
	// update the internal data
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return;
	}

	// set trim values
	m_ptr->fTrimX = fTrimX;
	m_ptr->fTrimY = fTrimY;
}

DARKSDK void SetPriority ( int iID, int iPriority )
{
	// update the internal data
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return;
	}

	// set whethe render first or last
	if ( iPriority==1 )
		m_ptr->bRenderAfterObjects = true;
	else
		m_ptr->bRenderAfterObjects = false;
}

//
// DBV1 Expression Functions
//

DARKSDK DWORD GetPositionXEx ( int iID )
{
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return 0;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return 0;
	}
	return MatrixGetXPositionEx( iID );
}

DARKSDK DWORD GetPositionYEx ( int iID )
{
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return 0;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return 0;
	}
	return MatrixGetYPositionEx( iID );
}

DARKSDK DWORD GetPositionZEx ( int iID )
{
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return 0;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return 0;
	}
	return MatrixGetZPositionEx( iID );
}

DARKSDK float DB_GetGridGroundLevel(float AtX, float AtZ, float fX, float fZ, float fSizeX, float fSizeZ, float fHeightA, float fHeightB, float fHeightC, float fHeightD)
{
	// Find triangle from tile pointed to by lpVector
//	D3DVALUE width = model[ModelIndex].width;
//	D3DVALUE depth = model[ModelIndex].depth;
//	int xseg = model[ModelIndex].xsegments;
//	int zseg = model[ModelIndex].zsegments;
//	int tilex = (int)(AtX / (width / (D3DVALUE)xseg));
//	int tilez = (int)(AtZ / (depth / (D3DVALUE)zseg));
//	int MeshIndex = model[ModelIndex].solid[0].MeshIndex;
//	LPD3DVECTOR lpVector = (LPD3DVECTOR)(MeshObject[MeshIndex].ObjVertices + (tilex * (zseg*12)) + (tilez * 12));

	// Work out actual Y from XZ vector
	D3DXVECTOR3		Vector0, Vector1, Vector2;
	float			TopX, TopZ, NowAtX, NowAtZ;
	float			Percentage;
	D3DXVECTOR3		Cut0to2, Cut1to2;
	float			Distance, FinalY;

	TopX = fX;
	TopZ = fZ;
	NowAtX = AtX - TopX;
	NowAtZ = AtZ - TopZ;

	if(NowAtZ<NowAtX)
	{
		// Triangle A
		Vector0.x = fX;			Vector0.z = fZ;			Vector0.y = fHeightA;//0
		Vector1.x = fX+fSizeX;	Vector1.z = fZ;			Vector1.y = fHeightB;//1
		Vector2.x = fX+fSizeX;	Vector2.z = fZ+fSizeZ;	Vector2.y = fHeightD;//2
	}
	else
	{
		// Triangle B
		Vector0.x = fX+fSizeX;	Vector0.z = fZ+fSizeZ;	Vector0.y = fHeightD;//2
		Vector1.x = fX;			Vector1.z = fZ+fSizeZ;	Vector1.y = fHeightC;//3
		Vector2.x = fX;			Vector2.z = fZ;			Vector2.y = fHeightA;//0
	}

	// Cut vector 0 to 2 to make new point
	if(Vector2.z > Vector0.z)
	{
		if((Vector2.z - Vector0.z)==0.0f)
			Percentage = 0.0f;
		else
			Percentage	=	NowAtZ / (Vector2.z - Vector0.z);
		
		Cut0to2.y	=	Vector0.y + ((Vector2.y - Vector0.y) * Percentage);
		Cut0to2.x	=	Vector0.x + ((Vector2.x - Vector0.x) * Percentage);
	}
	else
	{
		if((Vector0.z - Vector2.z)==0.0f)
			Percentage = 0.0f;
		else
			Percentage	=	NowAtZ / (Vector0.z - Vector2.z);

		Cut0to2.y	=	Vector2.y + ((Vector0.y - Vector2.y) * Percentage);
		Cut0to2.x	=	Vector2.x + ((Vector0.x - Vector2.x) * Percentage);
	}

	// Cut vector 1 to 2 to make new point
	if(Vector2.z > Vector1.z)
	{
		if((Vector2.z - Vector1.z)==0.0f)
			Percentage = 0.0f;		
		else
			Percentage	=	NowAtZ / (Vector2.z - Vector1.z);

		Cut1to2.y	=	Vector1.y + ((Vector2.y - Vector1.y) * Percentage);
		Cut1to2.x	=	Vector1.x + ((Vector2.x - Vector1.x) * Percentage);
	}
	else
	{
		if((Vector1.z - Vector2.z)==0.0f)
			Percentage = 0.0f;		
		else
			Percentage	=	NowAtZ / (Vector1.z - Vector2.z);

		Cut1to2.y	=	Vector2.y + ((Vector1.y - Vector2.y) * Percentage);
		Cut1to2.x	=	Vector2.x + ((Vector1.x - Vector2.x) * Percentage);
	}

	// Intersect new line with X to get real Y
	Distance = Cut1to2.x - Cut0to2.x;
	if(Distance==0.0f)
		Percentage=0.0f;
	else
		Percentage = (AtX - Cut0to2.x) / Distance;
	
	FinalY = Cut0to2.y + ((Cut1to2.y - Cut0to2.y) * Percentage);

	return FinalY;
}

DARKSDK DWORD GetGroundHeightEx	( int iID, float fX, float fZ )
{
	// get y height at tile
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return 0;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return 0;
	}

	// this coordin and next
	float fBitX = (float)m_ptr->iWidth/m_ptr->iXSegmented;
	float fBitZ = (float)m_ptr->iDepth/m_ptr->iZSegmented;
	int iX = (int)(fX/fBitX);
	int iZ = (int)(fZ/fBitZ);
	int iX2 = iX+1;
	int iZ2 = iZ+1;

	// ensure coordinare within datamap
	if(iX<0) iX=0;
	if(iZ<0) iZ=0;
	if(iX>m_ptr->iXSegmented) iX=m_ptr->iXSegmented;
	if(iZ>m_ptr->iZSegmented) iZ=m_ptr->iZSegmented;
	if(iX2<0) iX2=0;
	if(iZ2<0) iZ2=0;
	if(iX2>m_ptr->iXSegmented) iX2=m_ptr->iXSegmented;
	if(iZ2>m_ptr->iZSegmented) iZ2=m_ptr->iZSegmented;

	// establish corners of tile we are on
	int aX = iX;  int aZ = iZ;
	int bX = iX2; int bZ = iZ;
	int cX = iX;  int cZ = iZ2;
	int dX = iX2; int dZ = iZ2;

	// get heights of each corner
	float fHeightA = m_ptr->pTiles [ aX + ( aZ * m_ptr->iXSegmentedFull ) ].fHeight;
	float fHeightB = m_ptr->pTiles [ bX + ( bZ * m_ptr->iXSegmentedFull ) ].fHeight;
	float fHeightC = m_ptr->pTiles [ cX + ( cZ * m_ptr->iXSegmentedFull ) ].fHeight;
	float fHeightD = m_ptr->pTiles [ dX + ( dZ * m_ptr->iXSegmentedFull ) ].fHeight;

	// old DBV! matrix height finder
	float fHeight = DB_GetGridGroundLevel(fX, fZ, aX*fBitX, aZ*fBitZ, fBitX, fBitZ, fHeightA, fHeightB, fHeightC, fHeightD);

	// final height
	return *(DWORD*)&fHeight;
}

DARKSDK DWORD GetHeightEx ( int iID, int iX, int iZ )
{
	// get height at tile
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return 0;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return 0;
	}
	if ( iX < 0 || iX > m_ptr->iXSegmented || iZ < 0 || iZ > m_ptr->iZSegmented )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXTILECOORDSWRONG);
		return 0;
	}

	float fValue = m_ptr->pTiles [ iX + ( iZ * m_ptr->iXSegmentedFull ) ].fHeight;
	return *(DWORD*)&fValue;
}

DARKSDK int GetExistEx ( int iID )
{
	// returns true if the matrix does exist
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return 0;
	}
	if ( !UpdateMatrixPtr ( iID ) )
		return 0;
	
	return 1;
}

DARKSDK int GetTileCountEx ( int iID )
{
	// get number of texture tiles
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return 0;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return 0;
	}

	return m_ptr->iTextureAcross * m_ptr->iTextureDown;
}

DARKSDK int GetTilesExistEx ( int iID )
{
	// do tiles exist
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return 0;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return 0;
	}

	if ( m_ptr->lpTexture [ 0 ] )
		return 1;
	else
		return 0;
}

DARKSDK int GetWireframeEx ( int iID )
{
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return 0;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return 0;
	}

	if ( m_ptr->bWireframe )
		return 1;
	else
		return 0;
}

//
// DBPro commands
//

DARKSDK void Set ( int iID, bool bWireframe, bool bTransparency, bool bCull )
{
	// set object properties

	// update internal data
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return;
	}

	// update settings
	m_ptr->bWireframe    = bWireframe;		// wireframe
	m_ptr->bTransparency = bTransparency;	// transparency
	m_ptr->bCull         = bCull;			// cull
}

DARKSDK void Set ( int iID, bool bWireframe, bool bTransparency, bool bCull, int iFilter )
{
	// set object properties

	// update internal data
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return;
	}

	// update settings
	m_ptr->bWireframe    = bWireframe;		// wireframe
	m_ptr->bTransparency = bTransparency;	// transparency
	m_ptr->bCull         = bCull;			// cull
	m_ptr->iFilter       = iFilter;			// texture filter
}

DARKSDK void Set ( int iID, bool bWireframe, bool bTransparency, bool bCull, int iFilter, bool bLight )
{
	// set object properties

	// update internal data
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return;
	}

	// update settings
	m_ptr->bWireframe    = bWireframe;		// wireframe
	m_ptr->bTransparency = bTransparency;	// transparency
	m_ptr->bCull         = bCull;			// cull
	m_ptr->iFilter       = iFilter;			// texture filter
	m_ptr->bLight        = bLight;			// light
}

DARKSDK void Set ( int iID, bool bWireframe, bool bTransparency, bool bCull, int iFilter, bool bLight, bool bFog )
{
	// set object properties

	// update internal data
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return;
	}

	// update settings
	m_ptr->bWireframe    = bWireframe;		// wireframe
	m_ptr->bTransparency = bTransparency;	// transparency
	m_ptr->bCull         = bCull;			// cull
	m_ptr->iFilter       = iFilter;			// texture filter
	m_ptr->bLight        = bLight;			// light
	m_ptr->bFog          = bFog;			// fog
}

DARKSDK void Set ( int iID, bool bWireframe, bool bTransparency, bool bCull, int iFilter, bool bLight, bool bFog, bool bAmbient )
{
	// set object properties

	// update internal data
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return;
	}

	// update settings
	m_ptr->bWireframe    = bWireframe;		// wireframe
	m_ptr->bTransparency = bTransparency;	// transparency
	m_ptr->bCull         = bCull;			// cull
	m_ptr->iFilter       = iFilter;			// texture filter
	m_ptr->bLight        = bLight;			// light
	m_ptr->bFog          = bFog;			// fog
	m_ptr->bAmbient      = bAmbient;		// ambient
}

DARKSDK void  SetPositionVector3 ( int iID, int iVector )
{
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return;
	}
	if ( !g_Types_GetExist ( iVector ) )
	{
		RunTimeError ( RUNTIMEERROR_VECTORNOTEXIST );
		return;
	}
	D3DXVECTOR3 vec = g_Types_GetVector ( iVector );
	MatrixPosition ( iID, vec.x, vec.y, vec.z );
	Apply ( iID );
}

DARKSDK void GetPositionVector3 ( int iVector, int iID )
{
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNUMBERILLEGAL);
		return;
	}
	if ( !UpdateMatrixPtr ( iID ) )
	{
		RunTimeError(RUNTIMEERROR_B3DMATRIXNOTEXISTS);
		return;
	}
	if ( !g_Types_GetExist ( iVector ) )
	{
		RunTimeError ( RUNTIMEERROR_VECTORNOTEXIST );
		return;
	}
	g_Types_SetVector ( iVector, m_ptr->vecPosition.x, m_ptr->vecPosition.y, m_ptr->vecPosition.z );
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////////////
// DARK SDK SECTION //////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

#ifdef DARKSDK_COMPILE

void UpdateMatrix ( void )
{
	Update ( );
}

void UpdateLastMatrix ( void )
{
	UpdateLast ( );
}

void ConstructorMatrix ( HINSTANCE hSetup, HINSTANCE hImage )
{
	Constructor ( hSetup, hImage );
}

void DestructorMatrix ( void )
{
	Destructor ( );
}

void SetErrorHandlerMatrix ( LPVOID pErrorHandlerPtr )
{
	SetErrorHandler ( pErrorHandlerPtr );
}

void PassCoreDataMatrix ( LPVOID pGlobPtr )
{
	PassCoreData ( pGlobPtr );
}

void RefreshD3DMatrix ( int iMode )
{
	RefreshD3D ( iMode );
}

void dbMakeMatrix ( int iID, float fWidth, float fDepth, int iXSegmented, int iZSegmented )
{
	MakeEx ( iID, fWidth, fDepth, iXSegmented, iZSegmented );
}

void dbDeleteMatrix ( int iID )
{
	Delete ( iID );
}

void dbFillMatrix ( int iID, float fHeight, int iTile )
{
	Fill ( iID, fHeight, iTile );
}

void dbGhostMatrixOn ( int iID )
{
	SetTransparencyOn ( iID );
}

void dbGhostMatrixOff ( int iID )
{
	SetTransparencyOff ( iID );
}

void dbPositionMatrix ( int iID, float fX, float fY, float fZ )
{
	PositionEx ( iID, fX, fY, fZ );
}

void dbPrepareMatrixTexture ( int iID, int iImage, int iAcross, int iDown )
{
	PrepareTexture ( iID, iImage, iAcross, iDown );
}

void dbRandomizeMatrix ( int iID, int iHeight )
{
	Randomize ( iID, iHeight );
}

void dbSetMatrixHeight ( int iID, int iX, int iZ, float fHeight )
{
	SetHeight ( iID, iX, iZ, fHeight );
}

void dbSetMatrixNormal ( int iID, int iX, int iZ, float fX, float fY, float fZ )
{
	SetNormal ( iID, iX, iZ, fX, fY, fZ );
}

void dbSetMatrixTexture ( int iID, int iTextureMode, int iMipGen )
{
	SetTexture ( iID, iTextureMode, iMipGen );
}

void dbSetMatrixTile ( int iID, int iX, int iZ, int iTile )
{
	SetTile ( iID, iX, iZ, iTile );
}

void dbSetMatrixWireframeOn ( int iID )
{
	SetWireframeOn ( iID );
}

void dbGhostMatrixOn ( int iID, int iMode )
{
	SetTransparencyOn ( iID, iMode );
}

void dbSetMatrixWireframeOff ( int iID )
{
	SetWireframeOff ( iID );
}

void dbSetMatrix ( int iID, int bWireframe, int bTransparency, int bCull, int iFilter, int bLight, int bFog, int bAmbient )
{
	SetEx ( iID, bWireframe, bTransparency, bCull, iFilter, bLight, bFog, bAmbient );
}

void dbShiftMatrixUp ( int iID )
{
	ShiftUp ( iID );
}

void dbShiftMatrixDown ( int iID )
{
	ShiftDown ( iID );
}

void dbShiftMatrixLeft ( int iID )
{
	ShiftLeft ( iID );
}

void dbShiftMatrixRight ( int iID )
{
	ShiftRight ( iID );
}

void dbUpdateMatrix ( int iID )
{
	Apply ( iID );
}

void dbSetMatrixTrim ( int iID, float fTrimX, float fTrimY )
{
	SetTrim ( iID, fTrimX, fTrimY );
}

void dbSetMatrixPriority ( int iID, int iPriority )
{
	SetPriority ( iID, iPriority );
}

void dbPositionMatrix ( int iID, int iVector )
{
	SetPositionVector3 ( iID, iVector );
}

void dbSetVector3ToMatrixPosition ( int iVector, int iID )
{
	GetPositionVector3 ( iVector, iID );
}

void dbRotateMatrix ( int iID, int iVector )
{
	//SetRotationVector3 ( iID, iVector );
}

void dbSetVector3ToMatrixRotation ( int iVector, int iID )
{
	//GetRotationVector3 ( iVector, iID );
}

float dbMatrixPositionX ( int iID )
{
	DWORD dwReturn = GetPositionXEx ( iID );
	
	return *( float* ) &dwReturn;
}

float dbMatrixPositionY ( int iID )
{
	DWORD dwReturn = GetPositionYEx ( iID );
	
	return *( float* ) &dwReturn;
}

float dbMatrixPositionZ ( int iID )
{
	DWORD dwReturn = GetPositionZEx ( iID );
	
	return *( float* ) &dwReturn;
}

float dbGetGroundHeight ( int iID, float fX, float fZ )
{
	DWORD dwReturn = GetGroundHeightEx ( iID, fX, fZ );
	
	return *( float* ) &dwReturn;
}

float dbGetMatrixHeight ( int iID, int iX, int iZ )
{
	DWORD dwReturn = GetHeightEx ( iID, iX, iZ );
	
	return *( float* ) &dwReturn;
}

int dbMatrixExist ( int iID )
{
	return GetExistEx ( iID );
}

int dbMatrixTileCount ( int iID )
{
	return GetTileCountEx ( iID );
}

int dbMatrixTilesExist ( int iID )
{
	return GetTilesExistEx ( iID );
}

int dbMatrixWireframeState ( int iID )
{
	return GetWireframeEx ( iID );
}

#endif

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
