#include "universe.h"
#include "CommonC.h"
#include "BoxCollision\CCollision.h"
#include "ElipsoidCollision\Collision.h"

#include <stdio.h>
#include <mmsystem.h>
#include <algorithm>
#include <vector>
#include <list>
using namespace std;

#include ".\\compiler\\cbsptree.h"
#include ".\\compiler\\cportals.h"
#include ".\\compiler\\ccompiler.h"

extern void ComputeBoundValues ( int iPass, D3DXVECTOR3 vecXYZ, D3DXVECTOR3* pvecMin, D3DXVECTOR3* pvecMax, D3DXVECTOR3* pvecCenter, float* pfRadius );

cUniverse::sNode::sNode ( )
{
    location = TopLeftFront;
    ppMeshList = NULL;
    pMeshPortalList = NULL;
    iMeshCount = 0;
    iMeshPortalCount = 0;
    iPolygonCount = 0;
    iMaxPolygonCount = 2;
    bChecked = false;
    bClear = false;
    vecCentre = D3DXVECTOR3( 0.0, 0.0, 0.0 );
    vecDimension = D3DXVECTOR3( 0.0, 0.0, 0.0 );

    for (int i = 0; i < 6; ++i)
        pNeighbours[i] = NULL;

    vecColMin = D3DXVECTOR3( 0.0, 0.0, 0.0 );
    vecColMax = D3DXVECTOR3( 0.0, 0.0, 0.0 );
    dwNodeRefIndex = 0;
}
cUniverse::sNode::~sNode ( )
{
	SAFE_DELETE_ARRAY ( ppMeshList );
}

cUniverse::sPortal::sPortal ( )
{
	// clear out the vertex list
	// 20120327 IanM - Correctly clear the array
	memset ( vertices, 0, sizeof ( vertices ) );
//	memset ( &vertices, 0, sizeof ( vertices ) );

	// set both visible flags to true
	bVisible                 = true;
	bVisibleInViewingFrustum = true;
}

cUniverse::sArea::sArea ( )
{
    vecMin = D3DXVECTOR3( 0.0, 0.0, 0.0 );
    vecMax = D3DXVECTOR3( 0.0, 0.0, 0.0 );
    vecCentre = D3DXVECTOR3( 0.0, 0.0, 0.0 );
    iLinkMax = 0;
    iCount = 0;
    pDebugRegion = NULL;
    bRenderedThisCycle = false;
}

cUniverse::cUniverse ( )
{
	// initial set up
	
	// set pointers to null
	m_pNodeList			    = NULL;
	m_pNode                 = NULL;
	m_pNodeRef				= NULL;
	m_pNodeUseFlagMap		= NULL;
	
	// set bool flags to false
	m_bDebug			    = false;
	m_bGhostDebug           = false;
	m_bCreated			    = false;
	m_bPortalsActivated		= false;
	m_bPortalsCreated		= false;
	m_bRayCast              = false;
	
	// initial values for vectors
	m_vecLineStart          = D3DXVECTOR3 (        0.0f,        0.0f,        0.0f );
	m_vecLineEnd            = D3DXVECTOR3 (        0.0f,        0.0f,        0.0f );
	m_vecNodeMax            = D3DXVECTOR3 ( -1000000.0f, -1000000.0f, -1000000.0f );
	m_vecNodeMin            = D3DXVECTOR3 (  1000000.0f,  1000000.0f,  1000000.0f );

	// clear vis linked object list
	m_VisLinkedObjectList.clear ( );
	m_VisLinkedObjectInNodeList.clear ( );

	// clear lists out
	m_StaticObjectList.clear ( );
	m_pMeshList.clear ( );
	m_iMeshCollisionList.clear ( );
	m_pColMeshList.clear ( );
	m_pAreaList.clear ( );
	m_pMeshShortList.clear ( );

	// clear scorch ptr
	m_pScorchMesh			= NULL;
	m_iScorchTypeMax		= 0;
	m_dwScorchVPos			= 0;
	m_dwPolyDrawLimit		= 0;

	// clear shadow data
	m_pShadowCasterMasterList.clear();
	m_pShadowLightList.clear();

	// clear areabox scanning work vars
	m_pFrustrumList.clear();

	// member data
	m_pCameraData=NULL;
	m_dwColour=0;
	m_bMoveLeft=false;

	// Scorch Data
	m_pScorchMesh=NULL;
	m_dwScorchVPos=0;
	m_dwPolyDrawLimit=0;
	m_iScorchTypeMax=0;
	m_iScorchUVWidth=0;
	m_fScorchUTile=0;
	m_fScorchVTile=0;

	// Working collision pool
	m_pCollisionPool=NULL;
	m_pCollisionDiffuse=NULL;
	m_dwCollisionPoolMax=0;
	m_dwCollisionPoolIndex=0;

	// Shadow Data
	m_pShadowCasterMasterList.clear();
	m_pShadowLightList.clear();

	// runtime cycle properties
	m_dwDrawSignature=0;
	memset ( m_dwRecurseTable, 0, sizeof ( m_dwRecurseTable ) );
}

cUniverse::~cUniverse ( )
{
	// free any internal textures this static universe used (lightmaps)
	if ( m_pMasterMeshList.size ( ) > 0 )
		for ( int i = 0; i < (int)m_pMasterMeshList.size ( ); i++ )
			FreeInternalTextures ( m_pMasterMeshList [ i ] );

	// delete the nodes in universe
	SAFE_DELETE ( m_pNodeList );
	SAFE_DELETE_ARRAY ( m_pNode );
	SAFE_DELETE_ARRAY ( m_pNodeUseFlagMap );
	SAFE_DELETE_ARRAY ( m_pNodeRef );

	// clear m_pMeshList
	if ( m_pMeshList.size ( ) > 0 )
		for ( int i = 0; i < (int)m_pMeshList.size ( ); i++ )
			SAFE_DELETE ( m_pMeshList [ i ] );

	// clear m_pColMeshList
	if ( m_pColMeshList.size ( ) > 0 )
		for ( int i = 0; i < (int)m_pColMeshList.size ( ); i++ )
			SAFE_DELETE ( m_pColMeshList [ i ] );

	// clear m_pAreaList
	if ( m_pAreaList.size ( ) > 0 )
		for ( int i = 0; i < (int)m_pAreaList.size ( ); i++ )
			SAFE_DELETE ( m_pAreaList [ i ] );

	// free scorch
	SAFE_DELETE(m_pScorchMesh);

	// free collision pool
	SAFE_DELETE(m_pCollisionPool);
	SAFE_DELETE(m_pCollisionDiffuse);
}

void cUniverse::Load ( LPSTR pFilename, int iDivideTextureSize )
{
	// clear vars
	g_iAreaBoxCount = 0;
	g_iAreaBox = 0;

	// load the universe object
	sObject* pObject = NULL;
	if ( !LoadDBO ( pFilename, &pObject ) )
		return;

	// load master mesh list
	m_pMasterMeshList.clear();
	sFrame* pFrame = pObject->pFrame;
	while ( pFrame )
	{
		sMesh* pMesh = pFrame->pMesh;
		if ( pMesh ) m_pMasterMeshList.push_back ( pMesh );
		pFrame->pMesh=NULL; //remove mesh from objects awareness
		pFrame = pFrame->pSibling;
	}

	// free object
	SAFE_DELETE ( pObject );
	
	int iMesh = 0;

	// compute master mesh bounds
	for ( iMesh = 0; iMesh < (int)m_pMasterMeshList.size ( ); iMesh++ )
		CalculateMeshBounds ( m_pMasterMeshList [ iMesh ] );

	// if divide texture flag used, need to seperate lightmaps from process
	LPSTR pLightMapString = "levelbank\\testlevel\\lightmaps";

	// prepare textures for all master meshes (load them)
	// FPSCV104RC8 - scifilevel1 - 14secs (6sec with DDS)
	for ( iMesh = 0; iMesh < (int)m_pMasterMeshList.size ( ); iMesh++ )
		LoadInternalTextures ( m_pMasterMeshList [ iMesh ], "", 2, iDivideTextureSize, pLightMapString );

	// prepare effects for all master meshes (load them)
	for ( iMesh = 0; iMesh < (int)m_pMasterMeshList.size ( ); iMesh++ )
	{
		// get mesh ptr
		sMesh* pMesh = m_pMasterMeshList [ iMesh ];
		if ( pMesh->bUseVertexShader )
		{
			// Search if effect already loaded (else create a new)
			CreateNewOrSharedEffect ( pMesh, false );
		}
	}

	// load DBU storing all areaboxes and non-mesh universe data
	char pDBUFile [ _MAX_PATH ];
	strcpy ( pDBUFile, pFilename );
	pDBUFile [ strlen(pDBUFile)-4 ] = 0;
	strcat ( pDBUFile, ".dbu" );

	// universe created in here
	LoadDBU ( pDBUFile );

	// update vertex and index buffers
	CalculateNodeCollisionBoxes ();
	UploadMeshgroupsToBuffers ();

	// leenote - 310105 - could delete meshdata but USED FOR UNIVERSE RAYCAST
	// possible optimization on memory is to use 'unellipsed' COLLISION-E data

	// universe created
}

//
// TEST STRUCTURE for header!
//
struct sQuadList
{
	int iPolyVertIndexA;
	int iPolyVertIndexB;
	int iDirection;
	int iV[4][3];
	sMesh* pM;
};

void cUniverse::LeesTestFPSCQuadRemover ( void )
{
	// run through ALL meshes and find common quad-cancellations
	// array to store quads collected
	vector < sQuadList > quadList;
	quadList.clear();

	// all meshes
	for ( int iMeshIndex = 0; iMeshIndex < (int)m_pMasterMeshList.size ( ); iMeshIndex++ )
	{
		// mesh ptr
		sMesh* pMesh = m_pMasterMeshList [ iMeshIndex ];

		// vert data only
		ConvertLocalMeshToVertsOnly ( pMesh );

		// get the offset map for the FVF
		sOffsetMap offsetMap;
		GetFVFOffsetMap ( pMesh, &offsetMap );

		// array to store exclusions (polys turned to quads)
		bool* pbExclude = new bool [ (int)pMesh->dwVertexCount ];
		for ( int iCurrentVertex = 0; iCurrentVertex < (int)pMesh->dwVertexCount; iCurrentVertex++ )
			pbExclude [ iCurrentVertex ] = false;

		// go through all of the vertices
		for ( int iCurrentVertex = 0; iCurrentVertex < (int)pMesh->dwVertexCount; iCurrentVertex+=3 )
		{
			// a - for each polygon not excluded
			if ( pbExclude [ iCurrentVertex ]==false )
			{
				// gather vertex position
				D3DXVECTOR3 vecVert0 = *(D3DXVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * (iCurrentVertex+0) ) );
				D3DXVECTOR3 vecVert1 = *(D3DXVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * (iCurrentVertex+1) ) );
				D3DXVECTOR3 vecVert2 = *(D3DXVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * (iCurrentVertex+2) ) );
				D3DXVECTOR3 vecNormal0 = *(D3DXVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * (iCurrentVertex+0) ) );

				// reduce vertex resolution to nearest whole unit
				int iVert[3][3];
				iVert[0][0] = (int)(vecVert0.x+0.25f);
				iVert[0][1] = (int)(vecVert0.y+0.25f);
				iVert[0][2] = (int)(vecVert0.z+0.25f);
				iVert[1][0] = (int)(vecVert1.x+0.25f);
				iVert[1][1] = (int)(vecVert1.y+0.25f);
				iVert[1][2] = (int)(vecVert1.z+0.25f);
				iVert[2][0] = (int)(vecVert2.x+0.25f);
				iVert[2][1] = (int)(vecVert2.y+0.25f);
				iVert[2][2] = (int)(vecVert2.z+0.25f);

				// go through every other polygon
				for ( int iOtherVertex = 0; iOtherVertex < (int)pMesh->dwVertexCount; iOtherVertex+=3 )
				{
					// not the one we are making into a quad..
					if ( iCurrentVertex!=iOtherVertex && pbExclude[iOtherVertex]==false )
					{
						// b - possible poly to make quad..
						D3DXVECTOR3 vecOther0 = *(D3DXVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * (iOtherVertex+0) ) );
						D3DXVECTOR3 vecOther1 = *(D3DXVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * (iOtherVertex+1) ) );
						D3DXVECTOR3 vecOther2 = *(D3DXVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * (iOtherVertex+2) ) );
						D3DXVECTOR3 vecOtherNormal0 = *(D3DXVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * (iOtherVertex+0) ) );

						// reduce vertex resolution to nearest whole unit
						int iOther[3][3];
						iOther[0][0] = (int)(vecOther0.x+0.25f);
						iOther[0][1] = (int)(vecOther0.y+0.25f);
						iOther[0][2] = (int)(vecOther0.z+0.25f);
						iOther[1][0] = (int)(vecOther1.x+0.25f);
						iOther[1][1] = (int)(vecOther1.y+0.25f);
						iOther[1][2] = (int)(vecOther1.z+0.25f);
						iOther[2][0] = (int)(vecOther2.x+0.25f);
						iOther[2][1] = (int)(vecOther2.y+0.25f);
						iOther[2][2] = (int)(vecOther2.z+0.25f);

						// c - match any two verts
						int iCount=0;
						bool bMatch=false;
						bool bMatchV[3];
						bool bMatchOV[3];
						bMatchV[0]=false;
						bMatchV[1]=false;
						bMatchV[2]=false;
						bMatchOV[0]=false;
						bMatchOV[1]=false;
						bMatchOV[2]=false;
						int iThirdVert = -1;
						int iUntouchedOfVertPoly = -1;
						for ( int iOV=0; iOV<3; iOV++ )
						{
							for ( int iV=0; iV<3; iV++ )
							{
								// XYZ match exactly
								if ( iVert[iV][0]==iOther[iOV][0]
								&&	 iVert[iV][1]==iOther[iOV][1]
								&&	 iVert[iV][2]==iOther[iOV][2] )
								{
									bMatchV[iV]=true;
									bMatchOV[iOV]=true;
									iCount++;
								}
							}
						}
						if ( iCount==3 )
						{
							// polygon in exact same position
							// might erase this poly if Z clash an issue (later)
						}
						else
						{
							if ( iCount==2 )
							{
								// two verts match, find the un-used third
								if ( bMatchOV[0]==false ) iThirdVert=0;
								if ( bMatchOV[1]==false ) iThirdVert=1;
								if ( bMatchOV[2]==false ) iThirdVert=2;
								if ( bMatchV[0]==false ) iUntouchedOfVertPoly=0;
								if ( bMatchV[1]==false ) iUntouchedOfVertPoly=1;
								if ( bMatchV[2]==false ) iUntouchedOfVertPoly=2;
								bMatch=true;
							}
						}
						if ( bMatch==true )
						{
							// quick ref third vector position
							bool bFormsAQuad = false;
							int iTheX = iOther[iThirdVert][0];
							int iTheY = iOther[iThirdVert][1];
							int iTheZ = iOther[iThirdVert][2];

							// imply other two verts (forming second poly)
							int iFirst, iSecnd;
							if ( iThirdVert==0 ) { iFirst=1; iSecnd=2; }
							if ( iThirdVert==1 ) { iFirst=0; iSecnd=2; }
							if ( iThirdVert==2 ) { iFirst=0; iSecnd=1; }

							// e - does third vert share a single plane of iVert poly
							if ( iVert[0][0]==iTheX	&& iVert[1][0]==iTheX && iVert[2][0]==iTheX )
							{
								// X PLANE POLY : third vert lies on plane, as does iVert poly
								// a flat pair of polys slicing into the screen distance
								if ((iOther[iThirdVert][1]==iOther[iFirst][1]
								&&   iOther[iThirdVert][2]==iOther[iSecnd][2] )
								||  (iOther[iThirdVert][1]==iOther[iSecnd][1]
								&&   iOther[iThirdVert][2]==iOther[iFirst][2] ) )
								{
									// forms a Z plane quad
									bFormsAQuad=true;
								}
							}
							if ( iVert[0][1]==iTheY	&& iVert[1][1]==iTheY && iVert[2][1]==iTheY )
							{
								// Y PLANE POLY : third vert lies on plane, as does iVert poly
								// a flat pair of polys like a floor
								if ((iOther[iThirdVert][0]==iOther[iFirst][0]
								&&   iOther[iThirdVert][2]==iOther[iSecnd][2] )
								||  (iOther[iThirdVert][0]==iOther[iSecnd][0]
								&&   iOther[iThirdVert][2]==iOther[iFirst][2] ) )
								{
									// forms a Z plane quad
									bFormsAQuad=true;
								}
							}
							if ( iVert[0][2]==iTheZ	&& iVert[1][2]==iTheZ && iVert[2][2]==iTheZ )
							{
								// Z PLANE POLY : third vert lies on plane, as does iVert poly
								// a flat pair of polys like a decal facing the camera
								if ((iOther[iThirdVert][0]==iOther[iFirst][0]
								&&   iOther[iThirdVert][1]==iOther[iSecnd][1] )
								||  (iOther[iThirdVert][0]==iOther[iSecnd][0]
								&&   iOther[iThirdVert][1]==iOther[iFirst][1] ) )
								{
									// forms a Z plane quad
									bFormsAQuad=true;
								}
							}
							if ( bFormsAQuad==true )
							{
								// f - do both polys face same direction
								int iDiffX = (int)( vecNormal0.x - vecOtherNormal0.x );
								int iDiffY = (int)( vecNormal0.y - vecOtherNormal0.y );
								int iDiffZ = (int)( vecNormal0.z - vecOtherNormal0.z );
								if ( iDiffX==0 && iDiffY==0 && iDiffZ==0 )
								{
									// this pair forms a perfect quad facing a clean direction
									sQuadList quad;
									quad.iPolyVertIndexA = iCurrentVertex;
									quad.iPolyVertIndexB = iOtherVertex;
									if ( vecNormal0.x<-0.5f ) quad.iDirection = 0;
									if ( vecNormal0.x> 0.5f ) quad.iDirection = 1;
									if ( vecNormal0.y<-0.5f ) quad.iDirection = 2;
									if ( vecNormal0.y> 0.5f ) quad.iDirection = 3;
									if ( vecNormal0.z<-0.5f ) quad.iDirection = 4;
									if ( vecNormal0.z> 0.5f ) quad.iDirection = 5;

									// to reduce quad count
									bool bIgnoreQuad=false;

									// ignore up and down
									if ( quad.iDirection>=2 && quad.iDirection<=3 )
									{
										// ignore floors and ceilings
										bIgnoreQuad=true;
									}

									// add if not ignored
									if ( bIgnoreQuad==false )
									{
										// store int vert position of quad (for speed later)
										for ( int iI=0; iI<3; iI++ )
										{
											quad.iV[0][iI]=iVert[iUntouchedOfVertPoly][iI];
											quad.iV[1][iI]=iOther[0][iI];
											quad.iV[2][iI]=iOther[1][iI];
											quad.iV[3][iI]=iOther[2][iI];
										}

										// get relative axis sizes of quad
										int iXSize= (quad.iV[0][0]-quad.iV[1][0])+
													(quad.iV[0][0]-quad.iV[2][0])+
													(quad.iV[0][0]-quad.iV[3][0]);
										int iYSize= (quad.iV[0][1]-quad.iV[1][1])+
													(quad.iV[0][1]-quad.iV[2][1])+
													(quad.iV[0][1]-quad.iV[3][1]);
										int iZSize= (quad.iV[0][2]-quad.iV[1][2])+
													(quad.iV[0][2]-quad.iV[2][2])+
													(quad.iV[0][2]-quad.iV[3][2]);

										// ignore large aspect directions too - big flat walls
										if ( quad.iDirection==0 && abs(iYSize)>=abs(iZSize)/2.0f ) bIgnoreQuad=true;
										if ( quad.iDirection==1 && abs(iYSize)>=abs(iZSize)/2.0f ) bIgnoreQuad=true;
										if ( quad.iDirection==4 && abs(iYSize)>=abs(iXSize)/2.0f ) bIgnoreQuad=true;
										if ( quad.iDirection==5 && abs(iYSize)>=abs(iXSize)/2.0f ) bIgnoreQuad=true;

										// add to quad list
										if ( bIgnoreQuad==false )
										{
											quad.pM = pMesh;
											quadList.push_back ( quad );
										}
									}

									// exclude these two polys from future scans
									pbExclude [ iCurrentVertex ] = true;
									pbExclude [ iOtherVertex ] = true;

									// continue with next iCurrentVertex poly
									iOtherVertex=pMesh->dwVertexCount;
									continue;
								}
							}
						}
					}
				}
			}
		}

		// delete temp exclusion array
		SAFE_DELETE ( pbExclude );
	}

	// go through all meshes again (now have complete quad list)
	for ( int iMeshIndex = 0; iMeshIndex < (int)m_pMasterMeshList.size ( ); iMeshIndex++ )
	{
		// mesh ptr
		sMesh* pMesh = m_pMasterMeshList [ iMeshIndex ];

		// get the offset map for the FVF
		sOffsetMap offsetMap;
		GetFVFOffsetMap ( pMesh, &offsetMap );

		// eliminate all quads that are not quad-cancelled (two quads facing each other)
		bool* pbExclude = new bool [ (int)pMesh->dwVertexCount ];
		for ( int iCurrentVertex = 0; iCurrentVertex < (int)pMesh->dwVertexCount; iCurrentVertex++ )
			pbExclude [ iCurrentVertex ] = false;

		// as the remaining list is used to destroy un-wanted polygons from mesh
		DWORD dwReducedMeshCount=0;
		for ( int iQ=0; iQ<(int)quadList.size(); iQ++ )
		{
			// take a quad, ideally to remove it
			sQuadList* pQuadPtr = &(quadList [ iQ ]);

			// work out direction and counter-direction
			int iDirection = pQuadPtr->iDirection;
			int iOppositeDirection;
			switch ( iDirection )
			{
				case 0 : iOppositeDirection=1; break;
				case 1 : iOppositeDirection=0; break;
				case 2 : iOppositeDirection=3; break;
				case 3 : iOppositeDirection=2; break;
				case 4 : iOppositeDirection=5; break;
				case 5 : iOppositeDirection=4; break;
			}

			// scan rest of quads for opposite direction match
			for ( int iOQ=0; iOQ<(int)quadList.size(); iOQ++ )
			{
				// take other quad ptr
				sQuadList* pOtherQuadPtr = &(quadList [ iOQ ]);
				int iOtherDirection = pOtherQuadPtr->iDirection;

				// do quads oppose each other
				if ( iOtherDirection==iOppositeDirection )
				{
					// do quads share exact same space
					int iCount=0;
					for ( int iV=0; iV<4; iV++ )
					{
						for ( int iOV=0; iOV<4; iOV++ )
						{
							// XYZ match exactly
							if ( pQuadPtr->iV[iV][0]==pOtherQuadPtr->iV[iOV][0]
							&&	 pQuadPtr->iV[iV][1]==pOtherQuadPtr->iV[iOV][1]
							&&	 pQuadPtr->iV[iV][2]==pOtherQuadPtr->iV[iOV][2] )
							{
								iCount++;
							}
						}
						if ( iCount<=iV ) break;
					}
					if ( iCount==4 )
					{
						// mark related polygons for removal in next section
						for ( int iErase=0; iErase<4; iErase++ )
						{
							// vertex index
							int iVI=0;
							sMesh* pM=NULL;
							if ( iErase==0 ) { pM=pQuadPtr->pM; iVI=pQuadPtr->iPolyVertIndexA; }
							if ( iErase==1 ) { pM=pQuadPtr->pM; iVI=pQuadPtr->iPolyVertIndexB; }
							if ( iErase==2 ) { pM=pOtherQuadPtr->pM; iVI=pOtherQuadPtr->iPolyVertIndexA; }
							if ( iErase==3 ) { pM=pOtherQuadPtr->pM; iVI=pOtherQuadPtr->iPolyVertIndexB; }

							// reuse exclude array to Remove Polys when re-build
							if ( pM==pMesh )
							{
								// only if currently modifying mesh
								if ( pbExclude [ iVI ]==false )
								{
									pbExclude [ iVI ] = true;
									dwReducedMeshCount += 3;
								}
							}
						}

						// continue with scan
						iOQ = quadList.size();
						continue;
					}
				}
			}
		}

		// make a new mesh deleting all polygons that are specified by remaining quadlist
		DWORD dwNewVertexCount = pMesh->dwVertexCount - dwReducedMeshCount;

		// create new arrays
		BYTE* pNewVertexData = new BYTE [ dwNewVertexCount * pMesh->dwFVFSize ];

		// copy mesh data ignoring erased quads
		int iNewVertexIndex = 0;
		for ( int iCurrentVertex = 0; iCurrentVertex < (int)pMesh->dwVertexCount; iCurrentVertex+=3 )
		{
			// only add non-excluded polys
			if ( pbExclude [ iCurrentVertex ]==false )
			{
				// copy polygon from old to new (3 verts 
				BYTE* pDst = (BYTE*) ( ( float* ) pNewVertexData + offsetMap.dwX + ( offsetMap.dwSize * (iNewVertexIndex+0) ));
				BYTE* pSrc = (BYTE*) ( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * (iCurrentVertex+0) ));
				memcpy ( pDst, pSrc, pMesh->dwFVFSize * 3 );
				iNewVertexIndex+=3;
			}
		}

		// remove old arraus
		SAFE_DELETE_ARRAY(pMesh->pVertexData);

		// replace with new arrays
		pMesh->dwVertexCount = dwNewVertexCount;
		pMesh->pVertexData = pNewVertexData;
		pMesh->iDrawVertexCount = dwNewVertexCount;
		pMesh->iDrawPrimitives = dwNewVertexCount/3;

		// flag mesh for a VB replacement
		pMesh->bMeshHasBeenReplaced=true;

		// delete temp exclusion array
		SAFE_DELETE ( pbExclude );
	}
}

void cUniverse::Save ( LPSTR pFilename )
{
	// collapse universe into final universe data (areabox meshgroups)
	BuildAreaBoxMeshGroups ( );

	// master mesh list (and where they came from)
	m_pMasterMeshList.clear();
	
	// all meshgroups in universe
	for ( int iAreaBox = 0; iAreaBox < (int)m_pAreaList.size ( ); iAreaBox++ )
	{
	    int iIndex = 0;
	    
		sArea* pArea = m_pAreaList [ iAreaBox ];
		for ( iIndex = 0; iIndex < (int)pArea->meshgroups.size ( ); iIndex++ )
		{
			sMesh* pMesh = pArea->meshgroups [ iIndex ]->pMesh;
			m_pMasterMeshList.push_back ( pMesh );
		}
		for ( iIndex = 0; iIndex < (int)pArea->sharedmeshgroups.size ( ); iIndex++ )
		{
			sMesh* pMesh = pArea->sharedmeshgroups [ iIndex ]->pMesh;
			m_pMasterMeshList.push_back ( pMesh );
		}
	}

	// all meshgroup references in universe
	/*
	for ( iAreaBox = 0; iAreaBox < m_pAreaList.size ( ); iAreaBox++ )
	{
		sArea* pArea = m_pAreaList [ iAreaBox ];
		for ( int iRef = 0; iRef < pArea->meshgroupref.size ( ); iRef++ )
		{
			sMeshGroup* pMeshGroup = pArea->meshgroupref [ iRef ];
			if ( pMeshGroup )
			{
				sMesh* pMesh = pMeshGroup->pMesh;
				for ( int iMasterMeshListIndex = 0; iMasterMeshListIndex < m_pMasterMeshList.size ( ); iMasterMeshListIndex++ )
				{
					// get current area box ptr
					sMesh* pListMesh = m_pMasterMeshList [ iMasterMeshListIndex ];
					if ( pMesh==pListMesh )
					{
						// does nothing this one!
					}
				}
			}
		}
	}
	*/

	// create an object to store entire mastermeshlist
	char pDBOFile[256];
	strcpy ( pDBOFile, pFilename );
	sObject* pObject = new sObject;

	// link meshes to object for saving
	sFrame* pFrame = NULL;
	for ( int iMasterMeshListIndex = 0; iMasterMeshListIndex < (int)m_pMasterMeshList.size ( ); iMasterMeshListIndex++ )
	{
		if ( pObject->pFrame==NULL )
		{
			// new
			pFrame = new sFrame ;
			pObject->pFrame = pFrame;
		}
		else
		{
			// add mesh
			pFrame->pSibling = new sFrame;
			pFrame = pFrame->pSibling;
		}

		// apply mesh to frame
		pFrame->pMesh = m_pMasterMeshList [ iMasterMeshListIndex ];
	}

	// leeadd - 230503 - modify object to remove unseen quads
//	LeesTestFPSCQuadRemover();
	UploadMeshgroupsToBuffers ();

	// leenote - 110805 - do not attempt to smooth - messes normals (for decal splats)

	// save DBO storing all meshes ( textures and effects )
	SaveDBO ( pDBOFile, pObject );

	// compute master mesh bounds
	for ( int iMesh = 0; iMesh < (int)m_pMasterMeshList.size ( ); iMesh++ )
		CalculateMeshBounds ( m_pMasterMeshList [ iMesh ] );

	// save DBU storing all areaboxes and non-mesh universe data
	char pDBUFile [ _MAX_PATH ];
	strcpy ( pDBUFile, pDBOFile );
	pDBUFile [ strlen(pDBUFile)-4 ] = 0;
	strcat ( pDBUFile, ".dbu" );
	SaveDBU ( pDBUFile );

	// detatch meshes from object before releasing object 
	pFrame = pObject->pFrame;
	while ( pFrame )
	{
		sFrame* pThisFrame = pFrame;
		pFrame->pMesh = NULL;
		pFrame = pFrame->pSibling;
		pThisFrame->pSibling = NULL;
		SAFE_DELETE(pThisFrame);
	}
	pObject->pFrame=NULL;

	// free usages
	SAFE_DELETE ( pObject );
}

void cUniverse::SetEffectTechnique ( LPSTR pTechniqueName )
{
	// go through all meshes in universe
	for ( int iMesh = 0; iMesh < (int)m_pMasterMeshList.size ( ); iMesh++ )
	{
		// get mesh ptr
		sMesh* pMesh = m_pMasterMeshList [ iMesh ];
		if ( pMesh->bUseVertexShader )
		{
			// Search for effect
			int iEffectIDFound = 0;
			for ( int iEffectID=0; iEffectID<MAX_EFFECTS; iEffectID++ )
			{
				if ( m_EffectList [ iEffectID ] )
				{
					if ( _stricmp ( m_EffectList [ iEffectID ]->pEffectObj->m_pEffectName, (LPSTR)pMesh->pEffectName )==NULL )
					{
						iEffectIDFound=iEffectID;
						break;
					}
				}
			}
			if ( iEffectIDFound>0 )
			{
				sEffectItem* pEffectItem = m_EffectList [ iEffectIDFound ];
				LPD3DXEFFECT pEffectPtr = pEffectItem->pEffectObj->m_pEffect;
				if ( pEffectPtr )
				{
					D3DXHANDLE hTechnique = pEffectPtr->GetTechniqueByName ( (LPSTR)pTechniqueName );
					if ( hTechnique )
						pEffectPtr->SetTechnique(hTechnique);
				}
			}
		}
	}
}

DWORD cUniverse::GetMasterMeshIndex ( sMesh* pFindMesh )
{
	for ( DWORD dwMasterMeshListIndex = 0; dwMasterMeshListIndex < m_pMasterMeshList.size ( ); dwMasterMeshListIndex++ )
	{
		// get current area box ptr
		sMesh* pListMesh = m_pMasterMeshList [ dwMasterMeshListIndex ];
		if ( pListMesh==pFindMesh ) return dwMasterMeshListIndex;
	}
	return 0;
}

DWORD cUniverse::GetNodeIndex ( sNode* pFindNode )
{
	for ( DWORD dwMasterNodeListIndex = 0; (int)dwMasterNodeListIndex < m_iNodeListSize; dwMasterNodeListIndex++ )
	{
		sNode* pListNode = &m_pNode [ dwMasterNodeListIndex ];
		if ( pListNode==pFindNode ) return dwMasterNodeListIndex;
	}
	return 0;
}

void cUniverse::LoadDBU ( LPSTR pDBUFilename )
{
	DWORD read;
	HANDLE hreadfile = CreateFile(pDBUFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hreadfile!=INVALID_HANDLE_VALUE)
	{
		// GENERAL SETTINGS
		DWORD dwUniverseX, dwUniverseY, dwUniverseZ;
		ReadFile(hreadfile, &dwUniverseX, 4, &read, NULL); 
		ReadFile(hreadfile, &dwUniverseY, 4, &read, NULL); 
		ReadFile(hreadfile, &dwUniverseZ, 4, &read, NULL);
		float fX = (float)dwUniverseX;
		float fY = (float)dwUniverseY;
		float fZ = (float)dwUniverseZ;
		g_pUniverse->Make ( fX, fY, fZ );

		// AREA BOXES
		DWORD dwAreaBoxMax = 0;
		ReadFile(hreadfile, &dwAreaBoxMax, 4, &read, NULL); 
		g_iAreaBoxCount = (int)dwAreaBoxMax;
		for ( int iAreaBox = 0; iAreaBox < (int)dwAreaBoxMax; iAreaBox++ )
		{
			// Get Area Box Ptr
			sArea* pArea = new sArea;
            m_pAreaList.push_back ( pArea );

			// Area Box Dimensions
			ReadFile(hreadfile, (LPSTR)&pArea->vecMin, sizeof(D3DXVECTOR3), &read, NULL); 
			ReadFile(hreadfile, (LPSTR)&pArea->vecMax, sizeof(D3DXVECTOR3), &read, NULL); 
			ReadFile(hreadfile, (LPSTR)&pArea->vecCentre, sizeof(D3DXVECTOR3), &read, NULL); 

			// Area Box Geometry (indexes into master mesh)
			DWORD dwIndex = 0;
			DWORD dwWithinMax = 0;
			ReadFile(hreadfile, &dwWithinMax, 4, &read, NULL); 
			for ( int iWithinIndex = 0; iWithinIndex < (int)dwWithinMax; iWithinIndex++ )
			{
				ReadFile(hreadfile, &dwIndex, 4, &read, NULL); 
				sMesh* pMesh = m_pMasterMeshList [ dwIndex ];
				sMeshGroup* pMeshGroup = new sMeshGroup;
				pMeshGroup->pMesh = pMesh;
				pArea->meshgroups.push_back ( pMeshGroup );
			}
			DWORD dwShared = 0;
			ReadFile(hreadfile, &dwShared, 4, &read, NULL); 
			for ( int iSharedIndex = 0; iSharedIndex < (int)dwShared; iSharedIndex++ )
			{
				ReadFile(hreadfile, &dwIndex, 4, &read, NULL); 
				sMesh* pMesh = m_pMasterMeshList [ dwIndex ];
				sMeshGroup* pMeshGroup = new sMeshGroup;
				pMeshGroup->pMesh = pMesh;
				pArea->sharedmeshgroups.push_back ( pMeshGroup );
			}
			DWORD dwRef = 0;
			ReadFile(hreadfile, &dwRef, 4, &read, NULL); 
			for ( int iRefIndex = 0; iRefIndex < (int)dwRef; iRefIndex++ )
			{
				ReadFile(hreadfile, &dwIndex, 4, &read, NULL); 
				sMesh* pMesh = m_pMasterMeshList [ dwIndex ];
				sMeshGroup* pMeshGroup = new sMeshGroup;
				pMeshGroup->pMesh = pMesh;
				pArea->meshgroupref.push_back ( pMeshGroup );
			}

			// number of links in areabox
			DWORD dwLinkMax = 0;
			ReadFile(hreadfile, &dwLinkMax, 4, &read, NULL); 
			pArea->iLinkMax = (int)dwLinkMax;
			for ( int dwLinkIndex = 0; dwLinkIndex < (int)dwLinkMax; dwLinkIndex++ )
			{
				// Get Link Ptr
				sAreaLink* pAreaLink = new sAreaLink;
				memset ( pAreaLink, 0, sizeof ( sAreaLink ) );
				pArea->pLink.push_back ( pAreaLink );

				// Link Details
				ReadFile(hreadfile, (LPSTR)&pAreaLink->iLinkedTo, sizeof(int), &read, NULL); 
				ReadFile(hreadfile, (LPSTR)&pAreaLink->iTouchingSide, sizeof(int), &read, NULL); 
				ReadFile(hreadfile, (LPSTR)&pAreaLink->iSolidity, sizeof(int), &read, NULL); 

				// Link Vectors
				ReadFile(hreadfile, (LPSTR)&pAreaLink->vecPortalMin, sizeof(D3DXVECTOR3), &read, NULL); 
				ReadFile(hreadfile, (LPSTR)&pAreaLink->vecPortalMax, sizeof(D3DXVECTOR3), &read, NULL); 
				ReadFile(hreadfile, (LPSTR)&pAreaLink->vecPortalCenter, sizeof(D3DXVECTOR3), &read, NULL); 
				ReadFile(hreadfile, (LPSTR)&pAreaLink->vecPortalDim, sizeof(D3DXVECTOR3), &read, NULL); 
				ReadFile(hreadfile, (LPSTR)&pAreaLink->vecA, sizeof(D3DXVECTOR3), &read, NULL); 
				ReadFile(hreadfile, (LPSTR)&pAreaLink->vecB, sizeof(D3DXVECTOR3), &read, NULL); 
				ReadFile(hreadfile, (LPSTR)&pAreaLink->vecC, sizeof(D3DXVECTOR3), &read, NULL); 
				ReadFile(hreadfile, (LPSTR)&pAreaLink->vecD, sizeof(D3DXVECTOR3), &read, NULL); 
			}
		}

		// NODE DATA (ultimately move bulk of node fields to 'generate area'
		DWORD dwNodeListSize = 0;
		ReadFile(hreadfile, &dwNodeListSize, 4, &read, NULL); 
		m_iNodeListSize = (int)dwNodeListSize;
		m_pNode = new sNode [ m_iNodeListSize + 1 ];
		for ( int iNode = 0; iNode < (int)dwNodeListSize; iNode++ )
		{
			// get node ptr
			sNode* pNode = &m_pNode [ iNode ];

			// node dimensions
			ReadFile(hreadfile, (LPSTR)&pNode->vecCentre, sizeof(D3DXVECTOR3), &read, NULL); 
			ReadFile(hreadfile, (LPSTR)&pNode->vecDimension, sizeof(D3DXVECTOR3), &read, NULL); 

			// node neighbors
			for ( int n=0; n<6; n++ )
			{
				DWORD dwIndex = 0;
				ReadFile(hreadfile, (LPSTR)&dwIndex, sizeof(DWORD), &read, NULL); 
				pNode->pNeighbours[n] = &m_pNode [ dwIndex ];
			}

			// node collision data
			DWORD dwColESize = 0;
			ReadFile(hreadfile, &dwColESize, 4, &read, NULL); 
			sCollisionPolygon* pColData = new sCollisionPolygon [ dwColESize ];
			ReadFile(hreadfile, pColData, sizeof(sCollisionPolygon)*dwColESize, &read, NULL); 
			pNode->collisionE.clear();
			for ( int iCol = 0; iCol < (int)dwColESize; iCol++ )
				pNode->collisionE.push_back ( pColData [ iCol ] );
			SAFE_DELETE(pColData);
		}

		// close DBU file
		CloseHandle(hreadfile);
	}
}

void cUniverse::SaveDBU ( LPSTR pDBUFilename )
{
	// open DBU file to deposit universe (V1.0 DBU format hard coded for FPSC-dev-speed)
	DWORD written;
	LPSTR pData = NULL;
	DWORD dwDataSize = 0;
	HANDLE hwritefile = CreateFile(pDBUFilename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hwritefile!=INVALID_HANDLE_VALUE)
	{
		// GENERAL SETTINGS
		DWORD dwUniverseX = m_pNodeList->vecDimension.x;
		DWORD dwUniverseY = m_pNodeList->vecDimension.y;
		DWORD dwUniverseZ = m_pNodeList->vecDimension.z;
		WriteFile(hwritefile, &dwUniverseX, 4, &written, NULL); 
		WriteFile(hwritefile, &dwUniverseY, 4, &written, NULL); 
		WriteFile(hwritefile, &dwUniverseZ, 4, &written, NULL); 

		// AREA BOXES
		DWORD dwAreaBoxMax = m_pAreaList.size ( );
		WriteFile(hwritefile, &dwAreaBoxMax, 4, &written, NULL); 
		for ( int iAreaBox = 0; iAreaBox < (int)dwAreaBoxMax; iAreaBox++ )
		{
			// Get Area Box Ptr
			sArea* pArea = m_pAreaList [ iAreaBox ];

			// Area Box Dimensions
			WriteFile(hwritefile, (LPSTR)&pArea->vecMin, sizeof(D3DXVECTOR3), &written, NULL); 
			WriteFile(hwritefile, (LPSTR)&pArea->vecMax, sizeof(D3DXVECTOR3), &written, NULL); 
			WriteFile(hwritefile, (LPSTR)&pArea->vecCentre, sizeof(D3DXVECTOR3), &written, NULL); 

			// Area Box Geometry (indexes into master mesh)
			DWORD dwWithinMax = pArea->meshgroups.size ( );
			WriteFile(hwritefile, &dwWithinMax, 4, &written, NULL); 
			for ( int iWithinIndex = 0; iWithinIndex < (int)dwWithinMax; iWithinIndex++ )
			{
				DWORD dwIndex = GetMasterMeshIndex ( pArea->meshgroups [ iWithinIndex ]->pMesh );
				WriteFile(hwritefile, &dwIndex, 4, &written, NULL); 
			}
			DWORD dwShared = pArea->sharedmeshgroups.size ( );
			WriteFile(hwritefile, &dwShared, 4, &written, NULL); 
			for ( int iSharedIndex = 0; iSharedIndex < (int)dwShared; iSharedIndex++ )
			{
				DWORD dwIndex = GetMasterMeshIndex ( pArea->sharedmeshgroups [ iSharedIndex ]->pMesh );
				WriteFile(hwritefile, &dwIndex, 4, &written, NULL); 
			}
			DWORD dwRef = pArea->meshgroupref.size ( );
			WriteFile(hwritefile, &dwRef, 4, &written, NULL); 
			for ( int iRefIndex = 0; iRefIndex < (int)dwRef; iRefIndex++ )
			{
				DWORD dwIndex = 0;
				sMeshGroup* pMeshGroup = pArea->meshgroupref [ iRefIndex ];
				if ( pMeshGroup )
				{
					sMesh* pMesh = pMeshGroup->pMesh;
					dwIndex = GetMasterMeshIndex ( pMesh );
				}
				WriteFile(hwritefile, &dwIndex, 4, &written, NULL); 
			}

			// number of links in areabox
			DWORD dwLinkMax = pArea->iLinkMax;
			WriteFile(hwritefile, &dwLinkMax, 4, &written, NULL); 
			for ( int dwLinkIndex = 0; dwLinkIndex < (int)dwLinkMax; dwLinkIndex++ )
			{
				// Get Link Ptr
				sAreaLink* pAreaLink = pArea->pLink [ dwLinkIndex ];

				// Link Details
				WriteFile(hwritefile, (LPSTR)&pAreaLink->iLinkedTo, sizeof(int), &written, NULL); 
				WriteFile(hwritefile, (LPSTR)&pAreaLink->iTouchingSide, sizeof(int), &written, NULL); 
				WriteFile(hwritefile, (LPSTR)&pAreaLink->iSolidity, sizeof(int), &written, NULL); 

				// Link Vectors
				WriteFile(hwritefile, (LPSTR)&pAreaLink->vecPortalMin, sizeof(D3DXVECTOR3), &written, NULL); 
				WriteFile(hwritefile, (LPSTR)&pAreaLink->vecPortalMax, sizeof(D3DXVECTOR3), &written, NULL); 
				WriteFile(hwritefile, (LPSTR)&pAreaLink->vecPortalCenter, sizeof(D3DXVECTOR3), &written, NULL); 
				WriteFile(hwritefile, (LPSTR)&pAreaLink->vecPortalDim, sizeof(D3DXVECTOR3), &written, NULL); 
				WriteFile(hwritefile, (LPSTR)&pAreaLink->vecA, sizeof(D3DXVECTOR3), &written, NULL); 
				WriteFile(hwritefile, (LPSTR)&pAreaLink->vecB, sizeof(D3DXVECTOR3), &written, NULL); 
				WriteFile(hwritefile, (LPSTR)&pAreaLink->vecC, sizeof(D3DXVECTOR3), &written, NULL); 
				WriteFile(hwritefile, (LPSTR)&pAreaLink->vecD, sizeof(D3DXVECTOR3), &written, NULL); 
			}
		}

		// NODE DATA (ultimately move bulk of node fields to 'generate area'
		DWORD dwNodeListSize = m_iNodeListSize;
		WriteFile(hwritefile, &dwNodeListSize, 4, &written, NULL); 
		for ( int iNode = 0; iNode < (int)dwNodeListSize; iNode++ )
		{
			// get node ptr
			sNode* pNode = &m_pNode [ iNode ];

			// node dimensions
			WriteFile(hwritefile, (LPSTR)&pNode->vecCentre, sizeof(D3DXVECTOR3), &written, NULL); 
			WriteFile(hwritefile, (LPSTR)&pNode->vecDimension, sizeof(D3DXVECTOR3), &written, NULL); 

			// node neighbors
			for ( int n=0; n<6; n++ )
			{
				DWORD dwIndex = GetNodeIndex ( pNode->pNeighbours [ n ] );
				WriteFile(hwritefile, (LPSTR)&dwIndex, sizeof(DWORD), &written, NULL); 
			}

			// node collision data
			DWORD dwColESize = pNode->collisionE.size ( );
			WriteFile(hwritefile, &dwColESize, 4, &written, NULL); 
			sCollisionPolygon* pColData = new sCollisionPolygon[dwColESize];
			for ( int iCol = 0; iCol < (int)dwColESize; iCol++ )
				pColData [ iCol ] = pNode->collisionE [ iCol ];
			WriteFile(hwritefile, pColData, sizeof(sCollisionPolygon)*dwColESize, &written, NULL); 
			SAFE_DELETE(pColData);
		}
			
		// close DBU file
		CloseHandle(hwritefile);
	}
}

void cUniverse::GetMeshList ( vector < sMesh* > *pMeshList )
{
	// get the mesh list

	// check the mesh list is valid
	if ( !pMeshList )
		return;

	// go through all nodes
	for ( int iObject = 0; iObject < m_iNodeListSize; iObject++ )
	{
		// find all meshes
		for ( int iMesh = 0; iMesh < m_pNode [ iObject ].iMeshCount; iMesh++ )
		{
			// get pointer to mesh
			sMesh* pMesh = m_pNode [ iObject ].ppMeshList [ iMesh ];

			// send mesh to back of list
			pMeshList->push_back ( pMesh );
		}	
	}
}

bool cUniverse::Make ( float fX, float fY, float fZ )
{
	// make the first node of the universe, all other nodes
	// must be contained within the boundaries given

	// see if the universe has already been created, if this
	// is the case then we must return as failure
	if ( m_bCreated )
		return false;

	// create the first node
	m_pNodeList = new sNode;

	// check the newly allocated memory
	SAFE_MEMORY ( m_pNodeList );

	// store node properties
	m_pNodeList->vecCentre    = D3DXVECTOR3 ( 0.0f, 0.0f, 0.0f );	// centre point
	m_pNodeList->vecDimension = D3DXVECTOR3 (  fX,    fY,   fZ );	// dimensions given from parameters

	// set created flag to true
	m_bCreated = true;

	// everything went okay
	return true;
}

bool cUniverse::Attach ( sObject* pObject )
{
	// add object to vislinked list
	m_VisLinkedObjectList.push_back ( pObject );
	m_VisLinkedObjectInNodeList.push_back ( NULL );

	// complete
	return true;
}

bool cUniverse::Detach ( sObject* pObject )
{
	// find iterator and delete from both lists
	int iVisLinkedObjMax = m_VisLinkedObjectList.size ( );
	for ( int iObj = 0; iObj < iVisLinkedObjMax; iObj++ )
	{
		// object ptr
		sObject* pThisObject = m_VisLinkedObjectList [ iObj ];
		if ( pThisObject == pObject )
		{
			m_VisLinkedObjectList.erase ( m_VisLinkedObjectList.begin() + iObj );
			m_VisLinkedObjectInNodeList.erase ( m_VisLinkedObjectInNodeList.begin() + iObj );
			break;
		}
	}

	// also set universe visible back to absolutely visible
	pObject->bUniverseVisible = true;

	// complete
	return true;
}

#ifdef __GNUC__
    #define _inline 
#endif

_inline D3DXVECTOR3 cUniverse::GetTopLeftFront ( sNode* pNode )
{
	// get top left front point of node
	return D3DXVECTOR3 ( pNode->vecCentre.x - pNode->vecDimension.x, pNode->vecCentre.y + pNode->vecDimension.y, pNode->vecCentre.z + pNode->vecDimension.z );
}

_inline D3DXVECTOR3 cUniverse::GetTopLeftBack ( sNode* pNode )
{
	// get top left back point of node
	return D3DXVECTOR3 ( pNode->vecCentre.x - pNode->vecDimension.x, pNode->vecCentre.y + pNode->vecDimension.y, pNode->vecCentre.z - pNode->vecDimension.z );
}

_inline D3DXVECTOR3 cUniverse::GetTopRightFront ( sNode* pNode )
{
	// get top right front point of node
	return D3DXVECTOR3 ( pNode->vecCentre.x + pNode->vecDimension.x, pNode->vecCentre.y + pNode->vecDimension.y, pNode->vecCentre.z + pNode->vecDimension.z );
}

_inline D3DXVECTOR3 cUniverse::GetTopRightBack ( sNode* pNode )
{
	// get top right back point of node
	return D3DXVECTOR3 ( pNode->vecCentre.x + pNode->vecDimension.x, pNode->vecCentre.y + pNode->vecDimension.y, pNode->vecCentre.z - pNode->vecDimension.z );
}

_inline D3DXVECTOR3 cUniverse::GetBottomLeftFront ( sNode* pNode )
{
	// get bottom left front point of node
	return D3DXVECTOR3 ( pNode->vecCentre.x - pNode->vecDimension.x, pNode->vecCentre.y - pNode->vecDimension.y, pNode->vecCentre.z + pNode->vecDimension.z );
}

_inline D3DXVECTOR3 cUniverse::GetBottomLeftBack ( sNode* pNode )
{
	// get bottom left back point of node
	return D3DXVECTOR3 ( pNode->vecCentre.x - pNode->vecDimension.x, pNode->vecCentre.y - pNode->vecDimension.y, pNode->vecCentre.z - pNode->vecDimension.z );
}

_inline D3DXVECTOR3 cUniverse::GetBottomRightBack ( sNode* pNode )
{
	// get bottom right back point of node
	return D3DXVECTOR3 ( pNode->vecCentre.x + pNode->vecDimension.x, pNode->vecCentre.y - pNode->vecDimension.y, pNode->vecCentre.z - pNode->vecDimension.z );
}

_inline D3DXVECTOR3 cUniverse::GetBottomRightFront ( sNode* pNode )
{
	// get bottom right front of node
	return D3DXVECTOR3 ( pNode->vecCentre.x + pNode->vecDimension.x, pNode->vecCentre.y - pNode->vecDimension.y, pNode->vecCentre.z + pNode->vecDimension.z );
}

bool cUniverse::Add ( int iID, int iLimb, int iType, int iArbitaryValue, int iCastShadow, int iPortalBlocker )
{
	// add a limb from an object into the universe

	// make sure tree is created
	if ( !m_bCreated )
		return false;

	// check object is valid
	sObject* pObject = g_ObjectList [ iID ];
	if ( !pObject )
		return false;

	// actual object or instance of object
	sObject* pActualObject = pObject;
	if ( pObject->pInstanceOfObject )
		pActualObject = pObject->pInstanceOfObject;
	
	// clear the object list
	m_StaticObjectList.clear ( );

	// does the limb exist
	if ( pActualObject->ppFrameList [ iLimb ] )
	{
		// now see if the mesh exists
		sMesh* pMesh = pActualObject->ppFrameList [ iLimb ]->pMesh;
		if ( pMesh )
		{
			// ignore invisible meshes
			if ( pObject->pInstanceOfObject )
			{
				if ( !pObject->pInstanceMeshVisible [ iLimb ] )
					return false;
			}
			else
			{
				if ( !pMesh->bVisible )
					return false;
			}

			// ensure vertex count is valid
			if ( !pMesh->dwVertexCount )
				return false;

			// create a new object to add into list
			sStaticObject object;

			// set up object properties
			object.pFrame     = pActualObject->ppFrameList [ iLimb ];	// store frame
			object.pObject    = pObject;						// store pointer to object
			object.iID        = iID;				// id of object
			object.iCollision = iType; // collision type (0-poly/1-box/2-reduced)
			object.iCastShadow = iCastShadow;

			// set arbitary value
			pMesh->Collision.dwArbitaryValue = (DWORD)iArbitaryValue;
			pMesh->Collision.dwPortalBlocker = (DWORD)iPortalBlocker;
			
			// add this limb to the frame list
			m_StaticObjectList.push_back ( object );

			// build the tree
			if ( Build ( ) )
				return true;
		}
	}
	
	// failed
	return false;
}

bool cUniverse::Add ( int iID, int iType, int iArbitaryValue, int iCastShadow, int iPortalBlocker )
{
	// add an object into the universe

	// make sure tree is created
	if ( !m_bCreated )
		return false;

	// get the object and first mesh
	sObject* pObject = g_ObjectList [ iID ];
	if ( !pObject )
		return false;

	// actual object or instance of object
	sObject* pActualObject = pObject;
	if ( pObject->pInstanceOfObject )
		pActualObject = pObject->pInstanceOfObject;
	
	// clear the object list
	m_StaticObjectList.clear ( );

	// add each frame which contains a mesh into the frame list
	for ( int iFrame = 0; iFrame < pObject->iFrameCount; iFrame++ )
	{
		// ignore if we don't have a mesh
		sMesh* pMesh = pActualObject->ppFrameList [ iFrame ]->pMesh;
		if ( pMesh )
		{
			// ignore invisible meshes
			if ( pObject->pInstanceOfObject )
			{
				if ( !pObject->pInstanceMeshVisible [ iFrame ] )
					continue;
			}
			else
			{
				if ( !pMesh->bVisible )
					continue;
			}

			// ensure vertex count is valid
			if ( !pMesh->dwVertexCount )
				continue;

			// our static object
			sStaticObject object;

			// set up object properties
			object.pFrame     = pActualObject->ppFrameList [ iFrame ];	// frame pointer
			object.pObject    = pObject;							// store object pointer
			object.iID        = iID;								// id of object
			object.iCollision = iType;								// collision type (0-poly/1-box/2-reduced)
			object.iCastShadow = iCastShadow;

			// set arbitary value
			pMesh->Collision.dwArbitaryValue = (DWORD)iArbitaryValue;
			pMesh->Collision.dwPortalBlocker = (DWORD)iPortalBlocker;

			// send object to back of list
			m_StaticObjectList.push_back ( object );
		}
	}

	// build the tree
	if ( Build ( ) )
		return true;

	// something went wrong
	return false;
}

bool cUniverse::Build ( void )
{
	// build the universe

	// make sure the initial node has been created
	if ( !m_bCreated )
		return false;

	// go through all objects in the list
	for ( int iFrame = 0; iFrame < (int)m_StaticObjectList.size ( ); iFrame++ )
	{
		// get a pointer to the mesh
		sMesh* pMesh = m_StaticObjectList [ iFrame ].pFrame->pMesh;
		int iCastShadow = m_StaticObjectList [ iFrame ].iCastShadow;

		// turn the object's static flag on so it will not be drawn
		// in the main loop, we are now responsible for drawing
		m_StaticObjectList [ iFrame ].pObject->bStatic = true;

		// copy the mesh as we don't want to modify the original mesh
		sMesh* pNewMesh = new sMesh;

		// check the mesh was allocated
		SAFE_MEMORY ( pNewMesh );

		// make a new mesh from the original mesh
		MakeMeshFromOtherMesh       ( true, pNewMesh, pMesh, NULL );
		CopyMeshPropertiesToNewMesh ( pMesh, pNewMesh );

		// store the ID of the object in the mesh
		pNewMesh->dwMeshID = m_StaticObjectList [ iFrame ].iID;

		// get the objects world matrix
		CalcObjectWorld ( m_StaticObjectList [ iFrame ].pObject );

		// generate reduced mesh (for collision or reduced shadow casting)
		sMesh* pReducedMesh = NULL;
		if ( m_StaticObjectList [ iFrame ].iCollision==eReducedPolygon || iCastShadow==1 )
		{
			pReducedMesh = new sMesh;
			MakeMeshFromOtherMesh ( true, pReducedMesh, pNewMesh, NULL );
			ReduceMeshPolygons ( pReducedMesh, 0, 0, 20, 1, 20 );
		}
		else
		{
			// leeadd - 240105 - make eBox mesh here (so can handle rotated at 45 degrees instead of final 90-degree box after transform)
			if ( m_StaticObjectList [ iFrame ].iCollision==eBox )
			{
				// box from original mesh - before transform
				pReducedMesh = new sMesh;
				MakeMeshBox ( 	true,
								pReducedMesh,
								pMesh->Collision.vecMin.x * 1.0f,
								pMesh->Collision.vecMin.y * 1.0f,
								pMesh->Collision.vecMin.z * 1.0f,
								pMesh->Collision.vecMax.x * 1.0f,
								pMesh->Collision.vecMax.y * 1.0f,
								pMesh->Collision.vecMax.z * 1.0f,
								D3DFVF_XYZ,	0 );
			}
		}

		// transform the vertices
		if ( m_StaticObjectList [ iFrame ].pFrame->pMesh->bVertexTransform == false )
		{
			// get the absolute world matrix
			CalculateAbsoluteWorldMatrix ( 
											m_StaticObjectList [ iFrame ].pObject,
											m_StaticObjectList [ iFrame ].pFrame,
											m_StaticObjectList [ iFrame ].pFrame->pMesh
										 );

			// transform vertices
			TransformVertices ( pNewMesh, m_StaticObjectList [ iFrame ].pFrame->matAbsoluteWorld );

			// optionally transform reduced mesh 
			if ( pReducedMesh )
			{
				TransformVertices ( pReducedMesh, m_StaticObjectList [ iFrame ].pFrame->matAbsoluteWorld );
				CalculateMeshBounds ( pReducedMesh );
			}

			// keep a reminder that vertices have been transformed
			pNewMesh->bVertexTransform = true;
		}

		// get the mesh bounds
		CalculateMeshBounds ( pNewMesh );

		// now see if the object is outside the zone
		if ( !IsObjectWithinZone ( m_pNodeList, pNewMesh ) )
		{
			SAFE_DELETE ( pReducedMesh );
			SAFE_DELETE ( pNewMesh );
			return false;
		}

		// store castshadow value in new mesh
		pNewMesh->iCastShadowIfStatic = iCastShadow;

		// set shadow if flagged
		if ( iCastShadow>0 )
		{
			// use reduced mesh for shadow, no matrix (already transformed to world)
			if ( iCastShadow==1 ) SetShadow ( pReducedMesh, NULL );
			if ( iCastShadow==2 ) SetShadow ( pNewMesh, NULL );
		}

		// add the new mesh into the buffers
		if ( m_ObjectManager.AddObjectMeshToBuffers ( pNewMesh, false ) )
		{
			// send this mesh to the back of the mesh list
			m_pMeshList.push_back ( pNewMesh );

			// store the collision info for later
			// eCollision ePolygon-0 eBox-1 eReducedPolygon-2 eNone-3
			m_iMeshCollisionList.push_back ( m_StaticObjectList [ iFrame ].iCollision );
			m_pColMeshList.push_back ( pReducedMesh );
		}
		else
		{
			// failed to add to vertex buffers on card, delete mesh
			SAFE_DELETE ( pReducedMesh );
			SAFE_DELETE ( pNewMesh );
		}
	}

	// all done
	return true;
}

void cUniverse::AddMeshToNode ( sNode* pNode, sMesh* pMesh )
{
	// adds a mesh into the mesh list of a node

	// check parameters
	if ( !pNode || !pMesh )
		return;

	// if this is the first mesh then allocate the list
	if ( pNode->iMeshCount == 0 )
	{
		// create one mesh for now
		SAFE_DELETE_ARRAY ( pNode->ppMeshList );

		// allocate a new array item
		pNode->ppMeshList = new sMesh* [ 1 ];
		pNode->iMeshCount = 1;

		// check memory was allocated
		if ( !pNode->ppMeshList )
			return;

		// store pointer to the mesh
		pNode->ppMeshList [ 0 ] = pMesh;

		// increment polygon count
		pNode->iPolygonCount += pMesh->dwVertexCount;

		// finished
		return;
	}
	
	int iMesh = 0;

	// check for a match, if the mesh is already in the list
	// then do not add it
	for ( iMesh = 0; iMesh < pNode->iMeshCount; iMesh++ )
	{
		// are pointers the same
		if ( pMesh == pNode->ppMeshList [ iMesh ] )
			return;
	}
	
	// if we already have content then we must create a new list
	sMesh** ppNewMeshList = new sMesh* [ pNode->iMeshCount + 1 ];

	// memory must be valid
	if ( !ppNewMeshList )
		return;

	// copy old data across
	for ( iMesh = 0; iMesh < pNode->iMeshCount; iMesh++ )
		ppNewMeshList [ iMesh ] = pNode->ppMeshList [ iMesh ];
	
	// store pointer to new mesh in new list
	ppNewMeshList [ pNode->iMeshCount ] = pMesh;

	// increment the mesh count
	pNode->iMeshCount++;

	// increment polygon count
	pNode->iPolygonCount += pMesh->dwVertexCount;

	// delete the original list
	SAFE_DELETE_ARRAY ( pNode->ppMeshList );

	// store pointer to the new list
	pNode->ppMeshList = ppNewMeshList;
}

void cUniverse::CopyCollisionDataToNode ( sNode* pNode, sMesh* pOriginalMesh, sMesh* pReducedMesh, DWORD dwArbitaryValue )
{
	// check the parameters are valid
	if ( !pNode || !pOriginalMesh )
		return;

	// select mesh
	sMesh* pMesh = pOriginalMesh;
	if ( pReducedMesh ) pMesh = pReducedMesh;

	// get the offset map
	sOffsetMap	offsetMap;				// offset map for fvf data
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// get a pointer to the vertex data
	BYTE* pVertex = pMesh->pVertexData;
	if ( !pVertex )
		return;

	// indices or vertonly
	int iIndexMax = 0;
	if ( pMesh->dwIndexCount>0 )
		iIndexMax = (int)pMesh->dwIndexCount/3;
	else
		iIndexMax = (int)pMesh->dwVertexCount/3;

	// get polygons
	int iIndexPosition = 0;		// index position
	D3DXVECTOR3 vec [ 3 ];		// used to store position
	for ( int iIndex = 0; iIndex < iIndexMax; iIndex++ )
	{
		if ( pMesh->dwIndexCount>0 )
		{
			// indices
			// position a
			vec [ 0 ] = D3DXVECTOR3 ( 
										*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * pMesh->pIndices [ iIndexPosition ] ) ),
										*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * pMesh->pIndices [ iIndexPosition ] ) ),
										*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * pMesh->pIndices [ iIndexPosition ] ) )
									);
			iIndexPosition++;

			// position b
			vec [ 1 ] = D3DXVECTOR3 ( 
										*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * pMesh->pIndices [ iIndexPosition ] ) ),
										*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * pMesh->pIndices [ iIndexPosition ] ) ),
										*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * pMesh->pIndices [ iIndexPosition ] ) )
									);
			iIndexPosition++;

			// position c
			vec [ 2 ] = D3DXVECTOR3 ( 
										*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * pMesh->pIndices [ iIndexPosition ] ) ),
										*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * pMesh->pIndices [ iIndexPosition ] ) ),
										*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * pMesh->pIndices [ iIndexPosition ] ) )
									);
			iIndexPosition++;
		}
		else
		{
			// vertonly
			// position a
			vec [ 0 ] = D3DXVECTOR3 ( 
										*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * iIndexPosition ) ),
										*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * iIndexPosition ) ),
										*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * iIndexPosition ) )
									);
			iIndexPosition++;

			// position b
			vec [ 1 ] = D3DXVECTOR3 ( 
										*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * iIndexPosition ) ),
										*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * iIndexPosition ) ),
										*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * iIndexPosition ) )
									);
			iIndexPosition++;

			// position c
			vec [ 2 ] = D3DXVECTOR3 ( 
										*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * iIndexPosition ) ),
										*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * iIndexPosition ) ),
										*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * iIndexPosition ) )
									);
			iIndexPosition++;
		}

		// set up polygon
		sCollisionPolygon	polygon;

		// warp collision data with volume ellipse (was quicker in previous col system, now?)
		for ( int v=0; v<3; v++ )
		{
			vec [ v ].x /= 10.0f;
			vec [ v ].y /= 30.0f;
			vec [ v ].z /= 10.0f;
		}

		// add the vertices to the list
		polygon.triangle [ 0 ].vecPosition = vec [ 0 ];
		polygon.triangle [ 1 ].vecPosition = vec [ 1 ];
		polygon.triangle [ 2 ].vecPosition = vec [ 2 ];

		// apply special diffuse id used to store arbitary number (material-sound-value etc)
		polygon.diffuseid = dwArbitaryValue;

		// send polygon to back of collision layer
		pNode->collisionE.push_back ( polygon );
	}
}

bool cUniverse::IsObjectWithinZone ( sNode* pNode, sMesh* pMesh )
{
	// is the object within a node

	// start by checking our parameters
	if ( !pNode || !pMesh )
		return false;

	// get the offset map for the vertices
	sOffsetMap	offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// centre point
	D3DXVECTOR3 vecCentre    = pNode->vecCentre;
	D3DXVECTOR3 vecDimension = pNode->vecDimension;

	// work out the real world locations based on the centre point
	D3DXVECTOR3 vecTopLeftFront     = D3DXVECTOR3 ( vecCentre.x - vecDimension.x, vecCentre.y + vecDimension.y, vecCentre.z + vecDimension.z );
	D3DXVECTOR3 vecTopLeftBack      = D3DXVECTOR3 ( vecCentre.x - vecDimension.x, vecCentre.y + vecDimension.y, vecCentre.z - vecDimension.z );
	D3DXVECTOR3 vecTopRightBack     = D3DXVECTOR3 ( vecCentre.x + vecDimension.x, vecCentre.y + vecDimension.y, vecCentre.z - vecDimension.z );
	D3DXVECTOR3 vecTopRightFront    = D3DXVECTOR3 ( vecCentre.x + vecDimension.x, vecCentre.y + vecDimension.y, vecCentre.z + vecDimension.z );
	D3DXVECTOR3 vecBottomLeftFront  = D3DXVECTOR3 ( vecCentre.x - vecDimension.x, vecCentre.y - vecDimension.y, vecCentre.z + vecDimension.z );
	D3DXVECTOR3 vecBottomLeftBack   = D3DXVECTOR3 ( vecCentre.x - vecDimension.x, vecCentre.y - vecDimension.y, vecCentre.z - vecDimension.z );
	D3DXVECTOR3 vecBottomRightBack  = D3DXVECTOR3 ( vecCentre.x + vecDimension.x, vecCentre.y - vecDimension.y, vecCentre.z - vecDimension.z );
	D3DXVECTOR3 vecBottomRightFront = D3DXVECTOR3 ( vecCentre.x + vecDimension.x, vecCentre.y - vecDimension.y, vecCentre.z + vecDimension.z );

	// go through all vertices
	for ( int iVertex = 0; iVertex < ( int ) pMesh->dwVertexCount; iVertex++ )
	{
		// get a pointer to the vertex data
		BYTE* pVertex = pMesh->pVertexData;

		// check pointer
		if ( !pVertex )
			return false;

		// get position
		D3DXVECTOR3 vec = D3DXVECTOR3 ( 
										*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * iVertex ) ),
										*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * iVertex ) ),
										*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * iVertex ) )
									  );

		// left or right of boundary
		if ( vec.x < vecTopLeftFront.x || vec.x > vecTopRightBack.x )
			return false;

		// above or below
		if ( vec.y < vecBottomLeftFront.y || vec.y > vecTopLeftFront.y )
			return false;

		// behind or in front of boundary
		if ( vec.z < vecTopLeftBack.z || vec.z > vecTopLeftFront.z )
			return false;
	}

	// mesh must be within node
	return true;
}

void cUniverse::DetermineObjectLocation ( sNode* pNode, sMesh* pMesh, bool* pbLocationList )
{
	// find out which side the object is on

	// see if pointers are valid
	if ( !pNode || !pMesh || !pbLocationList )
		return;

	// initial location
	pbLocationList [ 0 ] = false;
	pbLocationList [ 1 ] = false;
	pbLocationList [ 2 ] = false;
	pbLocationList [ 3 ] = false;
	pbLocationList [ 4 ] = false;
	pbLocationList [ 5 ] = false;
	pbLocationList [ 6 ] = false;
	pbLocationList [ 7 ] = false;

	// get the offset map for the vertices
	sOffsetMap	offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// check against centre point only

	// centre point
	D3DXVECTOR3 vecCentre = pNode->vecCentre;
	D3DXVECTOR3 vec       = pMesh->Collision.vecCentre;

	if ( ( vec.x <= vecCentre.x ) && ( vec.y >= vecCentre.y ) && ( vec.z >= vecCentre.z ) )
		pbLocationList [ TopLeftFront ] = true;

	if ( ( vec.x <= vecCentre.x ) && ( vec.y >= vecCentre.y ) && ( vec.z <= vecCentre.z ) )
		pbLocationList [ TopLeftBack ] = true;

	if ( ( vec.x >= vecCentre.x ) && ( vec.y >= vecCentre.y ) && ( vec.z <= vecCentre.z ) )
		pbLocationList [ TopRightBack ] = true;

	if ( ( vec.x >= vecCentre.x ) && ( vec.y >= vecCentre.y ) && ( vec.z >= vecCentre.z ) )
		pbLocationList [ TopRightFront ] = true;

	if ( ( vec.x <= vecCentre.x ) && ( vec.y <= vecCentre.y ) && ( vec.z >= vecCentre.z ) )
		pbLocationList [ BottomLeftFront ] = true;

	if ( ( vec.x <= vecCentre.x ) && ( vec.y <= vecCentre.y ) && ( vec.z <= vecCentre.z ) )
		pbLocationList [ BottomLeftBack ] = true;

	if ( ( vec.x >= vecCentre.x ) && ( vec.y <= vecCentre.y ) && ( vec.z <= vecCentre.z ) )
		pbLocationList [ BottomRightBack ] = true;

	if ( ( vec.x >= vecCentre.x ) && ( vec.y <= vecCentre.y ) && ( vec.z >= vecCentre.z ) )
		pbLocationList [ BottomRightFront ] = true;
}

void cUniverse::TransformSelectedVertices ( sMesh* pMesh, DWORD dwFrom, DWORD dwTo, D3DXMATRIX* pmatWorld )
{
	// transform vertices by a given matrix

	// check the mesh is valid
	if ( !pMesh )
		return;

	// get the offset map for the vertices
	sOffsetMap	offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// world with no translation
	D3DXMATRIX matNoTrans = *pmatWorld;
	matNoTrans._41=0.0f;
	matNoTrans._42=0.0f;
	matNoTrans._43=0.0f;

	// now we need to transform the vertices for each object
	for ( int iVertex = dwFrom; iVertex < ( int ) dwTo; iVertex++ )
	{
		// get a pointer to the vertex data
		BYTE* pVertex = pMesh->pVertexData;

		// vertex pointer must be valid
		if ( !pVertex )
			return;

		// transform position
		if ( offsetMap.dwZ!=0 )
		{
			D3DXVECTOR3 vecPosition = D3DXVECTOR3 ( 
													*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * iVertex ) ),
													*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * iVertex ) ),
													*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * iVertex ) )
												  );

			D3DXVec3TransformCoord ( &vecPosition, &vecPosition, pmatWorld );

			*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * iVertex ) ) = vecPosition.x;
			*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * iVertex ) ) = vecPosition.y;
			*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * iVertex ) ) = vecPosition.z;
		}

		// transform normal
		if ( offsetMap.dwNX!=0 )
		{
			D3DXVECTOR3 vecNormal = D3DXVECTOR3 ( 
													*( ( float* ) pVertex + offsetMap.dwNX + ( offsetMap.dwSize * iVertex ) ),
													*( ( float* ) pVertex + offsetMap.dwNY + ( offsetMap.dwSize * iVertex ) ),
													*( ( float* ) pVertex + offsetMap.dwNZ + ( offsetMap.dwSize * iVertex ) )
												  );

			D3DXVec3TransformNormal ( &vecNormal, &vecNormal, &matNoTrans );

			*( ( float* ) pVertex + offsetMap.dwNX + ( offsetMap.dwSize * iVertex ) ) = vecNormal.x;
			*( ( float* ) pVertex + offsetMap.dwNY + ( offsetMap.dwSize * iVertex ) ) = vecNormal.y;
			*( ( float* ) pVertex + offsetMap.dwNZ + ( offsetMap.dwSize * iVertex ) ) = vecNormal.z;
		}

		// transform tangent coords
		if ( offsetMap.dwTW[1]!=0 )
		{
			D3DXVECTOR3 vecTangent = D3DXVECTOR3 ( 
													*( ( float* ) pVertex + offsetMap.dwTU[1] + ( offsetMap.dwSize * iVertex ) ),
													*( ( float* ) pVertex + offsetMap.dwTV[1] + ( offsetMap.dwSize * iVertex ) ),
													*( ( float* ) pVertex + offsetMap.dwTZ[1] + ( offsetMap.dwSize * iVertex ) )
												  );

			D3DXVec3TransformNormal ( &vecTangent, &vecTangent, &matNoTrans );

			*( ( float* ) pVertex + offsetMap.dwTU[1] + ( offsetMap.dwSize * iVertex ) ) = vecTangent.x;
			*( ( float* ) pVertex + offsetMap.dwTV[1] + ( offsetMap.dwSize * iVertex ) ) = vecTangent.y;
			*( ( float* ) pVertex + offsetMap.dwTZ[1] + ( offsetMap.dwSize * iVertex ) ) = vecTangent.z;
		}

		if ( offsetMap.dwTW[2]!=0 )
		{
			D3DXVECTOR3 vecTangent = D3DXVECTOR3 ( 
													*( ( float* ) pVertex + offsetMap.dwTU[2] + ( offsetMap.dwSize * iVertex ) ),
													*( ( float* ) pVertex + offsetMap.dwTV[2] + ( offsetMap.dwSize * iVertex ) ),
													*( ( float* ) pVertex + offsetMap.dwTZ[2] + ( offsetMap.dwSize * iVertex ) )
												  );

			D3DXVec3TransformNormal ( &vecTangent, &vecTangent, &matNoTrans );

			*( ( float* ) pVertex + offsetMap.dwTU[2] + ( offsetMap.dwSize * iVertex ) ) = vecTangent.x;
			*( ( float* ) pVertex + offsetMap.dwTV[2] + ( offsetMap.dwSize * iVertex ) ) = vecTangent.y;
			*( ( float* ) pVertex + offsetMap.dwTZ[2] + ( offsetMap.dwSize * iVertex ) ) = vecTangent.z;
		}
	}
}

void cUniverse::TransformVertices ( sMesh* pMesh, D3DXMATRIX matWorld )
{
	// transform vertices by a given matrix
	TransformSelectedVertices ( pMesh, 0, pMesh->dwVertexCount, &matWorld );
}

bool cUniverse::Remove ( int iID )
{
	// remove an object from nodes

	return true;
}

void cUniverse::SetupVertex ( sPortalVertex* pVertex, D3DXVECTOR3 vecPosition, D3DXVECTOR3 vecNormal, float tu, float tv )
{
	// set vertex information

	// check vertex pointer is valid
	if ( !pVertex )
		return;

	// store values
	pVertex->vecPosition = vecPosition;							// position
	pVertex->vecNormal   = vecNormal;							// normal
	pVertex->tu          = tu;									// texture a
	pVertex->tv          = tv;									// texture b
	pVertex->dwDiffuse   = m_dwColour;							// diffuse colour
}

void cUniverse::CreatePortalVertices ( sNode* pNode, bool bSetup, DWORD dwColour )
{
	// create vertices for portals

	// check node is valid
	if ( !pNode )
		return;

	m_dwColour = dwColour;

	// back
	SetupVertex ( &pNode->portals [ 0 ].vertices [ 0 ], GetTopRightBack    ( pNode ), D3DXVECTOR3 ( 0.0f, 0.0f, 1.0f ), 1.0f, 0.0f );
	SetupVertex ( &pNode->portals [ 0 ].vertices [ 1 ], GetTopLeftBack     ( pNode ), D3DXVECTOR3 ( 0.0f, 0.0f, 1.0f ), 0.0f, 0.0f );
	SetupVertex ( &pNode->portals [ 0 ].vertices [ 2 ], GetBottomRightBack ( pNode ), D3DXVECTOR3 ( 0.0f, 0.0f, 1.0f ), 0.0f, 1.0f );
	SetupVertex ( &pNode->portals [ 0 ].vertices [ 3 ], GetBottomLeftBack  ( pNode ), D3DXVECTOR3 ( 0.0f, 0.0f, 1.0f ), 1.0f, 1.0f );

	// front
	SetupVertex ( &pNode->portals [ 1 ].vertices [ 0 ], GetTopRightFront    ( pNode ), D3DXVECTOR3 ( 0.0f, 0.0f, -1.0f ), 1.0f, 0.0f );
	SetupVertex ( &pNode->portals [ 1 ].vertices [ 1 ], GetTopLeftFront     ( pNode ), D3DXVECTOR3 ( 0.0f, 0.0f, -1.0f ), 0.0f, 0.0f );
	SetupVertex ( &pNode->portals [ 1 ].vertices [ 2 ], GetBottomRightFront ( pNode ), D3DXVECTOR3 ( 0.0f, 0.0f, -1.0f ), 0.0f, 1.0f );
	SetupVertex ( &pNode->portals [ 1 ].vertices [ 3 ], GetBottomLeftFront  ( pNode ), D3DXVECTOR3 ( 0.0f, 0.0f, -1.0f ), 1.0f, 1.0f );

	// top
	SetupVertex ( &pNode->portals [ 2 ].vertices [ 0 ], GetTopRightBack  ( pNode ),	D3DXVECTOR3 ( 0.0f, 1.0f, 0.0f ), 1.0f, 0.0f );
	SetupVertex ( &pNode->portals [ 2 ].vertices [ 1 ], GetTopLeftBack   ( pNode ), D3DXVECTOR3 ( 0.0f, 1.0f, 0.0f ), 0.0f, 0.0f );
	SetupVertex ( &pNode->portals [ 2 ].vertices [ 2 ], GetTopRightFront ( pNode ),	D3DXVECTOR3 ( 0.0f, 1.0f, 0.0f ), 0.0f, 1.0f );
	SetupVertex ( &pNode->portals [ 2 ].vertices [ 3 ], GetTopLeftFront  ( pNode ), D3DXVECTOR3 ( 0.0f, 1.0f, 0.0f ), 1.0f, 1.0f );

	// bottom
	SetupVertex ( &pNode->portals [ 3 ].vertices [ 0 ], GetBottomRightBack  ( pNode ), D3DXVECTOR3 ( 0.0f, -1.0f, 0.0f ), 1.0f, 0.0f );
	SetupVertex ( &pNode->portals [ 3 ].vertices [ 1 ], GetBottomLeftBack   ( pNode ), D3DXVECTOR3 ( 0.0f, -1.0f, 0.0f ), 0.0f, 0.0f );
	SetupVertex ( &pNode->portals [ 3 ].vertices [ 2 ], GetBottomRightFront ( pNode ), D3DXVECTOR3 ( 0.0f, -1.0f, 0.0f ), 0.0f, 1.0f );
	SetupVertex ( &pNode->portals [ 3 ].vertices [ 3 ], GetBottomLeftFront  ( pNode ), D3DXVECTOR3 ( 0.0f, -1.0f, 0.0f ), 1.0f, 1.0f );

	// left
	SetupVertex ( &pNode->portals [ 4 ].vertices [ 0 ], GetTopRightBack     ( pNode ), D3DXVECTOR3 ( 1.0f, 0.0f, 0.0f ), 1.0f, 0.0f );
	SetupVertex ( &pNode->portals [ 4 ].vertices [ 1 ], GetTopRightFront    ( pNode ), D3DXVECTOR3 ( 1.0f, 0.0f, 0.0f ), 0.0f, 0.0f );
	SetupVertex ( &pNode->portals [ 4 ].vertices [ 2 ], GetBottomRightBack  ( pNode ), D3DXVECTOR3 ( 1.0f, 0.0f, 0.0f ), 0.0f, 1.0f );
	SetupVertex ( &pNode->portals [ 4 ].vertices [ 3 ], GetBottomRightFront ( pNode ), D3DXVECTOR3 ( 1.0f, 0.0f, 0.0f ), 1.0f, 1.0f );

	// right
	SetupVertex ( &pNode->portals [ 5 ].vertices [ 0 ], GetTopLeftBack     ( pNode ), D3DXVECTOR3 ( -1.0f, 0.0f, 0.0f ), 1.0f, 0.0f );
	SetupVertex ( &pNode->portals [ 5 ].vertices [ 1 ], GetTopLeftFront    ( pNode ), D3DXVECTOR3 ( -1.0f, 0.0f, 0.0f ), 0.0f, 0.0f );
	SetupVertex ( &pNode->portals [ 5 ].vertices [ 2 ], GetBottomLeftBack  ( pNode ), D3DXVECTOR3 ( -1.0f, 0.0f, 0.0f ), 0.0f, 1.0f );
	SetupVertex ( &pNode->portals [ 5 ].vertices [ 3 ], GetBottomLeftFront ( pNode ), D3DXVECTOR3 ( -1.0f, 0.0f, 0.0f ), 1.0f, 1.0f );

	if ( bSetup )
	{
		// all visible by default
		pNode->portals [ 0 ].bVisible = true;
		pNode->portals [ 1 ].bVisible = true;
		pNode->portals [ 2 ].bVisible = true;
		pNode->portals [ 3 ].bVisible = true;
		pNode->portals [ 4 ].bVisible = true;
		pNode->portals [ 5 ].bVisible = true;

		// in viewing frustum by default
		pNode->portals [ 0 ].bVisibleInViewingFrustum = true;
		pNode->portals [ 1 ].bVisibleInViewingFrustum = true;
		pNode->portals [ 2 ].bVisibleInViewingFrustum = true;
		pNode->portals [ 3 ].bVisibleInViewingFrustum = true;
		pNode->portals [ 4 ].bVisibleInViewingFrustum = true;
		pNode->portals [ 5 ].bVisibleInViewingFrustum = true;
	}
}

