
//
// DBOMesh Functions Implementation
//

//////////////////////////////////////////////////////////////////////////////////
// DBOMESH HEADER ////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//#define _CRT_SECURE_NO_DEPRECATE
#include "DBOMesh.h"
#include "DBOEffects.h"
#include "stdio.h"
#include <mmsystem.h>			// multimedia functions
#include "..\..\..\Include\CImageC.h"

#ifdef WICKEDENGINE
#include ".\..\..\..\..\Guru-WickedMAX\wickedcalls.h"
#endif

// Externals for DBO/Manager relationship
#include <vector>
#ifndef WICKEDENGINE
extern std::vector< sMesh* >		g_vRefreshMeshList;
#endif

// Prototypes
DARKSDK void ConvertToFVF				( sMesh* pMesh, DWORD dwFVF );
DARKSDK void SmoothNormals				( sMesh* pMesh, float fAngle );

//////////////////////////////////////////////////////////////////////////////////
// LOCAL CLASS TO HELP WITH SHADOW CALCULATION ///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
#define ADJACENCY_EPSILON 0.0001f
#define EXTRUDE_EPSILON 0.1f

struct CEdgeMapping
{
    int m_anOldEdge[2];  // vertex index of the original edge
    int m_aanNewEdge[2][2]; // vertex indexes of the new edge
                            // First subscript = index of the new edge
                            // Second subscript = index of the vertex for the edge

public:
    CEdgeMapping()
    {
        FillMemory( m_anOldEdge, sizeof(m_anOldEdge), -1 );
        FillMemory( m_aanNewEdge, sizeof(m_aanNewEdge), -1 );
    }
};

//////////////////////////////////////////////////////////////////////////////////
// INTERNAL MESH HELPER FUNCTIONS ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

DARKSDK_DLL bool EnsureTextureStageValid ( sMesh* pMesh, int iTextureStage )
{
	// textureindex
	DWORD dwTextureIndex = 1+iTextureStage;

	// create texture array if not present or too small
	if ( pMesh->dwTextureCount < dwTextureIndex )
	{
		sTexture* pNewTextureArray = NULL;
		pNewTextureArray = new sTexture [ dwTextureIndex ];
		memset(pNewTextureArray, 0, sizeof(pNewTextureArray));
		g_pGlob->dwInternalFunctionCode=11013;
		if ( pMesh->pTextures ) memcpy ( pNewTextureArray, pMesh->pTextures, sizeof(sTexture) * pMesh->dwTextureCount );

		// Remove old array
		SAFE_DELETE_ARRAY( pMesh->pTextures );

		// Assign new texture array
		pMesh->pTextures = pNewTextureArray;
		pMesh->dwTextureCount = dwTextureIndex;
	}

	// okay
	return true;
}

DARKSDK_DLL float AlphaFromRGBA ( DWORD dwRGB )
{
	return ((dwRGB & (255 << 24)) >> 24) / 255.0f;
}

DARKSDK_DLL float RedFromRGBA ( DWORD dwRGB )
{
	return ((dwRGB & (255 << 16)) >> 16) / 255.0f;
}

DARKSDK_DLL float GreenFromRGBA ( DWORD dwRGB )
{
	return ((dwRGB & (255 <<  8)) >>  8) / 255.0f;
}

DARKSDK_DLL float BlueFromRGBA ( DWORD dwRGB )
{
	return ((dwRGB & (255 <<  0)) >>  0) / 255.0f;
}

DARKSDK_DLL void ResetMaterial (D3DMATERIAL9PRETEND* pMaterial )
{
	pMaterial->Diffuse.r		= 1.0f;
	pMaterial->Diffuse.g		= 1.0f;
	pMaterial->Diffuse.b		= 1.0f;
	pMaterial->Diffuse.a		= 1.0f;
	pMaterial->Ambient.r		= 1.0f;
	pMaterial->Ambient.g		= 1.0f;
	pMaterial->Ambient.b		= 1.0f;
	pMaterial->Ambient.a		= 1.0f;
	pMaterial->Specular.r		= 0.0f;
	pMaterial->Specular.g		= 0.0f;
	pMaterial->Specular.b		= 0.0f;
	pMaterial->Specular.a		= 0.0f;
	pMaterial->Emissive.r		= 0.0f;
	pMaterial->Emissive.g		= 0.0f;
	pMaterial->Emissive.b		= 0.0f;
	pMaterial->Emissive.a		= 0.0f;
	pMaterial->Power			= 10.0f;
}

DARKSDK_DLL void ColorMaterial (D3DMATERIAL9PRETEND* pMaterial, DWORD dwRGBA )
{
	// Set Diffuse Of Material
	pMaterial->Diffuse.r = RedFromRGBA		( dwRGBA );
	pMaterial->Diffuse.g = GreenFromRGBA	( dwRGBA );
	pMaterial->Diffuse.b = BlueFromRGBA		( dwRGBA );
	pMaterial->Diffuse.a = AlphaFromRGBA	( dwRGBA );
}

DARKSDK_DLL GGVECTOR3 MultiplyVectorAndMatrix ( GGVECTOR3 &vec, GGMATRIX &m )
{
	GGVECTOR3 vecFinal;

	vecFinal.x = vec.x * m._11 + vec.y * m._21 + vec.z * m._31 + m._41;
	vecFinal.y = vec.x * m._12 + vec.y * m._22 + vec.z * m._32 + m._42;
	vecFinal.z = vec.x * m._13 + vec.y * m._23 + vec.z * m._33 + m._43;

	return vecFinal;
}

DARKSDK_DLL void LightEval(GGVECTOR4 *col,GGVECTOR2 *input, GGVECTOR2 *sampSize,void *pfPower)
{
    float fPower = (float) pow(input->y,*((float*)pfPower));
    col->x = fPower;
    col->y = fPower;
    col->z = fPower;
    col->w = input->x;
}

//////////////////////////////////////////////////////////////////////////////////
// INTERNAL MESH SHADER FUNCTIONS ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

DARKSDK_DLL BOOL SupportsVertexShaderV11()
{
	#ifdef DX11
	return TRUE;
	#else
    GGCAPS d3dCaps;
    m_pD3D->GetDeviceCaps( &d3dCaps );
    if ( d3dCaps.VertexShaderVersion >= D3DVS_VERSION ( 1,1 ) )
	    return TRUE;
	#endif
    return FALSE;
}

DARKSDK_DLL BOOL SupportsPixelShaderV11()
{
	#ifdef DX11
	return TRUE;
	#else
    GGCAPS d3dCaps;
    m_pD3D->GetDeviceCaps( &d3dCaps );
    if ( d3dCaps.PixelShaderVersion >= D3DPS_VERSION ( 1,1 ) )
	    return TRUE;
	#endif
    return FALSE;
}

DARKSDK_DLL void FreeVertexShaderMesh ( sMesh* pMesh ) 
{
	// store FVF before making shader
	if ( pMesh->pVertexShaderEffect )
	{
		// restore FVF format (if original known)
		if ( pMesh->dwFVF==0 && pMesh->dwFVFOriginal!=0 )
			RestoreLocalMesh ( pMesh );

		// if reference, simply blank effectptr
		if ( pMesh->bVertexShaderEffectRefOnly==true )
		{
			// clear reference from mesh
			pMesh->pVertexShaderEffect = NULL;

			// leefix - 011013 - should at least wipe ptrs out
			pMesh->pVertexShader = NULL;

			// 160616 - keep hold of this in case a shader replaces another that uses SAME vert dec (GG character shader)
			// as a fundamental problem is that the orig vertdata is WIPED OUT during the shader/clone process so you cannot
			// generate the shader verts again to create pVertexDec a fresh, and it gets wiped out
			//pMesh->pVertexDec = NULL;

			// 221114 - and also wipe any reference to shader use
			//pMesh->bUseVertexShader = false; see below
			pMesh->bOverridePixelShader = false;
		}
		else
		{
			// release any previous shader
			#ifdef DX11
			#else
			SAFE_DELETE ( pMesh->pVertexShaderEffect );
			SAFE_RELEASE ( pMesh->pVertexShader );
			SAFE_RELEASE ( pMesh->pVertexDec );
			#endif

			// deactivate shader usage
			//pMesh->bUseVertexShader = false; see below
			pMesh->bOverridePixelShader = false;
		}

		// clear from mesh
		pMesh->bVertexShaderEffectRefOnly = false;
		//strcpy ( pMesh->pEffectName, "" ); see below
	}

	// 100718 - moved from above so absolutely sure shader is wiped out
	pMesh->bUseVertexShader = false;
	strcpy ( pMesh->pEffectName, "" );
}

DARKSDK_DLL void ClearTextureSettings ( sMesh* pMesh )
{
	// create texture array if not present
	if ( !EnsureTextureStageValid ( pMesh, 0 ) )
		return;

	// clear alpha factor of mesh
	pMesh->dwAlphaOverride = 0;
	pMesh->bAlphaOverride = false;

	// get texture ptr
	sTexture* pTexture = &pMesh->pTextures [ 0 ];

	// free resource
	#ifdef DX11
	#else
	SAFE_RELEASE ( pTexture->pCubeTexture );
	#endif

	// set base texture to defaults
	#ifdef DX11
	#else
	pTexture->dwStage=0;
	pTexture->dwBlendMode=GGTOP_SELECTARG1;	
	pTexture->dwBlendArg1=GGTA_DIFFUSE;
	pTexture->dwBlendArg2=GGTA_DIFFUSE;
	pTexture->dwTexCoordMode=0;
	#endif

	// Delete any vertex shader being used
	FreeVertexShaderMesh ( pMesh );
}

DARKSDK_DLL bool ValidateMeshForShader ( sMesh* pMesh, DWORD dwStagesRequired )
{
	// Delete any vertex shader being used
	FreeVertexShaderMesh ( pMesh );

	// check support
	if(!SupportsVertexShaderV11())
		return false;

	// create a two stage texture array
	if ( !EnsureTextureStageValid ( pMesh, dwStagesRequired-1 ) )
		return false;

	// store original FVF before making shader
	if ( pMesh->dwFVFOriginal==0 && pMesh->dwFVF>0 )
		pMesh->dwFVFOriginal = pMesh->dwFVF;

	// okay
	return true;
}

//////////////////////////////////////////////////////////////////////////////////
// MESH FUNCTIONS ////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

// Mesh Setting Functions

DARKSDK_DLL void SetWireframe ( sMesh* pMesh, bool bWireframe )
{
	pMesh->bWireframe = bWireframe;
}

DARKSDK_DLL void SetTransparency ( sMesh* pMesh, bool bTransparency )
{
	pMesh->bTransparency = bTransparency;
}

DARKSDK_DLL void SetAlphaTest ( sMesh* pMesh, DWORD dwAlphaTestValue )
{
	pMesh->dwAlphaTestValue = dwAlphaTestValue;
}

DARKSDK_DLL void SetCullCWCCW ( sMesh* pMesh, int iCullMode )
{
	if ( iCullMode>0 )
	{
		pMesh->iCullMode = iCullMode;
		pMesh->bCull = true;
	}
	else
		pMesh->bCull = false;
}

DARKSDK_DLL void SetCull ( sMesh* pMesh, bool bCull )
{
	pMesh->bCull = bCull;
}

DARKSDK_DLL void SetZRead ( sMesh* pMesh, bool bZRead )
{
	pMesh->bZRead = bZRead;
}

DARKSDK_DLL void SetZWrite ( sMesh* pMesh, bool bZWrite )
{
	// 010220 - protect zwrite state if flagged
	if (pMesh->bProtectZWriteState == false)
	{
		pMesh->bZWrite = bZWrite;
	}

	#ifdef WICKEDENGINE
	// transparency can set opaque, alpha (or in the case of z=false, additive with no Z writing (DSS_READ)
	WickedCall_SetMeshTransparent(pMesh);
	#endif
}

DARKSDK_DLL void SetZBias ( sMesh* pMesh, bool bZBias, float fSlopeScale, float fDepth )
{
	pMesh->bZBiasActive = bZBias;
	pMesh->fZBiasSlopeScale = fSlopeScale;
	pMesh->fZBiasDepth = fDepth;
}

DARKSDK_DLL void SetFilter ( sMesh* pMesh, int iStage, int iFilter )
{
	#ifdef DX11
	#else
	if ( (DWORD)iStage<pMesh->dwTextureCount )
	{
		iFilter++; // DBV1 compatable.
		//    GGTEXF_NONE = 0,
		//    D3DTEXF_POINT = 1,
		//    GGTEXF_LINEAR = 2,
		//    D3DTEXF_ANISOTROPIC = 3,
		//    D3DTEXF_PYRAMIDALQUAD = 6,
		//    D3DTEXF_GAUSSIANQUAD = 7,
		sTexture* pTexture = &pMesh->pTextures[iStage];
		if(pTexture)
		{
			// confirm existence of filter in hardware, else use default
		    GGCAPS d3dCaps;
			m_pD3D->GetDeviceCaps( &d3dCaps );
			DWORD dwMinCapsFlag = 0, dwMagCapsFlag = 0;
			if ( iFilter==GGTEXF_LINEAR ) { dwMinCapsFlag = D3DPTFILTERCAPS_MINFLINEAR; dwMagCapsFlag = D3DPTFILTERCAPS_MAGFLINEAR; }
			if ( iFilter==D3DTEXF_ANISOTROPIC ) { dwMinCapsFlag = D3DPTFILTERCAPS_MINFANISOTROPIC; dwMagCapsFlag = D3DPTFILTERCAPS_MAGFANISOTROPIC; }
			if ( iFilter==D3DTEXF_PYRAMIDALQUAD ) { dwMinCapsFlag = D3DPTFILTERCAPS_MINFPYRAMIDALQUAD; dwMagCapsFlag = D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD; }
			if ( iFilter==D3DTEXF_GAUSSIANQUAD ) { dwMinCapsFlag = D3DPTFILTERCAPS_MINFGAUSSIANQUAD; dwMagCapsFlag = D3DPTFILTERCAPS_MAGFGAUSSIANQUAD; }
			(d3dCaps.TextureFilterCaps & dwMinCapsFlag) ? pTexture->dwMinState = iFilter : pTexture->dwMinState = D3DTEXF_POINT;
			(d3dCaps.TextureFilterCaps & dwMagCapsFlag) ? pTexture->dwMagState = iFilter : pTexture->dwMagState = D3DTEXF_POINT;
		}
	}
	#endif
}

DARKSDK_DLL void SetFilter ( sMesh* pMesh, int iFilter )
{
	SetFilter ( pMesh, 0, iFilter );
}

DARKSDK_DLL void SetLight ( sMesh* pMesh, bool bLight )
{
	pMesh->bLight = bLight;
}

DARKSDK_DLL void SetFog ( sMesh* pMesh, bool bFog )
{
	pMesh->bFog = bFog;
}

DARKSDK_DLL void SetAmbient ( sMesh* pMesh, bool bAmbient )
{
	pMesh->bAmbient = bAmbient;
}

// Mesh Component Data Functions

DARKSDK_DLL void SetDiffuseEx ( sMesh* pMesh, DWORD dwRGB )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// make sure we have data in the vertices
	if ( pMesh->dwFVF & GGFVF_DIFFUSE )
	{
		// go through all of the vertices
		for ( int iCurrentVertex = 0; iCurrentVertex < (int)pMesh->dwVertexCount; iCurrentVertex++ )
		{
			// dwDiffuse = RGB
			*( ( DWORD* ) pMesh->pVertexData + offsetMap.dwDiffuse + ( offsetMap.dwSize * iCurrentVertex ) ) = dwRGB;
		}
	}

	// flag mesh for a VB update
	pMesh->bVBRefreshRequired = true;
#ifndef WICKEDENGINE
	g_vRefreshMeshList.push_back ( pMesh );
#endif
}

DARKSDK_DLL void SetDiffuse	( sMesh* pMesh, float fPercentage )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// make sure we have data in the vertices
	if ( pMesh->dwFVF & GGFVF_DIFFUSE )
	{
		// calculate an RGB from iPercentage
		DWORD dwColor = (DWORD)(fPercentage*255);
		DWORD dwRGB = GGCOLOR_ARGB ( dwColor, dwColor, dwColor, dwColor );
		SetDiffuseEx ( pMesh, dwRGB );
	}
	else
	{
		// else apply diffuse to material and activate
		pMesh->bUsesMaterial=true;
		pMesh->mMaterial.Diffuse.r = fPercentage;
		pMesh->mMaterial.Diffuse.g = fPercentage;
		pMesh->mMaterial.Diffuse.b = fPercentage;
		pMesh->mMaterial.Diffuse.a = 1.0f;
		pMesh->mMaterial.Ambient.r = fPercentage;
		pMesh->mMaterial.Ambient.g = fPercentage;
		pMesh->mMaterial.Ambient.b = fPercentage;
		pMesh->mMaterial.Ambient.a = 1.0f;
		pMesh->mMaterial.Specular.r = 0.0f;
		pMesh->mMaterial.Specular.g = 0.0f;
		pMesh->mMaterial.Specular.b = 0.0f;
		pMesh->mMaterial.Specular.a = 0.0f;
		pMesh->mMaterial.Emissive.r = 0.0f;
		pMesh->mMaterial.Emissive.g = 0.0f;
		pMesh->mMaterial.Emissive.b = 0.0f;
		pMesh->mMaterial.Emissive.a = 0.0f;
	}
}

DARKSDK_DLL void ScrollTexture ( sMesh* pMesh, int iStage, float fU, float fV )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// permission
	bool bGoAhead = false;
	DWORD dwTB = pMesh->dwFVF & GGFVF_TEXCOUNT_MASK;
	if ( iStage==0 && ( (dwTB==GGFVF_TEX1) || (dwTB==GGFVF_TEX2) || (dwTB==GGFVF_TEX3) ) ) bGoAhead = true;
	if ( iStage==1 && ( (dwTB==GGFVF_TEX2) || (dwTB==GGFVF_TEX3) ) ) bGoAhead = true;
	if ( iStage==2 && ( (dwTB==GGFVF_TEX3) ) ) bGoAhead = true;

	// make sure we have data in the vertices
	if ( bGoAhead )
	{
		// go through all of the vertices
		for ( int iCurrentVertex = 0; iCurrentVertex < (int)pMesh->dwVertexCount; iCurrentVertex++ )
		{
			*( ( float* ) pMesh->pVertexData + offsetMap.dwTU[iStage] + ( offsetMap.dwSize * iCurrentVertex ) ) += fU;
			*( ( float* ) pMesh->pVertexData + offsetMap.dwTV[iStage] + ( offsetMap.dwSize * iCurrentVertex ) ) += fV;
		}
	}

	// flag mesh for a VB update
	pMesh->bVBRefreshRequired = true;
#ifndef WICKEDENGINE
	g_vRefreshMeshList.push_back ( pMesh );
#endif
}

DARKSDK_DLL void ScrollTexture ( sMesh* pMesh, float fU, float fV )
{
	ScrollTexture ( pMesh, 0, fU, fV );
}

DARKSDK_DLL void ScaleTexture ( sMesh* pMesh, int iStage, float fU, float fV )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// permission
	bool bGoAhead = false;
	DWORD dwTB = pMesh->dwFVF & GGFVF_TEXCOUNT_MASK;
	if ( iStage==0 && ( (dwTB==GGFVF_TEX1) || (dwTB==GGFVF_TEX2) || (dwTB==GGFVF_TEX3) ) ) bGoAhead = true;
	if ( iStage==1 && ( (dwTB==GGFVF_TEX2) || (dwTB==GGFVF_TEX3) ) ) bGoAhead = true;
	if ( iStage==2 && ( (dwTB==GGFVF_TEX3) ) ) bGoAhead = true;

	// make sure we have data in the vertices
	if ( bGoAhead )
	{
		// go through all of the vertices
		for ( int iCurrentVertex = 0; iCurrentVertex < (int)pMesh->dwVertexCount; iCurrentVertex++ )
		{
			*( ( float* ) pMesh->pVertexData + offsetMap.dwTU[iStage] + ( offsetMap.dwSize * iCurrentVertex ) ) *= fU;
			*( ( float* ) pMesh->pVertexData + offsetMap.dwTV[iStage] + ( offsetMap.dwSize * iCurrentVertex ) ) *= fV;
		}
	}

	// flag mesh for a VB update
	#ifdef WICKEDENGINE
	// for now we preempt changes like this AS we create the wicked object,
	// not retro-bake the data afterwards (inefficient if not ABSOLUTELY necessary)
	#else
	pMesh->bVBRefreshRequired=true;
	g_vRefreshMeshList.push_back ( pMesh );
	#endif
}

DARKSDK_DLL void ScaleTexture ( sMesh* pMesh, float fU, float fV )
{
	// modify stage zero UV data only
	ScaleTexture ( pMesh, 0, fU, fV );
}

DARKSDK_DLL void GenerateNormals ( sMesh* pMesh, int iMode )
{
	// calculate new normals from current mesh data
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );
	if ( offsetMap.dwZ>0 && offsetMap.dwNZ>0 )
		GenerateNewNormalsForMesh ( pMesh, iMode );
}

DARKSDK_DLL void AddBoxToMesh ( sMesh* pMesh, DWORD* pdwVertexOffset, DWORD* pdwIndexOffset, float fWidth1, float fHeight1, float fDepth1, float fWidth2, float fHeight2, float fDepth2, DWORD dwColor,
					bool bL, bool bR, bool bU, bool bD, bool bB, bool bF )
{
	// vertex offset
	int iVertIndexOffset = *pdwVertexOffset;
	int iIndiceIndexOffset = *pdwIndexOffset;

	// include faces that are flagged
	if ( bF )
	{
		SetupStandardVertexDec ( pMesh, pMesh->pVertexData, iVertIndexOffset +  0, fWidth1, fHeight2, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, 0.00f, 0.00f );	// front
		SetupStandardVertexDec ( pMesh, pMesh->pVertexData, iVertIndexOffset +  1, fWidth2, fHeight2, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, 1.00f, 0.00f );
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  2, fWidth2, fHeight1, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, 1.00f, 1.00f );
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  3, fWidth1, fHeight1, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, 0.00f, 1.00f );
		pMesh->pIndices [ iIndiceIndexOffset +  0 ] = iVertIndexOffset +  0;		pMesh->pIndices [ iIndiceIndexOffset +  1 ] = iVertIndexOffset +  1;		pMesh->pIndices [ iIndiceIndexOffset +  2 ] = iVertIndexOffset +  2;
		pMesh->pIndices [ iIndiceIndexOffset +  3 ] = iVertIndexOffset +  2;		pMesh->pIndices [ iIndiceIndexOffset +  4 ] = iVertIndexOffset +  3;		pMesh->pIndices [ iIndiceIndexOffset +  5 ] = iVertIndexOffset +  0;
		iIndiceIndexOffset+=6;
		iVertIndexOffset+=4;
	}
	if ( bB )
	{
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  0, fWidth1, fHeight2, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, 1.00f, 0.00f );	// back
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  1, fWidth1, fHeight1, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, 1.00f, 1.00f );
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  2, fWidth2, fHeight1, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, 0.00f, 1.00f );
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  3, fWidth2, fHeight2, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, 0.00f, 0.00f );
		pMesh->pIndices [ iIndiceIndexOffset +  0 ] = iVertIndexOffset +  0;		pMesh->pIndices [ iIndiceIndexOffset +  1 ] = iVertIndexOffset +  1;		pMesh->pIndices [ iIndiceIndexOffset +  2 ] = iVertIndexOffset +  2;
		pMesh->pIndices [ iIndiceIndexOffset +  3 ] = iVertIndexOffset +  2;		pMesh->pIndices [ iIndiceIndexOffset +  4 ] = iVertIndexOffset +  3;		pMesh->pIndices [ iIndiceIndexOffset +  5 ] = iVertIndexOffset +  0;
		iIndiceIndexOffset+=6;
		iVertIndexOffset+=4;
	}
	if ( bD )
	{
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  0, fWidth1, fHeight2, fDepth2,	 0.0f,  1.0f,  0.0f, dwColor, 0.00f, 0.00f );	// top
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  1, fWidth2, fHeight2, fDepth2,	 0.0f,  1.0f,  0.0f, dwColor, 1.00f, 0.00f );
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  2, fWidth2, fHeight2, fDepth1,	 0.0f,  1.0f,  0.0f, dwColor, 1.00f, 1.00f );
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  3, fWidth1, fHeight2, fDepth1,	 0.0f,  1.0f,  0.0f, dwColor, 0.00f, 1.00f );
		pMesh->pIndices [ iIndiceIndexOffset +  0 ] = iVertIndexOffset +  0;		pMesh->pIndices [ iIndiceIndexOffset +  1 ] = iVertIndexOffset +  1;		pMesh->pIndices [ iIndiceIndexOffset +  2 ] = iVertIndexOffset +  2;
		pMesh->pIndices [ iIndiceIndexOffset +  3 ] = iVertIndexOffset +  2;		pMesh->pIndices [ iIndiceIndexOffset +  4 ] = iVertIndexOffset +  3;		pMesh->pIndices [ iIndiceIndexOffset +  5 ] = iVertIndexOffset +  0;
		iIndiceIndexOffset+=6;
		iVertIndexOffset+=4;
	}
	if ( bU )
	{
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  0, fWidth1, fHeight1, fDepth2,  0.0f, -1.0f,  0.0f, dwColor, 0.00f, 1.00f );	// bottom
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  1, fWidth1, fHeight1, fDepth1,	 0.0f, -1.0f,  0.0f, dwColor, 0.00f, 0.00f );
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  2, fWidth2, fHeight1, fDepth1,	 0.0f, -1.0f,  0.0f, dwColor, 1.00f, 0.00f );
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  3, fWidth2, fHeight1, fDepth2,	 0.0f, -1.0f,  0.0f, dwColor, 1.00f, 1.00f );
		pMesh->pIndices [ iIndiceIndexOffset +  0 ] = iVertIndexOffset +  0;		pMesh->pIndices [ iIndiceIndexOffset +  1 ] = iVertIndexOffset +  1;		pMesh->pIndices [ iIndiceIndexOffset +  2 ] = iVertIndexOffset +  2;
		pMesh->pIndices [ iIndiceIndexOffset +  3 ] = iVertIndexOffset +  2;		pMesh->pIndices [ iIndiceIndexOffset +  4 ] = iVertIndexOffset +  3;		pMesh->pIndices [ iIndiceIndexOffset +  5 ] = iVertIndexOffset +  0;
		iIndiceIndexOffset+=6;
		iVertIndexOffset+=4;
	}
	if ( bR )
	{
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  0, fWidth2, fHeight2, fDepth1,	 1.0f,  0.0f,  0.0f, dwColor, 0.00f, 0.00f );	// right
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  1, fWidth2, fHeight2, fDepth2,	 1.0f,  0.0f,  0.0f, dwColor, 1.00f, 0.00f );
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  2, fWidth2, fHeight1, fDepth2,	 1.0f,  0.0f,  0.0f, dwColor, 1.00f, 1.00f );
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  3, fWidth2, fHeight1, fDepth1,	 1.0f,  0.0f,  0.0f, dwColor, 0.00f, 1.00f );
		pMesh->pIndices [ iIndiceIndexOffset +  0 ] = iVertIndexOffset +  0;		pMesh->pIndices [ iIndiceIndexOffset +  1 ] = iVertIndexOffset +  1;		pMesh->pIndices [ iIndiceIndexOffset +  2 ] = iVertIndexOffset +  2;
		pMesh->pIndices [ iIndiceIndexOffset +  3 ] = iVertIndexOffset +  2;		pMesh->pIndices [ iIndiceIndexOffset +  4 ] = iVertIndexOffset +  3;		pMesh->pIndices [ iIndiceIndexOffset +  5 ] = iVertIndexOffset +  0;
		iIndiceIndexOffset+=6;
		iVertIndexOffset+=4;
	}
	if ( bL )
	{
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  0, fWidth1, fHeight2, fDepth1,	-1.0f,  0.0f,  0.0f, dwColor, 1.00f, 0.00f );	// left
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  1, fWidth1, fHeight1, fDepth1,	-1.0f,  0.0f,  0.0f, dwColor, 1.00f, 1.00f );
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  2, fWidth1, fHeight1, fDepth2,	-1.0f,  0.0f,  0.0f, dwColor, 0.00f, 1.00f );
		SetupStandardVertexDec ( pMesh,	pMesh->pVertexData, iVertIndexOffset +  3, fWidth1, fHeight2, fDepth2,	-1.0f,  0.0f,  0.0f, dwColor, 0.00f, 0.00f );
		pMesh->pIndices [ iIndiceIndexOffset +  0 ] = iVertIndexOffset +  0;		pMesh->pIndices [ iIndiceIndexOffset +  1 ] = iVertIndexOffset +  1;		pMesh->pIndices [ iIndiceIndexOffset +  2 ] = iVertIndexOffset +  2;
		pMesh->pIndices [ iIndiceIndexOffset +  3 ] = iVertIndexOffset +  2;		pMesh->pIndices [ iIndiceIndexOffset +  4 ] = iVertIndexOffset +  3;		pMesh->pIndices [ iIndiceIndexOffset +  5 ] = iVertIndexOffset +  0;
		iIndiceIndexOffset+=6;
		iVertIndexOffset+=4;
	}
	
	// update counters
	*pdwVertexOffset = iVertIndexOffset;
	*pdwIndexOffset = iIndiceIndexOffset;
}

DARKSDK_DLL void ReduceMeshPolygons ( sMesh* pOriginalMesh, int iBlockMode, int iGridDimension, int iGX, int iGY, int iGZ )
{
	// limits of this process
	if ( iGX>50 || iGY>50 || iGZ>50 )
		return;

	// minimum values
	if ( iGX<3 ) iGX=3;
	if ( iGY<1 ) iGY=1;
	if ( iGZ<3 ) iGZ=3;
	
	int iScanZ = 0;

	// the collision area is a block of wood, that we are going to chip away 
	unsigned char bBlock[50][50][50];
	for ( iScanZ=0; iScanZ<iGZ; iScanZ++ )
		for ( int iScanY=0; iScanY<iGY; iScanY++ )
			for ( int iScanX=0; iScanX<iGX; iScanX++ )
				bBlock[iScanX][iScanY][iScanZ]=255;

	// get mesh verttype and size
	sOffsetMap offsetMap;
 	GetFVFOffsetMap ( pOriginalMesh, &offsetMap );

	// create a work mesh
	sMesh* pMesh = new sMesh;
	MakeLocalMeshFromOtherLocalMesh ( pMesh, pOriginalMesh );
	ConvertLocalMeshToVertsOnly ( pMesh, false );
	DWORD dwNumberOfVertices=pMesh->dwVertexCount;

	// boundary of mesh
	CalculateMeshBounds ( pMesh );
	GGVECTOR3 vecMin = pMesh->Collision.vecMin;
	GGVECTOR3 vecMax = pMesh->Collision.vecMax;

	// create grid system around mesh bounds
	float fGapX = (vecMax.x-vecMin.x)/iGX;
	float fGapY = (vecMax.y-vecMin.y)/iGY;
	float fGapZ = (vecMax.z-vecMin.z)/iGZ;

	// gap must be AT least one for the step through to work (objects so small would have a single pass anyway)
	if ( fGapX<1.0f ) fGapX=1.0f;
	if ( fGapY<1.0f ) fGapY=1.0f;
	if ( fGapZ<1.0f ) fGapZ=1.0f;

	// grid work varriables
	float fGapThirdX = fGapX/3.0f;
	float fGapThirdY = fGapY/3.0f;
	float fGapThirdZ = fGapZ/3.0f;
	float fGapHalfX = fGapX/2.0f;
	float fGapHalfY = fGapY/2.0f;
	float fGapHalfZ = fGapZ/2.0f;

	// scan mesh and chizel from all sides
	for ( int iSide=0; iSide<2; iSide++ )
	{
		// chizel values
		int iChZ, iChAdd;
		float fChZHalf, fChZDir, fGridForDist;
		if ( iSide==0 )
		{ 
			iChZ=0; fChZHalf=-fGapHalfZ; fChZDir=1.0f; iChAdd=1; fGridForDist=fGapZ;
		}
		if ( iSide==1 )
		{ 
			iChZ=iGZ-1; fChZHalf=fGapZ+fGapHalfZ; fChZDir=-1.0f; iChAdd=-1; fGridForDist=fGapZ;
		}

		// begin chizelling from a side
		iScanZ=iChZ;
		for ( int iScanY=0; iScanY<iGY; iScanY++ )
		{
			for ( int iScanX=0; iScanX<iGX; iScanX++ )
			{
				// originate a ray
				GGVECTOR3 vecRay = GGVECTOR3 ( vecMin.x+(iScanX*fGapX)+fGapHalfX, vecMin.y+(iScanY*fGapY)+fGapHalfY, vecMin.z+(iScanZ*fGapZ)+fChZHalf );
				GGVECTOR3 vecRayDir = GGVECTOR3 ( 0, 0, fChZDir );

				// check each poly
				int iChizelDistance=iGZ-1;

				// get average of point tests on grid square
				int iDistCount=0;
				float fDistAverage=0.0f;
				for ( float fBitX=-fGapThirdX; fBitX<=fGapThirdX; fBitX+=fGapThirdX )
				{
					for ( float fBitY=-fGapThirdY; fBitY<=fGapThirdY; fBitY+=fGapThirdY )
					{
						// sub-ray for clarity
						GGVECTOR3 vecRayBit = vecRay + GGVECTOR3(fBitX,fBitY,0);

						// go through each polygon and find CLOSEST polygon hit
						bool bPolygonHitDetected=false;
						float fU, fV, fDistance, fBestCollision=99999.0f;
						for ( DWORD dwCurrentVertex = 0; dwCurrentVertex < dwNumberOfVertices; dwCurrentVertex+=3 )
						{
							// polygon
							GGVECTOR3* pVec0 = (GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * (dwCurrentVertex+0) ) );
							GGVECTOR3* pVec1 = (GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * (dwCurrentVertex+1) ) );
							GGVECTOR3* pVec2 = (GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * (dwCurrentVertex+2) ) );
							if ( GGIntersectTri ( pVec0, pVec1, pVec2, &vecRayBit, &vecRayDir, &fU, &fV, &fDistance )==TRUE )
							{
								if ( fDistance<fBestCollision)
								{
									bPolygonHitDetected=true;
									fBestCollision=fDistance;
								}
							}
						}
						if ( bPolygonHitDetected )
						{
							if ( fBestCollision > fGapHalfZ+0.1f )
							{
								fDistAverage += fBestCollision-fGapHalfZ;
								iDistCount++;
							}
							else
							{
								// no chizelling
								iChizelDistance=-1;
							}
						}
					}
				}

				// test ray against polygon
				if ( iDistCount > 0 )
				{
					// average the distance for best match
					float fDistance = (fDistAverage / iDistCount)-0.5f;

					// grid lock it to the nearest line
					int iLine = (int)(fDistance/fGridForDist);

					// somewhere in between
					if ( iLine < iChizelDistance ) 
						iChizelDistance = iLine;
				}

				// ray struck a surface, chizzel away path
				int iChizelCount=0;
				int iChizel = iScanZ;
				while ( iChizelCount <= iChizelDistance )
				{
					bBlock[iScanX][iScanY][iChizel] = 253;
					iChizelCount++;
					iChizel+=iChAdd;
				}
			}
		}
	}

	// free usages
	SAFE_DELETE ( pMesh );

	// create logical boxes from blockdata
	struct logicalboxtype
	{
		int iX1, iY1, iZ1;
		int iX2, iY2, iZ2;
		bool bLeft, bRight, bUp, bDown, bBack, bFore;
	};
	DWORD dwLogicalBoxesMax=32;
	DWORD dwLogicalBoxesCount=0;
	logicalboxtype* pLogicalBoxes = new logicalboxtype[dwLogicalBoxesMax];

	// go through all blocks
	for ( iScanZ=0; iScanZ<iGZ; iScanZ++ )
	{
		for ( int iScanY=0; iScanY<iGY; iScanY++ )
		{
			for ( int iScanX=0; iScanX<iGX; iScanX++ )
			{
				if ( bBlock[iScanX][iScanY][iScanZ]==255 )
				{
					// block exists - expand a logic-box here
					logicalboxtype sBox;
					sBox.iX1=iScanX;
					sBox.iY1=iScanY;
					sBox.iZ1=iScanZ;
					sBox.iX2=iScanX;
					sBox.iY2=iScanY;
					sBox.iZ2=iScanZ;

					// expand in all areas where blocks permit
					bool bExpand = true;
					while ( bExpand )
					{
						// count expansions
						int iExpandCount=0;

						// try left expand
						int iX=sBox.iX1-1;
						bool bCanExpand=true;
						if ( iX==-1 ) bCanExpand=false;
						if ( bCanExpand )
							for ( int iY=sBox.iY1; iY<=sBox.iY2; iY++ )
								for ( int iZ=sBox.iZ1; iZ<=sBox.iZ2; iZ++ )
									if ( bBlock[iX][iY][iZ]<255 )
										bCanExpand=false;
						if ( bCanExpand ) { sBox.iX1=iX; iExpandCount++; }

						// try right expand
						iX=sBox.iX2+1;
						bCanExpand=true;
						if ( iX==iGX ) bCanExpand=false;
						if ( bCanExpand )
							for ( int iY=sBox.iY1; iY<=sBox.iY2; iY++ )
								for ( int iZ=sBox.iZ1; iZ<=sBox.iZ2; iZ++ )
									if ( bBlock[iX][iY][iZ]<255 )
										bCanExpand=false;
						if ( bCanExpand ) { sBox.iX2=iX; iExpandCount++; }

						// try up expand
						int iY=sBox.iY1-1;
						bCanExpand=true;
						if ( iY==-1 ) bCanExpand=false;
						if ( bCanExpand )
							for ( int iX=sBox.iX1; iX<=sBox.iX2; iX++ )
								for ( int iZ=sBox.iZ1; iZ<=sBox.iZ2; iZ++ )
									if ( bBlock[iX][iY][iZ]<255 )
										bCanExpand=false;
						if ( bCanExpand ) { sBox.iY1=iY; iExpandCount++; }

						// try down expand
						iY=sBox.iY2+1;
						bCanExpand=true;
						if ( iY==iGY ) bCanExpand=false;
						if ( bCanExpand )
							for ( int iX=sBox.iX1; iX<=sBox.iX2; iX++ )
								for ( int iZ=sBox.iZ1; iZ<=sBox.iZ2; iZ++ )
									if ( bBlock[iX][iY][iZ]<255 )
										bCanExpand=false;
						if ( bCanExpand ) { sBox.iY2=iY; iExpandCount++; }

						// try back expand
						int iZ=sBox.iZ1-1;
						bCanExpand=true;
						if ( iZ==-1 ) bCanExpand=false;
						if ( bCanExpand )
							for ( int iX=sBox.iX1; iX<=sBox.iX2; iX++ )
								for ( int iY=sBox.iY1; iY<=sBox.iY2; iY++ )
									if ( bBlock[iX][iY][iZ]<255 )
										bCanExpand=false;
						if ( bCanExpand ) { sBox.iZ1=iZ; iExpandCount++; }

						// try fore expand
						iZ=sBox.iZ2+1;
						bCanExpand=true;
						if ( iZ==iGZ ) bCanExpand=false;
						if ( bCanExpand )
							for ( int iX=sBox.iX1; iX<=sBox.iX2; iX++ )
								for ( int iY=sBox.iY1; iY<=sBox.iY2; iY++ )
									if ( bBlock[iX][iY][iZ]<255 )
										bCanExpand=false;
						if ( bCanExpand ) { sBox.iZ2=iZ; iExpandCount++; }

						// if cannot expand anywhere, exit
						if ( iExpandCount==0 ) bExpand=false;
					}
					
					int iX = 0;

					// once expanded as far as we can, delete all blocks within it
					for ( iX=sBox.iX1; iX<=sBox.iX2; iX++ )
						for ( int iY=sBox.iY1; iY<=sBox.iY2; iY++ )
							for ( int iZ=sBox.iZ1; iZ<=sBox.iZ2; iZ++ )
								bBlock[iX][iY][iZ] = 254;

					// see if logicbox sides completely hidden by more blocks (for HSR)
					iX=sBox.iX1-1;
					sBox.bLeft=false;
					if ( iX==-1 ) sBox.bLeft=true;
					if ( !sBox.bLeft )
						for ( int iY=sBox.iY1; iY<=sBox.iY2; iY++ )
							for ( int iZ=sBox.iZ1; iZ<=sBox.iZ2; iZ++ )
								if ( bBlock[iX][iY][iZ]==253 )
									sBox.bLeft=true;

					// check right
					iX=sBox.iX2+1;
					sBox.bRight=false;
					if ( iX==iGX ) sBox.bRight=true;
					if ( !sBox.bRight )
						for ( int iY=sBox.iY1; iY<=sBox.iY2; iY++ )
							for ( int iZ=sBox.iZ1; iZ<=sBox.iZ2; iZ++ )
								if ( bBlock[iX][iY][iZ]==253 )
									sBox.bRight=true;

					// check up
					int iY=sBox.iY1-1;
					sBox.bUp=false;
					if ( iY==-1 ) sBox.bUp=true;
					if ( !sBox.bUp )
						for ( iX=sBox.iX1; iX<=sBox.iX2; iX++ )
							for ( int iZ=sBox.iZ1; iZ<=sBox.iZ2; iZ++ )
								if ( bBlock[iX][iY][iZ]==253 )
									sBox.bUp=true;

					// check down
					iY=sBox.iY2+1;
					sBox.bDown=false;
					if ( iY==iGY ) sBox.bDown=true;
					if ( !sBox.bDown )
						for ( iX=sBox.iX1; iX<=sBox.iX2; iX++ )
							for ( int iZ=sBox.iZ1; iZ<=sBox.iZ2; iZ++ )
								if ( bBlock[iX][iY][iZ]==253 )
									sBox.bDown=true;

					// check back
					int iZ=sBox.iZ1-1;
					sBox.bBack=false;
					if ( iZ==-1 ) sBox.bBack=true;
					if ( !sBox.bBack )
						for ( iX=sBox.iX1; iX<=sBox.iX2; iX++ )
							for ( int iY=sBox.iY1; iY<=sBox.iY2; iY++ )
								if ( bBlock[iX][iY][iZ]==253 )
									sBox.bBack=true;

					// check fore
					iZ=sBox.iZ2+1;
					sBox.bFore=false;
					if ( iZ==iGZ ) sBox.bFore=true;
					if ( !sBox.bFore )
						for ( iX=sBox.iX1; iX<=sBox.iX2; iX++ )
							for ( int iY=sBox.iY1; iY<=sBox.iY2; iY++ )
								if ( bBlock[iX][iY][iZ]==253 )
									sBox.bFore=true;

					// expand list if too small
					if ( dwLogicalBoxesCount+1>=dwLogicalBoxesMax )
					{
						DWORD dwNewBoxMax = dwLogicalBoxesMax*2;
						logicalboxtype* pNewLogicalBoxes = new logicalboxtype[dwNewBoxMax];
						memcpy ( pNewLogicalBoxes, pLogicalBoxes, dwLogicalBoxesMax * sizeof(logicalboxtype) );
						SAFE_DELETE(pLogicalBoxes);
						pLogicalBoxes=pNewLogicalBoxes;
						dwLogicalBoxesMax=dwNewBoxMax;
					}

					// and add to logic box array
					pLogicalBoxes [ dwLogicalBoxesCount ] = sBox;
					dwLogicalBoxesCount++;
				}
			}
		}
	}

	// work out actual usage of new boxes
	DWORD dwVertexCount=0, dwIndexCount=0;
	for ( DWORD dwCurrentBox=0; dwCurrentBox<dwLogicalBoxesCount; dwCurrentBox++ )
	{
		// Box dimensions
		bool bL = pLogicalBoxes [ dwCurrentBox ].bLeft;
		bool bR = pLogicalBoxes [ dwCurrentBox ].bRight;
		bool bU = pLogicalBoxes [ dwCurrentBox ].bUp;
		bool bD = pLogicalBoxes [ dwCurrentBox ].bDown;
		bool bB = pLogicalBoxes [ dwCurrentBox ].bBack;
		bool bF = pLogicalBoxes [ dwCurrentBox ].bFore;
		if ( bL ) { dwVertexCount+=4; dwIndexCount+=6; }
		if ( bR ) { dwVertexCount+=4; dwIndexCount+=6; }
		if ( bU ) { dwVertexCount+=4; dwIndexCount+=6; }
		if ( bD ) { dwVertexCount+=4; dwIndexCount+=6; }
		if ( bB ) { dwVertexCount+=4; dwIndexCount+=6; }
		if ( bF ) { dwVertexCount+=4; dwIndexCount+=6; }
	}

	// create new mesh
	DWORD dwVertexOffset=0, dwIndexOffset=0;
	if ( SetupMeshData ( pOriginalMesh, dwVertexCount, dwIndexCount, false ) )
	{
		// default values
		DWORD dwColor = GGCOLOR(1,1,1,1);

		// fill new mesh with new meshdata
		for ( DWORD dwCurrentBox=0; dwCurrentBox<dwLogicalBoxesCount; dwCurrentBox++ )
		{
			// Box dimensions
			int iScanX = pLogicalBoxes [ dwCurrentBox ].iX1;
			int iScanY = pLogicalBoxes [ dwCurrentBox ].iY1;
			int iScanZ = pLogicalBoxes [ dwCurrentBox ].iZ1;
			int iScanWX = 1+(pLogicalBoxes [ dwCurrentBox ].iX2-iScanX);
			int iScanWY = 1+(pLogicalBoxes [ dwCurrentBox ].iY2-iScanY);
			int iScanWZ = 1+(pLogicalBoxes [ dwCurrentBox ].iZ2-iScanZ);
			bool bL = pLogicalBoxes [ dwCurrentBox ].bLeft;
			bool bR = pLogicalBoxes [ dwCurrentBox ].bRight;
			bool bU = pLogicalBoxes [ dwCurrentBox ].bUp;
			bool bD = pLogicalBoxes [ dwCurrentBox ].bDown;
			bool bB = pLogicalBoxes [ dwCurrentBox ].bBack;
			bool bF = pLogicalBoxes [ dwCurrentBox ].bFore;

			// calculate box size
			GGVECTOR3 vecBox = GGVECTOR3 ( vecMin.x+(iScanX*fGapX), vecMin.y+(iScanY*fGapY), vecMin.z+(iScanZ*fGapZ) );
			float fWidth1 = vecBox.x;
			float fHeight1 = vecBox.y;
			float fDepth1 = vecBox.z;
			float fWidth2 = fWidth1+(fGapX*iScanWX);
			float fHeight2 = fHeight1+(fGapY*iScanWY);
			float fDepth2 = fDepth1+(fGapZ*iScanWZ);
			
			// create box
			AddBoxToMesh ( pOriginalMesh, &dwVertexOffset, &dwIndexOffset, fWidth1, fHeight1, fDepth1, fWidth2, fHeight2, fDepth2, dwColor, bL, bR, bU, bD, bB, bF );
		}

		// setup mesh values
		pOriginalMesh->iPrimitiveType   = GGPT_TRIANGLELIST;
		pOriginalMesh->iDrawVertexCount = pOriginalMesh->dwVertexCount;
		pOriginalMesh->iDrawPrimitives  = pOriginalMesh->dwIndexCount  / 3;
	}

	// free usages
	SAFE_DELETE(pLogicalBoxes);

	// flag mesh for a VB update
	pOriginalMesh->bMeshHasBeenReplaced=true;
}

DARKSDK_DLL int CheckIfMeshSolid ( sMesh* pMesh, int iGX, int iGY, int iGZ )
{
	// limits of this process
	if ( iGX>50 || iGY>50 || iGZ>50 )
		return false;

	// minimum values
	if ( iGX<3 ) iGX=3;
	if ( iGY<3 ) iGY=3;
	if ( iGZ<3 ) iGZ=3;

	// cound cast hits and overall
	int iCountCastHits=0;
	int iOverallCasts=0;

	// get mesh verttype and size
	sOffsetMap offsetMap;
 	GetFVFOffsetMap ( pMesh, &offsetMap );

	// boundary of mesh
	GGVECTOR3 vecMin = pMesh->Collision.vecMin;
	GGVECTOR3 vecMax = pMesh->Collision.vecMax;

	// determine best side to scan from
	GGVECTOR3 vecSize = vecMax - vecMin;
	int iSideScan=0;
	float fSmallest=vecSize.x;
	if ( vecSize.y<fSmallest ) { fSmallest=vecSize.y; iSideScan=1; }
	if ( vecSize.z<fSmallest ) { fSmallest=vecSize.z; iSideScan=2; }

	// create grid system around mesh bounds
	float fGapX = (vecSize.x)/iGX;
	float fGapY = (vecSize.y)/iGY;
	float fGapZ = (vecSize.z)/iGZ;
	float fGapEdgeX = fGapX;
	float fGapEdgeY = fGapY;
	float fGapEdgeZ = fGapZ;

	// setup ray direction and plane
	GGVECTOR3 vecRayDir;
	if ( iSideScan==0 ) { vecRayDir = GGVECTOR3 ( 1.0f, 0, 0 ); fGapX=vecSize.x*1.01f; fGapEdgeX=0.0f; } 
	if ( iSideScan==1 ) { vecRayDir = GGVECTOR3 ( 0, 1.0f, 0 ); fGapY=vecSize.y*1.01f; fGapEdgeY=0.0f; } 
	if ( iSideScan==2 ) { vecRayDir = GGVECTOR3 ( 0, 0, 1.0f ); fGapZ=vecSize.z*1.01f; fGapEdgeZ=0.0f; } 

	// gap must be AT least one for the step through to work (objects so small would have a single pass anyway)
	if ( fGapX<1.0f ) fGapX=1.0f;
	if ( fGapY<1.0f ) fGapY=1.0f;
	if ( fGapZ<1.0f ) fGapZ=1.0f;

	// scan a grid
	for ( float fX=vecMin.x+fGapEdgeX; fX<=vecMax.x-fGapEdgeX; fX+=fGapX )
	{
		for ( float fY=vecMin.y+fGapEdgeY; fY<=vecMax.y-fGapEdgeY; fY+=fGapY )
		{
			for ( float fZ=vecMin.z+fGapEdgeZ; fZ<=vecMax.z-fGapEdgeZ; fZ+=fGapZ )
			{
				// originate a ray
				float fRayDist=0.0f;
				GGVECTOR3 vecRayStart;
				if ( iSideScan==0 ) { vecRayStart = GGVECTOR3 ( vecMin.x+(float)fabs(vecSize.x*2)*-1.0f, fY, fZ ); fRayDist=(float)fabs(vecMin.x-vecRayStart.x); }
				if ( iSideScan==1 ) { vecRayStart = GGVECTOR3 ( fX, vecMin.y+(float)fabs(vecSize.y*2)*-1.0f, fZ ); fRayDist=(float)fabs(vecMin.y-vecRayStart.y); }
				if ( iSideScan==2 ) { vecRayStart = GGVECTOR3 ( fX, fY, vecMin.z+(float)fabs(vecSize.z*2)*-1.0f ); fRayDist=(float)fabs(vecMin.z-vecRayStart.z); }

				// go through each polygon and find CLOSEST polygon hit
				bool bPolygonHitDetected=false;
				float fU, fV, fDistance, fBestCollision=99999.0f;

				// with indexed data
				if ( pMesh->pIndices )
				{
					DWORD dwNumberOfIndices = pMesh->dwIndexCount;
					for ( DWORD dwCurrentIndex = 0; dwCurrentIndex < dwNumberOfIndices; dwCurrentIndex+=3 )
					{
						// use indice data to find correct verts for poly
						DWORD dwV1 = pMesh->pIndices [ dwCurrentIndex+0 ];
						DWORD dwV2 = pMesh->pIndices [ dwCurrentIndex+1 ];
						DWORD dwV3 = pMesh->pIndices [ dwCurrentIndex+2 ];

						// polygon
						GGVECTOR3* pVec0 = (GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * dwV1 ) );
						GGVECTOR3* pVec1 = (GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * dwV2 ) );
						GGVECTOR3* pVec2 = (GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * dwV3 ) );
						if ( GGIntersectTri ( pVec0, pVec1, pVec2, &vecRayStart, &vecRayDir, &fU, &fV, &fDistance )==TRUE )
						{
							// I hit one, make sire it is right on the edge of the boundbox (or else it has a gap as though from a curve that we can see through)
							if ( fDistance<fRayDist+12.5f )
							{
								// can leave now
								bPolygonHitDetected=true;
								break;
							}
						}
					}
				}
				else
				{
					// pure vertex data
					DWORD dwNumberOfVertices = pMesh->dwVertexCount;
					for ( DWORD dwCurrentVertex = 0; dwCurrentVertex < dwNumberOfVertices; dwCurrentVertex+=3 )
					{
						// polygon
						GGVECTOR3* pVec0 = (GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * (dwCurrentVertex+0) ) );
						GGVECTOR3* pVec1 = (GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * (dwCurrentVertex+1) ) );
						GGVECTOR3* pVec2 = (GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * (dwCurrentVertex+2) ) );
						if ( GGIntersectTri ( pVec0, pVec1, pVec2, &vecRayStart, &vecRayDir, &fU, &fV, &fDistance )==TRUE )
						{
							// I hit one, make sire it is right on the edge of the boundbox (or else it has a gap as though from a curve that we can see through)
							if ( fDistance<fRayDist+12.5f )
							{
								// can leave now
								bPolygonHitDetected=true;
								break;
							}
						}
					}
				}
				if ( bPolygonHitDetected )
				{
					// hole found, can leave now
					iCountCastHits++;
				}
				iOverallCasts++;
			}
		}
	}

	// check if solid, partially solid or not even there
	int iMarginForError = iOverallCasts/20;
	if ( iCountCastHits>iOverallCasts-iMarginForError )
	{
		// entirely solid (no holes - solid wall)
		return 2;
	}
	else
	{
		if ( iCountCastHits>0 )
		{
			// partially solid (has hole in it)
			return 1;
		}
		else
		{
			// not solid
			return 0;
		}
	}
}

DARKSDK_DLL bool CheckIfMeshBlocking ( sMesh* pMesh, float X1, float Y1, float Z1, float X2, float Y2, float Z2 )
{
	// calculate bounds of mesh
	float boundboxX1 = pMesh->Collision.vecMin.x;
	float boundboxY1 = pMesh->Collision.vecMin.y;
	float boundboxZ1 = pMesh->Collision.vecMin.z;
	float boundboxX2 = pMesh->Collision.vecMax.x;
	float boundboxY2 = pMesh->Collision.vecMax.y;
	float boundboxZ2 = pMesh->Collision.vecMax.z;

	// ensure mesh encountering plane being detected
	if ( X1 >= boundboxX1 && X2 <= boundboxX2 )
	{
        if ( Y1 >= boundboxY1 && Y2 <= boundboxY2 )
		{
           if ( Z1 >= boundboxZ1 && Z2 <= boundboxZ2 )
		   {
				if ( CheckIfMeshSolid ( pMesh, 10, 10, 10 )>0 )
				{
					// blocking
					return true;
				}
		   }
		}
	}
	
	// not blocking
	return false;
}


// Mesh Vertex Data Functions

DARKSDK_DLL void SetPositionData ( sMesh* pMesh, int iCurrentVertex, float fX, float fY, float fZ )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// make sure we have data in the vertices
	if ( offsetMap.dwZ>0 )
	{
		// set component
		*( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * iCurrentVertex ) ) = fX;
		*( ( float* ) pMesh->pVertexData + offsetMap.dwY + ( offsetMap.dwSize * iCurrentVertex ) ) = fY;
		*( ( float* ) pMesh->pVertexData + offsetMap.dwZ + ( offsetMap.dwSize * iCurrentVertex ) ) = fZ;
	}

	// flag mesh for a VB update
	pMesh->bVBRefreshRequired=true;
#ifndef WICKEDENGINE
	g_vRefreshMeshList.push_back ( pMesh );
#endif
}

DARKSDK_DLL void SetNormalsData ( sMesh* pMesh, int iCurrentVertex, float fNX, float fNY, float fNZ )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// make sure we have data in the vertices
	if ( pMesh->dwFVF & GGFVF_NORMAL )
	{
		// set normals vector component
		*( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * iCurrentVertex ) ) = fNX;
		*( ( float* ) pMesh->pVertexData + offsetMap.dwNY + ( offsetMap.dwSize * iCurrentVertex ) ) = fNY;
		*( ( float* ) pMesh->pVertexData + offsetMap.dwNZ + ( offsetMap.dwSize * iCurrentVertex ) ) = fNZ;
	}

	// flag mesh for a VB update
	pMesh->bVBRefreshRequired=true;
#ifndef WICKEDENGINE
	g_vRefreshMeshList.push_back ( pMesh );
#endif
}

DARKSDK_DLL void SetDiffuseData ( sMesh* pMesh, int iCurrentVertex, DWORD dwDiffuse )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// make sure we have data in the vertices
	if ( pMesh->dwFVF & GGFVF_DIFFUSE )
	{
		// set component
		*( ( DWORD* ) pMesh->pVertexData + offsetMap.dwDiffuse + ( offsetMap.dwSize * iCurrentVertex ) ) = dwDiffuse;
	}

	// flag mesh for a VB update
	pMesh->bVBRefreshRequired=true;
#ifndef WICKEDENGINE
	g_vRefreshMeshList.push_back ( pMesh );
#endif
}

DARKSDK_DLL void SetUVData ( sMesh* pMesh, int iCurrentVertex, float fU, float fV )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// make sure we have data in the vertices
	if ( pMesh->dwFVF & GGFVF_TEX1 )
	{
		// set single UV vertex component
		*( ( float* ) pMesh->pVertexData + offsetMap.dwTU[0] + ( offsetMap.dwSize * iCurrentVertex ) ) = fU;
		*( ( float* ) pMesh->pVertexData + offsetMap.dwTV[0] + ( offsetMap.dwSize * iCurrentVertex ) ) = fV;
	}

	// flag mesh for a VB update
	pMesh->bVBRefreshRequired=true;
#ifndef WICKEDENGINE
	g_vRefreshMeshList.push_back ( pMesh );
#endif
}

DARKSDK_DLL bool AddMeshToData ( sMesh* pFinalMesh, sMesh* pMeshToAdd )
{
	// ensure meshes valid
	SAFE_MEMORY ( pFinalMesh );
	SAFE_MEMORY ( pMeshToAdd );

	// convert addmesh to standard
	sMesh* pOriginalMesh = new sMesh;
	MakeLocalMeshFromOtherLocalMesh ( pOriginalMesh, pFinalMesh );

	// convert addmesh to standard
	sMesh* pStandardMesh = new sMesh;
	MakeLocalMeshFromOtherLocalMesh ( pStandardMesh, pMeshToAdd );
	ConvertLocalMeshToFVF ( pStandardMesh, pOriginalMesh->dwFVF );
	
	// make vertex and index buffers for final
	DWORD dwTotalVertices = pOriginalMesh->dwVertexCount + pStandardMesh->dwVertexCount;
	DWORD dwTotalIndices = pOriginalMesh->dwIndexCount + pStandardMesh->dwIndexCount;

	// if creation successful, continue
	if ( SetupMeshFVFData ( pFinalMesh, pOriginalMesh->dwFVF, dwTotalVertices, dwTotalIndices, false ) )
	{
		// copy over original to final
		memcpy ( pFinalMesh->pVertexData, pOriginalMesh->pVertexData, pOriginalMesh->dwVertexCount * pOriginalMesh->dwFVFSize );
		memcpy ( pFinalMesh->pIndices, pOriginalMesh->pIndices, pOriginalMesh->dwIndexCount * sizeof(WORD) );

		// new vertex data in index list (word=65535max)
		WORD dwVertexStart = (WORD)pOriginalMesh->dwVertexCount;

		// copy over standard to final
		BYTE* pDestVertexData = (BYTE*)pFinalMesh->pVertexData + ( dwVertexStart * pOriginalMesh->dwFVFSize );
		BYTE* pDestIndexData = (BYTE*)pFinalMesh->pIndices + ( pOriginalMesh->dwIndexCount * sizeof(WORD) );
		memcpy ( pDestVertexData, pStandardMesh->pVertexData, pStandardMesh->dwVertexCount * pStandardMesh->dwFVFSize );
		memcpy ( pDestIndexData, pStandardMesh->pIndices, pStandardMesh->dwIndexCount * sizeof(WORD) );

		// increment index values to allign to vertex entries
		WORD* pIndexArray = (WORD*)pDestIndexData;
		for ( DWORD i=0; i<pStandardMesh->dwIndexCount; i++ )
			pIndexArray[i] += dwVertexStart;

		// update values of the mesh
		pFinalMesh->iDrawVertexCount = dwTotalVertices;
		pFinalMesh->iDrawPrimitives  = dwTotalIndices/3;

		// flag mesh for a VB replacement
		pFinalMesh->bMeshHasBeenReplaced=true;
	}

	// free usages
	SAFE_DELETE(pOriginalMesh);
	SAFE_DELETE(pStandardMesh);

	// success
	return true;
}

DARKSDK_DLL bool DeleteMeshFromData ( sMesh* pMesh, int iVertex1, int iVertex2, int iIndex1, int iIndex2 )
{
	// ensure mesh valid
	SAFE_MEMORY ( pMesh );

	// check ranges
  	if ( iVertex2<iVertex1 ) return false;
	if ( iIndex2<iIndex1 ) return false;
	if ( iVertex2<0 ) return false;
	if ( iIndex2<0 ) return false;
	if ( iVertex2>(int)pMesh->dwVertexCount ) return false;
	if ( iIndex2>(int)pMesh->dwIndexCount ) return false;

	// calculate new vertex and index arrays
	DWORD dwNewVertexCount = pMesh->dwVertexCount - (iVertex2-iVertex1);
	DWORD dwNewIndexCount = pMesh->dwIndexCount - (iIndex2-iIndex1);

	// create new arrays
	BYTE* pNewVertexData = new BYTE [ dwNewVertexCount * pMesh->dwFVFSize ];
	BYTE* pNewIndexData = new BYTE [ dwNewIndexCount * sizeof(WORD) ];

	// copy 'before' data
	DWORD dwVBeforeCount = iVertex1;
	DWORD dwIBeforeCount = iIndex1;
	if ( dwVBeforeCount > 0 ) memcpy ( pNewVertexData, pMesh->pVertexData, dwVBeforeCount * pMesh->dwFVFSize );
	if ( dwIBeforeCount > 0 ) memcpy ( pNewIndexData, pMesh->pIndices, dwIBeforeCount * sizeof(WORD) );

	// copy 'after' data
	DWORD dwVAfterCount = pMesh->dwVertexCount - iVertex2;
	DWORD dwIAfterCount = pMesh->dwIndexCount - iIndex2;
	if ( dwVAfterCount > 0 )
	{
		BYTE* pVertStart = (BYTE*)pNewVertexData + ( dwVBeforeCount * pMesh->dwFVFSize );
		BYTE* pVertEnd = (BYTE*)pMesh->pVertexData + ( iVertex2 * pMesh->dwFVFSize );
		memcpy ( pVertStart, pVertEnd, dwVAfterCount * pMesh->dwFVFSize );
	}
	if ( dwIAfterCount > 0 )
	{
		BYTE* pIndexStart = (BYTE*)pNewIndexData + ( dwIBeforeCount * sizeof(WORD) );
		BYTE* pIndexEnd = (BYTE*)pMesh->pIndices + ( iIndex2 * sizeof(WORD) );
		memcpy ( pIndexStart, pIndexEnd, dwIAfterCount * sizeof(WORD) );
	}

	// reduce 'indice' data after vertex data shuffle
	WORD wVGap = iVertex2 - iVertex1;
	WORD* pThisIndexData = (WORD*)pNewIndexData;
	for ( DWORD i=0; i<dwNewIndexCount; i++)
		if(pThisIndexData[i]>=iVertex2)
			pThisIndexData[i]=pThisIndexData[i]-wVGap;

	// remove old arraus
	SAFE_DELETE_ARRAY(pMesh->pVertexData);
	SAFE_DELETE_ARRAY(pMesh->pIndices);

	// replace with new arrays
	pMesh->dwVertexCount = dwNewVertexCount;
	pMesh->pVertexData = pNewVertexData;
	pMesh->dwIndexCount = dwNewIndexCount;
	pMesh->pIndices = (WORD*)pNewIndexData;
	pMesh->iDrawVertexCount = dwNewVertexCount;
	pMesh->iDrawPrimitives = dwNewIndexCount/3;

	// flag mesh for a VB replacement
	pMesh->bMeshHasBeenReplaced=true;

	// success
	return true;
}

DARKSDK_DLL int GetVertexCount ( sMesh* pMesh )
{
	return pMesh->dwVertexCount;
}

DARKSDK_DLL int GetIndexCount ( sMesh* pMesh )
{
	return pMesh->dwIndexCount;
}

DARKSDK_DLL float GetDataPositionX ( sMesh* pMesh, int iCurrentVertex )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );
	if ( offsetMap.dwZ>0 )
		return *( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * iCurrentVertex ) );

	return 0.0f;
}

DARKSDK_DLL float GetDataPositionY ( sMesh* pMesh, int iCurrentVertex )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );
	if ( offsetMap.dwZ>0 )
		return *( ( float* ) pMesh->pVertexData + offsetMap.dwY + ( offsetMap.dwSize * iCurrentVertex ) );

	return 0.0f;
}

DARKSDK_DLL float GetDataPositionZ ( sMesh* pMesh, int iCurrentVertex )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );
	if ( offsetMap.dwZ>0 )
		return *( ( float* ) pMesh->pVertexData + offsetMap.dwZ + ( offsetMap.dwSize * iCurrentVertex ) );

	return 0.0f;
}

DARKSDK_DLL float GetDataNormalsX ( sMesh* pMesh, int iCurrentVertex )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );
	if ( offsetMap.dwNZ>0 )
		return *( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * iCurrentVertex ) );

	return 0.0f;
}

DARKSDK_DLL float GetDataNormalsY ( sMesh* pMesh, int iCurrentVertex )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );
	if ( offsetMap.dwNZ>0 )
		return *( ( float* ) pMesh->pVertexData + offsetMap.dwNY + ( offsetMap.dwSize * iCurrentVertex ) );

	return 0.0f;
}

DARKSDK_DLL float GetDataNormalsZ ( sMesh* pMesh, int iCurrentVertex )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );
	if ( offsetMap.dwNZ>0 )
		return *( ( float* ) pMesh->pVertexData + offsetMap.dwNZ + ( offsetMap.dwSize * iCurrentVertex ) );

	return 0.0f;
}

DARKSDK_DLL DWORD GetDataDiffuse ( sMesh* pMesh, int iCurrentVertex )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );
	if ( offsetMap.dwDiffuse>0 )
		return *( ( DWORD* ) pMesh->pVertexData + offsetMap.dwDiffuse + ( offsetMap.dwSize * iCurrentVertex ) );

	return 0;
}

DARKSDK_DLL float GetDataU ( sMesh* pMesh, int iCurrentVertex )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );
	if ( offsetMap.dwTU[0]>0 )
		return *( ( float* ) pMesh->pVertexData + offsetMap.dwTU[0] + ( offsetMap.dwSize * iCurrentVertex ) );

	return 0.0f;
}

DARKSDK_DLL float GetDataV ( sMesh* pMesh, int iCurrentVertex )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );
	if ( offsetMap.dwTV[0]>0 )
		return *( ( float* ) pMesh->pVertexData + offsetMap.dwTV[0] + ( offsetMap.dwSize * iCurrentVertex ) );

	return 0.0f;
}

DARKSDK_DLL float GetDataU ( sMesh* pMesh, int iCurrentVertex, int iIndex )
{
	// mike - 050803 - new command to get another set of texture coords

	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );
	if ( offsetMap.dwTU[iIndex]>0 )
		return *( ( float* ) pMesh->pVertexData + offsetMap.dwTU[iIndex] + ( offsetMap.dwSize * iCurrentVertex ) );

	return 0.0f;
}

DARKSDK_DLL float GetDataV ( sMesh* pMesh, int iCurrentVertex, int iIndex )
{
	// mike - 050803 - new command to get another set of texture coords

	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );
	if ( offsetMap.dwTV[iIndex]>0 )
		return *( ( float* ) pMesh->pVertexData + offsetMap.dwTV[iIndex] + ( offsetMap.dwSize * iCurrentVertex ) );

	return 0.0f;
}

// Mesh Visual Functions

DARKSDK_DLL void Hide ( sMesh* pMesh )
{
	pMesh->bVisible = false;
}

DARKSDK_DLL void Show ( sMesh* pMesh )
{
	pMesh->bVisible = true;
}

/*
DARKSDK_DLL void SetGhost ( sMesh* pMesh, bool bGhost, int iGhostMode )
{
	// not used any more
	//pMesh->bGhost = bGhost;
	//if ( iGhostMode != -1 ) pMesh->iGhostMode = iGhostMode;
}
*/

DARKSDK_DLL void SetTextureMode ( sMesh* pMesh, int iStage, int iMode, int iMipMode )
{
	if ( (DWORD)iStage<pMesh->dwTextureCount )
	{
		sTexture* pTexture = &pMesh->pTextures[iStage];
		if(pTexture)
		{
			pTexture->dwAddressU = iMode;
			pTexture->dwAddressV = iMode;
			pTexture->dwMipState = iMipMode;
		}
	}
}

DARKSDK_DLL void SetTextureMode ( sMesh* pMesh, int iMode, int iMipMode )
{
	SetTextureMode ( pMesh, 0, iMode, iMipMode );
}

// Mesh Texture Functions

DARKSDK_DLL int LoadOrFindTextureAsImage ( LPSTR pTextureName, LPSTR TexturePath, int iDivideTextureSize )
{
	// load texture
	int iImageIndex = 0;

	// does we have a texture name
	if ( strlen(pTextureName)==0 )
		return 0;

	// get filename only
	char pNoPath [ _MAX_PATH ];
	strcpy(pNoPath, pTextureName );
	_strrev(pNoPath);
	for(DWORD n=0; n<strlen(pNoPath); n++)
		if(pNoPath[n]=='\\' || pNoPath[n]=='/' || (unsigned char)(pNoPath[n])<32)
			pNoPath[n]=0;
	_strrev(pNoPath);

	// build a texture path and load it
	char Path [ _MAX_PATH*2 ];
	sprintf ( Path, "%s%s", TexturePath, pNoPath );
	if ( strlen ( Path ) >= _MAX_PATH ) Path[_MAX_PATH]=0;

	//PE:-MEM: Double load images sample "entitybank\\\\CityscapePBR\\Palm Tree_ao.dds"
	//PE:-MEM: Origin , LoadObject -> LoadCore -> LoadInternalTextures . (FindInternalImage)
	// texture load a : default file
	iImageIndex = LoadImageInternalEx ( Path, iDivideTextureSize );
	if ( iImageIndex==0 )
	{
		// texture load b : file as DDS
		char pDDSFile [ _MAX_PATH*2 ];
		strcpy(pDDSFile, pNoPath);
		DWORD dwLenDot = strlen(pDDSFile);
		if ( dwLenDot>4 )
		{
			pDDSFile[dwLenDot-4]=0;
			strcat(pDDSFile, ".dds");
			sprintf ( Path, "%s%s", TexturePath, pDDSFile );
			iImageIndex = LoadImageInternalEx ( Path, iDivideTextureSize );
			if ( iImageIndex==0 )
			{
				// texture load b2 : try as _D.dds
				pDDSFile[dwLenDot-4]=0;
				strcat(pDDSFile, "_D.dds");
				sprintf ( Path, "%s%s", TexturePath, pDDSFile );
				iImageIndex = LoadImageInternalEx ( Path, iDivideTextureSize );
				if ( iImageIndex==0 )
				{
					// lee - 231017 - prevent ABSOLUTE path to ORIGINAL texture being used by standalones
					// (check local folder and fileonly before rest)

					//PE: This generates doubble load. moved as last check.
					//if (strstr(TexturePath, "lightmaps\\") != NULL) {
					//sprintf(Path, "%s", pNoPath);
					//iImageIndex = LoadImageInternalEx(Path, iDivideTextureSize);
					//}
					//if ( iImageIndex==0 )
					//{

					//PE: This combi works in standalone/test , so prefer this as it can be reused.
					// texture load e : 031208 - U71 - absolute path with relative path in model file
					sprintf(Path, "%s%s", TexturePath, pTextureName);
					if (strlen(Path) >= _MAX_PATH) Path[_MAX_PATH] = 0;
					iImageIndex = LoadImageInternalEx(Path, iDivideTextureSize);
					if (iImageIndex == 0)
					{
						// okay, check if texture file alongside model as DDS)
						Path[strlen(Path) - 4] = 0;
						strcat(Path, ".dds");
						iImageIndex = LoadImageInternalEx(Path, iDivideTextureSize);
						if (iImageIndex == 0)
						{
							// texture load c : original file
							iImageIndex = LoadImageInternalEx(pTextureName, iDivideTextureSize);
							if (iImageIndex == 0)
							{
								// texture load c2 : try as dds
								//PE: Create a doubble load in standalone , try .dds with relative path instead. ( can be reused ).
								//strcpy ( Path, pTextureName );
								sprintf(Path, "%s%s", TexturePath, pTextureName);
								if (strlen(Path) >= _MAX_PATH) Path[_MAX_PATH] = 0;
								Path[strlen(Path) - 4] = 0;
								strcat(Path, ".dds");
								iImageIndex = LoadImageInternalEx(Path, iDivideTextureSize);
								if (iImageIndex == 0)
								{
									// texture load e : 031208 - U71 - absolute path with relative path in model file
									sprintf(Path, "%s%s", TexturePath, pTextureName);
									if (strlen(Path) >= _MAX_PATH) Path[_MAX_PATH] = 0;
									iImageIndex = LoadImageInternalEx(Path, iDivideTextureSize);
									if (iImageIndex == 0)
									{
										// texture load f : 031208 - U71 - as above, but as DDS
										strcpy(pDDSFile, pTextureName);
										DWORD dwLenDot = strlen(pDDSFile);
										if (dwLenDot > 4)
										{
											pDDSFile[dwLenDot - 4] = 0;
											strcat(pDDSFile, ".dds");
											sprintf(Path, "%s%s", TexturePath, pDDSFile);
											iImageIndex = LoadImageInternalEx(Path, iDivideTextureSize);
										}
										if (iImageIndex == 0)
										{
											//PE: This can generates doubble loads. so this is the last combi to test.
											iImageIndex = LoadImageInternalEx(pNoPath, iDivideTextureSize);

											//LB: also test for DDS of NoPath (multitextured models that use PNG internally)
											if ( iImageIndex == 0 )
											{
												strcpy ( Path, pNoPath );
												Path[strlen(Path) - 4] = 0;
												strcat(Path, ".dds");
												iImageIndex = LoadImageInternalEx(Path, iDivideTextureSize);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return iImageIndex;
}

DARKSDK_DLL int LoadOrFindTextureAsImage ( LPSTR pTextureName, LPSTR TexturePath )
{
	return LoadOrFindTextureAsImage ( pTextureName, TexturePath, 0 );
}

