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
// sRGB: the bake shader (terrainBakeChunkCS.hlsl) applies the sRGB curve on write, because DX12
// forbids a UAV on an sRGB format so the texture has to be plain UNORM. That means the hardware
// will NOT de-gamma this fetch for us and we undo the curve here. RemoveSRGBCurve_Fast is the
// matching inverse of the ApplySRGBCurve_Fast used on the way in.
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

	const float3 normal = normalize( IN.normal );
	const float3 viewVec = g_xCamera_CamPos - IN.worldPos;
	const float dist = length( viewVec );

	Surface surface;
	surface.init();
	surface.P = IN.worldPos;
	surface.V = viewVec / max( dist, 0.0001 );
	surface.N = normal;
	// Terrain is a dielectric: no metalness, and a roughness high enough that the baked albedo
	// reads as ground rather than polished stone. The live terrain gets these per-layer from the
	// material surfacemap; baking that as a second texture would double this mode's memory for a
	// difference nobody looking at a low-spec fallback is going to notice.
	surface.createMetalness( 0, 1, 0.9, 0.04, baked, true );

	const float2 pixel = IN.position.xy;
	surface.pixel = pixel;
	surface.screenUV = pixel * g_xFrame_InternalResolution_rcp;
	surface.update();

	float3 ambient = GetAmbient( surface.N );

	Lighting lighting;
	lighting.create( 0, 0, ambient, 0 );
	GGTiledLightingSimple( surface, lighting, true );

	float4 color = float4( 0, 0, 0, 0 );
	GGApplyLighting( surface, lighting, color );

	color.rgb = ApplyFogCustom( IN.worldPos, dist, color.rgb, surface.V );
	color = max( 0, color );

	output.g0 = float4( color.rgb, 1 );
	output.g1 = float4( surface.N * 0.5f + 0.5f, surface.roughness );
	return output;
}
