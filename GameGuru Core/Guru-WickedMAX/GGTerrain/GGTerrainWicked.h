#ifndef _H_GGTERRAINWICKED
#define _H_GGTERRAINWICKED

#include "../../../../WickedEngineDX12/WickedEngine/wiScene.h"

namespace GGTerrain
{
	void GGTerrainWicked_Init();
	void GGTerrainWicked_Update(const wi::scene::CameraComponent& camera);
	void GGTerrainWicked_Shutdown();
	void GGTerrainWicked_OnPaintDataChanged();

	// Synchronously pre-build the terrain chunks the given camera can see, up to
	// maxMilliseconds. CURRENTLY UNCALLED (2026-07-18): calling this at the end
	// of a level load baked WRONG heights into every chunk — the legacy GG
	// terrain computes heights via readback over the FRAMES following the load
	// (the reason iDelayedCameraRestore waits 240 frames), so no heights exist
	// yet at load end. Do NOT re-wire this until it is gated on a genuine
	// heights-ready signal (which requires editor frames to run — i.e. a
	// hold-the-loading-overlay approach, not a synchronous block).
	void GGTerrainWicked_Pregenerate(float camX, float camY, float camZ,
		float dirX, float dirY, float dirZ, int maxMilliseconds);

	// Brush cursor: project a green ring onto the Wicked terrain at the current paint position.
	// Mirrors the legacy GG terrain-shader procedural circle. Call once per frame from the existing
	// raycast block in GGTerrain_part0.cpp with the pick result + brush radius. Pass visible=false
	// (or size<=0) to hide. `size` is the brush RADIUS in world units (matches the legacy `brushSize`).
	void GGTerrainWicked_SetBrushCursor(bool visible, float x, float y, float z, float size);
}

#endif
