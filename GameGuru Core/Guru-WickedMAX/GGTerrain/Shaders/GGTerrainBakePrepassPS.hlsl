#include "GGRootSignature.hlsli"
#include "PBR/ShaderInterop_Renderer.h"

// GGMAX 3.25 - TERRAIN BAKE depth prepass pixel shader.
//
// Depth-only in effect: the baked terrain is fully opaque with no alpha cutout anywhere, so
// there is nothing to discard and nothing this shader can do that would make its coverage
// disagree with the colour pass. That matters - a visibility test present in one pass of a
// prepass/colour pair and absent from the other writes depth without colour, which is the
// second half of the BLACK-terrain failure the shared VS guards against. Keeping this shader
// unconditional is the simplest possible proof that the two coverages match.
//
// RT0 is the engine's PrimitiveID visibility buffer (masked off since 3.75); RT1 is 3.77's
// rtCustomDepth. The DX11 names below are historical:  velocity at 0, virtual-texture readback
// at 1. Readback is written as 0 because a baked chunk has no virtual texture to feed.

struct PixelIn
{
	float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
	float3 normal   : TEXCOORD1;
	float2 uv       : TEXCOORD2;
};

struct Output
{
	float4 velocity : SV_TARGET0;   // masked off since 3.75 - see the PSO
	float  depth    : SV_TARGET1;  // GGMAX 3.77: rtCustomDepth
};

[RootSignature(GAMEGURU_ROOTSIGNATURE)]
Output main( PixelIn IN )
{
	Output output;
	// ★★★ GGMAX 3.77: SV_TARGET1 IS NOW THE PREPASS DEPTH CARRIER.
	// RT0 is the engine's R32_UINT PrimitiveID visibility buffer and this shader cannot pack a
	// valid id (no meshlet), so 3.75 masked our colour writes off it. But visibility_resolveCS
	// then reads our pixels as SKY, and texture_depth - which the clouds, SSAO, SSR and aerial
	// perspective all sample - is the texture that shader WRITES. Against a hill 3.75 was
	// enough (the hill's own id survived behind us); against open sky there is nothing behind,
	// so cloud drew straight over the billboards poking above the ridge. Writing the real
	// post-projection depth here is what makes these draws occluders in their own right.
	output.velocity = float4( 0, 0, 0, 1 );
	output.depth = IN.position.z;
	return output;
}
