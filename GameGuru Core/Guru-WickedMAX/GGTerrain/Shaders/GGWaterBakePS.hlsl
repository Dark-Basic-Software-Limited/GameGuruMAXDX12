#include "GGRootSignature.hlsli"
// brdf + lightingHF before GGCommonFunctions: ApplyFogCustom in there reaches GetDynamicSkyColor,
// which lives behind the engine lighting headers. Same include order GGGrassPS.hlsl uses.
#include "PBR/brdf.hlsli"
#include "PBR/lightingHF.hlsli"
#include "GGCommonFunctions.hlsli"

// GGMAX 3.25 - WATER BAKE, pixel shader.
//
// FLAT AUTHORED COLOUR AND NOTHING ELSE. No reflection, no refraction, no normal map, no FFT
// simulation, and - since Lee's 2026-08-26 feedback - no Fresnel either. The point of the switch
// is that the planar reflection pass (which redraws the whole scene from a mirrored camera,
// measured at 4.37 ms of a 10.06 ms GPU frame on TESTPRO1) stops being recorded; anything put
// back here works against that.
//
// ⚠ The ripples and refraction Lee reported in Water Bake mode were NOT this shader - they were
// the real ocean, still being drawn because the switch applied its visuals one frame before the
// flag it reads was written (see GGWaterBake.cpp). This shader never had either. The Fresnel
// lift it DID have is gone all the same: he asked for a plain colour with semi-transparency so
// the ground below the water line shows through, and that is exactly what this now is.
//
// Fog is kept, and that is a deliberate exception to "nothing else": the plane spans 250,000
// units, so without fog its far edge is a hard band of flat colour against a hazy sky. It is one
// lerp, not an effect.

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

	// ---- DEBUG BISECT ------------------------------------------------------------------------
	// Exact magenta arriving in the vertex colour means SET_WATERBAKEDEBUG is on, and this returns
	// it UNTOUCHED - no fog, no blend-relevant alpha, nothing.
	// The first bisect only forced the VERTEX colour and still ran ApplyFogCustom below, so at
	// several thousand units from the camera the fog could quietly absorb the whole test and the
	// result read as "the draw produced nothing" when it may have produced a fogged nothing.
	// A solid-colour bisect has to bypass the shader's own maths, or it is not solid.
	if ( IN.color.r > 0.99 && IN.color.g < 0.01 && IN.color.b > 0.99 )
		return float4( 1, 0, 1, 1 );
	// -------------------------------------------------------------------------------------------

	// the authored water colour, unmodified, at the authored opacity
	float3 rgb = IN.color.rgb;
	rgb = ApplyFogCustom( IN.worldPos, dist, rgb, V );
	return float4( max( 0, rgb ), IN.color.a );
}
