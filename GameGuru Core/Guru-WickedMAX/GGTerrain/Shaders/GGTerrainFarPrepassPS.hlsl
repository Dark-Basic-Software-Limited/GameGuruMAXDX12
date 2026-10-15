#include "GGRootSignature.hlsli"
#include "PBR/ShaderInterop_Renderer.h"
#include "GGTerrainFarCommon.hlsli"

// GGMAX 3.114 - FAR TERRAIN depth prepass. The same output contract as GGTerrainBakePrepassPS
// (RT0 = the engine's visibility buffer, masked off by the PSO; RT1 = rtCustomDepth, the real
// post-projection depth so clouds / SSAO / aerial perspective treat the far field as an occluder),
// plus the coverage discard - which MUST match GGTerrainFarPS exactly (shared hlsli).

struct PixelIn
{
	float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
	float3 normal   : TEXCOORD1;
	nointerpolation uint lod : TEXCOORD2;
};

struct Output
{
	float4 velocity : SV_TARGET0;
	float  depth    : SV_TARGET1;
};

[RootSignature(GAMEGURU_ROOTSIGNATURE)]
Output main( PixelIn IN )
{
	if ( FarInterior( IN.worldPos.xz ) ) discard;
	Output output;
	output.velocity = float4( 0, 0, 0, 1 );
	output.depth = IN.position.z;
	return output;
}
