#include "GGRootSignature.hlsli"

#include "GGTerrainConstants.hlsli"

#include "../GGTerrainPageSettings.h"

#include "PBR/brdf.hlsli"
#include "PBR/lightingHF.hlsli"

#include "GGCommonFunctions.hlsli"

Texture2DArray<float4> texColor  : register( t50 );
Texture2DArray<float2> texNormal : register( t51 );
Texture2DArray<float4> texSurface : register( t52 ); // R: occlusion, G: roughnes, B: metalness

SamplerState samplerTriWrap : register( s0 );

// EntityTiles now accessed via load_entitytile() from engine globals

#include "GGLighting.hlsli"

struct PixelIn
{
    float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD1;
	float3 normal : TEXCOORD3;
	uint instanceID : TEXCOORD2;
	float2 uv : TEXCOORD4;
};

struct GBuffer
{
	float4 g0 : SV_TARGET0;	/*FORMAT_R11G11B10_FLOAT*/
	float4 g1 : SV_TARGET1;	/*FORMAT_R8G8B8A8_FLOAT*/
};

GBuffer main( PixelIn IN )
{
	GBuffer output;

	uint indexX = IN.instanceID % 11;
	uint indexZ = IN.instanceID / 11;

	//float3 baseColor = float3( 0.6, 0.6, 0.6 );
	float3 baseColor = float3( 0.97, 0.74, 0.62 );  // copper
	//float3 baseColor = float3( 1.0, 0.85, 0.57 );  // gold
	float3 normal = float3( 0, 0, 1 );
	float3 surfaceTex = float3( 1, indexX * 0.1, indexZ ); // ao, roughness, metalness

	if ( IN.instanceID == 22 )
	{
		float matIndex = terrain_baseLayerMaterial & 0xFF;
		baseColor = texColor.Sample( samplerTriWrap, float3(IN.uv, matIndex) ).rgb;
		normal.rg = texNormal.Sample( samplerTriWrap, float3(IN.uv, matIndex) ).rg;
		surfaceTex = texSurface.Sample( samplerTriWrap, float3(IN.uv, matIndex) ).rgb;

		normal.rg = normal.rg * 2 - 1;
		normal.b = 1 - (normal.r*normal.r + normal.g*normal.g);
		if ( normal.b > 0 ) normal.b = sqrt( normal.b );
	}

	if ( indexZ == 0 ) baseColor *= 0.65;

	// calculate TBN matrix
	float3 vNormal = normalize( IN.normal );
	float3 tangent = cross( vNormal, float3(0,1,0) );
	if ( dot(tangent,tangent) > 0 ) tangent = normalize(tangent);
	else tangent = float3(1,0,0);
	float3 binormal = normalize( cross( vNormal, tangent ) );
	float3x3 TBN = float3x3( tangent, binormal, vNormal );

	// transform normal
	normal = mul( normal, TBN );
	normal = lerp( IN.normal, normal, terrain_bumpiness );
	normal = normalize( normal );

	//normal = vNormal;

	// WickedEngine PBR
	Surface surface;

	surface.N = normal;
	surface.P = IN.worldPos;
	surface.V = g_xCamera_CamPos - surface.P;
	float dist = length( surface.V );
	surface.V /= dist;

	// de-gamma is now done automatically by hardware due to sRGB texture
	//baseColor = pow( baseColor, terrain_textureGamma );
		
	float metalness = surfaceTex.b;
	float roughness = surfaceTex.g;
	float occlusion = surfaceTex.r;

	surface.createMetalness( metalness, roughness, occlusion, terrain_reflectance, baseColor, false );
	
	const float2 pixel = IN.position.xy;
	const float2 ScreenCoord = pixel * g_xFrame_InternalResolution_rcp;
	surface.pixel = pixel;
	surface.screenUV = ScreenCoord;
	
	surface.update();

	float3 ambient = GetAmbient(surface.N);
	//ambient = lerp(ambient, ambient * surface.sss.rgb, saturate(surface.sss.a));
	

	Lighting lighting;
	lighting.create(0, 0, ambient, 0);
	
	//ForwardLighting(surface, lighting);
	GGTiledLighting(surface, lighting);

	float4 color = float4(0,0,0,0);
	GGApplyLightingRefraction(surface, lighting, color);

	if ( (terrain_flags & GGTERRAIN_SHADER_FLAG2_USE_FOG) ) 
	{
		color.rgb = ApplyFogCustom( IN.worldPos, dist, color.rgb, surface.V );
	}
			
	color = max( 0, color );
	output.g0 = float4( color.rgb, 1 );
	output.g1 = float4( surface.N * 0.5f + 0.5f, surface.roughness ); // RGB=normal, A=roughness
	return output;
}