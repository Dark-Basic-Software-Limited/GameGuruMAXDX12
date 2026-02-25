#ifndef _H_GGTERRAINWICKED
#define _H_GGTERRAINWICKED

#include "../../../../WickedEngineDX12/WickedEngine/wiScene.h"

namespace GGTerrain
{
	void GGTerrainWicked_Init();
	void GGTerrainWicked_Update(const wi::scene::CameraComponent& camera);
	void GGTerrainWicked_Shutdown();
	void GGTerrainWicked_DebugDraw(wi::graphics::CommandList cmd);
}

#endif
