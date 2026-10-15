#include "GGRootSignature.hlsli"
#include "PBR/globals.hlsli"
#include "GGTerrainFarCommon.hlsli"

// =============================================================================================
// GGMAX 3.114 - FAR TERRAIN vertex shader. ONE compiled VS for the prepass AND the colour PSO -
// the shared-VS depth contract every GG prepass/colour pair keeps (see GGTerrainBakeVS.hlsl).
//
// Input is the legacy GGTerrain clipmap's own vertex format (TerrainVertex, 20 bytes): WORLD
// position, the normal as R8G8B8A8_UNORM (n*0.5+0.5 - unpacked here, the 3.113 lesson), and an
// ID whose bits 16..23 are the vertex's LOD level.
// =============================================================================================

struct VertexIn
{
	float3 position : POSITION;
	float4 inormal  : INORMAL;
	uint   id       : ID;
};

struct VertexOut
{
	float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;      // UN-sunk: the layer blend and the coverage test use it
	float3 normal   : TEXCOORD1;
	nointerpolation uint lod : TEXCOORD2;
	float clip      : SV_ClipDistance0;
};

[RootSignature(GAMEGURU_ROOTSIGNATURE)]
VertexOut main( VertexIn IN )
{
	VertexOut OUT;
	const uint lod = ( IN.id >> 16 ) & 0xFF;
	float4 pos = float4( IN.position, 1.0 );
	OUT.worldPos = IN.position;
	OUT.normal = IN.inormal.rgb * 2 - 1;
	OUT.lod = lod;

	// Border sink: a far vertex standing on a live chunk drops under it, so the live chunk wins
	// the depth test and the sloped far triangles leaving the ring edge form a skirt.
	if ( FarCovered( FarCell( IN.position.xz ) ) )
	{
		pos.y -= far_sinkMul * far_segBase * exp2( (float)lod );
	}

	OUT.position = mul( g_xCamera_VP, pos );
	// the 3.88 family rule: every GG terrain-family VS clips against the reflection plane
	OUT.clip = dot( pos, g_xCamera_ClipPlane );
	return OUT;
}
