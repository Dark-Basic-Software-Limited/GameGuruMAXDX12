#include "GGRootSignature.hlsli"
#include "PBR/globals.hlsli"

// =============================================================================================
// GGMAX 3.25 - TERRAIN BAKE, shared vertex shader.
//
// ONE compiled VS drives BOTH the depth prepass PSO and the colour PSO, and that is a
// correctness requirement, not tidiness. Reverse-Z depth uses GREATER_EQUAL, and the colour
// pass runs depth_write_mask = ZERO, so it only survives where its computed depth matches the
// depth the prepass laid down. Two TEXTUALLY IDENTICAL vertex shaders compiled separately do
// NOT guarantee the same result - approximate instructions and a different instruction schedule
// move the vertex in world space by more than one ULP, the colour fragment is then rejected, the
// gbuffer is never written, and the chunk renders BLACK and flickers with tiny camera moves.
// That exact bug cost days on the far-tree billboards (see NIGHT_INVESTIGATIONS 3.04-3.06).
// A pixel shader's input signature only has to be a SUBSET of the VS output, so one VS serves
// both passes. Do not split this file.
//
// Vertices arrive in WORLD space already. The bake writes them out that way on purpose: the
// baked chunks are static for the lifetime of the bake, so folding the chunk offset in once at
// build time removes a per-chunk constant buffer, a per-vertex add, and a whole class of
// "which space is this in" bug from the draw path.
// =============================================================================================

struct VertexIn
{
	float3 position : POSITION;   // world space
	float3 normal   : NORMAL;
	float2 uv       : TEXCOORD0;  // chunk UV, indexes the baked chunk texture directly
};

struct VertexOut
{
	float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
	float3 normal   : TEXCOORD1;
	float2 uv       : TEXCOORD2;
};

[RootSignature(GAMEGURU_ROOTSIGNATURE)]
VertexOut main( VertexIn IN )
{
	VertexOut OUT;
	const float4 pos = float4( IN.position, 1.0 );
	OUT.position = mul( g_xCamera_VP, pos );
	OUT.worldPos = IN.position;
	OUT.normal   = IN.normal;
	OUT.uv       = IN.uv;
	return OUT;
}
