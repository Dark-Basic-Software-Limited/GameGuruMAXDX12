#include "GGRootSignature.hlsli"

#include "PBR/brdf.hlsli"
#include "PBR/lightingHF.hlsli"
#include "GGCommonFunctions.hlsli"

#define GGLIGHTING_SIMPLE_SHADOWS
#include "GGLighting.hlsli"

#include "GGTerrainBakePush.hlsli"
PUSHCONSTANT(bakePush, GGTerrainBakePush);

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
// RemoveSRGBCurve on the baked fetch (an older version of this note said to): it would decode twice.
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
//
// GGMAX 3.115 - SHARP UP CLOSE. Lee: "fix the baked terrain blurriness up close". Two causes:
//  - ANISO: the baked texture was sampled through s1, which the bake never binds - it inherited
//    GGTrees' ISOTROPIC trilinear sampler (or the null sampler, mip 0 only, when trees were off), so
//    ground seen at a grazing angle lost about two mips from ~10 m out. Now the static 16x
//    anisotropic sampler (s106), no binding and no hidden dependency on what drew before.
//  - DETAIL: inside ~15 m an 8192 bake is MAGNIFIED - its texel is material mip 3 (0.64 units) where
//    the live terrain samples mip 0 (0.08 units). No resolution fixes that affordably, so where the
//    bake's own LOD goes below 0 the live terrain's material textures are blended back in: the same
//    blendmap (retained from the bake), the same layer materials, the same tiling (32 repeats per
//    chunk, world-continuous), the same splat/decal rule and the same fast-sRGB round trip as
//    terrainBakeChunkCS.hlsl - so at bake LOD 0, where bake mip 0 IS material mip 3 with aligned
//    texel centres, the two images carry the same information and the handover is invisible.
//    Faded in LOD space, not by distance: no FOV, render scale or eye height in it.
//    The C++ enables it in SPLAT mode only (decal mode measured a 2-5% tint against the bake); the
//    decal branch below is kept so the two rules stay side by side with the compute shader.
//  - DETAIL_NORMAL: the materials' BC5 normal maps there too, splat-averaged exactly as the live VT
//    does, in the live terrain's own diagonal tangent frame (wiTerrain.cpp: T = P(x+1,z) - P(x,z+1)),
//    rebuilt per pixel from the vertex normal - so the relief matches the live ground's.
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
	const uint bakeOpt = ggCustomFrame.ggOptions;

	// ★ Derivatives FIRST, outside every branch: the detail loop below runs per pixel under a weight
	// test and a LOD test, where an implicit-derivative Sample is undefined. Everything samples
	// with SampleGrad (or SampleLevel) from these.
	const float2 duvdx = ddx( IN.uv );
	const float2 duvdy = ddy( IN.uv );

	// The baked chunk, fetched bindlessly from the push constants (it used to be t50). BC1_UNORM_SRGB:
	// the hardware decodes the curve before filtering.
	Texture2D bakedTex = bindless_textures[ descriptor_index( bakePush.bakedTex ) ];
	float3 baked;
	[branch]
	if ( bakeOpt & GG_OPT_BAKE_ANISO )
		baked = bakedTex.SampleGrad( sampler_aniso_clamp, IN.uv, duvdx, duvdy ).rgb;
	else
		baked = bakedTex.SampleGrad( sampler_linear_clamp, IN.uv, duvdx, duvdy ).rgb;

	// GGMAX 3.113: R8G8B8A8_UNORM delivers n*0.5+0.5 - unpack it (see the header).
	const float3 normal = ( bakeOpt & GG_OPT_BAKE_DECODE_NORMAL ) ? normalize( IN.normal * 2 - 1 ) : normalize( IN.normal );

	// ---- GGMAX 3.115: near detail ----------------------------------------------------------------
	float3 albedo = baked;
	float3 shadeN = normal;
	float  detailW = 0;
	uint   layersSampled = 0;
	[branch]
	if ( ( bakeOpt & GG_OPT_BAKE_DETAIL ) && bakePush.blendmap >= 0 && bakePush.layerTable >= 0 )
	{
		// The bake's own LOD: < 0 = magnified.
		// the LOD the bake fetch above actually uses: 16x aniso, or isotropic (major axis) without the bit
		const float lodBake = get_lod( uint2( bakePush.bakeRes, bakePush.bakeRes ), duvdx, duvdy, ( bakeOpt & GG_OPT_BAKE_ANISO ) ? 16 : 0 );
		detailW = saturate( ( bakePush.fadeLodEnd - lodBake ) * bakePush.fadeRcpWidth );
		if ( bakePush.flags & GG_BAKEPUSH_FORCEFULL ) detailW = 1;
		if ( bakePush.flags & GG_BAKEPUSH_FORCENONE ) detailW = 0;

		[branch]
		if ( detailW > 0 )
		{
			Texture2DArray blendmap = bindless_textures2DArray[ descriptor_index( bakePush.blendmap ) ];
			ByteAddressBuffer layers = bindless_buffers[ descriptor_index( bakePush.layerTable ) ];
			// terrainBakeChunkCS: the blendmap at the UNFLIPPED chunk uv (its 66-spacing / 67-texel
			// stretch included, as the compute shader has it), the materials at (u, 1-v) * repeats.
			const float2 tuv = float2( IN.uv.x, 1 - IN.uv.y );
			const float2 tdx = float2( duvdx.x, -duvdx.y );
			const float2 tdy = float2( duvdy.x, -duvdy.y );
			const bool splat = ( bakePush.flags & GG_BAKEPUSH_SPLAT ) != 0;
			const bool wantNormals = splat && ( bakeOpt & GG_OPT_BAKE_DETAIL_NORMAL );

			float4 total = 0;
			float  accumulation = 0;
			float  splatWeight = 0;
			float2 normalSum = 0;
			// front-to-back with early exit, exactly as the bake (and the live VT) visit the layers
			for ( int i = (int)bakePush.layerCount - 1; i >= 0; i-- )
			{
				const float w = blendmap.SampleLevel( sampler_linear_clamp, float3( IN.uv, i ), 0 ).r;
				if ( w == 0 )
					continue;
				const uint  addr = bakePush.layerTableOffset + (uint)i * GG_BAKE_LAYER_STRIDE;
				const uint4 e = layers.Load4( addr );
				float4 c = asfloat( layers.Load4( addr + 16 ) );   // baseColor
				const float R = asfloat( e.z );
				[branch]
				if ( (int)e.x >= 0 )
				{
					Texture2D tex = bindless_textures[ descriptor_index( (int)e.x ) ];
					c *= tex.SampleGrad( sampler_aniso_wrap, tuv * R, tdx * R, tdy * R );
				}
				[branch]
				if ( wantNormals )
				{
					// a layer with no normal map is FLAT, not absent (terrainVirtualTextureUpdateCS 3.113)
					float2 nm = float2( 0.5, 0.5 );
					[branch]
					if ( (int)e.y >= 0 )
					{
						Texture2D ntex = bindless_textures[ descriptor_index( (int)e.y ) ];
						nm = ntex.SampleGrad( sampler_aniso_wrap, tuv * R, tdx * R, tdy * R ).rg;
					}
					normalSum += w * nm;
				}
				layersSampled++;
				[branch]
				if ( splat )
				{
					total += w * c;
					splatWeight += w;
					continue;
				}
				total = mad( 1 - accumulation, w * c, total );
				accumulation = mad( 1 - w, accumulation, w );
				if ( accumulation >= 1 )
					break;
			}
			if ( splat && splatWeight > 0 )
			{
				total /= splatWeight;
				normalSum /= splatWeight;
			}
			// The bake stored ApplySRGBCurve_Fast(colour) and BC1_UNORM_SRGB decodes the EXACT curve, so
			// the baked ground carries that small bias (-2% at 0.1 linear, +6% at 0.01). Put the detail
			// through the same round trip or the fade band shows a brightness step.
			const float3 detail = RemoveSRGBCurve( ApplySRGBCurve_Fast( max( 0, total.rgb ) ) );
			albedo = lerp( baked, detail, detailW );

			[branch]
			if ( wantNormals && splatWeight > 0 )
			{
				// objectHF: bump = (rg*2-1, 1), N = normalize(mul(bump, TBN)) at strength 1. The live chunk's
				// tangent is P(x+1,z) - P(x,z+1): in the surface plane, XZ direction (1,-1); B = cross(T, N).
				const float3 bump = float3( normalSum * 2 - 1, 1 );
				const float3 T = normalize( float3( 1, ( normal.z - normal.x ) / max( normal.y, 0.05 ), -1 ) );
				const float3 B = cross( T, normal );
				const float3 Nd = normalize( bump.x * T + bump.y * B + bump.z * normal );
				shadeN = normalize( lerp( normal, Nd, detailW ) );
			}
		}
	}

	const float3 viewVec = g_xCamera_CamPos - IN.worldPos;
	const float dist = length( viewVec );

	Surface surface;
	surface.init();
	surface.P = IN.worldPos;
	surface.V = viewVec / max( dist, 0.0001 );
	surface.N = shadeN;
	// Terrain is a dielectric. Arguments: metalness, roughness, occlusion, reflectance (f0).
	// GGMAX 3.113: the live chunk ends up f0 = 0.005 (GG layer reflectance 0.005 x Surface.dds A x
	// chunk reflectance 1) and its only occlusion is SSAO - Surface.dds AO is never applied because
	// the chunk material has no OCCLUSION_PRIMARY. The old 0.04 / 0.9 gave 8x the sky reflection.
	const bool liveMaterial = ( bakeOpt & GG_OPT_BAKE_LIVE_MATERIAL ) != 0;
	surface.createMetalness( 0, 1, liveMaterial ? 1.0 : 0.9, liveMaterial ? 0.005 : 0.04, albedo, true );

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
	// GGMAX 3.115 adds 40 = near detail weight, 41 = layers sampled per pixel (/4), 42 = baked-only albedo.
	{
		float4 dbg = float4( -1, 0, 0, 0 );
		switch ( GetFrame().gg_debugvis )
		{
		case 2:  dbg = float4( normal * 0.5 + 0.5, 1 ); break;                // vertex normal
		case 3:  dbg = float4( surface.N * 0.5 + 0.5, 1 ); break;             // final normal (detail normals when on)
		case 9:  dbg = float4( albedo, 1 ); break;                            // albedo input (bake + detail)
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
		case 40: dbg = float4( detailW.xxx, 1 ); break;                       // 3.115 near detail weight
		case 41: dbg = float4( saturate( layersSampled / 4.0 ).xxx, 1 ); break;
		case 42: dbg = float4( baked, 1 ); break;                             // the bake alone
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
