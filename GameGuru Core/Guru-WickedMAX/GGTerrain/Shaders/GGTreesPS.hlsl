#include "GGRootSignature.hlsli"
Texture2DArray texTree : register( t50 );
Texture2D<float> texNoise : register( t51 );
Texture2DArray texTreeNormal : register( t53 );

SamplerState samplerBilinearWrap : register( s0 );
SamplerState samplerTrilinearClamp : register( s1 );

#include "PBR/brdf.hlsli"
#include "PBR/lightingHF.hlsli"

#include "PBR/ShaderInterop_Renderer.h"
#include "GGTreesConstants.hlsli"

#include "GGCommonFunctions.hlsli"

// EntityTiles now accessed via load_entitytile() from engine globals

#define GGLIGHTING_SKIP_ENVMAPS
#define GGLIGHTING_ALWAYS_SIMPLE
#include "GGLighting.hlsli"

struct PixelIn
{
    float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
	float  clip : SV_ClipDistance0;
	float2 uv : TEXCOORD1;
	float2 dir : TEXCOORD4;
	uint data : TEXCOORD2;
	bool isFront : SV_IsFrontFace;
};

struct GBuffer
{
	float4 g0 : SV_TARGET0;	/*FORMAT_R11G11B10_FLOAT*/
	float4 g1 : SV_TARGET1;	/*FORMAT_R8G8B8A8_FLOAT*/
};

GBuffer main( PixelIn IN )
{
	uint treeType = GetTreeType( IN.data );
	uint index = GetTreeVariation( IN.data );

	// GGMAX 3.05: FLAT GREY DEBUG QUADS. No texture fetch, no alpha cutout, no lighting, no fog -
	// the raw billboard quad, so the mesh->billboard handover can be watched geometrically.
	// MUST stay coverage-identical to GGTreesPrepassPS's debug branch (3.04 lesson).
	if ( tree_debugSolid )
	{
		// Would the LOD dissolve discard this fragment? Computed the same way in the prepass.
		bool dbgCut = false;
		if ( !any(g_xCamera_ClipPlane) )
		{
			float3 dbgV = g_xCamera_CamPos - IN.worldPos;
			float  dbgD2 = dot( dbgV, dbgV );
			float  dbgN = texNoise.Sample( samplerBilinearWrap, IN.uv*3 );
			float  dbgLim = dbgN * GGTREES_LOD_TRANSITION + tree_lodDist;
			dbgCut = ( dbgD2 < dbgLim*dbgLim );
		}
		// mode 1: normal dissolve. mode 2: never discard (solid rectangles - isolates whether the
		// black is a DISCARD problem at all). mode 3: never discard, paint the would-discard region
		// RED, so the dissolve zone is visible instead of invisible.
		if ( tree_debugSolid == 1 && dbgCut ) discard;
		float3 dbgCol = float3( 0.5, 0.5, 0.5 );
		if ( tree_debugSolid == 3 && dbgCut ) dbgCol = float3( 1.0, 0.0, 0.0 );
		GBuffer dbg;
		dbg.g0 = float4( dbgCol, 1.0 );
		dbg.g1 = float4( 0.5, 0.5, 1.0, 1.0 );
		return dbg;
	}

	float4 baseColor = texTree.Sample( samplerTrilinearClamp, float3(IN.uv, tree_type[ treeType ].slice) );
	float alpha = baseColor.a;
	if ( alpha < 0.3 ) discard;
	
	//baseColor.rgb /= baseColor.a;
	alpha = (alpha - 0.3) / max(fwidth(alpha),0.0001) + 0.5;
	
	GBuffer output;
	
	/*
	output.g0 = float4( 0, 0, 0, 1 );
	if ( IN.isFront ) output.g0.r = 1;
	else output.g0.g = 1;
	output.g1 = float4( 0, 1, 0, 1 ); // RGB=normal, A=roughness
	return output;
	*/

	Surface surface;
	surface.P = IN.worldPos;
	surface.V = g_xCamera_CamPos - surface.P;
	float sqrDist = dot( surface.V, surface.V );

	if ( !any(g_xCamera_ClipPlane) )
	{
		float noise = texNoise.Sample( samplerBilinearWrap, IN.uv*3 );
		float limit = noise * GGTREES_LOD_TRANSITION + tree_lodDist;
		if( sqrDist < limit*limit ) discard;
	}

	float dist = sqrt( sqrDist );
	surface.V /= dist;

	float height = 1 - IN.uv.y;

	float sinAng = IN.dir.x;
	float cosAng = IN.dir.y;

	// GGMAX 2.98b: normal map RESTORED. 2.98 swapped it for the analytic normal below to
	// reclaim 26.1 MB, and Lee's side-by-side settled it immediately - the canopy went flat and
	// uniformly bright where DX11 has real depth. The VRAM has to come from somewhere else.
	//float3 normal = float3( 0, 0, 1 );
	//normal.x = -sinAng * 0.7071068;
	//normal.y = 0.7071068;
	//normal.z = cosAng * 0.7071068;

	float3 normal = texTreeNormal.Sample( samplerTrilinearClamp, float3(IN.uv, tree_type[ treeType ].slice) ).rgb;
	normal = normal * 2 - 1;
	normal.y = abs(normal.y);

	float normX = normal.x * cosAng - normal.z * sinAng;
	float normZ = -normal.x * sinAng - normal.z * cosAng;
	normal.x = normX;
	normal.z = normZ;

	float3 dir = float3( -sinAng, 0, -cosAng );
	normal = lerp( dir, normal, 2 );

	// GGMAX 3.07: soften how dark the SHADED side of a billboard goes, keeping the technique.
	// Lee: "I like the technique, and we should keep it, but just lessen the severity of when the
	// billboard is in shade."
	// ★ NOT the lerp constant above - measured, LOWERING that darkens the shade (it is what lifts
	// the canopy toward a high sun, because normal.y = abs() and dir is purely horizontal, so
	// N.y = t*|n.y|). t=1.0 took the dark tail from 0.136 to 0.000 and the very-dark share from
	// 10% to 26%. The right lever is a WRAP (half-Lambert) term on the diffuse.
	// Wicked never normalises surface.N (surfaceHF.hlsli update() only saturates roughness), so
	// feeding it (N + w*|N|*L)/(1+w) reproduces the wrap term (N.L + w)/(1+w) EXACTLY for the sun,
	// carrying |N| through unchanged. w = 0 is therefore bit-identical to the pre-3.07 shader.
	if ( tree_shadeWrap > 0 )
	{
		// GetSunDirection() points TOWARD the sun - skyHF.hlsli:159 draws the sun disc where the
		// view ray matches it - so it is the L in N.L, no sign flip.
		const float3 sunL = normalize( g_xFrame_SunDirection );
		normal = ( normal + tree_shadeWrap * length( normal ) * sunL ) / ( 1.0 + tree_shadeWrap );
	}
	
	/*
	output.g0 = float4( normal*0.5 + 0.5, 1 );
	output.g1 = float4( 0, 1, 0, 1 ); // RGB=normal, A=roughness
	return output;
	*/

	// WickedEngine PBR

	surface.N = normal;

	// de-gamma is now done automatically by hardware due to sRGB texture
	//baseColor.rgb = pow( baseColor.rgb, 2.2 ); // de-gamma

	surface.createMetalness( 0, 1, 1, GGTREES_REFLECTANCE, baseColor.rgb, true );
	
	const float2 pixel = IN.position.xy;
	const float2 ScreenCoord = pixel * g_xFrame_InternalResolution_rcp;
	surface.pixel = pixel;
	surface.screenUV = ScreenCoord;

	surface.update();
	
	float3 ambient = GetAmbient(surface.N);
	
	Lighting lighting;
	lighting.create(0, 0, ambient, 0);
	
	//ForwardLighting(surface, lighting);
	GGTiledLighting(surface, lighting);

	float4 color = float4(0,0,0,0);
	GGApplyLighting(surface, lighting, color);

	//color.rgb *= height * 0.5 + 0.3;

	//ApplyFog(dist, color);
	color.rgb = ApplyFogCustom( IN.worldPos, dist, color.rgb, surface.V );

	color = max( 0, color );

	if ( GetTreeHighlighted(IN.data) ) color.rgb = lerp( color.rgb, float3(178.0/255.0, 148.0/255.0, 45.0/255.0), 0.3 );
	
	output.g0 = float4( color.rgb, alpha );
	output.g1 = float4( surface.N * 0.5f + 0.5f, surface.roughness ); // RGB=normal, A=roughness
	return output;
}