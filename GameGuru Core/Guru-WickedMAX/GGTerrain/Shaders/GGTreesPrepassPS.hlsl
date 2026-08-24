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
	float4 velocity : SV_TARGET0;
	uint   readback : SV_TARGET1;  // virtual texture read back
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
		// identical LOD dither discard to the real path - the handover must still be watchable
		if ( !any(g_xCamera_ClipPlane) )
		{
			float3 dbgV = g_xCamera_CamPos - IN.worldPos;
			float  dbgD2 = dot( dbgV, dbgV );
			float  dbgN = texNoise.Sample( samplerBilinearWrap, IN.uv*3 );
			float  dbgLim = dbgN * GGTREES_LOD_TRANSITION + tree_lodDist;
			if ( dbgD2 < dbgLim*dbgLim ) discard;
		}
		Output dbg;
		dbg.velocity = float4( 0, 0, 0, 1 );
		dbg.readback = 0;
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
	output.velocity = float4( 0, 0, 0, alpha );
	output.readback = 0; 	
	return output;
}