//
// GG Animation Bridge Layer
//
// Reimplements animation behaviors that were previously embedded in WickedEngine
// via #ifdef GGREDUCED. All code here uses only WickedEngine's public API so
// that WickedEngineDX12 remains an unmodified upstream fork.
//
#include "stdafx.h"
#include "GGAnimBridge.h"

using namespace wi::scene;
using namespace wi::ecs;

void GGAnimBridge_OnLoadObject(Scene* scene, Entity animEntity)
{
	// P0: Convert backwards_compatibility_data to AnimationDataComponent entities.
	//
	// GameGuru builds animations programmatically and stores keyframe data in
	// sampler.backwards_compatibility_data, leaving sampler.data as INVALID_ENTITY.
	// The new WickedEngine only auto-converts during archive deserialization (a code
	// path GG never uses). Without this, RunAnimationUpdateSystem skips every channel
	// because it can't find an AnimationDataComponent.
	AnimationComponent* anim = scene->animations.GetComponent(animEntity);
	if (anim == nullptr)
		return;

	for (size_t i = 0; i < anim->samplers.size(); ++i)
	{
		auto& sampler = anim->samplers[i];
		if (sampler.data == INVALID_ENTITY &&
			!sampler.backwards_compatibility_data.keyframe_times.empty())
		{
			Entity dataEntity = CreateEntity();
			scene->animation_datas.Create(dataEntity) = sampler.backwards_compatibility_data;
			sampler.data = dataEntity;

			// free the temporary data now that it lives in a proper component
			sampler.backwards_compatibility_data.keyframe_times.clear();
			sampler.backwards_compatibility_data.keyframe_data.clear();
		}
	}

	// P1: Set GG default animation speed.
	//
	// Old WickedRepo (GGREDUCED) defaulted AnimationComponent::speed to 50.
	// GameGuru's entire animation timing system was built around this convention.
	// The new engine defaults to speed=1, so animations would play at 1/50th rate.
	// WickedCall_SetObjectSpeed already multiplies by 50 for runtime changes,
	// but the initial default must also be set here at load time.
	anim->speed = 50.0f;
}

void GGAnimBridge_PreUpdate(Scene* scene, float dt)
{
	// Future: P3 amount lerp, P4 timer sync, P6 dt cap, P8/P9 culling
}

void GGAnimBridge_PostUpdate(Scene* scene)
{
	// Future: P5 preframe bone overrides, P7 loop wrap fixup
}
