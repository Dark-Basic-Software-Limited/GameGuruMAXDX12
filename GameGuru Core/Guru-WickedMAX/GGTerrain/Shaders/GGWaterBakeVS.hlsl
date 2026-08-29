#include "GGRootSignature.hlsli"
#include "PBR/globals.hlsli"

// GGMAX 3.25 - WATER BAKE, vertex shader.
//
// A single quad standing in for the whole water table. The vertex buffer holds only the quad's
// LOCAL offsets (+/- half size in X and Z) and the water height in Y; the camera's XZ is added
// here, so the plane follows the player and always reaches the horizon without anyone having to
// know how big the level is. That matches what the real ocean does - it is unbounded - so
// ticking the switch does not change where the water appears to end.
//
// Colour rides in the vertex rather than a constant buffer. It is four vertices; a dedicated CB
// would mean claiming another slot in the b3-b13 descriptor table and getting the bind order
// right in a pass this code does not own, for no gain.

struct VertexIn
{
	float3 position : POSITION;   // xz = local offset, y = absolute water height
	float4 color    : COLOR;
	float2 fres     : TEXCOORD0;  // GGMAX 3.35f: (strength, power) of the distance opacity ramp
};

struct VertexOut
{
	float4 position : SV_POSITION;
	float4 color    : COLOR;
	float3 worldPos : TEXCOORD0;
	float2 fres     : TEXCOORD1;  // GGMAX 3.35f
};

[RootSignature(GAMEGURU_ROOTSIGNATURE)]
VertexOut main( VertexIn IN )
{
	VertexOut OUT;
	const float3 world = float3( IN.position.x + g_xCamera_CamPos.x,
	                             IN.position.y,
	                             IN.position.z + g_xCamera_CamPos.z );
	OUT.position = mul( g_xCamera_VP, float4( world, 1.0 ) );
	OUT.color    = IN.color;
	OUT.worldPos = world;
	OUT.fres     = IN.fres;   // GGMAX 3.35f
	return OUT;
}
