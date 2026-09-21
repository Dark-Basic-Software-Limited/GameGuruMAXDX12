#include "GGRootSignature.hlsli"

Texture2DArray texTree : register( t50 );
Texture2D<float> texNoise : register( t51 );

SamplerState samplerBilinearWrap : register( s0 );
SamplerState samplerTrilinearClamp : register( s1 );

#include "PBR/ShaderInterop_Renderer.h"
#include "GGTreesConstants.hlsli"

struct PixelIn
{
    float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
	float  clip : SV_ClipDistance0;
	float2 uv : TEXCOORD1;
	uint data : TEXCOORD2;
};

struct Output
{
	float4 velocity : SV_TARGET0;   // masked off since 3.75 - see the PSO
	float  depth    : SV_TARGET1;  // GGMAX 3.77: rtCustomDepth (was the VT readback slot)
};

[RootSignature(GAMEGURU_ROOTSIGNATURE)]
Output main( PixelIn IN )
{
	uint treeType = GetTreeType( IN.data );
	uint index = GetTreeVariation( IN.data );

	// GGMAX 3.05: matches GGTreesPS's debug branch EXACTLY. If this pass kept the alpha cutout
	// while the colour pass drew the whole quad, the two coverages would disagree and we would be
	// straight back to 3.04's black fringes - in the very build meant to diagnose the flicker.
	if ( tree_debugSolid )
	{
		// Mode 1 and 3 keep the dissolve so depth matches the colour pass. Mode 2 disables it in
		// BOTH passes - if the black survives mode 2 then no discard is involved anywhere and the
		// colour pass is either producing black or being depth-rejected.
		if ( tree_debugSolid != 2 && tree_debugSolid != 4 && tree_debugSolid != 5 && !any(g_xCamera_ClipPlane) )
		{
			float3 dbgV = g_xCamera_CamPos - IN.worldPos;
			float  dbgD2 = dot( dbgV, dbgV );
			float  dbgN = texNoise.Sample( samplerBilinearWrap, IN.uv*3 );
			float  dbgLim = dbgN * GGTREES_LOD_TRANSITION + tree_lodDist;
			if ( dbgD2 < dbgLim*dbgLim ) discard;
		}
		Output dbg;
		dbg.velocity = float4( 0, 0, 0, 1 );
		// 3.77: the debug branch is coverage-identical to the real one by design (3.04/3.05),
		// so it carries the depth too - otherwise the debug modes stop occluding clouds.
		dbg.depth = IN.position.z;
		return dbg;
	}

	float alpha = texTree.Sample( samplerTrilinearClamp, float3(IN.uv, tree_type[ treeType ].slice) ).a;
	if ( alpha < 0.3 ) discard;

	alpha = (alpha - 0.3) / max(fwidth(alpha),0.0001) + 0.5;

	if ( !any(g_xCamera_ClipPlane) )
	{
		float3 viewDir = g_xCamera_CamPos - IN.worldPos;
		float sqrDist = dot( viewDir, viewDir );

		float noise = texNoise.Sample( samplerBilinearWrap, IN.uv*3 );
		float limit = (noise * GGTREES_LOD_TRANSITION) + tree_lodDist;
		if ( sqrDist < limit*limit ) discard;
	}

	Output output;
	// ★★★ GGMAX 3.77: SV_TARGET1 IS NOW THE PREPASS DEPTH CARRIER.
	// RT0 is the engine's R32_UINT PrimitiveID visibility buffer and this shader cannot pack a
	// valid id (no meshlet), so 3.75 masked our colour writes off it. But visibility_resolveCS
	// then reads our pixels as SKY, and texture_depth - which the clouds, SSAO, SSR and aerial
	// perspective all sample - is the texture that shader WRITES. Against a hill 3.75 was
	// enough (the hill's own id survived behind us); against open sky there is nothing behind,
	// so cloud drew straight over the billboards poking above the ridge. Writing the real
	// post-projection depth here is what makes these draws occluders in their own right.
	output.velocity = float4( 0, 0, 0, alpha );
	output.depth = IN.position.z;
	return output;
}