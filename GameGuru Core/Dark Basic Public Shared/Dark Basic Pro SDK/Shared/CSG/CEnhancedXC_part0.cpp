#include "cenhancedxc.h"
#include ".\cerror.h"
#include ".\cdatac.h"

#define DEBUG_MODE	0




#define ANIM_SIMPLE 0
#define ANIM_LOOP   1
#define ANIM_CYCLE  2

#define ANIM_DIR_FORWARD 0
#define ANIM_DIR_BACK	 1
#define ANIM_DIR_END	 2

//////////////////////////////////////////////////////////////////////////////
// variable declarations /////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
tagModelData*		m_pModelData;

CData				m_List;
CData				m_MeshList;
float				m_fTimer;
LPDIRECT3DDEVICE8	m_pD3D;
D3DXMATRIX*			m_matObject;
char				m_szPath [ 256 ];

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// function pointer types ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
typedef void	          ( *RetVoidFunctionPointerPFN ) ( ... );
typedef bool	          ( *RetBoolFunctionPointerPFN ) ( ... );
typedef int 	          ( *RetIntFunctionPointerPFN  ) ( ... );
typedef char		      ( *RetCharFunctionPointerPFN ) ( ... );
typedef IDirect3DDevice8* ( *RetD3DFunctionPointerPFN  ) ( ... );
typedef LPDIRECT3DTEXTURE8	( *IMAGE_RetLPD3DTEX8ParamInt3PFN )  ( int, int, int );
typedef LPDIRECT3DTEXTURE8	( *IMAGE_RetLPD3DTEX8ParamIntPFN )  ( int );
typedef bool				( *IMAGE_LoadDirectPFN )  ( char* szFilename, LPDIRECT3DTEXTURE8* pImage );

///////////////////////////////////////////////////////////////////////////////////////////////
/// image /////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
/*
HINSTANCE						g_Image;

RetVoidFunctionPointerPFN		g_Image_Constructor;
RetVoidFunctionPointerPFN		g_Image_Destructor;

RetBoolFunctionPointerPFN		g_Image_Load;
IMAGE_LoadDirectPFN				g_Image_LoadDirect;

RetBoolFunctionPointerPFN		g_Image_Save;
RetVoidFunctionPointerPFN		g_Image_Delete;
IMAGE_RetLPD3DTEX8ParamInt3PFN	g_Image_Make;

RetIntFunctionPointerPFN		g_Image_GetWidth;
RetIntFunctionPointerPFN		g_Image_GetHeight;
RetBoolFunctionPointerPFN		g_Image_GetExist;

RetVoidFunctionPointerPFN		g_Image_SetSharing;
RetVoidFunctionPointerPFN		g_Image_SetMemory;

RetVoidFunctionPointerPFN		g_Image_Lock;
RetVoidFunctionPointerPFN		g_Image_Unlock;
RetVoidFunctionPointerPFN		g_Image_Write;
RetVoidFunctionPointerPFN		g_Image_Get;

RetVoidFunctionPointerPFN		g_Image_SetMipmapMode;
RetVoidFunctionPointerPFN		g_Image_SetMipmapType;
RetVoidFunctionPointerPFN		g_Image_SetMipmapBias;
RetVoidFunctionPointerPFN		g_Image_SetMipmapNum;

RetVoidFunctionPointerPFN		g_Image_SetColorKey;
RetVoidFunctionPointerPFN		g_Image_SetTranslucency;

IMAGE_RetLPD3DTEX8ParamIntPFN	g_Image_GetPointer;					// get pointer to image data, useful to external apps
*/
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
/*
typedef IDirect3DDevice8* ( *GFX_GetDirect3DDevicePFN ) ( void );

HINSTANCE					g_GFX;							// for dll loading

GFX_GetDirect3DDevicePFN	g_GFX_GetDirect3DDevice;	// get pointer to D3D device
*/
//////////////////////////////////////////////////////////////////////////

int							m_iCurrentID;
D3DMATERIAL8				gWhiteMaterial;

void Update ( int iID, D3DXMATRIX* matrix )
{
	m_pModelData              = ( tagModelData* ) m_List.Get ( iID );
	if(m_pModelData) m_pModelData->m_matObject = matrix;
}

void Constructor ( LPDIRECT3DDEVICE8 lpDevice )
{
	m_pD3D = lpDevice;

	/*
	g_GFX = hSetup;
	g_Image = hImage;
	if ( !hSetup || !hImage )
	{
		hSetup = LoadLibrary ( "DBProSetupDebug.dll" );		// load the setup library
		hImage = LoadLibrary ( "DBProImageDebug.dll" );		// load the image library
	}

	g_Image_Constructor          = ( RetVoidFunctionPointerPFN )		GetProcAddress ( hImage, "?Constructor@@YAXPAUHINSTANCE__@@@Z" );
	g_Image_Destructor           = ( RetVoidFunctionPointerPFN )		GetProcAddress ( hImage, "?Destructor@@YAXXZ" );

	g_Image_Load                 = ( RetBoolFunctionPointerPFN )		GetProcAddress ( hImage, "?Load@@YA_NPADH@Z" );
	g_Image_LoadDirect           = ( IMAGE_LoadDirectPFN )				GetProcAddress ( hImage, "?LoadDirect@@YA_NPADPAPAUIDirect3DTexture8@@@Z" );

	g_Image_Save                 = ( RetBoolFunctionPointerPFN )		GetProcAddress ( hImage, "?Save@@YA_NHPAD@Z" );
	g_Image_Delete               = ( RetVoidFunctionPointerPFN )		GetProcAddress ( hImage, "?Delete@@YAXH@Z" );
	g_Image_Make                 = ( IMAGE_RetLPD3DTEX8ParamInt3PFN )	GetProcAddress ( hImage, "?Make@@YAPAUIDirect3DTexture8@@HHH@Z" );
	
	g_Image_GetWidth             = ( RetIntFunctionPointerPFN )			GetProcAddress ( hImage, "?GetWidth@@YAHH@Z" );
	g_Image_GetHeight            = ( RetIntFunctionPointerPFN )			GetProcAddress ( hImage, "?GetHeight@@YAHH@Z" );
	g_Image_GetExist             = ( RetBoolFunctionPointerPFN )		GetProcAddress ( hImage, "?GetExist@@YA_NH@Z" );

	g_Image_GetPointer           = ( IMAGE_RetLPD3DTEX8ParamIntPFN )	GetProcAddress ( hImage, "?GetPointer@@YAPAUIDirect3DTexture8@@H@Z" );

	g_Image_SetSharing           = ( RetVoidFunctionPointerPFN )		GetProcAddress ( hImage, "?SetSharing@@YAX_N@Z" );
	g_Image_SetMemory            = ( RetVoidFunctionPointerPFN )		GetProcAddress ( hImage, "?SetMemory@@YAXH@Z" );

	g_Image_Lock                 = ( RetVoidFunctionPointerPFN )		GetProcAddress ( hImage, "?Lock@@YAXH@Z" );
	g_Image_Unlock               = ( RetVoidFunctionPointerPFN )		GetProcAddress ( hImage, "?Unlock@@YAXH@Z" );
	g_Image_Write                = ( RetVoidFunctionPointerPFN )		GetProcAddress ( hImage, "?Write@@YAXHHHHHHH@Z" );
	g_Image_Get                  = ( RetVoidFunctionPointerPFN )		GetProcAddress ( hImage, "?Get@@YAXHHHPAH00@Z" );

	g_Image_SetMipmapMode        = ( RetVoidFunctionPointerPFN )		GetProcAddress ( hImage, "?SetMipmapMode@@YAX_N@Z" );
	g_Image_SetMipmapType        = ( RetVoidFunctionPointerPFN )		GetProcAddress ( hImage, "?SetMipmapType@@YAXH@Z" );
	g_Image_SetMipmapBias        = ( RetVoidFunctionPointerPFN )		GetProcAddress ( hImage, "?SetMipmapBias@@YAXM@Z" );
	g_Image_SetMipmapNum         = ( RetVoidFunctionPointerPFN )		GetProcAddress ( hImage, "?SetMipmapNum@@YAXH@Z" );
	
	g_Image_SetColorKey          = ( RetVoidFunctionPointerPFN )		GetProcAddress ( hImage, "?SetColorKey@@YAXHHH@Z" );
	g_Image_SetTranslucency      = ( RetVoidFunctionPointerPFN )		GetProcAddress ( hImage, "?SetTranslucency@@YAXHH@Z" );

	g_GFX_GetDirect3DDevice = ( GFX_GetDirect3DDevicePFN ) GetProcAddress ( hSetup, "?GetDirect3DDevice@@YAPAUIDirect3DDevice8@@XZ" );
	m_pD3D                  = g_GFX_GetDirect3DDevice ( );
	*/

	memset ( m_szPath, 0, sizeof ( m_szPath ) );
	m_fTimer = 0.0f;

	// Prepare a pure white material for texture usage
	memset ( &gWhiteMaterial, 0, sizeof ( gWhiteMaterial ) );
	gWhiteMaterial.Diffuse.r = 1.0f;
	gWhiteMaterial.Diffuse.g = 1.0f;
	gWhiteMaterial.Diffuse.b = 1.0f;
	gWhiteMaterial.Diffuse.a = 1.0f;	
	gWhiteMaterial.Ambient.r = 1.0f;
	gWhiteMaterial.Ambient.g = 1.0f;
	gWhiteMaterial.Ambient.b = 1.0f;
	gWhiteMaterial.Ambient.a = 1.0f;
}

void Destructor ( void )
{
	// get rid of any meshes
	link* pCheck = m_MeshList.m_start;
	while(pCheck)
	{
		tagModelData* ptr = NULL;
		ptr = ( tagModelData* ) m_MeshList.Get ( pCheck->id );
		if ( ptr == NULL ) continue;

		SAFE_DELETE ( ptr->m_Object.m_Frames );
		SAFE_DELETE ( ptr->m_Object.m_Meshes );
		SAFE_DELETE ( ptr );

		pCheck = pCheck->next;
	}
	m_MeshList.DeleteAll ( );

	// get rid of any models
	pCheck = m_List.m_start;
	while(pCheck)
	{
		tagModelData* ptr = NULL;
		ptr = ( tagModelData* ) m_List.Get ( pCheck->id );
		if ( ptr == NULL ) continue;

		SAFE_DELETE ( ptr->m_Object.m_Frames );
		SAFE_DELETE ( ptr->m_Object.m_Meshes );
		SAFE_DELETE ( ptr );

		pCheck = pCheck->next;
	}
	m_List.DeleteAll ( );
}

void RefreshD3D ( int iMode )
{
	if(iMode==0)
	{
		// Remove all traces of old D3D usage
		Destructor();
	}
	if(iMode==1)
	{
		// Get new D3D and recreate everything D3D related
//		Constructor ( g_GFX, g_Image );
	}
}

void LoadMesh ( int iMeshID, char* szFilename )
{
	// Model Number Zero used as temp loading area (for mesh conversion)
	int iTempModelNumber=0;

	// variable definitions
	int iCharCount = 0;
	tagModelData modelData;
	memset ( &modelData, 0, sizeof ( modelData ) );
	for ( int iChar = strlen ( szFilename ); iChar > 0; iChar--, iCharCount++ )
	{
		if ( szFilename [ iChar ] == '\\' || szFilename [ iChar ] == '/' )
		{
			memcpy ( modelData.m_szPath, szFilename, sizeof ( char ) * ( strlen ( szFilename ) - iCharCount + 1 ) );
			break;
		}
	}

	m_pModelData = &modelData;

	///////////////////////////////////////
	// check if an object already exists //
	// with the same id, if it does then //
	// delete it                         //
	///////////////////////////////////////
	tagModelData* ptr = NULL;
	ptr = ( tagModelData* ) m_List.Get ( iTempModelNumber );
	if ( ptr != NULL )
	{
		m_List.Delete ( iTempModelNumber );
	}

	///////////////////////////////////////
	// create a new object and insert in //
	// the list                          //
	///////////////////////////////////////
	tagModelData* test;
	test = new tagModelData;
	memset ( test,          0, sizeof ( tagModelData ) );
	memcpy ( test, &modelData, sizeof ( tagModelData ) );
	m_List.Add ( iTempModelNumber, ( VOID* ) test, 0, 1 );
	///////////////////////////////////////

	// update internal model data
	m_pModelData = ( tagModelData* ) m_List.Get ( iTempModelNumber );
	if ( LoadModelData ( &m_pModelData->m_Object, szFilename, modelData.m_szPath, false ) )
	{
		// setup animation properties
		m_pModelData->m_bAnimPlaying = false;															// anim flag, off by default
		m_pModelData->m_fAnimSpeed   = 1.0f;																// current speed
		m_pModelData->m_dwLastTime   = timeGetTime ( );													// initial time
		m_pModelData->m_dwThisTime   = timeGetTime ( );													// initial time
		m_pModelData->m_fFrame       = 0.0f;																// current frame
		m_pModelData->m_iFrameJump   = 1;
		m_pModelData->m_iAnimDirection = ANIM_DIR_FORWARD;
		m_pModelData->m_iAnimMode = ANIM_SIMPLE;

		// when model loaded, convert to single mesh
		MakeMeshFromObject ( iMeshID, iTempModelNumber );

		// delete model data
		SAFE_DELETE ( test->m_Object.m_Frames );
		SAFE_DELETE ( test->m_Object.m_Meshes );
		SAFE_DELETE ( test );
		m_List.Delete ( iTempModelNumber );

		return;
	}
	else
	{
		if ( test != NULL )
		{
			SAFE_DELETE ( test->m_Object.m_Frames );
			SAFE_DELETE ( test->m_Object.m_Meshes );
			SAFE_DELETE ( test );
			m_List.Delete ( iTempModelNumber );
		}

		return;
	}
}

void DeleteMesh ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_MeshList.Get ( iID ) ) )
		return;

	SAFE_DELETE ( m_pModelData->m_Object.m_Frames );
	SAFE_DELETE ( m_pModelData->m_Object.m_Meshes );
	SAFE_DELETE ( m_pModelData );
	m_MeshList.Delete ( iID );
}

void SaveMesh ( int iMeshID, char* szFilename )
{
	// get mesh object
	if ( ! ( m_pModelData = ( tagModelData* ) m_MeshList.Get ( iMeshID ) ) )
		return;

	// get mesh info
	sMesh* pMesh = m_pModelData->m_Object.m_Meshes;

	// create D£DXMATERIAL array
	D3DXMATERIAL* pMatList = new D3DXMATERIAL[pMesh->m_NumMaterials];
	for(DWORD m=0; m<pMesh->m_NumMaterials; m++)
	{
		pMatList[m].MatD3D=pMesh->m_Materials[m];
		pMatList[m].pTextureFilename=NULL;
	}

	// save mesh to an X file
	HRESULT hRes = D3DXSaveMeshToX( szFilename, pMesh->m_Mesh, NULL, pMatList, pMesh->m_NumMaterials, DXFILEFORMAT_TEXT );

	// free usages
	SAFE_DELETE(pMatList);
}

void ChangeMesh	( int iObjectID, int iLimbID, int iMeshID )
{
	// variable declarations
	tagModelData* pObject		= NULL;	// source object
	tagModelData* pMeshObj		= NULL;	// mesh object
	sMesh* pActualMesh			= NULL;	// mesh data

	// get source
	if ( ! ( pObject = ( tagModelData* ) m_List.Get ( iObjectID ) ) )
		return;

	// get mesh object
	if ( ! ( pMeshObj = ( tagModelData* ) m_MeshList.Get ( iMeshID ) ) )
		return;

	// get first mesh from mesh object
	if ( ! ( pActualMesh = pMeshObj->m_Object.m_Meshes ) )
		return;

	// use objects FVF to control mesh creation
	DWORD dwInFVF = pObject->m_Object.m_dwFVF;
	DWORD dwInFVFSize = pObject->m_Object.m_dwFVFSize;

	// determine info on sourcemesh
	DWORD dwInNumPoly=pActualMesh->m_pAttributeTable [ 0 ].FaceCount;
	DWORD dwInNumVert=pActualMesh->m_pAttributeTable [ 0 ].VertexCount;

	// clone sourcemesh
	ID3DXMesh* pNewMesh;
	pActualMesh->m_Mesh->CloneMeshFVF ( 0, dwInFVF, m_pD3D, &pNewMesh );

	// create area for tempmeshdata
	float* pDataFromNewMesh;
	float* pTempMeshDataRaw = (float*)new char[dwInNumVert*dwInFVFSize];
	if ( SUCCEEDED ( pNewMesh->LockVertexBuffer ( D3DLOCK_NOSYSLOCK, ( BYTE** ) &pDataFromNewMesh ) ) )
	{
		memcpy(pTempMeshDataRaw, pDataFromNewMesh, dwInFVFSize*dwInNumVert);
		pNewMesh->UnlockVertexBuffer ( );
	}
	WORD* pIndiceFromNewMesh;
	WORD* pTempIndiceData = new WORD[dwInNumPoly*3];
	if ( SUCCEEDED ( pNewMesh->LockIndexBuffer ( D3DLOCK_NOSYSLOCK, ( BYTE** ) &pIndiceFromNewMesh ) ) )
	{
		memcpy(pTempIndiceData, pIndiceFromNewMesh, dwInNumPoly*3*sizeof(WORD));
		pNewMesh->UnlockIndexBuffer ( );
	}

	// Make mesh data not depend on indices
	DWORD dwNewVertCount=0;
	float* pTempMeshData = CreatePureTriangleMeshData( pTempMeshDataRaw, &dwNewVertCount, dwInNumVert, dwInFVFSize, pTempIndiceData, dwInNumPoly );

	// free temps so far
	SAFE_DELETE(pTempMeshDataRaw);
	SAFE_DELETE(pTempIndiceData);

	// find frame(limb) holdnig mesh(es)
	sFrame* pLimbFrame = pObject->m_Object.m_Frames->FindFrame ( iLimbID );

	// delete the mesh in the object
	sMesh* pStoreNeighborMesh=NULL;
	sFrameMeshList* List = pLimbFrame->m_MeshList;
	while ( List != NULL )
	{
		// Next List Item
		sFrameMeshList* NextList = List->m_Next;

		// Delete mesh(es)
		if ( List->m_Mesh )
		{
			pStoreNeighborMesh = List->m_Mesh->m_Next;
			List->m_Mesh->m_Next=NULL;
			delete List->m_Mesh;
		}

		// Delete framelist item
		if ( List->m_Next )
		{
			List->m_Next=NULL;
			delete List;
		}

		// next mesh in list
		List = NextList;
	}

	// create a new mesh
	sMesh* Mesh = MakeMeshFromData ( dwInFVF, dwInFVFSize, pTempMeshData, dwNewVertCount, D3DPT_TRIANGLELIST );

	// restore neighbor mesh
	Mesh->m_Next=pStoreNeighborMesh;

	// copy mesh data from mesh item to new object mesh
	sMesh* pReplacingMesh = pLimbFrame->m_MeshList->m_Mesh;
	pLimbFrame->m_MeshList->m_Mesh = Mesh;
	pLimbFrame->m_MeshList->m_Next = NULL;

	// replace reference to old mesh if found in meshlist
	if(pObject->m_Object.m_Meshes==pReplacingMesh)
		pObject->m_Object.m_Meshes=Mesh;
	else
	{
		sMesh* pCurrent = pObject->m_Object.m_Meshes;
		while(pCurrent)
		{
			sMesh* pNextMesh = pCurrent->m_Next;
			if(pNextMesh==pReplacingMesh)
			{
				pCurrent->m_Next = Mesh;
				break;
			}
			pCurrent=pNextMesh;
		}
	}

	// Free usages
	SAFE_DELETE(pTempMeshData);
	SAFE_RELEASE(pNewMesh);
}

bool MakeFromMesh (		int iID, DWORD dwInFVF, DWORD dwInFVFSize,
						float* pInMesh, DWORD dwInNumPoly, DWORD dwInNumVert, DWORD dwInPrimType )
{
	///////////////////////////////////////
	// check if an object already exists //
	// with the same id, if it does then //
	// delete it                         //
	///////////////////////////////////////
	tagModelData* ptr = NULL;
	ptr = ( tagModelData* ) m_List.Get ( iID );
	if ( ptr != NULL )
	{
		SAFE_DELETE ( ptr->m_Object.m_Frames );
		SAFE_DELETE ( ptr->m_Object.m_Meshes );
		SAFE_DELETE ( ptr );
		m_List.Delete ( iID );
	}
	///////////////////////////////////////

	///////////////////////////////////////
	// create a new object and insert in //
	// the list                          //
	///////////////////////////////////////
	tagModelData* test;
	test = new tagModelData;
	memset ( test,          0, sizeof ( tagModelData ) );
	m_List.Add ( iID, ( VOID* ) test, 0, 1 );
	///////////////////////////////////////

	// update internal model data
	m_pModelData = ( tagModelData* ) m_List.Get ( iID );
	if ( MakeModelData ( &m_pModelData->m_Object, dwInFVF, dwInFVFSize, pInMesh, dwInNumPoly, dwInNumVert, dwInPrimType ) )
	{
		// setup animation properties
		m_pModelData->m_bAnimPlaying = false;															// anim flag, off by default
		m_pModelData->m_fAnimSpeed   = 0.0f;																// current speed
		m_pModelData->m_dwLastTime   = timeGetTime ( );													// initial time
		m_pModelData->m_dwThisTime   = timeGetTime ( );													// initial time
		m_pModelData->m_fFrame       = 0.0f;																// current frame
		m_pModelData->m_iFrameJump   = 0;
		m_pModelData->m_iAnimDirection = ANIM_DIR_FORWARD;
		m_pModelData->m_iAnimMode = ANIM_SIMPLE;
		
		// Complete
		return true;
	}
	else
	{
		// Failed to load, remove from list
		if ( test != NULL )
		{
			SAFE_DELETE ( test->m_Object.m_Frames );
			SAFE_DELETE ( test->m_Object.m_Meshes );
			SAFE_DELETE ( test );
			m_List.Delete ( iID );
		}

		// Failed
		return false;
	}
}

void NewObjectFromMesh ( int iObjectID, int iMeshID )
{
	// variable declarations
	tagModelData* pMeshObj		= NULL;	// mesh object
	sMesh* pActualMesh			= NULL;	// mesh data

	// get mesh object
	if ( ! ( pMeshObj = ( tagModelData* ) m_MeshList.Get ( iMeshID ) ) )
		return;

	// get first mesh from mesh object
	if ( ! ( pActualMesh = pMeshObj->m_Object.m_Meshes ) )
		return;
	
	// use mesh FVF to control objectmesh creation
	DWORD dwInFVF = pMeshObj->m_Object.m_dwFVF;
	DWORD dwInFVFSize = pMeshObj->m_Object.m_dwFVFSize;
	DWORD dwInPrimType = pActualMesh->m_iPrimType;

	// determine info on sourcemesh
	DWORD dwInNumPoly=pActualMesh->m_pAttributeTable [ 0 ].FaceCount;
	DWORD dwInNumVert=pActualMesh->m_pAttributeTable [ 0 ].VertexCount;

	// create copy of meshdata
	float* pDataFromNewMesh;
	float* pInMesh = (float*)new char[dwInNumVert*dwInFVFSize];
	if ( SUCCEEDED ( pActualMesh->m_Mesh->LockVertexBuffer ( D3DLOCK_NOSYSLOCK, ( BYTE** ) &pDataFromNewMesh ) ) )
	{
		memcpy(pInMesh, pDataFromNewMesh, dwInNumVert*dwInFVFSize);
		pActualMesh->m_Mesh->UnlockVertexBuffer ( );
	}

	// make object from meshdata
	MakeFromMesh ( iObjectID, dwInFVF, dwInFVFSize, pInMesh, dwInNumPoly, dwInNumVert, dwInPrimType );

	// free usage
	SAFE_DELETE(pInMesh);
}

bool MakeMeshFromMesh (	int iID, DWORD dwInFVF, DWORD dwInFVFSize,
						float* pInMesh, DWORD dwInNumPoly, DWORD dwInNumVert, DWORD dwInPrimType )
{
	///////////////////////////////////////
	// check if an object already exists //
	// with the same id, if it does then //
	// delete it                         //
	///////////////////////////////////////
	tagModelData* ptr = NULL;
	ptr = ( tagModelData* ) m_MeshList.Get ( iID );
	if ( ptr != NULL )
	{
		SAFE_DELETE ( ptr->m_Object.m_Frames );
		SAFE_DELETE ( ptr->m_Object.m_Meshes );
		SAFE_DELETE ( ptr );
		m_MeshList.Delete ( iID );
	}
	///////////////////////////////////////

	///////////////////////////////////////
	// create a new object and insert in //
	// the list                          //
	///////////////////////////////////////
	tagModelData* test;
	test = new tagModelData;
	memset ( test, 0, sizeof ( tagModelData ) );
	m_MeshList.Add ( iID, ( VOID* ) test, 0, 1 );
	///////////////////////////////////////

	// update internal model data
	m_pModelData = ( tagModelData* ) m_MeshList.Get ( iID );
	if ( MakeModelData ( &m_pModelData->m_Object, dwInFVF, dwInFVFSize, pInMesh, dwInNumPoly, dwInNumVert, dwInPrimType ) )
	{
		// setup animation properties
		m_pModelData->m_bAnimPlaying = false;															// anim flag, off by default
		m_pModelData->m_fAnimSpeed   = 1.0f;																// current speed
		m_pModelData->m_dwLastTime   = timeGetTime ( );													// initial time
		m_pModelData->m_dwThisTime   = timeGetTime ( );													// initial time
		m_pModelData->m_fFrame       = 0.0f;																// current frame
		m_pModelData->m_iFrameJump   = 1;
		m_pModelData->m_iAnimDirection = ANIM_DIR_FORWARD;
		m_pModelData->m_iAnimMode = ANIM_SIMPLE;
		
		// Complete
		return true;
	}
	else
	{
		// Failed to load, remove from list
		if ( test != NULL )
		{
			SAFE_DELETE ( test->m_Object.m_Frames );
			SAFE_DELETE ( test->m_Object.m_Meshes );
			SAFE_DELETE ( test );
			m_MeshList.Delete ( iID );
		}

		// Failed
		return false;
	}
}

void MakeMeshFromObjectPart ( int iMeshID, int iObjectID, int iLimbPart )
{
	// makes a mesh from an object
	tagModelData* pObject = NULL;
	if ( ! ( pObject = ( tagModelData* ) m_List.Get ( iObjectID ) ) )
		return;

	// use objects FVF to control mesh creation
	DWORD dwFVF = pObject->m_Object.m_dwFVF;
	DWORD dwFVFSize = pObject->m_Object.m_dwFVFSize;

	// prepare temp mesh data array
	DWORD dwVertMax = 512;
	DWORD dwVertsSoFar = 0;
	float* pTempMeshData = (float*)new char[dwVertMax*dwFVFSize];
	float* pMeshDataPtr = pTempMeshData;

	// prepare temp indice data array
	DWORD dwPolyMax = 512;
	DWORD dwPolysSoFar = 0;
	WORD* pTempIndiceData = new WORD[dwPolyMax*3];
	WORD* pIndiceDataPtr = pTempIndiceData;

	// retain scale and rotation, but not position of main object
	bool bLocalMatrix=false;
	D3DXMATRIX RecordMatrix;
	if(*pObject->m_matObject)
	{
		RecordMatrix = *pObject->m_matObject;
		(*pObject->m_matObject)._41=0.0f;
		(*pObject->m_matObject)._42=0.0f;
		(*pObject->m_matObject)._43=0.0f;
	}
	else
	{
		D3DXMatrixIdentity(&RecordMatrix);
		pObject->m_matObject=&RecordMatrix;
		bLocalMatrix=true;
	}

	// convert and add all meshes to single mesh (as though drawing it)
	D3DXMATRIX Matrix;
	D3DXMatrixIdentity ( &Matrix );
	UpdateAnimation ( &pObject->m_Object, pObject->m_fFrame, TRUE );
	UpdateFrame ( &pObject->m_Object, pObject->m_Object.m_Frames, &Matrix );
	pObject->m_Object.m_Meshes->CopyFrameToBoneMatrices ( );
	
	// build mesh - all frame meshes
	pObject->m_Object.m_matObject = pObject->m_matObject;
	BuildMesh ( &pObject->m_Object, pObject->m_Object.m_Frames, dwFVF, dwFVFSize,
				&dwVertMax, &pTempMeshData, &pMeshDataPtr, &dwVertsSoFar,
				&dwPolyMax, &pTempIndiceData, &pIndiceDataPtr, &dwPolysSoFar, iLimbPart );

	// restore object world position
	if(bLocalMatrix)
		pObject->m_matObject = NULL;
	else
		*pObject->m_matObject = RecordMatrix;

	// make mesh data not depend on indices
	DWORD dwNewVertCount=0;
	float* pNoIndexRequiredMesh = CreatePureTriangleMeshData( pTempMeshData, &dwNewVertCount, dwVertsSoFar, dwFVFSize, pTempIndiceData, dwPolysSoFar );
	SAFE_DELETE(pTempMeshData);

	// create new mesh-object from single mesh
	MakeMeshFromMesh ( iMeshID, dwFVF, dwFVFSize, pNoIndexRequiredMesh, dwNewVertCount/3, dwNewVertCount, D3DPT_TRIANGLELIST );

	// free usages
	SAFE_DELETE(pTempMeshData);
	SAFE_DELETE(pTempIndiceData);
	SAFE_DELETE(pNoIndexRequiredMesh);
}

void NewObjectFromLimb ( int iNewObjectID, int iSrcObjectID, int iLimbID )
{
	// create temp mesh from limb
	int iTempMesh=0;
	MakeMeshFromObjectPart ( iTempMesh, iSrcObjectID, iLimbID );

	// create object from mesh
	NewObjectFromMesh ( iNewObjectID, iTempMesh );

	// free mesh
	tagModelData* ptr = NULL;
	ptr = ( tagModelData* ) m_MeshList.Get ( iTempMesh );
	if ( ptr != NULL )
	{
		SAFE_DELETE ( ptr->m_Object.m_Frames );
		SAFE_DELETE ( ptr->m_Object.m_Meshes );
		SAFE_DELETE ( ptr );
	}
	m_MeshList.Delete ( iTempMesh );
}

void MakeMeshFromObject ( int iMeshID, int iObjectID )
{
	int iNoPart=-1;
	MakeMeshFromObjectPart( iMeshID, iObjectID, iNoPart );
}

int GetMeshExist ( int iID )
{	
	if ( ( m_pModelData = ( tagModelData* ) m_MeshList.Get ( iID ) ) )
		return 1;
	else
		return 0;
}

tagModelData* Load ( int iID, char* szFilename )
{
	// variable definitions
	tagModelData			modelData;
	memset ( &modelData, 0, sizeof ( modelData ) );

	int iCharCount = 0;
	for ( int iChar = strlen ( szFilename ); iChar > 0; iChar--, iCharCount++ )
	{
		// run through the characters in the filename and find
		// the first occurrence of a slash, then we know we have
		// hit the part of the filename we're looking for - the path
		if ( szFilename [ iChar ] == '\\' || szFilename [ iChar ] == '/' )
		{
			memcpy ( modelData.m_szPath, szFilename, sizeof ( char ) * ( strlen ( szFilename ) - iCharCount + 1 ) );
			break;
		}
	}

	modelData.lpTexture = NULL;
	m_pModelData = &modelData;

	///////////////////////////////////////
	// check if an object already exists //
	// with the same id, if it does then //
	// delete it                         //
	///////////////////////////////////////
	tagModelData* ptr = NULL;
	ptr = ( tagModelData* ) m_List.Get ( iID );
	if ( ptr != NULL )
	{
		SAFE_DELETE ( ptr->m_Object.m_Frames );
		SAFE_DELETE ( ptr->m_Object.m_Meshes );
		SAFE_DELETE ( ptr );
		m_List.Delete ( iID );
	}

	///////////////////////////////////////

	///////////////////////////////////////
	// create a new object and insert in //
	// the list                          //
	///////////////////////////////////////
	tagModelData* test;
	test = new tagModelData;
	memset ( test,          0, sizeof ( tagModelData ) );
	memcpy ( test, &modelData, sizeof ( tagModelData ) );
	m_List.Add ( iID, ( VOID* ) test, 0, 1 );
	///////////////////////////////////////

	// update internal model data
	m_pModelData = ( tagModelData* ) m_List.Get ( iID );
	if ( LoadModelData ( &m_pModelData->m_Object, szFilename, modelData.m_szPath, true ) )
	{
		// setup animation properties
		m_pModelData->m_bAnimPlaying = false;															// anim flag, off by default
		m_pModelData->m_fAnimSpeed   = 1.0f;																// current speed
		m_pModelData->m_dwLastTime   = timeGetTime ( );													// initial time
		m_pModelData->m_dwThisTime   = timeGetTime ( );													// initial time
		m_pModelData->m_fFrame       = 0.0f;																// current frame
		m_pModelData->m_iFrameJump   = 1;
		m_pModelData->m_iAnimDirection = ANIM_DIR_FORWARD;

		m_pModelData->m_Object.m_bOverrideVertexShader = false;
		m_pModelData->m_Object.m_dwVertexShader		   = 0;

		if ( m_pModelData->m_Object.m_Animations.m_AnimationSet )
		{
			m_pModelData->m_iFrameCount = m_pModelData->m_Object.m_Animations.m_AnimationSet->m_Length;	// number of frames
			m_pModelData->m_iEndFrame   = m_pModelData->m_Object.m_Animations.m_AnimationSet->m_Length;
			m_pModelData->m_iStartFrame = 0;
		}

		m_pModelData->m_iAnimMode = ANIM_SIMPLE;

		/////////////////////////////////////////////////
		/////////////////////////////////////////////////

		// we now need to work out bounding boxes for all of the limbs
		sMesh* pMesh = m_pModelData->m_Object.m_Meshes;

		for ( int iTemp = 0; iTemp < m_pModelData->m_Object.m_NumMeshes; iTemp++ )
		{
			BYTE* pVertices;

			pMesh->m_Mesh->LockVertexBuffer ( D3DLOCK_READONLY, &pVertices );

			D3DXComputeBoundingBox ( 
										pVertices,
										pMesh->m_Mesh->GetNumVertices ( ),
										pMesh->m_Mesh->GetFVF ( ),
										&pMesh->m_vecMin,
										&pMesh->m_vecMax
								   );

			pMesh->m_Mesh->UnlockVertexBuffer ( );

			#if DEBUG_MODE_MESH
			if ( pMesh->m_pMeshBox )
			{
				// only create a box model if debug mode is on
				D3DXMATRIX matTrans;
				D3DXMATRIX matScale;

				D3DXMatrixTranslation (
											&matTrans,
											( pMesh->m_vecMin.x + pMesh->m_vecMax.x ) / 2,
											( pMesh->m_vecMin.y + pMesh->m_vecMax.y ) / 2,
											( pMesh->m_vecMin.z + pMesh->m_vecMax.z ) / 2
									  );

				D3DXMatrixScaling (
										&matScale,
										pMesh->m_vecMax.x - pMesh->m_vecMin.x,
										pMesh->m_vecMax.y - pMesh->m_vecMin.y,
										pMesh->m_vecMax.z - pMesh->m_vecMin.z
								  );

				D3DXCreateBox ( m_pD3D, 1.0f, 1.0f, 1.0f, &pMesh->m_pMeshBox, NULL );
				D3DXComputeNormals ( pMesh->m_pMeshBox, NULL );

				pMesh->m_matBox = matScale * matTrans;
			}
			#endif

			// next mesh
			pMesh = pMesh->m_Next;
		}
		
		// this is used to create the main bounding box, it's a bad way of doing it,
		// as it loads in the model again, every time I try and work it out manually the
		// bounding box isn't in the right position, or completely screws up, it's really
		// annoying me now............ it should be really easy to do aswell.....
		LPD3DXBUFFER	pD3DXMtrlBuffer;
		DWORD			dwNumMaterials;
		LPD3DXMESH		pTempMesh;
		BYTE*			pVertices;

		D3DXLoadMeshFromX ( szFilename, D3DXMESH_SYSTEMMEM, m_pD3D, NULL, &pD3DXMtrlBuffer, &dwNumMaterials, &pTempMesh );

		pTempMesh->LockVertexBuffer ( D3DLOCK_READONLY, &pVertices );
        
		D3DXComputeBoundingBox ( 
									pVertices,
									pTempMesh->GetNumVertices ( ),
									pTempMesh->GetFVF ( ),
									&m_pModelData->m_Object.m_Min,
									&m_pModelData->m_Object.m_Max
							   );
    
		pTempMesh->UnlockVertexBuffer ( );

		SAFE_RELEASE ( pTempMesh );
		SAFE_RELEASE ( pD3DXMtrlBuffer );
		
		/////////////////////////////////////////////////
	

		
		#if DEBUG_MODE
		if ( m_pModelData->m_pMeshBox )
		{
			D3DXMATRIX matTrans;
			D3DXMATRIX matScale;

			D3DXMatrixTranslation (
										&matTrans,
										( m_pModelData->m_Object.m_Min.x + m_pModelData->m_Object.m_Max.x ) / 2, 
										( m_pModelData->m_Object.m_Min.y + m_pModelData->m_Object.m_Max.y ) / 2, 
										( m_pModelData->m_Object.m_Min.z + m_pModelData->m_Object.m_Max.z ) / 2 
								);

			D3DXMatrixScaling (
								&matScale,
								m_pModelData->m_Object.m_Max.x - m_pModelData->m_Object.m_Min.x,
								m_pModelData->m_Object.m_Max.y - m_pModelData->m_Object.m_Min.y,
								m_pModelData->m_Object.m_Max.z - m_pModelData->m_Object.m_Min.z
							  );

			m_pModelData->m_matBox = matScale * matTrans;

			D3DXCreateBox ( m_pD3D, 1.0f, 1.0f, 1.0f, &m_pModelData->m_pMeshBox, NULL );
			D3DXComputeNormals ( m_pModelData->m_pMeshBox, NULL );
		}
		#endif
		

		// Complete
		return m_pModelData;
	}
	else
	{
		// Failed to load, remove from list
		if ( test != NULL )
		{
			m_List.Delete ( iID );
			SAFE_DELETE ( test->m_Object.m_Frames );
			SAFE_DELETE ( test->m_Object.m_Meshes );
			SAFE_DELETE ( test );
		}

		// Failed
		return NULL;
	}
}

bool Delete ( int iID )
{
	// update internal model data
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	SAFE_DELETE ( m_pModelData->m_Object.m_Frames );
	SAFE_DELETE ( m_pModelData->m_Object.m_Meshes );
	SAFE_DELETE ( m_pModelData );
	m_List.Delete ( iID );
	
	return true;
}

void SetVertexShaderOn ( int iID, DWORD dwShader )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	m_pModelData->m_Object.m_bOverrideVertexShader = true;
	m_pModelData->m_Object.m_dwVertexShader		   = dwShader;
}

void SetVertexShaderOff ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	m_pModelData->m_Object.m_bOverrideVertexShader = false;
	m_pModelData->m_Object.m_dwVertexShader		   = 0;
}

bool AppendAnimationFromFile ( int iID, char* szFilename, int iFrame )
{
	// Get Object
	tagModelData* ptr = NULL;
	ptr = ( tagModelData* ) m_List.Get ( iID );
	if ( ptr==NULL)
		return false;

	// Add animation only from file
	if ( AppendAnimationData ( &m_pModelData->m_Object, szFilename, iFrame ) )
	{
		m_pModelData->m_iFrameCount = m_pModelData->m_Object.m_Animations.m_AnimationSet->m_Length;
		m_pModelData->m_iEndFrame   = m_pModelData->m_Object.m_Animations.m_AnimationSet->m_Length;
	}
	else
		return false;

	// Complete
	return true;
}

void SetSphereRadius ( int iID, float fRadius )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	m_pModelData->m_Object.m_Radius = fRadius;
}

float GetRadius ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return 0.0f;

	return m_pModelData->m_Object.m_Radius;
}

void UpdateTimer ( void )
{

}

void Play ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	m_pModelData->m_bAnimPlaying = true;
}

void Play ( int iID, int iStart )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	m_pModelData->m_bAnimPlaying = true;
	m_pModelData->m_fFrame       = (float)iStart;
}

void Play ( int iID, int iStart, int iEnd )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	m_pModelData->m_bAnimPlaying = true;
	m_pModelData->m_fFrame       = (float)iStart;

	m_pModelData->m_iStartFrame  = iStart;
	m_pModelData->m_iEndFrame    = iEnd;
}

void Loop ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	m_pModelData->m_bAnimPlaying = true;
	//m_pModelData->m_bAnimLoop    = true;
	m_pModelData->m_iAnimMode = ANIM_LOOP;
}

void Loop ( int iID, int iStart )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	m_pModelData->m_iStartFrame  = iStart;
	m_pModelData->m_bAnimPlaying = true;
	m_pModelData->m_fFrame       = (float)iStart;
	m_pModelData->m_iAnimMode = ANIM_LOOP;
}

void Loop ( int iID, int iStart, int iEnd )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	m_pModelData->m_bAnimPlaying = true;
	m_pModelData->m_fFrame       = (float)iStart;

	m_pModelData->m_iStartFrame  = iStart;
	m_pModelData->m_iEndFrame    = iEnd;

	m_pModelData->m_iAnimMode = ANIM_LOOP;
}

float GetMinX ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return 0.0f;

	return m_pModelData->m_Object.m_Min.x;
}

float GetMinY ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return 0.0f;

	return m_pModelData->m_Object.m_Min.y;
}

float GetMinZ ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return 0.0f;

	return m_pModelData->m_Object.m_Min.z;
}

float GetMaxX ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return 0.0f;

	return m_pModelData->m_Object.m_Max.x;
}

float GetMaxY ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return 0.0f;

	return m_pModelData->m_Object.m_Max.y;
}

float GetMaxZ ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return 0.0f;

	return m_pModelData->m_Object.m_Max.z;
}

void Stop ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	m_pModelData->m_bAnimPlaying = false;
}

void SetAnimationMode ( int iID, int iMode )
{
	m_pModelData->m_iAnimMode = iMode;
}

void SetSpeed ( int iID, int iSpeed )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	m_pModelData->m_iFrameJump = iSpeed;
}

void SetFrame ( int iID, int iFrame )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	m_pModelData->m_fFrame = (float)iFrame;
}

void SetFrame ( int iID, float fFrame )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	m_pModelData->m_fFrame = fFrame;
}

void SetInterpolation ( int iID, int iJump )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	m_pModelData->m_fAnimSpeed = (float)iJump;
}

int GetAnimationCount ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	return m_pModelData->m_iFrameCount;
}

bool GetPlaying ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	return m_pModelData->m_bAnimPlaying;
}

bool GetLooping ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	if ( m_pModelData->m_iAnimMode == ANIM_LOOP )
		return true;
	else
		return false;
}

int GetFrame ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return -1;

	return (int)m_pModelData->m_fFrame;
}

int GetSpeed ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return -1;

	return m_pModelData->m_iFrameJump;
}

int GetInterpolation ( int iID )
{
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return -1;

	return (int)m_pModelData->m_fAnimSpeed;
}

void ClearAllKeyFrames ( int iID )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	SAFE_DELETE ( m_pModelData->m_Object.m_Animations.m_AnimationSet );
}

void ClearKeyFrame ( int iID, int iFrame )
{
	// clear a key frame of animation

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// variable declarations
	sAnimation** pAnimation = NULL;		// a list of pointers to pointers
	sAnimation* pFrame      = NULL;		// pointer to initial frame data
	int			iTemp       = 0;		// used for loops

	// allocate a list of pointers to pointers for the keyframes
	if ( ! ( pAnimation = new sAnimation* [ m_pModelData->m_Object.m_Animations.m_AnimationSet->m_Length ] ) )
		Error ( "Failed to allocate key frame pointer list" );

	// get the start frame
	pFrame = m_pModelData->m_Object.m_Animations.m_AnimationSet->m_Animation;

	// run through all frames and store pointers to data
	for ( iTemp = 0; iTemp < (int)m_pModelData->m_Object.m_Animations.m_AnimationSet->m_Length; iTemp++ )
	{
		// store the current frame
		pAnimation [ iTemp ] = pFrame;

		// jump to the next frame
		if ( pFrame->m_Next )
			pFrame = pFrame->m_Next;
	}

	// see which frame we're at
	if ( iFrame == 0 )
	{
		// update first frame link
		m_pModelData->m_Object.m_Animations.m_AnimationSet->m_Animation = pAnimation [ 1 ];
	}
	else if ( iFrame > 0 && iFrame < (int)m_pModelData->m_Object.m_Animations.m_AnimationSet->m_Length )
	{
		// update in between 2 frames
		pAnimation [ iFrame - 1 ]->m_Next = pAnimation [ iFrame + 1 ];
	}
	else if ( iFrame == (int)m_pModelData->m_Object.m_Animations.m_AnimationSet->m_Length )
	{
		// update last frame
		pAnimation [ iFrame - 1 ]->m_Next = NULL;
	}
	
	// minus the length
	m_pModelData->m_Object.m_Animations.m_AnimationSet->m_Length--;
	
	// delete the temp array
	SAFE_DELETE_ARRAY ( pAnimation );
}

void SetKeyFrame ( int iID, int iFrame )
{
	SetFrame ( iID, iFrame );
}

bool GetLimbExist ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	if ( ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return true;
	else
		return false;
}

bool GetLimbLink ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	if ( ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
	{
		return pFrame->m_bFree;
	}
	else
		return false;
}

float GetLimbOffsetX ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return 0.0f;
	
	return pFrame->m_Limb.vecOffset.x;
}

float GetLimbOffsetY ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return 0.0f;
	
	return pFrame->m_Limb.vecOffset.y;
}

float GetLimbOffsetZ ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return 0.0f;
	
	return pFrame->m_Limb.vecOffset.z;
}

float GetLimbAngleX ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return 0.0f;
	
	return pFrame->m_Limb.vecRotate.x;
}

float GetLimbAngleY ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return 0.0f;
	
	return pFrame->m_Limb.vecRotate.y;
}

float GetLimbAngleZ ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return 0.0f;
	
	return pFrame->m_Limb.vecRotate.z;
}

float GetLimbXPosition ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return 0.0f;
	
	return pFrame->m_matCombined._41;
}

float GetLimbYPosition ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return 0.0f;
	
	return pFrame->m_matCombined._42;
}

float GetLimbZPosition ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return 0.0f;
	
	return pFrame->m_matCombined._43;
}

float GetLimbXDirection ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return 0.0f;
	
	return pFrame->m_Limb.vecRotate.x;
}

float GetLimbYDirection ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return 0.0f;
	
	return pFrame->m_Limb.vecRotate.y;
}

float GetLimbZDirection ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return 0.0f;
	
	return pFrame->m_Limb.vecRotate.z;
}

IDirect3DTexture8* GetLimbTexturePtr ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return NULL;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return NULL;

	// get limb texture id
	return pFrame->m_MeshList->m_Mesh->GetTexture(0);
}

int GetLimbTexture ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return NULL;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return NULL;

	// no limb teture id info - need an imagedata scan!
	return 0;
}

bool GetLimbVisible ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return false;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return false;
	
	return pFrame->m_Limb.bVisible;
}

char* GetLimbTextureName ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return NULL;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return NULL;

	return NULL;
}

char* GetLimbName ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return NULL;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return NULL;

	return pFrame->m_Name;
}

int GetLimbCount ( int iID )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return 0;

	return m_pModelData->m_Object.m_NumFrames;
}

float GetLimbScaleX ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return 0.0f;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return 0.0f;

	return pFrame->m_Limb.vecScale.x;
}

float GetLimbScaleY ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return 0.0f;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return 0.0f;

	return pFrame->m_Limb.vecScale.y;
}

float GetLimbScaleZ ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return 0.0f;

	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return 0.0f;

	return pFrame->m_Limb.vecScale.z;
}

void Draw ( int iID, int iBaseTextureStage )
{
	D3DXMATRIX	Matrix;

	m_pModelData = ( tagModelData* ) m_List.Get ( iID );
	if(m_pModelData==NULL) return;
	m_pModelData->m_Object.m_matObject = m_pModelData->m_matObject;

	// animation speed is set here
	if ( m_pModelData->m_bAnimPlaying )
	{
		m_pModelData->m_dwThisTime = timeGetTime ( );

		if ( m_pModelData->m_dwThisTime > m_pModelData->m_dwLastTime + m_pModelData->m_fAnimSpeed )
		{
			if ( m_pModelData->m_iAnimDirection == ANIM_DIR_FORWARD )
			{
				if ( m_pModelData->m_fFrame < m_pModelData->m_iEndFrame )
				{
					m_pModelData->m_fFrame += (m_pModelData->m_iFrameJump/100.0f);
				}
				else
					m_pModelData->m_iAnimDirection = ANIM_DIR_END;
			}
			else if ( m_pModelData->m_iAnimDirection == ANIM_DIR_BACK )
			{
				if ( m_pModelData->m_fFrame > m_pModelData->m_iStartFrame )
				{
					m_pModelData->m_fFrame -= (m_pModelData->m_iFrameJump/100.0f);
				}
				else
					m_pModelData->m_iAnimDirection = ANIM_DIR_END;
			}

			if ( m_pModelData->m_iAnimDirection == ANIM_DIR_END )
			{
				if ( m_pModelData->m_iAnimMode == ANIM_LOOP )
				{
					m_pModelData->m_fFrame         = (float)m_pModelData->m_iStartFrame;
					m_pModelData->m_bAnimPlaying   = true;
					m_pModelData->m_iAnimDirection = ANIM_DIR_FORWARD;
				}
				else if ( m_pModelData->m_iAnimMode == ANIM_SIMPLE )
				{
					m_pModelData->m_fFrame         = (float)m_pModelData->m_iStartFrame;
					m_pModelData->m_bAnimPlaying   = false;
					m_pModelData->m_iAnimDirection = ANIM_DIR_FORWARD;
				}
				else if ( m_pModelData->m_iAnimMode == ANIM_CYCLE )
				{
					if ( m_pModelData->m_fFrame < (float)( m_pModelData->m_iEndFrame + (m_pModelData->m_iFrameJump/100.0f) ) && m_pModelData->m_fFrame > ( m_pModelData->m_iEndFrame - (m_pModelData->m_iFrameJump/100.0f)  ) )
					{
						m_pModelData->m_iAnimDirection = ANIM_DIR_BACK;
					}
					else if ( m_pModelData->m_fFrame == (float)m_pModelData->m_iStartFrame )
					{
						m_pModelData->m_iAnimDirection = ANIM_DIR_FORWARD;
					}
				}
			}

			m_pModelData->m_dwLastTime = timeGetTime ( );
		}
	}
	UpdateAnimation ( &m_pModelData->m_Object, m_pModelData->m_fFrame, TRUE );

	// update the frame matrices
	D3DXMatrixIdentity ( &Matrix );
	UpdateFrame ( &m_pModelData->m_Object, m_pModelData->m_Object.m_Frames, &Matrix );

	if(m_pModelData->m_Object.m_Meshes)
		m_pModelData->m_Object.m_Meshes->CopyFrameToBoneMatrices ( );
	
	// draw all meshes
	D3DXMATRIX matHierarchy;
	D3DXMatrixIdentity ( &matHierarchy );
	DrawFrame ( &m_pModelData->m_Object, m_pModelData->m_Object.m_Frames, iBaseTextureStage, &matHierarchy );
	
	#if DEBUG_MODE
	if ( m_pModelData->m_pMeshBox )
	{
		D3DXMATRIX mat = m_pModelData->m_matBox * *m_pModelData->m_matObject;
		m_pD3D->SetTransform( D3DTS_WORLD, &mat );
		/*
		m_pD3D->SetRenderState( D3DRS_LIGHTING, FALSE );
		m_pD3D->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
		m_pD3D->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCCOLOR );
		m_pD3D->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		m_pD3D->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		m_pModelData->m_pMeshBox->DrawSubset( 0 );
		m_pD3D->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		m_pD3D->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
		m_pD3D->SetRenderState( D3DRS_LIGHTING, TRUE );
		*/

		m_pD3D->SetRenderState ( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
		m_pD3D->SetRenderState( D3DRS_LIGHTING, FALSE );
		m_pD3D->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
		m_pModelData->m_pMeshBox->DrawSubset( 0 );
		m_pD3D->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
		m_pD3D->SetRenderState( D3DRS_LIGHTING, TRUE );
		m_pD3D->SetRenderState ( D3DRS_FILLMODE, D3DFILL_SOLID );
	}
	#endif
}

ID3DXMesh* GetMeshData ( int iID )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return NULL;

	if ( m_pModelData->m_Object.m_Meshes->m_Mesh )
		return m_pModelData->m_Object.m_Meshes->m_Mesh;
	else
		return NULL;
}

LPDIRECT3DTEXTURE8* GetTextureData ( int iID )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return NULL;

	if ( !m_pModelData->m_Object.m_Frames )
		return NULL;

	if ( m_pModelData->m_Object.m_Frames->m_MeshList )
	{
		if ( m_pModelData->m_Object.m_Frames->m_MeshList->m_Mesh )
		{
			if ( m_pModelData->m_Object.m_Frames->m_MeshList->m_Mesh->m_Textures )
			{
				return m_pModelData->m_Object.m_Frames->m_MeshList->m_Mesh->m_Textures;
			}
			else
				return NULL;
		}
		else
			return NULL;
	}
	else
		return NULL;
}

DWORD GetNumMaterials ( int iID )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return NULL;

	if ( !m_pModelData->m_Object.m_Frames )
		return NULL;

	if ( m_pModelData->m_Object.m_Frames->m_MeshList )
	{
		if ( m_pModelData->m_Object.m_Frames->m_MeshList->m_Mesh )
		{
			if ( m_pModelData->m_Object.m_Frames->m_MeshList->m_Mesh->m_Textures )
			{
				return m_pModelData->m_Object.m_Frames->m_MeshList->m_Mesh->m_NumMaterials;
			}
			else
				return NULL;
		}
		else
			return NULL;
	}
	else
		return NULL;
}

D3DMATERIAL8* GetMaterialData ( int iID )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return NULL;

	if ( !m_pModelData->m_Object.m_Frames )
		return NULL;

	if ( m_pModelData->m_Object.m_Frames->m_MeshList )
	{
		if ( m_pModelData->m_Object.m_Frames->m_MeshList->m_Mesh )
		{
			if ( m_pModelData->m_Object.m_Frames->m_MeshList->m_Mesh->m_Textures )
			{
				return m_pModelData->m_Object.m_Frames->m_MeshList->m_Mesh->m_Materials;
			}
			else
				return NULL;
		}
		else
			return NULL;
	}
	else
		return NULL;
}

D3DXMATRIX GetCombined ( int iID, int iFrame )
{
	// pointer to frame data
	sFrame* pFrame = NULL;

	D3DXMATRIX matZero;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return matZero;

	// now see if the limb exists
	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iFrame ) ) )
		return matZero;

	return pFrame->m_matCombined;
}

D3DXMATRIX GetTransformed ( int iID, int iFrame )
{
	// pointer to frame data
	sFrame* pFrame = NULL;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return NULL;

	// now see if the limb exists
	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iFrame ) ) )
		return NULL;

	return pFrame->m_matTransformed;
}

D3DXMATRIX GetOriginal ( int iID, int iFrame )
{
	// pointer to frame data
	sFrame* pFrame = NULL;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return NULL;

	// now see if the limb exists
	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iFrame ) ) )
		return NULL;

	return pFrame->m_matOriginal;
}

void GetFVF ( int iID, DWORD** pdwFVF )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	*pdwFVF = &m_pModelData->m_Object.m_dwFVF;
}

void GetFVFSize ( int iID, DWORD** pdwFVFSize )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	*pdwFVFSize = &m_pModelData->m_Object.m_dwFVFSize;
}

DWORD GetNumberOfMeshes ( int iID )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return 0;

	// return number of meshes in meshlist (for any mesh modifier)
	return m_pModelData->m_Object.m_NumMeshes;
}

void GetMeshDataFromMeshList ( int iID, int iIndex, LPD3DXMESH** pData )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// get a pointer to the start mesh
	sMesh* pMesh = m_pModelData->m_Object.m_Meshes;
	for ( int iTemp = 0; iTemp < m_pModelData->m_Object.m_NumMeshes; iTemp++ )
	{
		// Data from mesh
		if ( iTemp==iIndex )
		{
			*pData = &pMesh->m_Mesh;
			return;
		}

		// next mesh
		pMesh = pMesh->m_Next;
	}

	// not found
	*pData = NULL;
}

void GetMeshAttribFromMeshList ( int iID, int iIndex, D3DXATTRIBUTERANGE*** pData )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// get a pointer to the start mesh
	sMesh* pMesh = m_pModelData->m_Object.m_Meshes;
	for ( int iTemp = 0; iTemp < m_pModelData->m_Object.m_NumMeshes; iTemp++ )
	{
		// Data from mesh
		if ( iTemp==iIndex )
		{
			*pData = &pMesh->m_pAttributeTable;
			return;
		}

		// next mesh
		pMesh = pMesh->m_Next;
	}

	// not found
	*pData = NULL;
}

