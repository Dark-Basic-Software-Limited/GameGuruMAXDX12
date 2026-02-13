//
// DBOFormat Functions Implementation
//

#include "DBOFormat.h"
#include "DBOFrame.h"
#include "DBORawMesh.h"
#include "DBOMesh.h"
#include "DBOFile.h"
#include "..\..\..\Include\ConvX.h"
#include "..\..\..\Include\CGfxC.h"
#include "..\Objects\CommonC.h"
#include "DBOAssImp.h"
#include "CFileC.h"

// 291116 - Defined in DBDLLCORE to improve timer precision
DARKSDK float timeGetSecond ( void );

// Prototype for controlled execution of apps
BOOL DB_ExecuteFile(HANDLE* phExecuteFileProcess, char* Operation, char* Filename, char* String, char* Path, bool bWaitForTermination);
int GetExecutableRunning(DWORD pHandle);

#pragma warning(disable : 4786)
#include "Extras\NVMeshMender.h"
#include "mmsystem.h"
#include "direct.h"
#include "string.h"
#include "shellapi.h"
#ifdef WICKEDENGINE
//#include ".\..\Objects\CObjectManagerWicked.h"
#include ".\..\..\..\..\Guru-WickedMAX\wickedcalls.h"
#else
#endif
#include ".\..\Objects\CObjectManagerC.h"
#include "..\..\..\Include\CImageC.h"
#include <thread>

// External error helper
extern char g_strErrorClue[512];

extern		LPGGDEVICE							m_pD3D;				// d3d device
extern		LPGGIMMEDIATECONTEXT				m_pImmediateContext;
extern		sEffectConstantData					g_EffectConstant;
extern		sObject**							g_ObjectList;
extern      float								g_fShrinkObjectsTo;
extern		CObjectManager						m_ObjectManager;

// Global
bool		g_bGracefulWarningAboutOldXFiles	= false;
bool		g_bSwitchLegacyOn					= false;
float		g_fLastDeltaTime					= 0.0f;
bool		g_bFastBoundsCalculation			= true;
GGMATRIX	g_matThisViewProj;
GGMATRIX	g_matThisCameraView;
GGMATRIX	g_matPreviousViewProj;

GGPLANE								g_Planes [ 20 ][ NUM_CULLPLANES ];			// list of planes for frustum culling
DBPRO_GLOBAL GGVECTOR3				g_PlaneVector [ 20 ] [ NUM_CULLPLANES ];
DBPRO_GLOBAL sEffectConstantData	g_EffectConstant;							// used to store shader constants data
DWORD								g_dwEffectErrorMsgSize=0;
LPSTR								g_pEffectErrorMsg=NULL;
DWORD*								g_pConversionMap=NULL;						// single conversion array ptr held
char								g_WindowsTempDirectory[_MAX_PATH];
		
//Dave Performance
bool								g_bEarlyExcludeMode = false;

// globals for single thread (X File loading is NOT thread safe, can run in parallel if we know X files are not being loaded elsewhere)
std::thread* g_pT2 = NULL;
bool g_bT2 = false;
bool g_bRequestCleanInteruptionT2 = false;
std::vector<sPreLoadedObjectData> g_object_list;
std::vector<sPreLoadedObjectData> g_object_outputv;

// function to execute thread code
void object_thread_function(const std::vector<sPreLoadedObjectData> &v)
{
	g_bT2 = true;
	// in this thread, load each object data in turn
	g_object_outputv.clear();
	for ( int n = 0; n < v.size(); n++ )
	{
		sPreLoadedObjectData item = v[n];
		if ( item.pData == NULL ) if ( LoadDBODataBlock ( item.pFilename, &item.dwDataSize, (void**)&item.pData ) == false ) strcpy ( item.pFilename, "" );
		g_object_outputv.push_back(item);
		if (g_bRequestCleanInteruptionT2 == true)
		{
			// this flag can be set when we want to interupt this loading thread, and keep
			// what we have up to this point, allowing main thread to continue quickly (stops a possible 12 second pause in some cases!)
			break;
		}
	}
	g_bT2 = false;
}

void object_preload_files_start ( void )
{
	// clear list ready for new files to thread load
	g_object_list.clear();
}

void object_preload_files_add ( LPSTR pFilename )
{
	//Moved here so we can check if its already in the list.
	char *cUseFilename;
	char cResolvePath[MAX_PATH];
	if (GetFullPathNameA(pFilename, MAX_PATH, &cResolvePath[0], NULL) > 0) 
	{
		cUseFilename = &cResolvePath[0];
	}
	else 
	{
		cUseFilename = pFilename;
	}

	// check to make sure we don't add something we already have in the list
	for ( int n = 0; n < g_object_list.size(); n++ )
		if ( stricmp ( g_object_list[n].pFilename, cUseFilename) == NULL )
			return;

	// add item to list of work
	sPreLoadedObjectData item;
	strcpy ( item.pFilename, cUseFilename);
	item.dwDataSize = 0;
	item.pData = NULL;
	g_object_list.push_back(item);
}

void object_preload_files_finish ( void )
{
	// before send list to thread, load up list with previous preloaded file 'data' still in memory
	for ( int n = 0; n < g_object_list.size(); n++ )
	{
		if ( n < g_object_outputv.size() )
		{
			if ( stricmp ( g_object_list[n].pFilename, g_object_outputv[n].pFilename ) == NULL )
			{
				// this ensures we avoid reloading something we 'might need' that we have already preloaded previously
				g_object_list[n].pData = g_object_outputv[n].pData;
				g_object_list[n].dwDataSize = g_object_outputv[n].dwDataSize;
			}
		}
	}

	// start preloading
	g_pT2 = new std::thread(object_thread_function, std::ref(g_object_list));
}

void object_preload_files_strictwaittoend ( void )
{
	// wait for all work to finish
	if ( g_pT2 )
	{
		g_pT2->join();
		delete g_pT2;
		g_pT2 = NULL;
		g_bRequestCleanInteruptionT2 = false;
	}
}


void object_preload_files_wait(void)
{
	g_bRequestCleanInteruptionT2 = true;
	object_preload_files_strictwaittoend();
	
}

void object_preload_files_reset ( void )
{
	// clear finished list for next batch of work
	for ( int n = 0; n < g_object_outputv.size(); n++ )
	{
		if ( g_object_outputv[n].pData ) 
		{
			delete g_object_outputv[n].pData;
			g_object_outputv[n].pData = NULL;
			//g_object_outputv[n].dwDataSize = 0;
		}
	}
	g_object_outputv.clear();
}

bool object_preload_files_in_progress(void)
{
	return g_bT2;
}

///

cSpecialEffect::cSpecialEffect ( )
{
	// reset effect ptr
	memset ( this, 0, sizeof(cSpecialEffect) );
}

cSpecialEffect::~cSpecialEffect ( )
{
	if ( m_dwRTTexCount>0 )
	{
		for ( DWORD t=0; t<m_dwRTTexCount; t++ )
		{
			SAFE_RELEASE ( m_pRTTex[t] );
			SAFE_RELEASE ( m_pRTTexView[t] );
		}
	}

	// free default zfile mesh
	SAFE_DELETE ( m_pXFileMesh );

	// free effect
	SAFE_RELEASE ( m_pEffect );
	SAFE_DELETE ( m_pDefaultXFile );
}

bool cSpecialEffect::CorrectFXFile ( LPSTR pFile, LPSTR pModifiedFile )
{
	// result var
	bool bResult=false;

	// read in original file
	HANDLE hreadfile = GG_CreateFile(pFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hreadfile!=INVALID_HANDLE_VALUE)
	{
		// read file into memory
		DWORD bytesread;
		int filebuffersize = GetFileSize(hreadfile, NULL);	
		char* filebuffer = (char*)GlobalAlloc(GMEM_FIXED, filebuffersize);
		ReadFile(hreadfile, filebuffer, filebuffersize, &bytesread, NULL); 
		CloseHandle(hreadfile);		

		// Items an FX file may be missing
		bool bDCLTokensExist=false;
		bool bInvalidTargetTokenUsed=false;

		// scan and modify for corrections
		LPSTR pPtr = filebuffer;
		LPSTR pPtrEnd = filebuffer+filebuffersize;
		while ( pPtr<pPtrEnd )
		{
			// check for existance of commonly missing data
			if ( _strnicmp ( pPtr, "dcl_", 4 )==NULL ) bDCLTokensExist=true;
			if ( _strnicmp ( pPtr, "target[", 7 )==NULL ) bInvalidTargetTokenUsed=true;

			// next byte
			pPtr++;
		}

		// write corrections in new data
		int newbuffersize = filebuffersize*2;	
		char* newbuffer = (char*)GlobalAlloc(GMEM_FIXED, newbuffersize);
		LPSTR pWritePtr = newbuffer;
		LPSTR pWritePtrEnd = newbuffer+newbuffersize;

		// go through data again
		pPtr = filebuffer;
		while ( pPtr<pPtrEnd )
		{
			// change any VS to include standard declarations
			if ( bDCLTokensExist==false )
			{
				// from vs.1.1 to vs.1.1 dcl_position v0 dcl_normal v3 etc
				if ( _strnicmp ( pPtr, "vs.", 3 )==NULL )
				{
					// vs.x.x
					memcpy ( pWritePtr, pPtr, 6 ); //vs.x.x
					pWritePtr+=6;
					pPtr+=6;

					// add dcls
					memcpy ( pWritePtr, (LPSTR)" dcl_position v0", 16 ); pWritePtr+=16;
					memcpy ( pWritePtr, (LPSTR)" dcl_normal v3", 14 ); pWritePtr+=14;
					memcpy ( pWritePtr, (LPSTR)" dcl_color v6", 13 ); pWritePtr+=13;
					memcpy ( pWritePtr, (LPSTR)" dcl_texcoord v7", 16 ); pWritePtr+=16;
				}
			}

			// change any target tokens by commenting them out
			if ( bInvalidTargetTokenUsed==true )
			{
				if ( _strnicmp ( pPtr, "target[", 7 )==NULL )
				{
					*pWritePtr = '/'; pWritePtr++;
					*pWritePtr = '/'; pWritePtr++;
					pPtr+=2;
				}
			}

			// write from original data to new buffer
			*pWritePtr = *pPtr;

			// next bytes
			pWritePtr++;
			pPtr++;
		}

		// write new temp file
		DWORD actualnewdatasize = pWritePtr-newbuffer;
		HANDLE hwritefile = GG_CreateFile(pModifiedFile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hwritefile!=INVALID_HANDLE_VALUE)
		{
			// write new data
			DWORD byteswritten;
			WriteFile(hwritefile, newbuffer, actualnewdatasize, &byteswritten, NULL); 
			CloseHandle(hwritefile);

			// success
			bResult=true;
		}

		// free usages
		if ( filebuffer ) { GlobalFree ( filebuffer ); }
		if ( newbuffer ) { GlobalFree ( newbuffer ); }
	}

	// failed
	return bResult;
}

bool cSpecialEffect::Load ( int iEffectID, LPSTR pEffectFile, bool bUseXFile, bool bUseTextures )
{
	// record effect name
	strcpy ( m_pEffectName, pEffectFile );
	m_iEffectID = iEffectID;

	// split off path and switch for local resource loading
	char pPath[_MAX_PATH];
	char pFile[_MAX_PATH];
	strcpy ( pPath, "" );
	strcpy ( pFile, pEffectFile );
	for ( int n=strlen(pEffectFile); n>0; n--)
	{
		if ( pEffectFile[n]=='\\' ||  pEffectFile[n]=='/' )
		{
			// get path and file
			strcpy ( pFile, pEffectFile+n+1 );
			strcpy ( pPath, pEffectFile );
			pPath[n]=0;
			break;
		}
	}

	#ifdef DX11

	// need blob filename
	char pBlobFilename[1024];
	strcpy ( pBlobFilename, pEffectFile );
	pBlobFilename[strlen(pBlobFilename)-3] = 0;
	strcat ( pBlobFilename, ".blob" );

	// load effect from local file first
	m_pEffect = SETUPLoadShader ( pEffectFile, pBlobFilename, iEffectID );
	if ( m_pEffect == NULL ) 
		return false;

	// Associate data in effect with app data
	ParseEffect ( bUseXFile, bUseTextures );
	m_bUseShaderTextures = bUseTextures;

	// find valid technique (first one)
	m_hCurrentTechnique = m_pEffect->GetTechniqueByIndex(0);

	// complete
	return true;
	#else
	// buffer to hold error
	ID3DXBuffer* pErrorBuffer = NULL;

	// store old directory and set local one
	char pOldDir[_MAX_PATH];
	_getcwd(pOldDir, _MAX_PATH);
	if ( strlen(pPath)>0 ) _chdir(pPath);

	// Shader Legacy Mode for later DXSDK
	DWORD dwShaderLegacyMode = 0;
	#ifdef DARKSDK_COMPILE
        #ifdef D3DXSHADER_USE_LEGACY_D3DX9_31_DLL
		    dwShaderLegacyMode = D3DXSHADER_USE_LEGACY_D3DX9_31_DLL;
        #else
			// lee - 131010 - DarkGDK should support The October 2006 SDK (for legacy users)
			// previous DarkGDK builds may have used Aug 2007 (35.DLL)
            // #error You should be using DX SDK Aug 2007 or later
        #endif
	#endif

	// 131213 - improve shader loading for Reloaded
	dwShaderLegacyMode = D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY;
	//dwShaderLegacyMode = D3DXSHADER_SKIPVALIDATION; // 200417 - fixed shaders but no speed increase, ah well

	// 160214 - so I can debug in PIX
	//#if defined( DEBUG ) || defined( _DEBUG )
	//	dwShaderLegacyMode |= D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION;
	//#endif

	// load effect from local file first
	if(FAILED(hr = D3DXCreateEffectFromFileA(m_pD3D, pFile,
					NULL, NULL, dwShaderLegacyMode, NULL, &m_pEffect, &pErrorBuffer )))
	{
		// calculate temp folder location
		DBOCalculateLoaderTempFolder();

		// leefix - 310305 - alter path if in debugmode(plguins last), swiotch to effect
		char pFinalFile[_MAX_PATH];
		strcpy ( pFinalFile, g_WindowsTempDirectory );
		if ( _strnicmp ( "plugins\\", (g_WindowsTempDirectory + strlen(g_WindowsTempDirectory)) - 8, 8 )==NULL ) 
		{
			// switch from plugins to effects (for finding an internal FX file)
			strcpy ( pFinalFile, g_WindowsTempDirectory );
			pFinalFile [ strlen(pFinalFile) - 8 ] = 0;
			strcat ( pFinalFile, "effects\\" );
		}

		// try from dbpdata folder
		strcat ( pFinalFile, pFile );

		// load effect from internal file second
		if(FAILED(hr = D3DXCreateEffectFromFileA(m_pD3D, pFinalFile,
						NULL, NULL, dwShaderLegacyMode, NULL, &m_pEffect, &pErrorBuffer )))
		{
			// third possibility is that it is in the TEMP media folder
			strcpy ( pFinalFile, g_WindowsTempDirectory );
			strcat ( pFinalFile, "media\\" );
			strcat ( pFinalFile, pEffectFile );
			if(FAILED(hr = D3DXCreateEffectFromFileA(m_pD3D, pFinalFile,
							NULL, NULL, dwShaderLegacyMode, NULL, &m_pEffect, &pErrorBuffer )))
			{
				// try from dbpdata folder
				char pModifiedFile[_MAX_PATH];
				strcpy ( pModifiedFile, g_WindowsTempDirectory );
				strcat ( pModifiedFile, "_modified_fx.fx" );

				// forth possibility is that the local FX file cannot be parsed (nvidia)
				if ( CorrectFXFile ( pFile, pModifiedFile ) )
				{
					// load effect from local file first
					if(FAILED(hr = D3DXCreateEffectFromFileA(m_pD3D, pModifiedFile,
									NULL, NULL, dwShaderLegacyMode, NULL, &m_pEffect, &pErrorBuffer )))
					{
						// simply cannot payse the FX file
						DeleteFile ( pModifiedFile );
						goto failed;
					}

					// Remove temp file
					DeleteFile ( pModifiedFile );
				}
				else
				{
					// cannot correct FX file
					goto failed;
				}
			}
			else
			{
				// The TEMP\MEDIA was the right folder, need to switch CWD to it
				strcpy ( pFinalFile, g_WindowsTempDirectory );
				strcat ( pFinalFile, "media\\" );
				strcat ( pFinalFile, pPath );
				_chdir(pFinalFile);
			}
		}
	}

	// Associate data in effect with app data
	ParseEffect ( bUseXFile, bUseTextures );
	m_bUseShaderTextures = bUseTextures;

	// find valid technique
	GGHANDLE hTechnique;
	if(FAILED(hr = m_pEffect->FindNextValidTechnique(NULL, &hTechnique)))
	{
		SAFE_RELEASE(pErrorBuffer);
		_chdir(pOldDir);
		return false;
	}

	// set the first valid technique (DBPro FX files orders best to worst)
	if ( hTechnique )
		m_pEffect->SetTechnique(hTechnique);

	// 091115 - find a pass named 'RenderDepthPixelsPass' and flag if found
	// as we can skip this pass if engine does not use depth related stuff like DOF and MOTION BLUR (performance)
	if ( hTechnique ) m_DepthRenderPassHandle = m_pEffect->GetPassByName ( hTechnique, "RenderDepthPixelsPass");

	// restores
	SAFE_RELEASE(pErrorBuffer);
	_chdir(pOldDir);

	// complete
	return true;

	failed:

	// handle error buffer
	if ( pErrorBuffer ) 
	{
		g_dwEffectErrorMsgSize = pErrorBuffer->GetBufferSize();
		SAFE_DELETE(g_pEffectErrorMsg);
		g_pEffectErrorMsg = new char[g_dwEffectErrorMsgSize+1];
		memcpy ( g_pEffectErrorMsg, pErrorBuffer->GetBufferPointer(), g_dwEffectErrorMsgSize );
		MessageBox ( NULL, g_pEffectErrorMsg, g_pEffectErrorMsg, MB_OK );
	}

	// restores
	SAFE_RELEASE(pErrorBuffer);
	_chdir(pOldDir);
	#endif

	// failure
	return false;
}

bool cSpecialEffect::Setup ( sMesh* pMesh )
{
	// alter mesh with default xfile mesh
	if ( m_pXFileMesh ) MakeLocalMeshFromOtherLocalMesh ( pMesh, m_pXFileMesh );

	// complete
	return true;
}

void cSpecialEffect::Mesh ( sMesh* pMesh )
{
	// change mesh to suit effect
	// leeadd - 200204 - add bone data to mesh FVF if shader requests it
	// leeadd - 121208 - U71 - was (==0) now go into generate if 0 to 2 (first bit zero)
	if ( m_bDoNotGenerateExtraData==0 || m_bDoNotGenerateExtraData==2 )
	{
		// leeadd - 050906 - auto-generate if not flagged off to keep object pure
		if ( pMesh->dwVertexCount > 3 )
		{
			GenerateExtraDataForMeshEx ( pMesh, m_bGenerateNormals, m_bUsesTangents, m_bUsesBinormals, m_bUsesDiffuse, m_bUsesBoneData, m_bDoNotGenerateExtraData );
		}
	}

	// lee - 230306 - u6b4 - also generate a new 'original vertexdata store', otherwise CPU bone animate will be out of phase and crash
	if ( pMesh )
	{
		SAFE_DELETE ( pMesh->pOriginalVertexData );
		CollectOriginalVertexData ( pMesh );
	}
}

DWORD cSpecialEffect::Start	( sMesh* pMesh, GGMATRIX matObject )
{
	// if a valid FX effect
	#ifdef DX11
	if ( m_pEffect )
	{
		// now replace effect textures (usually none) with mesh textures
		#ifdef DX11
		#else
		int iParamTexArrayLimit = m_dwTextureCount;
		if ( iParamTexArrayLimit>32 ) iParamTexArrayLimit=32;
		for ( DWORD t=0; t<(DWORD)iParamTexArrayLimit; t++ )
		{
			int iParam = m_iParamOfTexture[t];
			if ( t<pMesh->dwTextureCount )
			{
				GGHANDLE pParam = m_pEffect->GetParameter( NULL, iParam );
				if ( pParam )
				{
					//D3DXPARAMETER_DESC PDesc; cube map just wont show
					//m_pEffect->GetParameterDesc(pParam, &PDesc );
					LPGGTEXTURE pTexToUse = pMesh->pTextures [ t ].pTexturesRef;
					m_pEffect->SetTexture( pParam, pTexToUse );
				}
			}
		}
		#endif

		// var
		UINT uPasses = 0;

		// begin effect and return number of required passes
		ID3DX11EffectTechnique* pTech = m_hCurrentTechnique;//m_pEffect->GetTechniqueByIndex(0);
		D3DX11_TECHNIQUE_DESC desc;
		pTech->GetDesc(&desc);
		uPasses = desc.Passes;

		// Apply associations to effect using latest application data
		ApplyEffect ( pMesh );

		// passes count
		return (DWORD)uPasses;
	}
	#else
	if ( m_pEffect )
	{
		// now replace effect textures (usually none) with mesh textures
		// LEELEE : Performance warning - is this slow or fast - good flexibility though!
		// LEELEE : This seems to cause massive slowdown when texturing NODETREE meshes!
		// Probably because it is swapping the same texture in the effect many times.
		// Solution is we should be sorting by EFFECT, then by TEXTURE..TODO!
		int iParamTexArrayLimit = m_dwTextureCount;
		if ( iParamTexArrayLimit>32 ) iParamTexArrayLimit=32;
		for ( DWORD t=0; t<(DWORD)iParamTexArrayLimit; t++ )
		{
			int iParam = m_iParamOfTexture[t];
			if ( t<pMesh->dwTextureCount )
			{
				GGHANDLE pParam = m_pEffect->GetParameter( NULL, iParam );
				if ( pParam )
				{
					//D3DXPARAMETER_DESC PDesc; cube map just wont show
					//m_pEffect->GetParameterDesc(pParam, &PDesc );
					LPGGTEXTURE pTexToUse = pMesh->pTextures [ t ].pTexturesRef;
					m_pEffect->SetTexture( pParam, pTexToUse );
				}
			}
		}

		// var
		UINT uPasses;

		// begin effect and return number of required passes
		m_pEffect->Begin(&uPasses, 0 );

		// Apply associations to effect using latest application data
		ApplyEffect ( pMesh );

		// passes count
		return (DWORD)uPasses;
	}
	#endif
	return 0;
}

void cSpecialEffect::End ( void )
{
	// if a valid FX effect
	#ifdef DX11
	#else
	if ( m_pEffect )
	{
		m_pEffect->End();
	}
	#endif
}

bool cSpecialEffect::AssignValueHookCore ( LPSTR pName, GGHANDLE hParam, DWORD dwClass, bool bRemove )
{
	#define ASSIGNNAME(a,b)	if ( bRemove ) { if ( b==hParam ) { b=NULL; return true; } } else { if ( _stricmp ( pName, a )==0 ) { if ( hParam ) { b=hParam; return true; } else { if ( b ) return true;  }; } };

	// auto-fail if no name
	if ( bRemove==false && pName==NULL ) return false;

	// COMMON UNTWEAKABLES
	ASSIGNNAME ( "world", m_MatWorldEffectHandle );
	ASSIGNNAME ( "view", m_MatViewEffectHandle );
	ASSIGNNAME ( "projection", m_MatProjEffectHandle );
	ASSIGNNAME ( "worldview", m_MatWorldViewEffectHandle );
	ASSIGNNAME ( "viewprojection", m_MatViewProjEffectHandle );
	ASSIGNNAME ( "worldviewprojection", m_MatWorldViewProjEffectHandle );

	// MS UNTWEAKABLES
	ASSIGNNAME ( "worldviewit", m_MatWorldViewInverseEffectHandle );
	ASSIGNNAME ( "worldit", m_MatWorldInverseEffectHandle );
	ASSIGNNAME ( "viewit", m_MatViewInverseEffectHandle );

	// NVIDIA UNTWEAKABLES
	ASSIGNNAME ( "WorldInverse", m_MatWorldInverseEffectHandle );
	ASSIGNNAME ( "WorldTranspose", m_MatWorldTEffectHandle );
	ASSIGNNAME ( "WorldInverseTranspose", m_MatWorldInverseTEffectHandle );
	ASSIGNNAME ( "ViewInverse", m_MatViewInverseEffectHandle );
	ASSIGNNAME ( "ViewTranspose", m_MatViewTEffectHandle );
	ASSIGNNAME ( "ViewInverseTranspose", m_MatViewInverseTEffectHandle );
	ASSIGNNAME ( "ProjectionInverse", m_MatProjectionInverseEffectHandle );
	ASSIGNNAME ( "ProjectionTranspose", m_MatProjTEffectHandle );
	ASSIGNNAME ( "WorldViewTranspose", m_MatWorldViewTEffectHandle );
	ASSIGNNAME ( "ViewProjectionTranspose", m_MatViewProjTEffectHandle );
	ASSIGNNAME ( "WorldViewProjectionTranspose", m_MatWorldViewProjTEffectHandle );
	ASSIGNNAME ( "WorldViewInverse", m_MatWorldViewInverseEffectHandle );

	// vectors
	ASSIGNNAME ( "cameraposition", m_VecCameraPosEffectHandle );
	ASSIGNNAME ( "eyeposition", m_VecEyePosEffectHandle );
	ASSIGNNAME ( "clipplane", m_VecClipPlaneEffectHandle );

	// MS lighting
	ASSIGNNAME ( "UIDirectional", m_LightDirHandle );
	ASSIGNNAME ( "UIDirectionalInv", m_LightDirInvHandle );
	ASSIGNNAME ( "UIPosition", m_LightPosHandle );

	// NVIDIA lighting
	ASSIGNNAME ( "directionalight", m_LightDirHandle );
	ASSIGNNAME ( "pointlight", m_LightPosHandle );
	ASSIGNNAME ( "spotlight", m_LightPosHandle );

	// rogue scalars
	ASSIGNNAME ( "time", m_TimeEffectHandle );
	ASSIGNNAME ( "sintime", m_SinTimeEffectHandle );
	ASSIGNNAME ( "deltatime", m_DeltaTimeEffectHandle );
	ASSIGNNAME ( "uvscaling", m_UVScalingHandle );
	
	//
	// DBPRO UNTWEAKABLES
	//

	ASSIGNNAME ( "alphaoverride", m_AlphaOverrideHandle );
	///ASSIGNNAME ( "bonecount", m_BoneCountHandle );
	ASSIGNNAME ( "bonematrixpalette", m_BoneMatrixPaletteHandle );

	// non-handle hook values
	if ( pName )
		if ( _strcmpi ( pName, "xfile" )==0 )
			return true;

	// could not find name match
	return false;
}

bool cSpecialEffect::AssignValueHook ( LPSTR pName, GGHANDLE hParam )
{
	return AssignValueHookCore ( pName, hParam, 0, false );
}

bool cSpecialEffect::ParseEffect ( bool bUseEffectXFile, bool bUseEffectTextures )
{
	// if no effect, skip
	if( m_pEffect == NULL )
		return false;

	// Used to assign from mesh textures
	m_dwTextureCount = 0;

	// get effect description
	m_pEffect->GetDesc( &m_EffectDesc );

	// u64 - 180107 - new mask to hold dynamic tecture flags
	m_dwUseDynamicTextureMask = 0; // default is effect uses NO dynamic textures
	DWORD dwCountTexturesInEffect = 0;

	// U75 - 200310 - clear RT mask as well
	m_dwCreatedRTTextureMask = 0; 
	m_bUsesAtLeastOneRT = false;

	// Look at parameters for semantics and annotations that we know how to interpret
	#ifdef DX11
	GGHANDLE hParam;
	D3DX11_EFFECT_VARIABLE_DESC ParamDesc;
	UINT iParametersCount = m_EffectDesc.GlobalVariables;
	for( UINT iParam = 0; iParam < iParametersCount; iParam++ )
	{
		// temp vars
		LPCSTR pstrName = NULL;
		LPCSTR pstrFunction = NULL;
		LPCSTR pstrTarget = NULL;
		LPCSTR pstrTextureType = NULL;
		INT Width = 0;
		INT Height= 0;
		INT Depth = 0;

		// get this parameter handle and description
		hParam = m_pEffect->GetVariableByIndex(iParam);
		hParam->GetDesc ( &ParamDesc );

		// annotations are associated with global variables
	    GGHANDLE hAnnot = NULL;
		/*
		// light handles from DX9 FX files
	    hAnnot = m_pEffect->GetAnnotationByName( hParam, "UIDirectional" );
        if( hAnnot != NULL ) AssignValueHook ( "UIDirectional", hParam );
	    hAnnot = m_pEffect->GetAnnotationByName( hParam, "UIDirectionalInv" );
        if( hAnnot != NULL ) AssignValueHook ( "UIDirectionalInv", hParam );
	    hAnnot = m_pEffect->GetAnnotationByName( hParam, "UIPosition" );
        if( hAnnot != NULL ) AssignValueHook ( "UIPosition", hParam );

		// light handles from NVIDIA FX files
	    hAnnot = m_pEffect->GetAnnotationByName( hParam, "Object" );
	    if( hAnnot == NULL ) hAnnot = m_pEffect->GetAnnotationByName( hParam, "UIObject" );
        if( hAnnot != NULL )
		{
			// light type
			LPCSTR pstrLightType = NULL;
			if( hAnnot != NULL ) m_pEffect->GetString( hAnnot, &pstrLightType );

			// light space
			LPCSTR pstrLightSpace = NULL;
		    hAnnot = m_pEffect->GetAnnotationByName( hParam, "Space" );
			if( hAnnot != NULL ) m_pEffect->GetString( hAnnot, &pstrLightSpace );

			// assign light position hanle
			AssignValueHook ( (char*)pstrLightType, hParam );
		}
		*/

		// get type of variable
		ID3DX11EffectType* pEffectType = hParam->GetType();
		D3DX11_EFFECT_TYPE_DESC typedesc;
		pEffectType->GetDesc(&typedesc);

		// basic matrix semantics
		if( ParamDesc.Semantic != NULL && ( typedesc.Type == D3D10_SVC_MATRIX_COLUMNS || typedesc.Type == D3D10_SVC_MATRIX_ROWS ) )
			AssignValueHookCore ( (char*)ParamDesc.Semantic, hParam, 0, false );

		// basic vector semantics
		if( ParamDesc.Semantic != NULL && ( typedesc.Type == D3D10_SVC_VECTOR ))
			AssignValueHook ( (char*)ParamDesc.Semantic, hParam );

		// basic value semantics
		if( ParamDesc.Semantic != NULL && ( typedesc.Type == D3D10_SVC_SCALAR ))
			AssignValueHook ( (char*)ParamDesc.Semantic, hParam );

		// go through any annotations associated with variable
		D3DX11_EFFECT_VARIABLE_DESC AnnotDesc;
		for( UINT iAnnot = 0; iAnnot < ParamDesc.Annotations; iAnnot++ )
		{
			// get annotation description
			hAnnot = hParam->GetAnnotationByIndex(iAnnot);
			hAnnot->GetDesc(&AnnotDesc);

			// texture name
			if ( _strcmpi( AnnotDesc.Name, "resourcename" ) == 0 )	hAnnot->AsString()->GetString( &pstrName );
			if ( _strcmpi( AnnotDesc.Name, "name" ) == 0 )			hAnnot->AsString()->GetString( &pstrName );
			if ( _strcmpi( AnnotDesc.Name, "file" ) == 0 )			hAnnot->AsString()->GetString( &pstrName );

			// texture type
			if ( _strcmpi( AnnotDesc.Name, "resourcetype" ) == 0 )	hAnnot->AsString()->GetString( &pstrTextureType );
			if ( _strcmpi( AnnotDesc.Name, "type" ) == 0 )			hAnnot->AsString()->GetString( &pstrTextureType );
			if ( _strcmpi( AnnotDesc.Name, "texturetype" ) == 0 )	hAnnot->AsString()->GetString( &pstrTextureType );

			// texture details
			if ( _strcmpi( AnnotDesc.Name, "function" ) == 0 )		hAnnot->AsString()->GetString( &pstrFunction );
			if ( _strcmpi( AnnotDesc.Name, "target" ) == 0 )		hAnnot->AsString()->GetString( &pstrTarget );
			if ( _strcmpi( AnnotDesc.Name, "width" ) == 0 )			hAnnot->AsScalar()->GetInt ( &Width );
			if ( _strcmpi( AnnotDesc.Name, "height" ) == 0 )		hAnnot->AsScalar()->GetInt ( &Height );
			if ( _strcmpi( AnnotDesc.Name, "depth" ) == 0 )			hAnnot->AsScalar()->GetInt ( &Depth );
		}
		if( pstrName != NULL )
		{
			// detect RENDERCOLORTARGET semantic here
			bool bIsThisAnRT = false;
			if ( ParamDesc.Semantic != NULL ) if ( _strcmpi ( ParamDesc.Semantic, "RENDERCOLORTARGET" ) == 0 ) bIsThisAnRT = true;
			if ( bIsThisAnRT==true )
			{
				// this indicates an RT texture we want our shader to render to during the passes, so we need to create a render target for it
				IGGTexture* pRTTex = NULL;
				LPGGSHADERRESOURCEVIEW pRTTexView = NULL;
				GGSURFACE_DESC desc;
				g_pGlob->pCurrentBitmapSurface->GetDesc(&desc);
				if ( Width == 0 ) Width = desc.Width;
				if ( Height == 0 ) Height = desc.Height;
				GGSURFACE_DESC StagedDesc = { Width, Height, 1, 1, GGFMT_A8R8G8B8, 1, 0, D3D11_USAGE_DEFAULT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, 0, 0 };
				m_pD3D->CreateTexture2D( &StagedDesc, NULL, (ID3D11Texture2D**)&pRTTex );

				// shader resource view
				D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
				ZeroMemory(&shaderResourceViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
				shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				shaderResourceViewDesc.Format = GGFMT_A8R8G8B8;
				shaderResourceViewDesc.Texture2D.MipLevels = 1;
				m_pD3D->CreateShaderResourceView ( pRTTex, &shaderResourceViewDesc, &pRTTexView );
				hParam->AsShaderResource()->SetResource ( pRTTexView );

				// render target view
				ID3D11RenderTargetView* pRTTexRenderView = NULL;
				m_pD3D->CreateRenderTargetView( pRTTex, NULL, &pRTTexRenderView );

				// set flag to indicate this specialeffect object uses at least one RT (render target)
				m_bUsesAtLeastOneRT = true;

				// mark in a bitfield which textures are RT (so we can release them when this shader is deleted)
				DWORD dwCorrectBitForThisTexture = 1 << m_dwTextureCount;
				m_dwCreatedRTTextureMask = m_dwCreatedRTTextureMask | dwCorrectBitForThisTexture;

				// record this texture and step through texture indexes
				if ( m_dwTextureCount<=31 ) 
				{
					m_iParamOfTexture [ m_dwTextureCount ] = iParam;
					m_pParamOfTextureRenderView [ m_dwTextureCount ] = pRTTexRenderView;
				}
				m_dwTextureCount++;

				// record this now for later release
				if ( m_dwRTTexCount<=31 ) 
				{
					m_pRTTex [ m_dwRTTexCount ] = pRTTex;
					m_pRTTexView [ m_dwRTTexCount ] = pRTTexView;
				}
				m_dwRTTexCount++;
			}
			else
			{
				/*
				// texture from effect or mesh
				if ( bUseEffectTextures )
				{
					// texture holder
					LPGGBASETEXTURE pTex = NULL;

					// 2D texture is stadnard texture
					if (pstrTextureType != NULL) 
						if( _strcmpi( pstrTextureType, "2d" ) == 0 )
							pstrTextureType=NULL;

					// assign effect texture from FX file
					if (pstrTextureType != NULL) 
					{
						if( _strcmpi( pstrTextureType, "volume" ) == 0 )
						{
							// support for internal volume textures
							LPDIRECT3DVOLUMETEXTURE9 pVolumeTex = NULL;
							if( SUCCEEDED( D3DXCreateVolumeTextureFromFileEx( m_pD3D, pstrName, 
								Width, Height, Depth, 1, 0, GGFMT_UNKNOWN, D3DPOOL_MANAGED,
								D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &pVolumeTex ) ) )
							{
								// iTextureStage
								pTex = pVolumeTex;
							}
						}
						else if( _strcmpi( pstrTextureType, "cube" ) == 0 )
						{
							// support for internal cube textures
							LPGGCUBETEXTURE pCubeTex = NULL;
							if( SUCCEEDED( D3DXCreateCubeTextureFromFileEx( m_pD3D, pstrName, 
								Width, D3DX_DEFAULT, 0, GGFMT_UNKNOWN, D3DPOOL_MANAGED,
								D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &pCubeTex ) ) )
							{
								// iTextureStage
								pTex = pCubeTex;
							}
						}

						// record this now for later release
						if ( m_dwRTTexCount<=31 ) m_pRTTex [ m_dwRTTexCount ] = pTex;
						m_dwRTTexCount++;
					}
					else
					{
						// support for internal basic textures
						int iImageIndex = LoadOrFindTextureAsImage ( (char*)pstrName, "" );
						pTex = GetImagePointer ( iImageIndex );
					}

					// assign texture to effect
					if ( pTex )
					{
						// assigns effect texture directly to effect
						m_pEffect->SetTexture( m_pEffect->GetParameter( NULL, iParam ), pTex );
					}
					else
					{
						// u64 - 180107 - set the bit to say this texture stage should use a dynamic texture from texture object command
						DWORD dwCorrectBitForThisStage = 1 << dwCountTexturesInEffect;
						m_dwUseDynamicTextureMask = m_dwUseDynamicTextureMask | dwCorrectBitForThisStage;
					}
					dwCountTexturesInEffect++;
				}
				else
				{
					// record this texture and step through texture indexes
					if ( m_dwTextureCount<=31 ) m_iParamOfTexture [ m_dwTextureCount ] = iParam;
					m_dwTextureCount++;
				}
				*/
			}
		}
	}
	#else
	UINT iParametersCount = m_EffectDesc.Parameters;
	D3DXPARAMETER_DESC ParamDesc;
	GGHANDLE hParam;
	for( UINT iParam = 0; iParam < iParametersCount; iParam++ )
	{
		// temp vars
		LPCSTR pstrName = NULL;
		LPCSTR pstrFunction = NULL;
		LPCSTR pstrTarget = NULL;
		LPCSTR pstrTextureType = NULL;
		INT Width = D3DX_DEFAULT;
		INT Height= D3DX_DEFAULT;
		INT Depth = D3DX_DEFAULT;

		// get this parameter handle and description
		hParam = m_pEffect->GetParameter ( NULL, iParam );
		m_pEffect->GetParameterDesc( hParam, &ParamDesc );

		// light handles from DX9 FX files
	    GGHANDLE hAnnot = m_pEffect->GetAnnotationByName( hParam, "UIDirectional" );
        if( hAnnot != NULL ) AssignValueHook ( "UIDirectional", hParam );
	    hAnnot = m_pEffect->GetAnnotationByName( hParam, "UIDirectionalInv" );
        if( hAnnot != NULL ) AssignValueHook ( "UIDirectionalInv", hParam );
	    hAnnot = m_pEffect->GetAnnotationByName( hParam, "UIPosition" );
        if( hAnnot != NULL ) AssignValueHook ( "UIPosition", hParam );

		// light handles from NVIDIA FX files
	    hAnnot = m_pEffect->GetAnnotationByName( hParam, "Object" );
	    if( hAnnot == NULL ) hAnnot = m_pEffect->GetAnnotationByName( hParam, "UIObject" );
        if( hAnnot != NULL )
		{
			// light type
			LPCSTR pstrLightType = NULL;
			if( hAnnot != NULL ) m_pEffect->GetString( hAnnot, &pstrLightType );

			// light space
			LPCSTR pstrLightSpace = NULL;
		    hAnnot = m_pEffect->GetAnnotationByName( hParam, "Space" );
			if( hAnnot != NULL ) m_pEffect->GetString( hAnnot, &pstrLightSpace );

			// assign light position hanle
			AssignValueHook ( (char*)pstrLightType, hParam );
		}

		// basic matrix semantics
		if( ParamDesc.Semantic != NULL && ( ParamDesc.Class == D3DXPC_MATRIX_ROWS || ParamDesc.Class == D3DXPC_MATRIX_COLUMNS ) )
			AssignValueHookCore ( (char*)ParamDesc.Semantic, hParam, ParamDesc.Class, false );

		// basic vector semantics
		if( ParamDesc.Semantic != NULL && ( ParamDesc.Class == D3DXPC_VECTOR ))
			AssignValueHook ( (char*)ParamDesc.Semantic, hParam );

		// basic value semantics
		if( ParamDesc.Semantic != NULL && ( ParamDesc.Class == D3DXPC_SCALAR ))
			AssignValueHook ( (char*)ParamDesc.Semantic, hParam );

		D3DXPARAMETER_DESC AnnotDesc;
		for( UINT iAnnot = 0; iAnnot < ParamDesc.Annotations; iAnnot++ )
		{
			hAnnot = m_pEffect->GetAnnotation ( hParam, iAnnot );
			m_pEffect->GetParameterDesc( hAnnot, &AnnotDesc );

			// texture name
			if ( _strcmpi( AnnotDesc.Name, "resourcename" ) == 0 )		m_pEffect->GetString( hAnnot, &pstrName );
			if ( _strcmpi( AnnotDesc.Name, "name" ) == 0 )		m_pEffect->GetString( hAnnot, &pstrName );
			if ( _strcmpi( AnnotDesc.Name, "file" ) == 0 )		m_pEffect->GetString( hAnnot, &pstrName );

			// texture type
			if ( _strcmpi( AnnotDesc.Name, "resourcetype" ) == 0 )		m_pEffect->GetString( hAnnot, &pstrTextureType );
			if ( _strcmpi( AnnotDesc.Name, "type" ) == 0 )		m_pEffect->GetString( hAnnot, &pstrTextureType );
			if ( _strcmpi( AnnotDesc.Name, "texturetype" ) == 0 )m_pEffect->GetString( hAnnot, &pstrTextureType );

			// texture details
			if ( _strcmpi( AnnotDesc.Name, "function" ) == 0 )	m_pEffect->GetString( hAnnot, &pstrFunction );
			if ( _strcmpi( AnnotDesc.Name, "target" ) == 0 )		m_pEffect->GetString( hAnnot, &pstrTarget );
			if ( _strcmpi( AnnotDesc.Name, "width" ) == 0 )		m_pEffect->GetInt( hAnnot, &Width );
			if ( _strcmpi( AnnotDesc.Name, "height" ) == 0 )		m_pEffect->GetInt( hAnnot, &Height );
			if ( _strcmpi( AnnotDesc.Name, "depth" ) == 0 )		m_pEffect->GetInt( hAnnot, &Depth );
		}
		if( pstrName != NULL )
		{
			// U75 - 200310 - detect RENDERCOLORTARGET semantic here
			bool bIsThisAnRT = false;
			if ( ParamDesc.Semantic != NULL ) if ( _strcmpi ( ParamDesc.Semantic, "RENDERCOLORTARGET" ) == 0 ) bIsThisAnRT = true;
			if ( bIsThisAnRT==true )
			{
				// this indicates an RT texture we want our shader to render to during the passes, so we need to create a render target for it
				IGGTexture* pRTTex = NULL;
				D3DSURFACE_DESC desc;
				IGGSurface *pCurrentRenderTarget = NULL;
				m_pD3D->GetRenderTarget(0,&pCurrentRenderTarget);
				if ( pCurrentRenderTarget )
				{
					pCurrentRenderTarget->GetDesc( &desc );
					if ( Width==D3DX_DEFAULT ) Width=desc.Width;
					if ( Height==D3DX_DEFAULT ) Height=desc.Height;
				}
				else
				{
					if ( Width==D3DX_DEFAULT ) Width=256;
					if ( Height==D3DX_DEFAULT ) Height=256;
				}
				D3DXCreateTexture( m_pD3D, Width, Height, 1, GGUSAGE_RENDERTARGET, GGFMT_A8R8G8B8, D3DPOOL_DEFAULT, (IGGTexture**)&pRTTex );

				// assigns RT texture directly to effect
				m_pEffect->SetTexture( m_pEffect->GetParameter( NULL, iParam ), pRTTex );

				// set flag to indicate this specialeffect object uses at least one RT (render target)
				m_bUsesAtLeastOneRT = true;

				// mark in a bitfield which textures are RT (so we can release them when this shader is deleted)
				DWORD dwCorrectBitForThisTexture = 1 << m_dwTextureCount;
				m_dwCreatedRTTextureMask = m_dwCreatedRTTextureMask | dwCorrectBitForThisTexture;

				// record this texture and step through texture indexes
				if ( m_dwTextureCount<=31 ) m_iParamOfTexture [ m_dwTextureCount ] = iParam;
				m_dwTextureCount++;

				// record this now for later release
				if ( m_dwRTTexCount<=31 ) m_pRTTex [ m_dwRTTexCount ] = pRTTex;
				m_dwRTTexCount++;
			}
			else
			{
				// texture from effect or mesh
				if ( bUseEffectTextures )
				{
					// texture holder
					LPGGBASETEXTURE pTex = NULL;

					// 2D texture is stadnard texture
					if (pstrTextureType != NULL) 
						if( _strcmpi( pstrTextureType, "2d" ) == 0 )
							pstrTextureType=NULL;

					// assign effect texture from FX file
					if (pstrTextureType != NULL) 
					{
						if( _strcmpi( pstrTextureType, "volume" ) == 0 )
						{
							// support for internal volume textures
							LPDIRECT3DVOLUMETEXTURE9 pVolumeTex = NULL;
							if( SUCCEEDED( D3DXCreateVolumeTextureFromFileEx( m_pD3D, pstrName, 
								Width, Height, Depth, 1, 0, GGFMT_UNKNOWN, D3DPOOL_MANAGED,
								D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &pVolumeTex ) ) )
							{
								// iTextureStage
								pTex = pVolumeTex;
							}
						}
						else if( _strcmpi( pstrTextureType, "cube" ) == 0 )
						{
							// support for internal cube textures
							LPGGCUBETEXTURE pCubeTex = NULL;
							if( SUCCEEDED( D3DXCreateCubeTextureFromFileEx( m_pD3D, pstrName, 
								Width, D3DX_DEFAULT, 0, GGFMT_UNKNOWN, D3DPOOL_MANAGED,
								D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &pCubeTex ) ) )
							{
								// iTextureStage
								pTex = pCubeTex;
							}
						}

						// record this now for later release
						if ( m_dwRTTexCount<=31 ) m_pRTTex [ m_dwRTTexCount ] = pTex;
						m_dwRTTexCount++;
					}
					else
					{
						// support for internal basic textures
						int iImageIndex = LoadOrFindTextureAsImage ( (char*)pstrName, "" );
						pTex = GetImagePointer ( iImageIndex );
					}

					// assign texture to effect
					if ( pTex )
					{
						// assigns effect texture directly to effect
						m_pEffect->SetTexture( m_pEffect->GetParameter( NULL, iParam ), pTex );
					}
					else
					{
						// u64 - 180107 - set the bit to say this texture stage should use a dynamic texture from texture object command
						DWORD dwCorrectBitForThisStage = 1 << dwCountTexturesInEffect;
						m_dwUseDynamicTextureMask = m_dwUseDynamicTextureMask | dwCorrectBitForThisStage;
					}
					dwCountTexturesInEffect++;
				}
				else
				{
					// record this texture and step through texture indexes
					if ( m_dwTextureCount<=31 ) m_iParamOfTexture [ m_dwTextureCount ] = iParam;
					m_dwTextureCount++;
				}
			}
		}
	}
	#endif

	#ifdef DX11
	#else
	// Look for default mesh
	if ( bUseEffectXFile )
	{
		D3DXPARAMETER_DESC Desc;
		if( NULL != m_pEffect->GetParameterByName( NULL, "XFile" ) &&
			SUCCEEDED( m_pEffect->GetParameterDesc( "XFile", &Desc ) ) )
		{
			// Store default xfile name
			const char* pStr = NULL;
			m_pEffect->GetString("XFile", &pStr);
			if ( pStr )
			{
				// get default xfile name
				m_pDefaultXFile = new char[strlen(pStr)+1];
				strcpy ( m_pDefaultXFile, pStr );

				// change current mesh with
				LoadRawMesh ( m_pDefaultXFile, &m_pXFileMesh );
			}
		}
	}
	#endif

	// Look for normals/diffuse/tangents/binormals semantic
	#ifdef DX11
	GGEFFECT_DESC EffectDesc;
	ID3DX11EffectTechnique* hTechnique;
	D3DX11_TECHNIQUE_DESC TechniqueDesc;
	ID3DX11EffectPass* hPass;
	m_bUsesNormals = FALSE;
	m_bUsesDiffuse = FALSE;
	m_bUsesTangents = FALSE;
	m_bUsesBinormals = FALSE;
	if ( m_BoneMatrixPaletteHandle )
		m_bUsesBoneData = TRUE;
	else
		m_bUsesBoneData = FALSE;

	m_pEffect->GetDesc( &EffectDesc );
	for( UINT iTech = 0; iTech < EffectDesc.Techniques; iTech++ )
	{
		hTechnique = m_pEffect->GetTechniqueByIndex( iTech );
		hTechnique->GetDesc ( &TechniqueDesc );
		for( UINT iPass = 0; iPass < TechniqueDesc.Passes; iPass++ )
		{
			hPass = hTechnique->GetPassByIndex ( iPass );
			D3DX11_PASS_SHADER_DESC vs_desc;
			hPass->GetVertexShaderDesc(&vs_desc);
			D3DX11_EFFECT_SHADER_DESC s_desc;
			vs_desc.pShaderVariable->GetShaderDesc(0, &s_desc);
            UINT NumVSSemanticsUsed = s_desc.NumInputSignatureEntries;
			for( UINT iSem = 0; iSem < NumVSSemanticsUsed; iSem++ )
			{
				D3D11_SIGNATURE_PARAMETER_DESC pSigParDesc;
				vs_desc.pShaderVariable->GetInputSignatureElementDesc ( 0, iSem, &pSigParDesc );
				if( stricmp ( pSigParDesc.SemanticName, "NORMAL" ) == NULL ) m_bUsesNormals = TRUE;
				if( stricmp ( pSigParDesc.SemanticName, "COLOR" ) == NULL ) m_bUsesDiffuse = TRUE;
				if( stricmp ( pSigParDesc.SemanticName, "TANGENT" ) == NULL ) m_bUsesTangents = TRUE;
				if( stricmp ( pSigParDesc.SemanticName, "BINORMAL" ) == NULL ) m_bUsesBinormals = TRUE;
			}
		}
	}
	#else
	GGEFFECT_DESC EffectDesc;
	GGHANDLE hTechnique;
	D3DXTECHNIQUE_DESC TechniqueDesc;
	GGHANDLE hPass;
	D3DXPASS_DESC PassDesc;
	m_bUsesNormals = FALSE;
	m_bUsesDiffuse = FALSE;
	m_bUsesTangents = FALSE;
	m_bUsesBinormals = FALSE;
	if ( m_BoneMatrixPaletteHandle )
		m_bUsesBoneData = TRUE;
	else
		m_bUsesBoneData = FALSE;

	m_pEffect->GetDesc( &EffectDesc );
	for( UINT iTech = 0; iTech < EffectDesc.Techniques; iTech++ )
	{
		hTechnique = m_pEffect->GetTechnique( iTech );
		m_pEffect->GetTechniqueDesc( hTechnique, &TechniqueDesc );
		for( UINT iPass = 0; iPass < TechniqueDesc.Passes; iPass++ )
		{
			hPass = m_pEffect->GetPass( hTechnique, iPass );
			m_pEffect->GetPassDesc( hPass, &PassDesc );

            UINT NumVSSemanticsUsed;
            D3DXSEMANTIC pVSSemantics[MAXD3DDECLLENGTH];

            #ifndef __GNUC__
            if( !PassDesc.pVertexShaderFunction || FAILED( D3DXGetShaderInputSemantics( PassDesc.pVertexShaderFunction, pVSSemantics, &NumVSSemanticsUsed ) ) )
                continue;

			for( UINT iSem = 0; iSem < NumVSSemanticsUsed; iSem++ )
			{
				if( pVSSemantics[iSem].Usage == GGDECLUSAGE_NORMAL ) m_bUsesNormals = TRUE;
				if( pVSSemantics[iSem].Usage == D3DDECLUSAGE_COLOR ) m_bUsesDiffuse = TRUE;
				if( pVSSemantics[iSem].Usage == D3DDECLUSAGE_TANGENT ) m_bUsesTangents = TRUE;
				if( pVSSemantics[iSem].Usage == D3DDECLUSAGE_BINORMAL ) m_bUsesBinormals = TRUE;
			}
			#endif
		}
	}
	#endif

	// complete
	return true;
}

void cSpecialEffect::ApplyEffect ( sMesh* pMesh )
{
	// Gather and calculate required constants data
    GGGetTransform( GGTS_WORLD, &g_EffectConstant.matWorld );
    GGGetTransform( GGTS_VIEW, &g_EffectConstant.matView );
    GGGetTransform( GGTS_PROJECTION, &g_EffectConstant.matProj );
    g_EffectConstant.matWorldView = g_EffectConstant.matWorld * g_EffectConstant.matView;
    g_EffectConstant.matViewProj = g_EffectConstant.matView * g_EffectConstant.matProj;
    g_EffectConstant.matWorldViewProj = g_EffectConstant.matWorld * g_EffectConstant.matView * g_EffectConstant.matProj;

	// Calculate inverse matrices
	GGMatrixInverse( &g_EffectConstant.matWorldInv, NULL, &g_EffectConstant.matWorld );
	GGMatrixInverse( &g_EffectConstant.matViewInv, NULL, &g_EffectConstant.matView );
	GGMatrixInverse( &g_EffectConstant.matProjInv, NULL, &g_EffectConstant.matProj );
	GGMatrixInverse( &g_EffectConstant.matWorldViewInv, NULL, &g_EffectConstant.matWorldView );

	// Get raw light data
	#ifdef DX11
	#else
	D3DLIGHT9 d3dLight;
	m_pD3D->GetLight(0,&d3dLight);
	if ( d3dLight.Type==GGLIGHT_DIRECTIONAL )
	{
		// get direction directly from structure
	    g_EffectConstant.vecLightDir = GGVECTOR4( d3dLight.Direction.x, d3dLight.Direction.y, d3dLight.Direction.z, 0.0f );
	    g_EffectConstant.vecLightPos = GGVECTOR4( d3dLight.Direction.x*-1000.0f, d3dLight.Direction.y*-1000.0f, d3dLight.Direction.z*-1000.0f, 1.0f );
	}
	else
	{
		// calculate direction from world position
		g_EffectConstant.vecLightDir.x = g_EffectConstant.matWorld._41-d3dLight.Position.x;
		g_EffectConstant.vecLightDir.y = g_EffectConstant.matWorld._42-d3dLight.Position.y;
		g_EffectConstant.vecLightDir.z = g_EffectConstant.matWorld._43-d3dLight.Position.z;
		g_EffectConstant.vecLightDir.w = 1.0f;

		// calculate light position (in object space)
	    g_EffectConstant.vecLightPos.x = d3dLight.Position.x;
	    g_EffectConstant.vecLightPos.y = d3dLight.Position.y;
	    g_EffectConstant.vecLightPos.z = d3dLight.Position.z;
	    g_EffectConstant.vecLightPos.w = 1.0f;
	}
	#endif

	// Calculate light and object-space light(inv)
	GGVec4Transform ( &g_EffectConstant.vecLightDirInv, &g_EffectConstant.vecLightDir, &g_EffectConstant.matWorldInv );
	GGVec4Normalize ( &g_EffectConstant.vecLightDirInv, &g_EffectConstant.vecLightDirInv );
	GGVec4Normalize ( &g_EffectConstant.vecLightDir, &g_EffectConstant.vecLightDir );

	// Get camera psition
    g_EffectConstant.vecCameraPosition = GGVECTOR4( g_EffectConstant.matViewInv._41, g_EffectConstant.matViewInv._42, g_EffectConstant.matViewInv._43, 1.0f );
	g_EffectConstant.vecEyePos = g_EffectConstant.vecCameraPosition;

	// Alpha override component
	if ( m_AlphaOverrideHandle )
	{
		float fPercentage = 1.0f;
		if ( pMesh->bAlphaOverride==true )
			fPercentage = (float)(pMesh->dwAlphaOverride>>24)/255.0f;

		GGSetEffectFloat( m_AlphaOverrideHandle, fPercentage );
	}

	// prepare tranposed matrices for column major matrices
	if ( m_bTranposeToggle )
	{
		GGMatrixTranspose( &g_EffectConstant.matWorld, &g_EffectConstant.matWorld );
		GGMatrixTranspose( &g_EffectConstant.matView, &g_EffectConstant.matView );
		GGMatrixTranspose( &g_EffectConstant.matProj, &g_EffectConstant.matProj );
		GGMatrixTranspose( &g_EffectConstant.matWorldView, &g_EffectConstant.matWorldView );
		GGMatrixTranspose( &g_EffectConstant.matViewProj, &g_EffectConstant.matViewProj );
		GGMatrixTranspose( &g_EffectConstant.matWorldViewProj, &g_EffectConstant.matWorldViewProj );
		GGMatrixTranspose( &g_EffectConstant.matWorldInv, &g_EffectConstant.matWorldInv );
		GGMatrixTranspose( &g_EffectConstant.matViewInv, &g_EffectConstant.matViewInv );
		GGMatrixTranspose( &g_EffectConstant.matProjInv, &g_EffectConstant.matProjInv );
		GGMatrixTranspose( &g_EffectConstant.matWorldViewInv, &g_EffectConstant.matWorldViewInv );
	}

	// leeadd - 290104 - addition of tranposed matrices for effects that use them
	GGMatrixTranspose( &g_EffectConstant.matWorldT, &g_EffectConstant.matWorld );
	GGMatrixTranspose( &g_EffectConstant.matViewT, &g_EffectConstant.matView );
	GGMatrixTranspose( &g_EffectConstant.matProjT, &g_EffectConstant.matProj );
	GGMatrixTranspose( &g_EffectConstant.matWorldInvT, &g_EffectConstant.matWorldInv );
	GGMatrixTranspose( &g_EffectConstant.matViewInvT, &g_EffectConstant.matViewInv );
	GGMatrixTranspose( &g_EffectConstant.matWorldViewInvT, &g_EffectConstant.matWorldViewInv );

	// 270515 - for depth texture motion blur we need the previous worldviewproj
	// and related matrices (from end of frame NOT from this post process camera!)
	g_EffectConstant.matWorldViewT = g_matThisViewProj;
	g_EffectConstant.matViewProjT = g_matThisCameraView;
	g_EffectConstant.matWorldViewProjT = g_matPreviousViewProj;

	// apply latest data to effect
    if( m_pEffect != NULL )
    {
		// main matrices (row major)
        if( m_MatWorldEffectHandle != NULL )
		{
            GGSetEffectMatrix( m_MatWorldEffectHandle, &g_EffectConstant.matWorld );
        }
        if( m_MatViewEffectHandle != NULL )
		{
            GGSetEffectMatrix( m_MatViewEffectHandle, &g_EffectConstant.matView );
        }
        if( m_MatProjEffectHandle != NULL )
		{
            GGSetEffectMatrix( m_MatProjEffectHandle, &g_EffectConstant.matProj );
        }
        if( m_MatWorldViewEffectHandle != NULL )
        {
            GGSetEffectMatrix( m_MatWorldViewEffectHandle, &g_EffectConstant.matWorldView );
        }
        if( m_MatViewProjEffectHandle != NULL )
        {
            GGSetEffectMatrix( m_MatViewProjEffectHandle, &g_EffectConstant.matViewProj );
        }
        if( m_MatWorldViewProjEffectHandle != NULL )
        {
            GGSetEffectMatrix( m_MatWorldViewProjEffectHandle, &g_EffectConstant.matWorldViewProj );
        }
        if( m_MatWorldInverseEffectHandle != NULL )
        {
            GGSetEffectMatrix( m_MatWorldInverseEffectHandle, &g_EffectConstant.matWorldInv );
        }		
        if( m_MatViewInverseEffectHandle != NULL )
        {
            GGSetEffectMatrix( m_MatViewInverseEffectHandle, &g_EffectConstant.matViewInv );
        }
        if( m_MatProjectionInverseEffectHandle != NULL )
        {
            GGSetEffectMatrix( m_MatProjectionInverseEffectHandle, &g_EffectConstant.matProjInv );
        }
        if( m_MatWorldViewInverseEffectHandle != NULL )
        {
            GGSetEffectMatrix( m_MatWorldViewInverseEffectHandle, &g_EffectConstant.matWorldViewInv );
        }

		// tranposed matrices (column major)
        if( m_MatWorldTEffectHandle != NULL )
		{
            GGSetEffectMatrix( m_MatWorldTEffectHandle, &g_EffectConstant.matWorldT );
        }
        if( m_MatViewTEffectHandle != NULL )
		{
            GGSetEffectMatrix( m_MatViewTEffectHandle, &g_EffectConstant.matViewT );
        }
        if( m_MatProjTEffectHandle != NULL )
		{
            GGSetEffectMatrix( m_MatProjTEffectHandle, &g_EffectConstant.matProjT );
        }
        if( m_MatWorldViewTEffectHandle != NULL )
        {
            GGSetEffectMatrix( m_MatWorldViewTEffectHandle, &g_EffectConstant.matWorldViewT );
        }
        if( m_MatViewProjTEffectHandle != NULL )
        {
            GGSetEffectMatrix( m_MatViewProjTEffectHandle, &g_EffectConstant.matViewProjT );
        }
        if( m_MatWorldViewProjTEffectHandle != NULL )
        {
            GGSetEffectMatrix( m_MatWorldViewProjTEffectHandle, &g_EffectConstant.matWorldViewProjT );
        }
        if( m_MatWorldInverseTEffectHandle != NULL )
        {
            GGSetEffectMatrix( m_MatWorldInverseTEffectHandle, &g_EffectConstant.matWorldInvT );
        }		
        if( m_MatViewInverseTEffectHandle != NULL )
        {
            GGSetEffectMatrix( m_MatViewInverseTEffectHandle, &g_EffectConstant.matViewInvT );
        }		
        if( m_MatWorldViewInverseTEffectHandle != NULL )
        {
            GGSetEffectMatrix( m_MatWorldViewInverseTEffectHandle, &g_EffectConstant.matWorldViewInvT );
        }

		// main vectors
		if ( m_LightDirHandle != NULL )
		{
            GGSetEffectVector( m_LightDirHandle, &g_EffectConstant.vecLightDir );
		}
		if ( m_LightDirInvHandle != NULL )
		{
            GGSetEffectVector( m_LightDirInvHandle, &g_EffectConstant.vecLightDirInv );
		}
		if ( m_LightPosHandle != NULL )
		{
            GGSetEffectVector( m_LightPosHandle, &g_EffectConstant.vecLightPos );
		}
        if( m_VecCameraPosEffectHandle != NULL )
        {
            GGSetEffectVector( m_VecCameraPosEffectHandle, &g_EffectConstant.vecCameraPosition );
        }
		if ( m_VecEyePosEffectHandle != NULL )
		{
            GGSetEffectVector( m_VecEyePosEffectHandle, &g_EffectConstant.vecEyePos );
		}

		// misclanious values
		if( m_TimeEffectHandle != NULL )
		{
			float fTime = timeGetSecond();
			GGSetEffectFloat( m_TimeEffectHandle, fTime );
		}
		if( m_SinTimeEffectHandle != NULL )
		{
			// TIME DATA IN SECONDS
			//float fSinTime = ((float)sin(timeGetTime())) / 1000.0f;
			float fSinTime = sin(timeGetSecond());
			GGSetEffectFloat( m_SinTimeEffectHandle, fSinTime );
		}
		if( m_DeltaTimeEffectHandle != NULL )
		{
			// DELTA TIME DATA IN SECONDS
			float fTimeNow = timeGetSecond();
			float fDeltaTime = fTimeNow - g_fLastDeltaTime;
			GGSetEffectFloat( m_DeltaTimeEffectHandle, fDeltaTime );
			g_fLastDeltaTime = fTimeNow;
		}
		if ( m_UVScalingHandle != NULL )
		{
			g_EffectConstant.vecUVScaling = GGVECTOR4 ( pMesh->fUVScalingU, pMesh->fUVScalingV, 0, 0 );
            GGSetEffectVector( m_UVScalingHandle, &g_EffectConstant.vecUVScaling );
		}

		// set bone matrix palette if required
		if ( m_BoneMatrixPaletteHandle )
		{
			// update all bone matrices
			DWORD dwBoneMax = pMesh->dwBoneCount;
			if ( dwBoneMax > 170 ) dwBoneMax = 170; // 121018 - was 60 from old Shader Model 3.0 days
			// send bone count to shader (so can skip bone anim if nothing in palette)
			///g_EffectConstant.fBoneCount = (float)dwBoneMax;
			///GGSetEffectFloat( m_BoneCountHandle, g_EffectConstant.fBoneCount );

			// update matrix palette if any
			if ( pMesh->dwForceCPUAnimationMode==1 )
			{
				// CPU does animation (or no anim transforms sent to shader)
				for ( DWORD dwMatrixIndex = 0; dwMatrixIndex < dwBoneMax; dwMatrixIndex++ )
					GGMatrixIdentity ( &g_EffectConstant.matBoneMatrixPalette [ dwMatrixIndex ] );
			}
			else
			{
				// GPU needs matrices to do animation
				for ( DWORD dwMatrixIndex = 0; dwMatrixIndex < dwBoneMax; dwMatrixIndex++ )
					if ( pMesh->pFrameMatrices [ dwMatrixIndex ] )
						GGMatrixMultiply ( &g_EffectConstant.matBoneMatrixPalette [ dwMatrixIndex ], &pMesh->pBones [ dwMatrixIndex ].matTranslation, pMesh->pFrameMatrices [ dwMatrixIndex ] );
			}

			// send matrix array to effect (column-based is default by FX compiler)
            GGSetEffectMatrixTransposeArray ( m_BoneMatrixPaletteHandle, g_EffectConstant.matBoneMatrixPalette, dwBoneMax );
		}
    }
}

DARKSDK_DLL bool CreateMesh ( sObject** pObject, LPSTR pName )
{
	// create a new, empty mesh

	// the pointer must be valid
	SAFE_MEMORY ( pObject );

	// create a new object and check allocation
	*pObject = new sObject;
	SAFE_MEMORY ( pObject );

	// create a new frame and check allocation
	pObject [ 0 ]->pFrame = new sFrame;
	SAFE_MEMORY ( pObject [ 0 ]->pFrame );

	// finally create the mesh object
	pObject [ 0 ]->pFrame->pMesh = new sMesh;
	SAFE_MEMORY ( pObject [ 0 ]->pFrame->pMesh );

	// give it a name for reference
	if ( pName )
	{
		if ( strlen(pName) < MAX_STRING )
			strcpy(pObject [ 0 ]->pFrame->szName, pName);
	}

	// all went okay
	return true;
}

DARKSDK_DLL bool DeleteMesh ( sObject** pObject )
{
	#ifdef WICKEDENGINE
	// must remove object from scene
	WickedCall_RemoveObject(*pObject);
	#else
	// Before we delete object, remove from any temp lists
    if ( !m_ObjectManager.m_vVisibleObjectList.empty() )
    {
        for ( DWORD iIndex = 0; iIndex < m_ObjectManager.m_vVisibleObjectList.size(); ++iIndex )
        {
            sObject* pThisObject = m_ObjectManager.m_vVisibleObjectList [ iIndex ];
			if ( pThisObject==pThisObject )
			{
				//m_ObjectManager.m_vVisibleObjectList [ iIndex ] = NULL;
				m_ObjectManager.m_vVisibleObjectList.erase(m_ObjectManager.m_vVisibleObjectList.begin() + iIndex);
			}
		}
	}
    if ( !m_ObjectManager.m_vVisibleObjectEarly.empty() )
    {
        for ( DWORD iIndex = 0; iIndex < m_ObjectManager.m_vVisibleObjectEarly.size(); ++iIndex )
        {
            sObject* pThisObject = m_ObjectManager.m_vVisibleObjectEarly [ iIndex ];
			if ( pThisObject==pThisObject )
			{
				//m_ObjectManager.m_vVisibleObjectEarly [ iIndex ] = NULL;
				m_ObjectManager.m_vVisibleObjectEarly.erase(m_ObjectManager.m_vVisibleObjectEarly.begin() + iIndex);
			}
		}
	}
    if ( !m_ObjectManager.m_vVisibleObjectTransparent.empty() )
    {
        for ( DWORD iIndex = 0; iIndex < m_ObjectManager.m_vVisibleObjectTransparent.size(); ++iIndex )
        {
            sObject* pThisObject = m_ObjectManager.m_vVisibleObjectTransparent [ iIndex ];
			if ( pThisObject==pThisObject )
			{
				//m_ObjectManager.m_vVisibleObjectTransparent [ iIndex ] = NULL;
				m_ObjectManager.m_vVisibleObjectTransparent.erase(m_ObjectManager.m_vVisibleObjectTransparent.begin() + iIndex);
			}
		}
	}
    if ( !m_ObjectManager.m_vVisibleObjectNoZDepth.empty() )
    {
        for ( DWORD iIndex = 0; iIndex < m_ObjectManager.m_vVisibleObjectNoZDepth.size(); ++iIndex )
        {
            sObject* pThisObject = m_ObjectManager.m_vVisibleObjectNoZDepth [ iIndex ];
			if ( pThisObject==pThisObject )
			{
				//m_ObjectManager.m_vVisibleObjectNoZDepth [ iIndex ] = NULL;
				m_ObjectManager.m_vVisibleObjectNoZDepth.erase(m_ObjectManager.m_vVisibleObjectNoZDepth.begin() + iIndex);
			}
		}
	}
    if ( !m_ObjectManager.m_vVisibleObjectStandard.empty() )
    {
        for ( DWORD iIndex = 0; iIndex < m_ObjectManager.m_vVisibleObjectStandard.size(); ++iIndex )
        {
            sObject* pThisObject = m_ObjectManager.m_vVisibleObjectStandard [ iIndex ];
			if ( pThisObject==pThisObject )
			{
				//m_ObjectManager.m_vVisibleObjectStandard [ iIndex ] = NULL;
				m_ObjectManager.m_vVisibleObjectStandard.erase(m_ObjectManager.m_vVisibleObjectStandard.begin() + iIndex);
			}
		}
	}
	#endif

	// Delete allocations
	SAFE_DELETE( *pObject );

	// all went okay
	return true;
}

DARKSDK_DLL void UpdateEulerRotation ( sObject* pObject )
{
	// euler rotation 
	GGMatrixRotationX ( &pObject->position.matRotateX, GGToRadian ( pObject->position.vecRotate.x ) );
	GGMatrixRotationY ( &pObject->position.matRotateY, GGToRadian ( pObject->position.vecRotate.y ) );
	GGMatrixRotationZ ( &pObject->position.matRotateZ, GGToRadian ( pObject->position.vecRotate.z ) );

	// choose rotation order for euler matrix
	switch ( pObject->position.dwRotationOrder )
	{
		case ROTORDER_XYZ :	
				pObject->position.matRotation =	pObject->position.matRotateX *
												pObject->position.matRotateY *
												pObject->position.matRotateZ;
				break;

		case ROTORDER_ZYX :	
				pObject->position.matRotation =	pObject->position.matRotateZ *
												pObject->position.matRotateY *
												pObject->position.matRotateX;
				break;

		case ROTORDER_ZXY :
				pObject->position.matRotation = pObject->position.matRotateZ *
												pObject->position.matRotateX *
												pObject->position.matRotateY;
				break;
	}
}

DARKSDK_DLL void UpdateObjectRotation ( sObject* pObject )
{
	if ( pObject->position.bFreeFlightRotation==false )
		UpdateEulerRotation ( pObject );
	else
		pObject->position.matRotation = pObject->position.matFreeFlightRotate;
}

DARKSDK_DLL bool CalcObjectWorld ( sObject* pObject )
{
	// special handling if the object is glued to something (need abs world pos in same cycle so need calc)
	sFrame* pGluedToFramePtr = NULL;
	if ( pObject->position.iGluedToObj )
	{
		if ( g_ObjectList [ pObject->position.iGluedToObj ] != NULL )
		{
			sObject* pOriginalGlueToObj = g_ObjectList [ pObject->position.iGluedToObj ];
			sObject* pChildObject = pOriginalGlueToObj;
			if ( pChildObject )
			{
				if ( pChildObject->ppFrameList == NULL ) 
				{
					pChildObject = pChildObject->pInstanceOfObject;
				}
				if ( pChildObject && pChildObject->ppFrameList != NULL )
				{
					// Must update any object glued to (for current absolute world data) -recurse!
					CalcObjectWorld ( pChildObject );

					// 051205 - if mode 1, wipe out frame orient, leaving position only
					int iFrame = pObject->position.iGluedToMesh;
					int iMode = 0; if ( iFrame < 0 ) { iFrame *= -1; iMode=1; }

					// Identify and extra frame ptr
					if ( iFrame < pChildObject->iFrameCount )
					{
						// get actual frame ptr
						pGluedToFramePtr = pChildObject->ppFrameList[ iFrame ];

						// Mode 1 is set by issuing a negative mesh id
						if ( iMode==1 )
						{
							GGVECTOR3 vecPos = GGVECTOR3 ( pGluedToFramePtr->matCombined._41, pGluedToFramePtr->matCombined._42, pGluedToFramePtr->matCombined._43 );
							GGMatrixIdentity ( &pGluedToFramePtr->matCombined );
							pGluedToFramePtr->matCombined._41 = vecPos.x;
							pGluedToFramePtr->matCombined._42 = vecPos.y;
							pGluedToFramePtr->matCombined._43 = vecPos.z;
						}

						// leefix - 100303 - Calculate correct absolute world matrix
						CalculateAbsoluteWorldMatrix ( pOriginalGlueToObj, pGluedToFramePtr, pGluedToFramePtr->pMesh );
					}
				}
			}
		}
	}

	// setup the world matrix for the object (and ensures matWorld (used by LimbPosition) respects a glued status
	CalculateObjectWorld ( pObject, pGluedToFramePtr );

	// finally pass to wicked to update transform
	#ifdef WICKEDENGINE
	WickedCall_UpdateObject(pObject);
	#endif

	// success
	return true;
}

DARKSDK_DLL bool CalculateObjectWorld ( sObject* pObject, sFrame* pGluedToFramePtr )
{
	if ( pObject->position.bCustomWorldMatrix == true )
	{
		// return with success
		return true;
	}

	// create a scaling and position matrix
	GGMatrixScaling ( &pObject->position.matScale, pObject->position.vecScale.x, pObject->position.vecScale.y, pObject->position.vecScale.z );
	GGMatrixTranslation ( &pObject->position.matTranslation, pObject->position.vecPosition.x, pObject->position.vecPosition.y, pObject->position.vecPosition.z );

	// GLobal setting to shrink each object (to defeat shadow map self shadowing effect)
	if ( g_fShrinkObjectsTo > 0.0f )
	{
		GGMATRIX matShrinkScale;
		GGMatrixScaling ( &matShrinkScale, g_fShrinkObjectsTo, g_fShrinkObjectsTo, g_fShrinkObjectsTo );
		GGMatrixMultiply ( &pObject->position.matScale, &pObject->position.matScale, &matShrinkScale );
	}
 
	LPVOID pWhenThisCHanges = (LPVOID)&pObject->position.vecPosition.x;

	// handle rotation as either euler or freeflight
	UpdateObjectRotation ( pObject );

	// Apply pivot if any
	if ( pObject->position.bApplyPivot )
	{
		// modify current rotation
		pObject->position.matRotation = pObject->position.matPivot * pObject->position.matRotation;
	}

	// build up final rotation and world matrix
	pObject->position.matObjectNoTran = pObject->position.matScale * pObject->position.matRotation;
	pObject->position.matWorld = pObject->position.matObjectNoTran * pObject->position.matTranslation;

	// Apply glue world-matrix if any
	if ( pObject->position.bGlued && pGluedToFramePtr )
	{
		// find target object:mesh
		sFrame* pTargetFrame = pGluedToFramePtr;
		if ( pTargetFrame )
		{
			// apply object world then limb world
			pObject->position.matWorld *= pTargetFrame->matAbsoluteWorld;

			// no trans taken from target, then clear translation data
			pObject->position.matObjectNoTran = pObject->position.matWorld;
			pObject->position.matObjectNoTran._41 = 0.0f;
			pObject->position.matObjectNoTran._42 = 0.0f;
			pObject->position.matObjectNoTran._43 = 0.0f;
		}
	}

	// all frames should be flagged for recalc of vectors (need data prior to sync!)
	if ( pObject->ppFrameList )
	{
		for ( int iCurrentFrame = 0; iCurrentFrame < pObject->iFrameCount; iCurrentFrame++ )
		{
			sFrame* pFrame = pObject->ppFrameList [ iCurrentFrame ];
			if ( pFrame ) pFrame->bVectorsCalculated = false;
		}
	}

	// success
	return true;
}

DARKSDK_DLL void CalculateAbsoluteWorldMatrix ( sObject* pObject, sFrame* pFrame, sMesh* pMesh )
{
	// bone or frame animation (bone anim includes frame matrix adjustment)
	bool bBoneAnimation = false;
	if ( pMesh )
	{
		// leefix - 110303 - even static bone models must apply the combined factor
		if ( pMesh->dwBoneCount )//&& (pObject->bAnimPlaying || pObject->bAnimManualSlerp) )
		{
			bBoneAnimation = true;
		}
	}

	// set the absolute matrix for the frame
	// 151003 - i have now added this to 'UpdateRealtimeFrameVectors' in dboframe.cpp
	if ( bBoneAnimation )
		pFrame->matAbsoluteWorld = pObject->position.matWorld;
	else
		pFrame->matAbsoluteWorld = pFrame->matCombined * pObject->position.matWorld;

	// 090416 - apply original transform of this frame as FBX models needed transforms applying uniformally
	if ( pObject->dwApplyOriginalScaling==1 || (pObject->pInstanceOfObject && pObject->pInstanceOfObject->dwApplyOriginalScaling==1) )
	{
		pFrame->matAbsoluteWorld = pFrame->matOriginal * pFrame->matAbsoluteWorld;
	}
}

