#include "GGRootSignature.hlsli"

// GGMAX 3.114: the live terrain's five layer colour maps - the very Color.dds textures its Wicked
// layer materials load (GGTerrainFar.cpp ResolveMaterials holds its own references, so they outlive
// the Wicked terrain under a Terrain Bake). Bound through their sRGB views: samples are linear.
Texture2D farMatBase   : register( t51 );
Texture2D farMatLow    : register( t52 );
Texture2D farMatLayer1 : register( t53 );
Texture2D farMatHigh   : register( t54 );
Texture2D farMatSlope  : register( t55 );

#include "PBR/brdf.hlsli"
#include "PBR/lightingHF.hlsli"
#include "GGCommonFunctions.hlsli"

#define GGLIGHTING_SIMPLE_SHADOWS
#include "GGLighting.hlsli"

#include "GGTerrainFarCommon.hlsli"

// =============================================================================================
// GGMAX 3.114 - FAR TERRAIN colour pixel shader.
//
// ALBEDO: GG's own height/slope layer rule, evaluated per pixel - the same rule, the same order
// and the same 5-slot material mapping the live terrain's blendmaps carry (FillChunkBlendmapGG /
// ApplyDX11StyleAutoBlend: base, then layer 0 -> low, layer 1 -> layer1, layers 2..4 -> high, then
// the slopes -> slope; DX11's GGTerrainPageGenPS ran it as successive lerps exactly like this), so
// the far field meets the ring edge in the same colours. Materials are the live terrain's own colour
// maps at the live terrain's tiling; at these distances the anisotropic mip chain makes that the material's
// average colour, which is what the live terrain's far tiles show too.
//
// LIGHTING + FOG: GGTerrainBakePS with every 3.113 term on - decoded normal, SSAO, probe ambient, full
// sun (atmosphere transmittance), f0 0.005 - and ApplyFogCustom, which mirrors the engine's fog on
// the live chunks. Painted materials are not applied out here yet (the live material map is not
// uploaded in Wicked mode); the editable area only reaches the far field from far off-centre.
// =============================================================================================

struct PixelIn
{
	float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
	float3 normal   : TEXCOORD1;
	nointerpolation uint lod : TEXCOORD2;
};

struct GBuffer
{
	float4 g0 : SV_TARGET0;
	float4 g1 : SV_TARGET1;
};

float2 FarUV( float3 worldPos )
{
	return float2( worldPos.x, -worldPos.z ) * far_texScale;
}

// uvdx/uvdy are taken by the caller BEFORE the coverage discard and outside every branch here: a
// derivative after a discard or inside divergent flow is undefined.
float3 FarAlbedo( float3 worldPos, float3 normal, float2 uvdx, float2 uvdy )
{
	const float2 uv = FarUV( worldPos );
	const float h = worldPos.y;
	float3 c = farMatBase.SampleGrad( sampler_aniso_wrap, uv, uvdx, uvdy ).rgb;

	// layer 0 -> low, layer 1 -> layer 1, layers 2..4 -> high (rcp 0 = start == end, unused layer)
	const float  st[5] = { far_layerStart[0].x, far_layerStart[0].y, far_layerStart[0].z, far_layerStart[0].w, far_layerStart[1].x };
	const float  rc[5] = { far_layerRcp[0].x,   far_layerRcp[0].y,   far_layerRcp[0].z,   far_layerRcp[0].w,   far_layerRcp[1].x };
	[unroll] for ( int i = 0; i < 5; i++ )
	{
		if ( rc[i] == 0 ) continue;
		const float t = saturate( ( h - st[i] ) * rc[i] );
		[branch] if ( t > 0 )
		{
			float3 m;
			if ( i == 0 ) m = farMatLow.SampleGrad( sampler_aniso_wrap, uv, uvdx, uvdy ).rgb;
			else if ( i == 1 ) m = farMatLayer1.SampleGrad( sampler_aniso_wrap, uv, uvdx, uvdy ).rgb;
			else m = farMatHigh.SampleGrad( sampler_aniso_wrap, uv, uvdx, uvdy ).rgb;
			c = lerp( c, m, t );
		}
	}
	const float normaly = 1 - abs( normal.y );
	[unroll] for ( int s = 0; s < 2; s++ )
	{
		const float sst = ( s == 0 ) ? far_slope.x : far_slope.z;
		const float src = ( s == 0 ) ? far_slope.y : far_slope.w;
		if ( src == 0 ) continue;
		const float t = saturate( ( normaly - sst ) * src );
		[branch] if ( t > 0 ) c = lerp( c, farMatSlope.SampleGrad( sampler_aniso_wrap, uv, uvdx, uvdy ).rgb, t );
	}
	return c;
}

[RootSignature(GAMEGURU_ROOTSIGNATURE)]
GBuffer main( PixelIn IN )
{
	const float2 uv = FarUV( IN.worldPos );
	const float2 uvdx = ddx( uv ), uvdy = ddy( uv );
	if ( FarInterior( IN.worldPos.xz ) ) discard;   // ★ identical to the prepass

	GBuffer output;
	// SET_TERRAINFAR_DEBUG 9: a flat colour straight after the discard - the cost instrument (an
	// FPS A/B against 0 splits shading from geometry/raster)
	[branch] if ( FAR_DEBUG_VIEW == 9 )
	{
		output.g0 = float4( 0.35, 0.4, 0.3, 1 );
		output.g1 = float4( 0.5, 1, 0.5, 1 );
		return output;
	}
	const float3 normal = normalize( IN.normal );
	float3 albedo = FarAlbedo( IN.worldPos, normal, uvdx, uvdy );

	const float3 viewVec = g_xCamera_CamPos - IN.worldPos;
	const float dist = length( viewVec );

	Surface surface;
	surface.init();
	surface.P = IN.worldPos;
	surface.V = viewVec / max( dist, 0.0001 );
	surface.N = normal;
	surface.createMetalness( 0, 1, 1.0, 0.005, albedo, true );   // the live terrain's material (3.113)
	const float2 pixel = IN.position.xy;
	surface.pixel = pixel;
	surface.screenUV = pixel * g_xFrame_InternalResolution_rcp;
	surface.update();

	[branch] if ( GetCamera().texture_ao_index >= 0 )
	{
		surface.occlusion *= bindless_textures_half4[descriptor_index( GetCamera().texture_ao_index )].SampleLevel( sampler_linear_clamp, surface.screenUV, 0 ).r;
	}

	float3 ambient = GetAmbient( surface.N );
	[branch] if ( GetScene().globalprobe >= 0 )
	{
		TextureCube<half4> probe = bindless_cubemaps_half4[descriptor_index( GetScene().globalprobe )];
		uint2 probeDim;
		uint probeMips;
		probe.GetDimensions( 0, probeDim.x, probeDim.y, probeMips );
		ambient += probe.SampleLevel( sampler_linear_clamp, surface.N, probeMips ).rgb;
	}

	Lighting lighting;
	lighting.create( 0, 0, ambient, 0 );
	GGTiledLightingSimple( surface, lighting, false );

	float4 color = float4( 0, 0, 0, 0 );
	GGApplyLighting( surface, lighting, color );
	color.rgb = ApplyFogCustom( IN.worldPos, dist, color.rgb, surface.V );
	color = max( 0, color );

	// SET_TERRAINFAR_DEBUG: 1 tint the far field, 2 coverage cells, 3 one colour per clipmap level
	const uint dbg = FAR_DEBUG_VIEW;
	[branch] if ( dbg == 1 ) color.rgb = lerp( color.rgb, float3( 1, 0.2, 0.8 ), 0.5 );
	else if ( dbg == 2 ) color.rgb = FarCovered( FarCell( IN.worldPos.xz ) ) ? float3( 1, 1, 0 ) : float3( 0, 0.4, 1 );
	else if ( dbg == 3 )
	{
		const float3 lc[4] = { float3( 1, 0, 0 ), float3( 0, 1, 0 ), float3( 0, 0, 1 ), float3( 1, 1, 0 ) };
		color.rgb = lc[ IN.lod & 3 ] * ( 0.4 + 0.15 * (float)( IN.lod >> 2 ) );
	}

	output.g0 = float4( color.rgb, 1 );
	output.g1 = float4( surface.N * 0.5f + 0.5f, surface.roughness );
	return output;
}
