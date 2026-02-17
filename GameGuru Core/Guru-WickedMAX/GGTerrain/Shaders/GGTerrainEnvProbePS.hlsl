#include "GGRootSignature.hlsli"
Texture2D texColorAndMetalness   : register( t50 );
Texture2D texNormalRoughnessAO   : register( t51 );
							     
Texture2DArray<float> texPageTableArray : register( t53 );
Texture2D<float> texPageTableFinal      : register( t54 );

SamplerState sampler1            : register( s1 );

#include "GGTerrainConstants.hlsli"

#include "../GGTerrainPageSettings.h"

#include "PBR/brdf.hlsli"
#include "PBR/lightingHF.hlsli"

#include "GGCommonFunctions.hlsli"

// EntityTiles now accessed via load_entitytile() from engine globals

#include "GGLighting.hlsli"

static const float4 mipColors[16] = {
	float4( 1.0, 0.0, 0.0, 1.0 ),
	float4( 0.0, 1.0, 0.0, 1.0 ),
	float4( 0.0, 0.0, 1.0, 1.0 ),
	float4( 1.0, 1.0, 0.0, 1.0 ),
	float4( 0.0, 1.0, 1.0, 1.0 ),
	float4( 1.0, 0.0, 1.0, 1.0 ),
	float4( 1.0, 1.0, 1.0, 1.0 ),
	float4( 0.5, 0.0, 0.0, 1.0 ),
	float4( 0.0, 0.5, 0.0, 1.0 ),
	float4( 0.0, 0.0, 0.5, 1.0 ),
	float4( 0.5, 0.5, 0.0, 1.0 ),
	float4( 0.0, 0.5, 0.5, 1.0 ),
	float4( 0.5, 0.0, 0.5, 1.0 ),
	float4( 0.5, 0.5, 0.5, 1.0 ),
	float4( 0.25, 0.25, 0.25, 1.0 ),
	float4( 0.0, 0.0, 0.0, 1.0 ),
};

struct PixelIn
{
    float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD1;
	float lodLevel: TEXCOORD2;
	float3 normal : TEXCOORD3;
	uint RenderTargetIndex : SV_RenderTargetArrayIndex;
	float2 uv : TEXCOORD4;
};

// virtual texture variables
static const float2 virtToPageSize = float2( pageSize / physTexSizeX, pageSize / physTexSizeY );
static const float2 texelOffset = float2( pagePaddingLeft / physTexSizeX, pagePaddingLeft / physTexSizeY );

float4 main( PixelIn IN ) : SV_TARGET
{
	// page table look up
	int maxLevel = terrain_numLODLevels - 1; 
	int detailLevel = texPageTableFinal.CalculateLevelOfDetailUnclamped( sampler1, IN.uv );
	detailLevel = max( detailLevel, IN.lodLevel );
	int origLOD = detailLevel;
	detailLevel = min( detailLevel, maxLevel + GGTERRAIN_MAX_PAGE_TABLE_MIP - 1 );
	float pageEntry = 0;
	float2 levelUV = float2(0,0);
	
	// go through the page table array until we find a suitable page
	while( pageEntry == 0 && detailLevel < maxLevel )
	{
		levelUV = IN.worldPos.xz - float2( terrain_LOD[ detailLevel ].x, terrain_LOD[ detailLevel ].z );
		levelUV *= terrain_LOD[ detailLevel ].size;
		levelUV.y = 1 - levelUV.y;
		
		int2 loadUV = levelUV * 256.0; // assumes texPageTableArray is 256 x 256 pixels
		pageEntry = texPageTableArray.Load( int4(loadUV, detailLevel, 0) ); // uv, arrayLevel, mipLevel
		
		detailLevel++;
	}

	uint mipLevel = max( detailLevel - maxLevel, 0 );

	// if still no page then go through the final page table mipmaps
	if ( pageEntry == 0 )
	{
		levelUV = IN.worldPos.xz - float2( terrain_LOD[ maxLevel ].x, terrain_LOD[ maxLevel ].z );
		levelUV *= terrain_LOD[ maxLevel ].size;
		levelUV.y = 1 - levelUV.y;

		uint mipSize = 256 >> mipLevel; // assumes texPageTableFinal is 256 x 256 pixels
		int2 loadUV = levelUV * mipSize; 
		
		while( pageEntry == 0 && mipLevel < GGTERRAIN_MAX_PAGE_TABLE_MIP ) // not a full mip stack
		{
			pageEntry = texPageTableFinal.Load( int3(loadUV, mipLevel) ); // uv, mipLevel
			loadUV >>= 1;
			mipLevel++;
			detailLevel++;
		}
		mipLevel--;
	}
	
	uint iPageEntry = (uint) (pageEntry * 65535);

	uint mipSize;
	if ( iPageEntry == 0 )
	{
		detailLevel = maxLevel + GGTERRAIN_MAX_PAGE_TABLE_MIP - 1;
		mipSize = 1;
	}
	else
	{
		detailLevel--;
		mipSize = 256 >> mipLevel;
	}
	
	// calculate UV for physical (page cache) texture
	uint iOffsetX = iPageEntry & 0xFF;
	uint iOffsetY = iPageEntry >> 8;
	float2 pageOffset = float2(iOffsetX / physPagesX, iOffsetY / physPagesY);

	float2 pageUV = frac( levelUV * mipSize ) * virtToPageSize + texelOffset;
	
	pageUV += pageOffset;

	float uvScale = 256.0 / (1 << detailLevel);
	float2 dx = ddx( IN.uv ) * (uvScale / physTexSizeX);
	float2 dy = ddy( IN.uv ) * (uvScale / physTexSizeY);

	// physical texture sample
	float4 colorMetalness = texColorAndMetalness.SampleGrad( sampler1, pageUV, dx, dy );
	float4 normalRoughnessAO = texNormalRoughnessAO.SampleGrad( sampler1, pageUV, dx, dy );

	// expand normal from 2 channels to 3 channels
	float3 normal;
	normal.rg = normalRoughnessAO.rg * 2 - 1;
	normal.b = 1 - (normal.r*normal.r + normal.g*normal.g);
	if ( normal.b > 0 ) normal.b = sqrt( normal.b );

	// calculate TBN matrix
	float3 tangent = normalize( cross( IN.normal, float3(0,0,1) ) );
	float3 binormal = normalize( cross( IN.normal, tangent ) );
	float3x3 TBN = float3x3( tangent, binormal, IN.normal );

	// transform normal
	normal = mul( normal, TBN );
	normal = lerp( IN.normal, normal, terrain_bumpiness );
	normal = normalize( normal );

	// WickedEngine PBR
	Surface surface;

	surface.N = normal;
	surface.P = IN.worldPos;
	surface.V = g_xCamera_CamPos - surface.P;
	float dist = length( surface.V );
	surface.V /= dist;

	float3 baseColor = colorMetalness.rgb;
	// de-gamma is now done automatically by hardware due to sRGB texture
	//baseColor = pow( baseColor, terrain_textureGamma ); // de-gamma
		
	float metalness = colorMetalness.a;
	float roughness = normalRoughnessAO.b;
	float occlusion = normalRoughnessAO.a;

	surface.createMetalness( metalness, roughness, occlusion, terrain_reflectance, baseColor, true );
	
	const float2 pixel = IN.position.xy;
	const float2 ScreenCoord = pixel * g_xFrame_InternalResolution_rcp;
	surface.pixel = pixel;
	surface.screenUV = ScreenCoord;
	
	surface.update();

	float3 ambient = GetAmbient(surface.N);
	//ambient = lerp(ambient, ambient * surface.sss.rgb, saturate(surface.sss.a));

	Lighting lighting;
	lighting.create(0, 0, ambient, 0);
	
	GGForwardLighting(surface, lighting);

	float4 color = float4(0,0,0,0);
	GGApplyLighting(surface, lighting, color);

	//ApplyFog(dist, color);
	color.rgb = ApplyFogCustom( IN.worldPos, dist, color.rgb, surface.V );
		
	color = max( 0, color );
	return float4( color.rgb, 1 );
}