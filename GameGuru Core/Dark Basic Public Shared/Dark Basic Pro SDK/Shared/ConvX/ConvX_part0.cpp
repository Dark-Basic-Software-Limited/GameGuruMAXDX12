#include "directx-macros.h"
#include "CFileC.h"

/*
#ifdef DX11

// Prototype function for later linking
bool DBOLoadBlockFile ( LPSTR pFilename, int* ppBlock, DWORD* pdwSize );

// DX11 should call a DX9 compiled silent app to convert X to DBO files (no DirectX file support in DX11)
void SetLegacyModeOn ( void )
{
}
void SetLegacyModeOff ( void )
{
}
bool ConvXConvert ( LPSTR pFilename, void** pBlock, DWORD* pdwSize )
{
	// 020917 - only if converter exists
	if ( FileExist ( "..\\ConvertXtoDBO\\ConvertXtoDBO.exe" ) == 1 )
	{
		// use external module to convert X to DBO file
		char pExt[3];
		char pLineDBO[1024];
		DWORD dwFilenameIndex = strlen(pFilename)-2;
		pExt[0] = pFilename[dwFilenameIndex+0];
		pExt[1] = pFilename[dwFilenameIndex+1];
		pExt[2] = 0;
		if ( stricmp ( pExt, ".x" ) == NULL )
		{
			// create DBO from X (if not exist)
			strcpy ( pLineDBO, pFilename );
			pLineDBO[strlen(pLineDBO)-2] = 0;
			strcat ( pLineDBO, ".dbo" );
			if ( FileExist ( pLineDBO ) == 0 )
			{
				STARTUPINFO si;
				ZeroMemory( &si, sizeof(si) );
				si.cb = sizeof(si);
				si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
				si.wShowWindow = SW_HIDE;
				si.lpTitle ="my_process_console";

				PROCESS_INFORMATION pi;
				ZeroMemory( &pi, sizeof(pi) );
				char pLine[512];
				strcpy ( pLine, "..\\ConvertXtoDBO\\ConvertXtoDBO.exe " );
				strcat ( pLine, pFilename );
				if( !CreateProcess( NULL,
					pLine,      
					NULL,           // Process handle not inheritable
					NULL,           // Thread handle not inheritable
					TRUE,          // Set handle inheritance to FALSE
					CREATE_NO_WINDOW, // No creation flags
					NULL,           // Use parent's environment block
					NULL,           // Use parent's starting directory 
					&si,            // Pointer to STARTUPINFO structure
					&pi )           // Pointer to PROCESS_INFORMATION structure
				) 
				{
					//printf( "CreateProcess failed (%d).\n", GetLastError() );
					return false;
				}
				HWND console_name =FindWindow(NULL,"my_process_console");
				if(console_name)
				{
				  ShowWindow(console_name,SW_HIDE);
				}
				WaitForSingleObject( pi.hProcess, INFINITE );
				CloseHandle( pi.hProcess );
				CloseHandle( pi.hThread );
			}
		}
		else
			strcpy ( pLineDBO, pFilename );

		// load DBO directly
		if ( FileExist ( pLineDBO ) == 1 )
		{
			if ( DBOLoadBlockFile ( pLineDBO, pBlock, pdwSize ) )
				return true;
			else
				return false;
		}
	}
	else
		return false;
}
void ConvXFree ( LPSTR pBlock )
{
}

#else
*/

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
_CrtMemState g_crt_mem_state;
#include "ConvX.h"
#include "CGfxC.h"
#include <initguid.h>
#include "dxfile.h"
#include "D3D9Types.h"
#include "D3dx9mesh.h"

#define IMPL_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)\
EXTERN_C const GUID DECLSPEC_SELECTANY name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
IMPL_GUID(DBIID_IDirectXFileData,           0x3d82ab44, 0x62da, 0x11cf, 0xab, 0x39, 0x0, 0x20, 0xaf, 0x71, 0xe4, 0x33);
IMPL_GUID(DBIID_IDirectXFileDataReference,  0x3d82ab45, 0x62da, 0x11cf, 0xab, 0x39, 0x0, 0x20, 0xaf, 0x71, 0xe4, 0x33);
/*
DEFINE_GUID(DXFILEOBJ_XSkinMeshHeader,
0x3cf169ce, 0xff7c, 0x44ab, 0x93, 0xc0, 0xf7, 0x8f, 0x62, 0xd1, 0x72, 0xe2);
DEFINE_GUID(DXFILEOBJ_SkinWeights, 
0x6f0d123b, 0xbad2, 0x4167, 0xa0, 0xd0, 0x80, 0x22, 0x4f, 0x25, 0xfa, 0xbb);
DEFINE_GUID(DXFILEOBJ_FVFData, 
0xb6e70a0e, 0x8ef9, 0x4e83, 0x94, 0xad, 0xec, 0xc8, 0xb0, 0xc0, 0x48, 0x97);
// 040908 - added for U70 to support TS76 new X file export (and others)
DEFINE_GUID(DXFILEOBJ_DeclData, 
0xbf22e553, 0x292c, 0x4781, 0x9f, 0xea, 0x62, 0xbd, 0x55, 0x4b, 0xdd, 0x93);
DEFINE_GUID(DXFILEOBJ_VertexElement, 
0xf752461c, 0x1e23, 0x48f6, 0xb9, 0xf8, 0x83, 0x50, 0x85, 0x0f, 0x33, 0x6f);
*/

#include "rmxftmpl.h"
#include "rmxfguid.h"	

#include < vector >
using namespace std;

struct sNewVertex
{
	// to store vertex information

	GGVECTOR3 vecPosition;
	GGVECTOR3 vecNormal;
	float		tu, tv;
	float		tu2, tv2; //leeadded - 261105 - for loading second set of UV data
	float		tu3, tv3; //leeadded - 120106 - for loading third set of UV data
};

int g_iLegacyModeIgnoresFVFDATA = 0; // U75 - default is no legacy mode (set by DLL function call)
int g_iNumberOfExtraUVDataStagesAvailable = 0;

struct sNewBones
{
	// bone data

    char*		szName;
    DWORD		dwNumWeights;
    DWORD*		pIndices;
    float*		pfWeights;
    GGMATRIX  matrix;
};

struct sNewMaterial
{
	// material information

	GGVECTOR4 vecColour;
	float		fPower;
	GGVECTOR3 vecSpecular;
	GGVECTOR3 vecEmissive;
	char*		szFilename;
};

extern LPGGDEVICE							m_pD3D;
DBPRO_GLOBAL sObject*						g_pObjectX					= NULL;
DBPRO_GLOBAL sObject*						g_pObjectXFinal   			= NULL;
extern GlobStruct*							g_pGlob;
DBPRO_GLOBAL sNewVertex*					g_pVertexList				= NULL;			// vertices
DBPRO_GLOBAL DWORD*							g_pIndexList				= NULL;			// indices
DBPRO_GLOBAL GGVECTOR3*						g_pNormalList				= NULL;			// indices
DBPRO_GLOBAL DWORD*							g_pNormalIndexList			= NULL;			// indices
DBPRO_GLOBAL DWORD*							g_pFaceIndexOldRef			= NULL;			// store original indexes (if quads or greater processed)
DBPRO_GLOBAL sVertexColor*					g_pVertexColorList			= NULL;			// vertice colors
DBPRO_GLOBAL DWORD							g_dwVertexCount				= 0;			// number of vertices
DBPRO_GLOBAL DWORD							g_dwFaceCount				= 0;			// number of indices
DBPRO_GLOBAL DWORD							g_dwNormalFaceCount			= 0;
DBPRO_GLOBAL DWORD							g_dwNormalCount				= 0;			// number of normals ( can be greater than vertices)
DBPRO_GLOBAL DWORD							g_dwVertexColorCount		= 0;
DBPRO_GLOBAL DWORD*							g_pMaterialList				= NULL;			// material list
DBPRO_GLOBAL DWORD							g_dwMaterialListIndexCount	= 0;			// number of indices in material list
DBPRO_GLOBAL DWORD							g_dwMaterialCount			= 0;			// number of materials
DBPRO_GLOBAL WORD							g_dMaxSkinWeightsPerVertex	= 0;			// vertex skin weights
DBPRO_GLOBAL WORD							g_dMaxSkinWeightsPerFace	= 0;			// face skin weights
DBPRO_GLOBAL DWORD							g_dwBones					= 0;			// bone count
DBPRO_GLOBAL bool							g_bDisableBones				= false;		// disable bones if needed
DBPRO_GLOBAL vector < sNewBones    >		g_BoneList;									// list of bones
DBPRO_GLOBAL vector < sNewMaterial >		g_NewMaterialList;							// list of materials

DARKSDK bool ConvXLoad ( char* szFilename )
{
	// load an X model which will get converted to the DBO format

	// free existing object core
	ConvXDelete();

	// create new object core
	g_pObjectX = new sObject;

	// check the filename and D3D device are valid
	SAFE_MEMORY ( g_pObjectX );
	SAFE_MEMORY ( szFilename );
	SAFE_MEMORY ( m_pD3D );

	// allocate a new frame for the object
	g_pObjectX->pFrame = new sFrame;

	// ensure creation successful
	SAFE_MEMORY ( g_pObjectX->pFrame );

	// because of the way in which an X file is parsed we have to run
	// 2 passes, the first will build the frame list with mesh information
	// and the second pass will retrieve the animation information
	if ( !ConvXLoadModelData ( szFilename, g_pObjectX->pFrame, false ) )
		return false;

	if ( !ConvXLoadModelData ( szFilename, g_pObjectX->pFrame, true ) )
		return false;

	// setup generic properties
	SetupObjectsGenericProperties ( g_pObjectX );

	// produce sets of indice-table by material
	if ( !ProduceMaterialIndiceTables ( ) )
		return false;

	// mike - 070804 - need to reset these values
	g_dwNormalFaceCount = 0;
	g_dwNormalCount = 0;

	// object must have mesh and frame list
	// 010818 - but can have no mehses (for DBP that carry just animation data)
	if ( g_pObjectX->ppFrameList ) //&& g_pObjectX->ppMeshList )
	{
		// everything went okay
		return true;
	}
	else
	{
		// failed to create the object
		return false;
	}
}

DARKSDK bool CreateMeshInFrame ( sFrame* pFrame, DWORD dwNumVertices, DWORD dwNumFaces )
{
	// check we have a valid frame
	SAFE_MEMORY ( pFrame );

	// ceate a new mesh to hold the data
	pFrame->pMesh = new sMesh;

	// ensure the mesh was created
	SAFE_MEMORY ( pFrame->pMesh );

	// setup mesh properties and allocate memory
	pFrame->pMesh->dwVertexCount	= dwNumVertices;
	pFrame->pMesh->dwIndexCount		= dwNumFaces * 3;
	pFrame->pMesh->pVertexData		= (BYTE*)new sVertex [ pFrame->pMesh->dwVertexCount ];	// allocate vertex memory
	pFrame->pMesh->pIndices			= new WORD    [ pFrame->pMesh->dwIndexCount  ];			// allocate index memory
	pFrame->pMesh->dwFVF			= GGFVF_XYZ | GGFVF_NORMAL | GGFVF_TEX1;
	pFrame->pMesh->dwFVFSize		= 32;

	// setup mesh drawing properties
	pFrame->pMesh->iPrimitiveType	= GGPT_TRIANGLELIST;
	pFrame->pMesh->iDrawVertexCount	= dwNumVertices;
	pFrame->pMesh->iDrawPrimitives	= dwNumFaces;

	// check the newly created memory is okay
	SAFE_MEMORY ( pFrame->pMesh->pVertexData );
	SAFE_MEMORY ( pFrame->pMesh->pIndices  );

	// clear out newly created memory
	memset ( pFrame->pMesh->pVertexData, 0, sizeof ( sVertex ) * pFrame->pMesh->dwVertexCount );
	memset ( pFrame->pMesh->pIndices,    0, sizeof ( WORD    ) * pFrame->pMesh->dwIndexCount  );

	return true;
}

DARKSDK bool CheckMeshIfItNeedsCuttingUp ( sMesh* pMesh )
{
	// no need to dice material if only 0 or 1 texture
	if ( pMesh->dwTextureCount <= 1 )
	{
		// success - dont need material bank for splitting no more
		SAFE_DELETE(pMesh->pMaterialBank);
		return false;
	}

	// make sure the textures within the mesh are actually different, in some cases
	// we can simply discard all but one of the textures and return
	bool bSame = true;
	bool bTextureFilesPresent = false;
	for ( int iTextureA = 0; iTextureA < (int)pMesh->dwTextureCount; iTextureA++ )
	{
		char* szCurrent = pMesh->pTextures [ iTextureA ].pName;
		if ( strlen ( szCurrent ) > 0 )
		{
			bTextureFilesPresent = true;
		}
		for ( int iTextureB = 0; iTextureB < (int)pMesh->dwTextureCount; iTextureB++ )
		{
			if ( strcmp ( szCurrent, pMesh->pTextures [ iTextureB ].pName ) != 0 )
			{
				bSame = false;
				break;
			}
		}
		if ( !bSame )
			break;
	}

	// if all are the same, make as one texture
	if ( bSame && bTextureFilesPresent )
	{
		// new single texture in place of many textures

		// MIKE - 011103 - must allocate array instead of single item!
		sTexture* tex = new sTexture [ 1 ];
		memcpy ( tex, &pMesh->pTextures [ 0 ], sizeof ( sTexture ) );
		SAFE_DELETE_ARRAY ( pMesh->pTextures );
		pMesh->pTextures = tex;
		pMesh->dwTextureCount = 1;

		// success - dont need material bank for splutting no more
		SAFE_DELETE(pMesh->pMaterialBank);

		// only texture quantity changed
		return false;
	}

	// yes it needs cutting up
	return true;
}

DARKSDK bool ProduceMaterialIndiceOfFrame ( sObject* pObject, sFrame* pOriginalFrame )
{
	// pointer to mesh to replace (maybe)
	sMesh* pMesh = pOriginalFrame->pMesh;

	// check if needs cutting up
	if ( CheckMeshIfItNeedsCuttingUp ( pMesh )==false )
	{
		// does not need cutting up - return success
		return true;
	}

	// mike - 111005 - no need to do anything if we have no indices
	if ( pMesh->dwIndexCount == 0 )
		return true;

	// new index data array
	DWORD dwIndexCount = pMesh->dwIndexCount;
	WORD* pNewIndiceData = new WORD[dwIndexCount];
	WORD* pIndiceDataPtr = pNewIndiceData;

	// prepare multi-material settings
	DWORD dwPolyCounter=0;
	DWORD dwIndexCounter=0;
	pMesh->bUseMultiMaterial = true;
	pMesh->dwMultiMaterialCount = pMesh->dwTextureCount;
	pMesh->pMultiMaterial = new sMultiMaterial [ pMesh->dwMultiMaterialCount ];
	memset ( pMesh->pMultiMaterial, 0, sizeof(sMultiMaterial) * pMesh->dwMultiMaterialCount );

	// go through all textures and extract data
	for ( int iTextureIndex = 0; iTextureIndex < (int)pMesh->dwTextureCount; iTextureIndex++ )
	{
		// reset polycount
		dwPolyCounter=0;

		// index start position
		pMesh->pMultiMaterial [ iTextureIndex ].dwIndexStart = dwIndexCounter;

		// start adding to new index data in order of texture use in material sets
		for ( int iIndex = 0; iIndex < (int)pMesh->dwIndexCount; iIndex+=3 )
		{
			// work out which texture this mesh should use
			int iFaceIndex = iIndex / 3;
			int iTextureUsedHere = pMesh->pAttributeWorkData [ iFaceIndex ];

			// only process vertices that belong in this mesh
			if ( iTextureIndex == iTextureUsedHere )
			{
				*(pIndiceDataPtr++) = pMesh->pIndices [ iIndex+0 ];
				*(pIndiceDataPtr++) = pMesh->pIndices [ iIndex+1 ];
				*(pIndiceDataPtr++) = pMesh->pIndices [ iIndex+2 ];
				dwIndexCounter+=3;
				dwPolyCounter++;
			}
		}

		// fill multi-material structure
		strcpy ( pMesh->pMultiMaterial [ iTextureIndex ].pName, pMesh->pTextures [ iTextureIndex ].pName );
		memcpy ( &pMesh->pMultiMaterial [ iTextureIndex ].mMaterial, &pMesh->pMaterialBank [ iTextureIndex ].MatD3D, sizeof ( D3DMATERIAL9PRETEND ) );

		// index end and polycount
		pMesh->pMultiMaterial [ iTextureIndex ].dwIndexCount = dwPolyCounter*3;
		pMesh->pMultiMaterial [ iTextureIndex ].dwPolyCount = dwPolyCounter;
	}

	// delete old index data
	SAFE_DELETE(pMesh->pIndices);

	// set index array
	pMesh->pIndices = pNewIndiceData;

	// success
	return true;
}

DARKSDK bool ProduceMaterialIndiceTables ( void )
{
	// LEELEE : takes a solid mesh which has multiple textures and
	// produces a list of material sets, each describing what the texture and
	// material are, and the index data to draw them. This index data is stored
	// in a sorted single list, which the material sets reference (so it can be
	// placed in the index buffer as a single block replacing the old index block

	// check the object
	SAFE_MEMORY ( g_pObjectX );

	// go through all frames of object
	for ( int iFrame = 0; iFrame < g_pObjectX->iFrameCount; iFrame++ )
	{
		if ( g_pObjectX->ppFrameList [ iFrame ]->pMesh )
			ProduceMaterialIndiceOfFrame ( g_pObjectX, g_pObjectX->ppFrameList [ iFrame ] );
	}

	// success
	return true;
}

DARKSDK bool ConvXLoadModelData ( char* szFilename, sFrame* pFrame, bool bAnim )
{
	// loads the model data

	// check the parameters are valid
	SAFE_MEMORY ( szFilename );
	SAFE_MEMORY ( pFrame );

	// setup variables
	IDirectXFile*			pDXFile       = NULL;	// file interface
	IDirectXFileEnumObject* pDXEnum       = NULL;	// object interface
	IDirectXFileData*		pDXData       = NULL;	// data interface
	char*					szTexturePath = "";		// default texture path

	//
	// LEE, can use newer D3DXFileCreate (https://www.gamedev.net/forums/topic/662730-missing-d3dxofdll-in-windows-7-64-bit-directx-development/)
	// in order to have a 64-bit version of this X file loader - if AssImp fails to preserve DBP style X loading, we can use this as a plan B
	//

	// create the file object
	if ( FAILED ( DirectXFileCreate ( &pDXFile ) ) )
		return false;

	// register the templates from RM
	if ( FAILED ( pDXFile->RegisterTemplates ( ( LPVOID ) D3DRM_XTEMPLATES, D3DRM_XTEMPLATE_BYTES ) ) )
		return false;

//	// leeadd - 171003 - this registers manual templates stored in the xfile itself
//	HANDLE hfile = CreateFile ( szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
//	if ( hfile != INVALID_HANDLE_VALUE )
//	{
//		DWORD dwBufferSize = GetFileSize ( hfile, 0 );
//		LPSTR pBuffer = new char[dwBufferSize];
//		DWORD bytesread=0;
//		ReadFile(hfile, pBuffer, dwBufferSize, &bytesread, NULL );
//		pDXFile->RegisterTemplates ( ( LPVOID ) pBuffer, dwBufferSize );
//		SAFE_DELETE(pBuffer);
//		CloseHandle ( hfile );
//	}
//	else
//		return false;

	// mem leak dump check
//	_CrtMemDumpAllObjectsSince( &g_crt_mem_state );

	// snapshot
//	_CrtMemCheckpoint ( &g_crt_mem_state );

	char szRealFilename[ MAX_PATH ];
	strcpy_s( szRealFilename, MAX_PATH, szFilename );
	GG_GetRealPath( szRealFilename, 0 );

	// create an enumeration object
	if ( FAILED ( pDXFile->CreateEnumObject ( ( LPVOID ) szRealFilename, DXFILELOAD_FROMFILE, &pDXEnum ) ) )
		return false;

	// loop through all objects looking for the frames and meshes
	while ( SUCCEEDED ( pDXEnum->GetNextDataObject ( &pDXData ) ) )
	{
		// check the current block of data
		if ( !ParseXFileData ( pDXData, pFrame, szTexturePath, NULL, NULL, bAnim ) )
			return false;
		
		// release it
		SAFE_RELEASE ( pDXData );
	}

	// release dx handles
	SAFE_RELEASE ( pDXEnum );
	SAFE_RELEASE ( pDXFile );

	// return to caller
	return true;
}

DARKSDK bool ConvXDelete ( void )
{
	SAFE_DELETE ( g_pObjectX );

	SAFE_DELETE_ARRAY ( g_pVertexList   );
	SAFE_DELETE_ARRAY ( g_pIndexList    );
	SAFE_DELETE_ARRAY ( g_pVertexColorList );
	SAFE_DELETE_ARRAY ( g_pMaterialList );
		
	g_dwVertexCount            = 0;
	g_dwFaceCount              = 0;
	g_dwVertexColorCount       = 0;
	g_dwMaterialListIndexCount = 0;
	g_dwMaterialCount          = 0;
	g_dMaxSkinWeightsPerVertex = 0;
	g_dMaxSkinWeightsPerFace   = 0;
	g_dwBones                  = 0;

	// free resources in bone list
	for ( int i = 0; i < (int)g_BoneList.size(); i++ )
	{
		SAFE_DELETE ( g_BoneList [ i ].pfWeights );
		SAFE_DELETE ( g_BoneList [ i ].pIndices );
		SAFE_DELETE ( g_BoneList [ i ].szName );
	}
	// free resources in material list
	for ( int i = 0; i < (int)g_NewMaterialList.size(); i++ )
		SAFE_DELETE ( g_NewMaterialList [ i ].szFilename );

	// clear lists
	g_BoneList.clear        ( );
	g_NewMaterialList.clear ( );

	return true;
}

#ifdef DARKSDK_COMPILE

bool	Convert		( LPSTR pFilename, DWORD *pBlock, DWORD* pdwSize );
void	Free		( LPSTR );

bool ConvertX ( LPSTR pFilename, DWORD *pBlock, DWORD* pdwSize )
{
	return Convert ( pFilename, pBlock, pdwSize );
}

void FreeX ( LPSTR pBlock )
{
	Free ( pBlock );
}

#endif

DARKSDK void SetLegacyModeOn ( void )
{
	g_iLegacyModeIgnoresFVFDATA = 1;
}

DARKSDK void SetLegacyModeOff ( void )
{
	g_iLegacyModeIgnoresFVFDATA = 0;
}

DARKSDK bool ConvXConvert ( LPSTR pFilename, void** pBlock, DWORD* pdwSize )
{
	// load object
	if ( !ConvXLoad ( pFilename ) )
	{
		// Failed to load
		ConvXFree ( NULL );
		return false;
	}

	// create block
	if ( !DBOConvertObjectToBlock ( g_pObjectX, pBlock, pdwSize ) )
	{
		// Failed to create
		ConvXFree ( NULL );
		return false;
	}

	// okay
	return true;
}

DARKSDK void ConvXFree ( LPSTR pBlock )
{
	// delete block
	SAFE_DELETE(pBlock);

	// free object
	ConvXDelete();

	// free local DLL hook
	//if ( g_hSetup )
	//{
	//	FreeLibrary ( g_hSetup );
	//	g_hSetup = NULL;
	//}
}

DARKSDK bool XFILE_GetTemplateInfo ( IDirectXFileData* pDataObj, const GUID** guidType, char** szName )
{
	// gets the template info - identifier and name

	// make sure we have a valid DX file interface
	SAFE_MEMORY ( pDataObj );

	// set size to 0
	DWORD dwSize = 0;

	// get the template type
	if ( FAILED ( pDataObj->GetType ( guidType ) ) )
		return false;

	// get the template name ( if any )
	if ( FAILED ( pDataObj->GetName ( NULL, &dwSize ) ) )
		return false;

	if ( dwSize )
	{
		if ( szName )
		{
			// allocate memory and copy the name across
			*szName = new char [ dwSize ];

			// check the memory was created
			SAFE_MEMORY ( *szName );

			// finally get the name
			pDataObj->GetName ( *szName, &dwSize );
		}
	}
	else
	{
		if ( szName )
		{
			// if there isn't a name then give it a default
			*szName = new char [ 9 ];

			// check the memory
			SAFE_MEMORY ( *szName );

			// copy the default name across
			strcpy ( *szName, "$NoName$" );
		}
	}
	
	// all okay
	return true;
}

DARKSDK bool XFILE_CreateFrame ( sFrame** ppFrame, sFrame* pParentFrame, sFrame** ppSubFrame, char* szName )
{
	// create a new frame structure
	*ppFrame = new sFrame;

	SAFE_MEMORY ( *ppFrame );
	SAFE_MEMORY ( pParentFrame );
	SAFE_MEMORY ( *ppSubFrame );

	// store the name
	strcpy ( ( *ppFrame )->szName, szName );
	szName = NULL;

	// add to parent frame
	( *ppFrame )->pParent  = pParentFrame;
	( *ppFrame )->pSibling = pParentFrame->pChild;
	pParentFrame->pChild   = *ppFrame;

	// set sub frame parent
	*ppSubFrame = *ppFrame;
	
	return true;
}

DARKSDK bool XFILE_CreateTransformationMatrix ( IDirectXFileData* pDataObj, sFrame* pParentFrame )
{
	// get the frame transformation matrix

	// check parameters are valid
	SAFE_MEMORY ( pDataObj );
	SAFE_MEMORY ( pParentFrame );

	// setup a matrix and size variable
	GGMATRIX*	pFrameMatrix = NULL;
	DWORD		dwSize       = 0;

	// get the data from the X file interface
	if ( FAILED ( pDataObj->GetData ( NULL, &dwSize, ( PVOID* ) &pFrameMatrix ) ) )
		return false;

	// store the matrix in the parent frame
	pParentFrame->matOriginal = *pFrameMatrix;

	return true;
}

DARKSDK void XFILE_NEW_MakeTriangleIndicesFromFaceData ( DWORD* pdwFaceCount, DWORD** ppTriIndices, DWORD** ppOldRef, DWORD* pOriginalIndices )
{
	// if face data available
	if ( *pdwFaceCount )
	{
		// read index for original buffer
		DWORD readindex = 0;
		vector < DWORD > indices;
		DWORD dwNewFaceCount = 0;

		// go through original index data
		for ( DWORD faceindex = 0; faceindex < *pdwFaceCount; faceindex++ )
		{
			// this is number of indices per face
			int iIndicePerFace = pOriginalIndices [ readindex++ ];

			// only handle triangles and above
			if ( iIndicePerFace>=3 )
			{
				// store first two indexes of first basic triangle
				int readindexA = pOriginalIndices [ readindex+0 ];
				int readindexB = pOriginalIndices [ readindex+2 ];

				// first three indices are basic triangle
				for ( int i = 0; i < 3; i++ )
					indices.push_back ( pOriginalIndices [ readindex++ ] );

				// additional triangles created (A+B+NEW)
				for ( int i=3; i<iIndicePerFace; i++ )
				{
					indices.push_back ( readindexA );
					indices.push_back ( readindexB );
					readindexB = pOriginalIndices [ readindex ];
					indices.push_back ( pOriginalIndices [ readindex++ ] );
				}

				// Count new faces
				dwNewFaceCount += (iIndicePerFace-2);
			}
		}

		// create old ref array for material assignments later
		if ( ppOldRef )
		{
			g_pFaceIndexOldRef = new DWORD [ dwNewFaceCount ];
			DWORD dwFaceIndex=0, dwNewFaceIndex=0;

			// mike - 020206 - addition for vs8
			DWORD faceindex = 0;

			// go through original index data again to record face indexes (for oldref used in material assigns)
			for ( readindex=0, faceindex=0; faceindex < *pdwFaceCount; faceindex++ )
			{
				// this is number of indices per face
				int iIndicePerFace = pOriginalIndices [ readindex++ ];

				// only handle triangles and above
				if ( iIndicePerFace>=3 )
				{
					// advance read index
					readindex+=iIndicePerFace;

					// Place face index in all triangles made from original face
					for ( int fill=0; fill<(iIndicePerFace-2); fill++ )
						g_pFaceIndexOldRef [ dwNewFaceIndex++ ] = dwFaceIndex;
					
					// next face
					dwFaceIndex++;
				}
			}
		}

		// use list of new indices to make new index buffer
		SAFE_DELETE(*ppTriIndices);
		DWORD dwIndexBufferSize = indices.size();
		*ppTriIndices = new DWORD [ dwIndexBufferSize ];
		*pdwFaceCount = dwNewFaceCount;
		for ( DWORD index = 0; index < dwIndexBufferSize; index++ )
			*((*ppTriIndices)+index) = indices [ index ];
	}
}

DARKSDK bool XFILE_NEW_GetVerticesAndIndices ( IDirectXFileData* pDataObject, sFrame* pFrame )
{
	// extracts vertices and indices from the data object

	// check the parameters are valid
	if ( !pDataObject || !pFrame )
		return false;

	// local variables
	DWORD     dwSize    = 0;		// size of data
	DWORD*    pData     = NULL;		// initial pointer to data
	float*    pVertices = NULL;		// used for vertices
	DWORD*	  pIndices  = NULL;		// for indices
	int		  i         = 0;		// i, j and k are for loops
	int		  j         = 0;
	int		  k         = 0;

	// get the next data block
	pDataObject->GetData ( NULL, &dwSize, ( void** ) &pData );

	// ensure the data is valid
	if ( !pData )
		return false;
	
	// get vertex count and pointer to vertices
	g_dwVertexCount  = *( pData + 0 );				// count is at start of block
	pVertices        = ( float* ) ( pData + 1 );	// starting vertex is + 1

	// allocate vertex array
	g_pVertexList = new sNewVertex [ g_dwVertexCount ];
	
	// check memory
	if ( !g_pVertexList )
		return false;

	// clear out vertex list
	memset ( g_pVertexList, 0, sizeof ( sNewVertex ) * g_dwVertexCount );
	
	// get each vertex from the pointer
	for ( i = 0; i < ( int ) g_dwVertexCount; i++ )
	{
		// store x, y and z, increment pointer each time to move to next piece of data
		g_pVertexList [ i ].vecPosition.x = *pVertices;		pVertices++;
		g_pVertexList [ i ].vecPosition.y = *pVertices;		pVertices++;
		g_pVertexList [ i ].vecPosition.z = *pVertices;		pVertices++;
	}

	// get index count
	g_dwFaceCount  = *( pData + ( g_dwVertexCount * 3 ) + 1 );				// count is + 1 from last vertex
	pIndices       = ( DWORD* ) ( pData + ( g_dwVertexCount * 3 ) + 2 );	// starting index is + 2 from last vertex

	// allocate indices (as triangles)
	SAFE_DELETE ( g_pIndexList );
	XFILE_NEW_MakeTriangleIndicesFromFaceData ( &g_dwFaceCount, &g_pIndexList, &g_pFaceIndexOldRef, pIndices );

	// finished, return back
	return true;
}

DARKSDK bool XFILE_NEW_GetNormals ( IDirectXFileData* pSubData, sFrame* pFrame )
{
	// get normals from data if they exist

	// check the parameters are valid
	if ( !pSubData || !pFrame )
		return false;

	// local variables
	DWORD*				pData         = NULL;	// pointer to data
	DWORD				dwSize        = 0;		// size of data
	DWORD*				pIndices      = NULL;
	float*				pNormals      = NULL;
	int					i             = 0;		// i, j and k are for loops
	int					j             = 0;
	int					k             = 0;

	// Rest globals
	g_dwNormalCount		= 0;
	g_dwNormalFaceCount	= 0;
	g_pNormalIndexList	= NULL;
	g_pNormalList		= NULL;

	// now extract data pointer
	pSubData->GetData ( NULL, &dwSize, ( void** ) &pData );

	// check it's okay
	if ( !pData )
		return false;

	// we may well have normals which have an index list, we need to
	// get the list of normals and then add this in based on the
	// indices into the main vertex list

	// set up initial data
	g_dwNormalCount = *( pData + 0 );				// number of normals
	pNormals      = ( float* ) ( pData + 1 );	// pointer to normal data

	// allocate array to store normals
	g_pNormalList = new GGVECTOR3 [ g_dwNormalCount ];

	// store normals in list
	for ( i = 0; i < ( int ) g_dwNormalCount; i++ )
	{
		g_pNormalList [ i ].x = *pNormals;	pNormals++;
		g_pNormalList [ i ].y = *pNormals;	pNormals++;
		g_pNormalList [ i ].z = *pNormals;	pNormals++;
	}

	// now we get the number of indices for the normal list
	g_dwNormalFaceCount = *( pData + ( g_dwNormalCount * 3 ) + 1 );
	pIndices    = ( DWORD* ) ( pData + ( g_dwNormalCount * 3 ) + 2 );

	// allocate list (as triangles)
	SAFE_DELETE ( g_pNormalIndexList );
	XFILE_NEW_MakeTriangleIndicesFromFaceData ( &g_dwNormalFaceCount, &g_pNormalIndexList, NULL, pIndices );

	return true;
}

DARKSDK void XFILE_NEW_CopyNormalsToVertices ( void )
{
	// if normal face count and vertex face count differ, something odd..
	if ( g_dwNormalFaceCount != g_dwFaceCount )
		return;

	// vertex data may be expanded if more normals
	if ( g_dwNormalCount > g_dwVertexCount )
	{
		// leefix - 200104 - vertex data is master, and normals copy over
		sNewVertex* pNewVertexData = new sNewVertex [ g_dwVertexCount ];
		DWORD* pNewIndexData = new DWORD [ g_dwFaceCount * 3 ];
		memcpy ( pNewVertexData, g_pVertexList, sizeof(sNewVertex)*g_dwVertexCount );
		memcpy ( pNewIndexData, g_pIndexList, sizeof(DWORD)*g_dwFaceCount*3 );

		// go through each face in face list
		for ( DWORD dwFaceBase = 0; dwFaceBase < g_dwFaceCount*3; dwFaceBase++ )
		{
			// vertices of face
			DWORD dwVIndex = g_pIndexList [ dwFaceBase ];

			// normals of face
			DWORD dwNIndex = g_pNormalIndexList [ dwFaceBase ];

			// copy the normal over
			pNewVertexData [ dwVIndex ].vecNormal.x = g_pNormalList [ dwNIndex ].x;
			pNewVertexData [ dwVIndex ].vecNormal.y = g_pNormalList [ dwNIndex ].y;
			pNewVertexData [ dwVIndex ].vecNormal.z = g_pNormalList [ dwNIndex ].z;
		}

		/* leefix - 300109 - hud.x model had missing polys as last seven normals had same number in faceindex data
		// create new vertex data allocation (based on normals data)
		sNewVertex* pNewVertexData = new sNewVertex [ g_dwNormalCount ];
		DWORD* pNewIndexData = new DWORD [ g_dwFaceCount * 3 ];

		// can simply copy over the index table from the normals data (the new template for the index)
		memcpy ( pNewIndexData, g_pNormalIndexList, sizeof(DWORD)*g_dwFaceCount*3 );

		// copy existing vertex data to new data
		for ( int i = 0; i < ( int ) g_dwFaceCount * 3; i++ )
		{
			// NEW vertex index based on normal data
			DWORD dwNewExpandedIndex = g_pNormalIndexList [ i ];

			// copy the vertex data from original position
			DWORD dwOldIndex = g_pIndexList [ i ];
			pNewVertexData [ dwNewExpandedIndex ] = g_pVertexList [ dwOldIndex ];

			// copy the normal vector (using the normal face data for both indexes)
			pNewVertexData [ dwNewExpandedIndex ].vecNormal.x = g_pNormalList [ dwNewExpandedIndex ].x;
			pNewVertexData [ dwNewExpandedIndex ].vecNormal.y = g_pNormalList [ dwNewExpandedIndex ].y;
			pNewVertexData [ dwNewExpandedIndex ].vecNormal.z = g_pNormalList [ dwNewExpandedIndex ].z;
		}
		g_dwVertexCount = g_dwNormalCount;
		*/

		// delete old and replace with new data
		SAFE_DELETE ( g_pVertexList );
		SAFE_DELETE ( g_pIndexList );
		g_pVertexList = pNewVertexData;
		g_pIndexList = pNewIndexData;
	}
	else
	{
		// copy all normals direct to vertice data
		for ( int i = 0; i < ( int ) g_dwNormalFaceCount * 3; i++ )
		{
			g_pVertexList [ g_pIndexList [ i ] ].vecNormal.x = g_pNormalList [ g_pNormalIndexList [ i ] ].x;
			g_pVertexList [ g_pIndexList [ i ] ].vecNormal.y = g_pNormalList [ g_pNormalIndexList [ i ] ].y;
			g_pVertexList [ g_pIndexList [ i ] ].vecNormal.z = g_pNormalList [ g_pNormalIndexList [ i ] ].z;
		}
	}

	// release any previously allocated memory
	SAFE_DELETE_ARRAY ( g_pNormalList );
	SAFE_DELETE_ARRAY ( g_pNormalIndexList  );
}

DARKSDK bool XFILE_NEW_GetTextureCoordinates ( IDirectXFileData* pSubData, sFrame* pFrame )
{
	// extract texture coordinates from model

	// check the parameters are valid
	if ( !pSubData || !pFrame )
		return false;

	// local variables
	DWORD*	pData          = NULL;	// pointer to data
	DWORD	dwSize         = 0;		// size of data
	int		i              = 0;		// i is for loops
	DWORD	dwTextureCount = 0;		// number of texture coordinates
	float*	pCoordinates   = NULL;	// pointer to coordinates

	// now extract data pointer
	pSubData->GetData ( NULL, &dwSize, ( void** ) &pData );

	// check it's okay
	if ( !pData )
		return false;

	// get access to data from pointers
	dwTextureCount = *( pData + 0 );
	pCoordinates   = ( float* ) ( pData + 1 );

	// now we can copy the texture coordinates across to the vertex list
	for ( i = 0; i < ( int ) dwTextureCount; i++ )
	{
		g_pVertexList [ i ].tu = *pCoordinates;		pCoordinates++;
		g_pVertexList [ i ].tv = *pCoordinates;		pCoordinates++;
	}

	// finished
	return true;
}

DARKSDK bool XFILE_NEW_GetFVFData ( IDirectXFileData* pSubData, sFrame* pFrame )
{
	// extract FVFDATA from model (can be second stage texture UV DATA)
	// FOR NOW, WE EXTRACT AND TREAT AS SECOND STAGE UV DATA
	// OR, WE EXTRACT AND TREAT AS SECOND+THIRD STAGE UV DATA

	// check the parameters are valid
	if ( !pSubData || !pFrame )
		return false;

	// now extract data pointer
	DWORD*	pData          = NULL;	// pointer to data
	DWORD	dwSize         = 0;		// size of data
	pSubData->GetData ( NULL, &dwSize, ( void** ) &pData );
	if ( !pData ) return false;

	// get access to data from pointers
	DWORD dwFVF = *( pData + 0 );
	DWORD dwDWORDS = *( pData + 1 );
	DWORD* pCoordinates   = ( DWORD* ) ( pData + 2 );

	// cordinates per vertex
	int iCoordsPerVertex = 2;
	g_iNumberOfExtraUVDataStagesAvailable = 1;
	if ( dwFVF==770 )
	{
		g_iNumberOfExtraUVDataStagesAvailable = 2;
		iCoordsPerVertex = 4;
	}

	// now we can copy across to the temp vertex list
	DWORD dwIndex = 0;
	for ( int i = 0; i < ( int ) dwDWORDS; i+=iCoordsPerVertex )
	{
		if ( dwFVF==258 ) 
		{
			// second stage
			DWORD dwU = *(pCoordinates++);
			DWORD dwV = *(pCoordinates++);
			float fU = *(float*)&dwU;
			float fV = *(float*)&dwV;
			g_pVertexList [ dwIndex ].tu2 = fU;
			g_pVertexList [ dwIndex ].tv2 = fV;
		}
		if ( dwFVF==770 ) 
		{
			// second+third stage
			DWORD dwU1 = *(pCoordinates++);
			DWORD dwV1 = *(pCoordinates++);
			DWORD dwU2 = *(pCoordinates++);
			DWORD dwV2 = *(pCoordinates++);
			float fU1 = *(float*)&dwU1;
			float fV1 = *(float*)&dwV1;
			float fU2 = *(float*)&dwU2;
			float fV2 = *(float*)&dwV2;
			g_pVertexList [ dwIndex ].tu2 = fU1;
			g_pVertexList [ dwIndex ].tv2 = fV1;
			g_pVertexList [ dwIndex ].tu3 = fU2;
			g_pVertexList [ dwIndex ].tv3 = fV2;
		}
		dwIndex++;
	}

	// U75 - 080809 - legacy loader ignores FVFDATA
	if ( g_iLegacyModeIgnoresFVFDATA==1 )
	{
		// solves FPSC source crashing when load an FVF=530 X as DBO
		g_iNumberOfExtraUVDataStagesAvailable = 0;
	}

	// finished
	return true;
}

DARKSDK bool XFILE_NEW_GetVertexColors ( IDirectXFileData* pSubData, sFrame* pFrame )
{
	// extract vertex colour from model

	// check the parameters are valid
	if ( !pSubData || !pFrame )
		return false;

	// get size and data ptr
	DWORD dwSize = 0;
	DWORD* pData = NULL;
	pSubData->GetData ( NULL, &dwSize, ( void** ) &pData );

	// check it's okay
	if ( !pData )
		return false;

	// get access to data from pointers
	g_dwVertexColorCount = *( pData + 0 );
	sVertexColor* pVColor = ( sVertexColor* ) ( pData + 1 );

	// copy vertex data into global store for combining later
	SAFE_DELETE(g_pVertexColorList);
	// need flag so VERTEX COLOR info is ignored during X file load (PBR shader not support this)
	//{
	//	g_pVertexColorList = new sVertexColor [ g_dwVertexColorCount ];
	//	memcpy ( g_pVertexColorList, pVColor, g_dwVertexColorCount*sizeof(sVertexColor) );
	//}

	// finished
	return true;
}

DARKSDK bool XFILE_NEW_GetMaterialName ( IDirectXFileData* pSubData, sNewMaterial* pMaterial )
{
	// see if we have a name for the material

	// check parameters
	if ( !pSubData || !pMaterial )
		return false;

	// local variables
	IDirectXFileObject*	pSubObjectTexture = NULL;	// object interface
	IDirectXFileData*	pSubDataTexture   = NULL;	// object data
	const GUID*			guidTypeTexture   = NULL;	// identifier
	char*				szNameTexture     = NULL;	// name

	// get sub object
	if ( SUCCEEDED ( pSubData->GetNextObject ( &pSubObjectTexture ) ) )
	{
		// determine object type
		if ( SUCCEEDED ( pSubObjectTexture->QueryInterface ( DBIID_IDirectXFileData, ( void** ) &pSubDataTexture ) ) )
		{
			// get guid and name
			if ( !XFILE_GetTemplateInfo ( pSubDataTexture, &guidTypeTexture, &szNameTexture ) )
				return false;

			// see if we have a texture filename
			if ( *guidTypeTexture == TID_D3DRMTextureFilename )
			{
				// locals
				DWORD*	pData  = NULL;	// data
				DWORD   dwSize = 0;		// size
				char*   szName = NULL;	// name

				// get the data
				pSubDataTexture->GetData ( NULL, &dwSize, ( void** ) &pData );

				// store pointer to name
				szName = ( char* ) *( pData + 0 );

				// allocate memory to hold name
				// LEEFIX : 301003 - need extra byte for null termionator :)
				pMaterial->szFilename = new char [ (strlen ( szName ) * sizeof ( char ))+1 ];

				if ( !pMaterial->szFilename )
					return false;

				// finally copy data
				strcpy ( pMaterial->szFilename, szName );
			}

			// MIKE - 311003
			SAFE_DELETE_ARRAY ( szNameTexture );
			SAFE_RELEASE ( pSubDataTexture );
		}

		SAFE_RELEASE ( pSubObjectTexture );
	}

	return true;
}

DARKSDK bool XFILE_NEW_GetMaterialData ( IDirectXFileData* pSubDataMaterial, char* szNameMaterial )
{
	// locals
	DWORD*			pData    = NULL;	// pointer to data
	DWORD			dwSize   = 0;		// size of data
	sNewMaterial	material;			// to store material information

	memset ( &material, 0, sizeof ( material ) );

	// extract data pointer
	pSubDataMaterial->GetData ( NULL, &dwSize, ( void** ) &pData );

	// get pointer to data
	sNewMaterial* pPointer = ( sNewMaterial* ) ( pData + 0 );

	// copy material data
	memcpy ( &material, pPointer, sizeof ( sNewMaterial ) );

	material.szFilename = NULL;

	// look for a name
	XFILE_NEW_GetMaterialName ( pSubDataMaterial, &material );

	// 090618 - ensure no TGA is converted over (default is PNG)
	if ( material.szFilename != NULL )
	{
		if ( strnicmp ( material.szFilename + strlen(material.szFilename) - 4, ".tga", 4 ) == NULL )
		{
			// chop TGA
			material.szFilename[strlen(material.szFilename)-4] = 0;

			// check if _ALB used, instead use _COLOR
			if ( strnicmp ( material.szFilename + strlen(material.szFilename) - 4, "_ALB", 4 ) == NULL )
			{
				LPSTR pNewMaterialFilename = new char[strlen(material.szFilename)+10];
				strcpy ( pNewMaterialFilename, material.szFilename );
				SAFE_DELETE ( material.szFilename );
				material.szFilename = pNewMaterialFilename;
				material.szFilename[strlen(material.szFilename)-4] = 0;
				strcat ( material.szFilename, "_color" );
			}

			// add correct default image extension
			strcat ( material.szFilename, ".png" );
		}
	}

	// lee - 300518 - if texture is blank or NULL, pass in material name (can assemble texture name from this later)
	if ( material.szFilename == NULL || strlen(material.szFilename) == 0 || stricmp ( material.szFilename, "null" ) == NULL )
	{
		// assemble a texture name that is likely to be found (typical FBX conversion to X)
		SAFE_DELETE( material.szFilename );
		if ( material.szFilename == NULL ) material.szFilename = new char[1024];
		strcpy ( material.szFilename, szNameMaterial );

		// adjust material name to texture name (from known material names to known texture name conventions)
		char pNewTextureName[1024];
		if ( strnicmp ( material.szFilename, "M_", 2 ) == NULL )
		{
			// material name starts with M_, cut this and add PBR texture designation
			strcpy ( pNewTextureName, material.szFilename+2 );
		}
		else
		{
			// material name has no prefix, so treat material name as suggested texture name
			strcpy ( pNewTextureName, material.szFilename );
		}
		strcat ( pNewTextureName, "_Color.png" );
		strcpy ( material.szFilename, pNewTextureName );
	}

	// send data to back of list
	g_NewMaterialList.push_back ( material );

	// all done
	return true;
}

DARKSDK bool XFILE_NEW_GetOptionalMaterialData ( IDirectXFileData* pSubData )
{
	// search for material data

	if ( !pSubData )
		return false;

	// local variables
	IDirectXFileObject*	pSubObjectMaterial = NULL;		// object interface
	IDirectXFileData*	pSubDataMaterial   = NULL;		// data interface
	const GUID*			guidTypeMaterial   = NULL;		// identifier
	char*				szNameMaterial     = NULL;		// name
	bool				bSearch            = true;		// search flag

	// loop round looking for material data
	while ( bSearch )
	{
		// get the sub object
		if ( SUCCEEDED ( pSubData->GetNextObject ( &pSubObjectMaterial ) ) )
		{
			// determine if data obj or reference
			pSubDataMaterial = NULL;
			if ( FAILED ( pSubObjectMaterial->QueryInterface ( DBIID_IDirectXFileData, ( void** ) &pSubDataMaterial ) ) )
			{
				IDirectXFileDataReference* pSubDataRef = NULL;
				if ( SUCCEEDED ( pSubObjectMaterial->QueryInterface ( DBIID_IDirectXFileDataReference, ( void** ) &pSubDataRef ) ) )
				{
					pSubDataRef->Resolve ( &pSubDataMaterial ); 
					SAFE_RELEASE ( pSubDataRef ); 
				}
			}

			// if valid data
			if ( pSubDataMaterial )
			{
				// get information about the type
				if ( XFILE_GetTemplateInfo ( pSubDataMaterial, &guidTypeMaterial, &szNameMaterial ) )
				{
					// do we have a material
					if ( *guidTypeMaterial == TID_D3DRMMaterial )
					{
						XFILE_NEW_GetMaterialData ( pSubDataMaterial, szNameMaterial );
					}

					// free material name
					SAFE_DELETE_ARRAY ( szNameMaterial );
				}

				SAFE_RELEASE ( pSubDataMaterial );
			}
			else
				bSearch = false;

			SAFE_RELEASE ( pSubObjectMaterial );
		}
		else
			bSearch = false;
	}

	// all done
	return true;
}

DARKSDK bool XFILE_NEW_GetMeshMaterialList ( IDirectXFileData* pSubData, sFrame* pFrame )
{
	// 111005

	// get the material list indices from the data

	if ( !pSubData || !pFrame )
		return false;

	// local variables
	DWORD*	pData    = NULL;	// pointer to data
	DWORD	dwSize   = 0;		// size of data
	DWORD*  pIndices = NULL;	// pointer to index data
	int		i        = 0;		// used for loops

	// now extract data pointer
	pSubData->GetData ( NULL, &dwSize, ( void** ) &pData );

	// store properties
	g_dwMaterialCount          = *( pData + 0 );							// number of materials
	g_dwMaterialListIndexCount = *( pData + 1 );							// number of indices
	g_pMaterialList            = new DWORD [ g_dwMaterialListIndexCount ];	// allocate list
	pIndices                   = ( DWORD* ) ( pData + 2 );					// pointer to indices

	// check newly allocated memory
	if ( !g_pMaterialList )
		return false;

	// copy material list indices
	for ( i = 0; i < ( int ) g_dwMaterialListIndexCount; i++ )
	{
		g_pMaterialList [ i ] = *pIndices;
		pIndices++;
	}

	// lee - 171203 - update material list if face data expanded (due to quads and other non-tri faces)
	#ifndef DARKSDK_COMPILE
	if ( g_pFaceIndexOldRef )
	{
		// create new material array and fill with correct data
		DWORD dwNewMaterialListIndexCount = g_dwFaceCount;
		DWORD* pNewMaterialList = new DWORD [ dwNewMaterialListIndexCount ];
		for ( DWORD newindex=0; newindex<dwNewMaterialListIndexCount; newindex++ )
		{
			int oldindex = g_pFaceIndexOldRef [ newindex ];

			// mike - 310305 - stop overflow
			if ( oldindex >= (int)g_dwMaterialListIndexCount )
				oldindex = g_dwMaterialListIndexCount - 1;

			pNewMaterialList [ newindex ] = g_pMaterialList [ oldindex ];
		}

		// remove old material array and replace with new
		SAFE_DELETE(g_pMaterialList);
		g_pMaterialList = pNewMaterialList;
		g_dwMaterialListIndexCount = dwNewMaterialListIndexCount;

		// free usages
		SAFE_DELETE_ARRAY ( g_pFaceIndexOldRef );
	}
	#endif

	// leefix - 041103 - consider materials reference by face and if we have 
	// processed quads or greater, those face references have changed!
	/*
	if ( g_pFaceIndexOldRef )
	{
		// only works for QUADS (2poly) - later must cater for 5,6,7 poly-etc
		DWORD newindex=0;
		DWORD dwNewMaterialListIndexCount = 0;
		dwNewMaterialListIndexCount = g_dwFaceCount;
		DWORD* pNewMaterialList = new DWORD [ dwNewMaterialListIndexCount ];
		for ( DWORD j = 0; j < ( int ) g_dwMaterialListIndexCount; j++ )
		{
			pNewMaterialList [ newindex+0 ] = g_pMaterialList [ j ];
			pNewMaterialList [ newindex+1 ] = g_pMaterialList [ j ];
			newindex+=2;
		}
		g_dwMaterialListIndexCount = dwNewMaterialListIndexCount;
		SAFE_DELETE(g_pMaterialList);
		g_pMaterialList = pNewMaterialList;

		// rather redundant as really a quad equates to two identical index references
		// might need this data though when we go for 5,6,7,8 sided polygons..
		SAFE_DELETE_ARRAY ( g_pFaceIndexOldRef );
	}
	*/

	// look for optional material and texture data
	XFILE_NEW_GetOptionalMaterialData ( pSubData );

	// all done
	return true;
}

DARKSDK bool XFILE_NEW_GetSkinMeshHeader ( IDirectXFileData* pSubData, sFrame* pFrame )
{
	// get the skin mesh header data

	if ( !pSubData || !pFrame )
		return false;

	// local variables
	WORD*	pData    = NULL;	// pointer to data
	DWORD	dwSize   = 0;		// size of data

	// get pointer to data and size
	pSubData->GetData ( NULL, &dwSize, ( void** ) &pData );

	// MIKE - 141003 - this is where we can get a problem, need to cast to
	//               - WORD or DWORD dependant on data size
	if ( dwSize == 12 )
	{
		DWORD*	pData  = NULL;
		DWORD	dwSize = 0;

		pSubData->GetData ( NULL, &dwSize, ( void** ) &pData );

		g_dMaxSkinWeightsPerVertex = (WORD) *( pData + 0 );
		g_dMaxSkinWeightsPerFace   = (WORD) *( pData + 1 );
		g_dwBones                  = *( pData + 2 );
	}
	else
	{
		WORD* pData  = NULL;
		DWORD dwSize = 0;

		pSubData->GetData ( NULL, &dwSize, ( void** ) &pData );

		g_dMaxSkinWeightsPerVertex = *( pData + 0 );
		g_dMaxSkinWeightsPerFace   = *( pData + 1 );
		g_dwBones                  = *( pData + 2 );
	}

	// finished
	return true;
}

DARKSDK bool XFILE_NEW_GetSkinWeights ( IDirectXFileData* pSubData, sFrame* pFrame )
{
	// get information on the bone, the structure contains
	//		
	//		name
	//		number of weights
	//		an array of indices [ number of weights ]
	//		an array of weights [ number of weights ]
	//		transform matrix

	// check parameters
	if ( !pSubData || !pFrame )
		return false;

	// local variables
	DWORD*		pData        = NULL;	// pointer to data
	DWORD		dwSize       = 0;		// size of data
	int			i			 = 0;		// used for temp loops
	char*       szName       = NULL;	// name of the bone
	DWORD*		pIndices     = NULL;	// indices into vertices
	float*		pWeights     = NULL;	// weights
	sNewBones	bone;					// bone structure to be filled in

	// now extract data pointer
	pSubData->GetData ( NULL, &dwSize, ( void** ) &pData );

	// check data is valid
	if ( !pData )
		return false;

	// clear out bone structure
	memset ( &bone, 0, sizeof ( sNewBones ) );

	// get pointer to name
	szName = ( char* ) *( pData + 0 );

	// allocate new memory to hold name
	bone.szName = new char [ strlen ( szName ) * sizeof ( char ) + 1 ];

	// check memory
	if ( !bone.szName )
		return false;

	// copy name across to bone structure
	strcpy ( bone.szName, szName );

	// store number of weights
	bone.dwNumWeights = *( pData + 1 );
	
	// see if we have any weights
	if ( bone.dwNumWeights )
	{
		// allocate index and weight arrays
		bone.pIndices  = new DWORD [ bone.dwNumWeights ];
		bone.pfWeights = new float [ bone.dwNumWeights ];

		// ensure memory is valid
		if ( !bone.pIndices || !bone.pfWeights )
			return false;

		// store pointer to indices and weights
		pIndices = ( pData + 2 );
		pWeights = ( float* ) ( pData + 2 + bone.dwNumWeights );

		// copy bone indices across
		for ( i = 0; i < (int)bone.dwNumWeights; i++ )
		{
			bone.pIndices [ i ] = *pIndices;
			pIndices++;
		}

		// and now copy weights across
		for ( i = 0; i < (int)bone.dwNumWeights; i++ )
		{
			bone.pfWeights [ i ] = *pWeights;
			pWeights++;
		}

		// copy matrix
		memcpy ( &bone.matrix, ( GGMATRIX* ) ( pData + 2 + ( bone.dwNumWeights * 2 ) ), sizeof ( GGMATRIX ) );
	}
	else
	{
		// if we have no weights matrix is directly after number of weights so + 2
		memcpy ( &bone.matrix, ( GGMATRIX* ) ( pData + 2 ), sizeof ( GGMATRIX ) );
	}
	
	// send bone to back of list
	g_BoneList.push_back ( bone );
	
	// all done
	return true;
}

DARKSDK bool XFILE_NEW_GetDeclData ( IDirectXFileData* pSubData, sFrame* pFrame )
{
	// DWORD nElements;
	// array VertexElement Elements[nElements];
	// DWORD nDWords;
	// array DWORD data[nDWords];
	//
	// VertexElement
	// {
	//  DWORD Type;
	//  DWORD Method;
	//  DWORD Usage;
	//  DWORD UsageIndex;
	// }
	#define MAX_FVF_DECL_SIZE 65
	GGVERTEXELEMENT pVertexDecl [ MAX_FVF_DECL_SIZE ];

	// check parameters
	if ( !pSubData || !pFrame )
		return false;

	// now extract data pointer
	DWORD*		pData        = NULL;	// pointer to data
	DWORD		dwSize       = 0;		// size of data
	pSubData->GetData ( NULL, &dwSize, ( void** ) &pData );

	// check data is valid
	if ( !pData ) return false;

	// element count
	DWORD dwVertexDeclCount			= *( pData++ );

	// element data
	for ( DWORD n=0; n<dwVertexDeclCount; n++ )
	{
		DWORD dwElementType			= *( pData+0 );
		DWORD dwElementMethod		= *( pData+1 );
		DWORD dwElementUsage		= *( pData+2 );
		DWORD dwElementUsageIndex	= *( pData+3 );
		pVertexDecl[n].Type			= (BYTE)dwElementType;
		pVertexDecl[n].Method		= (BYTE)dwElementMethod;
		pVertexDecl[n].Usage		= (BYTE)dwElementUsage;
		pVertexDecl[n].UsageIndex	= (BYTE)dwElementUsageIndex;
		pData+=4;
	}

	// DWORDs count
	DWORD dwVertexDeclDataCount =	*( pData++ );

	// DWORDs data
	DWORD* pVertexDeclData = new DWORD [ dwVertexDeclDataCount ];
	for ( DWORD n=0; n<dwVertexDeclDataCount; n++ )
		pVertexDeclData[n]			= *( pData++ );

	// now all data extracted, we can extract it into meaningful values
	// and place them in the arrays used to collect normals, texture coords, diffuse, etc
	// go through declaration data in order specified
	DWORD* pDWORDPtr = pVertexDeclData;
	for ( DWORD vi=0; vi<g_dwVertexCount; vi++ )
	{
		for ( DWORD n=0; n<dwVertexDeclCount; n++ )
		{
			switch ( pVertexDecl[n].Usage )
			{
				case D3DDECLUSAGE_POSITION:
					g_pVertexList [ vi ].vecPosition.x = *(float*)pDWORDPtr; pDWORDPtr++;
					g_pVertexList [ vi ].vecPosition.y = *(float*)pDWORDPtr; pDWORDPtr++;
					g_pVertexList [ vi ].vecPosition.z = *(float*)pDWORDPtr; pDWORDPtr++;
				break;
				case D3DDECLUSAGE_POSITIONT :	pDWORDPtr+=4; break;
				case D3DDECLUSAGE_PSIZE :		pDWORDPtr+=1; break;
				case D3DDECLUSAGE_NORMAL:
					g_pVertexList [ vi ].vecNormal.x = *(float*)pDWORDPtr; pDWORDPtr++;
					g_pVertexList [ vi ].vecNormal.y = *(float*)pDWORDPtr; pDWORDPtr++;
					g_pVertexList [ vi ].vecNormal.z = *(float*)pDWORDPtr; pDWORDPtr++;
				break;
				case D3DDECLUSAGE_COLOR:
					if ( g_pVertexColorList==NULL ) g_pVertexColorList = new sVertexColor [ g_dwVertexColorCount ];
					g_pVertexColorList [ vi ].index = vi;
					g_pVertexColorList [ vi ].color = *(sColorRGBA*)pDWORDPtr; pDWORDPtr++;
				break;
				case D3DDECLUSAGE_TEXCOORD:
				{
					switch ( pVertexDecl[n].Type )
					{
						case D3DDECLTYPE_FLOAT2:
							g_pVertexList [ vi ].tu = *(float*)pDWORDPtr; pDWORDPtr++;
							g_pVertexList [ vi ].tv = *(float*)pDWORDPtr; pDWORDPtr++;
						break;
						case D3DDECLTYPE_FLOAT3:
							g_pVertexList [ vi ].tu = *(float*)pDWORDPtr; pDWORDPtr++;
							g_pVertexList [ vi ].tv = *(float*)pDWORDPtr; pDWORDPtr++;
							pDWORDPtr++;
						break;
						case D3DDECLTYPE_FLOAT4:
							g_pVertexList [ vi ].tu = *(float*)pDWORDPtr; pDWORDPtr++;
							g_pVertexList [ vi ].tv = *(float*)pDWORDPtr; pDWORDPtr++;
							pDWORDPtr++;
							pDWORDPtr++;
						break;
					}
				}
				break;
				case D3DDECLUSAGE_TANGENT :		pDWORDPtr+=4; break;
				case D3DDECLUSAGE_BINORMAL :	pDWORDPtr+=4; break;
			}
		}
	}

	// free usages
	SAFE_DELETE ( pVertexDeclData );

	// all done
	return true;
}

DARKSDK bool XFILE_NEW_CreateMeshInFrame ( sFrame* pFrame, DWORD dwNumVertices, DWORD dwNumFaces )
{
	// LEELEE - Ensure when change IndiceCount, that this function still gets FACE COUNT!
	// check we have a valid frame
	SAFE_MEMORY ( pFrame );

	// ceate a new mesh to hold the data
	pFrame->pMesh = new sMesh;

	// ensure the mesh was created
	SAFE_MEMORY ( pFrame->pMesh );

	// mesh 'can' store 32bit index data temporarily for later conversion to vertex only.
	// and should only use 32bit if going to do a vertex expanding (due to >16bit verts)
	bool b32BITIndexData=false;
	if ( dwNumVertices > 0xFFFF )
		b32BITIndexData=true;

	// set standard FVF format
	pFrame->pMesh->dwFVF			= GGFVF_XYZ | GGFVF_NORMAL | GGFVF_TEX1;
	pFrame->pMesh->dwFVFSize		= 32;

#ifdef WICKEDENGINE
	// an extra set of UV Data is available
	if ( g_iNumberOfExtraUVDataStagesAvailable==1 )
	{
		pFrame->pMesh->dwFVF		= GGFVF_XYZ | GGFVF_NORMAL | GGFVF_TEX2;
		pFrame->pMesh->dwFVFSize	+= 8;
	}
	if ( g_iNumberOfExtraUVDataStagesAvailable==2 )
	{
		pFrame->pMesh->dwFVF		= GGFVF_XYZ | GGFVF_NORMAL | GGFVF_TEX3;
		pFrame->pMesh->dwFVFSize	+= 16;
	}

	// add additional FVF flags as required
	if ( g_pVertexColorList )
	{
		pFrame->pMesh->dwFVF		|= GGFVF_DIFFUSE;
		pFrame->pMesh->dwFVFSize	+= 4;
	}
#endif

	// setup mesh properties and allocate memory - index buffer
	pFrame->pMesh->dwVertexCount	= dwNumVertices;
	pFrame->pMesh->dwIndexCount		= dwNumFaces * 3;
	DWORD dwVertexDataSize = pFrame->pMesh->dwFVFSize * dwNumVertices;
	pFrame->pMesh->pVertexData		= ( BYTE* ) new char [ dwVertexDataSize ];
	if ( b32BITIndexData )
		pFrame->pMesh->pIndices			= (WORD*)new DWORD [ pFrame->pMesh->dwIndexCount ];
	else
		pFrame->pMesh->pIndices			= new WORD [ pFrame->pMesh->dwIndexCount ];

	// setup mesh drawing properties
	pFrame->pMesh->iPrimitiveType	= GGPT_TRIANGLELIST;
	pFrame->pMesh->iDrawVertexCount	= dwNumVertices;
	pFrame->pMesh->iDrawPrimitives	= dwNumFaces;

	// check the newly created memory is okay
	SAFE_MEMORY ( pFrame->pMesh->pVertexData );
	SAFE_MEMORY ( pFrame->pMesh->pIndices  );

	// clear out newly created memory
	memset ( pFrame->pMesh->pVertexData, 0, dwVertexDataSize );
	if ( b32BITIndexData )
	 	memset ( pFrame->pMesh->pIndices, 0, sizeof ( DWORD ) * pFrame->pMesh->dwIndexCount  );
	else
	 	memset ( pFrame->pMesh->pIndices, 0, sizeof ( WORD ) * pFrame->pMesh->dwIndexCount  );

	return true;
}

DARKSDK bool XFILE_NEW_SetupVertexData ( sFrame* pFrame )
{
	// get the vertices and indices into the object
	if ( !pFrame )
		return false;

	// get mesh ptr
	sMesh* pMesh = pFrame->pMesh;

	// get offset data
	sOffsetMap	offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// get a pointer to the vertex data
	BYTE* pVertex = pMesh->pVertexData;

	// copy vertex data
	for ( int iVertex = 0; iVertex < ( int ) pMesh->dwVertexCount; iVertex++ )
	{
		// position
		if ( offsetMap.dwZ != 0 )
		{
			*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * iVertex ) ) = g_pVertexList [ iVertex ].vecPosition.x;
			*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * iVertex ) ) = g_pVertexList [ iVertex ].vecPosition.y;
			*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * iVertex ) ) = g_pVertexList [ iVertex ].vecPosition.z;
		}

		// normal (dwNX changed to dwNZ 040908 - new use of function with DECLDATA read)
		if ( offsetMap.dwNZ != 0 )
		{
			*( ( float* ) pVertex + offsetMap.dwNX + ( offsetMap.dwSize * iVertex ) ) = g_pVertexList [ iVertex ].vecNormal.x;
			*( ( float* ) pVertex + offsetMap.dwNY + ( offsetMap.dwSize * iVertex ) ) = g_pVertexList [ iVertex ].vecNormal.y;
			*( ( float* ) pVertex + offsetMap.dwNZ + ( offsetMap.dwSize * iVertex ) ) = g_pVertexList [ iVertex ].vecNormal.z;
		}

		// base level UV data
		if ( offsetMap.dwTU [ 0 ] != 0 )
		{
			*( ( float* ) pVertex + offsetMap.dwTU [ 0 ] + ( offsetMap.dwSize * iVertex ) ) = g_pVertexList [ iVertex ].tu;
			*( ( float* ) pVertex + offsetMap.dwTV [ 0 ] + ( offsetMap.dwSize * iVertex ) ) = g_pVertexList [ iVertex ].tv;
		}

		// if diffuse present, fill it
		if ( offsetMap.dwDiffuse != 0 )
		{
			DWORD dwColor = GGCOLOR ( 1, 1, 1, 1 );
			*( ( DWORD* ) pVertex + offsetMap.dwDiffuse + ( offsetMap.dwSize * iVertex ) ) = dwColor;
		}

		// if extra set of UV Data
		if ( g_iNumberOfExtraUVDataStagesAvailable==1 )
		{
			*( ( float* ) pVertex + offsetMap.dwTU [ 1 ] + ( offsetMap.dwSize * iVertex ) ) = g_pVertexList [ iVertex ].tu2;
			*( ( float* ) pVertex + offsetMap.dwTV [ 1 ] + ( offsetMap.dwSize * iVertex ) ) = g_pVertexList [ iVertex ].tv2;
		}
		if ( g_iNumberOfExtraUVDataStagesAvailable==2 )
		{
			*( ( float* ) pVertex + offsetMap.dwTU [ 1 ] + ( offsetMap.dwSize * iVertex ) ) = g_pVertexList [ iVertex ].tu2;
			*( ( float* ) pVertex + offsetMap.dwTV [ 1 ] + ( offsetMap.dwSize * iVertex ) ) = g_pVertexList [ iVertex ].tv2;
			*( ( float* ) pVertex + offsetMap.dwTU [ 2 ] + ( offsetMap.dwSize * iVertex ) ) = g_pVertexList [ iVertex ].tu3;
			*( ( float* ) pVertex + offsetMap.dwTV [ 2 ] + ( offsetMap.dwSize * iVertex ) ) = g_pVertexList [ iVertex ].tv3;
		}
	}

	// diffuse color component
	if ( g_pVertexColorList && offsetMap.dwDiffuse != 0 )
	{
		// copy vertex color data (index provided by vertex color data itself)
		for ( DWORD dwVertColArrayIndex = 0; dwVertColArrayIndex < g_dwVertexColorCount; dwVertColArrayIndex++ )
		{
			// copy vertex color to correct place in vertex data
			sVertexColor VertexColor = g_pVertexColorList [ dwVertColArrayIndex ];
			int iVertex = VertexColor.index;
			DWORD dwColor = GGCOLOR ( VertexColor.color.red, VertexColor.color.green, VertexColor.color.blue, VertexColor.color.alpha );
			*( ( DWORD* ) pVertex + offsetMap.dwDiffuse + ( offsetMap.dwSize * iVertex ) ) = dwColor;
		}

		// switch mesh from material usage to vertex diffuse usage (this code comes after texture setup)
		pMesh->bUsesMaterial = false;

		// free usages
		SAFE_DELETE ( g_pVertexColorList );
	}

	// now copy indices (16 or 32 bit)
	if ( pMesh->dwVertexCount>0xFFFF )
	{
		DWORD* pDWORDIndexData = (DWORD*)pMesh->pIndices;
		for ( int iIndex = 0; iIndex < ( int ) pMesh->dwIndexCount; iIndex++ )
			pDWORDIndexData [ iIndex ] = g_pIndexList [ iIndex ];
	}
	else
	{
		for ( int iIndex = 0; iIndex < ( int ) pMesh->dwIndexCount; iIndex++ )
			pMesh->pIndices [ iIndex ] = (WORD) g_pIndexList [ iIndex ];
	}
	
	// we're finished now
	return true;
}

DARKSDK bool XFILE_NEW_SetupBones ( sFrame* pFrame )
{
	// sort out bones for the model

	if ( !pFrame )
		return false;

	pFrame->pMesh->dwBoneCount = g_dwBones;

	if ( pFrame->pMesh->dwBoneCount == 0 )
		return true;

	bool bDisable = false;
	
	// allocate bone array
	pFrame->pMesh->pBones = new sBone [ pFrame->pMesh->dwBoneCount ];
	
	// run through all bones and store information
	for ( int iTemp = 0; iTemp < ( int ) pFrame->pMesh->dwBoneCount; iTemp++ )
	{
		// get number of bone influences
		pFrame->pMesh->pBones [ iTemp ].dwNumInfluences = g_BoneList [ iTemp ].dwNumWeights;

		// LEEFIX - 161003 - It is perfectly okay for some bones to have no weights
//		g_bDisableBones = true;
		if ( g_BoneList [ iTemp ].dwNumWeights == 0 )
		{
			// this bone does not affect our model at this time
			pFrame->pMesh->pBones [ iTemp ].pVertices = NULL;
			pFrame->pMesh->pBones [ iTemp ].pWeights  = NULL;
		}
		else
		{
			// allocate vertex and weight arrays
			pFrame->pMesh->pBones [ iTemp ].pVertices = new DWORD [ pFrame->pMesh->pBones [ iTemp ].dwNumInfluences ];
			pFrame->pMesh->pBones [ iTemp ].pWeights  = new float [ pFrame->pMesh->pBones [ iTemp ].dwNumInfluences ];
			for ( int i = 0; i < (int)g_BoneList [ iTemp ].dwNumWeights; i++ )
			{
				pFrame->pMesh->pBones [ iTemp ].pVertices [ i ] = g_BoneList [ iTemp ].pIndices [ i ];
				pFrame->pMesh->pBones [ iTemp ].pWeights  [ i ] = g_BoneList [ iTemp ].pfWeights [ i ];
			}
		}

		// we can now get the translation matrix
		memcpy ( pFrame->pMesh->pBones [ iTemp ].matTranslation, &g_BoneList [ iTemp ].matrix, sizeof ( GGMATRIX ) );

		// finally store the name of the bone
		strcpy ( pFrame->pMesh->pBones [ iTemp ].szName, g_BoneList [ iTemp ].szName );
	}

	// return back to caller
	return true;
}

