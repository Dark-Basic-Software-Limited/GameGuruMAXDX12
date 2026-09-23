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
	float clip      : SV_ClipDistance0;   // GGMAX 3.88 - see main(); sibling GGTerrainVS.hlsl:22
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
	// GGMAX 3.88: THE REFLECTION PASS DRAWS THIS SHADER TOO, AND IT WAS THE ONLY ONE IN THE
	// FAMILY NOT CLIPPING.
	//
	// wiRenderPath3D.cpp:1642 calls customDraw_Opaque a SECOND time each frame with the planar-
	// reflection camera and mode 1, against the main-camera call at :1891 with mode 0. In GG's
	// lambda (master_part1.cpp:600-604) GGTrees_Draw and GGTerrain_Draw both take `mode`;
	// GGTerrainBake_Draw DROPS it, so the baked ground is drawn into the reflection with no idea
	// it is in one. The reflection camera carries a clip plane at the mirror height
	// (CameraComponent::Reflect, wiScene_Components.cpp:2809) and the bake rasterizes
	// CullMode::NONE, so from a reflection eye BELOW the water the ground's underside is drawn,
	// wins the reverse-Z GREATER_EQUAL test against the reflected scene behind it, and paints
	// over it. Lee's symptom: tick Terrain Bake and the puddle stops reflecting, while the whole
	// reflection ENABLE chain still reports healthy - DUMP_REFLECTION is byte-identical across
	// the toggle, because nothing is disabled. Only the CONTENT of the reflection is destroyed.
	//
	// GGTerrainVS.hlsl:33, GGTerrainPrepassRefVS.hlsl:27 and GGTreesVS.hlsl:64 have always done
	// this. The bake shader joined the family in 3.25 and never got it.
	// ★ A clip belongs to a FAMILY - prepass, colour, shadow, envprobe, reflection - and must
	//   land in all of them in the same edit. Third time this rule has been paid for.
	//
	// Free in the main pass: CameraComponent::clipPlane defaults to (0,0,0,0)
	// (wiScene_Components.h:1492), so the dot is 0 there and SV_ClipDistance only clips on
	// NEGATIVE. Safe in the shared VS: a PS input signature only has to be a SUBSET of the VS
	// output and SV_ClipDistance is a system value, so neither bake PS changes. Do NOT split the
	// file to add this - see the header above.
	OUT.clip     = dot( pos, g_xCamera_ClipPlane );
	return OUT;
}
