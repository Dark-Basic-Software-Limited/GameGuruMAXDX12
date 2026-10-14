#include "GGRootSignature.hlsli"

Texture2D texBakedChunk : register( t50 );

SamplerState samplerBilinearWrap   : register( s0 );
SamplerState samplerTrilinearClamp : register( s1 );
SamplerState samplerTrilinearWrap  : register( s2 );

#include "PBR/brdf.hlsli"
#include "PBR/lightingHF.hlsli"
#include "GGCommonFunctions.hlsli"

#define GGLIGHTING_SIMPLE_SHADOWS
#include "GGLighting.hlsli"

// =============================================================================================
// GGMAX 3.25 - TERRAIN BAKE colour pixel shader.
//
// One texture fetch and the shared cheap lighting path, and that is the whole point: the cost
// the Terrain Bake switch removes is not lighting maths, it is the virtual-texture indirection
// (residency map lookup, page table lookup, then the atlas fetch), the per-frame SVT feedback
// and page-request round trip, and the ECS transform/AABB/culling work for several hundred
// chunk entities. Take those away and what is left can afford to be lit normally, so the baked
// terrain still sits in the same lighting as everything around it instead of looking like a
// different game. GGTiledLightingSimple is the same path grass and the tree billboards use.
//
// sRGB: the bake shader (terrainBakeChunkCS.hlsl) applies the sRGB curve on write and the chunk
// texture is BC1_UNORM_SRGB, so the HARDWARE decodes this fetch. ⚠ Do NOT add a manual
// RemoveSRGBCurve here (an older version of this note said to): it would decode twice.
//
// GGMAX 3.113 - SHADED LIKE THE LIVE TERRAIN. Lee: the baked ground was a different colour from
// the live terrain - measured 1.30-1.35x brighter and flatter, same camera, grass off. Five terms
// differed from the engine's object shader that draws the live chunks; each is a ggOptions bit
// (GG_OPT_BAKE_*, GGCustomFrameCB.hlsli) so it can be A/B'd live, and all are on by default:
//  - DECODE_NORMAL: the vertex normal arrives as R8G8B8A8_UNORM packed n*0.5+0.5 and was never
//    unpacked, so flat ground was lit as if tilted ~35 degrees toward +X+Z - brighter or darker
//    depending on where the sun is, with slope relief cut to ~40%. A defect since 3.25.
//  - SSAO: the live terrain reads the camera AO texture (objectHF); the bake read none.
//  - PROBE_AMBIENT: the engine's ambient is the global probe's last mip + the flat ambient colour;
//    GG's GetAmbient is the flat colour alone.
//  - FULL_SUN: the 'simple' directional light skipped the atmosphere's transmittance (realistic
//    sky), which dims and reddens the sun on everything else.
//  - LIVE_MATERIAL: the live chunk ends up f0 0.005 with SSAO as its only occlusion; the bake used
//    f0 0.04 (8x the sky reflection) and a constant 0.9 occlusion.
// Still different by design: no normal map (it would be a second baked texture per chunk).
// =============================================================================================

struct PixelIn
{
	float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
	float3 normal   : TEXCOORD1;
	float2 uv       : TEXCOORD2;
};

struct GBuffer
{
	float4 g0 : SV_TARGET0;	/*FORMAT_R11G11B10_FLOAT*/
	float4 g1 : SV_TARGET1;	/*FORMAT_R8G8B8A8_FLOAT*/
};

[RootSignature(GAMEGURU_ROOTSIGNATURE)]
GBuffer main( PixelIn IN )
{
	GBuffer output;

	// ★ No manual sRGB decode any more. The baked chunk is now a BC1_UNORM_SRGB texture, so the
	// hardware decodes the curve the bake stored - and, unlike the old manual decode, it does so
	// BEFORE filtering, which is the correct order.
	float3 baked = texBakedChunk.Sample( samplerTrilinearClamp, IN.uv ).rgb;

	const uint bakeOpt = ggCustomFrame.ggOptions;
	// GGMAX 3.113: R8G8B8A8_UNORM delivers n*0.5+0.5 - unpack it (see the header).
	const float3 normal = ( bakeOpt & GG_OPT_BAKE_DECODE_NORMAL ) ? normalize( IN.normal * 2 - 1 ) : normalize( IN.normal );
	const float3 viewVec = g_xCamera_CamPos - IN.worldPos;
	const float dist = length( viewVec );

	Surface surface;
	surface.init();
	surface.P = IN.worldPos;
	surface.V = viewVec / max( dist, 0.0001 );
	surface.N = normal;
	// Terrain is a dielectric. Arguments: metalness, roughness, occlusion, reflectance (f0).
	// GGMAX 3.113: the live chunk ends up f0 = 0.005 (GG layer reflectance 0.005 x Surface.dds A x
	// chunk reflectance 1) and its only occlusion is SSAO - Surface.dds AO is never applied because
	// the chunk material has no OCCLUSION_PRIMARY. The old 0.04 / 0.9 gave 8x the sky reflection.
	const bool liveMaterial = ( bakeOpt & GG_OPT_BAKE_LIVE_MATERIAL ) != 0;
	surface.createMetalness( 0, 1, liveMaterial ? 1.0 : 0.9, liveMaterial ? 0.005 : 0.04, baked, true );

	const float2 pixel = IN.position.xy;
	surface.pixel = pixel;
	surface.screenUV = pixel * g_xFrame_InternalResolution_rcp;
	surface.update();

	// GGMAX 3.113: SSAO, exactly as objectHF.hlsli reads it. The bake has its own depth prepass,
	// and RenderAO runs after customDraw_Prepass, so the AO texture already includes the baked ground.
	[branch]
	if ( ( bakeOpt & GG_OPT_BAKE_SSAO ) && GetCamera().texture_ao_index >= 0 )
	{
		surface.occlusion *= bindless_textures_half4[descriptor_index( GetCamera().texture_ao_index )].SampleLevel( sampler_linear_clamp, surface.screenUV, 0 ).r;
	}

	// GG's GetAmbient is the flat ambient colour only. GGMAX 3.113: the engine's (lightingHF.hlsli
	// GetAmbient) adds the global probe's last mip along N - the sky's irradiance.
	float3 ambient = GetAmbient( surface.N );
	[branch]
	if ( ( bakeOpt & GG_OPT_BAKE_PROBE_AMBIENT ) && GetScene().globalprobe >= 0 )
	{
		TextureCube<half4> probe = bindless_cubemaps_half4[descriptor_index( GetScene().globalprobe )];
		uint2 probeDim;
		uint probeMips;
		probe.GetDimensions( 0, probeDim.x, probeDim.y, probeMips );
		ambient += probe.SampleLevel( sampler_linear_clamp, surface.N, probeMips ).rgb;
	}

	Lighting lighting;
	lighting.create( 0, 0, ambient, 0 );
	// GGMAX 3.113: simple = false is the full directional light - the atmosphere's transmittance on
	// the sun (realistic sky), every shadow cascade, direct specular - as the live terrain gets.
	GGTiledLightingSimple( surface, lighting, ( bakeOpt & GG_OPT_BAKE_FULL_SUN ) == 0 );

	// GGMAX 3.113: answer the engine's SET_TANGENTVIS debug views (objectHF.hlsli gg_debugvis) with
	// the SAME quantities and the SAME c/(1+c) encoding, so each lighting term of the baked ground
	// can be compared with the live terrain's from the same camera - the instrument that showed
	// which of the five differences mattered (tools/harness/bake_colour_probe.sh).
	{
		float4 dbg = float4( -1, 0, 0, 0 );
		switch ( GetFrame().gg_debugvis )
		{
		case 2:  dbg = float4( normal * 0.5 + 0.5, 1 ); break;                // vertex normal
		case 3:  dbg = float4( surface.N * 0.5 + 0.5, 1 ); break;             // final normal (no normal map)
		case 9:  dbg = float4( baked, 1 ); break;                             // albedo input
		case 12: dbg = float4( surface.f0, 1 ); break;                        // specular F0
		case 13: dbg = float4( surface.occlusion.xxx, 1 ); break;             // occlusion
		case 17: { float3 c = lighting.direct.diffuse;    dbg = float4( c / ( 1 + c ), 1 ); } break;
		case 18: { float3 c = lighting.direct.specular;   dbg = float4( c / ( 1 + c ), 1 ); } break;
		case 19: { float3 c = lighting.indirect.diffuse;  dbg = float4( c / ( 1 + c ), 1 ); } break;
		case 20: { float3 c = lighting.indirect.specular; dbg = float4( c / ( 1 + c ), 1 ); } break;
		case 23: dbg = float4( surface.albedo, 1 ); break;                    // albedo after createMetalness
		// the SUMMANDS of the final colour, unencoded - engine objectHF.hlsli 25/26/27. (GG's direct
		// diffuse already carries the 1/PI that the engine applies in ApplyLighting.)
		case 25: dbg = float4( surface.albedo * lighting.direct.diffuse, 1 ); break;
		case 26: dbg = float4( surface.albedo * lighting.indirect.diffuse * surface.occlusion, 1 ); break;
		case 27: dbg = float4( lighting.direct.specular + lighting.indirect.specular * surface.occlusion, 1 ); break;
		}
		[branch]
		if ( dbg.x >= 0 )
		{
			output.g0 = dbg;
			output.g1 = float4( surface.N * 0.5f + 0.5f, surface.roughness );
			return output;
		}
	}

	float4 color = float4( 0, 0, 0, 0 );
	GGApplyLighting( surface, lighting, color );

	color.rgb = ApplyFogCustom( IN.worldPos, dist, color.rgb, surface.V );
	color = max( 0, color );

	output.g0 = float4( color.rgb, 1 );
	output.g1 = float4( surface.N * 0.5f + 0.5f, surface.roughness );
	return output;
}
