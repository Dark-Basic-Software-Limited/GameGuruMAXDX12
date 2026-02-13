void cUniverse::DrawNodePortals ( sNode* pNode )
{
	// draw portals in node

	// check node pointer is valid
	if ( !pNode )
		return;

	// to store fvf
	DWORD dwFVF = 0;

	// get fvf
	m_pD3D->GetFVF ( &dwFVF );

	// set texture to null
	m_pD3D->SetTexture ( 0, NULL );
	m_pD3D->SetTexture ( 1, NULL );
	m_pD3D->SetTexture ( 2, NULL );
	m_pD3D->SetTexture ( 3, NULL );

	// draw in solid mode
	m_pD3D->SetRenderState ( D3DRS_FILLMODE, D3DFILL_SOLID );

	// set fvf
	m_pD3D->SetFVF ( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1 );

	// set render states
	m_pD3D->SetRenderState ( D3DRS_ALPHABLENDENABLE, true );					// alpha blend on
	m_pD3D->SetRenderState ( D3DRS_SRCBLEND,         D3DBLEND_ONE );			// source blend
	m_pD3D->SetRenderState ( D3DRS_DESTBLEND,        D3DBLEND_INVSRCCOLOR );	// destination blend
	m_pD3D->SetRenderState ( D3DRS_CULLMODE,         D3DCULL_NONE );			// no culling
	m_pD3D->SetRenderState ( D3DRS_LIGHTING,         FALSE );					// turn lighting off
	m_pD3D->SetRenderState ( D3DRS_ZENABLE,          false );
	m_pD3D->SetRenderState ( D3DRS_ZWRITEENABLE,     false );

	// go through each side
	for ( int iPortal = 0; iPortal < 6; iPortal++ )
	{
		// skip invisible portals
		if ( !pNode->portals [ iPortal ].bVisible )
				continue;

		// skip portals that are not in viewing frustum
		if ( !pNode->portals [ iPortal ].bVisibleInViewingFrustum )
			continue;

		// draw the portal
		m_pD3D->DrawPrimitiveUP (
									D3DPT_TRIANGLESTRIP,
									2,
									&pNode->portals [ iPortal ].vertices [ 0 ],
									sizeof ( sPortalVertex )
								);
		
	}
		
	// reset certain render states
	m_pD3D->SetRenderState ( D3DRS_ALPHABLENDENABLE, false );
	m_pD3D->SetRenderState ( D3DRS_CULLMODE,         D3DCULL_CCW );
	m_pD3D->SetRenderState ( D3DRS_ZWRITEENABLE,     true );

	// and finally go back to original fvf
	m_pD3D->SetFVF ( dwFVF );
}

void cUniverse::DrawNodeBounds ( sNode* pNode )
{
	// draw bounds of a node

	// check node pointer
	if ( !pNode )
		return;

	// used to store fvf
	DWORD dwFVF = 0;

	// get fvf and set a new one
	m_pD3D->GetFVF ( &dwFVF );
	m_pD3D->SetFVF ( D3DFVF_XYZ );

	// set no texture and render states
	m_pD3D->SetTexture     ( 0, NULL );
	m_pD3D->SetRenderState ( D3DRS_ALPHABLENDENABLE, FALSE );
	m_pD3D->SetRenderState ( D3DRS_FILLMODE,         D3DFILL_SOLID );
	m_pD3D->SetRenderState ( D3DRS_ZENABLE,          false );
	m_pD3D->SetRenderState ( D3DRS_ZWRITEENABLE,     false );

	// to store lines
	D3DXVECTOR3 vecLines [ 24 ];

	// get node bounds
	vecLines [  0 ] = GetTopLeftFront     ( pNode );	vecLines [  1 ] = GetTopRightFront    ( pNode );
	vecLines [  2 ] = GetTopLeftBack      ( pNode );  	vecLines [  3 ] = GetTopRightBack     ( pNode );
	vecLines [  4 ] = GetTopLeftFront     ( pNode );	vecLines [  5 ] = GetTopLeftBack      ( pNode );
	vecLines [  6 ] = GetTopRightFront    ( pNode );	vecLines [  7 ] = GetTopRightBack     ( pNode );
	vecLines [  8 ] = GetBottomLeftFront  ( pNode );	vecLines [  9 ] = GetBottomRightFront ( pNode );
	vecLines [ 10 ] = GetBottomLeftBack   ( pNode );	vecLines [ 11 ] = GetBottomRightBack  ( pNode );
	vecLines [ 12 ] = GetBottomLeftFront  ( pNode );	vecLines [ 13 ] = GetBottomLeftBack   ( pNode );
	vecLines [ 14 ] = GetBottomRightFront ( pNode );	vecLines [ 15 ] = GetBottomRightBack  ( pNode );
	vecLines [ 16 ] = GetTopLeftFront     ( pNode );	vecLines [ 17 ] = GetBottomLeftFront  ( pNode );
	vecLines [ 18 ] = GetTopLeftBack      ( pNode );	vecLines [ 19 ] = GetBottomLeftBack   ( pNode );
	vecLines [ 20 ] = GetTopRightBack     ( pNode );	vecLines [ 21 ] = GetBottomRightBack  ( pNode );
	vecLines [ 22 ] = GetTopRightFront    ( pNode );	vecLines [ 23 ] = GetBottomRightFront ( pNode );

	// draw the node bounds
	m_pD3D->DrawPrimitiveUP (
								D3DPT_LINELIST,
								12,
								&vecLines,
								sizeof ( float ) * 3
							);


	// go back to original fvf
	m_pD3D->SetFVF ( dwFVF );
	m_pD3D->SetRenderState ( D3DRS_ZENABLE, true );
	m_pD3D->SetRenderState ( D3DRS_ZWRITEENABLE,     true );
}

void cUniverse::GetAreaBounds ( sArea* pArea )
{
	// get the bounding box of a given area

	// check the area pointer is valid
	if ( !pArea )
		return;

	// go through all nodes within the area
	for ( int iNode = 0; iNode < (int)pArea->nodes.size ( ); iNode++ )
	{
		// get a pointer to the node
		sNode* pNode = pArea->nodes [ iNode ];

		// ensure node is valid
		if ( !pNode )
			continue;

		// store an array of positions for corners or nodes
		D3DXVECTOR3 vec [ ] =	{ 
									GetTopLeftFront     ( pNode ),
									GetTopLeftBack      ( pNode ),
									GetTopRightFront    ( pNode ),
									GetTopRightBack     ( pNode ),
									GetBottomLeftFront  ( pNode ),
									GetBottomLeftBack   ( pNode ),
									GetBottomRightBack  ( pNode ),
									GetBottomRightFront ( pNode )
								};

		// now loop through all positions
		for ( int i = 0; i < sizeof ( vec ) / sizeof ( D3DXVECTOR3 ); i++ )
		{
			// check minimum extents
			if ( vec [ i ].x < pArea->vecMin.x ) pArea->vecMin.x = vec [ i ].x;
			if ( vec [ i ].y < pArea->vecMin.y ) pArea->vecMin.y = vec [ i ].y;
			if ( vec [ i ].z < pArea->vecMin.z ) pArea->vecMin.z = vec [ i ].z;

			// check maximum extents
			if ( vec [ i ].x > pArea->vecMax.x ) pArea->vecMax.x = vec [ i ].x;
			if ( vec [ i ].y > pArea->vecMax.y ) pArea->vecMax.y = vec [ i ].y;
			if ( vec [ i ].z > pArea->vecMax.z ) pArea->vecMax.z = vec [ i ].z;

			// increment centre by current position
			pArea->vecCentre += vec [ i ];

			// and increment count value
			pArea->iCount++;
		}
	}
}

bool cUniverse::WillAreaBoxEnterAnotherAreaBox ( int iSide, int iAreaX1, int iAreaY1, int iAreaZ1, int iAreaX2, int iAreaY2, int iAreaZ2 )
{
	// pre-empt the advance
	if ( iSide==0 )	iAreaZ1--; 
	if ( iSide==1 )	iAreaZ2++;
	if ( iSide==2 )	iAreaY2++;
	if ( iSide==3 )	iAreaY1--;
	if ( iSide==4 )	iAreaX2++;
	if ( iSide==5 )	iAreaX1--;
	
	int iAreaBox = 0;

	// go through each area box created thus far to see if side penetrates 'any'
	for ( iAreaBox = 0; iAreaBox < (int)m_pAreaList.size ( ); iAreaBox++ )
	{
		// get current area box ptr
		sArea* pCurrentArea = m_pAreaList [ iAreaBox ];
		int iAX1 = m_iHalfUniverseSizeX+((int)(pCurrentArea->vecMin.x+50.0f)/100);
		int iAY1 = m_iHalfUniverseSizeY+((int)(pCurrentArea->vecMin.y+50.0f)/100);
		int iAZ1 = m_iHalfUniverseSizeZ+((int)(pCurrentArea->vecMin.z+50.0f)/100);
		int iAX2 = m_iHalfUniverseSizeX+((int)(pCurrentArea->vecMax.x-50.0f)/100);
		int iAY2 = m_iHalfUniverseSizeY+((int)(pCurrentArea->vecMax.y-50.0f)/100);
		int iAZ2 = m_iHalfUniverseSizeZ+((int)(pCurrentArea->vecMax.z-50.0f)/100);

		// does the side penetrate (or overlap) this area box at all?
		if ( iAreaX1<=iAX2 && iAreaX2>=iAX1 )
			if ( iAreaY1<=iAY2 && iAreaY2>=iAY1 )
				if ( iAreaZ1<=iAZ2 && iAreaZ2>=iAZ1 )
					break;
	}

	// cut short, area box penetrated, so it will
	if ( iAreaBox < (int)m_pAreaList.size ( ) )
		return true;
	else
		return false;
}

bool cUniverse::BuildAreaBoxes ( void )
{
    int iNode = 0;
    
	// go through whole node universe and expand area boxes into them
	for ( iNode = 0; iNode < m_iNodeListSize; iNode++ )
	{
		// get node ptr
		sNode* pNode = &m_pNode [ iNode ];

		// for each un-used node
		if ( pNode->bChecked==false )
		{
			// start with area of single node
			int iAreaX1 = m_iHalfUniverseSizeX+((int)pNode->vecCentre.x/100);
			int iAreaY1 = m_iHalfUniverseSizeY+((int)pNode->vecCentre.y/100);
			int iAreaZ1 = m_iHalfUniverseSizeZ+((int)pNode->vecCentre.z/100);
			int iAreaX2 = iAreaX1;
			int iAreaY2 = iAreaY1;
			int iAreaZ2 = iAreaZ1;

			// expand on six sides
			for ( int iSide=0; iSide<6; iSide++ )
			{
				// prepare values for each side
				int iX1=iAreaX1, iX2=iAreaX2;
				int iY1=iAreaY1, iY2=iAreaY2;
				int iZ1=iAreaZ1, iZ2=iAreaZ2;
				if ( iSide==0 ) { iZ2=iZ1; }
				if ( iSide==1 ) { iZ1=iZ2; }
				if ( iSide==2 ) { iY1=iY2; }
				if ( iSide==3 ) { iY2=iY1; }
				if ( iSide==4 ) { iX1=iX2; }
				if ( iSide==5 ) { iX2=iX1; }

				// calculate opposite side for secondary side check
				int iOSide, iOX, iOY, iOZ;
				if ( iSide==0 ) { iOSide=1; iOX=0; iOY=0; iOZ=-1; }
				if ( iSide==1 ) { iOSide=0; iOX=0; iOY=0; iOZ= 1; }
				if ( iSide==2 ) { iOSide=3; iOX=0; iOY= 1; iOZ=0; }
				if ( iSide==3 ) { iOSide=2; iOX=0; iOY=-1; iOZ=0; }
				if ( iSide==4 ) { iOSide=5; iOX= 1; iOY=0; iOZ=0; }
				if ( iSide==5 ) { iOSide=4; iOX=-1; iOY=0; iOZ=0; }

				// scan for solid or border obstruction
				int iSolidity;
				bool bSolidFound = false;
				if ( iSide==0 && iAreaZ1==0 ) bSolidFound=true;
				if ( iSide==1 && iAreaZ2==m_iUniverseSizeZ ) bSolidFound=true;
				if ( iSide==2 && iAreaY2==m_iUniverseSizeY ) bSolidFound=true;
				if ( iSide==3 && iAreaY1==0 ) bSolidFound=true;
				if ( iSide==4 && iAreaX2==m_iUniverseSizeX ) bSolidFound=true;
				if ( iSide==5 && iAreaX1==0 ) bSolidFound=true;
				if ( bSolidFound==false )
				{
					for ( int iX=iX1; iX<=iX2; iX++ )
					{
						for ( int iY=iY1; iY<=iY2; iY++ )
						{
							for ( int iZ=iZ1; iZ<=iZ2; iZ++ )
							{
								// check the direct side we want to expand into
								if ( GetSideFree ( iSide, iX, iY, iZ, &iSolidity )==false
								||	 GetSideFree ( iOSide, iX+iOX, iY+iOY, iZ+iOZ, &iSolidity )==false )
								{
									// solid side means no more in this direction
									bSolidFound=true;
								}

								// if about to proceed, make sure no secondary solids cut into areabox
								if ( bSolidFound==false )
								{
									// for Y expansion, we need to make sure there are no Z walls if we expanded up (+1)
									if ( iSide==2 )
									{
										// only inside Z walls, not the edges where the natural areabox walls are
										if ( iZ>iZ1 && iZ<iZ2 )
										{
											// check below to see if Z wall exists
											if ( GetSideFree ( 0, iX, iY + 1, iZ, &iSolidity )==false
											||	 GetSideFree ( 1, iX, iY + 1, iZ-1, &iSolidity )==false )
												bSolidFound=true;
										}
									}

									// for Y expansion, we need to make sure there are no Z walls if we expanded down (-1)
									if ( iSide==3 )
									{
										// only inside Z walls, not the edges where the natural areabox walls are
										if ( iZ>iZ1 && iZ<iZ2 )
										{
											// check below to see if Z wall exists
											if ( GetSideFree ( 0, iX, iY - 1, iZ, &iSolidity )==false
											||	 GetSideFree ( 1, iX, iY - 1, iZ-1, &iSolidity )==false )
												bSolidFound=true;
										}
									}

									// for X expansion, we need to make sure there are no Z and Y walls if we expanded right (+1)
									if ( iSide==4 )
									{
										// only inside Z and Y walls, not the edges where the natural areabox walls are
										if ( iZ>iZ1 && iZ<iZ2 )
										{
											// check right to see if Z wall exists
											if ( GetSideFree ( 0, iX + 1, iY, iZ, &iSolidity )==false
											||	 GetSideFree ( 1, iX + 1, iY, iZ-1, &iSolidity )==false )
												bSolidFound=true;
										}
										if ( iY>iY1 && iY<iY2 ) 
										{
											// check right to see if Y wall exists
											if ( GetSideFree ( 2, iX + 1, iY, iZ, &iSolidity )==false
											||	 GetSideFree ( 3, iX + 1, iY+1, iZ, &iSolidity )==false )
												bSolidFound=true;
										}
									}

									// for X expansion, we need to make sure there are no Z and Y walls if we expanded left (-1)
									if ( iSide==5 )
									{
										// only inside Z and Y walls, not the edges where the natural areabox walls are
										if ( iZ>iZ1 && iZ<iZ2 )
										{
											// check left to see if Z wall exists
											if ( GetSideFree ( 0, iX - 1, iY, iZ, &iSolidity )==false
											||	 GetSideFree ( 1, iX - 1, iY, iZ-1, &iSolidity )==false )
												bSolidFound=true;
										}
										if ( iY>iY1 && iY<iY2 ) 
										{
											// check left to see if Y wall exists
											if ( GetSideFree ( 2, iX - 1, iY, iZ, &iSolidity )==false
											||	 GetSideFree ( 3, iX - 1, iY+1, iZ, &iSolidity )==false )
												bSolidFound=true;
										}
									}
								}

								// in case of any solid found, exit iterations now
								if ( bSolidFound )
								{
									iX=iX2; iY=iY2; iZ=iZ2;
									break;
								}
							}
						}
					}
				}

				// when solid not found, expand in alternating directions
				if ( bSolidFound==false )
				{
					if ( !WillAreaBoxEnterAnotherAreaBox ( iSide, iAreaX1, iAreaY1, iAreaZ1, iAreaX2, iAreaY2, iAreaZ2 ) )
					{
						if ( iSide==0 )	iAreaZ1--; 
						if ( iSide==1 )	iAreaZ2++;
						if ( iSide==2 )	iAreaY2++;
						if ( iSide==3 )	iAreaY1--;
						if ( iSide==4 )	iAreaX2++;
						if ( iSide==5 )	iAreaX1--;
						iSide--;
					}
				}
			}

			// create a new area
			sArea* pArea = new sArea;
			if ( !pArea ) return false;

			// set up initial area properties
			pArea->nodes.clear ( );													// make sure node list is clear
			pArea->vecCentre = D3DXVECTOR3 (       0.0f,       0.0f,       0.0f );	// set centre point
			pArea->vecMin    = D3DXVECTOR3 (  100000.0f,  100000.0f,  100000.0f );	// set minimum bounds
			pArea->vecMax    = D3DXVECTOR3 ( -100000.0f, -100000.0f, -100000.0f );	// set maximum bounds
			pArea->iCount    = 0;													// reset count to 0

			// place all nodes into newly created area box
			for ( int iFindNode = 0; iFindNode < m_iNodeListSize; iFindNode++ )
			{
				// get node ptr
				sNode* pFindNode = &m_pNode [ iFindNode ];
				if ( pFindNode->bChecked==false )
				{
					// calculate location of node
					int iFindX = m_iHalfUniverseSizeX+((int)pFindNode->vecCentre.x/100);
					int iFindY = m_iHalfUniverseSizeY+((int)pFindNode->vecCentre.y/100);
					int iFindZ = m_iHalfUniverseSizeZ+((int)pFindNode->vecCentre.z/100);

					// add node if within area
					if ( iFindX>=iAreaX1 && iFindX<=iAreaX2
					&&   iFindY>=iAreaY1 && iFindY<=iAreaY2
					&&   iFindZ>=iAreaZ1 && iFindZ<=iAreaZ2 )
					{
						// add node and set to used
						pArea->nodes.push_back ( pFindNode );
						pFindNode->bChecked = true;
					}
				}
			}

			// get the area bounds
			GetAreaBounds ( pArea );

			// set up final bound box properties
			pArea->vecCentre /= ( float ) pArea->iCount;

			// store region for debug purposes
			pArea->pDebugRegion = new sNode;
			pArea->pDebugRegion->vecCentre = pArea->vecCentre;
			pArea->pDebugRegion->vecDimension = ( pArea->vecMax - pArea->vecMin ) / 2.0f;	// store dimension in region

			// send this area to back of list
			m_pAreaList.push_back ( pArea );
		}
	}

	// create debug data for each area box
	for ( iNode = 0; iNode < (int)m_pAreaList.size ( ); iNode++ )
		CreatePortalVertices ( m_pAreaList [ iNode ]->pDebugRegion );

	// store count of all area boxes
	g_iAreaBoxCount = m_pAreaList.size ( );
	
	// complete
	return true;
}

bool cUniverse::GetSideFree ( int iPortalSide, int iRefX, int iRefY, int iRefZ, int* pSolidity )
{
	if ( iRefX>=0 && iRefY>=0 && iRefZ>=0 )
	{
		// node determines side soliditiy
		sNode* pNode = m_pNodeRef [ iRefX + (iRefY*m_iUniverseSizeX) + (iRefZ*(m_iUniverseSizeX*m_iUniverseSizeY)) ];
		if ( pNode )
		{
			// assign solidity state
			if ( pSolidity )
				if ( pNode->portals [ iPortalSide ].iSolidityMode > 0 )
					*pSolidity = pNode->portals [ iPortalSide ].iSolidityMode;

			// return visibility
			return pNode->portals [ iPortalSide ].bVisible;
		}
		else
			return false;
	}
	else
	{
		// exceeds range of universe
		return false;
	}
}

void cUniverse::CreatePortalAndReset ( sArea* pCurrentArea, int iChkAreaBox, int iTouchingSide, D3DXVECTOR3 vecPortalMin, D3DXVECTOR3 vecPortalMax, int iSolidity )
{
	// work var
	sAreaLink AreaLink;
	memset ( &AreaLink, 0, sizeof(AreaLink) );

	// set link info and add to list
	AreaLink.iLinkedTo = iChkAreaBox;
	AreaLink.iTouchingSide = iTouchingSide;
	AreaLink.iSolidity = iSolidity;
	AreaLink.vecPortalMin = vecPortalMin;
	AreaLink.vecPortalMax = vecPortalMax;

	// calculate center and dimension for culling
	AreaLink.vecPortalDim = vecPortalMax-vecPortalMin;
	AreaLink.vecPortalCenter = vecPortalMin+((AreaLink.vecPortalDim)/2.0f);
	AreaLink.vecPortalDim /= 2.0f; //rectangle refined as x-halfsize, x+halfsize
	
	// generate vectors from portal region
	switch ( iTouchingSide )
	{
		case 0 :	// BACK (NORTH)
					AreaLink.vecA = D3DXVECTOR3 ( vecPortalMax.x, vecPortalMax.y, vecPortalMax.z );
					AreaLink.vecB = D3DXVECTOR3 ( vecPortalMin.x, vecPortalMax.y, vecPortalMax.z );
					AreaLink.vecC = D3DXVECTOR3 ( vecPortalMin.x, vecPortalMin.y, vecPortalMax.z );
					AreaLink.vecD = D3DXVECTOR3 ( vecPortalMax.x, vecPortalMin.y, vecPortalMax.z );	break;

		case 1 :	// FRONT (SOUTH)
					AreaLink.vecA = D3DXVECTOR3 ( vecPortalMin.x, vecPortalMax.y, vecPortalMin.z );
					AreaLink.vecB = D3DXVECTOR3 ( vecPortalMax.x, vecPortalMax.y, vecPortalMin.z );
					AreaLink.vecC = D3DXVECTOR3 ( vecPortalMax.x, vecPortalMin.y, vecPortalMin.z );
					AreaLink.vecD = D3DXVECTOR3 ( vecPortalMin.x, vecPortalMin.y, vecPortalMin.z );	break;

		case 2 :	// TOP (UP)
					AreaLink.vecA = D3DXVECTOR3 ( vecPortalMin.x, vecPortalMax.y, vecPortalMax.z );
					AreaLink.vecB = D3DXVECTOR3 ( vecPortalMax.x, vecPortalMax.y, vecPortalMax.z );
					AreaLink.vecC = D3DXVECTOR3 ( vecPortalMax.x, vecPortalMax.y, vecPortalMin.z );
					AreaLink.vecD = D3DXVECTOR3 ( vecPortalMin.x, vecPortalMax.y, vecPortalMin.z );	break;

		case 3 :	// BOTTOM (DOWN)
					AreaLink.vecA = D3DXVECTOR3 ( vecPortalMax.x, vecPortalMin.y, vecPortalMax.z );
					AreaLink.vecB = D3DXVECTOR3 ( vecPortalMin.x, vecPortalMin.y, vecPortalMax.z );
					AreaLink.vecC = D3DXVECTOR3 ( vecPortalMin.x, vecPortalMin.y, vecPortalMin.z );
					AreaLink.vecD = D3DXVECTOR3 ( vecPortalMax.x, vecPortalMin.y, vecPortalMin.z );	break;

		case 4 :	// LEFT (WEST)
					AreaLink.vecA = D3DXVECTOR3 ( vecPortalMin.x, vecPortalMax.y, vecPortalMax.z );
					AreaLink.vecB = D3DXVECTOR3 ( vecPortalMin.x, vecPortalMax.y, vecPortalMin.z );
					AreaLink.vecC = D3DXVECTOR3 ( vecPortalMin.x, vecPortalMin.y, vecPortalMin.z );
					AreaLink.vecD = D3DXVECTOR3 ( vecPortalMin.x, vecPortalMin.y, vecPortalMax.z );	break;

		case 5 :	// RIGHT (EAST)
					AreaLink.vecA = D3DXVECTOR3 ( vecPortalMax.x, vecPortalMax.y, vecPortalMin.z );
					AreaLink.vecB = D3DXVECTOR3 ( vecPortalMax.x, vecPortalMax.y, vecPortalMax.z );
					AreaLink.vecC = D3DXVECTOR3 ( vecPortalMax.x, vecPortalMin.y, vecPortalMax.z );
					AreaLink.vecD = D3DXVECTOR3 ( vecPortalMax.x, vecPortalMin.y, vecPortalMin.z );	break;
	}

	// create dynamic memory to store in list
	sAreaLink* pAreaLink = new sAreaLink;
	*pAreaLink = AreaLink;

	// add to list of links for this area box
	pCurrentArea->pLink.push_back ( pAreaLink );
	pCurrentArea->iLinkMax++;
}

void cUniverse::ExtendPortalMinMax ( int iPortalSide, int iX, int iY, int iZ, D3DXVECTOR3* pvecPortalMin, D3DXVECTOR3* pvecPortalMax )
{
	// side determines extension along approproate axis
	D3DXVECTOR3 vecExt = D3DXVECTOR3(0,0,0);

	// based on side creating portal on
	if ( iPortalSide==eBack )	vecExt = D3DXVECTOR3(100.0f,100.0f,0);
	if ( iPortalSide==eFront )	vecExt = D3DXVECTOR3(100.0f,100.0f,0);
	if ( iPortalSide==eLeft )	vecExt = D3DXVECTOR3(0,100.0f,100.0f);
	if ( iPortalSide==eRight )	vecExt = D3DXVECTOR3(0,100.0f,100.0f);
	if ( iPortalSide==eTop )	vecExt = D3DXVECTOR3(100.0f,0,100.0f);
	if ( iPortalSide==eBottom )	vecExt = D3DXVECTOR3(100.0f,0,100.0f);

	// increase size of portal rea using x,y,z
	if ( iX<pvecPortalMin->x ) pvecPortalMin->x=iX;
	if ( iY<pvecPortalMin->y ) pvecPortalMin->y=iY;
	if ( iZ<pvecPortalMin->z ) pvecPortalMin->z=iZ;
	if ( iX+vecExt.x>pvecPortalMax->x ) pvecPortalMax->x=iX+vecExt.x;
	if ( iY+vecExt.y>pvecPortalMax->y ) pvecPortalMax->y=iY+vecExt.y;
	if ( iZ+vecExt.z>pvecPortalMax->z ) pvecPortalMax->z=iZ+vecExt.z;
}

bool cUniverse::BuildAreaLinks ( void )
{
    int iAreaBox = 0;
    
	// stage 1 - work out which area boxes touch and calculate the portals that bind them
	for ( iAreaBox = 0; iAreaBox < (int)m_pAreaList.size ( ); iAreaBox++ )
	{
		// get current area box ptr
		sArea* pCurrentArea = m_pAreaList [ iAreaBox ];
		int iAX1 = pCurrentArea->vecMin.x;
		int iAY1 = pCurrentArea->vecMin.y;
		int iAZ1 = pCurrentArea->vecMin.z;
		int iAX2 = pCurrentArea->vecMax.x;
		int iAY2 = pCurrentArea->vecMax.y;
		int iAZ2 = pCurrentArea->vecMax.z;

		// clear link info
		pCurrentArea->pLink.clear();

		// for each area box, check if touching other
		for ( int iChkAreaBox = 0; iChkAreaBox < (int)m_pAreaList.size ( ); iChkAreaBox++ )
		{
			// and not touching itself
			if ( iAreaBox != iChkAreaBox )
			{
				// get area box ptr to check against
				sArea* pCheckArea = m_pAreaList[iChkAreaBox];
				int iBX1 = pCheckArea->vecMin.x;
				int iBY1 = pCheckArea->vecMin.y;
				int iBZ1 = pCheckArea->vecMin.z;
				int iBX2 = pCheckArea->vecMax.x;
				int iBY2 = pCheckArea->vecMax.y;
				int iBZ2 = pCheckArea->vecMax.z;

				// touch variable
				bool bScanValid = false;
				int iTouchingSide = eUnknown;
				int iOppositeSide = eUnknown;
				int iXStart=0, iXEnd=0;
				int iYStart=0, iYEnd=0;
				int iZStart=0, iZEnd=0;
				int iS1OffX, iS1OffY, iS1OffZ;
				int iS2OffX, iS2OffY, iS2OffZ;

				// NORTH and SOUTH checks scan the XY plane
				bool bNorthSouthScan=false;
				if ( iAZ1==iBZ2 || iAZ2==iBZ1 )
				{
					if ( iAX2>=iBX1 && iAX1<=iBX2 && iAY2>=iBY1 && iAY1<=iBY2 )
					{
						// determine contact region for XY
						iXStart=iAX1; if(iXStart<iBX1) iXStart=iBX1;
						iXEnd=iAX2; if(iXEnd>iBX2) iXEnd=iBX2;
						iYStart=iAY1; if(iYStart<iBY1) iYStart=iBY1;
						iYEnd=iAY2; if(iYEnd>iBY2) iYEnd=iBY2;
						iS1OffX= 0; iS1OffY= 0; iS1OffZ=-1;
						iS2OffX= 0; iS2OffY= 0; iS2OffZ= 0;
						bNorthSouthScan=true;
					}
				}
				if ( iAZ1==iBZ2 )
				{
					if ( bNorthSouthScan==true )
					{
						// areaboxes do touch
						iTouchingSide = eFront;
						iOppositeSide = eBack;
						iZStart = iAZ1;
						iZEnd = iZStart+100;
						bScanValid = true;
						iS1OffZ=-1;
						iS2OffZ=0;
					}
				}
				if ( iAZ2==iBZ1 )
				{
					if ( bNorthSouthScan==true )
					{
						// areaboxes do touch
						iTouchingSide = eBack;
						iOppositeSide = eFront;
						iZStart = iAZ2;
						iZEnd = iZStart+100;
						bScanValid = true;
						iS2OffZ=-1;
						iS1OffZ=0;
					}
				}

				// EAST and WEST checks scan the YZ plane
				bool bEastWestScan=false;
				if ( iAX1==iBX2 || iAX2==iBX1 )
				{
					if ( iAY2>=iBY1 && iAY1<=iBY2 && iAZ2>=iBZ1 && iAZ1<=iBZ2 )
					{
						// determine contact region for YZ
						iYStart=iAY1; if(iYStart<iBY1) iYStart=iBY1;
						iYEnd=iAY2; if(iYEnd>iBY2) iYEnd=iBY2;
						iZStart=iAZ1; if(iZStart<iBZ1) iZStart=iBZ1;
						iZEnd=iAZ2; if(iZEnd>iBZ2) iZEnd=iBZ2;
						iS1OffX=-1; iS1OffY= 0; iS1OffZ= 0;
						iS2OffX= 0; iS2OffY= 0; iS2OffZ= 0;
						bEastWestScan=true;
					}
				}
				if ( iAX1==iBX2 )
				{
					if ( bEastWestScan==true )
					{
						// areaboxes do touch
						iTouchingSide = eRight;
						iOppositeSide = eLeft;
						iXStart = iAX1;
						iXEnd = iXStart+100;
						bScanValid = true;
						iS1OffX=-1;
						iS2OffX=0;
					}
				}
				if ( iAX2==iBX1 )
				{
					if ( bEastWestScan==true )
					{
						// areaboxes do touch
						iTouchingSide = eLeft;
						iOppositeSide = eRight;
						iXStart = iAX2;
						iXEnd = iXStart+100;
						bScanValid = true;
						iS2OffX=-1;
						iS1OffX=0;
					}
				}

				// UP and DOWN checks scan the XZ plane
				bool bUpDownScan=false;
				if ( iAY1==iBY2 || iAY2==iBY1 )
				{
					if ( iAX2>=iBX1 && iAX1<=iBX2 && iAZ2>=iBZ1 && iAZ1<=iBZ2 )
					{
						// determine contact region for XZ
						iXStart=iAX1; if(iXStart<iBX1) iXStart=iBX1;
						iXEnd=iAX2; if(iXEnd>iBX2) iXEnd=iBX2;
						iZStart=iAZ1; if(iZStart<iBZ1) iZStart=iBZ1;
						iZEnd=iAZ2; if(iZEnd>iBZ2) iZEnd=iBZ2;
						iS1OffX= 0; iS1OffY=-1; iS1OffZ= 0;
						iS2OffX= 0; iS2OffY= 0; iS2OffZ= 0;
						bUpDownScan=true;
					}
				}
				if ( iAY1==iBY2 )
				{
					if ( bUpDownScan==true )
					{
						// areaboxes do touch
						iTouchingSide = eBottom;
						iOppositeSide = eTop;
						iYStart = iAY1;
						iYEnd = iYStart+100;
						bScanValid = true;
						iS2OffY=-1;
						iS1OffY=0;
					}
				}
				if ( iAY2==iBY1 )
				{
					if ( bUpDownScan==true )
					{
						// areaboxes do touch
						iTouchingSide = eTop;
						iOppositeSide = eBottom;
						iYStart = iAY2;
						iYEnd = iYStart+100;
						bScanValid = true;
						iS1OffY=-1;
						iS2OffY=0;
					}
				}

				// areabox side touch detected
				if ( bScanValid==true )
				{
					// stores nature of the portals solidity state (0-none/1-partial/2-fully solid)
					int iPartialSolidityDetected=0;

					// scan all nodes along the contact edge for holes
					D3DXVECTOR3 vecPortalMin;
					D3DXVECTOR3 vecPortalMax;
					bool bPortalBuildState = false;
					for ( int iZ=iZStart; iZ<iZEnd; iZ+=100 )
					{
						for ( int iX=iXStart; iX<iXEnd; iX+=100 )
						{
							for ( int iY=iYStart; iY<iYEnd; iY+=100 )
							{
								// get node at coordinate
								int iSolidity=0;
								int iRefX=m_iHalfUniverseSizeX+((iX+50)/100);
								int iRefY=m_iHalfUniverseSizeY+((iY+50)/100);
								int iRefZ=m_iHalfUniverseSizeZ+((iZ+50)/100);
								bool bSide1Free = GetSideFree ( iTouchingSide, iRefX+iS1OffX, iRefY+iS1OffY, iRefZ+iS1OffZ, &iSolidity );
								bool bSide2Free = GetSideFree ( iOppositeSide, iRefX+iS2OffX, iRefY+iS2OffY, iRefZ+iS2OffZ, &iSolidity );
								if ( bSide1Free && bSide2Free )
								{
									// first of new portal uses current ref
									if ( bPortalBuildState==false )
									{
										bPortalBuildState=true;
										iPartialSolidityDetected=0;
										vecPortalMin = D3DXVECTOR3(99999,99999,99999);
										vecPortalMax = D3DXVECTOR3(-99999,-99999,-99999);
									}

									// record partial solidity
									iPartialSolidityDetected = iSolidity;

									// individual node side touches area box
									ExtendPortalMinMax ( iTouchingSide, iX, iY, iZ, &vecPortalMin, &vecPortalMax );
								}
								else
								{
									// a solid detected, recreate portal so far
									if ( bPortalBuildState )
									{
										CreatePortalAndReset ( pCurrentArea, iChkAreaBox, iTouchingSide, vecPortalMin, vecPortalMax, iPartialSolidityDetected );
										iPartialSolidityDetected = 0;
										bPortalBuildState = false;
									}
								}
							}
						}
					}

					// create any portals not created in thiple fornext loop
					if ( bPortalBuildState ) CreatePortalAndReset ( pCurrentArea, iChkAreaBox, iTouchingSide, vecPortalMin, vecPortalMax, iPartialSolidityDetected );
				}
			}
		}
	}

	// stage 2 - create debug information (visuals) for merged area box link portals
	for ( iAreaBox = 0; iAreaBox < (int)m_pAreaList.size ( ); iAreaBox++ )
	{
		// go through links of each area bix
		for ( int iLink=0; iLink<m_pAreaList [ iAreaBox ]->iLinkMax; iLink++ )
		{
			// get link ptr
			sAreaLink* pAreaLink = m_pAreaList [ iAreaBox ]->pLink [ iLink ];
			if ( pAreaLink )
			{
				// store region for debug purposes
				pAreaLink->pDebugRegion = new sNode;
				pAreaLink->pDebugRegion->vecCentre    = pAreaLink->vecPortalMin + ( ( pAreaLink->vecPortalMax - pAreaLink->vecPortalMin ) / 2.0f );	
				pAreaLink->pDebugRegion->vecDimension = ( pAreaLink->vecPortalMax - pAreaLink->vecPortalMin ) / 2.0f;

				// X9 - 060208 - right colours more useful as we can see the portals
				DWORD dwColor = D3DCOLOR_ARGB ( 255, 255, 0, 0 );
				if ( pAreaLink->iSolidity==1 ) dwColor = D3DCOLOR_ARGB ( 255, 0, 255, 0 );
				if ( pAreaLink->iSolidity==2 ) dwColor = D3DCOLOR_ARGB ( 255, 0, 0, 255 );
				CreatePortalVertices ( pAreaLink->pDebugRegion, true, dwColor );
			}
		}
	}

	// complete
	return true;
}

bool cUniverse::RecurseCheckArea ( D3DXVECTOR3* pAtPos, D3DXVECTOR3* pvecToCenter, D3DXVECTOR3* pvecToSize, sArea* pParentArea, DWORD dwFrustumCount, sArea* pCurrentArea )
{
	// reject early - no area box - no seeing
	if ( !pCurrentArea )
		return false;

	// To box must overlap to some degree the areabox
	float fTrimArea = -0.01f;
	if ( pvecToCenter->x + pvecToSize->x >= pCurrentArea->vecMin.x+fTrimArea
	&&   pvecToCenter->y + pvecToSize->y >= pCurrentArea->vecMin.y+fTrimArea
	&&   pvecToCenter->z + pvecToSize->z >= pCurrentArea->vecMin.z+fTrimArea
	&&	 pvecToCenter->x - pvecToSize->x <= pCurrentArea->vecMax.x-fTrimArea
	&&   pvecToCenter->y - pvecToSize->y <= pCurrentArea->vecMax.y-fTrimArea
	&&   pvecToCenter->z - pvecToSize->z <= pCurrentArea->vecMax.z-fTrimArea )
	{
		// center of box must by within (5) units of areabox for light effect
//		float fTrimArea = 5.0f;
		float fTrimArea = 25.0f; //corridors with deep structures do not get lit (why need this at all?)
		if ( pvecToCenter->x >= pCurrentArea->vecMin.x-fTrimArea
		&&   pvecToCenter->y >= pCurrentArea->vecMin.y-fTrimArea
		&&   pvecToCenter->z >= pCurrentArea->vecMin.z-fTrimArea
		&&	 pvecToCenter->x <= pCurrentArea->vecMax.x+fTrimArea
		&&   pvecToCenter->y <= pCurrentArea->vecMax.y+fTrimArea
		&&   pvecToCenter->z <= pCurrentArea->vecMax.z+fTrimArea )
		{
			// check if point can be seen inside frustrum
			if ( dwFrustumCount==0 )
			{
				// no frustrum - full omni illumination
				return true;
			}
			else
			{
				// check if box within frustrum
				if ( CheckRectangleEx ( dwFrustumCount, pvecToCenter->x, pvecToCenter->y, pvecToCenter->z, pvecToSize->x, pvecToSize->y, pvecToSize->z ) )
				{
					// frustrum intersects box, record it
					D3DXPLANE* pCaptureFrustrumPlanes = new D3DXPLANE [ NUM_CULLPLANES ];
					for ( int iP=0; iP<NUM_CULLPLANES; iP++ )
						pCaptureFrustrumPlanes [ iP ] = g_Planes [ dwFrustumCount ][ iP ];

					// add to frustrum list for this box
					m_pFrustrumList.push_back ( pCaptureFrustrumPlanes );
				}
			}
		}
	}

	// recurse through any portals from this area
	for ( int iLink=0; iLink<pCurrentArea->iLinkMax; iLink++ )
	{
		// get link ptr
		sAreaLink* pAreaLink = pCurrentArea->pLink [ iLink ];
		if ( pAreaLink )
		{
			// find linked area box ptr
			int iLinkedArea = pAreaLink->iLinkedTo;
			sArea* pLinkedArea = m_pAreaList [ iLinkedArea ];

			// quick reject if portal is a partially blocked solid (window)
			if ( pAreaLink->iSolidity==1 )
				continue;

			// ensure we dont recurse backwards or go through portal that faces away
			if ( pLinkedArea != pParentArea && IsPortalFacingLookVector ( pAreaLink->iTouchingSide, &pAreaLink->vecPortalCenter, pAtPos )==true )
			{
				// ensure portal is in view of present frustrum (frustrum zero is omni)
				if ( dwFrustumCount>0 )
					if ( !CheckRectangleEx ( dwFrustumCount, pAreaLink->vecPortalCenter.x, pAreaLink->vecPortalCenter.y, pAreaLink->vecPortalCenter.z, pAreaLink->vecPortalDim.x, pAreaLink->vecPortalDim.y, pAreaLink->vecPortalDim.z ) )
						continue;

				// store frustrum
				D3DXPLANE StorePlanes [ NUM_CULLPLANES ];
				if ( dwFrustumCount>0 )
					for ( int iP=0; iP<NUM_CULLPLANES; iP++ )
						StorePlanes [ iP ] = g_Planes [ dwFrustumCount ][ iP ];

				// vectors
				D3DXVECTOR3 vecNewA = pAreaLink->vecA;
				D3DXVECTOR3 vecNewB = pAreaLink->vecB;
				D3DXVECTOR3 vecNewC = pAreaLink->vecC;
				D3DXVECTOR3 vecNewD = pAreaLink->vecD;

				// always ensure area not already traversed in this traverse-event (different from traversing back out and in another way!)
				bool bSameAreaIndexInTraverseEevent = false;
				if ( dwFrustumCount>1 )
					for ( DWORD dwTrIndex=0; dwTrIndex<dwFrustumCount; dwTrIndex++ )
						if ( m_dwRecurseTable [ dwTrIndex ]==iLinkedArea )
							bSameAreaIndexInTraverseEevent = true;

				// render area box linked to (if not done before)
				if ( bSameAreaIndexInTraverseEevent==false )
				{
					if ( dwFrustumCount<=18 )
					{
						// calculate a new child frustrum (frustrum zero is not valid as it has no shape, ie omni so last param=false)
						SetupPortalFrustum ( dwFrustumCount+1, pAtPos, &vecNewA, &vecNewB, &vecNewC, &vecNewD, false );

						// recurse through this child frustrum
						m_dwRecurseTable [ dwFrustumCount+1 ] = iLinkedArea;
						if ( RecurseCheckArea ( pAtPos, pvecToCenter, pvecToSize, pCurrentArea, dwFrustumCount+1, pLinkedArea )==true )
							return true;
					}
					else
					{
						// use last frustum for remaining nests beyond X
						m_dwRecurseTable [ 19 ] = iLinkedArea;
						if ( RecurseCheckArea ( pAtPos, pvecToCenter, pvecToSize, pCurrentArea, 19, pLinkedArea )==true )
							return true;
					}
				}

				// restore frustrum
				if ( dwFrustumCount>0 )
					for ( int iP=0; iP<NUM_CULLPLANES; iP++ )
						g_Planes [ dwFrustumCount ][ iP ] = StorePlanes [ iP ];
			}
		}
	}

	// did not detect full omni
	return false;
}

bool cUniverse::CanAreaPointSeeBox ( int iAtAreaBox, D3DXVECTOR3* pvecAt, D3DXVECTOR3* pvecToCenter, D3DXVECTOR3* pvecToSize )
{
	// start in current area and see if can see obj from light pos
	if ( iAtAreaBox > 0 )
	{
		DWORD dwFrustrumIndex = 0;
		m_dwRecurseTable [ dwFrustrumIndex ] = iAtAreaBox - 1;
		if ( RecurseCheckArea ( pvecAt, pvecToCenter, pvecToSize, NULL, dwFrustrumIndex, m_pAreaList [ iAtAreaBox-1 ] )==true )
		{
			// can see this point - direct omni illumination
			return true;
		}
	}
	else
	{
		// light outside ALL areaboxes, so light as much as possible (everything in range)
		return true;
	}

	// could not see point from omni
	return false;
}


void cUniverse::DrawDebug ( sNode* pNode )
{
	// draw node bounds and portals

	// check node list size
	if ( m_iNodeListSize == 0 )
		return;

	// draw area box debug view
	for ( int i = 0; i < (int)m_pAreaList.size ( ); i++ )
	{
		// do not draw area box currently in
		if ( g_iAreaBox != i+1 )
		{
			// draw whole area box for those not standing within
			DrawNodeBounds  ( m_pAreaList [ i ]->pDebugRegion );
			DrawNodePortals ( m_pAreaList [ i ]->pDebugRegion );
		}
		else
		{
			// for the area box we are in, draw portals out of the area
			for ( int iLink=0; iLink<m_pAreaList [ i ]->iLinkMax; iLink++ )
			{
				// area link to work on
				sAreaLink* pAreaLink = m_pAreaList [ i ]->pLink [ iLink ];

				// if visuals do not exist, and flagged, create them for debug purposes
				if ( pAreaLink->pDebugRegion==NULL && 1 )
				{
					pAreaLink->pDebugRegion = new sNode;
					pAreaLink->pDebugRegion->vecCentre    = pAreaLink->vecPortalMin + ( ( pAreaLink->vecPortalMax - pAreaLink->vecPortalMin ) / 2.0f );	
					pAreaLink->pDebugRegion->vecDimension = ( pAreaLink->vecPortalMax - pAreaLink->vecPortalMin ) / 2.0f;
					DWORD dwColor = D3DCOLOR_ARGB ( 255, 255, 0, 0 );
					if ( pAreaLink->iSolidity==1 ) dwColor = D3DCOLOR_ARGB ( 255, 0, 255, 0 );
					if ( pAreaLink->iSolidity==2 ) dwColor = D3DCOLOR_ARGB ( 255, 0, 0, 255 );
					CreatePortalVertices ( pAreaLink->pDebugRegion, true, dwColor );
				}

				// draw debug visual
				DrawNodePortals ( pAreaLink->pDebugRegion );
			}
		}
	}
}

bool cUniverse::BuildVisibility ( sNode* pNode )
{
	if ( !pNode )
		return true;

	// if a portal exists in the mesh then we know
	// it has a hole in so the portal must be visible
	if ( pNode->iMeshPortalCount == 0 )
	{
		// go through each mesh in the node
		for ( int iMesh = 0; iMesh < pNode->iMeshCount; iMesh++ )
		{
			// get pointer to mesh
			sMesh* pMesh = pNode->ppMeshList [ iMesh ];

			// quickly reject mesh if 'nothing' (2-solid,1-partial,0-not solid)
			int iSolidForVisibility=pMesh->iSolidForVisibility;
			if ( iSolidForVisibility==0 )
				continue;

			// iSolidForVisibility controls partially solid portals
			bool bVisStateFalse = false;
			if ( iSolidForVisibility==1 )
				bVisStateFalse = true;

			// * check bounding box of mesh - does it cover a side of node
			// * hide portal which object covers

			// set cover to false to start with
			int iPosition = -1;

			// use variables as lines start to get very long when referring straight
			// to the mesh and node, it's easier to read with smaller variable names
			float fMinX    = pMesh->Collision.vecMin.x;
			float fMinY    = pMesh->Collision.vecMin.y;
			float fMinZ    = pMesh->Collision.vecMin.z;
			float fMaxX    = pMesh->Collision.vecMax.x;
			float fMaxY    = pMesh->Collision.vecMax.y;
			float fMaxZ    = pMesh->Collision.vecMax.z;
			float fDimX    = pNode->vecDimension.x;
			float fDimY    = pNode->vecDimension.y;
			float fDimZ    = pNode->vecDimension.z;
			float fCentreX = pNode->vecCentre.x;
			float fCentreY = pNode->vecCentre.y;
			float fCentreZ = pNode->vecCentre.z;

			// due to floating point margins we have to use an epsilon as values can
			// be slightly off at times, if we don't test within an error range we
			// won't get any portals hidden
			//float epsilon = 0.01f;
			// X9 - 060208 - discovered that large meshes (wider than a node portal) would be rejected
			// from being given an 'iPosition' value because they are too wide as in the case of the maint. corr. deadend
			// which MinX + epsilon >= fCentreX - fDimX  meant 189.0 + 0.01 >= 250 - 50 = false even though it clearly
			// was at that end of the node and was totally blocking it, so we change the EPSILON to allow WIDER meshes by
			// up to 25.0f extra units which means even very thick walled corridors can now be placed so they can ultimately
			// be used to block visibility and seal up the corridor segments.
			float epsilon = 50.0f; // large enough to capture the Service Corridor widths (very wide)

			// now we need to find out where the mesh is in the node
			bool bLocationList [ 8 ];
			DetermineObjectLocation ( pNode, pMesh, bLocationList );
			
			// let's see if this mesh covers a whole top or bottom side
			if ( ( fMinX + epsilon >= fCentreX - fDimX ) && ( fMinX <= fCentreX - fDimX + epsilon ) )
			{
				if ( ( fMinZ + epsilon >= fCentreZ - fDimZ ) && ( fMinZ <= fCentreZ - fDimZ + epsilon ) )
				{
					if ( ( fMaxX + epsilon >= fCentreX + fDimX ) && ( fMaxX <= fCentreX + fDimX + epsilon ) )
					{
						if ( ( fMaxZ + epsilon >= fCentreZ + fDimZ ) && ( fMaxZ <= fCentreZ + fDimZ + epsilon ) )
						{
							if ( 
									bLocationList [ BottomLeftFront ] || bLocationList [ BottomLeftBack   ] ||
									bLocationList [ BottomRightBack ] || bLocationList [ BottomRightFront ]
							   )
									iPosition = eBottom;

							if ( 
									bLocationList [ TopLeftFront ] || bLocationList [ TopLeftBack   ] ||
									bLocationList [ TopRightBack ] || bLocationList [ TopRightFront ]
							   )
									iPosition = eTop;
						}
					}
				}
			}

			// see if we cover front or back of node
			if ( ( fMinX + epsilon >= fCentreX - fDimX ) && ( fMinX <= fCentreX - fDimX + epsilon ) )
			{
				if ( ( fMaxX + epsilon >= fCentreX + fDimX ) && ( fMaxX <= fCentreX + fDimX + epsilon ) )
				{
					if ( ( fMinY + epsilon >= fCentreY - fDimY ) && ( fMinY <= fCentreY - fDimY + epsilon ) )
					{
						if ( ( fMaxY + epsilon >= fCentreY + fDimY ) && ( fMaxY <= fCentreY + fDimY + epsilon ) )
						{
							if (
								 bLocationList [ TopLeftBack     ] || bLocationList [ BottomLeftBack ] ||
								 bLocationList [ BottomRightBack ] || bLocationList [ TopRightBack   ]
							   )
									iPosition = eBack;

							if (
								 bLocationList [ TopLeftFront     ] || bLocationList [ BottomLeftFront ] ||
								 bLocationList [ BottomRightFront ] || bLocationList [ TopRightFront   ]
							   )
									iPosition = eFront;
						}
					}
				}
			}

			// do we cover left or right side of node
			if ( ( fMinZ + epsilon >= fCentreZ - fDimZ ) && ( fMinZ <= fCentreZ - fDimZ + epsilon ) )
			{
				if ( ( fMaxZ + epsilon >= fCentreZ + fDimZ ) && ( fMaxZ <= fCentreZ + fDimZ + epsilon ) )
				{
					if ( ( fMinY + epsilon >= fCentreY - fDimY ) && ( fMinY <= fCentreY - fDimY + epsilon ) )
					{
						if ( ( fMaxY + epsilon >= fCentreY + fDimY ) && ( fMaxY <= fCentreY + fDimY + epsilon ) )
						{
							if (
								 bLocationList [ TopLeftFront    ] || bLocationList [ TopLeftBack    ] ||
								 bLocationList [ BottomLeftFront ] || bLocationList [ BottomLeftBack ]
							   )
									iPosition = eLeft;

							if (
								 bLocationList [ TopRightBack    ] || bLocationList [ TopRightFront    ] ||
								 bLocationList [ BottomRightBack ] || bLocationList [ BottomRightFront ]
							   )
									iPosition = eRight;
						}
					}
				}
			}

			if ( iPosition == eTop )
			{
				// only assign if can upgrade from nothing->partial or partial->solid
				if ( iSolidForVisibility>pNode->portals [ eTop ].iSolidityMode )
				{
					// now we can turn this portal off
					pNode->portals [ eTop ].bVisible = bVisStateFalse;
					pNode->portals [ eTop ].pBlocker = pMesh;
					pNode->portals [ eTop ].iSolidityMode = iSolidForVisibility;
					// apply to neighbor also
					sNode* pNeighbor = pNode->pNeighbours [ eTop ];
					if ( pNeighbor )
					{
						pNeighbor->portals [ eBottom ].bVisible = bVisStateFalse;
						pNeighbor->portals [ eBottom ].pBlocker = pMesh;
						pNeighbor->portals [ eBottom ].iSolidityMode = iSolidForVisibility;
					}
				}
			}

			if ( iPosition == eBottom )
			{
				// only assign if can upgrade from nothing->partial or partial->solid
				if ( iSolidForVisibility>pNode->portals [ eBottom ].iSolidityMode )
				{
					// now we can turn this portal off
					pNode->portals [ eBottom ].bVisible = bVisStateFalse;
					pNode->portals [ eBottom ].pBlocker = pMesh;
					pNode->portals [ eBottom ].iSolidityMode = iSolidForVisibility;
					// apply to neighbor also
					sNode* pNeighbor = pNode->pNeighbours [ eBottom ];
					if ( pNeighbor )
					{
						pNeighbor->portals [ eTop ].bVisible = bVisStateFalse;
						pNeighbor->portals [ eTop ].pBlocker = pMesh;
						pNeighbor->portals [ eTop ].iSolidityMode = iSolidForVisibility;
					}
				}

				// mike - 080803 - store highest vertex point -
				// lee - 271103 - why??
				pNode->portals [ eBottom ].fHighestPoint = -100000.0f;
				sOffsetMap	offsetMap;
				int			iIndexPosition = 0;
				BYTE*		pVertex        = pMesh->pVertexData;
				GetFVFOffsetMap ( pMesh, &offsetMap );
				for ( int iIndex = 0; iIndex < ( int ) pMesh->dwIndexCount / 3; iIndex++ )
				{
					float fNewY = *( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * pMesh->pIndices [ iIndexPosition ] ) );
					if ( fNewY > pNode->portals [ eBottom ].fHighestPoint )
						pNode->portals [ eBottom ].fHighestPoint = fNewY;
					iIndexPosition++;
				}
			}

			if ( iPosition == eBack )
			{
				// only assign if can upgrade from nothing->partial or partial->solid
				if ( iSolidForVisibility>pNode->portals [ eBack ].iSolidityMode )
				{
					pNode->portals [ eBack ].bVisible = bVisStateFalse;
					pNode->portals [ eBack ].pBlocker = pMesh;
					pNode->portals [ eBack ].iSolidityMode = iSolidForVisibility;
					sNode* pNeighbor = pNode->pNeighbours [ eBack ];
					if ( pNeighbor )
					{
						pNeighbor->portals [ eFront ].bVisible = bVisStateFalse;
						pNeighbor->portals [ eFront ].pBlocker = pMesh;
						pNeighbor->portals [ eFront ].iSolidityMode = iSolidForVisibility;
					}
				}
			}

			if ( iPosition == eFront )
			{
				// only assign if can upgrade from nothing->partial or partial->solid
				if ( iSolidForVisibility>pNode->portals [ eFront ].iSolidityMode )
				{
					pNode->portals [ eFront ].bVisible = bVisStateFalse;
					pNode->portals [ eFront ].pBlocker = pMesh;
					pNode->portals [ eFront ].iSolidityMode = iSolidForVisibility;
					sNode* pNeighbor = pNode->pNeighbours [ eFront ];
					if ( pNeighbor )
					{
						pNeighbor->portals [ eBack ].bVisible = bVisStateFalse;
						pNeighbor->portals [ eBack ].pBlocker = pMesh;
						pNeighbor->portals [ eBack ].iSolidityMode = iSolidForVisibility;
					}
				}
			}

			if ( iPosition == eLeft )
			{
				// only assign if can upgrade from nothing->partial or partial->solid
				if ( iSolidForVisibility>pNode->portals [ eLeft ].iSolidityMode )
				{
					pNode->portals [ eLeft ].bVisible = bVisStateFalse;
					pNode->portals [ eLeft ].pBlocker = pMesh;
					pNode->portals [ eLeft ].iSolidityMode = iSolidForVisibility;
					sNode* pNeighbor = pNode->pNeighbours [ eLeft ];
					if ( pNeighbor )
					{
						pNeighbor->portals [ eRight ].bVisible = bVisStateFalse;
						pNeighbor->portals [ eRight ].pBlocker = pMesh;
						pNeighbor->portals [ eRight ].iSolidityMode = iSolidForVisibility;
					}
				}
			}

			if ( iPosition == eRight )
			{
				// only assign if can upgrade from nothing->partial or partial->solid
				if ( iSolidForVisibility>pNode->portals [ eRight ].iSolidityMode )
				{
					pNode->portals [ eRight ].bVisible = bVisStateFalse;
					pNode->portals [ eRight ].pBlocker = pMesh;
					pNode->portals [ eRight ].iSolidityMode = iSolidForVisibility;
					sNode* pNeighbor = pNode->pNeighbours [ eRight ];
					if ( pNeighbor )
					{
						pNeighbor->portals [ eLeft ].bVisible = bVisStateFalse;
						pNeighbor->portals [ eLeft ].pBlocker = pMesh;
						pNeighbor->portals [ eLeft ].iSolidityMode = iSolidForVisibility;
					}
				}
			}
		}
	}

	return true;
}

void cUniverse::DrawRayCast ( void )
{
	DWORD dwFVF = 0;
	m_pD3D->GetFVF ( &dwFVF );
	m_pD3D->SetFVF ( D3DFVF_XYZ );

	D3DXVECTOR3 vecLines [ 2 ];
	vecLines [ 0 ] = m_vecLineStart;
	vecLines [ 1 ] = m_vecLineEnd;

	// white ray cast line
	m_pD3D->SetRenderState ( D3DRS_CULLMODE,  D3DCULL_NONE );
	m_pD3D->SetRenderState ( D3DRS_LIGHTING,  FALSE );
	m_pD3D->SetTextureStageState ( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	m_pD3D->SetTextureStageState ( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );

	m_pD3D->DrawPrimitiveUP (
								D3DPT_LINELIST,
								1,
								&vecLines,
								sizeof ( float ) * 3
							);

	m_pD3D->SetFVF ( dwFVF );
}

void cUniverse::SetWorldMatrix ( void )
{
	// set the world matrix

	// create a scaling and position matrix
	D3DXMATRIX	matScale,
				matTranslation,
				matRotation,
				matWorld,
				matRotateX,
				matRotateY,
				matRotateZ;

	D3DXMatrixScaling     ( &matScale,       1.0f, 1.0f, 1.0f );
	D3DXMatrixTranslation ( &matTranslation, 0.0f, 0.0f, 0.0f );

	// handle rotations
	D3DXMatrixRotationX ( &matRotateX, D3DXToRadian ( 0.0f ) );
	D3DXMatrixRotationY ( &matRotateY, D3DXToRadian ( 0.0f ) );
	D3DXMatrixRotationZ ( &matRotateZ, D3DXToRadian ( 0.0f ) );

	// build up final rotation and world matrix
	matRotation = matRotateX  * matRotateY * matRotateZ;
	matWorld    = matRotation * matScale   * matTranslation;

	// set world transform for node tree base
	if ( m_pD3D )
		m_pD3D->SetTransform ( D3DTS_WORLD, &matWorld );
}

bool cUniverse::RenderArea ( DWORD dwFrustumCount, sArea* pArea )
{
	// record draw prims before main render
	DWORD dwStoredPolygonsDrawn, dwStoredDrawPrimCount;
	if ( g_pGlob )
	{
		dwStoredPolygonsDrawn = g_pGlob->dwNumberOfPolygonsDrawn;
		dwStoredDrawPrimCount = g_pGlob->dwNumberOfPrimCalls;
	}

	// render area box
	if ( m_pMasterMeshList.size()>0 )
	{
	    int iIndex = 0;
	    
		// render all meshgroups within areabox
		for ( iIndex = 0; iIndex < (int)pArea->meshgroups.size ( ); iIndex++ )
		{
			sMesh* pMesh = pArea->meshgroups [ iIndex ]->pMesh;
			if ( pMesh )
			{
				if ( pMesh->dwDrawSignature != m_dwDrawSignature )
				{
					pMesh->dwDrawSignature = m_dwDrawSignature;
					if ( m_bGhostDebug ) pMesh->bWireframe=true; else pMesh->bWireframe=false;
					m_ObjectManager.DrawMesh ( pMesh );
				}
			}
		}
		// render all 'shared' meshgroups within areabox
		for ( iIndex = 0; iIndex < (int)pArea->sharedmeshgroups.size ( ); iIndex++ )
		{
			sMesh* pMesh = pArea->sharedmeshgroups [ iIndex ]->pMesh;
			if ( pMesh )
			{
				if ( pMesh->dwDrawSignature != m_dwDrawSignature )
				{
					pMesh->dwDrawSignature = m_dwDrawSignature;
					if ( m_bGhostDebug ) pMesh->bWireframe=true; else pMesh->bWireframe=false;
					m_ObjectManager.DrawMesh ( pMesh );
				}
			}
		}
		// render all meshgroup references outside but connected to areabox
		for ( int iRef = 0; iRef < (int)pArea->meshgroupref.size ( ); iRef++ )
		{
			sMeshGroup* pMeshGroup = pArea->meshgroupref [ iRef ];
			if ( pMeshGroup )
			{
				sMesh* pMesh = pMeshGroup->pMesh;
				if ( pMesh )
				{
					if ( pMesh->dwDrawSignature != m_dwDrawSignature )
					{
						pMesh->dwDrawSignature = m_dwDrawSignature;
						if ( m_bGhostDebug ) pMesh->bWireframe=true; else pMesh->bWireframe=false;
						m_ObjectManager.DrawMesh ( pMesh );
					}
				}
			}
		}
	}
	else
	{
		// when universe is not optimized (by saving it), this is used
		// old way, go through all nodes within the area
		for ( int iNode = 0; iNode < (int)pArea->nodes.size ( ); iNode++ )
		{
			// get a pointer to the node
			sNode* pNode = pArea->nodes [ iNode ];

			// only draw node once per cycle
			if ( m_pNodeUseFlagMap [ pNode->dwNodeRefIndex ]==false )
			{
				// quick reject camera frustrum check
				if ( !CheckRectangleEx ( dwFrustumCount, pNode->vecCentre.x, pNode->vecCentre.y, pNode->vecCentre.z, pNode->vecDimension.x, pNode->vecDimension.y, pNode->vecDimension.z ) )
					continue;

				// draw mesh in any old order for now
				for ( int iMesh = 0; iMesh < pNode->iMeshCount; iMesh++ )
				{
					sMesh* pMesh = pNode->ppMeshList [ iMesh ];
					if ( pMesh->dwDrawSignature != m_dwDrawSignature )
					{
						// ensure only one mesh per cycle
						pMesh->dwDrawSignature = m_dwDrawSignature;

						// draw mesh from universe
						if ( m_bGhostDebug ) pMesh->bWireframe=true; else pMesh->bWireframe=false;
						m_ObjectManager.DrawMesh ( pMesh );
					}
				}

				// node used this cycle
				m_pNodeUseFlagMap [ pNode->dwNodeRefIndex ]=true;
			}
		}
	}

	// record frustrum zero drawprims for statistics
	if ( g_pGlob && dwFrustumCount==0 )
	{
		m_dwNumberOfCurrentAreaBoxPolygonsDrawn = g_pGlob->dwNumberOfPolygonsDrawn - dwStoredPolygonsDrawn;
		m_dwNumberOfCurrentAreaBoxPrimCalls = g_pGlob->dwNumberOfPrimCalls - dwStoredDrawPrimCount;
	}

	// set this as rendered (for attached obj visibility code)
	pArea->bRenderedThisCycle = true;

	// complete
	return true;
}

bool cUniverse::IsPortalFacingLookVector ( int iSide, D3DXVECTOR3* pvecPortalCenter, D3DXVECTOR3* pvecAt )
{
	// is detection directional?
	if ( pvecPortalCenter && pvecAt )
	{
		// side normal
		D3DXVECTOR3 vecSideNormal;
		switch ( iSide )
		{
			case eBack :	vecSideNormal = D3DXVECTOR3( 0, 0, 1);	break;
			case eFront :	vecSideNormal = D3DXVECTOR3( 0, 0,-1);	break;
			case eLeft :	vecSideNormal = D3DXVECTOR3( 1, 0, 0);	break;
			case eRight :	vecSideNormal = D3DXVECTOR3(-1, 0, 0);	break;
			case eTop :		vecSideNormal = D3DXVECTOR3( 0, 1, 0);	break;
			case eBottom :	vecSideNormal = D3DXVECTOR3( 0,-1, 0);	break;
		}

		// if camera normal faces side normal, we must draw beyond
		D3DXVECTOR3 vecLookNormal = *pvecPortalCenter-*pvecAt;
		float fDiff = D3DXVec3Dot ( &vecLookNormal, &vecSideNormal );
		if ( fDiff >= 0.0f )
		{
			// portal faces the camera
			return true;
		}
	}
	else
	{
		// vector amin0directional
		return true;
	}

	// portal faces away
	return false;
}

int limitrecursions = 0;
int* pReportRecursion = NULL;

bool cUniverse::RecurseRenderArea ( sArea* pParentArea, DWORD dwFrustumCount, sArea* pCurrentArea )
{
	// reject early
	if ( !pCurrentArea )
		return true;

	// render this area
	RenderArea ( dwFrustumCount, pCurrentArea );

	// recurse through any portals from this area
	for ( int iLink=0; iLink<pCurrentArea->iLinkMax; iLink++ )
	{
		// get link ptr
		sAreaLink* pAreaLink = pCurrentArea->pLink [ iLink ];
		if ( pAreaLink )
		{
			// find linked area box ptr
			int iLinkedArea = pAreaLink->iLinkedTo;
			sArea* pLinkedArea = m_pAreaList [ iLinkedArea ];

			// x9 - 060208 - if this area has ALREADY been rendered, no need to traverse it again
			// which solves the iterative chaos that slows a 38fps game down to 10fps. However is
			// there a side effect of not allowing an area to be used by a different traversal in
			// cases where two doors (portals) go through to the same room (areabox), the second
			// traversal fails to explore the portals view fully, so we only skip the areabox if
			// the areabox in question is an OUTSIDE areabox (not an inside one with rooms that
			// must always be considered). So areas with no meshes inside it and already used
			// must be airspace that we have already traversed and will not cause us to cull
			// parts of the scene that multiple portal routes may have directed us to, which
			// hopefully excludes large area spaces within a large room with portals leading in
			// and out of that space (phew).
			// X9 - V110 - 230508 - UMAN had large FPSC level which had a large amount of 
			// interior spaces that where portal divided, and the code below caused some paths
			// not to render causing geometry to disappear, so..
			// V111 - 140608 - we must distinguish between an INTERIOR blank space, and an
			// exterior blank space which I think can be done because sharedmeshgroups is
			// not zero when it is an interior space (so code commented back in to operation)
			if ( m_iFullPortalRecurse==0 )
			{
				// quick portal resurse (only primary meshgroup check made
				if ( pLinkedArea->meshgroups.size()==0 )
					if ( pLinkedArea->bRenderedThisCycle==true )
						continue;
			}
			else
			{
				// full portal recurse (checks main AND shared groups)
				if ( pLinkedArea->meshgroups.size()==0 )
					if ( pLinkedArea->sharedmeshgroups.size()==0 ) // V111 - 140608 - added
						if ( pLinkedArea->bRenderedThisCycle==true )
							continue;
			}

			// ensure we dont recurse backwards
			if ( (dwFrustumCount==0 || IsPortalFacingLookVector ( pAreaLink->iTouchingSide, &pAreaLink->vecPortalCenter, &m_pCameraData->vecPosition )==true)
			&&   pLinkedArea != pParentArea )
			{
				// ensure portal is in view of present frustrum
				// leelee test always
				if ( !CheckRectangleEx ( dwFrustumCount, pAreaLink->vecPortalCenter.x, pAreaLink->vecPortalCenter.y, pAreaLink->vecPortalCenter.z, pAreaLink->vecPortalDim.x, pAreaLink->vecPortalDim.y, pAreaLink->vecPortalDim.z ) )
					continue;
					
				int iP = 0;

				// store frustrum
				D3DXPLANE StorePlanes [ NUM_CULLPLANES ];
				for ( iP=0; iP<NUM_CULLPLANES; iP++ )
					StorePlanes [ iP ] = g_Planes [ dwFrustumCount ][ iP ];

				// vectors
				D3DXVECTOR3 vecNewA, vecNewB, vecNewC, vecNewD;
				D3DXVECTOR3 vecCam = m_pCameraData->vecPosition;
				vecNewA = pAreaLink->vecA;
				vecNewB = pAreaLink->vecB;
				vecNewC = pAreaLink->vecC;
				vecNewD = pAreaLink->vecD;

				// always ensure area not already traversed in this traverse-event (different from traversing back out and in another way!)
				bool bSameAreaIndexInTraverseEevent = false;
				if ( dwFrustumCount>1 )
					for ( DWORD dwTrIndex=0; dwTrIndex<dwFrustumCount; dwTrIndex++ )
						if ( m_dwRecurseTable [ dwTrIndex ]==iLinkedArea )
							bSameAreaIndexInTraverseEevent = true;

				// render area box linked to (if not done before)
				if ( bSameAreaIndexInTraverseEevent==false )
				{
					if ( dwFrustumCount<=18 )
					{
						// upto X frustum nest checks
						SetupPortalFrustum ( dwFrustumCount+1, &vecCam, &vecNewA, &vecNewB, &vecNewC, &vecNewD, true );

						// recurse through child frustrum
						m_dwRecurseTable [ dwFrustumCount+1 ] = iLinkedArea;
						RecurseRenderArea ( pCurrentArea, dwFrustumCount+1, pLinkedArea );
					}
					else
					{
						// use last frustum for remaining nests beyond X
						m_dwRecurseTable [ 19 ] = iLinkedArea;
						RecurseRenderArea ( pCurrentArea, 19, pLinkedArea );
					}
				}

				// restore frustrum
				for ( iP=0; iP<NUM_CULLPLANES; iP++ )
					g_Planes [ dwFrustumCount ][ iP ] = StorePlanes [ iP ];
			}
		}
	}

	// complete
	return true;
}

bool cUniverse::Render ( void )
{
	// ensure created
	if ( !m_bCreated )
		return false;

	// exist still exists
	SAFE_MEMORY ( m_pNodeList );

	// set the world matrix
	SetWorldMatrix ( );

	// gather camera zero details (V111 - 100608 - stereoscopics requires current camera to be X)
	//m_pCameraData = ( tagCameraData* ) GetCameraInternalData ( 0 );
	m_pCameraData = ( tagCameraData* ) GetCameraInternalData ( g_pGlob->dwCurrentSetCameraID );
	
	// determine areabox-location of camera (first check current, as 99% of the time you are still there)
	bool bFoundBox = false;
	if ( g_iAreaBox > 0 )
	{
		if ( CollisionBoundBoxTest (	&m_pCameraData->vecPosition,
										&m_pCameraData->vecPosition,
										&m_pAreaList [ g_iAreaBox-1 ]->vecMin,
										&m_pAreaList [ g_iAreaBox-1 ]->vecMax  ) ) bFoundBox = true;
	}
	if ( bFoundBox==false )
	{
		int iNewAreaBox = 0;
		for ( int i = 0; i < (int)m_pAreaList.size ( ); i++ )
		{
			if ( CollisionBoundBoxTest (	&m_pCameraData->vecPosition,
											&m_pCameraData->vecPosition,
											&m_pAreaList [ i ]->vecMin,
											&m_pAreaList [ i ]->vecMax	  ) )
			{
				iNewAreaBox = 1+i;
				break;
			}
		}

		// leemod - 210105 - do not loose last area box if leave, keep visibility of last state of universe (leap out of top of it)
		if ( iNewAreaBox>0 )
			g_iAreaBox = iNewAreaBox;
	}

	// clear visibility flags of the areaboxes
	// U75 - 290310 - can only account for camera zero (as only have one flag to hold the rendered state
	if ( g_pGlob->dwRenderCameraID==0 )
	{
		for ( int i = 0; i < (int)m_pAreaList.size ( ); i++ )
			m_pAreaList [ i ]->bRenderedThisCycle = false;
	}

	// draw area within
	if ( g_iAreaBox > 0 )
	{
		// draw signature (so meshes only draw once)
		m_dwDrawSignature++; if ( m_dwDrawSignature>65535 ) m_dwDrawSignature=0;

		// clear node usage flags (nodes only need adding to scene once)
		if ( m_pNodeUseFlagMap ) memset ( m_pNodeUseFlagMap, 0, sizeof(bool) * m_dwNodeUseFlagMapSize );

		// start with full camera frustrum
		SetupFrustum ( 0.0f );

		// pre draw state resets
		m_ObjectManager.PreDrawSettings();

		// start in current area and recursively draw out from it
		DWORD dwFrustrumIndex = 0;
		m_dwRecurseTable [ dwFrustrumIndex ] = g_iAreaBox - 1;
		RecurseRenderArea ( NULL, dwFrustrumIndex, m_pAreaList [ g_iAreaBox-1 ] );
	}

	// all objects in visible areabox to be visible
	int iVisLinkedObjMax = m_VisLinkedObjectList.size ( );
	for ( int iObj = 0; iObj < iVisLinkedObjMax; iObj++ )
	{
		// object ptr
		sObject* pObject = m_VisLinkedObjectList [ iObj ];
		if ( pObject )
		{
			// adjust object slightly so small downward penetration is allowed (fpsc-190805)
			D3DXVECTOR3 vecAdjPos = pObject->position.vecPosition;
			vecAdjPos.y+=5.0f;

			// FPGC - 050410 - use 'middle' of object to determine which areabox the obj is in
			vecAdjPos += pObject->collision.vecColCenter;

			// first check previous areabox the object was in
			if ( pObject->iInsideUniverseArea>=0 )
			{
				if ( CollisionBoundBoxTest (	&vecAdjPos,
												&vecAdjPos,
												&m_pAreaList [ pObject->iInsideUniverseArea ]->vecMin,
												&m_pAreaList [ pObject->iInsideUniverseArea ]->vecMax	  )==false )
				{
					// moved from areabox
					pObject->iInsideUniverseArea = -1;
				}
			}
			// find area box holding the object
			if ( pObject->iInsideUniverseArea==-1 )
			{
				for ( int i = 0; i < (int)m_pAreaList.size ( ); i++ )
				{
					if ( CollisionBoundBoxTest (	&vecAdjPos,
													&vecAdjPos,
													&m_pAreaList [ i ]->vecMin,
													&m_pAreaList [ i ]->vecMax ) )
					{
						pObject->iInsideUniverseArea = i;
						break;
					}
				}
			}
			// set visibility from areabox state
			if ( pObject->iInsideUniverseArea>=0 )
			{
				// U75 - 290310 - can only account for camera zero (as only have one flag to hold the rendered state
				if ( g_pGlob->dwRenderCameraID==0 )
				{
					// still in this areabox, transfer visiblity state over to object
					pObject->bUniverseVisible = m_pAreaList [ pObject->iInsideUniverseArea ]->bRenderedThisCycle;
				}
				else
					pObject->bUniverseVisible = true;
			}
		}
	}

	// if scorch active, render it
	if ( m_pScorchMesh )
	{
		// renders mesh over top of universe
		m_ObjectManager.DrawMesh ( m_pScorchMesh );
	}

	// if debug active
	if ( m_bDebug )
	{
		DrawDebug ( NULL );
		if ( m_bRayCast ) DrawRayCast ( );
	}

	return true;
}

int cUniverse::RayCast ( float fX, float fY, float fZ, float fNewX, float fNewY, float fNewZ )
{
	// set the flag to true
	m_bRayCast = true;

	// perform single ray cast
	return CollisionRayCast ( fX, fY, fZ, fNewX, fNewY, fNewZ );
}

bool cUniverse::RayVolume ( float fX, float fY, float fZ, float fNewX, float fNewY, float fNewZ, float fSize )
{
	if ( CollisionRayVolume ( fX, fY, fZ, fNewX, fNewY, fNewZ, fSize ) )
		return true;
	else
		return false;

	return false;
}

bool cUniverse::CollisionQuickRayCast ( sMesh* pMesh, float fX, float fY, float fZ, float fNewX, float fNewY, float fNewZ )
{
	// for best result from cast
	float fBestDistance = 99999.0f;
	float fBestU, fBestV;
	sMesh* pBestMesh;
	int iBestTriangle;
	if ( CollisionSingleRayCast ( pMesh, fX, fY, fZ, fNewX, fNewY, fNewZ, &fBestDistance, &fBestU, &fBestV, &pBestMesh, &iBestTriangle, false ) )
		return true;
	else
		return false;
}

bool cUniverse::CollisionSingleRayCast (	sMesh* pMesh, float fX, float fY, float fZ, float fNewX, float fNewY, float fNewZ,
											float* fBestDistance, 
											float* fBestU, 
											float* fBestV, 
											sMesh** pBestMesh,
											int* iBestTriangle,
											bool bCollectFeedback )
{
	// true if best updated
	bool bResult=false;

	// extra data collected for checklist feedback
	int iRefToObject=0, iRefToLimbOfTheObject=0;
	DWORD dwVertex0IndexOfHitPoly, dwVertex1IndexOfHitPoly, dwVertex2IndexOfHitPoly;
	D3DXVECTOR3 vec0Hit, vec1Hit, vec2Hit;
	D3DXVECTOR3 vecHitPoint;

	// vectors
	m_vecLineStart			 = D3DXVECTOR3 ( fX, fY, fZ );				// starting point
	m_vecLineEnd			 = D3DXVECTOR3 ( fNewX, fNewY, fNewZ );		// end point
	D3DXVECTOR3 vecDifference = m_vecLineEnd - m_vecLineStart;			// direction

	// reduce the length of the direction vector
	D3DXVECTOR3 vecDirection;
	D3DXVec3Normalize ( &vecDirection, &vecDifference );

	// get a pointer to the vertex data
	BYTE* pVertex = pMesh->pVertexData;
	int   iIndex  = 0;

	// get the offset map
	sOffsetMap	 offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// indices or vertonly
	int iIndexMax = 0;
	if ( pMesh->dwIndexCount>0 )
		iIndexMax = (int)pMesh->dwIndexCount/3;
	else
		iIndexMax = (int)pMesh->dwVertexCount/3;

	// now go through each triangle
	for ( int iTriangle = 0; iTriangle < iIndexMax; iTriangle++ )
	{
		// get each vector
		D3DXVECTOR3 vecA, vecB, vecC;
		if ( pMesh->dwIndexCount>0 )
		{
			// indices
			vecA = D3DXVECTOR3 ( 
												*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * pMesh->pIndices [ iIndex   ] ) ),
												*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * pMesh->pIndices [ iIndex   ] ) ),
												*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * pMesh->pIndices [ iIndex   ] ) )
										   );
			iIndex++;

			vecB = D3DXVECTOR3 ( 
												*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * pMesh->pIndices [ iIndex   ] ) ),
												*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * pMesh->pIndices [ iIndex   ] ) ),
												*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * pMesh->pIndices [ iIndex   ] ) )
										   );
			iIndex++;

			vecC = D3DXVECTOR3 ( 
												*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * pMesh->pIndices [ iIndex   ] ) ),
												*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * pMesh->pIndices [ iIndex   ] ) ),
												*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * pMesh->pIndices [ iIndex   ] ) )
										   );
			iIndex++;
		}
		else
		{
			// vertonly
			vecA = D3DXVECTOR3 ( 
												*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * iIndex ) ),
												*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * iIndex ) ),
												*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * iIndex ) )
										   );
			iIndex++;

			vecB = D3DXVECTOR3 ( 
												*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * iIndex ) ),
												*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * iIndex ) ),
												*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * iIndex ) )
										   );
			iIndex++;

			vecC = D3DXVECTOR3 ( 
												*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * iIndex ) ),
												*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * iIndex ) ),
												*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * iIndex ) )
										   );
			iIndex++;
		}

		// tri test against ray
		float fU, fV, fDistance = 0.0f;
		if ( D3DXIntersectTri ( &vecA, &vecB, &vecC, &m_vecLineStart, &vecDirection, &fU, &fV, &fDistance ) )
		{
			// we have a collision - is the closest?
			if ( fDistance < (*fBestDistance) )
			{
				// instant feedback data (must be put back eventually)
				*fBestDistance = fDistance;
				*fBestU = fU;
				*fBestV = fV;
				*pBestMesh = pMesh;
				*iBestTriangle = iTriangle;

				// store extra data
				if ( bCollectFeedback )
				{
					if ( pMesh->dwIndexCount>0 )
					{
						dwVertex0IndexOfHitPoly = pMesh->pIndices [ iIndex-3 ];
						dwVertex1IndexOfHitPoly = pMesh->pIndices [ iIndex-2 ];
						dwVertex2IndexOfHitPoly = pMesh->pIndices [ iIndex-1 ];
					}
					else
					{
						dwVertex0IndexOfHitPoly = iIndex-3;
						dwVertex1IndexOfHitPoly = iIndex-2;
						dwVertex2IndexOfHitPoly = iIndex-1;
					}
					vec0Hit = vecA; vec1Hit = vecB; vec2Hit = vecC;

					// new info
					vecHitPoint = m_vecLineStart + (vecDirection*fDistance);
				}

				// flag as hit
				bResult=true;
			}
		}
	}

	// if success, store extra data in feedback
	if ( bResult==true )
	{
		// collect data from final best hit
		if ( bCollectFeedback )
		{
			// get normal of polygon
			D3DXVECTOR3 vecNormal = D3DXVECTOR3 ( 	*( ( float* ) pVertex + offsetMap.dwNX + ( offsetMap.dwSize * dwVertex0IndexOfHitPoly ) ),
													*( ( float* ) pVertex + offsetMap.dwNY + ( offsetMap.dwSize * dwVertex0IndexOfHitPoly ) ),
													*( ( float* ) pVertex + offsetMap.dwNZ + ( offsetMap.dwSize * dwVertex0IndexOfHitPoly ) )   );

			// calculate the direction if the impact bounced off
			D3DXVECTOR3 vecReflectedNormal = vecNormal - ((vecDirection)*-1.0f);
			D3DXVec3Normalize ( &vecReflectedNormal, &vecReflectedNormal );
			vecReflectedNormal = vecReflectedNormal * ((*fBestDistance)/2.0f);

			// create a checklist to store all collision feedback
			MegaCollisionFeedback.iFrameCollision = iRefToObject;
			MegaCollisionFeedback.iFrameRelatedToBone = iRefToLimbOfTheObject;
			MegaCollisionFeedback.dwVertex0IndexOfHitPoly = dwVertex0IndexOfHitPoly;
			MegaCollisionFeedback.dwVertex1IndexOfHitPoly = dwVertex1IndexOfHitPoly;
			MegaCollisionFeedback.dwVertex2IndexOfHitPoly = dwVertex2IndexOfHitPoly;
			MegaCollisionFeedback.vec0Hit = vec0Hit;
			MegaCollisionFeedback.vec1Hit = vec1Hit;
			MegaCollisionFeedback.vec2Hit = vec2Hit;
			MegaCollisionFeedback.vecHitPoint = vecHitPoint;
			MegaCollisionFeedback.vecNormal = vecNormal;
			MegaCollisionFeedback.vecReflectedNormal = vecReflectedNormal;
			MegaCollisionFeedback.pHitMesh = pMesh;
			CreateCollisionChecklist();

			// also store in static data structure (for instant retrieval)
			g_DBPROCollisionResult.dwArbitaryValue = pMesh->Collision.dwArbitaryValue;
		}
	}

	// success
	return bResult;
}

