void PathFinderAdvanced::SetRadius ( float fRadius )
{
	//expand polygons out by the collision radius
	for ( int i = 0; i < (int) sPolygonOrigData_list.size ( ); i++ )
    {
        int iNumV = (int) sPolygonOrigData_list [ i ].sVertexData_list.size ( );
		if ( !sPolygonOrigData_list [ i ].bBlocksPath ) continue;
        
        float fVec0X, fVec0Y;    //prev edge vector
        float fVec1X, fVec1Y;    //curr edge vector
        float fDotP;
		float fX, fY;
        
        fVec0X = sPolygonOrigData_list [ i ].sVertexData_list [ iNumV - 1 ].fNormVX;
        fVec0Y = sPolygonOrigData_list [ i ].sVertexData_list [ iNumV - 1 ].fNormVY;
        
        for ( int j = 0; j < iNumV; j++ )
        {
            fVec1X = sPolygonOrigData_list [ i ].sVertexData_list [ j ].fNormVX;
            fVec1Y = sPolygonOrigData_list [ i ].sVertexData_list [ j ].fNormVY;

			fX = sPolygonOrigData_list [ i ].sVertexData_list [ j ].fX;
			fY = sPolygonOrigData_list [ i ].sVertexData_list [ j ].fY;
            
			if ( fabs( fVec1X*fVec0Y - fVec1Y*fVec0X ) < 0.0000001 ) 
			{
				sPolygonData_list [ i ].sVertexData_list [ j ].fX = fX - ( fRadius * fVec1Y );
				sPolygonData_list [ i ].sVertexData_list [ j ].fY = fY + ( fRadius * fVec1X );				
			}
            else 
			{
				fDotP = fRadius / ( fVec1X*fVec0Y - fVec1Y*fVec0X );
            
				sPolygonData_list [ i ].sVertexData_list [ j ].fX = fX + ( ( fDotP * fVec0X ) - ( fDotP * fVec1X ) );
				sPolygonData_list [ i ].sVertexData_list [ j ].fY = fY + ( ( fDotP * fVec0Y ) - ( fDotP * fVec1Y ) );
			}
        
            fVec0X = fVec1X;
            fVec0Y = fVec1Y;
        }
    }

	fCurrRadius = fRadius;
	SetGridRadius( fRadius * 2.5f );
	//SetGridRadius( 100.0f );

	//BuildWaypoints ( );
}

void PathFinderAdvanced::RemovePolygon ( int id )
{
	bool bRebuild = false;
	
	for ( int i=0; i < (int) sPolygonData_list.size( ); i++ )
	{
		if ( sPolygonData_list [ i ].id == id )
		{
			if ( sPolygonData_list [ i ].bBlocksPath ) bRebuild = true;
			sPolygonData_list.erase ( sPolygonData_list.begin( ) + i );
			i--;
		}
	}

	for ( int i=0; i < (int) sPolygonOrigData_list.size( ); i++ )
	{
		if ( sPolygonOrigData_list [ i ].id == id )
		{
			if ( sPolygonOrigData_list [ i ].bBlocksPath ) bRebuild = true;
			sPolygonOrigData_list.erase ( sPolygonOrigData_list.begin( ) + i );
			i--;
		}
	}

	for ( int i=0; i < (int) sViewBlockingData_list.size( ); i++ )
	{
		if ( sViewBlockingData_list [ i ].id == id )
		{
			sViewBlockingData_list.erase ( sViewBlockingData_list.begin( ) + i );
			i--;
		}
	}

	//if ( bRebuild ) BuildWaypoints ( );
}

bool PathFinderAdvanced::BlockedByDoor( float x1, float y1, float x2, float y2 )
{
	sDoorInfo *pDoor = pAllDoors;
	while ( pDoor )
	{
		float result = CheckRay( pDoor->x1, pDoor->y1, pDoor->x2-pDoor->x1, pDoor->y2-pDoor->y1, x1,y1, x2-x1, y2-y1 );
		if ( result >= 0 ) return true;

		pDoor = pDoor->pNextDoor;
	}

	return false;
}

void PathFinderAdvanced::AddDoor ( int id, float x1, float y1, float x2, float y2 )
{
	// each edge contains a list of doors blocking it
	sWaypoint *pWaypoint = pWaypointList;
	while ( pWaypoint )
	{
		sWaypointEdge *pEdge = pWaypoint->pEdgeList;
		while ( pEdge )
		{
			sWaypoint *pOther = pEdge->pOtherWP;
			float diffX = pOther->fX - pWaypoint->fX;
			float diffY = pOther->fY - pWaypoint->fY;

			float result = CheckRay( x1,y1, x2-x1, y2-y1, pWaypoint->fX, pWaypoint->fY, diffX, diffY );
			if ( result >= 0 )
			{
				// found an edge blocked by this door
				sDoor *pDoor = new sDoor();
				pDoor->id = id;
				pDoor->pNextDoor = pEdge->pDoors;
				pEdge->pDoors = pDoor;
			}
			
			pEdge = pEdge->pNextEdge;
		}

		pWaypoint = pWaypoint->pNextWaypoint;
	}

	// add door to global list of doors
	sDoorInfo *pDoor2 = new sDoorInfo();
	pDoor2->id = id;
	pDoor2->x1 = x1;
	pDoor2->y1 = y1;
	pDoor2->x2 = x2;
	pDoor2->y2 = y2;
	pDoor2->pNextDoor = pAllDoors;
	pAllDoors = pDoor2;
}

void PathFinderAdvanced::RemoveDoor ( int id )
{
	// remove door from every waypoint edge
	sWaypoint *pWaypoint = pWaypointList;
	while ( pWaypoint )
	{
		sWaypointEdge *pEdge = pWaypoint->pEdgeList;
		while ( pEdge )
		{
			sDoor *pDoor = pEdge->pDoors;
			sDoor *pLast = 0;
			while ( pDoor )
			{
				sDoor *pNext = pDoor->pNextDoor;

				if ( pDoor->id == id )
				{
					if ( pLast ) pLast->pNextDoor = pNext;
					else pEdge->pDoors = pNext;
					
					delete pDoor;
				}
				else pLast = pDoor;
					
				pDoor = pNext;
			}
			
			pEdge = pEdge->pNextEdge;
		}

		pWaypoint = pWaypoint->pNextWaypoint;
	}

	// remove from global list of doors
	sDoorInfo *pDoor2 = pAllDoors;
	sDoorInfo *pLast = 0;
	sDoorInfo *pNext = 0;
	while ( pDoor2 )
	{
		pNext = pDoor2->pNextDoor;

		if ( pDoor2->id == id )
		{
			if ( pLast ) pLast->pNextDoor = pNext;
			else pAllDoors = pNext;

			delete pDoor2;
		}
		else pLast = pDoor2;

		pDoor2 = pNext;
	}
}

void PathFinderAdvanced::OptimizeObstacles( )
{	
	// delete old optimized col lists
	if ( pOptimizedCol ) delete pOptimizedCol;
	if ( pOptimizedOrigCol ) delete pOptimizedOrigCol;
	for ( int iZ = 0; iZ < 128; iZ++ )
		for ( int iX = 0; iX < 128; iX++ )
			if ( pOptimizedColGrid[iX][iZ] ) 
				delete pOptimizedColGrid[iX][iZ];

	// set radius for optimized col lists
	if ( fCurrRadius > 0 ) SetRadius( fCurrRadius );
	
	// create pOptimizedCol from sPolygonData_list
	pOptimizedCol = 0;
	vector<sPolygonData>::iterator pIter = sPolygonData_list.begin ( );
    vector<sVertexData>::iterator vIter, vIterPrev;
	int iNumEdges = 0;
	EdgeData *pEdgeList = 0;
    while ( pIter < sPolygonData_list.end ( ) )
    {
		if ( pIter->sVertexData_list.size( ) < 2 ) { pIter++; continue; }
		vIter = pIter->sVertexData_list.begin ( );
        vIterPrev = pIter->sVertexData_list.end ( ) - 1;
        while ( vIter < pIter->sVertexData_list.end ( ) )
        {
            EdgeData *pNewEdge = new EdgeData( );
			pNewEdge->bHalfHeight = pIter->bHalfHeight;
			pNewEdge->fNormVX = vIterPrev->fNormVX;
			pNewEdge->fNormVY = vIterPrev->fNormVY;
			pNewEdge->fX = vIterPrev->fX;
			pNewEdge->fY = vIterPrev->fY;
            pNewEdge->fX2 = vIter->fX;
			pNewEdge->fY2 = vIter->fY;
			pNewEdge->pNextEdge = pEdgeList;
			pEdgeList = pNewEdge;
			iNumEdges++;
			vIterPrev = vIter;
			vIter++;
        }
        pIter++;
    }
	pOptimizedCol = new CollisionNode( iNumEdges, 2, pEdgeList );

	// create pOptimizedOrigCol from sPolygonOrigData_list
	pIter = sPolygonOrigData_list.begin ( );
	iNumEdges = 0;
	pEdgeList = 0;
    while ( pIter < sPolygonOrigData_list.end ( ) )
    {
		if ( pIter->sVertexData_list.size( ) < 2 ) { pIter++; continue; }
		vIter = pIter->sVertexData_list.begin ( );
        vIterPrev = pIter->sVertexData_list.end ( ) - 1;
        while ( vIter < pIter->sVertexData_list.end ( ) )
        {
            EdgeData *pNewEdge = new EdgeData( );
			pNewEdge->bHalfHeight = pIter->bHalfHeight;
			pNewEdge->fNormVX = vIterPrev->fNormVX;
			pNewEdge->fNormVY = vIterPrev->fNormVY;
			pNewEdge->fX = vIterPrev->fX;
			pNewEdge->fY = vIterPrev->fY;
            pNewEdge->fX2 = vIter->fX;
			pNewEdge->fY2 = vIter->fY;
			pNewEdge->pNextEdge = pEdgeList;
			pEdgeList = pNewEdge;
			iNumEdges++;
			vIterPrev = vIter;
			vIter++;
        }
        pIter++;
    }
	pOptimizedOrigCol = new CollisionNode( iNumEdges, 2, pEdgeList );

	// create pOptimizedColGrid from sPolygonData_list for new grid system (to handle LARGE levels without perf spike)
	/* takes AGES and breaks path finding - nice ;)
	for ( int iZ = 0; iZ < 128; iZ++ )
	{
		for ( int iX = 0; iX < 128; iX++ )
		{
			float fBoundX1 = iX*400.0f;
			float fBoundZ1 = iZ*400.0f;
			float fBoundX2 = fBoundX1 + 400.0f;
			float fBoundZ2 = fBoundZ1 + 400.0f;
			vector<sPolygonData>::iterator pIter = sPolygonData_list.begin ( );
			vector<sVertexData>::iterator vIter, vIterPrev;
			int iNumEdges = 0;
			EdgeData *pEdgeList = 0;
			while ( pIter < sPolygonData_list.end ( ) )
			{
				if ( pIter->sVertexData_list.size( ) < 2 ) { pIter++; continue; }
				vIter = pIter->sVertexData_list.begin ( );
				vIterPrev = pIter->sVertexData_list.end ( ) - 1;
				while ( vIter < pIter->sVertexData_list.end ( ) )
				{
					// only add edge if center is within grid being worked on
					float fCenterX = vIter->fX + (( vIterPrev->fX-vIter->fX )/2.0f);
					float fCenterZ = vIter->fY + (( vIterPrev->fY-vIter->fY )/2.0f);
					if ( fCenterX >= fBoundX1 && fCenterX < fBoundX2 )
					{
						if ( fCenterZ >= fBoundZ1 && fCenterZ < fBoundZ2 )
						{
							EdgeData *pNewEdge = new EdgeData( );
							pNewEdge->bHalfHeight = pIter->bHalfHeight;
							pNewEdge->fNormVX = vIterPrev->fNormVX;
							pNewEdge->fNormVY = vIterPrev->fNormVY;
							pNewEdge->fX = vIterPrev->fX;
							pNewEdge->fY = vIterPrev->fY;
							pNewEdge->fX2 = vIter->fX;
							pNewEdge->fY2 = vIter->fY;
							pNewEdge->pNextEdge = pEdgeList;
							pEdgeList = pNewEdge;
							iNumEdges++;
							vIterPrev = vIter;
						}
					}
					vIter++;
				}
				pIter++;
			}

			// add these edges to a specific place within the grid
			if ( iNumEdges > 0 )
				pOptimizedColGrid[iX][iZ] = new CollisionNode( iNumEdges, 2, pEdgeList );
		}
	}
	*/
}

void PathFinderAdvanced::CompleteObstacles( )
{	
	// optimize obstacles
	OptimizeObstacles();

	// finally construct waypoints
	BuildWaypoints( );
}

float PathFinderAdvanced::CheckRay ( float fVx, float fVy, float fDiffVx, float fDiffVy,    //polygon vector
                                     float fPx, float fPy, float fDiffPx, float fDiffPy )   //path ray
{
    float fRel = ( fDiffPy * fDiffVx ) - ( fDiffPx * fDiffVy );
    
    //parallel
    if ( fRel == 0 ) return -1;
    fRel = 1.0f / fRel;
    
    //t  - distance along path ray to collision
    //t2 - distance along polygon vector to collision
    float t  = ( ( fPx-fVx )*fDiffVy - ( fPy-fVy )*fDiffVx ) * fRel;
    float t2 = ( ( fVy-fPy )*fDiffPx - ( fVx-fPx )*fDiffPy ) * fRel;
    
    if ( ( t < 0 || t > 1 ) || ( t2 < 0 || t2 > 1 ) ) return -1;
    
    return t;
}

float PathFinderAdvanced::CheckInfRay ( float fVx, float fVy, float fDiffVx, float fDiffVy,    //polygon vector
                                        float fPx, float fPy, float fDiffPx, float fDiffPy,	   //path ray
										int *iSide )   
{
    float fRel = ( fDiffPy * fDiffVx ) - ( fDiffPx * fDiffVy );
    
    //parallel
    if ( fRel == 0 ) return -1;
    float fInvRel = 1.0f / fRel;
    
	//t  - distance along path ray to collision
    //t2 - distance along polygon vector to collision
	float t  = ( ( fPx-fVx )*fDiffVy - ( fPy-fVy )*fDiffVx ) * fInvRel;
    float t2 = ( ( fVy-fPy )*fDiffPx - ( fVx-fPx )*fDiffPy ) * fInvRel;
    
    if ( t < 0 || t2 < 0 || t2 > 1 ) return -1;
    
	*iSide =  ( fRel > 0 ) ? -1 : 1;
    return t2;
}


float PathFinderAdvanced::CheckSidedRay ( float fVx, float fVy, float fDiffVx, float fDiffVy,    //polygon vector
                                          float fPx, float fPy, float fDiffPx, float fDiffPy,    //path ray
                                          int *iSide )
{
    float fRel = ( fDiffPy * fDiffVx ) - ( fDiffPx * fDiffVy );
    
    //parallel
    if ( fRel == 0 ) return -1;
    float fInvRel = 1.0f / fRel;
    
    //t  - distance along path ray to collision
    //t2 - distance along polygon vector to collision
    float t  = ( ( fPx-fVx )*fDiffVy - ( fPy-fVy )*fDiffVx ) * fInvRel;
    float t2 = ( ( fVy-fPy )*fDiffPx - ( fVx-fPx )*fDiffPy ) * fInvRel;
    
    if ( ( t < 0 || t > 1 ) || ( t2 < 0 || t2 > 1 ) ) return -1;
    
    *iSide =  ( fRel > 0 ) ? -1 : 1;
    return t;
}

//just returns true if the ray hits something, false if not
bool PathFinderAdvanced::QuickPolygonsCheck ( float fSX, float fSY, float fEX, float fEY, int iHeight )
{
    if ( pOptimizedCol )
	{
		return pOptimizedCol->Intersects( fSX, fSY, fEX, fEY, iHeight );
	}
	else
	{
		// no obstacles created for non-zero containers, so assume no collisions (allowing free movement of AI inside non-zero containers)
		return false;
	}
	/* broken!!
	int iGridSX = fSX/400.0f;
	int iGridSZ = fSY/400.0f;
	int iGridEX = fEX/400.0f;
	int iGridEZ = fEY/400.0f;
	if ( iGridSX < 0 ) iGridSX = 0;
	if ( iGridSZ < 0 ) iGridSZ = 0;
	if ( iGridEX < 0 ) iGridEX = 0;
	if ( iGridEZ < 0 ) iGridEZ = 0;
	if ( iGridSX > 127 ) iGridSX = 127;
	if ( iGridSZ > 127 ) iGridSZ = 127;
	if ( iGridEX > 127 ) iGridEX = 127;
	if ( iGridEZ > 127 ) iGridEZ = 127;
	for ( int iRefZ = iGridSZ; iRefZ <= iGridEZ; iRefZ++ )
	{
		for ( int iRefX = iGridSX; iRefX <= iGridEX; iRefX++ )
		{
			CollisionNode* pColNode = pOptimizedColGrid[iRefX][iRefZ];
			if ( pColNode )
			{
				if ( pColNode->Intersects( fSX, fSY, fEX, fEY, iHeight ) == true )
				{
					// hit, can exit early
					return true;
				}
			}
		}
	}
	return false;
	*/
	// 120417 - when have 13000 edges, this spikes performance!!
    //if ( pOptimizedCol )
	//{
	//	return pOptimizedCol->Intersects( fSX, fSY, fEX, fEY, iHeight );
	//}
	/*
	else
	{
		vector<sPolygonData>::iterator pIter = sPolygonData_list.begin ( );
		vector<sVertexData>::iterator vIter, vIterNext;
		float x, y, vx, vy;
		float fResult;
		int iSide = 0;
	    
		while ( pIter < sPolygonData_list.end ( ) )
		{
			if ( pIter->sVertexData_list.size( ) < 2 ) { pIter++; continue; }
			if ( pIter->bHalfHeight && iHeight == 1 ) { pIter++; continue; }
			if ( !pIter->bHalfHeight && iHeight == 0 ) { pIter++; continue; }
			
			vIter = pIter->sVertexData_list.begin ( );
			vIterNext = vIter + 1;
	        
			x = vIter->fX;
			y = vIter->fY;
	        
			while ( vIter < pIter->sVertexData_list.end ( ) )
			{
				vx = vIterNext->fX - x;
				vy = vIterNext->fY - y;
	            
				fResult = CheckSidedRay ( x, y, vx, vy, fSX, fSY, fEX - fSX, fEY - fSY, &iSide );
	            
				if ( fResult >= 0 && iSide > 0 ) return true;
	            
				x += vx;
				y += vy;
	            
				vIter++;
				if ( vIter < pIter->sVertexData_list.end ( ) - 1 ) vIterNext++;
				else vIterNext = pIter->sVertexData_list.begin ( );
			}
	        
			pIter++;    
		}
	    
		return false;
	}
	*/
}

bool PathFinderAdvanced::QuickPolygonsCheckVisible ( float fSX, float fSY, float fEX, float fEY, int iHeight )
{
    if ( pOptimizedOrigCol )
	{
		if ( pOptimizedOrigCol->Intersects( fSX, fSY, fEX, fEY, iHeight ) ) return true;
	}

	vector<sPolygonData>::iterator pIter = sViewBlockingData_list.begin ( );
	vector<sVertexData>::iterator vIter, vIterNext;
	float x, y, vx, vy;
	float fResult;
	int iSide = 0;
    
	while ( pIter < sViewBlockingData_list.end ( ) )
	{
		if ( pIter->sVertexData_list.size( ) < 2 ) { pIter++; continue; }
		if ( pIter->bHalfHeight && iHeight == 1 ) { pIter++; continue; }
		if ( !pIter->bHalfHeight && iHeight == 0 ) { pIter++; continue; }

		vIter = pIter->sVertexData_list.begin ( );
		vIterNext = vIter + 1;
        
		x = vIter->fX;
		y = vIter->fY;
        
		while ( vIter < pIter->sVertexData_list.end ( ) )
		{
			vx = vIterNext->fX - x;
			vy = vIterNext->fY - y;
            
			fResult = CheckSidedRay ( x, y, vx, vy, fSX, fSY, fEX - fSX, fEY - fSY, &iSide );
            
			if ( fResult >= 0 && iSide > 0 ) return true;
            
			x += vx;
			y += vy;
            
			vIter++;
			if ( vIter < pIter->sVertexData_list.end ( ) - 1 ) vIterNext++;
			else vIterNext = pIter->sVertexData_list.begin ( );
		}
        
		pIter++;    
	}

	return false;
	//}

	//return false;
	/*
	else
	{
		vector<sPolygonData>::iterator pIter = sPolygonOrigData_list.begin ( );
		vector<sVertexData>::iterator vIter, vIterNext;
		float x, y, vx, vy;
		float fResult;
		int iSide = 0;
	    
		while ( pIter < sPolygonOrigData_list.end ( ) )
		{
			if ( pIter->sVertexData_list.size( ) < 2 ) { pIter++; continue; }
			if ( pIter->bHalfHeight && iHeight == 1 ) { pIter++; continue; }
			if ( !pIter->bHalfHeight && iHeight == 0 ) { pIter++; continue; }

			vIter = pIter->sVertexData_list.begin ( );
			vIterNext = vIter + 1;
	        
			x = vIter->fX;
			y = vIter->fY;
	        
			while ( vIter < pIter->sVertexData_list.end ( ) )
			{
				vx = vIterNext->fX - x;
				vy = vIterNext->fY - y;
	            
				fResult = CheckSidedRay ( x, y, vx, vy, fSX, fSY, fEX - fSX, fEY - fSY, &iSide );
	            
				if ( fResult >= 0 && iSide > 0 ) return true;
	            
				x += vx;
				y += vy;
	            
				vIter++;
				if ( vIter < pIter->sVertexData_list.end ( ) - 1 ) vIterNext++;
				else vIterNext = pIter->sVertexData_list.begin ( );
			}
	        
			pIter++;    
		}
	    
		return false;
	 }
	 */
}

float PathFinderAdvanced::FindClosestPolygon ( float fSX, float fSY, float fEX, float fEY )
{
	vector<sPolygonData>::iterator pIter = sPolygonData_list.begin ( );
    vector<sVertexData>::iterator vIter;
    float x, y, vx, vy;
    float fResult;
	float fClosestResult = -1;
	bool bFound = false;

    while ( pIter < sPolygonData_list.end ( ) )
    {
		if ( pIter->sVertexData_list.size( ) < 2 ) 
		{
			pIter++;
			continue;
		}
		
		vIter = pIter->sVertexData_list.end( ) - 1;
		x = vIter->fX;
        y = vIter->fY;

		vIter = pIter->sVertexData_list.begin ( );
        
        while ( vIter < pIter->sVertexData_list.end ( ) )
        {
            vx = vIter->fX - x;
            vy = vIter->fY - y;
			int iSide = 0;
            
            fResult = CheckSidedRay ( x, y, vx, vy, fSX, fSY, fEX - fSX, fEY - fSY, &iSide );
            
            if ( fResult >= 0 && iSide > 0 ) 
			{
				if ( fResult < fClosestResult || !bFound )
				{
					fClosestResult = fResult;
					bFound = true;
				}
			}
            
            x += vx;
            y += vy;
            
            vIter++;
        }
        
        pIter++;    
    }

	return fClosestResult;
}

void PathFinderAdvanced::FindClosestOutsidePoint ( float *pX, float *pY )
{
	vector < sPolygonData >::iterator pIter = sPolygonData_list.begin ( );
	vector<sVertexData>::iterator vIter;
    float fX, fY, fVnX, fVnY, vx, vy;

	while ( pIter < sPolygonData_list.end ( ) )
	{
		if ( pIter->sVertexData_list.size( ) < 2 )
		{
			pIter++;
			continue;
		}
		
		vIter = pIter->sVertexData_list.end ( ) - 1;
		fX = vIter->fX;
		fY = vIter->fY;
		fVnX = vIter->fNormVX;
		fVnY = vIter->fNormVY;
		vIter = pIter->sVertexData_list.begin ( );

		int iCountSide = 0;
		float fClosestDist = -1.0f;
		float fClosestX = 0.0f;
		float fClosestY = 0.0f;
		float fClosestIntersect = -1.0f;
		int iClosestSide = 0;

		float fEX = ( vIter->fX + fX ) / 2.0f;
		float fEY = ( vIter->fY + fY ) / 2.0f;
		//fEX = fEX + ( fEX - *pX )*0.1f;
		//fEY = fEY + ( fEY - *pY )*0.1f;

		float fDiffX = fEX - *pX;
		float fDiffY = fEY - *pY;
		float dist = fDiffX*fDiffX + fDiffY*fDiffY;
		fEX = 100000 * fEX / dist;
		fEY = 100000 * fEY / dist;

		//CHECK THIS CODE

		while ( vIter < pIter->sVertexData_list.end ( ) )
		{
			vx = vIter->fX - fX;
			vy = vIter->fY - fY;

			int iSide = 0;
			float fResult = CheckSidedRay ( fX, fY, vx, vy, *pX, *pY, fEX - *pX, fEY - *pY, &iSide );

			if ( fResult >= 0.0f )
			{
				if ( fClosestIntersect < 0 || fResult < fClosestIntersect ) 
				{
					fClosestIntersect = fResult;
					iClosestSide = iSide;
				}
			}

			float fLength = vx*vx + vy*vy;
			float fDotP = ( ( (*pX) - fX ) * vx + ( (*pY) - fY ) * vy );
			
			if ( fLength > 0.001f ) fDotP /= fLength;
			else fDotP = 0.0;

			if ( fDotP < 0.0f ) fDotP = 0.0f;
			if ( fDotP > 1.0f ) fDotP = 1.0f;

			float fDiffX = ( fX + vx*fDotP ) - (*pX);
			float fDiffY = ( fY + vy*fDotP ) - (*pY);

			float fDist = fDiffX*fDiffX + fDiffY*fDiffY;
			if ( fDist < fClosestDist || fClosestDist < 0.0f )
			{
				fClosestDist = fDist;
				fClosestX = fX + vx*fDotP - fVnY*fCurrRadius*0.1f;
				fClosestY = fY + vy*fDotP + fVnX*fCurrRadius*0.1f;
			}

			fX = vIter->fX;
			fY = vIter->fY;
			fVnX = vIter->fNormVX;
			fVnY = vIter->fNormVY;
			vIter++;
		}

		if ( iClosestSide < 0 )
		{
			*pX = fClosestX;
			*pY = fClosestY;
		}

		pIter++;
	}
}

//returns the number of times the ray crosses an edge
int PathFinderAdvanced::CountCrossings ( float fSX, float fSY, float fEX, float fEY )
{
    vector<sPolygonData>::iterator pIter = sPolygonData_list.begin ( );
    vector<sVertexData>::iterator vIter, vIterNext;
    float x, y, vx, vy;
    float fResult;
    
    int iCount = 0;
    
    while ( pIter < sPolygonData_list.end ( ) )
    {
        vIter = pIter->sVertexData_list.begin ( );
        vIterNext = vIter + 1;
        
        x = vIter->fX;
        y = vIter->fY;
        
        while ( vIter < pIter->sVertexData_list.end ( ) )
        {
            vx = vIterNext->fX - x;
            vy = vIterNext->fY - y;
            
            fResult = CheckRay ( x, y, vx, vy, fSX, fSY, fEX - fSX, fEY - fSY );
            
            if ( fResult >= 0 ) iCount++;
            
            x += vx;
            y += vy;
            
            vIter++;
            if ( vIter < pIter->sVertexData_list.end ( ) - 1 ) vIterNext++;
            else vIterNext = pIter->sVertexData_list.begin ( );
        }
        
        pIter++;    
    }
    
    return iCount;
}

//returns non zero if start and end points are on different sides (inside, outside)
int PathFinderAdvanced::CountSidedness ( float fSX, float fSY, float fEX, float fEY )
{
    vector<sPolygonData>::iterator pIter = sPolygonData_list.begin ( );
    vector<sVertexData>::iterator vIter, vIterNext;
    float x, y, vx, vy;
    float fResult;
    
    int iCount = 0;
    
    while ( pIter < sPolygonData_list.end ( ) )
    {
        vIter = pIter->sVertexData_list.begin ( );
        vIterNext = vIter + 1;
        
        x = vIter->fX;
        y = vIter->fY;
        
        while ( vIter < pIter->sVertexData_list.end ( ) )
        {
            vx = vIterNext->fX - x;
            vy = vIterNext->fY - y;
            
			int iSide = 0;
            fResult = CheckSidedRay ( x, y, vx, vy, fSX, fSY, fEX - fSX, fEY - fSY, &iSide );
            
            if ( fResult >= 0 ) iCount += iSide;
            
            x += vx;
            y += vy;
            
            vIter++;
            if ( vIter < pIter->sVertexData_list.end ( ) - 1 ) vIterNext++;
            else vIterNext = pIter->sVertexData_list.begin ( );
        }
        
        pIter++;    
    }
    
    return iCount;
}

//counts the number of polygons the point is in
int PathFinderAdvanced::InPolygons ( float fSX, float fSY )
{
    vector<sPolygonData>::iterator pIter = sPolygonData_list.begin ( );
    vector<sVertexData>::iterator vIter, vIterNext;
    float x, y, vx, vy;
    float fResult;
    
    int iCount = 0;
    
    while ( pIter < sPolygonData_list.end ( ) )
    {
		if ( pIter->sVertexData_list.size( ) < 2 )
		{
			pIter++;
			continue;
		}

		vIter = pIter->sVertexData_list.begin ( );
        vIterNext = vIter + 1;
        
        x = vIter->fX;
        y = vIter->fY;

		float fEX = ( vIter->fX + vIterNext->fX ) / 2;
		float fEY = ( vIter->fY + vIterNext->fY ) / 2;

		float dist = sqrt((fEX - fSX)*(fEX - fSX) + (fEY - fSY)*(fEY - fSY));
		fEX = 100000 * (fEX - fSX) / dist;
		fEY = 100000 * (fEY - fSY) / dist;

		//fEX = fEX + ( fEX - fSX )*0.1f;
		//fEY = fEY + ( fEY - fSY )*0.1f;

		int iClosestSide = 0;
		float fClosestIntersect = -1.0f;
        
        while ( vIter < pIter->sVertexData_list.end ( ) )
        {
            vx = vIterNext->fX - x;
            vy = vIterNext->fY - y;
            
			int iSide = 0;
            fResult = CheckSidedRay ( x, y, vx, vy, fSX, fSY, fEX - fSX, fEY - fSY, &iSide );
            
            if ( fResult >= 0 )
			{
				if ( fClosestIntersect < 0 || fResult < fClosestIntersect )
				{
					fClosestIntersect = fResult;
					iClosestSide = iSide;
				}
			}
            
            x += vx;
            y += vy;
            
            vIter++;
            if ( vIter < pIter->sVertexData_list.end ( ) - 1 ) vIterNext++;
            else vIterNext = pIter->sVertexData_list.begin ( );
        }

		if ( fClosestIntersect >= 0 && iClosestSide < 0 )
		{
			iCount++;
		}
        
        pIter++;
    }
    
    return iCount;
}

void PathFinderAdvanced::ActivateAllWaypoints ( )
{
	sWaypoint *pWaypoint = pWaypointList;

	while ( pWaypoint )
	{
		pWaypoint->bActive = true;
		pWaypoint = pWaypoint->pNextWaypoint;
	}
}

void PathFinderAdvanced::DeActivateWaypoint ( sWaypoint *pWaypoint )
{
	if ( !pWaypoint ) return;
	
	pWaypoint->bActive = false;
}

//gauranteed to find a path if one exists
bool PathFinderAdvanced::ShortestPath ( float fEX, float fEY, Path* pBuildPath, int iDestinationContainer )
{
    if ( !pBuildPath ) return false;
    
    bool bFound = false;
    
    sWaypoint *pWaypoint, *pNextWP;
    vector<sWaypointInfo>::iterator wInfoIter;
    //vector<sWaypointEdge>::iterator wEdgeIter;
	sWaypointEdge *pEdge = 0;
    sWaypointInfo* pNewInfo;
    
//    char str [ 256 ];
    int iIterations = 0;
    
    while ( !bFound && sWaypointOpen_list.size ( ) > 0 )
    {        
        iIterations++;
        
        //get the shortest path so far
        wInfoIter = sWaypointOpen_list.begin ( );
        
        pWaypoint = wInfoIter->pThisWP;
		
		if ( !pWaypoint )
		{
			pop_heap ( sWaypointOpen_list.begin ( ), sWaypointOpen_list.end ( ) );
			sWaypointOpen_list.pop_back ( );
			continue;
		}

		//get the edge list for this waypoint
        //wEdgeIter = wIter->sEdge_list.begin ( );
		pEdge = pWaypoint->pEdgeList;
        
		if ( pWaypoint->fDistH < 1 && pWaypoint->pContainer->GetID( ) == iDestinationContainer )
        {
            bFound = true;
            break;
            //endpoint sits on top of the open list, use it to jump back to work out the path
        }
        
        pWaypoint->iVisited = 2;
		int iCount = 0;

		if ( pWaypoint->iNumEdges > 0 )
		{
        
			//build the data for these new edges ready to add to the open list
			pNewInfo = new sWaypointInfo [ pWaypoint->iNumEdges ];
	        
			while ( pEdge )
			{
				pNextWP = pEdge->pOtherWP;

				if ( !pNextWP )
				{
					pEdge = pEdge->pNextEdge;
					continue;
				}

				if ( pEdge->pDoors )
				{
					// edge is blocked by a door, skip it
					pEdge = pEdge->pNextEdge;
					continue;
				}
	            
				//if ( wNextIter->bActive )
				{
					if ( pNextWP->iVisited == 0 )
					{
						pNextWP->pParent = pWaypoint;
						pNextWP->fDistG = pWaypoint->fDistG + pEdge->fCost + pNextWP->fWPCost;
						pNextWP->fDistH = EstimateDistance ( pNextWP->fX, pNextWP->fY, fEX, fEY );
						pNextWP->fDistF = pNextWP->fDistG + pNextWP->fDistH;
						pNextWP->iVisited = 1;
		                
						if ( iCount < pWaypoint->iNumEdges )
						{
							pNewInfo [ iCount ].pThisWP = pNextWP;
							pNewInfo [ iCount ].fCost = pNextWP->fDistF;
							iCount++;
						}
						else
						{
							// this should not happen - investigate (MP finestgladelevel)
						}
					}
					else if ( pNextWP->iVisited == 1 )
					{
						if ( pWaypoint->fDistG + pEdge->fCost + pNextWP->fWPCost < pNextWP->fDistG )
						{
							pNextWP->pParent = pWaypoint;
							pNextWP->fDistG = pWaypoint->fDistG + pEdge->fCost + pNextWP->fWPCost;
							pNextWP->fDistH = EstimateDistance ( pNextWP->fX, pNextWP->fY, fEX, fEY );
							pNextWP->fDistF = pNextWP->fDistG + pNextWP->fDistH;
		                    
							if ( iCount < pWaypoint->iNumEdges )
							{
								pNewInfo [ iCount ].pThisWP = pNextWP;
								pNewInfo [ iCount ].fCost = pNextWP->fDistF;
								iCount++;
							}
							else
							{
								// this should not happen - investigate (MP finestgladelevel)
							}
						}
					}
				}
	            
				pEdge = pEdge->pNextEdge;
			}

			pop_heap ( sWaypointOpen_list.begin ( ), sWaypointOpen_list.end ( ) );
			sWaypointOpen_list.pop_back ( );
	        
			for (int i = 0; i < iCount; i++ )
			{
				sWaypointOpen_list.push_back ( pNewInfo [ i ] );
				push_heap ( sWaypointOpen_list.begin ( ), sWaypointOpen_list.end ( ) );
			}
	        
			delete [] pNewInfo;
		}
		else
		{
			pop_heap ( sWaypointOpen_list.begin ( ), sWaypointOpen_list.end ( ) );
			sWaypointOpen_list.pop_back ( );
		}
    }
    
//    sprintf ( str, "Iterations: %d", iIterations );
//    TextOut( debugOutput, 5, 100, str, strlen ( str ) );
    
    if ( bFound )
    {
        wInfoIter = sWaypointOpen_list.begin ( );
        pWaypoint = wInfoIter->pThisWP;
		pBuildPath->InsertPoint ( 0, pWaypoint->fX, pWaypoint->fY, pWaypoint->pContainer->GetID( ) );
        
        do
        {
            pWaypoint = pWaypoint->pParent;
            pBuildPath->InsertPoint ( 0, pWaypoint->fX, pWaypoint->fY, pWaypoint->pContainer->GetID( ) );
            
        } while ( pWaypoint != pWaypoint->pParent );
        
        return true;
    }
    
    return false;
}

//asumes polygons all have a clockwise winding order
void PathFinderAdvanced::CalculatePath ( float fSX, float fSY, float fEX, float fEY,    //points to check between
                                         Path* pFinalPath, 								//place to store the final path
										 float fMaxEdgeCost, 							//max distance without crossing a waypoint
										 int iDestinationContainer )
{	
	if ( !pFinalPath ) return;
    
    //nothing hit
	/*if ( iDestinationContainer == pOwner->GetID( ) && !QuickPolygonsCheck ( fSX, fSY, fEX, fEY, 2 ) && !BlockedByDoor(fSX, fSY, fEX, fEY) )
    {
        pFinalPath->AddPoint( fSX, 0, fSY, iDestinationContainer );
        pFinalPath->AddPoint( fEX, 0, fEY, iDestinationContainer );
        return;
    }*/

	PathFinderAdvanced *pDestPathFinder = this;

	if ( iDestinationContainer >= 0 ) 
	{
		Container *pDestContainer = cWorld.GetContainer( iDestinationContainer );
		if ( !pDestContainer ) return;
		pDestPathFinder = pDestContainer->pPathFinder;
	}

	//iResult = CountSidedness ( fSX, fSY, fEX, fEY );
    //points on different sides (inside, outside), impossible to solve with enclosed polygons
    //if ( iResult != 0 )
    //{
    //    return;
    //}
	
	//int iStartPointSide = CountInfSidedness ( fSX, fSY );
	//int iEndPointSide = CountInfSidedness ( fEX, fEY );
    
	
	//if ( iStartPointSide != 0 )
	{
		//vector < sWaypoint >::iterator wIter = GetClosestWaypointIter ( fSX, fSY );
		//fSX = wIter->fX;
		//fSY = wIter->fY;
		FindClosestOutsidePoint ( &fSX, &fSY );
	}

	//if ( iEndPointSide != 0 )
	{
		//vector < sWaypoint >::iterator wIter = GetClosestWaypointIter ( fEX, fEY );
		//fEX = wIter->fX;
		//fEY = wIter->fY;
		pDestPathFinder->FindClosestOutsidePoint ( &fEX, &fEY );
	}

	if ( iDestinationContainer == pOwner->GetID( ) && !QuickPolygonsCheck ( fSX, fSY, fEX, fEY, 2 ) && !BlockedByDoor(fSX, fSY, fEX, fEY) )
    {
		if ( fMaxEdgeCost < 0 || (fSX-fEX)*(fSX-fEX) + (fSY-fEY)*(fSY-fEY) < fMaxEdgeCost*fMaxEdgeCost )
		{
			pFinalPath->AddPoint( fSX, 0, fSY );
			pFinalPath->AddPoint( fEX, 0, fEY );
			return;
		}
    }

	if ( !pWaypointList || !pDestPathFinder->pWaypointList ) return;

    sWaypoint *pStartPoint = new sWaypoint();
	sWaypoint *pEndPoint = new sWaypoint();
    
    pStartPoint->fX = fSX;
    pStartPoint->fY = fSY;
    pStartPoint->fDistG = 0;
    pStartPoint->fDistH = EstimateDistance ( fSX, fSY, fEX, fEY );
    pStartPoint->fDistF = pStartPoint->fDistG + pStartPoint->fDistH;
    pStartPoint->iVisited = 1;
	pStartPoint->pEdgeList = 0;
	pStartPoint->iNumEdges = 0;
	pStartPoint->pParent = pStartPoint;
	pStartPoint->pContainer = pOwner;

    pEndPoint->fX = fEX;
    pEndPoint->fY = fEY;
    pEndPoint->fDistG = 0;
    pEndPoint->fDistH = 0;
    pEndPoint->fDistF = 0;
    pEndPoint->iVisited = 0;
	pEndPoint->pEdgeList = 0;
	pEndPoint->iNumEdges = 0;
	pEndPoint->pParent = 0;
	pEndPoint->pContainer = pDestPathFinder->pOwner;

	int result = WaitForSingleObject( hPathFindingMutex, INFINITE );
	if ( result == WAIT_FAILED ) 
	{
		char str[64];
		sprintf_s( str, 64, "Failed to lock PF mutex, error: %d", GetLastError( ) );
		MessageBox( NULL, str, "Error", 0 );
		exit(-1);
	}

	cWorld.SetupAllPathFinders( );
    
	//sWaypoint *pWaypoint = pWaypointList;

	UpdateSingleVisibility( pStartPoint, fMaxEdgeCost, false );
	pDestPathFinder->UpdateSingleVisibility( pEndPoint, fMaxEdgeCost );

	AddWaypoint ( pStartPoint );
	pDestPathFinder->AddWaypoint ( pEndPoint );
    
	//start the open list with the start point
    sWaypointOpen_list.clear ( );
    sWaypointInfo sStartPointInfo;
    
    sStartPointInfo.pThisWP = pStartPoint;
    sStartPointInfo.fCost = pStartPoint->fDistF;
    
    sWaypointOpen_list.push_back ( sStartPointInfo );
    
    make_heap ( sWaypointOpen_list.begin ( ), sWaypointOpen_list.end ( ) );

    ShortestPath ( fEX, fEY, pFinalPath, iDestinationContainer );

	sWaypointOpen_list.clear ( );
    
    //remove the start and endpoints from the waypoints, and all edges in between
	pDestPathFinder->RemoveEndWaypoint( pEndPoint );
    
    RemoveLastWaypoint ( );
	pDestPathFinder->RemoveLastWaypoint ( );

	ReleaseMutex( hPathFindingMutex );
}

void PathFinderAdvanced::RemoveEndWaypoint( sWaypoint *pWaypoint )
{
	sWaypointEdge *pEdge = pWaypoint->pEdgeList;
    
    while ( pEdge )
    {
        //last edge added to the other waypoint would have been to the end point
		pEdge->pOtherWP->RemoveLastEdge( );
        
		pEdge = pEdge->pNextEdge;
    }
}

void PathFinderAdvanced::ClearDistanceData( )
{
	sWaypoint *pWaypoint = pWaypointList;
    
	while ( pWaypoint )
    {
		pWaypoint->iVisited = 0;
        pWaypoint->fDistG = 0;
        pWaypoint->fDistH = 0;
        pWaypoint->fDistF = 0;
        
		pWaypoint = pWaypoint->pNextWaypoint;
    }
}

void PathFinderAdvanced::SearchCoverPoints ( float fSX, float fSY, float fTX, float fTY, Path *pPoints )
{
	sCoverPoint *pCoverPoint = pCoverPointList;
	
	//find all cover points visible from this point suitable for hiding behind
	
    while ( pCoverPoint )
    {
		if ( !pCoverPoint->IsInUse() )
		{
			float dirX = fSX - fTX; 
			float dirY = fSY - fTY;
			float dir2X = pCoverPoint->fX - fTX; 
			float dir2Y = pCoverPoint->fY - fTY;

			// is the cover in front of the target
			float dotp = dirX*dir2X + dirY*dir2Y;
			if ( dotp >= 0 )
			{
				// can the entity get to this point
				if ( !QuickPolygonsCheckVisible ( fSX, fSY, pCoverPoint->fX, pCoverPoint->fY, 1 ) )
				{
					float distX = pCoverPoint->fX - fTX;
					float distY = pCoverPoint->fY - fTY;
					float dist = distX*distX + distY*distY;

					// can the entity fire at the target from this point
					if ( dist > fCurrRadius*fCurrRadius*16 && !QuickPolygonsCheckVisible ( pCoverPoint->fX, pCoverPoint->fY, fTX, fTY, 1 ) )
					{
						// is the cover orientated correctly
						float fVX = fTX - pCoverPoint->fX;
						float fVY = fTY - pCoverPoint->fY;
						float length = sqrt(fVX*fVX + fVY*fVY);

						float dotp = (fVX*pCoverPoint->fDirX + fVY*pCoverPoint->fDirY)/length;

						if ( dotp > 0.5f ) 
						{
							// everything checks out
							pPoints->AddPoint ( pCoverPoint->fX, 0, pCoverPoint->fY, pCoverPoint->iID );
						}
					}
				}
			}
		}

		pCoverPoint = pCoverPoint->pNextPoint;
    }
}

void PathFinderAdvanced::SearchPeekingPoints ( float fSX, float fSY, float fTX, float fTY, Path *pPoints, Path *pDirections, Path *pPeekPoints )
{
	sWaypoint *pWaypoint = pWaypointList;
	
	//find all waypoints visible from this point suitable for hiding behind a corner
	
    while ( pWaypoint )
    {
		if ( pWaypoint->bCanPeek )
		{
			float dirX = fSX - fTX; 
			float dirY = fSY - fTY;
			float dir2X = pWaypoint->fX - fTX; 
			float dir2Y = pWaypoint->fY - fTY;

			// is the point in front of the target
			float dotp = dirX*dir2X + dirY*dir2Y;
			if ( dotp >= 0 )
			{
				// can the entity get to this point
				if ( !QuickPolygonsCheckVisible ( fSX, fSY, pWaypoint->fX, pWaypoint->fY, 2 ) )
				{
					float distX = pWaypoint->fX - fTX;
					float distY = pWaypoint->fY - fTY;
					float dist = distX*distX + distY*distY;

					// can the entity fire at the target from this point
					if ( dist > fCurrRadius*fCurrRadius*36 && !QuickPolygonsCheckVisible ( pWaypoint->fX, pWaypoint->fY, fTX, fTY, 1 ) )
					{
						// is the corner orientated correctly
						float fVX = fTX - pWaypoint->fX;
						float fVY = fTY - pWaypoint->fY;

						float dotp = fVX*pWaypoint->fVX + fVY*pWaypoint->fVY;

						if ( dotp >= 0 ) 
						{
							// everything checks out so far, find the correct side of the corner
							float dirX = pWaypoint->fVY;
							float dirY = -pWaypoint->fVX;

							float ang = acos( pWaypoint->fCAngle );
							float sign = 1;
							if ( fVX*dirX + fVY*dirY > 0 )
							{
								ang = -ang;
								sign = -1;
							}

							// check wall is big enough to hide behind
							if ( (sign > 0 && (pWaypoint->bFlags & DARKAI_WAYPOINT_PEEK_RIGHT))
							  || (sign < 0 && (pWaypoint->bFlags & DARKAI_WAYPOINT_PEEK_LEFT)) )
							{
								float x = pWaypoint->fVX*cos(ang) + pWaypoint->fVY*sin(ang);
								float y = pWaypoint->fVY*cos(ang) - pWaypoint->fVX*sin(ang);

								float dx = y*sign;
								float dy = (-x)*sign;

								x = pWaypoint->fX + x * fCurrRadius*1.5f;
								y = pWaypoint->fY + y * fCurrRadius*1.5f;

								pPoints->AddPoint ( x, 0, y );
								pDirections->AddPoint( dx, 0, dy );
								pPeekPoints->AddPoint( pWaypoint->fX, 0, pWaypoint->fY );
							}
						}
					}
				}
			}
		}

		pWaypoint = pWaypoint->pNextWaypoint;
    }
}

void PathFinderAdvanced::SearchPoints ( float fSX, float fSY, Path *pPoints, int iHeight )
{
	sWaypoint *pWaypoint = pWaypointList;
	
	//find all waypoints visible from this point
	
    while ( pWaypoint )
    {
        if ( !QuickPolygonsCheckVisible ( fSX, fSY, pWaypoint->fX, pWaypoint->fY, iHeight ) )
		{
			pPoints->AddPoint ( pWaypoint->fX, 0, pWaypoint->fY, pWaypoint->pContainer->GetID() );
			if ( pWaypoint->bFlags & DARKAI_WAYPOINT_BRIDGE )
			{
				// add waypoint from other container and see what happens
				sWaypointEdge *pEdge = pWaypoint->pEdgeList;
				if ( pEdge ) pPoints->AddPoint ( pEdge->pOtherWP->fX, 0, pEdge->pOtherWP->fY, pEdge->pOtherWP->pContainer->GetID() );
			}
		}

		pWaypoint = pWaypoint->pNextWaypoint;
    }
}

void PathFinderAdvanced::DebugDrawCoverPoints ( float fHeight )
{
}

void PathFinderAdvanced::DebugHideCoverPoints ( )
{      
}

void PathFinderAdvanced::DebugDrawWaypoints ( float fHeight )
{
    sWaypoint *pWaypoint = pWaypointList;
    
	int iTempMesh = dbMakePointMesh ( );
	if ( iTempMesh == 0 ) return;

	if ( iWaypointObject > 0 && ObjectExist ( iWaypointObject ) )	DeleteObject ( iWaypointObject );
	if ( iWaypointObject == 0 ) iWaypointObject = dbFreeObject ( );
	if ( iWaypointObject == 0 ) return;
	
	MakeObjectPlane ( iWaypointObject, 0.0f, 0.0f, 1 );
	SetObjectMask ( iWaypointObject, 1 );
	
	int iLimb = 1;
	while ( pWaypoint )
    {
		AddLimb ( iWaypointObject, iLimb, iTempMesh );
		ScaleLimb ( iWaypointObject, iLimb, 40*fCurrRadius, 40*fCurrRadius, 40*fCurrRadius );
		OffsetLimb ( iWaypointObject, iLimb, pWaypoint->fX, 0.0f, pWaypoint->fY );
		iLimb++;
        pWaypoint = pWaypoint->pNextWaypoint;
    }
	
	DeleteMesh ( iTempMesh );
	dbCombineLimbs ( iWaypointObject );
	PositionObject ( iWaypointObject, 0.0f, fHeight, 0.0f );
	ColorObject ( iWaypointObject, 0xff0000ff );
	SetObjectEmissive ( iWaypointObject, 0xff0000ff );
	SetObjectEffect ( iWaypointObject, g_GUIShaderEffectID );
	SetObjectCollisionOff ( iWaypointObject );
}

void PathFinderAdvanced::DebugHideWaypoints ( )
{      
    if ( iWaypointObject > 0 && ObjectExist ( iWaypointObject ) )
	{
		DeleteObject ( iWaypointObject );
	}
	iWaypointObject = 0;
}

void PathFinderAdvanced::DebugDrawWaypointEdges ( float fHeight )
{  
	sWaypoint *pWaypoint = pWaypointList;
	sWaypointEdge *pEdge;

	int iTempMesh = dbMakeEdgeMesh ( );
	if ( iTempMesh == 0 ) return;

	if ( iWaypointEdgeObject > 0 && ObjectExist ( iWaypointEdgeObject ) )
	{
		DeleteObject ( iWaypointEdgeObject );
	}

	if ( iWaypointEdgeObject == 0 ) iWaypointEdgeObject = dbFreeObject ( );
	if ( iWaypointEdgeObject == 0 ) return;
	
	MakeObjectPlane ( iWaypointEdgeObject, 0.0f, 0.0f, 1 );
	SetObjectMask ( iWaypointEdgeObject, 1 );
	
	// work objects to work out orientations
	int iTempObj1 = dbFreeObject ( );
	if ( iTempObj1 == 0 ) return;
	MakeObjectCube ( iTempObj1, 25.0f );

	int iLimb = 1;
    while ( pWaypoint )
    {
        //wEdgeIter = wIter->sEdge_list.begin ( );
		pEdge = pWaypoint->pEdgeList;
        
        while ( pEdge )
        {
			float fX = ( pEdge->pOtherWP->fX + pWaypoint->fX ) / 2.0f;
			float fZ = ( pEdge->pOtherWP->fY + pWaypoint->fY ) / 2.0f;
			float fDirX = pEdge->pOtherWP->fX - pWaypoint->fX;
			float fDirZ = pEdge->pOtherWP->fY - pWaypoint->fY;
			float fDist = sqrt ( fDirX*fDirX + fDirZ*fDirZ );
			if ( fDist < 1.0f ) 
			{
				pEdge = pEdge->pNextEdge;
				continue;
			}
			fDirX /= fDist;
			fDirZ /= fDist;
			float fAngY = acos ( fDirZ ) * 57.295779513f;
			if ( fDirX < 0.0f ) fAngY = 360.0f - fAngY;

			//AddLimb ( iWaypointEdgeObject, iLimb, iTempMesh );
			//ScaleLimb ( iWaypointEdgeObject, iLimb, 40*fCurrRadius, 100.0f, fDist * 100.0f );
			//RotateLimb ( iWaypointEdgeObject, iLimb, 0.0f, fAngY, 0.0f );
			//OffsetLimb ( iWaypointEdgeObject, iLimb, fX, 0.0f, fZ );
			AddLimb ( iWaypointEdgeObject, iLimb, iTempMesh );
			float fRealStartY = GetLUATerrainHeightEx ( pWaypoint->fX, pWaypoint->fY );
			PositionObject ( iTempObj1, pWaypoint->fX, fRealStartY, pWaypoint->fY );
			float fRealFinishY = GetLUATerrainHeightEx ( pEdge->pOtherWP->fX, pEdge->pOtherWP->fY );
			fDirX = pEdge->pOtherWP->fX - pWaypoint->fX;
			float fDirY = fRealFinishY - fRealStartY;
			fDirZ = pEdge->pOtherWP->fY - pWaypoint->fY;
			fDist = sqrt ( fDirX*fDirX + fDirY*fDirY + fDirZ*fDirZ );
			ScaleLimb ( iWaypointEdgeObject, iLimb, 40*fCurrRadius, 100.0f, fDist * 100.0f );
			PointObject ( iTempObj1, pEdge->pOtherWP->fX, fRealFinishY, pEdge->pOtherWP->fY );
			float fRealMiddleY = fRealStartY+((fRealFinishY-fRealStartY)/2.0f);
			OffsetLimb ( iWaypointEdgeObject, iLimb, fX, fRealMiddleY + 7.5f, fZ );
			RotateLimb ( iWaypointEdgeObject, iLimb, ObjectAngleX(iTempObj1), ObjectAngleY(iTempObj1), ObjectAngleZ(iTempObj1) );

			iLimb++;
			pEdge = pEdge->pNextEdge;
        }
        
		pWaypoint = pWaypoint->pNextWaypoint;
    }

	DeleteMesh ( iTempMesh );

	// delete temp object
	if ( ObjectExist(iTempObj1) == 1 ) DeleteObject ( iTempObj1 );

	dbCombineLimbs ( iWaypointEdgeObject );

	PositionObject ( iWaypointEdgeObject, 0.0f, fHeight, 0.0f );
	ColorObject ( iWaypointEdgeObject, 0xff0000ff );
	SetObjectEmissive ( iWaypointEdgeObject, 0xff0000ff );
	SetObjectEffect ( iWaypointEdgeObject, g_GUIShaderEffectID );

	SetObjectCollisionOff ( iWaypointEdgeObject );
}

void PathFinderAdvanced::DebugHideWaypointEdges ( )
{  
	if ( iWaypointEdgeObject > 0 && ObjectExist ( iWaypointEdgeObject ) )
	{
		DeleteObject ( iWaypointEdgeObject );
	}

	iWaypointEdgeObject = 0;
}

void PathFinderAdvanced::DebugDrawPolygonBounds ( float fHeight )
{
	vector < sPolygonData >::iterator pIter = sPolygonData_list.begin ( );
	vector < sVertexData >::iterator vIter, vIterPrev;
	
	//make a plane mesh to represent edges.
	int iTempMesh = dbMakeEdgeMesh ( );
	if ( iTempMesh == 0 ) return;
	
	//clear any current debug object
	if ( iPolygonBoundsObject > 0 && ObjectExist ( iPolygonBoundsObject ) == 1 )
	{
		DeleteObject ( iPolygonBoundsObject );
	}
	
	//find a free object
	if ( iPolygonBoundsObject == 0 ) iPolygonBoundsObject = dbFreeObject ( );
	if ( iPolygonBoundsObject == 0 ) return;

	//root node
	MakeObjectPlane ( iPolygonBoundsObject, 0.0f, 0.0f, 1 );
	SetObjectMask ( iPolygonBoundsObject, 1 );	

	//do the same for the half height debug object
	if ( iHHPolygonBoundsObject > 0 && ObjectExist ( iHHPolygonBoundsObject ) == 1 )
	{
		DeleteObject ( iHHPolygonBoundsObject );
	}

	if ( iHHPolygonBoundsObject == 0 ) iHHPolygonBoundsObject = dbFreeObject ( );
	if ( iHHPolygonBoundsObject == 0 ) return;

	//root node
	MakeObjectPlane ( iHHPolygonBoundsObject, 0.0f, 0.0f, 1 );
	SetObjectMask ( iHHPolygonBoundsObject, 1 );	
	
	int iLimb = 1;
	int iLimb2 = 1;

	// work objects to work out orientations
	int iTempObj1 = dbFreeObject ( );
	if ( iTempObj1 == 0 ) return;
	MakeObjectCube ( iTempObj1, 25.0f );

	//for all obstacles (polygons)
	while ( pIter < sPolygonData_list.end ( ) )
    {
		if ( pIter->bBlocksPath && pIter->sVertexData_list.size ( ) > 1 )
		{
			vIter = pIter->sVertexData_list.begin ( );
			vIterPrev = pIter->sVertexData_list.end ( ) - 1;
			
			//for all vertices (edges)
			while ( vIter < pIter->sVertexData_list.end ( ) )
			{
				// get average position and direction of this edge
				float fX = ( vIter->fX + vIterPrev->fX ) / 2.0f;
				float fZ = ( vIter->fY + vIterPrev->fY ) / 2.0f;
				float fDirX = vIter->fX - vIterPrev->fX;
				float fDirZ = vIter->fY - vIterPrev->fY;
				float fDist = sqrt ( fDirX*fDirX + fDirZ*fDirZ );
				if ( fDist < 0.1f ) 
				{
					vIterPrev = vIter;
					vIter++;
					continue;
				}
				fDirX /= fDist;
				fDirZ /= fDist;
				float fAngY = acos ( fDirZ ) * 57.295779513f;
				if ( fDirX < 0.0f ) fAngY = 360.0f - fAngY;

				// add to the relevant object, half height or full height
				if ( pIter->bHalfHeight )
				{
					AddLimb ( iHHPolygonBoundsObject, iLimb2, iTempMesh );
					float fRealStartY = GetLUATerrainHeightEx ( vIterPrev->fX, vIterPrev->fY );
					PositionObject ( iTempObj1, vIterPrev->fX, fRealStartY, vIterPrev->fY );
					float fRealFinishY = GetLUATerrainHeightEx ( vIter->fX, vIter->fY );
					fDirX = vIter->fX - vIterPrev->fX;
					float fDirY = fRealFinishY - fRealStartY;
					fDirZ = vIter->fY - vIterPrev->fY;
					fDist = sqrt ( fDirX*fDirX + fDirY*fDirY + fDirZ*fDirZ );
					ScaleLimb ( iHHPolygonBoundsObject, iLimb2, 40*fCurrRadius, 100.0f, fDist * 100.0f );
					PointObject ( iTempObj1, vIter->fX, fRealFinishY, vIter->fY );
					float fRealMiddleY = fRealStartY+((fRealFinishY-fRealStartY)/2.0f);
					OffsetLimb ( iHHPolygonBoundsObject, iLimb2, fX, fRealMiddleY + 7.5f, fZ );
					RotateLimb ( iHHPolygonBoundsObject, iLimb2, ObjectAngleX(iTempObj1), ObjectAngleY(iTempObj1), ObjectAngleZ(iTempObj1) );
					iLimb2++;
				}
				else
				{
					AddLimb ( iPolygonBoundsObject, iLimb, iTempMesh );
					float fRealStartY = GetLUATerrainHeightEx ( vIterPrev->fX, vIterPrev->fY );
					PositionObject ( iTempObj1, vIterPrev->fX, fRealStartY, vIterPrev->fY );
					float fRealFinishY = GetLUATerrainHeightEx ( vIter->fX, vIter->fY );
					fDirX = vIter->fX - vIterPrev->fX;
					float fDirY = fRealFinishY - fRealStartY;
					fDirZ = vIter->fY - vIterPrev->fY;
					fDist = sqrt ( fDirX*fDirX + fDirY*fDirY + fDirZ*fDirZ );
					ScaleLimb ( iPolygonBoundsObject, iLimb, 40*fCurrRadius, 100.0f, fDist * 100.0f );
					PointObject ( iTempObj1, vIter->fX, fRealFinishY, vIter->fY );
					float fRealMiddleY = fRealStartY+((fRealFinishY-fRealStartY)/2.0f);
					OffsetLimb ( iPolygonBoundsObject, iLimb, fX, fRealMiddleY + 7.5f, fZ );
					RotateLimb ( iPolygonBoundsObject, iLimb, ObjectAngleX(iTempObj1), ObjectAngleY(iTempObj1), ObjectAngleZ(iTempObj1) );
					iLimb++;
				}

				// next edge
				vIterPrev = vIter;
				vIter++;
			}
		}
        
        pIter++;
    }

	//now do the same for view blocking obstacles
	if ( iVBPolygonBoundsObject > 0 && ObjectExist ( iVBPolygonBoundsObject ) == 1 )
	{
		DeleteObject ( iVBPolygonBoundsObject );
	}

	if ( iVBPolygonBoundsObject == 0 ) iVBPolygonBoundsObject = dbFreeObject ( );
	if ( iVBPolygonBoundsObject == 0 ) return;

	//root node
	MakeObjectPlane ( iVBPolygonBoundsObject, 0.0f, 0.0f, 1 );
	SetObjectMask ( iVBPolygonBoundsObject, 1 );	

	pIter = sViewBlockingData_list.begin ( );
	
	iLimb = 1;
	while ( pIter < sViewBlockingData_list.end ( ) )
	{
		if ( pIter->sVertexData_list.size ( ) > 1 )
		{
			vIter = pIter->sVertexData_list.begin ( );
			vIterPrev = pIter->sVertexData_list.end ( ) - 1;

			while ( vIter < pIter->sVertexData_list.end ( ) )
			{
				float fX = ( vIter->fX + vIterPrev->fX ) / 2.0f;
				float fZ = ( vIter->fY + vIterPrev->fY ) / 2.0f;
				float fDirX = vIter->fX - vIterPrev->fX;
				float fDirZ = vIter->fY - vIterPrev->fY;
				float fDist = sqrt ( fDirX*fDirX + fDirZ*fDirZ );
				if ( fDist < 0.1f ) 
				{
					vIterPrev = vIter;
					vIter++;
					continue;
				}
				fDirX /= fDist;
				fDirZ /= fDist;
				float fAngY = acos ( fDirZ ) * 57.295779513f;
				if ( fDirX < 0.0f ) fAngY = 360.0f - fAngY;

				//AddLimb ( iVBPolygonBoundsObject, iLimb, iTempMesh );
				//ScaleLimb ( iVBPolygonBoundsObject, iLimb, 100.0f, 100.0f, fDist * 100.0f );
				//RotateLimb ( iVBPolygonBoundsObject, iLimb, 0.0f, fAngY, 0.0f );
				//OffsetLimb ( iVBPolygonBoundsObject, iLimb, fX, fHeight, fZ );//0.0f, fZ );
				AddLimb ( iVBPolygonBoundsObject, iLimb, iTempMesh );
				float fRealStartY = GetLUATerrainHeightEx ( vIterPrev->fX, vIterPrev->fY );
				PositionObject ( iTempObj1, vIterPrev->fX, fRealStartY, vIterPrev->fY );
				float fRealFinishY = GetLUATerrainHeightEx ( vIter->fX, vIter->fY );
				fDirX = vIter->fX - vIterPrev->fX;
				float fDirY = fRealFinishY - fRealStartY;
				fDirZ = vIter->fY - vIterPrev->fY;
				fDist = sqrt ( fDirX*fDirX + fDirY*fDirY + fDirZ*fDirZ );
				ScaleLimb ( iVBPolygonBoundsObject, iLimb, 100.0f, 100.0f, fDist * 100.0f );
				PointObject ( iTempObj1, vIter->fX, fRealFinishY, vIter->fY );
				float fRealMiddleY = fRealStartY+((fRealFinishY-fRealStartY)/2.0f);
				OffsetLimb ( iVBPolygonBoundsObject, iLimb, fX, fRealMiddleY + 7.5f, fZ );
				RotateLimb ( iVBPolygonBoundsObject, iLimb, ObjectAngleX(iTempObj1), ObjectAngleY(iTempObj1), ObjectAngleZ(iTempObj1) );
				iLimb++;

				vIterPrev = vIter;
				vIter++;
			}
		}

		pIter++;
	}
	
	//delete the plane mesh
	DeleteMesh ( iTempMesh );

	// delete temp object
	if ( ObjectExist(iTempObj1) == 1 ) DeleteObject ( iTempObj1 );

	//collect all limbs into a single limb for speed
	dbCombineLimbs ( iPolygonBoundsObject );
	dbCombineLimbs ( iHHPolygonBoundsObject );
	dbCombineLimbs ( iVBPolygonBoundsObject );
	
	PositionObject ( iPolygonBoundsObject, 0.0f, 0.0f, 0.0f );// fHeight, 0.0f );
	PositionObject ( iHHPolygonBoundsObject, 0.0f, 0.0f, 0.0f );// fHeight, 0.0f );
	PositionObject ( iVBPolygonBoundsObject, 0.0f, 0.0f, 0.0f );// fHeight, 0.0f );

	SetObjectCollisionOff ( iPolygonBoundsObject );
	SetObjectCollisionOff ( iHHPolygonBoundsObject );
	SetObjectCollisionOff ( iVBPolygonBoundsObject );

	//colour the objects.
	ColorObject ( iPolygonBoundsObject, 0xff00ff00 );
	ColorObject ( iHHPolygonBoundsObject, 0xff005A00 );
	ColorObject ( iVBPolygonBoundsObject, 0xff00ffff );
	SetObjectEmissive ( iPolygonBoundsObject, 0xff00ff00 );
	SetObjectEffect ( iPolygonBoundsObject, g_GUIShaderEffectID );
	SetObjectEmissive ( iHHPolygonBoundsObject, 0xff005A00 );
	SetObjectEffect ( iHHPolygonBoundsObject, g_GUIShaderEffectID );
	SetObjectEmissive ( iVBPolygonBoundsObject, 0xff00ffff );
	SetObjectEffect ( iVBPolygonBoundsObject, g_GUIShaderEffectID );

	SetObjectEmissive ( iPolygonBoundsObject, 0xff000000 );
	SetObjectEmissive ( iHHPolygonBoundsObject, 0xff000000 );
	SetObjectEmissive ( iVBPolygonBoundsObject, 0xff000000 );
}

void PathFinderAdvanced::DebugHidePolygonBounds ( )
{
	if ( iPolygonBoundsObject > 0 && ObjectExist ( iPolygonBoundsObject ) )
	{
		DeleteObject ( iPolygonBoundsObject );
	}

	iPolygonBoundsObject = 0;

	if ( iHHPolygonBoundsObject > 0 && ObjectExist ( iHHPolygonBoundsObject ) )
	{
		DeleteObject ( iHHPolygonBoundsObject );
	}

	iHHPolygonBoundsObject = 0;

	if ( iVBPolygonBoundsObject > 0 && ObjectExist ( iVBPolygonBoundsObject ) )
	{
		DeleteObject ( iVBPolygonBoundsObject );
	}

	iVBPolygonBoundsObject = 0;
}

void PathFinderAdvanced::DebugUpdatePolygonColour( float fTimeDelta )
{
	//if ( iPolygonBoundsObject == 0 ) return;
	//if ( iHHPolygonBoundsObject == 0 ) return;
	//if ( iVBPolygonBoundsObject == 0 ) return;

	fFlashingTimer += fTimeDelta*2.0f;
	if ( fFlashingTimer > 12 ) fFlashingTimer -= 12;

	//char str[256];

	DWORD iColourValue = (DWORD) ( ( sin( fFlashingTimer ) + 1.0f ) * 127.0f );
	DWORD dwColour = 0xff000000 
			  | ( ( 0x000000ff & iColourValue ) << 16 ) 
			  | ( ( 0x000000ff & iColourValue ) << 8 )
			  | ( ( 0x000000ff & iColourValue ) );

	//sprintf(str, "FlashTime: %3.3f, timeDelta: %3.3f, Colour: %d, dwColour: %x\n", fFlashingTimer, fTimeDelta, iColourValue, dwColour );
	//FILE *pInfo = GG_fopen("Flashing.txt","a");
	//fputs(str,pInfo);
	//fclose(pInfo);

	if ( iPolygonBoundsObject != 0 ) SetObjectEmissive( iPolygonBoundsObject, dwColour );
	if ( iHHPolygonBoundsObject != 0 ) SetObjectEmissive( iHHPolygonBoundsObject, dwColour );
	if ( iVBPolygonBoundsObject != 0 ) SetObjectEmissive( iVBPolygonBoundsObject, dwColour );
}

void PathFinderAdvanced::DebugDrawAvoidanceGrid( float fHeight )
{
	bShowAvoidanceGrid = true;
	fAvoidanceGridDebugTimer = 0;
	fAvoidanceGridHeight = fHeight;

	if ( iGridObject <= 0 || ObjectExist ( iGridObject ) == 0 )
	{
		//make a plane mesh to represent edges.
		int iTempMesh = dbMakeEdgeMesh ( );
		if ( iTempMesh == 0 ) return;
		
		//clear any current debug object
		if ( iGridObject > 0 && ObjectExist ( iGridObject ) == 1 )
		{
			DeleteObject ( iGridObject );
		}
		
		//find a free object
		if ( iGridObject == 0 ) iGridObject = dbFreeObject ( );
		if ( iGridObject == 0 ) return;

		//root node
		MakeObjectPlane ( iGridObject, 0.0f, 0.0f, 1 );
		SetObjectMask ( iGridObject, 1 );	

		int iLimb = 1;

		for( int i = 0; i < 100; i ++ )
		{
			AddLimb ( iGridObject, iLimb, iTempMesh );
			ScaleLimb ( iGridObject, iLimb, 100.0f, 100.0f, 100.0f * fGridRadius * 100.0f );
			//RotateLimb ( iGridObject, iLimb, 0.0f, 90.0f, 0.0f );
			OffsetLimb ( iGridObject, iLimb, i * fGridRadius, 0.0f, -50.0f * fGridRadius );

			iLimb++;

			AddLimb ( iGridObject, iLimb, iTempMesh );
			ScaleLimb ( iGridObject, iLimb, 100.0f, 100.0f, 100.0f * fGridRadius * 100.0f );
			RotateLimb ( iGridObject, iLimb, 0.0f, 90.0f, 0.0f );
			OffsetLimb ( iGridObject, iLimb, 50.0f * fGridRadius, 0.0f, -i * fGridRadius );

			iLimb++;
		}

		//delete the plane mesh
		DeleteMesh ( iTempMesh );

		//collect all limbs into a single limb for speed
		dbCombineLimbs ( iGridObject );
		PositionObject ( iGridObject, 0.0f, fAvoidanceGridHeight, 0.0f );
		SetObjectCollisionOff ( iGridObject );
		ColorObject ( iGridObject, 0x000000ff );
		SetObjectEmissive ( iGridObject, 0x000000ff );
		SetObjectEffect ( iGridObject, g_GUIShaderEffectID );
	}
}

void PathFinderAdvanced::DebugHideAvoidanceGrid( )
{
	bShowAvoidanceGrid = false;

	if ( iGridObject > 0 && ObjectExist ( iGridObject ) )
	{
		DeleteObject ( iGridObject );
	}

	iGridObject = 0;

	if ( iGridObject2 > 0 && ObjectExist ( iGridObject2 ) )
	{
		DeleteObject ( iGridObject2 );
	}

	iGridObject2 = 0;
}

void PathFinderAdvanced::DebugUpdateAvoidanceGrid( float fTimeDelta )
{
	if ( !bShowAvoidanceGrid ) return;

	fAvoidanceGridDebugTimer -= fTimeDelta;

	if ( fAvoidanceGridDebugTimer <= 0 )
	{
		fAvoidanceGridDebugTimer = 0.2f;

		int iTempObj = dbFreeObject ( );
		if ( iTempObj == 0 ) return;
		int iTempMesh = dbFreeMesh ( );
		if ( iTempMesh == 0 ) return;

		MakeObjectPlane ( iTempObj, fGridRadius - 0.3f, fGridRadius - 0.3f, 1 );
		XRotateObject ( iTempObj, 90.0f );
		FixObjectPivot ( iTempObj );
		MakeMeshFromObject ( iTempMesh, iTempObj );
		DeleteObject ( iTempObj );

		int iTempMesh2 = dbMakeEdgeMesh ( );
		if ( iTempMesh2 == 0 ) return;

		if ( iGridObject2 > 0 && ObjectExist ( iGridObject2 ) == 1 )
		{
			DeleteObject ( iGridObject2 );
		}
		
		//find a free object
		if ( iGridObject2 == 0 ) iGridObject2 = dbFreeObject ( );
		if ( iGridObject2 == 0 ) return;

		//root node
		MakeObjectPlane ( iGridObject2, 0.0f, 0.0f, 1 );
		SetObjectMask ( iGridObject2, 1 );	

		int iLimb = 1;
		float length = sqrt( 2*fGridRadius*fGridRadius );

		for( int i = 0; i < 100; i++ )
		{
			for( int j = 0; j < 100; j++ )
			{
				GridElement *pElement = cGrid.GetRootElement( i,j );

				while ( pElement )
				{
					if ( pElement->GetValue() > 0 )
					{
						AddLimb ( iGridObject2, iLimb, iTempMesh );
						OffsetLimb ( iGridObject2, iLimb, GridItoF(pElement->GetX()), 0.0f, GridItoF(pElement->GetZ()) );

						iLimb++;
					}

					if ( pElement->GetReserved() > 0 )
					{
						AddLimb ( iGridObject2, iLimb, iTempMesh );
						ScaleLimb ( iGridObject2, iLimb, 50.0f, 50.0f, 50.0f );
						RotateLimb ( iGridObject2, iLimb, 0.0f, 45.0f, 0.0f );
						OffsetLimb ( iGridObject2, iLimb, GridItoF(pElement->GetX()), 0.0f, GridItoF(pElement->GetZ()) );

						iLimb++;
					}

					pElement = pElement->pNextElement;
				}

				GridElement *pUElement = cUndesirableGrid.GetRootElement( i,j );

				while ( pUElement )
				{
					if ( pUElement->GetValue() > 0 )
					{
						AddLimb ( iGridObject2, iLimb, iTempMesh2 );
						ScaleLimb ( iGridObject2, iLimb, 100.0f, 100.0f, length * 100.0f );
						RotateLimb ( iGridObject2, iLimb, 0.0f, 45.0f, 0.0f );
						OffsetLimb ( iGridObject2, iLimb, GridItoF(pUElement->GetX()), 0.0f, GridItoF(pUElement->GetZ()) );

						iLimb++;

						AddLimb ( iGridObject2, iLimb, iTempMesh2 );
						ScaleLimb ( iGridObject2, iLimb, 100.0f, 100.0f, length * 100.0f );
						RotateLimb ( iGridObject2, iLimb, 0.0f, 135.0f, 0.0f );
						OffsetLimb ( iGridObject2, iLimb, GridItoF(pUElement->GetX()), 0.0f, GridItoF(pUElement->GetZ()) );

						iLimb++;
					}

					pUElement = pUElement->pNextElement;
				}
			}
		}

		DeleteMesh ( iTempMesh );
		DeleteMesh ( iTempMesh2 );

		//collect all limbs into a single limb for speed
		dbCombineLimbs ( iGridObject2 );
		PositionObject ( iGridObject2, 0.0f, fAvoidanceGridHeight, 0.0f );
		SetObjectCollisionOff ( iGridObject2 );
		ColorObject ( iGridObject2, 0x00ff0000 );
		SetObjectEmissive ( iGridObject2, 0x00ff0000 );
		SetObjectEffect ( iGridObject2, g_GUIShaderEffectID );
	}
}
