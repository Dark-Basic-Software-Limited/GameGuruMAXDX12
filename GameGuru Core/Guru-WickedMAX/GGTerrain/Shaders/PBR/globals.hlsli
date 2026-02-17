// PBR/globals.hlsli - GG compatibility globals layer over new Wicked Engine DX12.
//
// MUST use a different include guard than the engine's globals.hlsli
// (which uses WI_SHADER_GLOBALS_HF) so both can coexist.
//
// Include chain:
//   shader.hlsl -> PBR/globals.hlsli (GG_SHADER_GLOBALS_HF)
//     -> GGEngineGlobals.hlsli -> engine globals.hlsli (WI_SHADER_GLOBALS_HF)
//         -> engine ShaderInterop.h, ShaderInterop_Renderer.h
//     -> PBR/ShaderInterop.h (GG resource macros)
//     -> PBR/ShaderInterop_Renderer.h (GG option bits)
//     -> GGCustomFrameCB.hlsli (b4 CB)
//     -> GGFrameCompat.hlsli (old name -> new accessor macros)
//     -> ResourceMapping.h (TEXSLOT_* defines)

#ifndef GG_SHADER_GLOBALS_HF
#define GG_SHADER_GLOBALS_HF

// ============================================================================
// 1. New engine globals (provides everything in WI_SHADER_GLOBALS_HF):
//    FrameCB/CameraCB structs, GetFrame()/GetCamera()/GetScene()/GetWeather(),
//    load_entity(), lights()/probes()/decals() iterators,
//    bindless arrays, sampler declarations, pack/unpack, tonemap, blue_noise,
//    is_saturated, sqr, flatten2D/unflatten2D, PI/FLT_MAX/etc.
// ============================================================================
#include "GGEngineGlobals.hlsli"

// ============================================================================
// 2. GG slot-based resource declaration macros (TEXTURE2D, STRUCTUREDBUFFER, etc.)
//    The engine uses 100% bindless. GG custom shaders use explicit SRV slots.
// ============================================================================
#include "ShaderInterop.h"

// ============================================================================
// 3. GG-specific OPTION_BIT values not in the new engine's enum
// ============================================================================
#include "ShaderInterop_Renderer.h"

// ============================================================================
// 4. GG custom per-frame constant buffer at b4
//    Contains fields that were in the old FrameCB but don't exist in the new one.
// ============================================================================
#include "GGCustomFrameCB.hlsli"

// ============================================================================
// 5. Old g_xFrame_* / g_xCamera_* name -> new accessor compatibility macros
// ============================================================================
#include "GGFrameCompat.hlsli"

// ============================================================================
// 6. TEXSLOT_* / SBSLOT_* slot defines for GG resources
// ============================================================================
#include "ResourceMapping.h"

// ============================================================================
// 7. Slot-based texture declarations for resources NOT provided as engine
//    bindless macros. These are used by lightingHF.hlsli, GGLighting.hlsli,
//    GGTerrainPS.hlsl, etc. for shadow/envmap/voxel access.
//
//    NOTE: Many textures that were slot-based in the old engine are now
//    bindless macros in the new engine (texture_depth, texture_bluenoise,
//    texture_gbuffer*, etc.). Those MUST NOT be redeclared here.
// ============================================================================

TEXTURECUBE(texture_globalenvmap, float4, TEXSLOT_GLOBALENVMAP);
TEXTURE2D(texture_globallightmap, float4, TEXSLOT_GLOBALLIGHTMAP);
// texture_envmaparray REMOVED: env probes now use bindless cubemaps via
//   bindless_cubemaps_half4[descriptor_index(probeTextureIndex)]
TEXTURE2D(texture_decalatlas, float4, TEXSLOT_DECALATLAS);

// Shadow textures REMOVED: now using engine's shadow atlas via bindless access
//   shadow_2D() and shadow_cube() in engine's shadowHF.hlsli handle atlas lookup

TEXTURE3D(texture_voxelradiance, float4, TEXSLOT_VOXELRADIANCE);

// EntityArray/MatrixArray REMOVED: now inline in FrameCB, accessed via
//   load_entity(index) and load_entitymatrix(index) macros from engine globals.

// ============================================================================
// 8. GG-specific helper functions (NOT in engine globals)
// ============================================================================

#define DEGAMMA_SKY(x)	pow(abs(x), g_xFrame_StaticSkyGamma)
#define DEGAMMA(x)		pow(abs(x), 2.2)
#define GAMMA(x)		pow(abs(x), 1.0 / g_xFrame_Gamma)

inline float GetSunEnergy() { return ggCustomFrame.sunEnergy; }
inline float3 GetFogColor() { return ggCustomFrame.fogColor; }
inline float GetFogOpacity() { return ggCustomFrame.fogOpacity; }

// Exponential height fog based on: https://www.iquilezles.org/www/articles/fog/fog.htm
inline float GetFogAmount(float distance, float3 O, float3 V)
{
	float fogDensity = saturate((distance - g_xFrame_Fog.x) / (g_xFrame_Fog.y - g_xFrame_Fog.x));

	if (g_xFrame_Options & OPTION_BIT_HEIGHT_FOG)
	{
		float fogHeightStart = g_xFrame_Fog.z;
		float fogHeightEnd = g_xFrame_Fog.w;
		float fogFalloffScale = 1.0 / max(0.01, fogHeightEnd - fogHeightStart);

		// solve for x, e^(-h * x) = 0.001
		// x = 6.907755 * h^-1
		float fogFalloff = 6.907755 * fogFalloffScale;

		float originHeight = O.y;
		float Z = -V.y;
		float effectiveZ = max(abs(Z), 0.001);

		float endLineHeight = originHeight + distance * Z;
		float minLineHeight = min(originHeight, endLineHeight);
		float heightLineFalloff = max(minLineHeight - fogHeightStart, 0);

		float baseHeightFogDistance = clamp((fogHeightStart - minLineHeight) / effectiveZ, 0, distance);
		float exponentialFogDistance = distance - baseHeightFogDistance;
		float exponentialHeightLineIntegral = exp(-heightLineFalloff * fogFalloff) * (1.0 - exp(-exponentialFogDistance * effectiveZ * fogFalloff)) / (effectiveZ * fogFalloff);

		float opticalDepthHeightFog = fogDensity * (baseHeightFogDistance + exponentialHeightLineIntegral);
		float transmittanceHeightFog = exp(-opticalDepthHeightFog);

		float fogAmount = transmittanceHeightFog;
		return 1.0 - fogAmount;
	}
	else
	{
		return fogDensity;
	}
}

// ============================================================================
// 9. GG-specific utility functions with OLD naming conventions.
//    The new engine provides equivalent functions under different names
//    (e.g., vertexID_create_cube, compute_lineardepth, get_tangentspace,
//    reconstruct_position, etc.). These wrappers preserve the old API that
//    GG shaders use.
//
//    Functions that are EXACT DUPLICATES of the engine's globals.hlsli
//    (hammersley2d, InterleavedGradientNoise, hash1, flatten3D, unflatten3D,
//    compute_tangent_frame, hemispherepoint_uniform/cos, SampleTextureCatmullRom,
//    expandBits, morton3D, BayerMatrix2/3/4/8, ditherMask2/3/4/8, dither,
//    halton64) have been REMOVED - they're now provided by the engine.
// ============================================================================

// Old-name compat for inverse_tonemap (engine uses inverse_tonemap)
inline float3 inverseTonemap(float3 x) { return inverse_tonemap(x); }

// returns a random float in range (0, 1). seed must be >0!
inline float rand(inout float seed, in float2 uv)
{
	float result = frac(sin(seed * dot(uv, float2(12.9898, 78.233))) * 43758.5453);
	seed += 1;
	return result;
}

// Creates a unit cube triangle strip from just vertex ID (14 vertices)
inline float3 CreateCube(in uint vertexID)
{
	uint b = 1u << vertexID;
	return float3((0x287a & b) != 0, (0x02af & b) != 0, (0x31e3 & b) != 0);
}

// Creates a full screen triangle from 3 vertices
inline void FullScreenTriangle(in uint vertexID, out float4 pos)
{
	pos.x = (float)(vertexID / 2) * 4.0 - 1.0;
	pos.y = (float)(vertexID % 2) * 4.0 - 1.0;
	pos.z = 0;
	pos.w = 1;
}
inline void FullScreenTriangle(in uint vertexID, out float4 pos, out float2 uv)
{
	FullScreenTriangle(vertexID, pos);

	uv.x = (float)(vertexID / 2) * 2;
	uv.y = 1 - (float)(vertexID % 2) * 2;
}

// Computes linear depth from post-projection depth
inline float getLinearDepth(in float z, in float near, in float far)
{
	float lin = (1 - z) * near / far;
	lin += z;
	return near / lin;
}
inline float getLinearDepth(in float z)
{
	return getLinearDepth(z, g_xCamera_ZNearP, g_xCamera_ZFarP);
}

inline float3x3 GetTangentSpace(in float3 normal)
{
	float3 helper = abs(normal.x) > 0.99 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 tangent = normalize(cross(normal, helper));
	float3 binormal = normalize(cross(normal, tangent));
	return float3x3(tangent, binormal, normal);
}

// GG-specific hemisphere sampling (uses seed+pixel API, not RNG struct)
inline float3 SampleHemisphere_uniform(in float3 normal, inout float seed, in float2 pixel)
{
	return mul(hemispherepoint_uniform(rand(seed, pixel), rand(seed, pixel)), GetTangentSpace(normal));
}
inline float3 SampleHemisphere_cos(in float3 normal, inout float seed, in float2 pixel)
{
	return mul(hemispherepoint_cos(rand(seed, pixel), rand(seed, pixel)), GetTangentSpace(normal));
}

// Reconstructs world-space position from depth buffer
inline float3 reconstructPosition(in float2 uv, in float z, in float4x4 InvVP)
{
	float x = uv.x * 2 - 1;
	float y = (1 - uv.y) * 2 - 1;
	float4 position_s = float4(x, y, z, 1);
	float4 position_v = mul(InvVP, position_s);
	return position_v.xyz / position_v.w;
}
inline float3 reconstructPosition(in float2 uv, in float z)
{
	return reconstructPosition(uv, z, g_xCamera_InvVP);
}

// Spheremap transform normal encoding/decoding (different from engine's octahedral encode_oct/decode_oct)
float2 encodeNormal(float3 n)
{
	float f = sqrt(8 * n.z + 8);
	return n.xy / f + 0.5;
}
float3 decodeNormal(float2 enc)
{
	float2 fenc = enc * 4 - 2;
	float f = dot(fenc, fenc);
	float g = sqrt(1 - f / 4);
	float3 n;
	n.xy = fenc * g;
	n.z = 1 - f / 2;
	return n;
}

// Convert texture coordinates on a cubemap face to cubemap sampling coordinates
inline float3 UV_to_CubeMap(in float2 uv, in uint faceIndex)
{
	uv = uv * 2 - 1;
	uv.y *= -1;

	switch (faceIndex)
	{
	case 0: return float3(1, uv.y, -uv.x);
	case 1: return float3(-1, uv.yx);
	case 2: return float3(uv.x, 1, -uv.y);
	case 3: return float3(uv.x, -1, uv.y);
	case 4: return float3(uv, 1);
	case 5: return float3(-uv.x, uv.y, -1);
	default: return 0;
	}
}

// Ray-primitive intersection functions (GG PascalCase naming)
float Trace_sphere(float3 o, float3 d, float3 center, float radius)
{
	float3 rc = o - center;
	float c = dot(rc, rc) - (radius * radius);
	float b = dot(d, rc);
	float dd = b * b - c;
	float t = -b - sqrt(abs(dd));
	float st = step(0.0, min(t, dd));
	return lerp(-1.0, t, st);
}
float Trace_plane(float3 o, float3 d, float3 planeOrigin, float3 planeNormal)
{
	return dot(planeNormal, (planeOrigin - o) / dot(planeNormal, d));
}
float Trace_triangle(float3 o, float3 d, float3 A, float3 B, float3 C)
{
	float3 planeNormal = normalize(cross(B - A, C - B));
	float t = Trace_plane(o, d, A, planeNormal);
	float3 p = o + d * t;

	float3 N1 = normalize(cross(B - A, p - B));
	float3 N2 = normalize(cross(C - B, p - C));
	float3 N3 = normalize(cross(A - C, p - A));

	float d0 = dot(N1, N2);
	float d1 = dot(N2, N3);

	float threshold = 1.0 - 0.001;
	return (d0 > threshold && d1 > threshold) ? 1.0 : 0.0;
}
float Trace_rectangle(float3 o, float3 d, float3 A, float3 B, float3 C, float3 D)
{
	return max(Trace_triangle(o, d, A, B, C), Trace_triangle(o, d, C, D, A));
}
float Trace_disk(float3 o, float3 d, float3 diskCenter, float diskRadius, float3 diskNormal)
{
	float t = Trace_plane(o, d, diskCenter, diskNormal);
	float3 p = o + d * t;
	float3 diff = p - diskCenter;
	return dot(diff, diff) < sqr(diskRadius);
}

// Closest point on line/segment (GG PascalCase naming)
float3 ClosestPointOnLine(float3 a, float3 b, float3 c)
{
	float3 ab = b - a;
	float t = dot(c - a, ab) / dot(ab, ab);
	return a + t * ab;
}
float3 ClosestPointOnSegment(float3 a, float3 b, float3 c)
{
	float3 ab = b - a;
	float t = dot(c - a, ab) / dot(ab, ab);
	return a + saturate(t) * ab;
}

#endif // GG_SHADER_GLOBALS_HF
