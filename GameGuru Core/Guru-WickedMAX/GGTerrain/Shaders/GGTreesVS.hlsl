
#define GGTREES_DEBUG_SOLID 0   // GGMAX 2.96c BISECT stage A - REMOVE WHEN DONE
#include "GGRootSignature.hlsli"

/*
cbuffer CameraCB : register( b1 )
{
	float4x4	g_xCamera_VP;			// View*Projection
	float4		g_xCamera_ClipPlane;
	float3		g_xCamera_CamPos;
};
*/

#include "PBR/globals.hlsli"
#include "GGTreesConstants.hlsli"

struct VertexIn
{
	float2 position : POSITION;
	float3 offset: OFFSET;
	uint data : DATA;
	//uint instanceID : SV_InstanceID;
};

struct VertexOut
{
	float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
	float  clip : SV_ClipDistance0;
	float2 uv : TEXCOORD1;
	float2 dir : TEXCOORD4;
	uint data : TEXCOORD2;
};

[RootSignature(GAMEGURU_ROOTSIGNATURE)]
VertexOut main( VertexIn IN )
{
    VertexOut OUT;

	uint treeType = GetTreeType( IN.data );
	uint index = GetTreeVariation( IN.data );
	
	float2 posOrig = IN.position.xy * GetTreeScale( IN.data );
	posOrig.x *= tree_type[ treeType ].scaleX;
	posOrig.y *= tree_type[ treeType ].scaleY;

	float4 pos = float4( posOrig, 0, 1 );
	
	float2 diff = IN.offset.xz - g_xCamera_CamPos.xz;
	float invV = rsqrt( diff.x*diff.x + diff.y*diff.y ); // approximation
	diff *= invV;
	float posX = pos.x * diff.y + pos.z * diff.x;
	float posZ = pos.z * diff.y - pos.x * diff.x;

	pos.x = posX;
	pos.z = posZ;
	 
	pos.xyz += IN.offset;
	OUT.position = mul( g_xCamera_VP, pos );

	OUT.dir.x = diff.x;
	OUT.dir.y = diff.y;

	OUT.worldPos = pos.xyz;
	OUT.clip = dot( pos, g_xCamera_ClipPlane );
	OUT.uv.x = IN.position.x + 0.5;
	OUT.uv.y = 1 - IN.position.y;
	OUT.data = IN.data;

#if GGTREES_DEBUG_SOLID
	// GGMAX 2.96c BISECT stage A: keep the REAL transform (so the quads stay small, distant and
	// spread out - the earlier fixed-fullscreen-quad version drew 98,410 near-fullscreen quads
	// per frame and was a guaranteed GPU timeout). Only never-clip, so the clip-plane path
	// cannot be what hides them.
	OUT.clip = 1.0;
#endif

    return OUT;
}

