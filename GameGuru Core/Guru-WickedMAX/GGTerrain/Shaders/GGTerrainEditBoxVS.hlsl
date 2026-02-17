#include "GGRootSignature.hlsli"
#include "GGTerrainConstants.hlsli"

#include "GGEngineGlobals.hlsli"
#include "GGCustomFrameCB.hlsli"
#include "GGFrameCompat.hlsli"

struct VertexIn
{
    float3 position : POSITION;
};

struct VertexOut
{
	float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
	float3 origPos : TEXCOORD1;
};

VertexOut main( VertexIn IN )
{
    VertexOut OUT;
 
	float4 pos;
	pos.x = IN.position.x * terrain_mapEditSize;
	pos.y = IN.position.y * 30000;
	pos.z = IN.position.z * terrain_mapEditSize;
	pos.w = 1;

	OUT.origPos = IN.position;
	OUT.worldPos = pos.xyz;
	OUT.position = mul( g_xCamera_VP, pos );

	return OUT;
}

