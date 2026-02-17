#include "GGRootSignature.hlsli"
Texture2DArray texBranchesHigh : register( t54 );

SamplerState samplerBilinearWrap : register( s0 );
SamplerState samplerTrilinearWrap : register( s2 );

#include "PBR/brdf.hlsli"
#include "PBR/lightingHF.hlsli"

#include "PBR/ShaderInterop_Renderer.h"
#include "GGTreesConstants.hlsli"

#include "GGCommonFunctions.hlsli"

// EntityTiles now accessed via load_entitytile() from engine globals

#include "GGLighting.hlsli"

struct PixelIn
{
    float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
	float3 normal : TEXCOORD1;
	uint RenderTargetIndex : SV_RenderTargetArrayIndex;
	float2 uv : TEXCOORD2;
    uint data : TEXCOORD4;
	float3 origPos : TEXCOORD3;
	bool isFront : SV_IsFrontFace;
};

float4 main( PixelIn IN ) : SV_TARGET
{
	uint treeType = GetTreeType( IN.data );
	uint index = GetTreeVariation( IN.data );

	float4 baseColor = texBranchesHigh.Sample( samplerTrilinearWrap, float3(IN.uv, treeType) );
	float alpha = baseColor.a;
	if ( alpha < 0.3 ) discard;
	
	//baseColor.rgb /= baseColor.a;
	alpha = (alpha - 0.3) / max(fwidth(alpha),0.0001) + 0.5;

	Surface surface;
	surface.P = IN.worldPos;
	surface.V = g_xCamera_CamPos - surface.P;
	float dist = length( surface.V );
	surface.V /= dist;

	float3 normal = normalize( IN.normal );
	if ( !IN.isFront ) normal = -normal;

	// WickedEngine PBR

	surface.N = normal;

	// de-gamma is now done automatically by hardware due to sRGB texture
	//baseColor.rgb = pow( baseColor.rgb, 2.2 ); // de-gamma

	surface.createMetalness( 0, 1, 1, GGTREES_REFLECTANCE, baseColor.rgb, true );
	
	const float2 pixel = IN.position.xy;
	const float2 ScreenCoord = pixel * g_xFrame_InternalResolution_rcp;
	surface.pixel = pixel;
	surface.screenUV = ScreenCoord;

	surface.update();
	
	float3 ambient = GetAmbient(surface.N);
	
	Lighting lighting;
	lighting.create(0, 0, ambient, 0);
	
	GGForwardLighting(surface, lighting);

	float4 color = float4(0,0,0,0);
	GGApplyLighting(surface, lighting, color);

	color.rgb = ApplyFogCustom( IN.worldPos, dist, color.rgb, surface.V );

	color = max( 0, color );
	return float4( color.rgb, alpha );
}