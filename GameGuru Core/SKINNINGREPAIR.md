# Skinning & Animation Repair Guide

## Problem Statement

Animated characters in GameGuru MAX DX12 are not being skinned. Characters appear frozen in their bind/T-pose. The root cause is a set of missing integrations between the GameGuru animation pipeline and the new WickedEngineDX12 repo, which has undergone a major architecture rewrite compared to the old WickedRepo that GameGuru was originally built against.

The old WickedRepo contained `#ifdef GGREDUCED` custom modifications throughout its animation and skinning systems. The new WickedEngineDX12 is a clean upstream fork with none of these customizations. This document catalogs every difference, provides a prioritized fix list, and defines the architecture for implementing all fixes **without modifying WickedEngine source files**.

## Table of Contents

1. [Architecture Change Summary](#architecture-change-summary)
2. [Key Source Files](#key-source-files)
3. [GameGuru Frame Loop & Call Chain](#gameguru-frame-loop--call-chain)
4. [Implementation Architecture: GG Animation Bridge Layer](#implementation-architecture-gg-animation-bridge-layer)
5. [Priority Fix List (P0-P10)](#priority-fix-list)
6. [Reference: GGREDUCED Blocks in Old WickedRepo](#reference-ggreduced-blocks-in-old-wickedrepo)
7. [Verification Checklist](#verification-checklist)

---

## Architecture Change Summary

The skinning and animation system was completely rewritten between the old WickedRepo and the new WickedEngineDX12:

| Area | OLD (WickedRepo) | NEW (WickedEngineDX12) |
|------|-------------------|------------------------|
| Resource binding | Fixed register slots (t30-t33, u0-u1) | Fully bindless via push constants + descriptor arrays |
| Bone buffer | Per-armature `StructuredBuffer`, `UpdateBuffer()` each frame | Single global `skinningBuffer` ByteAddressBuffer, byte offsets per armature |
| Bone index+weight packing | 16-bit index + 16-bit weight (separate, 8B/influence) | 20-bit index + 12-bit weight (packed together, 4B/influence) |
| Streamout buffers | Separate `streamoutBuffer_POS` + `streamoutBuffer_TAN` created inline in `CreateRenderData()` | Single combined `streamoutBuffer` with sub-views (so_pos, so_nor, so_tan, so_pre), created by separate `CreateStreamoutRenderData()` |
| Vertex layout | Interleaved pos+normal packed (16B) | Separate typed buffers (float4 pos, half4 nor, half4 tan) |
| Skinning dispatch | Per-armature bind+dispatch, LDS variant for <128 bones | Single shader, global buffer, no LDS |
| Compute thread group | 128 threads | 64 threads |
| Morph targets | Not in skinning shader | Integrated into same compute pass |
| Normal/tangent precision | float | half |
| Animation data storage | `backwards_compatibility_data` on sampler (runtime-converted) | `AnimationDataComponent` entity (must exist before animation runs) |
| Animation execution | Single-threaded for loop, two-pass primary/secondary | Multi-threaded via `animation_queues` + `wi::jobsystem::Dispatch` |
| Bone influence count | Fixed 4 per vertex | Variable (4, 8, 12... via `influence_div4`) |

---

## Key Source Files

### GameGuru Integration Layer (where ALL fixes are applied)

| File | Role |
|------|------|
| `Guru-WickedMAX/wickedcalls.h` | Animation API declarations |
| `Guru-WickedMAX/wickedcalls_part0.cpp` | `WickedCall_AddObject` (line 1168), `WickedCall_LoadNode` (line 485), `WickedCall_RefreshObjectAnimations` (line 900) |
| `Guru-WickedMAX/wickedcalls_part2.cpp` | `WickedCall_PlayObject`, `WickedCall_StopObject`, `WickedCall_SetObjectFrame`, `WickedCall_GlueObjectToObject`, limb overrides |
| `Guru-WickedMAX/wickedcalls_part3.cpp` | `WickedCall_GetSkinable` (line 1228), `WickedCall_SetLDSSkinningEnabled` (line 1385, stub) |
| `Guru-WickedMAX/master.h` | `Master` class (inherits `wi::Application`), `MasterRenderer` class (inherits `RenderPath3D`) |
| `Guru-WickedMAX/master_part0.cpp` | `Master::Update()` (line 610), `Master::RunCustom()` (line 1345) |
| `Guru-WickedMAX/master_part1.cpp` | `MasterRenderer::Update()` (line 110) -- **the key pre-hook location** |
| `Dark Basic Public Shared/.../CObjectManagerWicked_part0.cpp` | `UpdateAnimationCycle()` (line 155, currently empty no-op) |
| **`Guru-WickedMAX/GGAnimBridge.h`** | **NEW FILE TO CREATE** -- Animation bridge layer header |
| **`Guru-WickedMAX/GGAnimBridge.cpp`** | **NEW FILE TO CREATE** -- Animation bridge layer implementation |

### New WickedEngineDX12 (reference only -- DO NOT MODIFY)

| File | Role |
|------|------|
| `WickedEngine/wiScene_Components.h` | Component structs: `ArmatureComponent`, `MeshComponent`, `AnimationComponent`, `AnimationDataComponent` |
| `WickedEngine/wiScene_Components.cpp` | `CreateRenderData()`, `CreateStreamoutRenderData()`, bone buffer packing |
| `WickedEngine/wiScene.h` | `Scene` class, skinning buffer allocation, update system declarations |
| `WickedEngine/wiScene.cpp` | `Scene::Update()` pipeline (line 34+), all `RunXxxUpdateSystem()` implementations |
| `WickedEngine/wiRenderer.cpp` | GPU skinning compute dispatch (line 5200+), buffer uploads |
| `WickedEngine/wiApplication.cpp` | `Application::Run()` frame pipeline (line 87+), calls `Update`/`Render`/`Compose` virtuals |
| `WickedEngine/wiRenderPath3D.cpp` | `RenderPath3D::Update()` (line 339) -- calls `scene->Update(dt)` at line 361 |
| `WickedEngine/shaders/skinningCS.hlsl` | Skinning compute shader (morph targets + bone transforms) |
| `WickedEngine/shaders/ShaderInterop_Renderer.h` | `SkinningPushConstants`, `ShaderTransform`, `MorphTargetGPU` structs |
| `WickedEngine/wiScene_Serializers.cpp` | `backwards_compatibility_data` conversion during archive load (lines 2742-2749) |

### Old WickedRepo (reference for GGREDUCED behavior -- located at `D:\max\WickedRepo`)

| File | Role |
|------|------|
| `WickedEngine/wiScene.h` | AnimationComponent with GGREDUCED fields: `speed=50`, `updateonce`, `UsePrimaryAnimTimer`, `iUsePreFrame` |
| `WickedEngine/wiScene.cpp` | RunAnimationUpdateSystem with GGREDUCED: runtime backwards-compat conversion, 30fps culling, primary/secondary sync, preframe overrides, amount auto-lerp |
| `WickedEngine/wiRenderer.cpp` | Skinning dispatch with GGREDUCED: `IsRenderable()` check, null armature guard, soft body removal |
| `WickedEngine/MainComponent.cpp` | Delta time capping (1/30s max) |

---

## GameGuru Frame Loop & Call Chain

Understanding the call chain is essential for knowing where to insert hooks. GameGuru uses virtual method overrides on WickedEngine's `Application` and `RenderPath3D` classes.

### Class Hierarchy

```
wi::Application (alias: MainComponent)      [WickedEngine]
  └── Master                                 [GameGuru: master.h line 24]

wi::RenderPath3D                             [WickedEngine]
  └── MasterRenderer                         [GameGuru: master.h line 9]
```

`Master` owns a `MasterRenderer masterrenderer` member. During initialization, `masterrenderer` is set as the active render path.

### Complete Per-Frame Call Chain

```
main.cpp: Win32 message loop (line 253)
  │
  └─► Master::RunCustom()                          [master_part0.cpp:1345]
        │
        └─► wi::Application::Run()                 [wiApplication.cpp:87]
              │
              ├── wi::profiler::BeginFrame()
              ├── deltaTime = timer.record_elapsed_seconds()
              ├── wi::input::Update()
              ├── activePath->PreUpdate()            [saves previous camera]
              │
              ├── Master::Update(dt)                 [master_part0.cpp:610] ◄─ VIRTUAL OVERRIDE
              │     │
              │     └─► wi::Application::Update(dt)  [wiApplication.cpp:390]
              │           │
              │           ├─► MasterRenderer::Update(dt)  [master_part1.cpp:110] ◄─ VIRTUAL OVERRIDE
              │           │     │
              │           │     ├── GuruLoopLogic()        ◄─ GG game logic runs here
              │           │     ├── gpup_update(dt)        ◄─ GPU particles
              │           │     ├── GGTerrain_Update()     ◄─ Terrain
              │           │     ├── GGTrees/GGGrass        ◄─ Vegetation
              │           │     ├── WickedCall_UpdateEmitters()
              │           │     ├── GuruLoopRender()        ◄─ ImGui prep
              │           │     │
              │           │     ├── *** PRE-HOOK POINT ***  ◄─ INSERT GGAnimBridge_PreUpdate() HERE
              │           │     │
              │           │     └── __super::Update(dt)     [line 176]
              │           │           │
              │           │           └── RenderPath3D::Update(dt)  [wiRenderPath3D.cpp:339]
              │           │                 │
              │           │                 └── scene->Update(dt * GameSpeed)  [line 361]
              │           │                       │
              │           │                       ├── RunAnimationUpdateSystem    [line 253]
              │           │                       ├── RunTransformUpdateSystem    [line 257]
              │           │                       ├── RunHierarchyUpdateSystem    [line 261]
              │           │                       ├── RunMeshUpdateSystem         [line 366]
              │           │                       ├── RunArmatureUpdateSystem     [line 380]
              │           │                       └── (20+ other subsystems)
              │           │
              │           └── activePath->PostUpdate()   ◄─ POST-HOOK POINT (override in MasterRenderer)
              │
              ├── Render()                          [passthrough to RenderPath3D::Render]
              │     ├── activePath->PreRender()      [visibility/culling]
              │     ├── activePath->Render()
              │     └── activePath->PostRender()
              │
              ├── Compose(cmd)                      [MasterRenderer::Compose at master_part1.cpp:348]
              │     ├── __super::Compose(cmd)        [2D overlays]
              │     └── ImGui_DX12_RenderBridge(cmd)
              │
              ├── wi::profiler::EndFrame(cmd)
              └── graphicsDevice->SubmitCommandLists()
```

### Scene::Update(dt) Internal Subsystem Order (NEW DX12 Engine)

From `wiScene.cpp` line 34+:

```
 1. UpdateHumanoidFacings()
 2. RunScriptUpdateSystem(ctx)
 3. RunSplineUpdateSystem(ctx)
 4. ScanAnimationDependencies()                ◄─ groups animations into parallel queues
 5. Skinning pre-scan (counts bone/morph sizes for buffer allocation)
 6. RunCharacterUpdateSystem(ctx)
 7. RunAnimationUpdateSystem(ctx)              ◄─ processes keyframes, writes to TransformComponent locals
 8. wi::physics::RunPhysicsUpdateSystem(ctx)
 9. RunTransformUpdateSystem(ctx)              ◄─ builds local matrices from locals
10. --- Wait (sync point) ---
11. RunHierarchyUpdateSystem(ctx)              ◄─ propagates parent transforms to build world matrices
12. Skinning buffer allocation (create/grow skinningBuffer + upload buffer)
13. RunExpressionUpdateSystem(ctx)
14. RunMeshUpdateSystem(ctx)                   ◄─ morph GPU data upload, streamout buffer swap
15. RunVideoUpdateSystem(ctx)
16. RunMaterialUpdateSystem(ctx)
17. --- Wait (sync point) ---
18. RunProceduralAnimationUpdateSystem(ctx)
19. RunArmatureUpdateSystem(ctx)               ◄─ computes final bone matrices, writes to skinningBuffer
20. --- Wait (sync point) ---
21. RunObjectUpdateSystem(ctx)
22. RunCameraUpdateSystem(ctx)
23-28. Decals, Probes, Forces, Lights, Particles, Sounds, Impostors, Sprites, Fonts
```

### Hook Points Summary

| Hook | Location | When It Runs | What Goes Here |
|------|----------|-------------|----------------|
| **Pre-hook** | `MasterRenderer::Update()` at `master_part1.cpp`, just before `__super::Update(dt)` on line 176 | Before `scene->Update(dt)` | dt capping, timer sync, amount lerp, animation culling |
| **Post-hook** | `MasterRenderer::PostUpdate()` override (new virtual override to add) | After `scene->Update(dt)` completes, called by `wi::Application::Update()` | PreFrame bone overrides, loop wrap fixup |
| **Load-time** | Inside existing `WickedCall_RefreshObjectAnimations` and `WickedCall_AddObject` | When objects are loaded | AnimationDataComponent creation (P0), speed=50 (P1) |

---

## Implementation Architecture: GG Animation Bridge Layer

### Design Constraint

**WickedEngineDX12 must remain an unmodified upstream fork.** All fixes live in GameGuru's own code (`Guru-WickedMAX/`). The WickedEngine repo at `D:\max\WickedEngineDX12` can be replaced wholesale with the latest upstream, rebuilt as a static lib, and GameGuru continues to work.

### The Bridge Layer

A single new file pair (`GGAnimBridge.h` / `GGAnimBridge.cpp`) in `Guru-WickedMAX/` that owns all animation behavior that was previously embedded in WickedEngine via `#ifdef GGREDUCED`. It uses only WickedEngine's public API.

```
┌─────────────────────────────────────────────────┐
│  GameGuru Game Logic                            │
│  (wickedcalls*.cpp, M-*.cpp, G-*.cpp)           │
│  Calls WickedCall_PlayObject, etc.              │
└──────────────────┬──────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────┐
│  GG Animation Bridge Layer                      │
│  (Guru-WickedMAX/GGAnimBridge.cpp/.h)           │
│                                                 │
│  Functions:                                     │
│                                                 │
│  GGAnimBridge_OnLoadObject(scene, object)       │
│    - Create AnimationDataComponent entities     │
│      from backwards_compatibility_data  [P0]    │
│    - Set animation.speed = 50           [P1]    │
│                                                 │
│  GGAnimBridge_PreUpdate(scene, dt)              │
│    - Cap dt to 1/30s max                [P6]    │
│    - Sync primary→secondary timers      [P4]    │
│    - Lerp animation.amount toward 1.0   [P3]    │
│    - Pause culled/occluded anims        [P8]    │
│    - 30fps throttle for distant chars   [P9]    │
│                                                 │
│  GGAnimBridge_PostUpdate(scene)                 │
│    - Apply preframe bone overrides      [P5]    │
│    - Fix loop timer wrapping            [P7]    │
│                                                 │
│  GGAnimBridge_SetUpdateOnce(animEntity)         │
│    - Workaround for missing updateonce  [P2]    │
│                                                 │
│  Internal storage:                              │
│    - GGAnimExtra[] -- per-animation data:       │
│        primaryanimid, useprimaryanimtimer,       │
│        objectIndex, updateonce flag              │
│    - GGPreFrame[] -- per-channel overrides:     │
│        iUsePreFrame, fSmoothAmount,             │
│        vPreFrameTranslation/Rotation/Scale      │
│                                                 │
└──────────────────┬──────────────────────────────┘
                   │ (uses only public API)
┌──────────────────▼──────────────────────────────┐
│  WickedEngineDX12  (UNMODIFIED static lib)      │
│  Scene::Update(dt) handles:                     │
│    animation, transforms, hierarchy,            │
│    armatures, skinning, rendering               │
└─────────────────────────────────────────────────┘
```

### Wiring the Hooks (Changes to Existing Files)

#### 1. `master.h` -- Add PostUpdate override to MasterRenderer

```cpp
class MasterRenderer : public RenderPath3D
{
    void Load() override;
    void Update(float dt) override;
    void PostUpdate() override;        // ◄─ ADD THIS LINE
    void Render() const override;
    void Compose(wiGraphics::CommandList cmd) const override;
    // ...
};
```

#### 2. `master_part1.cpp` -- Wire pre-hook and post-hook

```cpp
#include "GGAnimBridge.h"   // ◄─ ADD at top

void MasterRenderer::Update(float dt)
{
    GuruLoopLogic();
    // ... existing terrain/tree/grass/emitter/render calls ...
    GuruLoopRender();

    GGAnimBridge_PreUpdate(&wi::scene::GetScene(), dt);  // ◄─ ADD before super
    __super::Update(dt);   // existing line 176
}

void MasterRenderer::PostUpdate()                         // ◄─ ADD entire function
{
    GGAnimBridge_PostUpdate(&wi::scene::GetScene());
    __super::PostUpdate();
}
```

#### 3. `wickedcalls_part0.cpp` -- Wire load-time hook

In `WickedCall_RefreshObjectAnimations`, after creating animation samplers with `backwards_compatibility_data`:

```cpp
#include "GGAnimBridge.h"   // ◄─ ADD at top

// Inside WickedCall_RefreshObjectAnimations, after the animation setup loop:
GGAnimBridge_OnLoadObject(&scene, pObject);  // ◄─ ADD -- creates AnimationDataComponent entities, sets speed
```

### GG-Side Data Structures

The bridge layer maintains its own per-animation and per-channel tracking data, stored in GameGuru's memory (not in WickedEngine components):

```cpp
// GGAnimBridge.h

struct GGAnimExtra
{
    wi::ecs::Entity animEntity = wi::ecs::INVALID_ENTITY;
    uint32_t primaryanimid = 0;           // for primary/secondary sync [P4]
    uint32_t useprimaryanimtimer = 0;     // for primary/secondary sync [P4]
    uint32_t objectIndex = 0;             // for visibility culling [P8]
    bool updateonce = false;              // for single-frame evaluation [P2]
};

struct GGPreFrame
{
    wi::ecs::Entity boneEntity = wi::ecs::INVALID_ENTITY;
    int iUsePreFrame = 0;                 // mode 0/1/2/3/10000+ [P5]
    float fSmoothAmount = 1.0f;
    XMFLOAT3 vPreFrameTranslation;
    XMFLOAT4 qPreFrameRotation;
    XMFLOAT3 vPreFrameScale;
};
```

These are managed with simple maps keyed by entity ID. They persist across frames and are cleaned up when objects are destroyed.

### Public API Used by Bridge Layer

Only stable WickedEngine public API is used:

```cpp
// Entity creation
wi::ecs::Entity entity = wi::ecs::CreateEntity();

// AnimationDataComponent (keyframe storage)
scene.animation_datas.Create(entity) = sampler.backwards_compatibility_data;

// AnimationComponent control
AnimationComponent& anim = scene.animations[index];
anim.Play(); anim.Pause(); anim.Stop();
anim.timer;  anim.speed;  anim.amount;
anim.start;  anim.end;
anim.IsPlaying();  anim.IsLooped();
anim.last_update_time;   // for updateonce workaround
anim.samplers[i].data;   // entity link to AnimationDataComponent

// TransformComponent (for preframe bone overrides)
TransformComponent* transform = scene.transforms.GetComponent(boneEntity);
transform->translation_local = value;
transform->rotation_local = value;
transform->scale_local = value;
transform->SetDirty();

// ObjectComponent (for visibility culling)
ObjectComponent* obj = scene.objects.GetComponent(objectEntity);
obj->IsOccluded();  obj->IsCulled();

// Component iteration
scene.animations.GetCount();
scene.animations.GetEntity(index);
scene.animations[index];
```

### P5 PreFrame: Same-Frame vs One-Frame-Late

The preframe bone override system (P5) needs to apply **after** animation interpolation but **before** armature bone matrix computation. Since `PostUpdate()` runs after the entire `scene->Update()` (including `RunArmatureUpdateSystem`), overrides applied there are technically one frame late.

**For head look-at and lip sync at 60+ fps, one frame of latency (16ms) is imperceptible.** This is acceptable for initial implementation.

If same-frame behavior is later needed, the `PostUpdate` hook can:
1. Override the bone `TransformComponent` locals
2. Call `transform->UpdateTransform()` to rebuild the local matrix
3. Walk the hierarchy chain up from the bone to rebuild world matrices
4. Recompute just that armature's `boneData` using the same formula: `inverseBind * boneWorld * inverseArmatureWorld`

This is cheap since it affects only 1-2 bones per character (head, jaw), not the whole scene.

---

## Priority Fix List

### P0 -- SHOWSTOPPER: `backwards_compatibility_data` Never Converted to `AnimationDataComponent`

**Symptom**: Zero animations play. All characters frozen in bind/T-pose.

**Root Cause**: GameGuru's `WickedCall_RefreshObjectAnimations` (`wickedcalls_part0.cpp:900-1166`) stores all keyframe data into `sampler.backwards_compatibility_data` and leaves `sampler.data` as `INVALID_ENTITY`. The old WickedRepo had a runtime conversion in `RunAnimationUpdateSystem` (`wiScene.cpp:2586-2593`) that auto-created an `AnimationDataComponent` entity from this data on first use:

```cpp
// OLD wiScene.cpp:2586-2593 (inside RunAnimationUpdateSystem, GGREDUCED block)
if (sampler.data == INVALID_ENTITY)
{
    sampler.data = CreateEntity();
    animation_datas.Create(sampler.data) = sampler.backwards_compatibility_data;
    sampler.backwards_compatibility_data.keyframe_times.clear();
    sampler.backwards_compatibility_data.keyframe_data.clear();
}
```

The new WickedEngineDX12 only does this conversion during scene deserialization (`wiScene_Serializers.cpp:2742-2749`) -- a code path GameGuru never uses because it builds animations programmatically. The new `RunAnimationUpdateSystem` (`wiScene.cpp:1948-1953`) goes straight to entity lookup and skips every channel:

```cpp
// NEW wiScene.cpp:1948-1953
const AnimationDataComponent* animationdata = data_scene->animation_datas.GetComponent(sampler.data);
if (animationdata == nullptr)   // sampler.data is INVALID_ENTITY -> always null
    continue;                    // every channel silently skipped
```

**Fix Location**: `GGAnimBridge_OnLoadObject()`, called from `WickedCall_RefreshObjectAnimations`

**Fix**: For each animation sampler that has `backwards_compatibility_data` populated and `sampler.data == INVALID_ENTITY`:

```cpp
wi::ecs::Entity dataEntity = wi::ecs::CreateEntity();
scene.animation_datas.Create(dataEntity) = sampler.backwards_compatibility_data;
sampler.data = dataEntity;
sampler.backwards_compatibility_data.keyframe_times.clear();
sampler.backwards_compatibility_data.keyframe_data.clear();
```

**Files to modify**: `Guru-WickedMAX/GGAnimBridge.cpp` (new), `Guru-WickedMAX/wickedcalls_part0.cpp` (add call to bridge)

---

### P1 -- CRITICAL: Default Animation Speed 1 vs 50

**Symptom**: Even after P0 fix, animations play at 1/50th expected speed -- effectively frozen.

**Root Cause**: Old WickedRepo GGREDUCED changed `AnimationComponent::speed` default from `1` to `50` (`wiScene.h:1115`). GameGuru's animation timing system was built around this 50x convention. The new engine defaults to `speed = 1`.

**Fix Location**: `GGAnimBridge_OnLoadObject()`, and verify in `WickedCall_PlayObject` / `WickedCall_SetObjectSpeed`

**Fix**: After creating any `AnimationComponent`, set `animation.speed = 50.0f`. Audit all `WickedCall_*` functions that set speed to ensure they use the GG convention.

**Files to modify**: `Guru-WickedMAX/GGAnimBridge.cpp`, `Guru-WickedMAX/wickedcalls_part0.cpp`, `Guru-WickedMAX/wickedcalls_part2.cpp`

---

### P2 -- CRITICAL: `updateonce` Mechanism Removed

**Symptom**: `WickedCall_SetObjectFrame` and `WickedCall_InstantObjectFrameUpdate` set the timer but the pose never evaluates because the animation isn't playing.

**Root Cause**: Old GGREDUCED added `AnimationComponent::updateonce` (`wiScene.h:1196`) -- forces one animation evaluation even when stopped, then auto-resets. The new engine has no equivalent.

The new engine's skip condition (`wiScene.cpp:1940`) is:
```cpp
if (!animation.IsPlaying() && animation.last_update_time == animation.timer)
    continue;
```

It uses `last_update_time` -- it will process a stopped animation if the timer changed externally.

**Fix Location**: `GGAnimBridge_SetUpdateOnce()`, called from `WickedCall_SetObjectFrame` / `WickedCall_InstantObjectFrameUpdate`

**Fix**: Exploit the `last_update_time != timer` check. When GameGuru sets a frame on a stopped animation, ensure the timer value differs from `last_update_time` (e.g., nudge by epsilon if they're equal). Store the `updateonce` flag in `GGAnimExtra` and in `GGAnimBridge_PreUpdate`, briefly play then pause after one frame if needed.

**Files to modify**: `Guru-WickedMAX/GGAnimBridge.cpp`, `Guru-WickedMAX/wickedcalls_part2.cpp`

---

### P3 -- HIGH: `amount` Auto-Lerp Toward 1.0 Removed

**Symptom**: Animation blend transitions get stuck at partial blend weight.

**Root Cause**: Old GGREDUCED (`wiScene.cpp:2864-2867`) gradually lerped `animation.amount` toward 1.0:

```cpp
float fAmountCurveProportionalToSpeed = 0.0002f;
if (animation.speed < 0.5f) fAmountCurveProportionalToSpeed = 0.0001f;
if (animation.speed > 1.5f) fAmountCurveProportionalToSpeed = 0.0005f;
animation.amount = wiMath::Lerp(animation.amount, 1, fAmountCurveProportionalToSpeed);
```

The new engine never modifies `amount`.

**Fix Location**: `GGAnimBridge_PreUpdate()` -- iterate all GG-tracked animations, lerp amount toward 1.0

**Files to modify**: `Guru-WickedMAX/GGAnimBridge.cpp`

---

### P4 -- HIGH: Primary/Secondary Animation Timer Sync Removed

**Symptom**: Split-body animations (walk+attack) and glued object animation sync broken.

**Root Cause**: Old GGREDUCED ran a two-pass animation loop (`wiScene.cpp:2503-2551`). Pass 0: primaries (`useprimaryanimtimer == 0`). Pass 1: secondaries copy timer/speed/amount from their linked primary. The new engine has no concept of this.

**Fix Location**: `GGAnimBridge_PreUpdate()` -- before `scene->Update(dt)`, iterate `GGAnimExtra[]`, find secondaries and copy timer/speed/amount from their linked primary.

**Files to modify**: `Guru-WickedMAX/GGAnimBridge.cpp`, `Guru-WickedMAX/wickedcalls_part2.cpp` (to populate linkage data in `GGAnimExtra`)

---

### P5 -- HIGH: PreFrame Bone Override System Removed

**Symptom**: No head look-at, no lip sync, no procedural per-bone rotation overrides.

**Root Cause**: Old GGREDUCED added `AnimationChannel::iUsePreFrame` with modes 1, 2, 3, 10000+ (`wiScene.h:1138-1145`, `wiScene.cpp:2637-2946`):
- Mode 1: Additively blend preframe transforms
- Mode 2: Replace entirely (head look-at, mouth)
- Mode 3: Replace translation (keeping Y) and rotation
- Mode 10000+: Snap to specific keyframe index

**Fix Location**: `GGAnimBridge_PostUpdate()` -- override `TransformComponent` locals on tracked bone entities after animation has run. One frame late but imperceptible at 60+ fps.

**Files to modify**: `Guru-WickedMAX/GGAnimBridge.cpp`, `Guru-WickedMAX/wickedcalls_part2.cpp` (to populate `GGPreFrame` data)

---

### P6 -- MEDIUM: Delta Time Capping Removed

**Symptom**: Animation timer jumps after alt-tab or stalls.

**Root Cause**: Old GGREDUCED (`MainComponent.cpp:134-139`) capped `deltaTime` to 1/30s. New engine has no cap.

**Fix Location**: `GGAnimBridge_PreUpdate()` -- clamp dt before it reaches scene update. Note: the dt value must be clamped where GameGuru can affect it. Since `MasterRenderer::Update(dt)` receives dt from WickedEngine's timer, the bridge may need to store its own capped dt and pass it through, or clamp animation timers post-update.

**Alternative**: Clamp in `MasterRenderer::Update()` before calling `__super::Update(dt)`.

**Files to modify**: `Guru-WickedMAX/master_part1.cpp` or `Guru-WickedMAX/GGAnimBridge.cpp`

---

### P7 -- MEDIUM: Loop Timer Snap vs Subtraction Wrapping

**Symptom**: Visible animation "pop" at loop boundaries.

**Root Cause**: Old GGREDUCED (`wiScene.cpp:2995-3009`) used `timer -= length` to preserve fractional overshoot. New engine snaps: `animation.timer = animation.start`.

**Fix Location**: `GGAnimBridge_PostUpdate()` -- detect animations that just snapped to start and adjust by fractional overshoot. May require tracking previous-frame timer values.

**Files to modify**: `Guru-WickedMAX/GGAnimBridge.cpp`

---

### P8 -- MEDIUM: Animation Visibility/Occlusion Culling Removed

**Symptom**: Performance regression -- all characters animate even off-screen.

**Root Cause**: Old GGREDUCED (`wiScene.cpp:2561-2574`) used `animation.objectIndex` to skip animation for culled objects.

**Fix Location**: `GGAnimBridge_PreUpdate()` -- pause animations for culled/occluded objects, unpause when visible again. Uses `ObjectComponent::IsOccluded()` / `IsCulled()`.

**Files to modify**: `Guru-WickedMAX/GGAnimBridge.cpp`

---

### P9 -- MEDIUM: 30FPS Animation Throttling Removed

**Symptom**: Performance regression -- double animation CPU cost.

**Root Cause**: Old GGREDUCED (`wiScene.cpp:2515-2523`) skipped every other frame for distant characters.

**Fix Location**: `GGAnimBridge_PreUpdate()` -- pause/unpause animations on alternating frames for distant characters.

**Files to modify**: `Guru-WickedMAX/GGAnimBridge.cpp`

---

### P10 -- LOW: Spring/IK Systems Now Enabled

**Symptom**: Unexpected secondary bone motion on GG characters.

**Root Cause**: Old GGREDUCED disabled `RunSpringUpdateSystem` and `RunInverseKinematicsUpdateSystem` entirely. New engine runs them.

**Fix Location**: At load time, ensure GG characters don't have spring/IK components, or clear `scene.springs` / `scene.inverse_kinematics` for GG entities.

**Files to modify**: `Guru-WickedMAX/GGAnimBridge.cpp` or `Guru-WickedMAX/wickedcalls_part0.cpp`

---

## Implementation Order

The fixes should be implemented in this order for incremental testability:

| Step | Fixes | Expected Result After Step |
|------|-------|---------------------------|
| 1 | Create `GGAnimBridge.h/.cpp` skeleton, wire hooks into `master.h` / `master_part1.cpp` | Compiles, no behavior change |
| 2 | P0 (AnimationDataComponent creation) | Bone transforms start changing each frame |
| 3 | P1 (speed = 50) | Animations play at correct speed, characters visibly animate |
| 4 | P2 (updateonce workaround) | SetObjectFrame works for posing static characters |
| 5 | P6 (dt capping) | No animation jumps after stalls |
| 6 | P3 (amount auto-lerp) | Blend transitions work smoothly |
| 7 | P4 (primary/secondary sync) | Glued object animations stay in sync |
| 8 | P7 (loop wrapping) | Smooth loop transitions |
| 9 | P5 (preframe bone overrides) | Head look-at and lip sync working |
| 10 | P8, P9 (culling, throttling) | Performance back to old levels |
| 11 | P10 (spring/IK disable) | No unexpected bone motion |

Steps 2+3 together are the minimum viable fix -- after those, characters should visibly animate.

---

## Reference: GGREDUCED Blocks in Old WickedRepo

### wiScene.h -- AnimationComponent

| Lines | Feature | Description |
|-------|---------|-------------|
| 1115-1125 | `speed = 50` default | Changed from 1 to 50 for GG's frame-based timing convention |
| 1138-1145 | `iUsePreFrame` system | Per-channel preframe bone override (head turn, lip sync) with modes 1/2/3/10000+ |
| 1194-1199 | `updateonce`, `SetSpeed`, `UsePrimaryAnimTimer` | Force single evaluation, speed setter, animation sync |

### wiScene.cpp -- RunAnimationUpdateSystem

| Lines | Feature | Description |
|-------|---------|-------------|
| 2503-2551 | Two-pass primary/secondary loop | First pass processes primaries, second pass syncs secondaries |
| 2515-2523 | 30FPS animation throttling | `(iAnimFrames + i) % 2 == 0` skips alternating frames |
| 2550-2562 | `updateonce` handling | Forces one evaluation then resets flag |
| 2561-2574 | Visibility-based culling | Skips animation for occluded/culled objects via `objectIndex` |
| 2586-2593 | **Runtime backwards-compat conversion** | Creates `AnimationDataComponent` from `backwards_compatibility_data` -- **THE smoking gun for P0** |
| 2603-2633 | `prevKeyRight` cached keyframe search | Avoids linear scan from 0 each frame |
| 2616-2622 | Loop timer clamping | Prevents keyframe lookup beyond `animation.end` during loop wrap |
| 2637-2649 | `iUsePreFrame >= 10000` keyframe snap | Snaps rotation channel to specific keyframe index |
| 2862-2946 | Post-interpolation preframe overrides | Modes 1/2/3 for additive/replace bone transforms + `amount` auto-lerp |
| 2995-3020 | Subtraction loop wrapping | `timer -= length` instead of snap to start |

### wiScene.cpp -- Other Systems

| Lines | Feature | Description |
|-------|---------|-------------|
| 1730-1757 | Scene::Update order changes | Disables Spring, IK, Impostor, Physics; reorders mesh/material update |
| 3379-3385 | `boneMatrices` cache in RunArmatureUpdateSystem | Pre-decompressed XMMATRIX for fast CPU SkinVertex |
| 3410-3421 | Soft body removal in RunMeshUpdateSystem | Unconditional PRE buffer creation for skinned meshes |
| 4471-4476 | SkinVertex bounds check | Safety guard on bone index/weight array access |

### wiRenderer.cpp -- Skinning Dispatch

| Lines | Feature | Description |
|-------|---------|-------------|
| 5428-5432 | Modified entry condition | Added `IsRenderable()`, removed `armatures.Contains()` |
| 5434-5448 | Null armature guard + soft body removal | Prevents crash on missing armature, removes unused soft body skip |

### MainComponent.cpp

| Lines | Feature | Description |
|-------|---------|-------------|
| 134-139 | Delta time capping | `if (deltaTime > 1.0/30.0) deltaTime = 1.0/30.0;` |

### wiScene.h -- Other Components (Non-Animation)

| Lines | Component | Feature |
|-------|-----------|---------|
| 266-268 | MaterialComponent | `GetAlphaRef()` accessor |
| 643-644, 687-741 | ObjectComponent | Full LOD system (flags, distance, methods), render order bias, anti-culling, collision disable |
| 1253-1255 | WeatherComponent | `fogColorAndOpacity` (promoted to always-on) |
| 1432-1449 | Scene | Mutable `envmapRes` with runtime setter |

### wiEnums.h

| Lines | Feature | Description |
|-------|---------|-------------|
| 7-10 | `BLENDMODE_ALPHANOZ`, `BLENDMODE_FORCEDEPTH` | Extra blend modes (promoted to always-on) |
| 402-408 | `CSTYPE_POSTPROCESS_RAIN/SNOW`, `*_LOD` shaders | Weather post-process + LOD shader types |

---

## Verification Checklist

After implementing fixes, verify with the automation harness (see `WETEST.md`):

1. **Load Switch Escape demo** and enter editor -- characters should not be in T-pose
2. **Test Level** -- AI characters should animate (walk, idle, attack cycles)
3. **Check FPS regression** -- compare against baseline in WETEST.md
4. **Verify loop smoothness** -- watch a looping walk cycle for visible pops
5. **Alt-tab test** -- switch away and back, characters should not jump/teleport
6. **Head look-at** (after P5) -- NPCs should track player position
7. **Glued objects** (after P4) -- weapons/items attached to characters should animate in sync

Use `LIST_ENTITIES` and `GET_ENTITY` harness commands to inspect entity state at runtime.

### Automation Test Sequence

```bash
D="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"

# Build Release
"D:/max/GameGuruMAXDX12/GameGuru Core/build.bat" Release

# Launch, load Switch Escape into editor, enter test game
# (see WETEST.md "Standard Sequence" for full command list)

# Check for animated entities in game state:
echo "LIST_ENTITIES" > "$D/auto_command.txt"
# Look for entities with LIGHT data, character markers, etc.

# Check performance:
echo "GET_PERF_DATA" > "$D/auto_command.txt"
# Compare SCENE_ANIMATIONS, SCENE_ARMATURES counts against expected values
```

---

*Document created: 2026-02-17*
*Updated: 2026-02-17 -- Added frame loop call chain, GGAnimBridge architecture, implementation order, hook wiring details*
*Based on comparative analysis of WickedRepo (old, with GGREDUCED at `D:\max\WickedRepo`) vs WickedEngineDX12 (new, clean upstream at `D:\max\WickedEngineDX12`)*
