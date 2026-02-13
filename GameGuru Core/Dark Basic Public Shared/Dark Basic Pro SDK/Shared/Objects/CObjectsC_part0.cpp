//
// CObjectsC Functions Implementation
//

//#define _CRT_SECURE_NO_DEPRECATE
#pragma warning(disable:4800)
#include "CObjectsC.h"
#include "CGfxC.h"
#include ".\..\Core\SteamCheckForWorkshop.h"
#include "cVectorC.h"
#include "CMemblocks.h"
#include "cOccluderThread.h"
#include <algorithm>

//#include "ShadowMapping\cShadowMaps.h" DX12 not needed here

#include "CFileC.h"
#ifdef WICKEDENGINE
#include ".\..\..\..\..\Guru-WickedMAX\wickedcalls.h"
#include "M-Entity.h"
#endif

// Occlusion object global
#include "Occlusion\cOcclusion.h"
extern COcclusion g_Occlusion;

//.extern CascadedShadowsManager g_CascadedShadow;

// External Globals
extern bool g_bSwitchLegacyOn;
extern bool g_bFastBoundsCalculation;
extern GGHANDLE g_pMainCameraDepthHandle;
extern LPGGEFFECT g_pMainCameraDepthEffect;
extern bool g_bSkipAnyDedicatedDepthRendering;

// Global Lists
std::vector< GGHANDLE > g_EffectParamHandleList;
int g_iCurrentGunObj = 0; // updated in G-Entity.cpp

// Offloads interect tests when performance needed to extra thread
#include "..\\..\\..\\..\\Guru-WickedMAX\\GGThread.h"
GGThread::threadLock g_IntersectDatabaseExtraThreadItemListLock;
struct sIntersectDatabaseExtraThreadItem
{
	int iPrimaryStart;
	int iPrimaryEnd;
	float fX;
	float fY;
	float fZ;
	float fNewX;
	float fNewY;
	float fNewZ;
	int iIgnoreObjNo;
	int iStaticOnly;
	int iIndexInIntersectDatabase;
	int iLifeInMilliseconds;
	int iIgnorePlayerCapsule;
	bool bFullWickedAccuracy;
};
std::vector<sIntersectDatabaseExtraThreadItem> g_IntersectDatabaseExtraThreadItemList;

#ifndef NOSTEAMORVIDEO
void timestampactivity(int i, char* desc_s); // for debug
#endif

// Global Intersect All Helpers
struct OrderByCamDistance
{
    bool operator()(sObject* pObjectA, sObject* pObjectB)
    {
        if (pObjectA->position.fCamDistance < pObjectB->position.fCamDistance)
            return true;
        if (pObjectA->position.fCamDistance == pObjectB->position.fCamDistance)
            return (pObjectA->dwObjectNumber < pObjectB->dwObjectNumber);
        return false;
    }
};
std::vector< sObject* > g_pIntersectShortList;

// Global to store a second range of objects for IntersectAll special mode
int g_iIntersectAllSecondStart = 0;
int g_iIntersectAllSecondEnd = 0;
// Globals for a third rane of objects
int g_iIntersectAllThirdStart = 0;
int g_iIntersectAllThirdEnd = 0;
bool g_bIgnoreCollisionPropertyOnce = false;
//#define SKIPGRIDUSED
#ifdef SKIPGRIDUSED
float g_fIntersectAllSkipGridX = 0;
float g_fIntersectAllSkipGridZ = 0;
DWORD g_dwSkipGrid[1024][1024];
int g_iSkipGridResult[1024][1024];
#endif

// Global can be changed to improve performance (0-allow CPU, 3-do not allow any CPU anim animation)
int g_iDefaultCPUAnimState = 0;

// For DB_ObjectScreenData for more accurate reporting of 'in screen' setting.
namespace
{
    // u74b7 - Generate the frustum planes from the transformation matrix
    void ExtractFrustumPlanes(GGPLANE p_Planes[6], const GGMATRIX & matCamera)
    {
        // Left clipping plane
        p_Planes[0].a = matCamera._14 + matCamera._11;
        p_Planes[0].b = matCamera._24 + matCamera._21;
        p_Planes[0].c = matCamera._34 + matCamera._31;
        p_Planes[0].d = matCamera._44 + matCamera._41;

        // Right clipping plane
        p_Planes[1].a = matCamera._14 - matCamera._11;
        p_Planes[1].b = matCamera._24 - matCamera._21;
        p_Planes[1].c = matCamera._34 - matCamera._31;
        p_Planes[1].d = matCamera._44 - matCamera._41;
        
        // Top clipping plane
        p_Planes[2].a = matCamera._14 - matCamera._12;
        p_Planes[2].b = matCamera._24 - matCamera._22;
        p_Planes[2].c = matCamera._34 - matCamera._32;
        p_Planes[2].d = matCamera._44 - matCamera._42;
        
        // Bottom clipping plane
        p_Planes[3].a = matCamera._14 + matCamera._12;
        p_Planes[3].b = matCamera._24 + matCamera._22;
        p_Planes[3].c = matCamera._34 + matCamera._32;
        p_Planes[3].d = matCamera._44 + matCamera._42;
        
        // Near clipping plane
        p_Planes[4].a = matCamera._13;
        p_Planes[4].b = matCamera._23;
        p_Planes[4].c = matCamera._33;
        p_Planes[4].d = matCamera._43;
        
        // Far clipping plane
        p_Planes[5].a = matCamera._14 - matCamera._13;
        p_Planes[5].b = matCamera._24 - matCamera._23;
        p_Planes[5].c = matCamera._34 - matCamera._33;
        p_Planes[5].d = matCamera._44 - matCamera._43;
        
        // Normalise the planes
        for (int i = 0; i < 6; ++i)
            GGPlaneNormalize(&p_Planes[i], &p_Planes[i]);
    }

    // Calculate the minimum signed distance from the plane to a point
    inline float DistancePlaneToPoint(const GGPLANE & Plane, const GGVECTOR3 & pt)
    {
        return Plane.a*pt.x + Plane.b*pt.y + Plane.c*pt.z + Plane.d;
    }

    bool ContainsSphere(const GGPLANE p_Planes[6], const GGVECTOR3& vecCentre, const float fRadius)
    {
	    // calculate if sphere is on the correct 'side' of each plane
	    for(int i = 0; i < 6; ++i)
        {
            // If sphere on the wrong side, we're done
            if (DistancePlaneToPoint(p_Planes[i], vecCentre) < -fRadius)
                 return false;
	    }

	    // otherwise we are fully in view
	    return true;
    }
    
    /*
    inline bool ContainsPoint(const GGPLANE p_Plane[6], const GGVECTOR3& vecPoint)
    {
        return ContainsSphere(p_Plane, vecPoint, 0.0);
    }
    */

    float ApplyPivot ( sObject* pObject, int iMode, GGVECTOR3 vecValue, float fValue )
    {
	    if ( pObject->position.bApplyPivot )
	    {
		    GGVec3TransformCoord ( &vecValue, &vecValue, &pObject->position.matPivot );

		    if ( iMode == 0 ) return vecValue.x;
		    if ( iMode == 1 ) return vecValue.y;
		    if ( iMode == 2 ) return vecValue.z;
	    }

	    return fValue;
    }

    SDK_FLOAT GetAxisSizeFromVectorOffset ( int iID, int iActualSize, int iVectorOffset )
    {
        // iActualSize is 0 = unscaled, 1 = scaled, 2 = scaled (including originalmatrix scaling)
        // iVectorOffset is 0 = x, 1 = y, 2 = z

	    // Check the object exists
	    if ( !ConfirmObjectInstance ( iID ) )
		    return 0;

        // Get the object pointer
	    sObject* pObject = g_ObjectList [ iID ];

        // If the object is an instance, grab the scaling from the instance (to be applied later)
        // and move on to the object itself
        float fAdjustScale = 1.0;
	    if ( pObject->pInstanceOfObject )
        {
            fAdjustScale = pObject->position.vecScale[ iVectorOffset ];
            pObject = pObject->pInstanceOfObject;
        }

        // Get the precomputed size of the objects x dimension
        float fValue = (pObject->collision.vecMax[ iVectorOffset ] - pObject->collision.vecMin[ iVectorOffset ]);
    	
	    // Apply pivot if needed
	    fValue = ApplyPivot ( pObject, iVectorOffset, GGVECTOR3 ( pObject->collision.vecMax - pObject->collision.vecMin ), fValue );

	    // Ensure size is reported as positive
	    fValue = fabs ( fValue );

	    // Adjusts to scale now
	    if ( iActualSize==1 || iActualSize==2 )
		{
			// basic scale value from object
            fValue = fValue * pObject->position.vecScale[ iVectorOffset ] * fAdjustScale;
		}
	    if ( iActualSize==2 )
		{
			// additionally, apply original matrix scaling
			GGVECTOR3 vecUnit = GGVECTOR3(1,1,1);
			GGVec3TransformCoord ( &vecUnit, &vecUnit, &pObject->ppFrameList[0]->matOriginal );
            fValue = fValue * vecUnit[ iVectorOffset ];
		}

		// return scale value
	    return fValue;
    }
}

void* GetObjectsInternalData ( int iID )
{
	// ensure the object is present
	if ( !ConfirmObject ( iID ) )
		return NULL;

	// return a pointer to the data
	return (void*)g_ObjectList [ iID ];
}

// Functions moved from other parts so they can be called by g_xxx_funcptrs

DARKSDK_DLL void ConvertToFVF ( sMesh* pMesh, DWORD dwFVF )
{
	//PE: Only if we are actually going to change FVF.
	if (pMesh->dwFVF != dwFVF)
	{
		// when mesh changes FVF, really need to erase old orig data
		// simply because it will attempt to 'copy' when asked to reset, and it will copy the wrong FVF pattern
		SAFE_DELETE_ARRAY(pMesh->pOriginalVertexData);

		// Use main FVF converter function
		ConvertLocalMeshToFVF(pMesh, dwFVF);
	}
}

DARKSDK_DLL void SmoothNormals ( sMesh* pMesh, float fPercentage )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// faster method assumes vertices are shared, so go through faces, collect normals for each vertex in the face
	// then we can average them at the end
	if ( offsetMap.dwZ>0 && offsetMap.dwNZ>0 )
	{
		// index buffer or raw vertice list
		bool bUsingIndices = true;
		DWORD iCount = pMesh->dwIndexCount;
		if ( iCount == 0 ) { iCount = pMesh->dwVertexCount; bUsingIndices = false; }

		// a normal for each vertex in mesh (we will accumilate normals into these slots)
		int* iNormalCount = new int [pMesh->dwVertexCount];
		GGVECTOR3* fNormals = new GGVECTOR3 [pMesh->dwVertexCount];
		for ( DWORD v=0; v<pMesh->dwVertexCount; v++ ) 
		{
			iNormalCount[v] = 0;
			fNormals[v] = GGVECTOR3(0,0,0);
		}

		// go through all polys, collect normals for each face vert
		for ( DWORD i=0; i<iCount; i+=3 )
		{
			// read face
			DWORD dwFace0, dwFace1, dwFace2;
			if ( bUsingIndices == true )
			{
				dwFace0 = pMesh->pIndices[i+0];
				dwFace1 = pMesh->pIndices[i+1];
				dwFace2 = pMesh->pIndices[i+2];
			}
			else
			{
				dwFace0 = i+0;
				dwFace1 = i+1;
				dwFace2 = i+2;
			}

			// get vertex
			GGVECTOR3 vecVert0 = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * dwFace0 ) );
			GGVECTOR3 vecVert1 = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * dwFace1 ) );
			GGVECTOR3 vecVert2 = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * dwFace2 ) );

			// get normal
			//GGVECTOR3* pvecNorm0 = (GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * dwFace0 ) );
			//GGVECTOR3* pvecNorm1 = (GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * dwFace1 ) );
			//GGVECTOR3* pvecNorm2 = (GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * dwFace2 ) );

			// calculate normal from vertices
			GGVECTOR3 vNormal;
			GGVec3Cross ( &vNormal, &( vecVert2 - vecVert1 ), &( vecVert0 - vecVert1 ) );
			GGVec3Normalize ( &vNormal, &vNormal );

			// apply new normal to geometry for all normals associated with the poly
			//*(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * dwFace0 ) ) = vNormal;
			//*(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * dwFace1 ) ) = vNormal;
			//*(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * dwFace2 ) ) = vNormal;

			// for now average everything, ignore percentage threshold
			fNormals[dwFace0] += vNormal;
			fNormals[dwFace1] += vNormal;
			fNormals[dwFace2] += vNormal;
			iNormalCount[dwFace0] += 1;
			iNormalCount[dwFace1] += 1;
			iNormalCount[dwFace2] += 1;
		}

		// we now have accumilated normals associated with vertices, now average them and write result into normal vector
		for ( int iCurrentVertex = 0; iCurrentVertex < (int)pMesh->dwVertexCount; iCurrentVertex++ )
		{
			// get normal vector for vertex
			GGVECTOR3* pvecOrigNormal = (GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * iCurrentVertex ) );

			// can only average normals that are used by faces
			if ( iNormalCount[iCurrentVertex] > 0 )
			{
				// average normal
				GGVECTOR3 vecAveragedNormal = fNormals[iCurrentVertex] / iNormalCount[iCurrentVertex];
				*pvecOrigNormal = vecAveragedNormal;
			}
		}

		// free usages
		SAFE_DELETE ( iNormalCount );
		SAFE_DELETE ( fNormals );
	}

	/*
	// assume no more than 32 shared vertex points
	DWORD dwSharedVertexMax=32;
	DWORD dwNumberOfVertices=pMesh->dwVertexCount;
	BYTE* NormalCount = new BYTE [ dwNumberOfVertices ];
	ZeroMemory ( NormalCount, dwNumberOfVertices*sizeof(BYTE) );
	GGVECTOR3* fNormals = new GGVECTOR3 [dwNumberOfVertices*dwSharedVertexMax];
	int* pNormalRef = new int [dwNumberOfVertices*dwSharedVertexMax];

	// check if same
	float fEpsilonRange = 0.01f;

	// make sure we have data in the vertices
	if ( offsetMap.dwZ>0 && offsetMap.dwNZ>0 )
	{
		// This is super slow (8-15seconds on regular 18K character)
		// each vertex checks every other vertex 7000*7000 iterations!
		// go through all of the vertices
		for ( int iCurrentVertex = 0; iCurrentVertex < (int)dwNumberOfVertices; iCurrentVertex++ )
		{
			// see how many other vertices share this space
			GGVECTOR3 vecVert = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * iCurrentVertex ) );
			//float* pQuickScanVertPtr = (float*) pMesh->pVertexData + offsetMap.dwX;
			for ( int iScanVert = 0; iScanVert < (int)dwNumberOfVertices; iScanVert++ )
			{
				// 150416 -V1.131-b1 too slow this one, need an early out
				//if ( *pQuickScanVertPtr != vecVert.x ) continue;
				//pQuickScanVertPtr += offsetMap.dwSize;

				// get vertex position and scan vertex position
				GGVECTOR3 vecScanVert = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * iScanVert ) );

				// determine if vectors near each other
				float fDX = fabs ( vecVert.x - vecScanVert.x );
				float fDY = fabs ( vecVert.y - vecScanVert.y );
				float fDZ = fabs ( vecVert.z - vecScanVert.z );
				float fDD = (fDX+fDY+fDZ)/3.0f;

				// if they match, add normal to table
				if ( iCurrentVertex!=iScanVert && fDD <= fEpsilonRange )
				{
					if ( fDD > 0.0f )
					{
						int iVertsCloseButNotLinedUp = 42;
					}

					// get normal from the scanned vertex
					GGVECTOR3 vecScannedNormal = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * iScanVert ) );

					// add normal to table
					BYTE Index = NormalCount [ iCurrentVertex ];
					if ( Index < 32 )
					{
						fNormals [ (iCurrentVertex*dwSharedVertexMax)+Index ] = vecScannedNormal;
						pNormalRef [ (iCurrentVertex*dwSharedVertexMax)+Index ] = iScanVert;
						NormalCount [ iCurrentVertex ] = NormalCount [ iCurrentVertex ] + 1;
					}
				}
			}
		}
	}

	// for each vertex, choose new normal that fits the smoothing rule
	for ( int iCurrentVertex = 0; iCurrentVertex < (int)dwNumberOfVertices; iCurrentVertex++ )
	{
		// get position and normal from vertex
		GGVECTOR3 vecOrigPosition = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * iCurrentVertex ) );
		GGVECTOR3 vecOrigNormal = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * iCurrentVertex ) );
				
		// for any vertices that share this space, consider their normals
		DWORD dwOtherNormalsCount = NormalCount [ iCurrentVertex ];
		if ( dwOtherNormalsCount > 0 )
		{
			// clear averaging vector
			DWORD dwAverageCount = 1;
			GGVECTOR3 vecAverageNormal = vecOrigNormal;

			// only include normals whos angle is within rule
			for ( DWORD iIndex=0; iIndex<dwOtherNormalsCount; iIndex++ )
			{
				// get other normal
				GGVECTOR3 vecOtherNormal = fNormals [ (iCurrentVertex*dwSharedVertexMax)+iIndex ];

				// calculate difference
				float fDiffX = (float)fabs(vecOrigNormal.x - vecOtherNormal.x);
				float fDiffY = (float)fabs(vecOrigNormal.y - vecOtherNormal.y);
				float fDiffZ = (float)fabs(vecOrigNormal.z - vecOtherNormal.z);

				// if all are within rule, add to averaging normal
				float fDifference = (fDiffX+fDiffY+fDiffZ) / 3.0f;
				if ( fDifference <= fPercentage )
				{
					vecAverageNormal += vecOtherNormal;
					dwAverageCount++;
				}
			}

			// finalse averaging
			vecAverageNormal /= (float)dwAverageCount;

			// apply new normal to data
			*(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * iCurrentVertex ) ) = vecAverageNormal;

			// also apply to all those that contributed to this average (as we stamp through this list in one direction only)
			for ( DWORD iIndex=0; iIndex<dwOtherNormalsCount; iIndex++ )
			{
				// set all other normals at same time
				int iOtherVertIndex = pNormalRef [ (iCurrentVertex*dwSharedVertexMax)+iIndex ];
				*(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * iOtherVertIndex ) ) = vecAverageNormal;

				// and also align position to be exactly this one (to help with true edge detection)
				*(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * iOtherVertIndex ) ) = vecOrigPosition;
			}
		}
	}

	// free usages
	SAFE_DELETE ( NormalCount );
	SAFE_DELETE ( fNormals );
	SAFE_DELETE ( pNormalRef );
	*/

	// flag mesh for a VB update
	pMesh->bVBRefreshRequired=true;
#ifndef WICKEDENGINE
	g_vRefreshMeshList.push_back ( pMesh );
#endif
}

//////////////////////////////////////////////////////////////////////////////////
// COMMANDS //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

DARKSDK_DLL void RefreshMeshShortList ( sMesh* pMesh )
{
	if ( pMesh==NULL ) return;
#ifndef WICKEDENGINE
	g_vRefreshMeshList.push_back ( pMesh );
#endif
}

DARKSDK_DLL void LoadCore ( char* szFilename, char* szOrgFilename, int iID, int iDBProMode, int iDivideTextureSize )
{
	// ensure the object is okay to use
	ConfirmNewObject ( iID );

	// check memory allocation
	ID_ALLOCATION ( iID );

	#ifdef WICKEDENGINE
	// textures loaded with model, so need path sooner
	char szModelPath[MAX_PATH];
	strcpy( szModelPath, "" );
	LPSTR pFullModelFile = (LPSTR)szOrgFilename;
	DWORD dwLength = strlen(pFullModelFile);
	for ( int n=dwLength; n>0; n-- )
	{
		if ( pFullModelFile[n]=='\\' || pFullModelFile[n]=='/' )
		{
			strcpy ( szModelPath, pFullModelFile );
			szModelPath[n+1]=0;
			break;
		}
	}
	#endif

	// load the object
	if ( !LoadDBO ( szFilename, &g_ObjectList [ iID ] ) )
		return;

	// setup new object and introduce to buffers
	if ( !SetNewObjectFinalProperties ( iID, -1.0f ) )
		return;

	#ifdef WICKEDENGINE
	//PE: Hide lowest LOD frames.
	Wicked_Hide_Lower_Lod_Meshes(iID);
	#endif

	// add object id to shortlist
	AddObjectToObjectListRef ( iID );

	// calculate path from filename
	char szPath [ MAX_PATH ];
	if ( _strnicmp ( (char*)szFilename+strlen((char*)szFilename)-4, ".mdl", 4 )==NULL )
	{
		// MDL models store their textures in the temp folder
		DBOCalculateLoaderTempFolder();
		strcpy ( szPath, g_WindowsTempDirectory );
	}
	else
	{
		//PE: This is why textures is found, and are doubble loaded.
		//PE: In standalone path is C:\Users\name\AppData\Local\Temp\\dbpdata\ , We need to use the real path.
		
		// Path is current model location
		strcpy( szPath, "" );
		LPSTR pFile = (LPSTR)szOrgFilename;
		DWORD dwLength = strlen(pFile);
		for ( int n=dwLength; n>0; n-- )
		{
			if ( pFile[n]=='\\' || pFile[n]=='/' )
			{
				strcpy ( szPath, pFile );
				szPath[n+1]=0;
				break;
			}
		}	
	}

	// prepare textures for all meshes (load them)
	sObject* pObject = g_ObjectList [ iID ];

	#ifdef WICKEDENGINE
	for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
	{
		sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
		if (pMesh)
		{
			// there is a chance an old DBO holds an absolute path to a texture
			// not very useful for making standalones, etc so detect for this and correct
			// if possible, otherwise allow absolute path through for deeper debugging
			LPSTR pAbsPathTest = pMesh->pTextures[0].pName;
			if (pAbsPathTest)
			{
				if (strlen(pAbsPathTest) > 2)
				{
					if (pAbsPathTest[1] == ':')
					{
						// this is an absolute path, now see if we can find the relative path in there
						char pCompare1[MAX_PATH];
						strcpy(pCompare1, pAbsPathTest);
						strlwr(pCompare1);
						char pCompare2[MAX_PATH];
						strcpy(pCompare2, szPath);
						strlwr(pCompare2);
						LPSTR pRelPathDetect = strstr (pCompare1, pCompare2);
						if (pRelPathDetect)
						{
							// found one, we can with confidence replace the absolute path with the relative path
							int iLengthOfDiscard = pRelPathDetect - pCompare1;
							if (iLengthOfDiscard > 0)
							{
								// now we can replace the absolute path with the relative path
								char szNewPath[MAX_PATH];
								strcpy(szNewPath, pAbsPathTest + iLengthOfDiscard);
								strcpy(pMesh->pTextures[0].pName, szNewPath);
							}
						}
						else
						{
							// this absolute path does not include the relative path to the DBO in the texture, which
							// suggests it relied on a local file outside of the MAX model naming convention, so should
							// be left as is to be picked up later to identify non compliant models
						}
					}
				}
			}

			// load this texture into wicked mesh
			WickedSetMeshNumber(iMeshIndex);
			LoadInternalTextures(pObject, pMesh, szPath, iDBProMode, iDivideTextureSize);
		}
	}
	#else
	for (int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++) 
	{
		LoadInternalTextures(pObject, pObject->ppMeshList[iMesh], szPath, iDBProMode, iDivideTextureSize);
	}
	#endif

	// 140817 - need this for lightmapper objects loading with shaders applied
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
	{
		// get mesh ptr
		sMesh* pMesh = pObject->ppMeshList [ iMesh ];
		if ( pMesh->bUseVertexShader )
		{
			// Search if effect already loaded (else create a new)
			CreateNewOrSharedEffect ( pMesh, true );
		}
	}
}

#ifndef NOSTEAMORVIDEO
void timestampactivity(int i, char* desc_s); // for debug
#endif

DARKSDK_DLL void LoadObject(LPSTR szFilename, int iID)
{
	// Uses actual or virtual file..
	char VirtualFilename[_MAX_PATH];
	strcpy(VirtualFilename, szFilename);

	// store current folder (typically mode dir)
	char pStoreCurrentDir[_MAX_PATH];
	GetCurrentDirectory(_MAX_PATH, pStoreCurrentDir);

	// determine if loading an encrypted model file
	bool bTempFolderChangeForEncrypt = CheckForWorkshopFile(VirtualFilename);

	// get path of original model file passed in
	char pPathToOriginalFile[_MAX_PATH];
	strcpy(pPathToOriginalFile, "");
	if ( strlen(VirtualFilename) > 0 )
	{
		// get relative path from current
		strcpy(pPathToOriginalFile, VirtualFilename);
		for (DWORD n = strlen(pPathToOriginalFile) - 1; n > 0; n--)
		{
			if (pPathToOriginalFile[n] == '\\' || pPathToOriginalFile[n] == '/' || (unsigned char)(pPathToOriginalFile[n]) < 32)
			{
				pPathToOriginalFile[n] = 0;
				break;
			}
		}
	}

	// Decrypt and use media, re-encrypt
	if ( g_pGlob->Decrypt ) g_pGlob->Decrypt(VirtualFilename);

	// Load media
	LoadCore ( VirtualFilename, szFilename, iID, 0, 0 );

	// Re-encrypt
	if ( g_pGlob->Encrypt ) g_pGlob->Encrypt( VirtualFilename );
}

DARKSDK_DLL void LoadObject ( LPSTR szFilename, int iID, int iDBProMode )
{
	// Uses actual or virtual file..
	char VirtualFilename[_MAX_PATH];
	strcpy(VirtualFilename, szFilename);
	CheckForWorkshopFile (VirtualFilename);

	// Decrypt and use media, re-encrypt
	g_pGlob->Decrypt( VirtualFilename );
	LoadCore ( VirtualFilename, szFilename, iID, iDBProMode, 0 );
	g_pGlob->Encrypt( VirtualFilename );
}

DARKSDK_DLL void LoadObject ( LPSTR szFilename, int iID, int iDBProMode, int iDivideTextureSize )
{
	// Uses actual or virtual file..
	char VirtualFilename[_MAX_PATH];
	strcpy(VirtualFilename, szFilename);
	CheckForWorkshopFile (VirtualFilename);

	// Decrypt and use media, re-encrypt
	g_pGlob->Decrypt( VirtualFilename );
	LoadCore ( VirtualFilename, szFilename, iID, iDBProMode, iDivideTextureSize );
	g_pGlob->Encrypt( VirtualFilename );
}

DARKSDK_DLL void EnsureObjectDBOIsFVF ( int iID, LPSTR pFileToLoad, DWORD dwRequiredFVF )
{
	// for new DX11, must ensure object FVF inside DBO is a specific type
	// i.e. the projectile/smoke DBOs where 338, but need to be 274 for decal/gui shaders, etc
	if ( !ConfirmObject ( iID ) )
		return;

	// get object ptr
	bool bConvertedOneOrMoreMeshes = false;
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMeshIndex=0; iMeshIndex<pObject->iMeshCount; iMeshIndex++ )
	{
		// make a new mesh from the original mesh, and ensure it's verts only
		sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
		if ( pMesh->dwFVF != dwRequiredFVF )
		{
			ConvertToFVF ( pMesh, dwRequiredFVF );
			bConvertedOneOrMoreMeshes = true;
		}
	}

	// and save new DBO if converted any of the meshes
	if ( bConvertedOneOrMoreMeshes == true )
	{
		if ( DoesFileExist ( pFileToLoad ) ) DeleteFile ( pFileToLoad );
		SaveObject ( pFileToLoad, iID );
	}
}


#define MAXFILECACHE 65536
char cache_buffer[MAXFILECACHE];
int cache_index = 0;

void CWriteFile(HANDLE hfile, char *pLine, int len, DWORD *byteswritten, void *overlap)
{
	if (cache_index + len >= MAXFILECACHE-10)
	{
		//Writeout.
		DWORD tmp;
		WriteFile(hfile, cache_buffer, cache_index, &tmp, NULL);
		cache_index = 0;
	}
	strncpy(&cache_buffer[cache_index], pLine, len);
	cache_index += len;
	byteswritten += len;
}
void CCloseHandle(HANDLE hfile)
{
	//Write out last buffers.
	DWORD tmp;
	WriteFile(hfile, cache_buffer, cache_index, &tmp, NULL);
	cache_index = 0;
	CloseHandle(hfile);
}


DARKSDK_DLL void SaveObjectEx ( LPSTR szFilename, int iID, bool bCompactOBJ )
{
	// ensure the object is present
	if ( !ConfirmObject ( iID ) )
		return;

	// get object ptr
	sObject* pObject = g_ObjectList [ iID ];

	// check ptr valid
	LPSTR pDBPFilename = szFilename;
	if ( pDBPFilename )
	{
		// U78 - if OBJ extension detected, switch to OBJ exporter
		// OBJ is a static format (no transforms, no animation, not much really)
		if ( strnicmp ( pDBPFilename + strlen(pDBPFilename) - 4, ".obj", 4 )==NULL )
		{
			// vertex indices are global to the file
			DWORD dwStartOfVertexBatch = 1;

			// Get just the name
			char pJustObjName[512];
			strcpy ( pJustObjName, pDBPFilename );
			pJustObjName[strlen(pJustObjName)-4]=0;

			// MTL file
			char pMTLFilename[512];
			strcpy ( pMTLFilename, pJustObjName );
			strcat ( pMTLFilename, ".mtl" );

			// open MTL file for writing
			HANDLE hMTLfile = NULL;
			if ( bCompactOBJ == false ) hMTLfile = GG_CreateFile ( pMTLFilename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			if ( bCompactOBJ == true || hMTLfile != INVALID_HANDLE_VALUE )
			{
				// write OBJ format
				LPSTR pLine = 0;
				DWORD byteswritten=0;
				HANDLE hfile = GG_CreateFile ( pDBPFilename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
				if ( hfile != INVALID_HANDLE_VALUE )
				{
					// header
					pLine = "# OBJ Model File converted by the mighty Game Creators\n";
					CWriteFile( hfile, pLine, strlen(pLine), &byteswritten, NULL ); 
					pLine = "# more tools found at www.thegamecreators.com\n";
					CWriteFile( hfile, pLine, strlen(pLine), &byteswritten, NULL ); 
					pLine = "\n";
					CWriteFile( hfile, pLine, strlen(pLine), &byteswritten, NULL ); 

					// material file name
					char pDynLine[512];
					if (bCompactOBJ == false)
					{
						pLine = "# Material library\n";
						CWriteFile(hfile, pLine, strlen(pLine), &byteswritten, NULL);
						sprintf (pDynLine, "mtllib %s\n\n", pMTLFilename);
						CWriteFile(hfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
					}

					// object name
					if (bCompactOBJ == false)
					{
						pLine = "# Object\n";
						CWriteFile(hfile, pLine, strlen(pLine), &byteswritten, NULL);
						sprintf (pDynLine, "o %s\n", pJustObjName);
						CWriteFile(hfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
						pLine = "\n";
						CWriteFile(hfile, pLine, strlen(pLine), &byteswritten, NULL);
					}

					// group name
					if (bCompactOBJ == false)
					{
						pLine = "# Mesh\n";
						CWriteFile(hfile, pLine, strlen(pLine), &byteswritten, NULL);
						pLine = "g mesh\n";
						CWriteFile(hfile, pLine, strlen(pLine), &byteswritten, NULL);
						pLine = "\n";
						CWriteFile(hfile, pLine, strlen(pLine), &byteswritten, NULL);
					}

					// vertices
					pLine = "# Vertex list\n";
					CWriteFile(hfile, pLine, strlen(pLine), &byteswritten, NULL);

					// dump all vertices from all meshes into OBJ
					#ifndef WICKEDENGINE
					sMesh** pVertOnlyMeshes = new sMesh*[pObject->iFrameCount];
					#endif
					for (int iFrameIndex = 0; iFrameIndex < pObject->iFrameCount; iFrameIndex++)
					{
						sFrame* pFrame = pObject->ppFrameList[iFrameIndex];
						if (pFrame)
						{
							sMesh* pMesh = pFrame->pMesh;
							if (pMesh)
							{
								sMesh* pWorkMesh = NULL;
								#ifdef WICKEDENGINE
								// for MAX, we retain indexing for OBJ export (more efficient)
								pWorkMesh = pMesh;
								#else
								// make a new mesh from the original mesh, and ensure it's verts only
								pVertOnlyMeshes[iFrameIndex] = new sMesh;
								sMesh* pVertOnlyMesh = pVertOnlyMeshes[iFrameIndex];
								MakeMeshFromOtherMesh (true, pVertOnlyMesh, pMesh, NULL);
								ConvertLocalMeshToVertsOnly (pVertOnlyMesh, false);
								pWorkMesh = pVertOnlyMesh;
								#endif

								// create matrix to transform XYZ
								GGMATRIX matTemp;
								GGMATRIX matThisFrame;
								GGMatrixIdentity(&matThisFrame);
								// scale and rotation done when first added obj(as mesh) into monster object
								// scale
								//GGMatrixScaling(&matTemp, pFrame->vecScale.x, pFrame->vecScale.y, pFrame->vecScale.z);
								//GGMatrixMultiply(&matThisFrame, &matThisFrame, &matTemp);
								// rotation
								//GGMatrixRotationX(&matTemp, GGToRadian (pFrame->vecRotation.x));
								//GGMatrixMultiply(&matThisFrame, &matThisFrame, &matTemp);
								//GGMatrixRotationY(&matTemp, GGToRadian (pFrame->vecRotation.y));
								//GGMatrixMultiply(&matThisFrame, &matThisFrame, &matTemp);
								//GGMatrixRotationZ(&matTemp, GGToRadian (pFrame->vecRotation.z));
								//GGMatrixMultiply(&matThisFrame, &matThisFrame, &matTemp);
								// translation
								GGMatrixTranslation	(&matTemp, pFrame->vecOffset.x, pFrame->vecOffset.y, pFrame->vecOffset.z);
								GGMatrixMultiply(&matThisFrame, &matThisFrame, &matTemp);

								// for each vertex
								BYTE* pVertData = pWorkMesh->pVertexData;
								for (DWORD dwV = 0; dwV < pWorkMesh->dwVertexCount; dwV++)
								{
									GGVECTOR3 vecXYZ = GGVECTOR3((float)*((float*)pVertData + 0), (float)*((float*)pVertData + 1), (float)*((float*)pVertData + 2));
									GGVec3TransformCoord(&vecXYZ, &vecXYZ, &matThisFrame);
									sprintf (pDynLine, "v %f %f %f\n", vecXYZ.x, vecXYZ.y, vecXYZ.z);
									CWriteFile(hfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
									pVertData += pWorkMesh->dwFVFSize;
								}
								if (bCompactOBJ == false)
								{
									pVertData = pWorkMesh->pVertexData;
									for (DWORD dwV = 0; dwV < pWorkMesh->dwVertexCount; dwV++)
									{
										float fReverseVCoordForOBJ = -(float)*((float*)pVertData + 7);
										sprintf (pDynLine, "vt %f %f\n", (float)*((float*)pVertData + 6), fReverseVCoordForOBJ);
										CWriteFile(hfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
										pVertData += pWorkMesh->dwFVFSize;
									}
									pVertData = pWorkMesh->pVertexData;
									for (DWORD dwV = 0; dwV < pWorkMesh->dwVertexCount; dwV++)
									{
										sprintf (pDynLine, "vn %f %f %f\n", (float)*((float*)pVertData + 3), (float)*((float*)pVertData + 4), (float)*((float*)pVertData + 5));
										CWriteFile(hfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
										pVertData += pWorkMesh->dwFVFSize;
									}
								}
							}
						}
					}

					// faces
					pLine = "\n";
					CWriteFile(hfile, pLine, strlen(pLine), &byteswritten, NULL);
					pLine = "# Face list\n";
					CWriteFile(hfile, pLine, strlen(pLine), &byteswritten, NULL);
					
					// dump all face index data from all meshes into OBJ
					for (int iFrameIndex = 0; iFrameIndex < pObject->iFrameCount; iFrameIndex++)
					{
						sFrame* pFrame = pObject->ppFrameList[iFrameIndex];
						if (pFrame)
						{
							sMesh* pMesh = pFrame->pMesh;
							if (pMesh)
							{
								sMesh* pWorkMesh = NULL;
								#ifdef WICKEDENGINE
								pWorkMesh = pMesh;
								#else
								// verts only meshes created above
								sMesh* pVertOnlyMesh = pVertOnlyMeshes[iFrameIndex];
								pWorkMesh = pVertOnlyMesh;
								#endif

								// texture filename
								if (bCompactOBJ == false)
								{
									// no guarentee its in the X file - but we try first
									// else we use the name of the model file
									char pFileOnlyNoExt[512];
									for (int iTry = 0; iTry < 2; iTry++)
									{
										LPSTR pOrigPathAndFile = NULL;
										if (iTry == 0) pOrigPathAndFile = pMesh->pTextures[0].pName;
										if (iTry == 1) pOrigPathAndFile = pDBPFilename;
										strcpy (pFileOnlyNoExt, "");
										if (pOrigPathAndFile)
										{
											// trim off any paths first
											strcpy (pFileOnlyNoExt, pOrigPathAndFile);
											for (int n = strlen(pOrigPathAndFile) - 1; n > 0; n--)
											{
												if (pOrigPathAndFile[n] == '\\' || pOrigPathAndFile[n] == '/')
												{
													strcpy (pFileOnlyNoExt, pOrigPathAndFile + n + 1);
													n = 0; break;
												}
											}

											// now we see if any variations of this filename exists
											if (strlen(pFileOnlyNoExt) > 4)
											{
												pFileOnlyNoExt[strlen(pFileOnlyNoExt) - 4] = 0;
												strcat (pFileOnlyNoExt, ".png");
												HANDLE hExists = GG_CreateFile(pFileOnlyNoExt, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
												if (hExists != INVALID_HANDLE_VALUE)
												{
													// this texture file exists - we have our texture name
													CloseHandle(hExists);
													iTry = 99;
												}
												else
												{
													// the PNG of the named texture does not exist, but sometimes
													// texture names are truncated (chair_a.x) so need to be sliced
													// up to find which part of the end is the actual texture (up to 32 chars)
													char pSlicedVariant[512];
													for (int n = 32; n > 5; n--)
													{
														strcpy (pSlicedVariant, pFileOnlyNoExt + strlen(pFileOnlyNoExt) - n);
														hExists = GG_CreateFile(pSlicedVariant, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
														if (hExists != INVALID_HANDLE_VALUE)
														{
															// this texture file exists - we have our texture name
															strcpy (pFileOnlyNoExt, pSlicedVariant);
															CloseHandle(hExists);
															iTry = 99;
															n = 0;
														}
													}
												}
											}
										}
									}

									// remove spaces from material record
									char pNoSpacesFile[512];
									strcpy (pNoSpacesFile, pFileOnlyNoExt);
									for (DWORD n = 0; n < strlen(pNoSpacesFile); n++)
										if (pNoSpacesFile[n] == ' ') pNoSpacesFile[n] = '_';

									// write material(texture) for this collecion of faces(mesh)
									sprintf (pDynLine, "usemtl %s\n", pNoSpacesFile);
									CWriteFile(hfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);

									// also write this into the MTL file
									sprintf (pDynLine, "newmtl %s\n", pNoSpacesFile);
									WriteFile(hMTLfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
									sprintf (pDynLine, "    Ns 20\n");
									WriteFile(hMTLfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
									sprintf (pDynLine, "    d 1\n");
									WriteFile(hMTLfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
									sprintf (pDynLine, "    illum 2\n");
									WriteFile(hMTLfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
									sprintf (pDynLine, "    Kd 1.0 1.0 1.0\n");
									WriteFile(hMTLfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
									sprintf (pDynLine, "    Ks 0 0 0\n");
									WriteFile(hMTLfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
									sprintf (pDynLine, "    Ka 0 0 0\n");
									WriteFile(hMTLfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
									sprintf (pDynLine, "    map_Kd %s\n\n", pNoSpacesFile);
									WriteFile(hMTLfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);

									// for each face
									#ifdef WICKEDENGINE
									if (pWorkMesh->dwIndexCount == 0)
									{
										// has no indice data
										for (DWORD dwV = 0; dwV < pWorkMesh->dwVertexCount; dwV += 3)
										{
											int iA = dwV + dwStartOfVertexBatch + 0;
											int iB = dwV + dwStartOfVertexBatch + 1;
											int iC = dwV + dwStartOfVertexBatch + 2;
											sprintf (pDynLine, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", iA, iA, iA, iB, iB, iB, iC, iC, iC);
											CWriteFile(hfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
										}
									}
									else
									{
										// has indice data
										for (DWORD dwI = 0; dwI < pWorkMesh->dwIndexCount; dwI += 3)
										{
											DWORD dwV0 = pWorkMesh->pIndices[dwI + 0];
											DWORD dwV1 = pWorkMesh->pIndices[dwI + 1];
											DWORD dwV2 = pWorkMesh->pIndices[dwI + 2];
											int iA = dwV0 + dwStartOfVertexBatch;
											int iB = dwV1 + dwStartOfVertexBatch;
											int iC = dwV2 + dwStartOfVertexBatch;
											sprintf (pDynLine, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", iA, iA, iA, iB, iB, iB, iC, iC, iC);
											CWriteFile(hfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
										}
									}
									#else
									for (DWORD dwV = 0; dwV < pWorkMesh->dwVertexCount; dwV += 3)
									{
										int iA = dwV + dwStartOfVertexBatch + 0;
										int iB = dwV + dwStartOfVertexBatch + 1;
										int iC = dwV + dwStartOfVertexBatch + 2;
										sprintf (pDynLine, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", iA, iA, iA, iB, iB, iB, iC, iC, iC);
										CWriteFile(hfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
									}
									#endif
								}
								else
								{
									int iLimit = 9999999;
									#ifdef WICKEDENGINE
									if (pWorkMesh->dwIndexCount == 0)
									{
										// has no indice data
										for (DWORD dwV = 0; dwV < pWorkMesh->dwVertexCount; dwV += 3)
										{
											int iA = dwV + dwStartOfVertexBatch + 0;
											int iB = dwV + dwStartOfVertexBatch + 1;
											int iC = dwV + dwStartOfVertexBatch + 2;
											if (iA < iLimit && iB < iLimit && iC < iLimit)
											{
												sprintf (pDynLine, "f %d %d %d\n", iA, iB, iC);
												CWriteFile(hfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
											}
										}
									}
									else
									{
										// has indice data
										for (DWORD dwI = 0; dwI < pWorkMesh->dwIndexCount; dwI += 3)
										{
											DWORD dwV0 = pWorkMesh->pIndices[dwI + 0];
											DWORD dwV1 = pWorkMesh->pIndices[dwI + 1];
											DWORD dwV2 = pWorkMesh->pIndices[dwI + 2];
											int iA = dwV0 + dwStartOfVertexBatch;
											int iB = dwV1 + dwStartOfVertexBatch;
											int iC = dwV2 + dwStartOfVertexBatch;
											if (iA < iLimit && iB < iLimit && iC < iLimit)
											{
												sprintf (pDynLine, "f %d %d %d\n", iA, iB, iC);
												CWriteFile(hfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
											}
										}
									}
									#else
									// for each face - simple face index only
									for (DWORD dwV = 0; dwV < pWorkMesh->dwVertexCount; dwV += 3)
									{
										int iA = dwV + dwStartOfVertexBatch + 0;
										int iB = dwV + dwStartOfVertexBatch + 1;
										int iC = dwV + dwStartOfVertexBatch + 2;
										if (iA < iLimit && iB < iLimit && iC < iLimit)
										{
											sprintf (pDynLine, "f %d %d %d\n", iA, iB, iC);
											CWriteFile(hfile, pDynLine, strlen(pDynLine), &byteswritten, NULL);
										}
									}
									#endif
								}

								// Advance global start marker for vertice indices
								dwStartOfVertexBatch += pWorkMesh->dwVertexCount;

								// free temp mesh
								#ifndef WICKEDENGINE
								SAFE_DELETE (pWorkMesh);
								#endif
							}
						}
					}

					// End of file marker
					pLine = "\n";
					CWriteFile(hfile, pLine, strlen(pLine), &byteswritten, NULL);
					pLine = "# End of file\n";
					CWriteFile( hfile, pLine, strlen(pLine), &byteswritten, NULL ); 
					
					// finish file
					CCloseHandle ( hfile );
				}

				// finish file
				CloseHandle ( hMTLfile );
			}
		}
		else
		{
			// ensure filename uses DBO extension
			if ( strnicmp ( pDBPFilename + strlen(pDBPFilename) - 4, ".dbo", 4 )!=NULL )
			{
				RunTimeError ( RUNTIMEERROR_B3DMUSTUSEDBOEXTENSION );
				return;
			}

			// save the object as DBO
			if ( !SaveDBO ( pDBPFilename, pObject ) )
				return;
		}
	}
}

DARKSDK_DLL void SaveObject (LPSTR szFilename, int iID)
{
	SaveObjectEx (szFilename, iID, false);
}

DARKSDK_DLL void SetDeleteCallBack ( int iID, ON_OBJECT_DELETE_CALLBACK pfn, int userData )
{
	// mike - 050803 - delete object override

	// ensure the object is present
	if ( !ConfirmObject ( iID ) )
		return;

	if ( g_ObjectList [ iID ]->iDeleteCount == 0 )
	{
		g_ObjectList [ iID ]->iDeleteCount += 25;
		g_ObjectList [ iID ]->pDelete       = new sObject::sDelete [ g_ObjectList [ iID ]->iDeleteCount ];
	}
	
	if ( g_ObjectList [ iID ]->iDeleteID < g_ObjectList [ iID ]->iDeleteCount )
	{
		g_ObjectList [ iID ]->pDelete [ g_ObjectList [ iID ]->iDeleteID ].onDelete = pfn;
		g_ObjectList [ iID ]->pDelete [ g_ObjectList [ iID ]->iDeleteID ].userData = userData;	

		g_ObjectList [ iID ]->iDeleteID++;
	}

	//g_ObjectList [ iID ]->onDelete = pfn;
	//g_ObjectList [ iID ]->userData = userData;
}

DARKSDK_DLL void SetDisableTransform ( int iID, bool bTransform )
{
	// mike - 050803 - can stop DB Pro transforming an object

	// ensure the object is present
	if ( !ConfirmObject ( iID ) )
		return;

	g_ObjectList [ iID ]->bDisableTransform = bTransform;
}

DARKSDK_DLL void CreateMeshForObject ( int iID, DWORD dwFVF, DWORD dwVertexCount, DWORD dwIndexCount )
{
	// mike - 050803 - create a new mesh for an object

	// ensure the object is present
	if ( !ConfirmObject ( iID ) )
		return;

	sObject* pObject = g_ObjectList [ iID ];

	if ( pObject->pFrame )
		SAFE_DELETE ( pObject->pFrame );
	
	pObject->pFrame = new sFrame;

	if ( !pObject->pFrame )
	{
		RunTimeError ( RUNTIMEERROR_B3DMESHLOADFAILED );
		return;
	}

	pObject->pFrame->pMesh = new sMesh;

	if ( !pObject->pFrame->pMesh )
	{
		RunTimeError ( RUNTIMEERROR_B3DMESHLOADFAILED );
		return;
	}
	
	if ( !SetupMeshFVFData ( pObject->pFrame->pMesh, dwFVF, dwVertexCount, dwIndexCount, false ) )
	{
		RunTimeError ( RUNTIMEERROR_B3DMESHLOADFAILED );
		return;
	}

	pObject->pFrame->pMesh->iPrimitiveType   = GGPT_TRIANGLELIST;
	pObject->pFrame->pMesh->iDrawVertexCount = pObject->pFrame->pMesh->dwVertexCount;
	pObject->pFrame->pMesh->iDrawPrimitives  = pObject->pFrame->pMesh->dwIndexCount / 3;

	SetNewObjectFinalProperties ( iID, 100 );

	// setup new object and introduce to buffers
	//SetNewObjectFinalProperties ( iID, (100.0f)/2 );

	// box collision for box shapes
	SetColToBoxes ( g_ObjectList [ iID ] );

	// give the object a default texture
	TextureObject ( iID, 0 );
}

DARKSDK_DLL void DeleteObject ( int iID )
{
	// mike - 101005 - excluded objects could not previously by delete
	if ( !CheckObjectExist ( iID ) )
		return;

	// get object ptr
	sObject* pObject = g_ObjectList [ iID ];
	bool bObjectusedUniqueNotSharedBuffers = pObject->bUsesItsOwnBuffers;

	// ensure delete calls ondelete code
	for ( int i = 0; i < g_ObjectList [ iID ]->iDeleteID; i++ )
	{
		if ( pObject->pDelete [ i ].onDelete )
		{
			pObject->pDelete [ i ].onDelete ( iID, pObject->pDelete [ i ].userData );
		}
	}

	// delete object
	DeleteObjectSpecial ( iID );

	// leespeed - 140307 - if the object only used unique VBIB buffers, it did not share any VB IB
	// which means there will be nothing to add back in, so we can skip this step
	if ( bObjectusedUniqueNotSharedBuffers==false )
	{
		m_ObjectManager.AddFlaggedObjectsBackToBuffers ();
	}

	// update
	m_ObjectManager.UpdateTextures();
}

DARKSDK_DLL void DeleteObjects ( int iFrom, int iTo )
{
	// some simple checks
	if ( iTo==0 || iFrom==0 ) return;
	if ( iTo<iFrom ) return;

	// delete multiple objects
	for ( int iID=iFrom; iID<=iTo; iID++ )
	{
		// ensure it exists first
		if ( !CheckObjectExist ( iID ) )
			continue;

		// get object ptr
		sObject* pObject = g_ObjectList [ iID ];
		if ( pObject )
		{
			// ensure delete calls ondelete code
			for ( int i = 0; i < g_ObjectList [ iID ]->iDeleteID; i++ )
			{
				if ( pObject->pDelete [ i ].onDelete )
				{
					pObject->pDelete [ i ].onDelete ( iID, pObject->pDelete [ i ].userData );
				}
			}

			// delete object
			DeleteObjectSpecial ( iID );
		}
	}

	// upon buffer removal, some object where flagged for re-creation
	m_ObjectManager.AddFlaggedObjectsBackToBuffers ();

	// update texture list when introduce new object(s)
	m_ObjectManager.UpdateTextures();
}

DARKSDK_DLL void ClearObjectsOfTextureRef ( LPGGTEXTURE pTextureRef )
{
	// go through all objects being managed and remove texture ref
	m_ObjectManager.RemoveTextureRefFromAllObjects ( pTextureRef );
}

DARKSDK_DLL void SetObject ( int iID, SDK_BOOL bWireframe, int iTransparency, SDK_BOOL bCull )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// new transparency mode
	SDK_BOOL bTransparency=TRUE;
	if ( iTransparency==0 ) bTransparency=FALSE;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
	{
		SetWireframe	( pObject->ppMeshList [ iMesh ], bWireframe==FALSE		);
		SetTransparency	( pObject->ppMeshList [ iMesh ], bTransparency==TRUE	);
		SetCull			( pObject->ppMeshList [ iMesh ], bCull==TRUE			);
	}

	// apply transparency as object overlay
	SetObjectTransparency ( pObject, iTransparency );
}

DARKSDK_DLL void SetObject ( int iID, SDK_BOOL bWireframe, int iTransparency, SDK_BOOL bCull, int iFilter )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// new transparency mode
	SDK_BOOL bTransparency=TRUE;
	if ( iTransparency==0 ) bTransparency=FALSE;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
	{
		//SetWireframe	( pObject->ppMeshList [ iMesh ], bWireframe==TRUE		);
		// mike - 011005 - state must be false
		SetWireframe	( pObject->ppMeshList [ iMesh ], bWireframe==FALSE		);
		SetTransparency	( pObject->ppMeshList [ iMesh ], bTransparency==TRUE	);
		SetCull			( pObject->ppMeshList [ iMesh ], bCull==TRUE			);
		SetFilter		( pObject->ppMeshList [ iMesh ], iFilter				);
	}

	// apply transparency as object overlay
	SetObjectTransparency ( pObject, iTransparency );
}

DARKSDK_DLL void SetObject ( int iID, SDK_BOOL bWireframe, int iTransparency, SDK_BOOL bCull, int iFilter, SDK_BOOL bLight )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// new transparency mode
	SDK_BOOL bTransparency=TRUE;
	if ( iTransparency==0 ) bTransparency=FALSE;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
	{
		SetWireframe	( pObject->ppMeshList [ iMesh ], bWireframe==FALSE		);
		SetTransparency	( pObject->ppMeshList [ iMesh ], bTransparency==TRUE	);
		SetCull			( pObject->ppMeshList [ iMesh ], bCull==TRUE			);
		SetFilter		( pObject->ppMeshList [ iMesh ], iFilter				);
		SetLight		( pObject->ppMeshList [ iMesh ], bLight==TRUE			);
	}

	// apply transparency as object overlay
	SetObjectTransparency ( pObject, iTransparency );
}

DARKSDK_DLL void SetObject ( int iID, SDK_BOOL bWireframe, int iTransparency, SDK_BOOL bCull, int iFilter, SDK_BOOL bLight, SDK_BOOL bFog )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// new transparency mode
	SDK_BOOL bTransparency=TRUE;
	if ( iTransparency==0 ) bTransparency=FALSE;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
	{
		SetWireframe	( pObject->ppMeshList [ iMesh ], bWireframe==FALSE		);
		SetTransparency	( pObject->ppMeshList [ iMesh ], bTransparency==TRUE	);
		SetCull			( pObject->ppMeshList [ iMesh ], bCull==TRUE			);
		SetFilter		( pObject->ppMeshList [ iMesh ], iFilter				);
		SetLight		( pObject->ppMeshList [ iMesh ], bLight==TRUE			);
		SetFog			( pObject->ppMeshList [ iMesh ], bFog==TRUE				);
	}

	// apply transparency as object overlay
	SetObjectTransparency ( pObject, iTransparency );
}

DARKSDK_DLL void SetObject ( int iID, SDK_BOOL bWireframe, int iTransparency, SDK_BOOL bCull, int iFilter, SDK_BOOL bLight, SDK_BOOL bFog, SDK_BOOL bAmbient )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// new transparency mode
	SDK_BOOL bTransparency=TRUE;
	if ( iTransparency==0 ) bTransparency=FALSE;
	if ( iTransparency==7 ) bTransparency=FALSE;  // U75 - 051209 - Deal with early-rendered objects correctly

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
	{
		SetWireframe	( pObject->ppMeshList [ iMesh ], bWireframe==FALSE		);
		SetTransparency	( pObject->ppMeshList [ iMesh ], bTransparency==TRUE	);
		SetCull			( pObject->ppMeshList [ iMesh ], bCull==TRUE			);
		SetFilter		( pObject->ppMeshList [ iMesh ], iFilter				);
		SetLight		( pObject->ppMeshList [ iMesh ], bLight==TRUE			);
		SetFog			( pObject->ppMeshList [ iMesh ], bFog==TRUE				);
		SetAmbient		( pObject->ppMeshList [ iMesh ], bAmbient==TRUE			);
	}

	// apply transparency as object overlay
	SetObjectTransparency ( pObject, iTransparency );
}

DARKSDK_DLL void SetObjectWireframe ( int iID, SDK_BOOL bWireframe )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
		SetWireframe ( pObject->ppMeshList [ iMesh ], bWireframe==TRUE );
}

DARKSDK_DLL void SetObjectTransparency ( int iID, int iTransparency )
{
	// Transparency Modes
	// 0 - first-phase no alpha
	// 1 - first-phase with alpha masking
	// 2 and 3 - second-phase which overlaps solid geometry
	// 4 - alpha test (only render beyond 0x000000CF alpha values)
	// 5 - water line object (seperates depth sort automatically)
	// 6 - combination of 3 and 4 (second phase render with alpha blend AND alpha test, used for fading LOD leaves)
	// 7 - very early draw phase no alpha
	// 8 - below water line , render before water.

	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// new transparency mode
	SDK_BOOL bTransparency=TRUE;
	if ( iTransparency==0 ) bTransparency=FALSE;
	if ( iTransparency==7 ) bTransparency=FALSE;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
	{
		SetTransparency ( pObject->ppMeshList [ iMesh ], bTransparency==TRUE );
		SetAlphaTest ( pObject->ppMeshList [ iMesh ], 0x0 ); 
		if ( iTransparency==4 || iTransparency==6 || iTransparency == 8 )
		{
			SetAlphaTest ( pObject->ppMeshList [ iMesh ], 0x000000CF );
		}
	}

	// apply transparency as object overlay
	SetObjectTransparency ( pObject, iTransparency );

	#ifdef WICKEDENGINE
	//PE: Set Transparent on wicked objects.
	WickedCall_SetObjectTransparent(pObject);
	#endif
}

DARKSDK_DLL void SetObjectCull ( int iID, SDK_BOOL bCull )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// lee - 040306 - u6rc5 - solve CW/CCW issue with some model imports
	int iCullMode = bCull;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
		SetCullCWCCW ( pObject->ppMeshList [ iMesh ], iCullMode );

	#ifdef WICKEDENGINE
	//PE: Set cullmode on wicked objects.
	WickedCall_SetObjectCullmode(pObject);
	#endif
}

DARKSDK_DLL void SetLimbCull ( int iID, int iLimbIndex, SDK_BOOL bCull )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbIndex ) )
		return;

	// solve CW/CCW issue with some model imports
	int iCullMode = bCull;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	sFrame* pFrame = pObject->ppFrameList[iLimbIndex];
	if ( pFrame ) 
		if ( pFrame->pMesh )
			SetCullCWCCW ( pFrame->pMesh, iCullMode );

#ifdef WICKEDENGINE
	//PE: Set cullmode on wicked objects.
	WickedCall_SetObjectCullmode(pObject);
#endif

}

DARKSDK_DLL void SetObjectFilterStage ( int iID, int iStage, int iFilter )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
		SetFilter ( pObject->ppMeshList [ iMesh ], iStage, iFilter );
}

DARKSDK_DLL void SetObjectFilter ( int iID, int iFilter )
{
	// iFilter
	// D3DTEXF_POINT = 1 (use 0)
	// GGTEXF_LINEAR = 2 (use 1)
	// D3DTEXF_ANISOTROPIC = 3 (use 2)
	// D3DTEXF_PYRAMIDALQUAD = 6 (use 5)
	// D3DTEXF_GAUSSIANQUAD = 7 (use 6)
	// When assigned, MeshCPP (iFilter++)
	SetObjectFilterStage ( iID, 0, iFilter );
}

DARKSDK_DLL void SetObjectLight ( int iID, SDK_BOOL bLight )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
		SetLight ( pObject->ppMeshList [ iMesh ], bLight==TRUE );
}

DARKSDK_DLL void SetObjectFog ( int iID, SDK_BOOL bFog )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
		SetFog ( pObject->ppMeshList [ iMesh ], bFog==TRUE );
}

DARKSDK_DLL void SetObjectAmbient ( int iID, SDK_BOOL bAmbient )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
		SetAmbient ( pObject->ppMeshList [ iMesh ], bAmbient==TRUE );
}

DARKSDK_DLL void SetObjectRenderMatrixMode ( int iID, int iRenderMatrixMode )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// apply setting for Render Matrix Mode
	sObject* pObject = g_ObjectList [ iID ];
	if ( iRenderMatrixMode==1 )
		pObject->dwApplyOriginalScaling = 1;
	else
		pObject->dwApplyOriginalScaling = 0;
}

DARKSDK_DLL void SetObjectMask ( int iID, int iMASK )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	pObject->dwCameraMaskBits = (DWORD)iMASK; // u63 - 0-30 camera bits in mask
}

DARKSDK_DLL void AddObjectMask ( int iID, DWORD dwAddMASK )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	pObject->dwCameraMaskBits |= dwAddMASK;
}

/*
DARKSDK_DLL int RgbR ( DWORD iRGB )
{
	return (int)((iRGB & 0x00FF0000) >> 16);
}

DARKSDK_DLL int RgbG ( DWORD iRGB )
{
	return (int)((iRGB & 0x0000FF00) >> 8);
}

DARKSDK_DLL int RgbB ( DWORD iRGB )
{
	return (int)((iRGB & 0x000000FF) );
}
*/

DARKSDK_DLL void ColorObject ( int iID, DWORD dwRGB )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
	{
		SetDiffuse ( pObject->ppMeshList [ iMesh ], 1.0f );
		SetBaseColor ( pObject->ppMeshList [ iMesh ], dwRGB );
	}

	// trigger a re-new and re-sort
	m_ObjectManager.RenewReplacedMeshes ( pObject );
	m_ObjectManager.UpdateTextures ( );
}

DARKSDK_DLL void SetObjectDiffuseEx ( int iID, DWORD dwRGB, int iMaterialOrVertexData )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// object ptr
	sObject* pObject = g_ObjectList [ iID ];

	// lee - 240206 - u60 - if object is instance, use custom-data-slot to store diffuse colour
	sObject* pActualObject = pObject->pInstanceOfObject;
	if ( pActualObject )
	{
		// only if have at least one mesh
		if ( pActualObject->iMeshCount>0 )
		{
			// mesh ptr
			sMesh* pMesh = pActualObject->ppMeshList [ 0 ];

			// this object is an instance
			SetObjectStatisticsInteger(iID,0,dwRGB);
			if ( pObject->dwCustomSize==0 )
			{
				// create custom slot
				DWORD dwStatisticsDataSize = 8;
				pObject->dwCustomSize = dwStatisticsDataSize*-1;
				pObject->pCustomData = (LPVOID)new DWORD[dwStatisticsDataSize];
				for ( DWORD i=0; i<dwStatisticsDataSize; i++ )
					*(((DWORD*)pObject->pCustomData)+i) = 0;
			}
			if ( pObject->dwCustomSize>4000000000 )
			{
				// set diffuse colour
				*(((DWORD*)pObject->pCustomData)+0) = dwRGB;
			}
		}
	}
	else
	{
		// apply setting to all meshes
		for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
		{
			// lee - 240206 - u60 - do both to ensure diffuse is written (SetDiffuseEx added)
			if ( iMaterialOrVertexData==1 )
				SetDiffuseEx ( pObject->ppMeshList [ iMesh ], dwRGB );
			else
				SetDiffuseMaterial ( pObject->ppMeshList [ iMesh ], dwRGB );
		}
	}
}

DARKSDK_DLL void SetObjectDiffuse ( int iID, DWORD dwRGB )
{
	// see above
	SetObjectDiffuseEx ( iID, dwRGB, 0 );
}

DARKSDK_DLL void SetObjectAmbience ( int iID, DWORD dwRGB )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
		SetAmbienceMaterial ( pObject->ppMeshList [ iMesh ], dwRGB );
}

DARKSDK_DLL void SetObjectSpecular ( int iID, DWORD dwRGB )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
		SetSpecularMaterial ( pObject->ppMeshList [ iMesh ], dwRGB );
}

DARKSDK_DLL void SetObjectSpecularPower ( int iID, float fPower )
{
	/* not used in MAX
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
		SetSpecularPower ( pObject->ppMeshList [ iMesh ], fPower );
	*/
}

DARKSDK_DLL void SetObjectScrollScaleUV ( int iID, float fScrU, float fScrV, float fScaU, float fScaV )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
	{
		sMesh* pMesh = pObject->ppMeshList [ iMesh ];
		if ( pMesh )
		{
			pMesh->fScrollOffsetU = fScrU;
			pMesh->fScrollOffsetV = fScrV;
			pMesh->fScaleOffsetU = fScaU;
			pMesh->fScaleOffsetV = fScaV;
		}
	}
}

DARKSDK_DLL void SetObjectArtFlags ( int iID, DWORD dwArtFlags, float fBoostIntensity )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// apply setting to all meshes (or parent if just instance)
	sObject* pObject = g_ObjectList [ iID ];
	if ( pObject->pInstanceOfObject ) pObject = pObject->pInstanceOfObject;
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
	{
		sMesh* pMesh = pObject->ppMeshList [ iMesh ];
		if ( pMesh )
		{
			pMesh->dwArtFlags = dwArtFlags;
			pMesh->fBoostIntensity = fBoostIntensity;
		}
	}
}

DARKSDK_DLL void SetObjectSpecular ( int iID, DWORD dwRGB, float fPower )
{
	// U73 - 230309 - helper extra function parameter
	SetObjectSpecular ( iID, dwRGB );
	//SetObjectSpecularPower ( iID, fPower ); not used in MAX
}

DARKSDK_DLL void SetObjectEmissive ( int iID, DWORD dwRGB )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
		SetEmissiveMaterial ( pObject->ppMeshList [ iMesh ], dwRGB );
}

DARKSDK_DLL void SetObjectArbitaryValue ( int iID, DWORD dwArbValue )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
		pObject->ppMeshList [ iMesh ]->Collision.dwArbitaryValue = dwArbValue;
}

DARKSDK_DLL void MakeObject ( int iID, int iMeshID, int iImageID )
{
	// check the mesh exists
	if ( !ConfirmMesh ( iMeshID ) )
		return;

	// attempt to create a new object
	if ( !CreateNewObject ( iID, "mesh" ) )
		return;

	// no transform of new limb
	GGMATRIX matWorld;
	GGMatrixIdentity ( &matWorld );

	// setup general object data
	sMesh* pMesh = g_ObjectList [ iID ]->pFrame->pMesh;
	MakeMeshFromOtherMesh ( true, pMesh, g_RawMeshList [ iMeshID ], &matWorld );

	// setup new object and introduce to buffers
	SetNewObjectFinalProperties ( iID, -1.0f );

	// give the object a texture (optional)
	if ( iImageID==-1 )
	{
		pMesh->dwTextureCount = g_RawMeshList [ iMeshID ]->dwTextureCount; 
		pMesh->pTextures = new sTexture [ pMesh->dwTextureCount ]; 
		CloneInternalTextures ( pMesh, g_RawMeshList [ iMeshID ] );
	}
	else
		TextureObject ( iID, iImageID );
}

// Recursive Support Function (move to DBO when solid)
sFrame* MakeObjectFromLimbRec ( sFrame* pCurrentFrameToCopy, sFrame* pDstParentFrame )
{
	sFrame* pDstFrameRoot = NULL;
	sFrame* pDstFrame = NULL;
	while ( pCurrentFrameToCopy )
	{
		// frame hierarchy being created
		sFrame* pThisDstFrame = new sFrame;

		// first frame is root frame
		if ( pDstFrameRoot==NULL )
		{
			// first frame of list holds parent ref
			pDstFrameRoot = pThisDstFrame;
			pDstFrameRoot->pParent = pDstParentFrame;
		}

		// last dst frame is sybling to this new one
		if ( pDstFrame ) pDstFrame->pSibling = pThisDstFrame;

		// new dst current frame for copy
		pDstFrame = pThisDstFrame;

		// copy src frame data to dst frame data
		memcpy ( pDstFrame, pCurrentFrameToCopy, sizeof(sFrame) );
		pDstFrame->pChild = NULL;
		pDstFrame->pSibling = NULL;
		pDstFrame->pMesh = NULL;

		// go into children of this frame
		pDstFrame->pChild = MakeObjectFromLimbRec ( pCurrentFrameToCopy->pChild, pDstFrame );

		// next sybling
		pCurrentFrameToCopy = pCurrentFrameToCopy->pSibling;
	}

	// return created and filled frame
	return pDstFrameRoot;
}

