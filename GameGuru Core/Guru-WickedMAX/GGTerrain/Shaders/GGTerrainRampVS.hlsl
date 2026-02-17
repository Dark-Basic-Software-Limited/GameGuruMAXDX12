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
	float3 origPos : TEXCOORD1;
};

VertexOut main( VertexIn IN )
{
    VertexOut OUT;
 
	OUT.origPos = IN.position;
	float4 pos = float4( IN.position, 1 );
	pos = mul( terrain_rampWorldMat, pos );
	OUT.position = mul( g_xCamera_VP, pos );

	return OUT;
}

