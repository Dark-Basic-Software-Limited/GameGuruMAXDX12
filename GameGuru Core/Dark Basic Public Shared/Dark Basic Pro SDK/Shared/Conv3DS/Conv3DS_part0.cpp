// include
#include ".\..\..\..\..\Include\Conv3DS.h"
#include ".\..\..\..\..\Include\CGfxC.h"
#include "d3dmath.h"

///typedef IDirect3DDevice9*		( *GFX_GetDirect3DDevicePFN )	( void );
	
DBPRO_GLOBAL HINSTANCE						g_hSetup						= NULL;
extern LPGGDEVICE							m_pD3D;
DBPRO_GLOBAL sObject*						g_pObject						= NULL;
DBPRO_GLOBAL sObject*						g_pObjectFinal   				= NULL;
DBPRO_GLOBAL sMesh*							g_pNewMeshList;
extern GlobStruct*							g_pGlob;
DBPRO_GLOBAL sObject**						g_ObjectList;

DARKSDK int DB_Convert3DSBinary(char* Filename, char* DestFile);

DARKSDK bool Load ( char* szFilename )
{
	// Calculate temp model file
	char szModelFile [ _MAX_PATH ];
	DBOCalculateLoaderTempFolder();
	strcpy ( szModelFile, g_WindowsTempDirectory );
	strcat ( szModelFile, "dbotemp.x" );

	// Convert model file
	DB_Convert3DSBinary ( szFilename, szModelFile );

	// Load model file
	LoadDBO ( szModelFile, &g_pObject );

	// Delete temp model file
	DeleteFile ( szModelFile );

	// success
	return true;
}

DARKSDK bool Conv3DSDelete ( void )
{
	return true;
}

DARKSDK bool  Conv3DSConvert ( LPSTR pFilename, DWORD *pBlock, DWORD* pdwSize )
{
	// load object
	if ( !Load ( pFilename ) )
	{
		// Failed to load
		Conv3DSFree ( NULL );
		return false;
	}

	// create block
	if ( !DBOConvertObjectToBlock ( g_pObject, pBlock, pdwSize ) )
	{
		// Failed to create
		Conv3DSFree ( NULL );
		return false;
	}

	// okay
	return true;
}

DARKSDK void Conv3DSFree ( LPSTR pBlock )
{
	// delete block
	SAFE_DELETE(pBlock);

	// free object
	Conv3DSDelete();

	// free local DLL hook
	if ( g_hSetup )
	{
		FreeLibrary ( g_hSetup );
		g_hSetup = NULL;
	}
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <dos.h>

class CConvBuffer
{
	private:
		char  m_szData [ 50000000 ];
		DWORD dwSize;
		bool  bHeader;

	public:
		CConvBuffer  ( );
		~CConvBuffer ( );

		char* GetMem                      ( void );
		DWORD GetSize                     ( void );
		void  WriteHeader                 ( void );
		void  WriteFrameStart             ( char* szName );
		void  WriteFrameEnd               ( void );
		void  WriteMeshMatrix             ( float* pMatrix );
		void  WriteMeshStart              ( char* szName );
		void  WriteMeshEnd                ( void );
		void  WriteVertexData             ( int iNum, float* pfData );
		void  WriteFaceData               ( int iNum, DWORD* pdwData );
		void  WriteMeshMaterialList       ( int iMaterialNum, DWORD dwFaceNum, DWORD dwMaterialUsed, char* szMatName );
		void  WriteMeshTextureCoordsStart ( void );
		void  WriteTextureCoordsData      ( int iNum, float* pfData );
		void  WriteMeshTextureCoordsEnd   ( void );
		void  WriteMeshNormalsStart       ( void );
		void  WriteMeshNormalsEnd         ( void );
		void  WriteLine                   ( char* szString );
		void  WriteString                 ( char* szString );
		void  WriteMaterialItem           ( char* szMaterialName, float diffuse0, float diffuse1, float diffuse2, float diffuse3, float power,
											float specular0, float specular1, float specular2, float a, float b, float c, float percentage, char* szTextureName );
};

CConvBuffer::CConvBuffer ( )
{
	bHeader = false;
}

CConvBuffer::~CConvBuffer ( )
{

}

void CConvBuffer::WriteHeader ( void )
{
	strcpy ( m_szData, "xof 0302txt 0032\n" );
	strcat ( m_szData, "Header {\n" );
	strcat ( m_szData, "1;\n" );
	strcat ( m_szData, "0;\n" );
	strcat ( m_szData, "1;\n" );
	strcat ( m_szData, "}\n" );

	bHeader = true;
}

char* CConvBuffer::GetMem ( void )
{
	return m_szData;
}

DWORD CConvBuffer::GetSize ( void )
{
	return strlen ( m_szData ) * sizeof ( char );
}

void CConvBuffer::WriteFrameStart ( char* szName )
{
	if ( !bHeader )
		return;

	strcat ( m_szData, "Frame " );
	strcat ( m_szData, szName );
	strcat ( m_szData, " {\n" );
}

void CConvBuffer::WriteFrameEnd ( void )
{
	if ( !bHeader )
		return;

	strcat ( m_szData, "}\n" );
}

void CConvBuffer::WriteMeshMatrix ( float* pMatrix )
{
	if ( !bHeader )
		return;

	char szTemp [ 256 ];

	strcat ( m_szData, "FrameTransformMatrix {\n" );
	
	for ( int iMatrix = 0; iMatrix < 16; iMatrix++ )
	{
		sprintf ( szTemp, "%f", pMatrix [ iMatrix ] );
		strcat ( m_szData, szTemp );

		if ( iMatrix < 15 )
			strcat ( m_szData, "," );
		else
			strcat ( m_szData, ";;\n}\n" );

		if ( iMatrix < 15 )
		{
			if ( ( iMatrix + 1 ) % 4 == 0 )
				strcat ( m_szData, "\n" );
		}
	}
}

void CConvBuffer::WriteMeshStart ( char* szName )
{
	if ( !bHeader )
		return;

	strcat ( m_szData, "Mesh " );
	strcat ( m_szData, szName );
	strcat ( m_szData, " {\n" );
}

void CConvBuffer::WriteLine ( char* szString )
{
	strcat ( m_szData, szString );
	strcat ( m_szData, "\n" );
}

void CConvBuffer::WriteString ( char* szString )
{
	strcat ( m_szData, szString );
}

void CConvBuffer::WriteMeshEnd ( void )
{
	if ( !bHeader )
		return;

	strcat ( m_szData, "}\n" );
}

void CConvBuffer::WriteMeshNormalsStart ( void )
{
	if ( !bHeader )
		return;

	strcat ( m_szData, "MeshNormals " );
	strcat ( m_szData, " {\n" );
}

void CConvBuffer::WriteMeshNormalsEnd ( void )
{
	if ( !bHeader )
		return;

	strcat ( m_szData, "}\n" );
}

void CConvBuffer::WriteVertexData ( int iNum, float* pfData )
{
	if ( !bHeader )
		return;

	char szTemp [ 256 ];
	sprintf ( szTemp, "%i", iNum );

	strcat ( m_szData, szTemp );
	strcat ( m_szData, ";\n" );

	int iPos = 0;

	for ( int iVertex = 0; iVertex < iNum; iVertex++ )
	{
		sprintf ( szTemp, "%f", pfData [ iPos++ ] );
		strcat  ( m_szData, szTemp );
		strcat  ( m_szData, ";" );

		sprintf ( szTemp, "%f", pfData [ iPos++ ] );
		strcat  ( m_szData, szTemp );
		strcat  ( m_szData, ";" );

		sprintf ( szTemp, "%f", pfData [ iPos++ ] );
		strcat  ( m_szData, szTemp );

		if ( iVertex == iNum - 1 )
			strcat  ( m_szData, ";;\n" );
		else
			strcat  ( m_szData, ";,\n" );
	}
}

void CConvBuffer::WriteFaceData ( int iNum, DWORD* pdwData )
{
	if ( !bHeader )
		return;

	strcat ( m_szData, "\n" );

	char szTemp [ 256 ];

	sprintf ( szTemp, "%i", iNum );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, ";\n" );

	int iPos = 0;

	for ( int iIndex = 0; iIndex < iNum; iIndex++ )
	{
		strcat ( m_szData, "3;" );

		sprintf ( szTemp, "%i", pdwData [ iPos++ ] );
		strcat  ( m_szData, szTemp );
		strcat  ( m_szData, "," );

		sprintf ( szTemp, "%i", pdwData [ iPos++ ] );
		strcat  ( m_szData, szTemp );
		strcat  ( m_szData, "," );

		sprintf ( szTemp, "%i", pdwData [ iPos++ ] );
		strcat  ( m_szData, szTemp );

		if ( iIndex == iNum - 1 )
			strcat  ( m_szData, ";;\n" );
		else
			strcat  ( m_szData, ";,\n" );
	}
}

// lee - 040406 - u6rc6 - additional array for material face data
DBPRO_GLOBAL WORD*		g_pMesh_MaterialPerFace = NULL;
DBPRO_GLOBAL char**		g_pMesh_MaterialNameList = NULL;
DBPRO_GLOBAL int		g_iMesh_MaterialCounter = 0;


void CConvBuffer::WriteMeshMaterialList ( int iMaterialNum, DWORD dwFaceNum, DWORD dwMaterialUsed, char* szMatName )
{
	if ( !bHeader ) return;

	// materiallist
	strcat ( m_szData, "MeshMaterialList {\n" );

	// materiallist number of materials and number of faces
	char szTemp [ 256 ];
	sprintf ( szTemp, "%i", iMaterialNum );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, ";\n" );
	sprintf ( szTemp, "%i", dwFaceNum );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, ";\n" );

	// list of all material indexes for each face
	for ( int iIndex = 0; iIndex < (int)dwFaceNum; iIndex++ )
	{
		if ( g_pMesh_MaterialPerFace )
		{
			sprintf ( szTemp, "%i", g_pMesh_MaterialPerFace [ iIndex ] );
			strcat ( m_szData, szTemp );
		}
		else
		{
			sprintf ( szTemp, "%i", dwMaterialUsed );
			strcat ( m_szData, szTemp );
		}
		if ( iIndex == (int)dwFaceNum - 1 )
			strcat ( m_szData, ";;\n" );
		else
			strcat ( m_szData, ",\n" );
	}

	// write all materials from list collected
	if ( g_pMesh_MaterialNameList )
	{
		for ( int i=0; i<g_iMesh_MaterialCounter; i++ )
		{
			char* pThisMaterialName = g_pMesh_MaterialNameList [ i ];
			if ( pThisMaterialName )
			{
				for ( int iChar = 0; iChar < (int)strlen ( pThisMaterialName ); iChar++ )
				{
					if ( pThisMaterialName [ iChar ] == '#' ) pThisMaterialName [ iChar ] = '_';
					if ( pThisMaterialName [ iChar ] >= '0' && pThisMaterialName [ iChar ] <= '9' ) pThisMaterialName [ iChar ] = 'a' + (pThisMaterialName [ iChar ]-'0');
					if ( pThisMaterialName [ iChar ] == '-' ) pThisMaterialName [ iChar ] = '_';
				}
				strcat ( m_szData, "{" );
				strcat ( m_szData, pThisMaterialName );
				strcat ( m_szData, "}\n" );
			}
		}
	}

	// end materiallist
	strcat ( m_szData, "}\n" );

	// free material array 
	if ( g_pMesh_MaterialPerFace )
	{
		delete g_pMesh_MaterialPerFace;
		g_pMesh_MaterialPerFace=NULL;
	}
	if ( g_pMesh_MaterialNameList )
	{
		for ( int i=0; i<g_iMesh_MaterialCounter; i++ )
		{
			if ( g_pMesh_MaterialNameList [ i ] )
			{
				delete g_pMesh_MaterialNameList [ i ];
				g_pMesh_MaterialNameList [ i ] = NULL;
			}
		}
		delete g_pMesh_MaterialNameList;
		g_pMesh_MaterialNameList = NULL;
	}

	/* old way - material face data is important now
	char szTemp [ 256 ];

	sprintf ( szTemp, "%i", iMaterialNum );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, ";\n" );

	sprintf ( szTemp, "%i", dwFaceNum );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, ";\n" );

	for ( int iIndex = 0; iIndex < (int)dwFaceNum; iIndex++ )
	{
		sprintf ( szTemp, "%i", dwMaterialUsed );
		strcat ( m_szData, szTemp );

		

		if ( iIndex == (int)dwFaceNum - 1 )
			strcat ( m_szData, ";;\n" );
		else
			strcat ( m_szData, ",\n" );
	}


	// change matname if needed - no illegal characters (from other duplicate cde)
	for ( int iChar = 0; iChar < (int)strlen ( szMatName ); iChar++ )
	{
		// lee - 0404 06 - u6rc5 - removed break, so it converts all # to _
		if ( szMatName [ iChar ] == '#' ) szMatName [ iChar ] = '_';
		// lee - 040406 - u6rc5 - if material has a number, it cannot load it seems
		if ( szMatName [ iChar ] >= '0' && szMatName [ iChar ] <= '9' ) szMatName [ iChar ] = 'a' + (szMatName [ iChar ]-'0');
		if ( szMatName [ iChar ] == '-' ) szMatName [ iChar ] = '_';
	}
	//for ( int iChar = 0; iChar < (int)strlen ( szMatName ); iChar++ )
	//{
	//	if ( szMatName [ iChar ] == '#' )
	//		szMatName [ iChar ] = '_';
	//}

	strcat ( m_szData, "{" );
	strcat ( m_szData, szMatName );
	strcat ( m_szData, "}\n" );
	strcat ( m_szData, "}\n" );
	*/
}

void CConvBuffer::WriteMaterialItem ( char* szMaterialName, float diffuse0, float diffuse1, float diffuse2, float diffuse3, float power,
							 float specular0, float specular1, float specular2,
							 float a, float b, float c,
							 float percentage,
							 char* szTextureName
						)
{
	if ( !bHeader )
		return;

	// change matname if needed - no illegal characters
	for ( int iChar = 0; iChar < (int)strlen ( szMaterialName ); iChar++ )
	{
		// lee - 0404 06 - u6rc5 - removed break, so it converts all # to _
		if ( szMaterialName [ iChar ] == '#' ) szMaterialName [ iChar ] = '_';
		// lee - 040406 - u6rc5 - if material has a number, it cannot load it seems
		if ( szMaterialName [ iChar ] >= '0' && szMaterialName [ iChar ] <= '9' ) szMaterialName [ iChar ] = 'a' + (szMaterialName [ iChar ]-'0');
		if ( szMaterialName [ iChar ] == '-' ) szMaterialName [ iChar ] = '_';
	}

	strcat ( m_szData, "Material " );
	strcat ( m_szData, szMaterialName );
	strcat ( m_szData, " {\n" );

	char szTemp [ 256 ];

	sprintf ( szTemp, "%f", diffuse0 );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, "," );

	sprintf ( szTemp, "%f", diffuse1 );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, "," );

	sprintf ( szTemp, "%f", diffuse2 );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, "," );

	sprintf ( szTemp, "%f", diffuse3 );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, ";;\n" );

	sprintf ( szTemp, "%f", power );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, ";\n" );

	sprintf ( szTemp, "%f", specular0 );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, "," );

	sprintf ( szTemp, "%f", specular1 );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, "," );

	sprintf ( szTemp, "%f", specular2 );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, ";;\n" );

	sprintf ( szTemp, "%f", a );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, "," );

	sprintf ( szTemp, "%f", b );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, "," );

	sprintf ( szTemp, "%f", c );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, ";;\n" );

	strcat ( m_szData, "TextureFilename {\n" );
	strcat ( m_szData, "\"" );

	// lee - 040306 - u6rc5 - ensure texture filename does not exceed 255 (related systems need this limit)
//	strcat ( m_szData, szTextureName );
	int n = 0;
	char pLimitTextureName [ 256 ];
	int iRealMax = strlen ( szTextureName );
	for ( n=0; n<iRealMax; n++ )
	{
		pLimitTextureName [ n ] = szTextureName [ n ];
		if ( n==255 ) break;
	}
	pLimitTextureName [ n ] = 0;
	strcat ( m_szData, pLimitTextureName );

	strcat ( m_szData, "\";\n}\n" );

	strcat ( m_szData, "}\n" );
}

void CConvBuffer::WriteMeshTextureCoordsStart ( void )
{
	if ( !bHeader )
		return;

	strcat ( m_szData, "MeshTextureCoords {\n" );
}

void CConvBuffer::WriteTextureCoordsData ( int iNum, float* pfData )
{
	if ( !bHeader )
		return;

	char szTemp [ 256 ];

	sprintf ( szTemp, "%i", iNum );
	strcat ( m_szData, szTemp );
	strcat ( m_szData, ";\n" );

	int iPos = 0;

	for ( int iIndex = 0; iIndex < iNum; iIndex++ )
	{
		sprintf ( szTemp, "%f", pfData [ iPos++ ] );
		strcat ( m_szData, szTemp );
		strcat ( m_szData, ";" );

		sprintf ( szTemp, "%f", pfData [ iPos++ ] *= - 1);
		strcat ( m_szData, szTemp );
		strcat ( m_szData, ";" );

		if ( iIndex == iNum - 1 )
			strcat ( m_szData, ";\n" );
		else
			strcat ( m_szData, ",\n" );
	}
}

void CConvBuffer::WriteMeshTextureCoordsEnd ( void )
{
	if ( !bHeader )
		return;

	strcat ( m_szData, "}\n" );
}

// DEBUG MACRO SYSTEM
//#ifdef DEBUG
//#include "GlobalDebug.h"
//extern CGlobalDebug GlobalDebug;
//#define GlobalAlloc(x,y)	GlobalDebug.DebugGlobalAlloc(x,y);
//#define GlobalFree(x)		GlobalDebug.DebugGlobalFree(x);
//#endif


DBPRO_GLOBAL CConvBuffer* pConvertBuffer = NULL;

#include "directx-macros.h"

//>----- Tools

#define __DEBUG__          0


//>------ Types

#define INT_PERCENTAGE		0x0030
#define FLOAT_PERCENTAGE	0x0031

//>------ Id Chunk

#define MAIN3DS       0x4D4D

//>------ Main Chunks

#define EDIT3DS       0x3D3D  // this is the start of the editor config
#define KEYF3DS       0xB000  // this is the start of the keyframer config

//>------ sub defines of EDIT3DS

#define EDIT_MATERIAL 0xAFFF
#define EDIT_CONFIG1  0x0100
#define EDIT_CONFIG2  0x3E3D
#define EDIT_VIEW_P1  0x7012
#define EDIT_VIEW_P2  0x7011
#define EDIT_VIEW_P3  0x7020
#define EDIT_VIEW1    0x7001
#define EDIT_BACKGR   0x1200
#define EDIT_AMBIENT  0x2100
#define EDIT_OBJECT   0x4000

#define EDIT_UNKNW01  0x1100
#define EDIT_UNKNW02  0x1201
#define EDIT_UNKNW03  0x1300
#define EDIT_UNKNW04  0x1400
#define EDIT_UNKNW05  0x1420
#define EDIT_UNKNW06  0x1450
#define EDIT_UNKNW07  0x1500
#define EDIT_UNKNW08  0x2200
#define EDIT_UNKNW09  0x2201
#define EDIT_UNKNW10  0x2210
#define EDIT_UNKNW11  0x2300
#define EDIT_UNKNW12  0x2302 // new chunk type
#define EDIT_UNKNW13  0x3000
#define EDIT_UNKNW14  0xAFFF

//>------ sub defines of EDIT_MATERIAL
#define MAT_NAME01    0xA000  //> includes name (see mli doc for materials)
#define MAT_AMBIENT   0xA010 // Ambient Colour
#define MAT_DIFFUSE   0xA020 // Diffuse Colour	
#define MAT_SPECULAR  0xA030 // Specular
#define MAT_SHININESS 0xA040 // Power

#define MAT_TEXMAP    0xA200 // Texture Map
#define MAT_MAPNAME   0xA300 // Map Name

//>------ sub defines of EDIT_OBJECT

#define OBJ_TRIMESH   0x4100
#define OBJ_LIGHT     0x4600
#define OBJ_CAMERA    0x4700

#define OBJ_UNKNWN01  0x4010
#define OBJ_UNKNWN02  0x4012 //>>---- Could be shadow

//>------ sub defines of OBJ_CAMERA
#define CAM_UNKNWN01  0x4710 // new chunk type
#define CAM_UNKNWN02  0x4720 // new chunk type

//>------ sub defines of OBJ_LIGHT
#define LIT_OFF       0x4620
#define LIT_SPOT      0x4610
#define LIT_UNKNWN01  0x465A

//>------ sub defines of OBJ_TRIMESH
#define TRI_VERTEXL   0x4110
#define TRI_FACEL2    0x4111 // unknown yet
#define TRI_FACEL1    0x4120
#define TRI_MATGROUP  0x4130 
#define TRI_TEXVERTS  0x4140 
#define TRI_SMOOTH    0x4150
#define TRI_TRANSL    0x4160 
#define TRI_VISIBLE   0x4165


//>>------ sub defs of KEYF3DS
#define OBJ_FRAMES    0xB000

#define FRM_OBJECT_NODE_TAG 0xB002
#define KEYF_OBJDES   0xB002
#define FRM_KFSEG     0xB008
#define KEYF_FRAMES   0xB008
#define FRM_KFCURTIME 0xB009
#define KEYF_UNKNWN01 0xB009
#define FRM_KFHDR     0xB00A
#define KEYF_UNKNWN02 0xB00A

#define OBJDES_NAH    0xB010
#define OBJDES_DUMMY  0xB011
#define OBJDES_PRESCALE 0xB012
#define OBJDES_PIVOT  0xB013
#define OBJDES_POSTAG 0xB020
#define OBJDES_ROTTAG 0xB021
#define OBJDES_SCLTAG 0xB022
#define NODE_ID       0xB030

//>>------  these define the different color chunk types
#define COL_RGB  0x0010
#define COL_TRU  0x0011
#define COL_UNK  0x0013 // unknown

//>>------ defines for viewport chunks

#define TOP           0x0001
#define BOTTOM        0x0002
#define LEFT          0x0003
#define RIGHT         0x0004
#define FRONT         0x0005
#define BACK          0x0006
#define USER          0x0007
#define CAMERA        0x0008 // 0xFFFF is the code read from file
#define LIGHT         0x0009
#define DISABLED      0x0010
#define BOGUS         0x0011

//>------ global vars

char *viewports [11]={
                      "Bogus",
                      "Top",
                      "Bottom",
                      "Left",
                      "Right",
                      "Front",
                      "Back",
                      "User",
                      "Camera",
                      "Light",
                      "Disabled"
                     };

DBPRO_GLOBAL FILE					*bin3ds;
DBPRO_GLOBAL unsigned long			current_chunk = 0L;
DBPRO_GLOBAL unsigned char			views_read = 0;
DBPRO_GLOBAL unsigned int			numb_faces = 0, numb_vertices = 0;
DBPRO_GLOBAL char					temp_name[256];
DBPRO_GLOBAL char					temp2_name[256];
DBPRO_GLOBAL float					trans_mat[4][4]; // translation matrix for objects


// General state variables used for data transfer (conv construction)
DBPRO_GLOBAL float		g_colorR;
DBPRO_GLOBAL float		g_colorG;
DBPRO_GLOBAL float		g_colorB;
DBPRO_GLOBAL float		g_float1;
DBPRO_GLOBAL float		g_float2;
DBPRO_GLOBAL float		g_float3;
DBPRO_GLOBAL float		g_float4;

// Hierarchy State Data
DBPRO_GLOBAL bool		g_FindHierarchy;
DBPRO_GLOBAL short		g_HierarchyList[5000];
DBPRO_GLOBAL GGVECTOR3	g_HierarchyFramePivots[5000];
DBPRO_GLOBAL int			g_HierarchyIndex;
DBPRO_GLOBAL int			g_HierarchyMax;
DBPRO_GLOBAL int			g_HierarchyFrameList[5000];
DBPRO_GLOBAL D3DMATRIX	g_HierarchyFrameMatrices[5000];
DBPRO_GLOBAL int			g_HierarchyFrameDepth=0;
DBPRO_GLOBAL D3DMATRIX	g_HierarchyParentFrameMatrix;
DBPRO_GLOBAL D3DMATRIX	g_HierarchyFrameMatrix;
DBPRO_GLOBAL D3DMATRIX	g_HierarchyFrameMatrixStore;

// Mesh state variables
DBPRO_GLOBAL DWORD		g_Mesh_VertexNum = 0;
DBPRO_GLOBAL float*		g_Mesh_pVertexData = NULL;
DBPRO_GLOBAL DWORD		g_Mesh_FaceNum = 0;
DBPRO_GLOBAL DWORD*		g_Mesh_pFaceData = NULL;
DBPRO_GLOBAL DWORD*		g_Mesh_FaceDataForNormalsVertex = NULL;
DBPRO_GLOBAL DWORD		g_Mesh_MaterialUsed = 0;
DBPRO_GLOBAL DWORD		g_Mesh_NormalsNum = 0;
DBPRO_GLOBAL float*		g_Mesh_pNormalsData = NULL;
DBPRO_GLOBAL float*		g_Mesh_pTextureData = NULL;

// Animation Data Class
class CAnimDataFrame
{
	public:
		CAnimDataFrame();
		~CAnimDataFrame();

		void SetNext(CAnimDataFrame* pNext) { m_pNext = pNext; }
		CAnimDataFrame* GetNext(void) { return m_pNext; }
		void AddKeyFrame(CAnimDataFrame* pAnimDataFrame);

	private:
		CAnimDataFrame*		m_pNext;

	public:
		short				m_frameid;
		float				m_s;
		float				m_x;
		float				m_y;
		float				m_z;
};

CAnimDataFrame::CAnimDataFrame()
{
	m_pNext=NULL;
}
CAnimDataFrame::~CAnimDataFrame()
{
	if(m_pNext)
	{
		delete m_pNext;
		m_pNext=NULL;
	}
}
void CAnimDataFrame::AddKeyFrame(CAnimDataFrame* pAnimDataFrame)
{
	if(m_pNext)
		m_pNext->AddKeyFrame(pAnimDataFrame);
	else
		m_pNext = pAnimDataFrame;
}

class CAnimData
{
	public:
		CAnimData();
		~CAnimData();

		void SetNext(CAnimData* pNext) { m_pNext = pNext; }
		CAnimData* GetNext(void) { return m_pNext; }
		void AddAnimDataToList(CAnimData* pAnimData);

	private:
		CAnimData*			m_pNext;
		
	public:
		int					m_NodeId;
		char				m_Name[256];
		int					m_KeyPosQuantity;
		int					m_KeyRotQuantity;
		int					m_KeySclQuantity;
		CAnimDataFrame*		m_pFirstAnimPosFrame;
		CAnimDataFrame*		m_pFirstAnimRotFrame;
		CAnimDataFrame*		m_pFirstAnimSclFrame;
};

CAnimData::CAnimData()
{
	m_pNext=NULL;
	m_NodeId=-1;
	strcpy(m_Name,"");
	m_KeyPosQuantity=0;
	m_KeyRotQuantity=0;
	m_KeySclQuantity=0;
	m_pFirstAnimPosFrame=NULL;
	m_pFirstAnimRotFrame=NULL;
	m_pFirstAnimSclFrame=NULL;
}
CAnimData::~CAnimData()
{
	if(m_pNext)
	{
		delete m_pNext;
		m_pNext=NULL;
	}
	if(m_pFirstAnimPosFrame)
	{
		delete m_pFirstAnimPosFrame;
		m_pFirstAnimPosFrame=NULL;
	}
	if(m_pFirstAnimRotFrame)
	{
		delete m_pFirstAnimRotFrame;
		m_pFirstAnimRotFrame=NULL;
	}
	if(m_pFirstAnimSclFrame)
	{
		delete m_pFirstAnimSclFrame;
		m_pFirstAnimSclFrame=NULL;
	}
}
void CAnimData::AddAnimDataToList(CAnimData* pAnimData)
{
	if(m_pNext)
		m_pNext->AddAnimDataToList(pAnimData);
	else
		m_pNext = pAnimData;
}

DBPRO_GLOBAL CAnimData* g_pKeyframeAnimation = NULL;

/*----------------------------------------------------------------------------*/
DARKSDK unsigned char ReadChar (void)
{
 return (fgetc (bin3ds));
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned int ReadInt (void)
{
 unsigned char a=ReadChar ();
 unsigned char b=ReadChar ();
 return (a | (b << 8));
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadLong (void)
{
 unsigned long temp1,temp2;

 unsigned char a=ReadChar ();
 unsigned char b=ReadChar ();
 unsigned char c=ReadChar ();
 unsigned char d=ReadChar ();

 temp1=(a | (b << 8));
 temp2=(c | (d << 8));

 return (temp1+(temp2*0x10000L));
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadChunkPointer (void)
{
 return (ReadLong ());
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long GetChunkPointer (void)
{
 return (ftell (bin3ds)-2); // compensate for the already read Marker
}
/*----------------------------------------------------------------------------*/
DARKSDK void ChangeChunkPointer (unsigned long temp_pointer)
{
 fseek (bin3ds,temp_pointer,SEEK_SET);
}
/*----------------------------------------------------------------------------*/
DARKSDK int ReadName (void)
{
 unsigned int teller=0;
 unsigned char letter;

 strcpy (temp_name,"Default name");

 letter=ReadChar ();
 if (letter==0) return (-1); // dummy object

 if (letter==' ') letter='_';
 temp_name [teller]=letter;
 teller++;

 do
 {
  letter=ReadChar ();
  if (letter==' ') letter='_';
  temp_name [teller]=letter;
  teller++;
 }
 while ((letter!=0) && (teller<12));

 temp_name [teller-1]=0;

 #ifdef __DEBUG__
  printf ("     Found name : %s\n",temp_name);
 #endif
 return (0);
}
/*----------------------------------------------------------------------------*/
DARKSDK int ReadLongName (void)
{
 unsigned int teller=0;
 unsigned char letter;

 strcpy (temp_name,"Default name");

 letter=ReadChar ();
 if (letter==0) return (-1); // dummy object
 if (letter==' ') letter='_';
 temp_name [teller]=letter;
 teller++;

 do
 {
  letter=ReadChar ();
  if (letter==' ') letter='_';
  temp_name [teller]=letter;
  teller++;
 }
 while (letter!=0);

 temp_name [teller-1]=0;

 #ifdef __DEBUG__
   printf ("Found name : %s\n",temp_name);
 #endif
 return (0);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadUnknownChunk (unsigned int chunk_id)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;

 chunk_id=chunk_id;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadRGBColor (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;
 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 float rgb_val [3];
 for (int i=0;i<3;i++)
  fread (&(rgb_val [i]),sizeof (float),1,bin3ds);

 #ifdef __DEBUG__
 printf ("     Found Color (RGB) def of: R:%5.2f,G:%5.2f,B:%5.2f\n",
          rgb_val [0],
          rgb_val [1],
          rgb_val [2]);
 #endif

 // Copy to Global
 g_colorR = rgb_val[0];
 g_colorG = rgb_val[1];
 g_colorB = rgb_val[2];

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadTrueColor (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;
 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 unsigned char true_c_val [3];
 for (int i=0;i<3;i++)
  true_c_val [i]=ReadChar ();

 #ifdef __DEBUG__
 printf ("     Found Color (24bit) def of: R:%d,G:%d,B:%d\n",
          true_c_val [0],
          true_c_val [1],
          true_c_val [2]);
 #endif

 // Copy to Global
 g_colorR = (float)true_c_val[0]/255.0f;
 g_colorG = (float)true_c_val[1]/255.0f;
 g_colorB = (float)true_c_val[2]/255.0f;

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadIntPercentageChunk (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 g_float1 = (float)ReadInt();

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadFloatPercentageChunk (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 fread (&(g_float1),sizeof (float),1,bin3ds);

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadPercentageChunk (void)
{
 unsigned char end_found=FALSE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L; // 2 id + 4 pointer

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 while (end_found==FALSE)
 {
	temp_int=ReadInt();
	switch (temp_int)
	{
		case INT_PERCENTAGE :
			tellertje+=ReadIntPercentageChunk();			
			break;

		case FLOAT_PERCENTAGE :
			tellertje+=ReadFloatPercentageChunk();			
			break;

		default:
			break;
	}

	tellertje+=2;
	if (tellertje>=temp_pointer)
		end_found=TRUE;
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadBooleanChunk (unsigned char *boolean)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 *boolean=ReadChar ();

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadSpotChunk (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;
 float target [4];
 float hotspot,falloff;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 fread (&(target [0]),sizeof (float),1,bin3ds);
 fread (&(target [1]),sizeof (float),1,bin3ds);
 fread (&(target [2]),sizeof (float),1,bin3ds);
 fread (&hotspot,sizeof (float),1,bin3ds);
 fread (&falloff,sizeof (float),1,bin3ds);

 #ifdef __DEBUG__
 printf ("      The target of the spot is at: X:%5.2f Y:%5.2f Y:%5.2f\n",
          target [0],
          target [1],
          target [2]);
 printf ("      The hotspot of this light is : %5.2f\n",hotspot);
 printf ("      The falloff of this light is : %5.2f\n",falloff);
 #endif

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadLightChunk (void)
{
 unsigned char end_found=FALSE,boolean;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L; // 2 id + 4 pointer
 float light_coors [3];

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 fread (&(light_coors [0]),sizeof (float),1,bin3ds);
 fread (&(light_coors [1]),sizeof (float),1,bin3ds);
 fread (&(light_coors [2]),sizeof (float),1,bin3ds);

 #ifdef __DEBUG__
 printf ("     Found light at coordinates: X: %5.2f, Y: %5.2f,Z: %5.2f\n",
          light_coors [0],
          light_coors [1],
          light_coors [2]);
 #endif

 while (end_found==FALSE)
 {
   temp_int=ReadInt ();

       switch (temp_int)
       {
        case LIT_UNKNWN01 :
                           #ifdef __DEBUG__
                           printf (">>>>> Found Light unknown chunk id of %0X\n",LIT_UNKNWN01);
                           #endif
                           tellertje+=ReadUnknownChunk (LIT_UNKNWN01);
                           break;
        case LIT_OFF      :
                           #ifdef __DEBUG__
                           printf (">>>>> Light is (on/off) chunk: %0X\n",LIT_OFF);
                           #endif
                           tellertje+=ReadBooleanChunk (&boolean);
                           #ifdef __DEBUG__
                           if (boolean==TRUE)
                             printf ("      Light is on\n");
                           else
                             printf ("      Light is off\n");
                           #endif
                           break;
        case LIT_SPOT     :
                           #ifdef __DEBUG__
                           printf (">>>>> Light is SpotLight: %0X\n",TRI_VERTEXL);
                           #endif
                           tellertje+=ReadSpotChunk ();
                           break;
        case COL_RGB      :
                           #ifdef __DEBUG__
                           printf (">>>>> Found Color def (RGB) chunk id of %0X\n",temp_int);
                           #endif
                           tellertje+=ReadRGBColor ();
                           break;
        case COL_TRU      :
                           #ifdef __DEBUG__
                           printf (">>>>> Found Color def (24bit) chunk id of %0X\n",temp_int);
                           #endif
                           tellertje+=ReadTrueColor ();
                           break;
        default           :break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
     end_found=TRUE;
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadCameraChunk (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;
 float camera_eye [3];
 float camera_focus [3];
 float rotation,lens;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 fread (&(camera_eye [0]),sizeof (float),1,bin3ds);
 fread (&(camera_eye [1]),sizeof (float),1,bin3ds);
 fread (&(camera_eye [2]),sizeof (float),1,bin3ds);

 #ifdef __DEBUG__
 printf ("     Found Camera viewpoint at coordinates: X: %5.2f, Y: %5.2f,Z: %5.2f\n",
          camera_eye [0],
          camera_eye [1],
          camera_eye [2]);
 #endif

 fread (&(camera_focus [0]),sizeof (float),1,bin3ds);
 fread (&(camera_focus [1]),sizeof (float),1,bin3ds);
 fread (&(camera_focus [2]),sizeof (float),1,bin3ds);

 #ifdef __DEBUG__
 printf ("     Found Camera focus coors at coordinates: X: %5.2f, Y: %5.2f,Z: %5.2f\n",
          camera_focus [0],
          camera_focus [1],
          camera_focus [2]);
 #endif

 fread (&rotation,sizeof (float),1,bin3ds);
 fread (&lens,sizeof (float),1,bin3ds);
 #ifdef __DEBUG__
 printf ("     Rotation of camera is:  %5.4f\n",rotation);
 printf ("     Lens in used camera is: %5.4fmm\n",lens);
 #endif

 if ((temp_pointer-38)>0) // this means more chunks are to follow
 {
  #ifdef __DEBUG__
  printf ("     **** found extra cam chunks ****\n");
  #endif
  if (ReadInt ()==CAM_UNKNWN01)
  {
   #ifdef __DEBUG__
   printf ("     **** Found cam 1 type ch ****\n");
   #endif
   ReadUnknownChunk (CAM_UNKNWN01);
  }
  if (ReadInt ()==CAM_UNKNWN02)
  {
   #ifdef __DEBUG__
   printf ("     **** Found cam 2 type ch ****\n");
   #endif
   ReadUnknownChunk (CAM_UNKNWN02);
  }
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadVerticesChunk (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;
 float vertices [3]; // x,y,z

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();
 numb_vertices  =ReadInt ();

 #ifdef __DEBUG__
 printf ("      Found (%d) number of vertices\n",numb_vertices);
 #endif

 g_Mesh_VertexNum = numb_vertices;
 g_Mesh_pVertexData = (float*)GlobalAlloc(GMEM_FIXED, numb_vertices*3*sizeof(float));
 float* pWorkVert = g_Mesh_pVertexData;

 for (unsigned int i=0;i<numb_vertices;i++)
 {
  fread (&(vertices [0]),sizeof (float),1,bin3ds);
  fread (&(vertices [1]),sizeof (float),1,bin3ds);
  fread (&(vertices [2]),sizeof (float),1,bin3ds);

  *(pWorkVert++) = vertices[0];
  *(pWorkVert++) = vertices[2]; // Z > Y and Y > Z for proper 3DS > X conversion
  *(pWorkVert++) = vertices[1];
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadTextureVerticesChunk (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;
 float texverts[2]; // u,v

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();
 numb_vertices  =ReadInt ();

 g_Mesh_pTextureData = (float*)GlobalAlloc(GMEM_FIXED, numb_vertices*2*sizeof(float));
 float* pWorkVert = g_Mesh_pTextureData;

 for (unsigned int i=0;i<numb_vertices;i++)
 {
  fread (&(texverts [0]),sizeof (float),1,bin3ds);
  fread (&(texverts [1]),sizeof (float),1,bin3ds);

  *(pWorkVert++) = texverts[0];
  *(pWorkVert++) = texverts[1];
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/ DBPRO_GLOBAL unsigned long current_pointer;
DARKSDK unsigned long ReadSmoothingChunk ()
{
	unsigned long current_pointer;
	unsigned long temp_pointer;
	unsigned long smoothing;

	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	for (unsigned int i=0;i<g_Mesh_FaceNum;i++)
	{
		smoothing=ReadLong();
		smoothing=smoothing; // Sod knows what this is...maybe late :)
	}

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/ 
DARKSDK unsigned long ReadMatGroupChunk ()
{
	unsigned long current_pointer;
	unsigned long temp_pointer;
	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	// lee - 040406 - u6rc5 - extract cstr (material name), numfaces, array[index]=face to paint with material
	unsigned int teller=0;
	strcpy ( temp_name, "Default name" );
	unsigned char letter = ReadChar ();
	if ( letter==' ' ) letter='_';
	temp_name [ teller ] = letter;
	teller++;
	if ( letter>0 )
	{
		// first character not zero, must be material name here
		do
		{
			letter=ReadChar ();
			if ( letter==' ' ) letter='_';
			temp_name [teller]=letter;
			teller++;
		}
		while (letter!=0);
		temp_name [ teller-1 ] = 0;
	}

	// numfaces in array
	short nfaces = ReadInt();

	// read faceindices into array
	if ( g_Mesh_FaceNum > 0 && nfaces > 0 && g_FindHierarchy==false )
	{
		if ( g_pMesh_MaterialPerFace==NULL )
		{
			g_pMesh_MaterialPerFace = new WORD [ g_Mesh_FaceNum ];
			memset ( g_pMesh_MaterialPerFace, 0, sizeof(WORD) * g_Mesh_FaceNum );
			g_pMesh_MaterialNameList = new char* [ g_Mesh_FaceNum ];
			memset ( g_pMesh_MaterialNameList, 0, sizeof(char*) * g_Mesh_FaceNum );
			g_iMesh_MaterialCounter=0;
		}
		if ( g_pMesh_MaterialNameList )
		{
			g_pMesh_MaterialNameList [ g_iMesh_MaterialCounter ] = new char [ strlen(temp_name)+1 ];
			strcpy ( g_pMesh_MaterialNameList [ g_iMesh_MaterialCounter ], temp_name );
		}
		for ( int i=0; i<nfaces; i++ )
		{
			WORD wFaceIndex = ReadInt();
			g_pMesh_MaterialPerFace [ wFaceIndex ] = g_iMesh_MaterialCounter;
		}
		g_iMesh_MaterialCounter++;
	}

	/* old method pre-U6 - DBPro come along way, can now use material face index data!
	if(ReadLongName()==-1)
	{
	 // No material name found
	}
	// Ignore NumFace and FaceTable Data as DB does not texture individual faces
	*/

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK void ComputeVertexNormals(void)
{
	int   i0;
	int   i1;
	int   i2;
	int   iface;
	int   ivert;
	int   jp1;
	int   jp2;
	int   nfix;
	float norm;
	float x0;
	float x1;
	float x2;
	float xc;
	float y0;
	float y1;
	float y2;
	float yc;
	float z0;
	float z1;
	float z2;
	float zc;

	// Do not bother if no faces
	int num_face = g_Mesh_FaceNum;
	if ( num_face <= 0)
		return;

	// Construct normals from face flag data 0,1,0 or 1,0,0 or 0,0,1 etc
	int largestnumberofnormals = num_face * 3;
	g_Mesh_pNormalsData = (float*)GlobalAlloc(GMEM_FIXED, largestnumberofnormals*3*sizeof(float));
	g_Mesh_NormalsNum = 0;

	// Face Data required for proper normal face data
	g_Mesh_FaceDataForNormalsVertex = (DWORD*)GlobalAlloc(GMEM_FIXED, num_face*3*sizeof(DWORD));

	nfix = 0;
	int face_order_always = 3;
	for ( iface = 0; iface < num_face; iface++ )
	{
		for ( ivert = 0; ivert < face_order_always; ivert++ )
		{
			// Use the face data to point to the vertex, use it to get the vertex XYZ
			i0 = *(g_Mesh_pFaceData+(iface*3)+ivert);
			x0 = *(g_Mesh_pVertexData+(i0*3)+0);
			y0 = *(g_Mesh_pVertexData+(i0*3)+1);
			z0 = *(g_Mesh_pVertexData+(i0*3)+2);
//			i0 = face[ivert][iface];
//			x0 = cor3[0][i0];
//			y0 = cor3[1][i0];
//			z0 = cor3[2][i0];

			jp1 = ivert + 1;
			if ( jp1 >= face_order_always )
				jp1 = jp1 - face_order_always;

			i1 = *(g_Mesh_pFaceData+(iface*3)+jp1);
			x1 = *(g_Mesh_pVertexData+(i1*3)+0);
			y1 = *(g_Mesh_pVertexData+(i1*3)+1);
			z1 = *(g_Mesh_pVertexData+(i1*3)+2);
//			i1 = face[jp1][iface];
//			x1 = cor3[0][i1];
//			y1 = cor3[1][i1];
//			z1 = cor3[2][i1];

			jp2 = ivert + 2;
			if ( jp2 >= face_order_always )
				jp2 = jp2 - face_order_always;

			i2 = *(g_Mesh_pFaceData+(iface*3)+jp2);
			x2 = *(g_Mesh_pVertexData+(i2*3)+0);
			y2 = *(g_Mesh_pVertexData+(i2*3)+1);
			z2 = *(g_Mesh_pVertexData+(i2*3)+2);
//			i2 = face[jp2][iface];
//			x2 = cor3[0][i2];
//			y2 = cor3[1][i2];
//			z2 = cor3[2][i2];

			xc = ( y1 - y0 ) * ( z2 - z0 ) - ( z1 - z0 ) * ( y2 - y0 );
			yc = ( z1 - z0 ) * ( x2 - x0 ) - ( x1 - x0 ) * ( z2 - z0 );
			zc = ( x1 - x0 ) * ( y2 - y0 ) - ( y1 - y0 ) * ( x2 - x0 );

			norm = ( float ) sqrt ( xc * xc + yc * yc + zc * zc );

			if ( norm == 0.0 )
			{
				xc = ( float ) (1.0 / sqrt ( 3.0 ));
				yc = ( float ) (1.0 / sqrt ( 3.0 ));
				zc = ( float ) (1.0 / sqrt ( 3.0 ));
			}
			else
			{
				xc = xc / norm;
				yc = yc / norm;
				zc = zc / norm;
			}

			*(g_Mesh_pNormalsData+(g_Mesh_NormalsNum*3)+0) = xc;
			*(g_Mesh_pNormalsData+(g_Mesh_NormalsNum*3)+1) = yc;
			*(g_Mesh_pNormalsData+(g_Mesh_NormalsNum*3)+2) = zc;
//			vertex_normal[0][ivert][iface] = xc;
//			vertex_normal[1][ivert][iface] = yc;
//			vertex_normal[2][ivert][iface] = zc;

			// Normals Own Face Data
			*(g_Mesh_FaceDataForNormalsVertex+(iface*3)+ivert) = g_Mesh_NormalsNum;

			// Next normal..
			g_Mesh_NormalsNum++;
		}
	}
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadFacesChunk (void)
{
	unsigned long current_pointer;
	unsigned long temp_pointer;
	unsigned int temp_diff;
	unsigned int faces [6]; // a,b,c,Diff (Diff= AB: BC: CA: )

	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();
	numb_faces     =ReadInt ();

	// Plain Face Data pointing to vertice indexes
	g_Mesh_FaceNum = numb_faces;
	g_Mesh_pFaceData = (DWORD*)GlobalAlloc(GMEM_FIXED, numb_faces*3*sizeof(DWORD));
	DWORD* pWorkFace = g_Mesh_pFaceData;

	for (unsigned int i=0;i<numb_faces;i++)
	{
		faces [0]=ReadInt ();
		faces [1]=ReadInt ();
		faces [2]=ReadInt ();
		temp_diff=ReadInt () & 0x000F;
		faces [3]=(temp_diff & 0x0004) >> 2;
		faces [4]=(temp_diff & 0x0002) >> 1;
		faces [5]=(temp_diff & 0x0001);

		*(pWorkFace++) = faces[0];
		*(pWorkFace++) = faces[2]; // Face Reversal!! 3DS->X
		*(pWorkFace++) = faces[1];
	}

	// Now read other data within this chunk
 	unsigned char end_found=FALSE;
	unsigned int temp_int;
	unsigned long tellertje=6L; // 2 id + 4 pointer
	bool SmoothingDataFound=false;

	while (end_found==FALSE)
	{
		temp_int=ReadInt ();

		switch (temp_int)
		{
			case TRI_MATGROUP :
				tellertje+=ReadMatGroupChunk();
				break;

			case TRI_SMOOTH :
				tellertje+=ReadSmoothingChunk();
				SmoothingDataFound=true;
				break;

			default:		  
				tellertje+=ReadUnknownChunk(0); 
				break;
		}

		tellertje+=2;
		if (tellertje>=temp_pointer)
			end_found=TRUE;
	}

	// Work out normals data based on vertex data
	ComputeVertexNormals();

	/*
	if(SmoothingDataFound==false)
	{
		// Construct normals from face flag data 0,1,0 or 1,0,0 or 0,0,1 etc
		g_Mesh_pNormalsData = (float*)GlobalAlloc(GMEM_FIXED, g_Mesh_VertexNum*3*sizeof(float));
		float* pWorkNorm = g_Mesh_pNormalsData;

		for(i=0;i<g_Mesh_VertexNum;i++)
		{
			*(pWorkNorm++) = 1.0f-(float)faces[3];//may need to read each face norm, not just last one
			*(pWorkNorm++) = 1.0f-(float)faces[4];
			*(pWorkNorm++) = 1.0f-(float)faces[5];
		}
	}
	*/

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadTranslationChunk (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;
 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 for (int j=0;j<4;j++)
 {
   for (int i=0;i<3;i++)
    fread (&(trans_mat [j][i]),sizeof (float),1,bin3ds);
 }

 // 3DS > X conversion requires transpositions of Z and Y on both matrix axiums (er, I made up that last word :)
 // mike - 020206 - addition for vs8
 //for (j=0;j<4;j++)
 for (int j=0;j<4;j++)
 {
	float old = trans_mat[j][1];
	trans_mat[j][1] = trans_mat[j][2];
	trans_mat[j][2] = old;
 }
 for (int i=0;i<3;i++)
 {
	float old = trans_mat[1][i];
	trans_mat[1][i] = trans_mat[2][i];
	trans_mat[2][i] = old;
 }

 trans_mat [0][3]=0;
 trans_mat [1][3]=0;
 trans_mat [2][3]=0;
 trans_mat [3][3]=1;

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK void ReverseMatrixOnVector(D3DMATRIX* pMat, GGVECTOR3* pVec)
{
	// Get Position
	GGVECTOR3 vPos = GGVECTOR3(pMat->_41, pMat->_42, pMat->_43);

	// Deduct position from vector
	pVec->x -= vPos.x;
	pVec->y -= vPos.y;
	pVec->z -= vPos.z;
}

DARKSDK void OffsetVertexData(D3DMATRIX Offset)
{
	float* pWorkVert = g_Mesh_pVertexData;
	for (unsigned int i=0;i<g_Mesh_VertexNum;i++)
	{
		GGVECTOR3 vVec = GGVECTOR3(*(pWorkVert+0), *(pWorkVert+1), *(pWorkVert+2));

		// Revert Vector using Abs.Matrix
		D3DMath_VectorMatrixMultiply(vVec, vVec, Offset);

		*(pWorkVert++) = vVec.x;
		*(pWorkVert++) = vVec.y;
		*(pWorkVert++) = vVec.z;
	}
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadObjChunk (void)
{
	unsigned char end_found=FALSE;
	unsigned int temp_int;
	unsigned long current_pointer;
	unsigned long temp_pointer;
	unsigned long tellertje=6L; // 2 id + 4 pointer

	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	// INIT Mesh Item
	char MeshName[256];
	char store_name[256];
	strcpy(MeshName, temp_name);
	char MaterialNameUsed[256];
	strcpy(MaterialNameUsed, "");
	bool bMeshMatrixFound=false;

	g_Mesh_VertexNum = 0;
	g_Mesh_pVertexData = NULL;
	g_Mesh_FaceNum = 0;
	g_Mesh_pFaceData = NULL;
	g_Mesh_FaceDataForNormalsVertex = NULL;
	g_Mesh_MaterialUsed = 0;    /// optionally assigned by 0x4130 (implement!!)
	g_Mesh_NormalsNum = 0;
	g_Mesh_pNormalsData = NULL;
	g_Mesh_pTextureData = NULL;

	// Set default identity matrix
	D3DMATRIX DefaultMatrix;
	D3DUtil_SetIdentityMatrix(DefaultMatrix);
	for(int tx=0; tx<4; tx++)
		for(int ty=0; ty<4; ty++)
			trans_mat[tx][ty] = DefaultMatrix.m[tx][ty];

	while (end_found==FALSE)
	{
		temp_int=ReadInt ();

		switch (temp_int)
		{
			case TRI_VERTEXL :
				tellertje+=ReadVerticesChunk ();
				break;

			case TRI_TEXVERTS :
				tellertje+=ReadTextureVerticesChunk ();
				break;

			case TRI_TRANSL  :
				tellertje+=ReadTranslationChunk();
				bMeshMatrixFound=true;
				break;

			case TRI_FACEL1  :
				strcpy(store_name, temp_name);
				strcpy(temp_name, "");
				tellertje+=ReadFacesChunk();
				strcpy(MaterialNameUsed, temp_name);
				strcpy(temp_name, store_name);
				break;

			default:		  
				tellertje+=ReadUnknownChunk(0); 
				break;
		}

		tellertje+=2;
		if (tellertje>=temp_pointer)
			end_found=TRUE;
	}

	if(g_FindHierarchy==false)
	{
		// CREATE Mesh Item
		D3DMATRIX AbsoluteMatrix;
		for(int tx=0; tx<4; tx++)
			for(int ty=0; ty<4; ty++)
				AbsoluteMatrix.m[tx][ty] = trans_mat[tx][ty];

		// Create Inverted Mesh Matrix (affected by pivot)
		D3DMATRIX LocalMatrix;
		D3DMath_MatrixInvert(LocalMatrix, AbsoluteMatrix);
		LocalMatrix._41 -= g_HierarchyFramePivots[1+g_HierarchyIndex].x;
		LocalMatrix._42 -= g_HierarchyFramePivots[1+g_HierarchyIndex].y;
		LocalMatrix._43 -= g_HierarchyFramePivots[1+g_HierarchyIndex].z;

		// MESH DATA CORRECTION (Only requires inverted local mesh & pivot)
		OffsetVertexData(LocalMatrix);

		// Manage correct hierarchy traversal for next mesh..
		D3DMATRIX NewTransMatrix, WorkMatrix;
		D3DMATRIX ExistingTransMatrix = AbsoluteMatrix;
		D3DMath_MatrixInvert(WorkMatrix, g_HierarchyFrameMatrix);
//		D3DMath_MatrixMultiply(NewTransMatrix, WorkMatrix, ExistingTransMatrix);
		D3DMath_MatrixMultiply(NewTransMatrix, ExistingTransMatrix, WorkMatrix);
		g_HierarchyFrameMatrixStore = AbsoluteMatrix;

		// Update trans_mat
		// mike - 020206 - addition for vs8
		//for(tx=0; tx<4; tx++)
		for(int tx=0; tx<4; tx++)
			for(int ty=0; ty<4; ty++)
				trans_mat[tx][ty] = NewTransMatrix.m[tx][ty];
	}

	// Only if mesh has polygons..
	if(g_Mesh_pVertexData)
	{
		
		// Mesh Matrix
		if(bMeshMatrixFound==true) pConvertBuffer->WriteMeshMatrix((float*)trans_mat);

		// Mesh Start
		if(atoi(MeshName)>0) wsprintf(MeshName, "x%s", MeshName);

		pConvertBuffer->WriteMeshStart(MeshName);

		
		// Vertex Entry
		if(g_Mesh_pVertexData) pConvertBuffer->WriteVertexData(g_Mesh_VertexNum, g_Mesh_pVertexData);

		// Faces Entry
		if(g_Mesh_pFaceData) pConvertBuffer->WriteFaceData(g_Mesh_FaceNum, g_Mesh_pFaceData);

		// Use MaterialList
		if(strcmp(MaterialNameUsed,"")!=0)
		{
			// MaterialList Entry
			int mat_num = 1;
			char mat_name[256];
			strcpy(mat_name, MaterialNameUsed);
			pConvertBuffer->WriteMeshMaterialList(mat_num, g_Mesh_FaceNum, g_Mesh_MaterialUsed, mat_name);
		}

		// Normals Entry
		if(g_Mesh_pNormalsData)
		{
			pConvertBuffer->WriteMeshNormalsStart();
			pConvertBuffer->WriteVertexData(g_Mesh_NormalsNum, g_Mesh_pNormalsData);
			pConvertBuffer->WriteFaceData(g_Mesh_FaceNum, g_Mesh_FaceDataForNormalsVertex);
			pConvertBuffer->WriteMeshNormalsEnd();
			
		}

		// Texture Entry
		if(g_Mesh_pTextureData)
		{
			pConvertBuffer->WriteMeshTextureCoordsStart();
			pConvertBuffer->WriteTextureCoordsData(g_Mesh_VertexNum, g_Mesh_pTextureData);
			pConvertBuffer->WriteMeshTextureCoordsEnd();
			
		}

		// End
		pConvertBuffer->WriteMeshEnd();
	}

	// DELETE Mesh Data
	if(g_Mesh_pVertexData)
	{
		GlobalFree(g_Mesh_pVertexData);
		g_Mesh_pVertexData=NULL;
	}
	if(g_Mesh_pFaceData)
	{
		GlobalFree(g_Mesh_pFaceData);
		g_Mesh_pFaceData=NULL;
	}
	if(g_Mesh_FaceDataForNormalsVertex)
	{
		GlobalFree(g_Mesh_FaceDataForNormalsVertex);
		g_Mesh_FaceDataForNormalsVertex=NULL;
	}
	if(g_Mesh_pNormalsData)
	{
		GlobalFree(g_Mesh_pNormalsData);
		g_Mesh_pNormalsData=NULL;
	}
	if(g_Mesh_pTextureData)
	{
		GlobalFree(g_Mesh_pTextureData);
		g_Mesh_pTextureData=NULL;
	}

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
DARKSDK unsigned long ReadFrameChunk (void)
{
	unsigned char end_found=FALSE;
	unsigned int temp_int;
	unsigned long current_pointer;
	unsigned long temp_pointer;
	unsigned long tellertje=6L; // 2 id + 4 pointer

	current_pointer=GetChunkPointer ();
	temp_pointer   =ReadChunkPointer ();

	while (end_found==FALSE)
	{
		temp_int=ReadInt ();

		switch (temp_int)
		{
			case FRM_KFHDR :
				tellertje+=ReadUnknownChunk(0);
				break;

			case FRM_KFSEG :
				tellertje+=ReadFrameChunk();
				break;

			case FRM_KFCURTIME : 
				tellertje+=ReadUnknownChunk(0);
				break;

			case FRM_OBJECT_NODE_TAG :
				tellertje+=ReadUnknownChunk(0);
				break;

			default:          
				break;
		}

		tellertje+=2;
		if (tellertje>=temp_pointer)
			end_found=TRUE;
	}

	ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
	return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
void StartFrameEntry ( LPSTR temp_name )
{
 pConvertBuffer->WriteFrameStart(temp_name);
 if(g_FindHierarchy==false)
 {
	 g_HierarchyFrameMatrix = g_HierarchyFrameMatrixStore;
	 g_HierarchyFrameList[1+g_HierarchyIndex]=g_HierarchyFrameDepth;
 	 g_HierarchyFrameMatrices[g_HierarchyFrameDepth]=g_HierarchyFrameMatrix;
 }
}

