#include "GGRootSignature.hlsli"
// brdf + lightingHF before GGCommonFunctions: ApplyFogCustom in there reaches GetDynamicSkyColor,
// which lives behind the engine lighting headers. Same include order GGGrassPS.hlsl uses.
#include "PBR/brdf.hlsli"
#include "PBR/lightingHF.hlsli"
#include "GGCommonFunctions.hlsli"

// GGMAX 3.25 - WATER BAKE, pixel shader.
//
// Flat authored water colour, one Fresnel term, and the scene's own fog. No reflection, no
// refraction, no normal map, no FFT simulation - the entire point of the switch is that the
// planar reflection pass (which redraws the whole scene from a mirrored camera, measured at
// 4.37 ms of a 10.06 ms GPU frame on TESTPRO1) stops being recorded. Putting any of that back
// here would defeat it. What this buys over plain "Water Off" is that the water still READS as
// water from the shore: a lake does not vanish, it goes flat.
//
// The Fresnel lift is the one flourish and it is three instructions: a plane of constant colour
// looks like a hole in the world, whereas brightening at grazing angles is most of what makes a
// flat surface read as a liquid one.

struct PixelIn
{
	float4 position : SV_POSITION;
	float4 color    : COLOR;
	float3 worldPos : TEXCOORD0;
};

[RootSignature(GAMEGURU_ROOTSIGNATURE)]
float4 main( PixelIn IN ) : SV_TARGET
{
	const float3 viewVec = g_xCamera_CamPos - IN.worldPos;
	const float  dist    = length( viewVec );
	const float3 V       = viewVec / max( dist, 0.0001 );

	// grazing angles go lighter and more opaque, as water does
	const float fresnel = pow( saturate( 1.0 - abs( V.y ) ), 4.0 );

	float3 rgb   = IN.color.rgb * ( 0.85 + 0.55 * fresnel );
	float  alpha = saturate( IN.color.a + fresnel * ( 1.0 - IN.color.a ) * 0.85 );

	rgb = ApplyFogCustom( IN.worldPos, dist, rgb, V );
	return float4( max( 0, rgb ), alpha );
}
