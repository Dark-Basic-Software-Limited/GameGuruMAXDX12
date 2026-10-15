#pragma once
// =============================================================================================
// GGMAX 3.114 - FAR TERRAIN: the DX11 horizon.
//
// Lee: DX11 shows a distant mountain range from the editor's default view that DX12 does not, and
// "we need as close to 100% parity with the DX11 experience if we are to win over existing users".
// The live Wicked terrain is a camera-centred ring of 25x25 chunks that ends ~63,360 units out
// (1.6 km); DX11 drew terrain to its 500,000-unit far plane (12.7 km). Raising the ring cannot get
// there - it is quadratic: the setup.ini maximum (24) reaches only 3.2 km for ~3 ms/frame and the
// whole 4 GB VRAM headroom, and parity would need ~36,000 chunk entities.
//
// But DX12 still generates DX11's own terrain every frame: the legacy GGTerrain clipmap (9 LOD
// levels x 64 chunks, vertex spacing doubling per level, camera-following, stitched, ~58 MB of
// already-resident buffers, same height function as the Wicked chunks) - only its DRAW was gated
// off. This module draws that clipmap's outer levels everywhere the live (or baked) chunks are not:
// no new geometry, no new generation cost, about 50-70 extra draws. See GGTerrainFarCommon.hlsli.
// =============================================================================================

#include "wiGraphicsDevice.h"
#include "wiPrimitive.h"

extern bool  gg_terrain_far;          // the switch: setup.ini terrainfar, harness SET_TERRAINFAR (default ON)
extern bool  gg_terrain_far_trees;    // tree billboards reach as far as the far field (setup.ini terrainfartrees)
extern float gg_terrain_far_sink;     // border sink, x segment size (harness SET_TERRAINFAR_SINK)
extern int   gg_terrain_far_debug;    // 0 off, 1 tint, 2 coverage, 3 clipmap levels (SET_TERRAINFAR_DEBUG)
extern bool  gg_terrain_far_reflect;  // draw it into the planar water reflection too (SET_TERRAINFAR_REFLECT)

void  GGTerrainFar_Init();             // STARTUP, beside GGTerrainBake_Init (shaders are a relative path)
void  GGTerrainFar_RequestUpdate();    // MAIN thread: the terrain block ran this frame (from the bridge site)
void  GGTerrainFar_Update();           // MAIN thread, every frame, AFTER Scene::Update (__super::Update)
void  GGTerrainFar_DrawPrepass( const wi::primitive::Frustum* frustum, wi::graphics::CommandList cmd, bool reflection );
void  GGTerrainFar_Draw( const wi::primitive::Frustum* frustum, int mode, wi::graphics::CommandList cmd );
float GGTerrainFar_GetReach();         // how far the far field reaches from the camera; 0 = not drawing
const char* GGTerrainFar_Report();     // harness TERRAIN_FAR

namespace GGTerrain
{
	// One drawable chunk of the legacy clipmap's CURRENT LOD set (GGTerrain_part0.cpp).
	struct GGFarChunkRef
	{
		const wi::graphics::GPUBuffer* vb;
		const wi::graphics::GPUBuffer* ib;
		wi::primitive::AABB aabb;
		uint32_t lod;
		uint32_t numIndices;            // triangle strip
	};
	// Fills `out` with the chunks the legacy draw itself would have drawn (same visibility rule);
	// returns how many. *pReach = half-width of the outermost level, *pSegBase = LOD 0 vertex spacing.
	int GGTerrain_GetFarDrawChunks( GGFarChunkRef* out, int maxOut, float* pReach, float* pSegBase, uint32_t* pLevels );
}
