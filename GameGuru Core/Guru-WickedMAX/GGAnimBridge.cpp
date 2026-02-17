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

// --- Internal storage ---

// P4: Map from secondary anim entity -> primary anim entity
static std::unordered_map<Entity, Entity> g_AnimSyncMap;

// P5: Map from bone entity -> preframe override data
static std::unordered_map<Entity, GGPreFrame> g_PreFrameMap;

// P7: Map from anim entity -> previous frame timer (for loop wrap detection)
static std::unordered_map<Entity, float> g_PrevTimerMap;

// P8: Map from anim entity -> object entity (for visibility culling)
static std::unordered_map<Entity, Entity> g_AnimObjectMap;

// P9: Global frame counter for alternating-frame throttle
static uint32_t g_iAnimFrameCounter = 0;


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
		if (anim.amount < 1.0f)
		{
			float fRate = 0.0002f;
			if (anim.speed < 0.5f) fRate = 0.0001f;
			if (anim.speed > 1.5f) fRate = 0.0005f;
			anim.amount = wi::math::Lerp(anim.amount, 1.0f, fRate);
			if (anim.amount > 0.9999f) anim.amount = 1.0f;
		}

		// P8: Pause animations for culled/occluded objects
		auto itObj = g_AnimObjectMap.find(animEntity);
		if (itObj != g_AnimObjectMap.end())
		{
			ObjectComponent* obj = scene->objects.GetComponent(itObj->second);
			if (obj != nullptr)
			{
				// Skip animation if object is not visible (but don't interfere with
				// animations that the game logic has explicitly stopped)
				if (anim.IsPlaying())
				{
					// P9: 30fps throttle -- skip every other frame per animation
					bool bThrottle = ((g_iAnimFrameCounter + (uint32_t)i) % 2) != 0;

					// If culled/occluded, always skip; if visible, apply throttle
					if (!obj->IsRenderable())
					{
						// Fully culled -- skip this frame by not advancing timer.
						// We don't pause (which resets state), just skip the update
						// by temporarily matching last_update_time.
						anim.last_update_time = anim.timer;
					}
					else if (bThrottle)
					{
						// Visible but throttled this frame
						anim.last_update_time = anim.timer;
					}
				}
			}
		}

		// P7: Store timer for loop-wrap detection in PostUpdate
		g_PrevTimerMap[animEntity] = anim.timer;
	}
}


// ============================================================
// PostUpdate: runs after scene->Update(dt)
// ============================================================

void GGAnimBridge_PostUpdate(Scene* scene)
{
	// P7: Fix loop timer wrapping -- use subtraction instead of snap
	for (size_t i = 0; i < scene->animations.GetCount(); ++i)
	{
		AnimationComponent& anim = scene->animations[i];
		Entity animEntity = scene->animations.GetEntity(i);

		if (anim.IsLooped() && anim.IsPlaying())
		{
			auto itPrev = g_PrevTimerMap.find(animEntity);
			if (itPrev != g_PrevTimerMap.end())
			{
				float prevTimer = itPrev->second;
				float length = anim.end - anim.start;
				if (length > 0)
				{
					// Detect that the engine snapped timer back to start
					// (prevTimer was near end, now timer is at start)
					if (prevTimer > anim.start + length * 0.5f && anim.timer == anim.start)
					{
						// Calculate fractional overshoot and apply it
						float overshoot = prevTimer - anim.end;
						if (overshoot > 0 && overshoot < length)
						{
							anim.timer = anim.start + overshoot;
						}
					}
				}
			}
		}
	}

	// P5: Apply preframe bone overrides after animation has run
	for (auto& pair : g_PreFrameMap)
	{
		const GGPreFrame& pf = pair.second;
		if (pf.iUsePreFrame == 0) continue;

		TransformComponent* transform = scene->transforms.GetComponent(pf.boneEntity);
		if (transform == nullptr) continue;

		if (pf.iUsePreFrame == 1)
		{
			// Mode 1: Additive blend
			XMVECTOR curTrans = XMLoadFloat3(&transform->translation_local);
			XMVECTOR preTrans = XMLoadFloat3(&pf.vPreFrameTranslation);
			XMVECTOR curRot = XMLoadFloat4(&transform->rotation_local);
			XMVECTOR preRot = XMLoadFloat4(&pf.qPreFrameRotation);
			float t = pf.fSmoothAmount;

			XMStoreFloat3(&transform->translation_local, XMVectorLerp(curTrans, curTrans + preTrans, t));
			XMStoreFloat4(&transform->rotation_local, XMQuaternionNormalize(XMQuaternionSlerp(curRot, XMQuaternionMultiply(curRot, preRot), t)));
			transform->SetDirty();
		}
		else if (pf.iUsePreFrame == 2)
		{
			// Mode 2: Replace entirely (head look-at, mouth)
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
			// The rotation is pre-looked-up and stored in qPreFrameRotation by the caller
			float t = pf.fSmoothAmount;
			XMVECTOR curRot = XMLoadFloat4(&transform->rotation_local);
			XMVECTOR preRot = XMLoadFloat4(&pf.qPreFrameRotation);
			XMStoreFloat4(&transform->rotation_local, XMQuaternionNormalize(XMQuaternionSlerp(curRot, preRot, t)));
			transform->SetDirty();
		}
	}
}
