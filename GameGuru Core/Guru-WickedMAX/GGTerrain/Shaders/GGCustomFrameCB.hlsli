// GGCustomFrameCB.hlsli - Constant buffer for GameGuru-specific per-frame data.
//
// Register b4 is inside the descriptor table range (b3-b13) defined in
// GGRootSignature.hlsli. The C++ side (GGTerrain_Draw, GGGrass_Draw, GGTrees_Draw)
// must create and bind a GPUBuffer containing GGCustomFrameData before custom draws.
//
// These fields were previously part of the old monolithic FrameCB at b0 but do NOT
// exist in the new Wicked Engine's FrameCB/ShaderWeather/ShaderScene structs.
//
// Shared between HLSL and C++. In C++ mode, use:
//   #define GG_CUSTOMFRAME_FULL_DECL
//   #include "Shaders/GGCustomFrameCB.hlsli"

#ifndef GG_CUSTOMFRAMECB_HLSLI
#define GG_CUSTOMFRAMECB_HLSLI

#if defined(GG_CUSTOMFRAME_FULL_DECL) || !defined(__cplusplus)

#ifdef __cplusplus
	#define GG_FLOAT3 XMFLOAT3
	#define GG_UINT uint32_t
#else
	#define GG_FLOAT3 float3
	#define GG_UINT uint
#endif

struct GGCustomFrameData {
	float     treeWind;                    // offset 0
	float     treeSubSurfaceScattering;    // offset 4
	float     sunEnergy;                   // offset 8
	float     deSaturate;                  // offset 12
	GG_FLOAT3 waterColor;                  // offset 16
	float     waterHeight;                 // offset 28
	float     waterFogMin;                 // offset 32
	float     waterFogMax;                 // offset 36
	float     waterFogMinAmount;           // offset 40
	float     fogOpacity;                  // offset 44
	GG_FLOAT3 fogColor;                    // offset 48
	float     fogHeightSky;                // offset 60 (atmospheric density for sky)
	float     cloudiness;                  // offset 64
	float     cloudScale;                  // offset 68
	float     cloudSpeed;                  // offset 72
	GG_UINT   ggOptions;                   // offset 76 (GG-specific option bits - GG_OPT_* below)
	// struct size = 80 bytes (5*16), 16-byte aligned.
};

// ggOptions bits. GGMAX 3.113: how the Terrain Bake pixel shader shades (GGTerrainBakePS.hlsl).
// Each bit brings one term in line with the LIVE terrain (the engine's object shader); all on is
// the shipped setting. They are bits rather than #ifdefs so the harness can A/B each one in a
// single session (SET_BAKESHADING <mask>) - the colour mismatch they fix was invisible until it
// was measured against the live terrain from the same camera.
#define GG_OPT_BAKE_DECODE_NORMAL  0x01u   // unpack the n*0.5+0.5 UNORM vertex normal (the 3.25 defect)
#define GG_OPT_BAKE_SSAO           0x02u   // screen-space AO, as objectHF reads it
#define GG_OPT_BAKE_PROBE_AMBIENT  0x04u   // global probe irradiance in the ambient, as the engine GetAmbient
#define GG_OPT_BAKE_FULL_SUN       0x08u   // full directional light: atmosphere transmittance, all cascades
#define GG_OPT_BAKE_LIVE_MATERIAL  0x10u   // f0 0.005 + no constant occlusion, as the live chunk material
// GGMAX 3.115: SHARPNESS. The bake used to sample its texture through s1, which it never binds - it
// inherited GGTrees' isotropic trilinear sampler (or the null sampler, mip 0 only, with trees off) - and
// up close it is magnified up to 8x. ANISO samples the bake with the static 16x anisotropic sampler;
// DETAIL blends the live terrain's own material textures back in where the bake is magnified (GGTerrainBakePS);
// DETAIL_NORMAL adds the materials' normal maps there too.
#define GG_OPT_BAKE_ANISO          0x20u   // bake fetch through sampler_aniso_clamp (s106), else sampler_linear_clamp (s100)
#define GG_OPT_BAKE_DETAIL         0x40u   // near detail: live material colour where the bake is magnified
#define GG_OPT_BAKE_DETAIL_NORMAL  0x80u   // near detail: the live materials' normal maps too
#define GG_OPT_BAKE_SHADING_ALL    0xFFu
#define GG_OPT_BAKE_SHADING_MASK   0xFFu   // the low byte of ggOptions belongs to the bake
// GGMAX 3.119: Water Bake DX11 water fog (GGWaterBakePS.hlsl). DEPTH = DX11's depth/distance opacity from the
// scene depth behind the plane (rtLinearDepth); LIT = the water colour lit by sun + ambient as DX11's ocean
// did; VIEW_ALPHA / VIEW_THICK = debug: opaque grey of the opacity / of thickness over Water Fog Max.
#define GG_OPT_WATERBAKE_DEPTH      0x100u
#define GG_OPT_WATERBAKE_LIT        0x200u
#define GG_OPT_WATERBAKE_VIEW_ALPHA 0x1000u
#define GG_OPT_WATERBAKE_VIEW_THICK 0x2000u
#define GG_OPT_WATERBAKE_MASK       0xFF00u

#ifdef __cplusplus
	#undef GG_FLOAT3
	#undef GG_UINT
#else
// HLSL cbuffer declaration
#ifdef CBUFFER
CBUFFER(GGCustomFrameCB, 4)
{
	GGCustomFrameData ggCustomFrame;
};
#else
// Fallback if included before engine ShaderInterop.h
cbuffer GGCustomFrameCB : register(b4)
{
	GGCustomFrameData ggCustomFrame;
};
#endif
#endif // __cplusplus

#endif // GG_CUSTOMFRAME_FULL_DECL || !__cplusplus

#endif // GG_CUSTOMFRAMECB_HLSLI
