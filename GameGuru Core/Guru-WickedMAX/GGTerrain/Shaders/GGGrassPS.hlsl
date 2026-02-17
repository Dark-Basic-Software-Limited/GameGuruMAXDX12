#include "GGRootSignature.hlsli"
Texture2DArray texGrass : register( t50 );
Texture2D<float> texNoise : register( t51 );
Texture2DArray texGrassNormal : register( t53 );

SamplerState samplerPointWrap : register( s0 );
SamplerState samplerTrilinearClamp : register( s1 );
SamplerState samplerTrilinearWrap : register( s2 );

#include "PBR/brdf.hlsli"
#include "PBR/lightingHF.hlsli"

#include "GGGrassConstants.hlsli"

#include "GGCommonFunctions.hlsli"

// EntityTiles now accessed via load_entitytile() from engine globals

#define GGLIGHTING_SIMPLE_SHADOWS
#include "GGLighting.hlsli"

struct PixelIn
{
    float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
	uint data : TEXCOORD2;
	float4 origPos : TEXCOORD3;
	float2 uv : TEXCOORD1;
	float2 uvNoise : TEXCOORD4;
	bool isFront : SV_IsFrontFace;
};

struct GBuffer
{
	float4 g0 : SV_TARGET0;	/*FORMAT_R11G11B10_FLOAT*/
	float4 g1 : SV_TARGET1;	/*FORMAT_R8G8B8A8_FLOAT*/
};

GBuffer main( PixelIn IN )
{
	GBuffer output;
	
	uint grassType = GetGrassType( IN.data );
	uint index = GetGrassVariation( IN.data );

	float4 baseColor = texGrass.Sample( samplerTrilinearClamp, float3(IN.uv, grassType) );
	float alpha = baseColor.a;
	if ( alpha < 0.5 ) discard;

	/*
	output.g0 = float4( 0.7,0.7,0.7,1 );
	output.g1 = float4( 0, 1, 0, 1 ); // RGB=normal, A=roughness
	return output;
	*/
	
	/*
	output.g0 = float4( 0, 0, 0, 1 );
	if ( IN.isFront ) output.g0.r = 1;
	else output.g0.g = 1;
	if ( IN.origPos.z > 50 ) output.g0.b = 1;
	output.g1 = float4( 0, 1, 0, 1 ); // RGB=normal, A=roughness
	return output;
	*/

	Surface surface;
	surface.P = IN.worldPos;
	surface.V = g_xCamera_CamPos - surface.P;
	
	float sqrDist = dot( surface.V, surface.V );

	float noise = texNoise.Sample( samplerTrilinearWrap, IN.uvNoise );
	float limit = noise * GGGRASS_LOD_TRANSITION + grass_lodDist;
	if( sqrDist > limit*limit ) discard;

	float dist = sqrt( sqrDist );
	surface.V /= dist;

	//baseColor.rgb /= baseColor.a;
	alpha = (alpha - 0.5) / max(fwidth(alpha),0.0001) + 0.5;

	//float4 baseColor = float4( 0.7,0.7,0.7,1 );
	//float alpha = 1;

	float height = IN.origPos.w;

	float3 normal = float3( 0, 1, 0 );
	/*
	{
		float2x2 rotMat = { grass_rotMat[ index ].x, grass_rotMat[ index ].y, grass_rotMat[ index ].z, grass_rotMat[ index ].w };

		//normal = texGrassNormal.Sample( samplerTrilinearClamp, float3(IN.uv, grassType) ).rgb;
		//normal = normal * 2 - 1;
		//normal.y = -normal.y;

		normal = float3( 0, 0, 1 );

		if( IN.isFront )
		{
			normal.x = -normal.x;
			normal.z = -normal.z;
		}
		
		normal.xz = mul( rotMat, normal.xz );
	}
	*/
	/*
	{
		normal.xz = surface.V.xz;

		normal = normalize( normal );

		normal.y = height;
		normal.y = normal.y * normal.y * 0.5;
		normal.y += 0.0001;

		normal = normalize( normal );
	}
	*/
	/*
	output.g0 = float4( normal*0.5 + 0.5, 1 );
	output.g1 = float4( 0, 1, 0, 1 ); // RGB=normal, A=roughness
	return output;
	*/

	// WickedEngine PBR

	surface.N = normal;

	// de-gamma is now done automatically by hardware due to sRGB texture
	//baseColor.rgb = pow( baseColor.rgb, 2.2 ); // de-gamma

	surface.createMetalness( 0, 1, 1, GGGRASS_REFLECTANCE, baseColor.rgb, true );
		
	const float2 pixel = IN.position.xy;
	const float2 ScreenCoord = pixel * g_xFrame_InternalResolution_rcp;
	surface.pixel = pixel;
	surface.screenUV = ScreenCoord;

	surface.update();
	
	float3 ambient = GetAmbient(surface.N);
	
	Lighting lighting;
	lighting.create(0, 0, ambient, 0);
	
	//ForwardLighting(surface, lighting);
	bool simplePBR = ((grass_flags & GGGRASS_FLAGS_SIMPLE_PBR) != 0);
	GGTiledLightingSimple( surface, lighting, simplePBR );

	float4 color = float4(0,0,0,0);
	GGApplyLighting(surface, lighting, color);

	color.rgb *= (height * 0.6 + 0.45);

	//ApplyFog(dist, color);
	color.rgb = ApplyFogCustom( IN.worldPos, dist, color.rgb, surface.V );

	//color.rgb *= shade;

	color = max( 0, color );

	output.g0 = float4( color.rgb, alpha );
	output.g1 = float4( surface.N * 0.5f + 0.5f, surface.roughness ); // RGB=normal, A=roughness
	return output;
}