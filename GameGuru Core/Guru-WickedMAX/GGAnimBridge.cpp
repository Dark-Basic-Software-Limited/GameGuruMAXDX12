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

extern bool bEnableAnimationCulling;
extern bool bEnable30FpsAnimations;

// --- Internal storage ---

// P4: Map from secondary anim entity -> primary anim entity
static std::unordered_map<Entity, Entity> g_AnimSyncMap;

// P5: Map from bone entity -> preframe override data
static std::unordered_map<Entity, GGPreFrame> g_PreFrameMap;

// P8: Map from anim entity -> object entity (for visibility culling)
static std::unordered_map<Entity, Entity> g_AnimObjectMap;

// P9: Global frame counter for alternating-frame throttle
static uint32_t g_iAnimFrameCounter = 0;

// P8: Track animations temporarily paused for culling (restored in PostUpdate)
static std::vector<wi::ecs::Entity> g_CullPausedAnims;

// Bip01 X/Z zeroing: saved original keyframe values per AnimationDataComponent entity
struct SavedBip01XZ
{
	std::vector<float> savedX;
	std::vector<float> savedZ;
};
static std::unordered_map<Entity, SavedBip01XZ> g_Bip01XZSaved;

// Bip01 rotation zeroing: saved original quaternion keyframe values per AnimationDataComponent entity
struct SavedBip01Rot
{
	std::vector<float> savedData; // all 4 floats per key (x,y,z,w) flattened
};
static std::unordered_map<Entity, SavedBip01Rot> g_Bip01RotSaved;

// Additive rotation override (head/spine tracking): saved original rotation keyframe values
struct SavedAdditiveRot
{
	std::vector<float> savedData; // original quaternion keyframes (x,y,z,w) flattened
};
static std::unordered_map<Entity, SavedAdditiveRot> g_AdditiveRotSaved;
// Track which data entities were modified this frame (for PostUpdate restoration)
static std::vector<Entity> g_AdditiveRotModifiedThisFrame;


// ============================================================
// P0 + P1: Load-time setup
// ============================================================

void GGAnimBridge_OnLoadObject(Scene* scene, Entity animEntity)
{
	// P0: Convert backwards_compatibility_data to AnimationDataComponent entities.
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
			sampler.backwards_compatibility_data.keyframe_times.clear();
			sampler.backwards_compatibility_data.keyframe_data.clear();
		}
	}

	// P1: Set GG default animation speed (convention: speed=50).
	anim->speed = 50.0f;
}


// ============================================================
// P2: UpdateOnce workaround
// ============================================================

void GGAnimBridge_SetUpdateOnce(AnimationComponent* anim)
{
	// Force one animation evaluation on a stopped animation by making
	// last_update_time differ from timer. The new engine's skip condition:
	//   if (!IsPlaying() && last_update_time == timer) continue;
	if (anim != nullptr)
	{
		anim->last_update_time = anim->timer - 1.0f;
	}
}


// ============================================================
// P4: Primary/Secondary animation sync
// ============================================================

void GGAnimBridge_SetPrimaryAnimSync(Entity secondaryAnimEntity, Entity primaryAnimEntity)
{
	if (secondaryAnimEntity != INVALID_ENTITY && primaryAnimEntity != INVALID_ENTITY)
		g_AnimSyncMap[secondaryAnimEntity] = primaryAnimEntity;
}

void GGAnimBridge_ClearPrimaryAnimSync(Entity secondaryAnimEntity)
{
	g_AnimSyncMap.erase(secondaryAnimEntity);
}


// ============================================================
// P5: PreFrame bone overrides
// ============================================================

void GGAnimBridge_SetPreFrame(Entity boneEntity, int iMode, float fSmooth,
	const XMFLOAT3& translation, const XMFLOAT4& rotation, const XMFLOAT3& scale)
{
	if (boneEntity == INVALID_ENTITY) return;
	GGPreFrame& pf = g_PreFrameMap[boneEntity];
	pf.boneEntity = boneEntity;
	pf.iUsePreFrame = iMode;
	pf.fSmoothAmount = fSmooth;
	pf.vPreFrameTranslation = translation;
	pf.qPreFrameRotation = rotation;
	pf.vPreFrameScale = scale;
}

void GGAnimBridge_ClearPreFrame(Entity boneEntity)
{
	g_PreFrameMap.erase(boneEntity);
}


// ============================================================
// P8: Anim-to-object entity linkage (for visibility culling)
// ============================================================

void GGAnimBridge_SetAnimObjectLink(Entity animEntity, Entity objectEntity)
{
	if (animEntity != INVALID_ENTITY && objectEntity != INVALID_ENTITY)
		g_AnimObjectMap[animEntity] = objectEntity;
}

void GGAnimBridge_ClearAnimObjectLink(Entity animEntity)
{
	g_AnimObjectMap.erase(animEntity);
}


// ============================================================
// Bip01 translation X/Z zeroing in animation keyframe data
// ============================================================

void GGAnimBridge_ZeroBip01TranslationXZ(Scene* scene, Entity animEntity, int samplerIndex)
{
	AnimationComponent* anim = scene->animations.GetComponent(animEntity);
	if (!anim || samplerIndex < 0 || samplerIndex >= (int)anim->samplers.size())
		return;

	Entity dataEntity = anim->samplers[samplerIndex].data;
	if (dataEntity == INVALID_ENTITY)
		return;

	AnimationDataComponent* data = scene->animation_datas.GetComponent(dataEntity);
	if (!data)
		return;

	// Already zeroed? (idempotent -- safe to call every frame)
	if (g_Bip01XZSaved.count(dataEntity))
		return;

	size_t numKeys = data->keyframe_times.size();
	if (numKeys == 0 || data->keyframe_data.size() < numKeys * 3)
		return;

	// Save original X/Z and zero them (keep Y for walk bounce)
	SavedBip01XZ& saved = g_Bip01XZSaved[dataEntity];
	saved.savedX.resize(numKeys);
	saved.savedZ.resize(numKeys);
	for (size_t i = 0; i < numKeys; i++)
	{
		saved.savedX[i] = data->keyframe_data[i * 3 + 0];
		saved.savedZ[i] = data->keyframe_data[i * 3 + 2];
		data->keyframe_data[i * 3 + 0] = 0.0f;
		data->keyframe_data[i * 3 + 2] = 0.0f;
	}
}

void GGAnimBridge_RestoreBip01TranslationXZ(Scene* scene, Entity animEntity, int samplerIndex)
{
	AnimationComponent* anim = scene->animations.GetComponent(animEntity);
	if (!anim || samplerIndex < 0 || samplerIndex >= (int)anim->samplers.size())
		return;

	Entity dataEntity = anim->samplers[samplerIndex].data;
	if (dataEntity == INVALID_ENTITY)
		return;

	AnimationDataComponent* data = scene->animation_datas.GetComponent(dataEntity);
	if (!data)
		return;

	auto it = g_Bip01XZSaved.find(dataEntity);
	if (it == g_Bip01XZSaved.end())
		return;

	SavedBip01XZ& saved = it->second;
	size_t numKeys = saved.savedX.size();
	if (numKeys > data->keyframe_times.size())
		numKeys = data->keyframe_times.size();
	for (size_t i = 0; i < numKeys; i++)
	{
		data->keyframe_data[i * 3 + 0] = saved.savedX[i];
		data->keyframe_data[i * 3 + 2] = saved.savedZ[i];
	}
	g_Bip01XZSaved.erase(it);
}


// ============================================================
// Bip01 rotation zeroing in animation keyframe data
// ============================================================

void GGAnimBridge_ZeroBip01Rotation(Scene* scene, Entity animEntity, int samplerIndex, XMFLOAT4* pOutBaseRotation)
{
	AnimationComponent* anim = scene->animations.GetComponent(animEntity);
	if (!anim || samplerIndex < 0 || samplerIndex >= (int)anim->samplers.size())
		return;

	Entity dataEntity = anim->samplers[samplerIndex].data;
	if (dataEntity == INVALID_ENTITY)
		return;

	AnimationDataComponent* data = scene->animation_datas.GetComponent(dataEntity);
	if (!data)
		return;

	// Already frozen? (idempotent -- safe to call every frame)
	// Still output the base rotation if requested
	if (g_Bip01RotSaved.count(dataEntity))
	{
		if (pOutBaseRotation && data->keyframe_data.size() >= 4)
			*pOutBaseRotation = XMFLOAT4(data->keyframe_data[0], data->keyframe_data[1], data->keyframe_data[2], data->keyframe_data[3]);
		return;
	}

	size_t numKeys = data->keyframe_times.size();
	if (numKeys == 0 || data->keyframe_data.size() < numKeys * 4)
		return;

	// Save original quaternion keyframes and freeze all to the first keyframe's
	// rotation. This preserves the base model orientation (so the character faces
	// forward) while eliminating per-keyframe variation that causes rotation snaps
	// during animation transitions (e.g. walk -> idle).
	float baseX = data->keyframe_data[0];
	float baseY = data->keyframe_data[1];
	float baseZ = data->keyframe_data[2];
	float baseW = data->keyframe_data[3];
	if (pOutBaseRotation)
		*pOutBaseRotation = XMFLOAT4(baseX, baseY, baseZ, baseW);
	SavedBip01Rot& saved = g_Bip01RotSaved[dataEntity];
	saved.savedData.resize(numKeys * 4);
	for (size_t i = 0; i < numKeys; i++)
	{
		saved.savedData[i * 4 + 0] = data->keyframe_data[i * 4 + 0];
		saved.savedData[i * 4 + 1] = data->keyframe_data[i * 4 + 1];
		saved.savedData[i * 4 + 2] = data->keyframe_data[i * 4 + 2];
		saved.savedData[i * 4 + 3] = data->keyframe_data[i * 4 + 3];
		data->keyframe_data[i * 4 + 0] = baseX;
		data->keyframe_data[i * 4 + 1] = baseY;
		data->keyframe_data[i * 4 + 2] = baseZ;
		data->keyframe_data[i * 4 + 3] = baseW;
	}
}

void GGAnimBridge_RestoreBip01Rotation(Scene* scene, Entity animEntity, int samplerIndex)
{
	AnimationComponent* anim = scene->animations.GetComponent(animEntity);
	if (!anim || samplerIndex < 0 || samplerIndex >= (int)anim->samplers.size())
		return;

	Entity dataEntity = anim->samplers[samplerIndex].data;
	if (dataEntity == INVALID_ENTITY)
		return;

	AnimationDataComponent* data = scene->animation_datas.GetComponent(dataEntity);
	if (!data)
		return;

	auto it = g_Bip01RotSaved.find(dataEntity);
	if (it == g_Bip01RotSaved.end())
		return;

	SavedBip01Rot& saved = it->second;
	size_t numKeys = saved.savedData.size() / 4;
	if (numKeys > data->keyframe_times.size())
		numKeys = data->keyframe_times.size();
	for (size_t i = 0; i < numKeys; i++)
	{
		data->keyframe_data[i * 4 + 0] = saved.savedData[i * 4 + 0];
		data->keyframe_data[i * 4 + 1] = saved.savedData[i * 4 + 1];
		data->keyframe_data[i * 4 + 2] = saved.savedData[i * 4 + 2];
		data->keyframe_data[i * 4 + 3] = saved.savedData[i * 4 + 3];
	}
	g_Bip01RotSaved.erase(it);
}


// ============================================================
// Additive rotation override (head/spine tracking)
// Modifies rotation keyframes directly so the animation system
// evaluates them and the armature picks up the result in the
// normal single pass -- no need to re-run scene systems.
// ============================================================

void GGAnimBridge_ApplyAdditiveRotation(Scene* scene, Entity animEntity, int samplerIndex, const XMFLOAT4& additiveRot)
{
	AnimationComponent* anim = scene->animations.GetComponent(animEntity);
	if (!anim || samplerIndex < 0 || samplerIndex >= (int)anim->samplers.size())
		return;

	Entity dataEntity = anim->samplers[samplerIndex].data;
	if (dataEntity == INVALID_ENTITY)
		return;

	AnimationDataComponent* data = scene->animation_datas.GetComponent(dataEntity);
	if (!data)
		return;

	size_t numKeys = data->keyframe_times.size();
	if (numKeys == 0 || data->keyframe_data.size() < numKeys * 4)
		return;

	auto it = g_AdditiveRotSaved.find(dataEntity);
	if (it == g_AdditiveRotSaved.end())
	{
		// First call: save original keyframes
		SavedAdditiveRot& saved = g_AdditiveRotSaved[dataEntity];
		saved.savedData.assign(data->keyframe_data.begin(), data->keyframe_data.begin() + numKeys * 4);
	}
	else
	{
		// Restore originals before re-applying (prevents accumulation across frames)
		SavedAdditiveRot& saved = it->second;
		size_t count = std::min(saved.savedData.size(), numKeys * 4);
		for (size_t i = 0; i < count; i++)
			data->keyframe_data[i] = saved.savedData[i];
	}

	// Apply additive rotation to every keyframe: result = additiveRot * keyframeRot
	// slerp is equivariant under left-multiplication, so interpolation between
	// modified keyframes produces the same result as additive-after-interpolation.
	XMVECTOR addRot = XMLoadFloat4(&additiveRot);
	for (size_t i = 0; i < numKeys; i++)
	{
		XMFLOAT4 q(data->keyframe_data[i * 4 + 0], data->keyframe_data[i * 4 + 1],
			data->keyframe_data[i * 4 + 2], data->keyframe_data[i * 4 + 3]);
		XMVECTOR kfRot = XMLoadFloat4(&q);
		XMVECTOR result = XMQuaternionNormalize(XMQuaternionMultiply(addRot, kfRot));
		XMStoreFloat4(&q, result);
		data->keyframe_data[i * 4 + 0] = q.x;
		data->keyframe_data[i * 4 + 1] = q.y;
		data->keyframe_data[i * 4 + 2] = q.z;
		data->keyframe_data[i * 4 + 3] = q.w;
	}

	// Track for PostUpdate restoration
	g_AdditiveRotModifiedThisFrame.push_back(dataEntity);
}


// ============================================================
// PreUpdate: runs before scene->Update(dt)
// ============================================================

void GGAnimBridge_PreUpdate(Scene* scene, float dt)
{
	g_iAnimFrameCounter++;

	for (size_t i = 0; i < scene->animations.GetCount(); ++i)
	{
		AnimationComponent& anim = scene->animations[i];
		Entity animEntity = scene->animations.GetEntity(i);

		// P7: Pre-wrap looping timers that were advanced past end by the
		// previous frame's scene->Update. The new engine evaluates bones at
		// the current timer, THEN snaps to start. If the timer exceeds end,
		// bones are clamped to the end-of-cycle pose causing a visual jerk.
		// By wrapping here (before this frame's scene->Update), bones are
		// evaluated at the correct wrapped position -- matching the old
		// GGREDUCED subtraction-wrapping behavior.
		if (anim.IsLooped() && anim.IsPlaying() && anim.speed > 0)
		{
			float length = anim.end - anim.start;
			if (length > 0 && anim.timer > anim.end)
			{
				float excess = anim.timer - anim.start;
				anim.timer = anim.start + fmodf(excess, length);
			}
		}

		// P4: Sync secondary animations from their linked primary
		auto itSync = g_AnimSyncMap.find(animEntity);
		if (itSync != g_AnimSyncMap.end())
		{
			AnimationComponent* primary = scene->animations.GetComponent(itSync->second);
			if (primary != nullptr)
			{
				anim.timer = primary->timer;
				anim.speed = primary->speed;
				anim.amount = primary->amount;
			}
		}

		// P3: Auto-lerp animation.amount toward 1.0
		//
		// The old GGREDUCED code ran this lerp once per CHANNEL (not per animation).
		// With speed=50 as default, the speed > 1.5 branch always triggered (rate 0.0005).
		// A typical character has ~50 channels, so the effective per-animation rate was
		// approximately 1-(1-0.0005)^50 = ~0.025. We approximate this with a single
		// higher rate per animation, scaled by channel count for accuracy.
		if (anim.amount < 1.0f)
		{
			float fBaseRate = 0.0005f; // old per-channel rate (speed > 1.5 always true at speed=50)
			size_t nChannels = anim.channels.size();
			if (nChannels < 1) nChannels = 1;
			float fRate = 1.0f - powf(1.0f - fBaseRate, (float)nChannels);
			anim.amount = wi::math::Lerp(anim.amount, 1.0f, fRate);
			if (anim.amount > 0.9999f) anim.amount = 1.0f;
		}

		// P8: Pause animations for culled/occluded objects
		if (bEnableAnimationCulling || bEnable30FpsAnimations)
		{
			auto itObj = g_AnimObjectMap.find(animEntity);
			if (itObj != g_AnimObjectMap.end())
			{
				ObjectComponent* obj = scene->objects.GetComponent(itObj->second);
				if (obj != nullptr && anim.IsPlaying())
				{
					bool bShouldCull = false;

					// Occlusion/visibility culling
					if (bEnableAnimationCulling && !obj->IsRenderable())
					{
						bShouldCull = true;
					}
					// 30fps throttle: skip every other frame
					else if (bEnable30FpsAnimations)
					{
						if (((g_iAnimFrameCounter + (uint32_t)i) % 2) != 0)
							bShouldCull = true;
					}

					if (bShouldCull)
					{
						// Temporarily pause + match timer so Wicked's skip triggers:
						//   !IsPlaying() && last_update_time == timer -> skip
						anim.Pause();
						anim.last_update_time = anim.timer;
						g_CullPausedAnims.push_back(animEntity);
					}
				}
			}
		}

	}
}


// ============================================================
// PostUpdate: runs after scene->Update(dt)
// ============================================================

void GGAnimBridge_PostUpdate(Scene* scene)
{
	// P8: Restore animations that were temporarily paused for culling
	for (Entity animEntity : g_CullPausedAnims)
	{
		AnimationComponent* anim = scene->animations.GetComponent(animEntity);
		if (anim != nullptr)
			anim->Play();
	}
	g_CullPausedAnims.clear();

	// P5: Apply preframe bone overrides after animation has run
	// Note: Mode 1 (additive head/spine) is now handled via keyframe modification
	// in GGAnimBridge_ApplyAdditiveRotation, which runs before Scene::Update so the
	// armature picks up the result in the normal single pass. Modes 2, 3, 10000+
	// remain here for transforms that don't need armature propagation (mode 3 matches
	// frozen keyframes; modes 2/10000+ are cosmetic overrides).
	for (auto& pair : g_PreFrameMap)
	{
		const GGPreFrame& pf = pair.second;
		if (pf.iUsePreFrame == 0) continue;

		TransformComponent* transform = scene->transforms.GetComponent(pf.boneEntity);
		if (transform == nullptr) continue;

		if (pf.iUsePreFrame == 2)
		{
			// Mode 2: Replace entirely (mouth shapes, blink)
			float t = pf.fSmoothAmount;
			XMVECTOR curTrans = XMLoadFloat3(&transform->translation_local);
			XMVECTOR preTrans = XMLoadFloat3(&pf.vPreFrameTranslation);
			XMVECTOR curRot = XMLoadFloat4(&transform->rotation_local);
			XMVECTOR preRot = XMLoadFloat4(&pf.qPreFrameRotation);

			XMStoreFloat3(&transform->translation_local, XMVectorLerp(curTrans, preTrans, t));
			XMStoreFloat4(&transform->rotation_local, XMQuaternionNormalize(XMQuaternionSlerp(curRot, preRot, t)));
			transform->SetDirty();
		}
		else if (pf.iUsePreFrame == 3)
		{
			// Mode 3: Replace translation (keeping Y) and rotation
			float t = pf.fSmoothAmount;
			XMVECTOR curRot = XMLoadFloat4(&transform->rotation_local);
			XMVECTOR preRot = XMLoadFloat4(&pf.qPreFrameRotation);
			XMStoreFloat4(&transform->rotation_local, XMQuaternionNormalize(XMQuaternionSlerp(curRot, preRot, t)));

			// Replace X and Z translation, keep Y
			XMFLOAT3 preTrans = pf.vPreFrameTranslation;
			transform->translation_local.x = wi::math::Lerp(transform->translation_local.x, preTrans.x, t);
			transform->translation_local.z = wi::math::Lerp(transform->translation_local.z, preTrans.z, t);
			transform->SetDirty();
		}
		else if (pf.iUsePreFrame >= 10000)
		{
			// Mode 10000+: Snap to specific keyframe (rotation only)
			float t = pf.fSmoothAmount;
			XMVECTOR curRot = XMLoadFloat4(&transform->rotation_local);
			XMVECTOR preRot = XMLoadFloat4(&pf.qPreFrameRotation);
			XMStoreFloat4(&transform->rotation_local, XMQuaternionNormalize(XMQuaternionSlerp(curRot, preRot, t)));
			transform->SetDirty();
		}
	}

	// Restore additive rotation keyframes that were modified this frame.
	// The animation system has already evaluated them; now restore originals
	// so the next frame's ApplyAdditiveRotation works on clean data.
	for (Entity dataEntity : g_AdditiveRotModifiedThisFrame)
	{
		auto it = g_AdditiveRotSaved.find(dataEntity);
		if (it == g_AdditiveRotSaved.end()) continue;

		AnimationDataComponent* data = scene->animation_datas.GetComponent(dataEntity);
		if (!data) { g_AdditiveRotSaved.erase(it); continue; }

		SavedAdditiveRot& saved = it->second;
		size_t count = std::min(saved.savedData.size(), data->keyframe_data.size());
		for (size_t i = 0; i < count; i++)
			data->keyframe_data[i] = saved.savedData[i];
	}
	g_AdditiveRotModifiedThisFrame.clear();
}
