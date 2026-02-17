#pragma once
//
// GG Animation Bridge Layer
//
// Reimplements animation behaviors that were previously embedded in WickedEngine
// via #ifdef GGREDUCED. All code here uses only WickedEngine's public API so
// that WickedEngineDX12 remains an unmodified upstream fork.
//

#include "../../../WickedEngineDX12/WickedEngine/WickedEngine.h"

// Called at object load time (from WickedCall_RefreshObjectAnimations)
// - Creates AnimationDataComponent entities from backwards_compatibility_data  [P0]
// - Sets animation speed to GG convention (speed=50)                           [P1]
void GGAnimBridge_OnLoadObject(wi::scene::Scene* scene, wi::ecs::Entity animEntity);

// Called each frame before scene->Update(dt) (from MasterRenderer::Update)
// Future home of: dt capping [P6], timer sync [P4], amount lerp [P3], culling [P8/P9]
void GGAnimBridge_PreUpdate(wi::scene::Scene* scene, float dt);

// Called each frame after scene->Update(dt) (from MasterRenderer::PostUpdate)
// Future home of: preframe bone overrides [P5], loop wrap fixup [P7]
void GGAnimBridge_PostUpdate(wi::scene::Scene* scene);
