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
	// maxMilliseconds. Called at the END of a level load, while the loading
	// screen is still up — so the user never watches the radial chunk
	// construction (DX11 built its terrain instantly). Direction = camera look
	// vector; a rough value is fine (the view-cone filter is ~70 degrees wide).
	void GGTerrainWicked_Pregenerate(float camX, float camY, float camZ,
		float dirX, float dirY, float dirZ, int maxMilliseconds);

	// Brush cursor: project a green ring onto the Wicked terrain at the current paint position.
	// Mirrors the legacy GG terrain-shader procedural circle. Call once per frame from the existing
	// raycast block in GGTerrain_part0.cpp with the pick result + brush radius. Pass visible=false
	// (or size<=0) to hide. `size` is the brush RADIUS in world units (matches the legacy `brushSize`).
	void GGTerrainWicked_SetBrushCursor(bool visible, float x, float y, float z, float size);
}

#endif
