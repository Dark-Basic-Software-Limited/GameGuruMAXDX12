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
	GG_UINT   ggOptions;                   // offset 76 (GG-specific option bits)
};

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
