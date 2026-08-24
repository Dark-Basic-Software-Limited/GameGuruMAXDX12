
#ifndef WI_SHADERINTEROP_RENDERER_H
#include "PBR/ShaderInterop_Renderer.h"
#endif

#ifndef __cplusplus
#ifndef GG_FRAMECOMPAT_HLSLI
#include "GGCustomFrameCB.hlsli"
#include "GGFrameCompat.hlsli"
#endif
#endif

#define GGTREES_REFLECTANCE   0.004

#define GGTREES_LOD_TRANSITION    500.0
#define GGTREES_LOD_SHADOW_TRANSITION 500.0

#if defined(GGTREES_CONSTANTS_FULL_DECL) || !defined(__cplusplus)

#ifdef __cplusplus
	#define VAR_UNIT uint32_t
#else
	#define VAR_UNIT uint
#endif

struct TreeType
{
	float scaleX;
	float scaleY;
	// GGMAX 2.99: ATLAS SLICE for this tree type. The billboard atlases used to be allocated
	// with one slice per tree type in the build (38) and every slice uploaded, whether the level
	// used the type or not. They are now sized to the types the level ACTUALLY places, so the
	// array index is no longer the type index and the shader has to look it up. Reuses a spare
	// padding float - no CB layout change. -1 = type not present in this level.
	float slice;
	float padding2;
};

static const VAR_UNIT numTreeTypes = 38;

#ifdef __cplusplus
struct TreeCB
#else
cbuffer TreeCB : register( b3 )
#endif
{
	float4   tree_rotMatShadow;

	float4   tree_rotMat[ 8 ];	

	TreeType tree_type[ numTreeTypes ];
	
	float3   tree_playerPos;
	uint     tree_padding0;

	float    tree_lodDist;
	float    tree_lodDistShadow;
	// GGMAX 3.00: how far terrain actually exists from the camera. The chunk-granular CPU cull
	// keeps any chunk that merely OVERLAPS the terrain, so trees near a straddling chunk's far
	// edge still drew over nothing. This is the exact per-tree test, done in the VS for free.
	float    tree_terrainReach;
	// GGMAX 3.04: DIAGNOSTIC. 1 = the PREPASS applies tree_terrainReach too (correct, default).
	// 0 = prepass skips it, reproducing the 3.00-3.03 defect on demand: depth written for quads
	// the colour pass clips, gbuffer never filled, tree renders BLACK. Kept because this failure
	// mode (a visibility test in one pass and not its partner) has now bitten twice. Spare
	// padding float, no CB layout change.
	float    tree_prepassReach;
};

uint GetTreeType( uint data ) { return (data >> 11) & 0x3F; }
uint GetTreeVariation( uint data ) { return (data >> 8) & 0x7; }
uint GetTreeHighlighted( uint data ) { return data & 0x4; }
float GetTreeScale( uint data ) { return ((data >> 16) & 0xFE) / 170.0 + 0.5; }

#ifndef __cplusplus
//PE: Animate the trees a bit.
float TreeWaveX(float posy, float posx)
{
    if (g_xFrame_TreeWind <= 0)
        return (0);
    const float swayspeed = g_xFrame_TreeWind * 6.0; // (0.85)
    const float swayamount = g_xFrame_TreeWind * 0.35; //0.075
    const float time = g_xFrame_Time;
    const float sdat = sin((time * (swayspeed * 1.5)) + posx) + cos((time * (swayspeed * 0.8)) + posx) + sin((time * (swayspeed * 1.2)));
    const float wave = sdat * 0.335;
    return (wave * (clamp((posy - 120) * 0.35, 0, posy) * swayamount));
}
float TreeWaveZ(float posy, float posx)
{
    if (g_xFrame_TreeWind <= 0)
        return (0);
    const float swayspeed = g_xFrame_TreeWind * 6.0; // (0.85)
    const float swayamount = g_xFrame_TreeWind * 0.20; //0.055
    const float time = g_xFrame_Time;
    const float sdat = sin((time * swayspeed) + posx) + cos((time * (swayspeed * 1.5)) + posx);
    const float wave = sdat * 0.5;
    return (wave * (clamp((posy - 120) * 0.35, 0, posy) * swayamount));
}
#endif
#endif // GGTREES_CONSTANTS_FULL_DECL