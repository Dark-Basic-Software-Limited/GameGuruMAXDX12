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
// The render targets are the engine's prepass targets: velocity at 0, virtual-texture readback
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
	float4 velocity : SV_TARGET0;
	uint   readback : SV_TARGET1;
};

[RootSignature(GAMEGURU_ROOTSIGNATURE)]
Output main( PixelIn IN )
{
	Output output;
	output.velocity = float4( 0, 0, 0, 1 );
	output.readback = 0;
	return output;
}
