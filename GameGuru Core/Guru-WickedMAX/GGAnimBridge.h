#pragma once
//
// GG Animation Bridge Layer
//
// Reimplements animation behaviors that were previously embedded in WickedEngine
// via #ifdef GGREDUCED. All code here uses only WickedEngine's public API so
// that WickedEngineDX12 remains an unmodified upstream fork.
//

#include "../../../WickedEngineDX12/WickedEngine/WickedEngine.h"
#include <unordered_map>

// --- P4: Primary/Secondary animation sync data ---
struct GGAnimExtra
{
	wi::ecs::Entity animEntity = wi::ecs::INVALID_ENTITY;
	wi::ecs::Entity primaryAnimEntity = wi::ecs::INVALID_ENTITY; // if non-INVALID, sync from this primary
	wi::ecs::Entity objectEntity = wi::ecs::INVALID_ENTITY;      // for visibility culling [P8]
};

// --- P5: PreFrame bone override data ---
struct GGPreFrame
{
	wi::ecs::Entity boneEntity = wi::ecs::INVALID_ENTITY;
	int iUsePreFrame = 0;               // mode 0/1/2/3/10000+
	float fSmoothAmount = 1.0f;
	XMFLOAT3 vPreFrameTranslation = XMFLOAT3(0, 0, 0);
	XMFLOAT4 qPreFrameRotation = XMFLOAT4(0, 0, 0, 1);
	XMFLOAT3 vPreFrameScale = XMFLOAT3(1, 1, 1);
};

// Key: animation channel target entity. Value: preframe override data.
// Managed by WickedCall_RotateLimb, OverrideLimbWithCombined, etc.

// --- Public API ---

// Called at object load time (from WickedCall_RefreshObjectAnimations)
void GGAnimBridge_OnLoadObject(wi::scene::Scene* scene, wi::ecs::Entity animEntity);

// Called after setting timer on a stopped animation to force one evaluation [P2]
void GGAnimBridge_SetUpdateOnce(wi::scene::AnimationComponent* anim);

// P4: Register a secondary animation to sync from a primary
void GGAnimBridge_SetPrimaryAnimSync(wi::ecs::Entity secondaryAnimEntity, wi::ecs::Entity primaryAnimEntity);

// P4: Remove primary/secondary sync linkage
void GGAnimBridge_ClearPrimaryAnimSync(wi::ecs::Entity secondaryAnimEntity);

// P5: Set preframe bone override on a specific bone entity
void GGAnimBridge_SetPreFrame(wi::ecs::Entity boneEntity, int iMode, float fSmooth,
	const XMFLOAT3& translation, const XMFLOAT4& rotation, const XMFLOAT3& scale);

// P5: Clear preframe bone override
void GGAnimBridge_ClearPreFrame(wi::ecs::Entity boneEntity);

// P8: Associate an animation entity with an object entity for visibility culling
void GGAnimBridge_SetAnimObjectLink(wi::ecs::Entity animEntity, wi::ecs::Entity objectEntity);

// P8: Remove animation-to-object link (call before removing animation entity)
void GGAnimBridge_ClearAnimObjectLink(wi::ecs::Entity animEntity);

// Zero Bip01 translation X/Z in animation keyframe data so the engine's normal
// pipeline produces (0, Y, 0) -- prevents double-movement from bone drift + root motion.
// Saves original values for restoration. Safe to call every frame (idempotent).
void GGAnimBridge_ZeroBip01TranslationXZ(wi::scene::Scene* scene, wi::ecs::Entity animEntity, int samplerIndex);

// Restore original Bip01 translation X/Z keyframe data (undo the zeroing above).
void GGAnimBridge_RestoreBip01TranslationXZ(wi::scene::Scene* scene, wi::ecs::Entity animEntity, int samplerIndex);

// Freeze Bip01 rotation keyframes to the first keyframe's value so the engine's
// pipeline produces a constant rotation -- prevents per-keyframe variation from
// causing rotation snaps during animation transitions (e.g. walk -> idle).
// Saves original values for restoration. Safe to call every frame (idempotent).
// If pOutBaseRotation is non-null, stores the captured base rotation quaternion.
void GGAnimBridge_ZeroBip01Rotation(wi::scene::Scene* scene, wi::ecs::Entity animEntity, int samplerIndex, XMFLOAT4* pOutBaseRotation = nullptr);

// Restore original Bip01 rotation keyframe data (undo the zeroing above).
void GGAnimBridge_RestoreBip01Rotation(wi::scene::Scene* scene, wi::ecs::Entity animEntity, int samplerIndex);

// Apply additive rotation to a bone's rotation keyframes. Called from game logic
// (before Scene::Update) so the animation system evaluates modified keyframes and
// the armature picks up the correct rotation in the normal single pass.
// Originals are saved and restored automatically in PostUpdate.
void GGAnimBridge_ApplyAdditiveRotation(wi::scene::Scene* scene, wi::ecs::Entity animEntity, int samplerIndex, const XMFLOAT4& additiveRot);

// Called each frame before scene->Update(dt) (from MasterRenderer::Update)
void GGAnimBridge_PreUpdate(wi::scene::Scene* scene, float dt);

// Called each frame after scene->Update(dt) (from MasterRenderer::PostUpdate)
void GGAnimBridge_PostUpdate(wi::scene::Scene* scene);
