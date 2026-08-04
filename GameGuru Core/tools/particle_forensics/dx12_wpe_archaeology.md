# DX12 Port Archaeology — WPE Particle System (`.PE` / `wiEmittedParticle`)

Date of investigation: 2026-08-04
Trees examined (all read-only except where noted):

| Role | Path | HEAD |
|---|---|---|
| DX12 game | `D:\max\GameGuruMAXDX12` | `33c5347e` (branch `main`) |
| DX12 engine | `D:\max\WickedEngineDX12` | working copy |
| DX11 game (reference, READ ONLY) | `D:\max\GameGuruMAX` | working copy |
| DX11 engine fork (reference, READ ONLY) | `D:\max\WickedRepo` | `842a0a0` "Merge pull request #30 from plemsoft/PE-2026-1" |

`WICKEDPARTICLESYSTEM` is **defined** — `GameGuru Core\GameGuru\Include\preprocessor-moreflags.h:119`.
The WPE system is therefore **compiled in**. Its breakage is by stub-out and by an archive-version
gate, not by a preprocessor switch.

---

## 0. HEADLINE ANSWER — can the modern engine deserialise the shipped `.PE` files?

**NO. Proven, twice over, at two independent layers.**

### 0.1 Proof: the files on disk carry archive version 5076

Read of the first 8 bytes (the `uint64_t version` that is the first field of every wiArchive) of every
`.PE` in `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\Files\particlesbank` (14 files):

```
birds.pe                  size=5440     version=5076
burst-explosion2-g.pe     size=3860     version=5076
dirt-splash4.pe           size=3826     version=5076
downpour.pe               size=2935     version=5076
dust devil.pe             size=1204     version=5076
Explosion Burst.pe        size=168357   version=5076
firearea.pe               size=5366712  version=5076
heavy-rain3.pe            size=49369    version=5076
   ... (14 total, all 5076)
```

### 0.2 Proof: the version spaces are hard-forked and do not overlap

| | file | constant | value |
|---|---|---|---|
| DX11 fork | `D:\max\WickedRepo\WickedEngine\wiArchive.cpp:7` | `__archiveVersion` | **5077** |
| DX11 fork | `wiArchive.cpp:9` | `__archiveVersionBarrier` | 22 |
| DX12 engine | `D:\max\WickedEngineDX12\WickedEngine\wiArchive.cpp:18` | `__archiveVersion` | **93** |
| DX12 engine | `wiArchive.cpp:20` | `__archiveVersionBarrier` | 22 |

The GameGuru team deliberately jumped the archive version into a private 5000-block so their own
extra serialised fields could be gated. `wiEmittedParticle.cpp:952,961,966,984,993,997` (WickedRepo)
gate on `archive.GetVersion() >= 5072 / 5073 / 5074 / 5075 / 5076 / 5077`, each marked
`//PE: Special ggm version.` The modern engine is at 93 and will never reach 5072.

### 0.3 Proof: the modern engine rejects the file outright

`D:\max\WickedEngineDX12\WickedEngine\wiArchive.cpp:80-85`:

```cpp
if (header.version > __archiveVersion)
{
    wi::helper::messageBox("File is not supported!\nFile: " + fileName + "\nReason: The archive version ("
        + std::to_string(header.version) + ") is higher than the program's ("
        + std::to_string(__archiveVersion) + ")!\n...", "Error!");
    Close();
    return;
}
```

5076 > 93 → **modal message box + `Close()`**. `IsOpen()` is then false and nothing is read.

### 0.4 Proof: the game already knows this and short-circuits before the engine even sees the file

Commit **`f189857b`** — Lee Bamber, **Sun Feb 15 16:07:32 2026 +0000** —
*"Fix wi::Archive messagebox errors from old DX11 particle effect files"*:

> Old .wpe.pe decal particle files (blood, dust, explosion, impact, sparks, splash_large,
> splinters) have incompatible binary format (version 5076/5077) from the DX11 engine era.
> These were being fed directly to wiArchive which showed ~35 blocking messagebox errors
> during Test Level.
> Added pre-validation in WickedCall_LoadWiSceneDirect that reads the first 8 bytes and
> checks the archive version is in valid range (22-200) before passing to wiArchive.
> Invalid files are silently skipped with a log entry to Guru-MapEditor.log via timestampactivity.

Current code — `GameGuru Core\Guru-WickedMAX\wickedcalls_part3.cpp:1961-1979`:

```cpp
// Validate file is a genuine Wicked Engine archive before opening
// (old .pe particle files from the DX11 engine have incompatible binary format)
{
    FILE* fcheck = fopen(filename, "rb");
    if (fcheck)
    {
        uint64_t fileVersion = 0;
        size_t bytesRead = fread(&fileVersion, 1, sizeof(fileVersion), fcheck);
        fclose(fcheck);
        if (bytesRead < sizeof(fileVersion) || fileVersion < 22 || fileVersion > 200)
        {
            char msg[1024];
            sprintf(msg, "Skipped non-Wicked archive: %s (version=%llu, expected 22-93)", filename, (unsigned long long)fileVersion);
            void timestampactivity(int i, char* desc_s);
            timestampactivity(0, msg);
            return 0;
        }
    }
}
```

5076 > 200 → `return 0`.

### 0.5 Consequence trace

```
WickedCall_LoadWPE(filename)                 part3.cpp:2130
  -> WickedCall_LoadWiScene(path,...)        part3.cpp:2140  -> part3.cpp:2118
       -> WickedCall_LoadWiSceneDirect(...)  part3.cpp:1951
            -> version pre-check FAILS       part3.cpp:1970  -> return 0
       -> GetScene().Merge(scene2)           part3.cpp:2126  (merges an EMPTY scene)
  -> count_before == count_after             part3.cpp:2142  (no emitter was added)
  -> if-body skipped, root stays 0
  -> return 0
```

So **`WickedCall_LoadWPE` returns 0 for every shipped `.PE`.** No entity, no emitter component,
no material, no texture. The effect does not load, does not emit and does not render.
It fails **silently** — one `timestampactivity` line into `Guru-MapEditor.log` and nothing else.

Every downstream call is then made against root `0`:
`WickedCall_PerformEmitterAction(iAction, 0)` scans `scene.emitters` for
`hier->parentID == 0` and matches nothing.

### 0.6 A second, independent breakage that would bite even if you lifted the version gate — INFERENCE (strong)

`WickedCall_LoadWiSceneDirect` does not call `Scene::Serialize`; it hand-rolls the component-by-component
read order. The port **deleted four `aabb_*` reads and the `prev_transforms` read and the
`wiResourceManager::Serialize` read** (commit `d3ae5996`, quoted in §3). A v5076 `.PE` byte stream
*contains* those blocks. Removing the reads leaves the stream cursor misaligned from the
`hierarchy.Serialize` call onward, so every subsequent component (materials, meshes, objects,
**emitters**) would be decoded from the wrong offset — garbage, or a crash on a bogus length prefix.

Label: **inference**, not proof (nothing currently reaches that code with a real `.PE`, so it has
never been observed). But it is a direct reading of the code and must be fixed as part of any
re-enable, not just the version gate.

### 0.7 What a fix has to look like

There is no "bump a number" fix. The old format is a *different schema*, not merely a different
version stamp:
- the stock prefix differs (see §1, e.g. `Particle.rotationalVelocity` float → `rotation_rotationVelocity` packed uint; `color_mirror` → `color`; `ParticleCounters` grew 2 fields and the offset constants were re-ordered),
- the 5072–5077 GG tail fields have no receiving members in the modern class,
- the scene-level component ordering differs.

Realistic options: (a) write a one-shot **offline converter** that reads `.PE` v5076 with a
standalone copy of the old reader and re-emits a v93 archive plus a sidecar for the GG-only fields;
or (b) **re-port the GG extensions** into `wi::EmittedParticleSystem` and add a v5076-compatible
read path guarded by the file version. Either way the GG-local members in §1 must come back or the
authored effects lose their look.

---

## 1. ENGINE-SIDE REGRESSION TABLE — `wiEmittedParticle.h`

`D:\max\WickedRepo\WickedEngine\wiEmittedParticle.h` (218 lines, class `wiScene::wiEmittedParticle`)
vs `D:\max\WickedEngineDX12\WickedEngine\wiEmittedParticle.h` (181 lines, class `wi::EmittedParticleSystem`).

The DX12 game still writes `wiEmittedParticle` — that is a compatibility alias:
`D:\max\WickedEngineDX12\WickedEngine\WickedEngine.h:137` → `using wiEmittedParticle = wi::EmittedParticleSystem;`
So the *name* survived the port; the *members* did not.

Classification via `git blame` in `D:\max\WickedRepo` (read-only). Author **Preben Eriksen** = GameGuru
team ("PE:"); author **Lee Bamber 2022-11-27 `0988337c`** = the original vendor drop, i.e. stock upstream
of that era.

### 1.1 GameGuru-local members REMOVED by the port (all absent in DX12)

| Member / method | WickedRepo line | blame commit / author / date | Origin | What it did |
|---|---|---|---|---|
| `fadein_time` | h:115 | `983e8f7c` Preben 2024-12-02 | GG-local | per-particle alpha fade-in ramp; fed to shader as `xEmitterFadeinTime` (`wiEmittedParticle.cpp:353`) |
| `burst_amount` | h:116 | `983e8f7c` Preben 2024-12-02 | GG-local | authored burst size |
| `burst_split` | h:117 | `eefbe334` Preben 2025-01-07 | GG-local | spreads a burst over N frames instead of one spike (`cpp:238-246`) |
| `burst_delay` | h:118 | `983e8f7c` Preben 2024-12-02 | GG-local | delay before a burst fires |
| `burst_delay_timer` | h:127 | `983e8f7c` Preben 2024-12-02 | GG-local | runtime countdown for the above (`cpp:231-235`) |
| `normal_factor_x/y/z` | h:119-121 | `983e8f7c` Preben 2024-12-02 | GG-local | per-axis emission direction bias |
| `burst_factor_x/y/z` | h:123-125 | `eefbe334` Preben 2025-01-07 | GG-local | per-axis burst direction bias |
| `burst_factor_speed` | h:144 | `eefbe334` Preben 2025-01-07 | GG-local | burst velocity multiplier |
| `startpos` (XMFLOAT3) | h:128 | `983e8f7c` Preben 2024-12-02 | GG-local | emitter local spawn offset |
| `normal_random` | h:130 | `eefbe334` Preben 2025-01-07 | GG-local | randomisation of emission normal |
| `rotation_random` | h:131 | `983e8f7c` Preben 2024-12-02 | GG-local | random initial spin |
| `size_random` | h:132 | `983e8f7c` Preben 2024-12-02 | GG-local | random particle size |
| `scaling_random` | h:133 | `eefbe334` Preben 2025-01-07 | GG-local | random scale-over-life |
| `spawn_random` | h:135 | `983e8f7c` Preben 2024-12-02 | GG-local | drives the stuttered/gusting emission model (`cpp:207-227`) |
| `spawn_pause`, `spawn_pause_random` | h:136-137 | `983e8f7c` Preben 2024-12-02 | GG-local | pause between emission bursts |
| `endcolor_red/green/blue` | h:139-141 | `983e8f7c` Preben 2024-12-02 | GG-local | colour-over-life end tint; shader `xParticleEndColorRed/Green/Blue` |
| **`bFindFloor`** | **h:143** | `eefbe334` Preben 2025-01-07 | GG-local | snap emitter root to terrain height each frame |
| `start_rotation` | h:145 | `eefbe334` Preben 2025-01-07 | GG-local | fixed initial rotation; shader `xParticleStartRotation` |
| **`bFollowCamera`** | **h:146** | `eefbe334` Preben 2025-01-07 | GG-local | pin emitter root to camera (rain/snow/dust volumes) |
| `random_position`, `random_position_scale` | h:148-149 | `eefbe334` Preben 2025-01-07 | GG-local | random spawn jitter; shader `xParticleRandomPos`, `xParticleRandomPosScale` |
| `total_emit_count` | h:150 | `eefbe334` Preben 2025-01-07 | GG-local | running emit counter, sent to shader as `xTotalEmitCount` (`cpp:379`) |
| `bTriggerOutDoor` / `bTriggerInDoor` / `bTriggerUnderWater` | h:152-154 | `eefbe334` Preben 2025-01-07 | GG-local | environment gating of the effect |
| `randemit`, `randpause` | h:156-157 | `c5031382` Preben 2025-06-15 | GG-local | state for the gusting emission model (`cpp:207-227`) |
| **`distance_sort_bias`** | h:158 | `c5031382` Preben 2025-06-15 | GG-local | per-emitter depth-sort nudge; consumed by the renderer at `WickedRepo\WickedEngine\wiRenderer.cpp:6122` and `:6252` |
| `wpe_filler_1/2/3` | h:159-161 | `c5031382` Preben 2025-06-15 | GG-local | reserved slots (forward-compat padding in the v5077 tail) |
| `emittimer` + `SetTimer()` / `GetTimer()` | h:163-165 | `54b37150` Preben 2025-05-21 | GG-local | lifetime bookkeeping for auto-despawn |
| **`bVisible` + `IsVisible()` / `SetVisible()`** | **h:167-169** | `983e8f7c` Preben 2024-12-02 | GG-local | render gate. Consumed at `WickedRepo\wiRenderer.cpp:4360` inside the emitter cull |
| **`bActive` + `IsActive()` / `SetActive()`** | **h:171-173** | `54b37150` Preben 2025-05-21 | GG-local | second render gate. `wiRenderer.cpp:4362` |
| `bStatActive` + `IsStatActive()` / `SetStatActive()` | h:175-177 | `54b37150` Preben 2025-05-21 | GG-local | statistics/readback opt-in |
| `GetEmit()` | h:191 | `61a5407e` Preben 2025-08-08 | GG-local | expose accumulated emit for the editor |
| **`FLAG_EMIT_PAUSE = 1 << 7`** | **h:74** | `d85fe6e8` Preben 2025-06-20 | GG-local | flag bit |
| **`IsEmitPaused()`** | **h:195** | `d85fe6e8` Preben 2025-06-20 | GG-local | read of the above |
| **`SetEmitPaused()`** | **h:205** | `d85fe6e8` Preben 2025-06-20 | GG-local | "stop spawning but let existing particles live out" — `cpp:253-258` zeroes `emit` and `burst` without killing the system |
| `restitution` default **0.70f** | h:114 | `983e8f7c` Preben 2024-12-02 | GG-local retune of a stock field | DX12 keeps the field but restores the stock default **0.98f** (`WickedEngineDX12\wiEmittedParticle.h`) — bounciness will differ even for effects that do survive |

The DX11 fork wraps this whole block in `//#ifdef GGREDUCED` / `//#endif` (commented-out guards,
h:113 and h:179) — i.e. the team explicitly marked it as their local extension zone.

### 1.2 What the DX12 engine has that the DX11 fork does not (new upstream capability)

Not regressions — these are gains, listed so the re-port target is clear.

| Member | Note |
|---|---|
| `FLAG_COLLIDERS_DISABLED`, `FLAG_USE_RAIN_BLOCKER`, `FLAG_TAKE_COLOR_FROM_MESH` | new upstream flag bits (1<<7, 1<<8, 1<<9). **Note `1<<7` is now `FLAG_COLLIDERS_DISABLED` — the exact bit the GG fork used for `FLAG_EMIT_PAUSE`.** Any old `_flags` value read from a `.PE` would be reinterpreted. |
| `Burst(int, const XMFLOAT3&, wi::Color)` / `Burst(int, const XMFLOAT4X4&, wi::Color)` | positional/coloured bursts |
| `opacityCurveControlPeakStart/End`, `opacityCurveTex`, `SetOpacityCurveControl()` | upstream's own answer to fade-in/out — the natural landing spot for GG `fadein_time` |
| `CreateRaytracingRenderData()`, `BLAS`, `primitiveBuffer` | particles in RT |
| `culledIndirectionBuffer`, `culledIndirectionBuffer2`, `generalBuffer`, `vb_pos/nor/uvs/col` | GPU-culled draw path |
| `IsInactive()` / `active_frames` | upstream idle detection (partial substitute for `bActive`) |
| `emit_locations` + `EmitLocation` struct | multi-location emission |
| `GetMemorySizeInBytes()` returns `uint64_t` (was `uint32_t`) | mechanical |

### 1.3 Shader-interop regressions — `shaders\ShaderInterop_EmittedParticle.h`

Verified by grepping each symbol across both `shaders\` trees. `DX11fork_files` = number of files
referencing it in `D:\max\WickedRepo\WickedEngine\shaders`, `DX12_files` = same in
`D:\max\WickedEngineDX12\WickedEngine\shaders`.

```
xParticleEndColorRed         DX11fork=2   DX12=0
xParticleNormalFactorX       DX11fork=2   DX12=0
xParticleNormalFactor2X      DX11fork=2   DX12=0
xParticleStartRotation       DX11fork=2   DX12=0
xParticleRandomPos           DX11fork=2   DX12=0
xParticleNormalRandom        DX11fork=2   DX12=0
xParticleRotationRandom      DX11fork=2   DX12=0
xParticleSizeRandom          DX11fork=2   DX12=0
xParticleScalingRandom       DX11fork=2   DX12=0
xParticleBurstFactorDpeed    DX11fork=2   DX12=0
xEmitterFadeinTime           DX11fork=2   DX12=0
xEmitterOpacity              DX11fork=3   DX12=0
xParticleEmissive            DX11fork=2   DX12=0
xParticleSinPos              DX11fork=2   DX12=0
xTotalEmitCount              DX11fork=2   DX12=0
color_mirror                 DX11fork=5   DX12=0
```

**Every GameGuru-authored shader uniform is gone from the DX12 shader set. Zero references.**

Binary-layout changes in the same header (these are what make the old `.PE` unreadable even
before you reach the GG tail):

| DX11 fork | DX12 |
|---|---|
| `Particle.rotationalVelocity` (float) | `Particle.rotation_rotationVelocity` (packed uint) |
| `Particle.color_mirror` (uint) | `Particle.color` (uint) — mirror bit dropped |
| `ParticleCounters` = 4 uints | 6 uints (`+culledCount`, `+cellAllocator`) |
| `PARTICLECOUNTER_OFFSET_REALEMITCOUNT` exists | **removed**; the remaining offsets were renumbered |
| `ARGUMENTBUFFER_OFFSET_DISPATCHEMIT` exists | **removed**; emit is now push-constant driven |
| `THREADCOUNT_EMIT = 256` | removed; emit CS is `[numthreads(64,1,1)]` |
| `xEmitterWorld` float4x4 in the CB | removed — world matrix now arrives per-`EmitLocation` |

The whole emit/simulate pipeline was rearchitected upstream: `emittedparticle_emitCS.hlsl` went from
`RWSTRUCTUREDBUFFER(...)` macros + `TEXSLOT_ONDEMAND` bindings to explicit
`register(u0..u4)` + `PUSHCONSTANT(push, PushEmit)` + bindless `load_geometry()` /
`bindless_buffers_float4[]`. SPH gained `emittedparticle_sphbinningCS.hlsl` and
`emittedparticle_sphcellallocationCS.hlsl` and lost `sphpartitionoffsetsCS` /
`sphpartitionoffsetsresetCS`. These are **upstream generational changes, not port damage** — but they
mean the GG shader edits cannot be cherry-picked; they must be re-implemented.

---

## 2. GAME-SIDE STUB-OUT INVENTORY

All in `D:\max\GameGuruMAXDX12\GameGuru Core\Guru-WickedMAX\`.

| # | Location | What was there (DX11) | What is there now | Runtime consequence |
|---|---|---|---|---|
| S1 | `part3.cpp:1961-1979` | *(nothing)* | archive version pre-check, `return 0` outside 22..200 | **Every shipped `.PE` (v5076) is skipped.** Effect never loads. Silent apart from one `timestampactivity` log line. |
| S2 | `part3.cpp` (was `wickedcalls.cpp:7565-7569`) | `wiResourceManager::ResourceSerializer resource_seri; if (archive.GetVersion() >= 63) wiResourceManager::Serialize(archive, resource_seri);` | fully commented, `//wiResourceManager::Serialize removed from API` | embedded resources in the archive are not kept alive / not read. Stream desync (see §0.6). |
| S3 | `part3.cpp` | `scene2.prev_transforms.Serialize(archive, seri);` | commented `// removed, replaced by matrix_objects_prev` | motion-vector transforms not read. Stream desync. |
| S4 | `part3.cpp` | `scene2.aabb_objects / aabb_lights / aabb_probes / aabb_decals .Serialize(archive, seri);` (4 calls) | all four commented `// aabb vectors no longer have Serialize` | 4 more missing reads. Stream desync. |
| S5 | `part3.cpp` | `BLENDMODE_FORCEDEPTH` remap | `BLENDMODE_OPAQUE` remap `// was BLENDMODE_FORCEDEPTH, removed` | authored force-depth materials now land on the wrong blend mode |
| S6 | `part3.cpp` | `BLENDMODE_ALPHANOZ` remap | `BLENDMODE_ALPHA` remap `// was BLENDMODE_ALPHANOZ, removed` | the remap became `ALPHA → ALPHA`, a no-op; alpha-no-Z-write effects will now z-write |
| S7 | `part3.cpp:2157` (`WickedCall_LoadWPE`) | `ec->SetVisible(false);` | `//ec->SetVisible(false); // SetVisible removed` | freshly loaded effects are **not** hidden on load. Every editor preview / weapon-projectile effect that relied on load-hidden-then-show pops visible for a frame or stays permanently on. |
| S8 | `part4.cpp:42` | `case 5: ec->SetVisible(true);` | commented | **emitter action 5 ("visible") is a silent no-op** |
| S9 | `part4.cpp:47` | `case 6: ec->SetVisible(false);` | commented | **emitter action 6 ("not visible") is a silent no-op.** This is the most-called action in the codebase — 8 call sites across `imgui_gg_dx11_part5.cpp`, `M-GridEditB_part12/24.cpp`, `M-Entity_part5.cpp`, `M-Weapon.cpp`. Preview effects can never be hidden. |
| S10 | `part4.cpp:52` | `case 7: ec->SetEmitPaused(true);` | commented | **action 7 ("pause emit") is a no-op.** No way to stop spawning while letting live particles die out; callers get a hard on/off only. |
| S11 | `part4.cpp:57` | `case 8: ec->SetEmitPaused(false);` | commented | **action 8 ("resume emit") is a no-op** |
| S12 | `part4.cpp:152-153` | `if (ec && (ec->bFindFloor \|\| ec->bFollowCamera))` | `if (false) // disabled` | **the entire camera-follow / floor-snap block in `WickedCall_UpdateEmitters` is dead.** Weather volumes (rain, snow, dust, downpour, heavy-rain3) no longer track the player; they stay wherever they were placed. |
| S13 | `part4.cpp:176` | `if (ec->bFollowCamera)` | `if (false)` | inner camera-follow translate is dead (unreachable anyway via S12) |
| S14 | `part4.cpp:187` | `if (ec->bFindFloor && ec->bFollowCamera)` | `if (false)` | inner `BT_GetGroundHeight` floor-snap is dead |
| S15 | `part4.cpp:303-306` | `if (!ec.IsVisible()) continue; if (!ec.IsActive()) continue;` | both commented | **`GetVisibleWEmitters()` now returns the TOTAL emitter count, not the visible count.** Any editor/perf readout using it over-reports. |
| S16 | `part4.cpp:337-393` | `customShaderParam1..7` reads/writes | all seven commented, `// TODO: customShaderParam1-7 removed from MaterialComponent, replaced with uint4 userdata` | `WickedCall_SetShaderParameter` is a **complete no-op** — `bChanged` can never become true, `SetDirty()` is never called. (Not WPE-specific but sits in the same file and was neutered by the same commit.) |

### 2.1 Engine-side loss that makes S7/S8/S9/S15 *unrecoverable* without a re-port

The DX11 fork gated the emitter cull on those flags —
`D:\max\WickedRepo\WickedEngine\wiRenderer.cpp:4357-4369`:

```cpp
for (size_t i = 0; i < vis.scene->emitters.GetCount(); ++i)
{
    const wiEmittedParticle& emitter = vis.scene->emitters[i];
    if(!emitter.IsVisible())
        continue;
    if (!emitter.IsActive())
        continue;
    if (!(emitter.layerMask & vis.layerMask))
    {
        continue;
    }
    vis.visibleEmitters.push_back((uint32_t)i);
}
```

The DX12 engine — `D:\max\WickedEngineDX12\WickedEngine\wiRenderer.cpp:4156-4164`:

```cpp
for (size_t i = 0; i < vis.scene->emitters.GetCount(); ++i)
{
    const wi::EmittedParticleSystem& emitter = vis.scene->emitters[i];
    if (!(emitter.layerMask & vis.layerMask))
    {
        continue;
    }
    vis.visibleEmitters.push_back((uint32_t)i);
}
```

**Only `layerMask` remains.** So even if the game-side `SetVisible` calls were restored, there is
nothing in the engine to honour them. Re-enabling WPE visibility requires an engine delta
(add `bVisible`/`bActive` + the two `continue`s back), or a re-expression on top of `layerMask`.

### 2.2 The `part4.cpp:161` loop bug — PRE-EXISTING, NOT PORT-INTRODUCED

`GameGuruMAXDX12\...\wickedcalls_part4.cpp:161`:

```cpp
for (int a = 0; a > parent_used.size(); a++)
```

`>` should be `<`; the loop body never executes, so `bAlreadySet` is always false and every emitter
re-processes its parent root (O(n) redundant transform updates when the block was live).

**Proof it is pre-existing:** the identical line is in the READ-ONLY DX11 original at
`D:\max\GameGuruMAX\GameGuru Core\Guru-WickedMAX\wickedcalls.cpp:7893`, and it does **not** appear as
a changed line in the `d3ae5996` diff (§3). The port copied it verbatim. Currently harmless because
S12 makes the whole block unreachable — but it must be fixed at the same time as S12 is restored,
or the restored code will do redundant work.

### 2.3 Is the system still called at all?

Yes — it is fully wired, which is why the breakage is user-visible rather than dormant:

- `DarkLUA_part5.cpp:1209` — LUA `PerformEmitterAction` bridge
- `imgui_gg_dx11_part5.cpp:245-284` — editor particle-preview panel (`LoadWPE` + actions 1,4,5,6)
- `M-Entity_part5.cpp:121-133, 253-256` — placed particle entities (pause/hide on place, resume/restart/show/burst on activate)
- `M-GridEditB_part12.cpp:179,1824,2228,2450`, `M-GridEditB_part24.cpp:1008` — editor hide-preview
- `M-Weapon.cpp:1215-1217` — weapon projectile trail effects (`LoadWPE` then action 6)

---

## 3. GAME-SIDE DIFF, CATEGORISED (DX11 `wickedcalls.cpp:7542-8045` vs DX12 `part3.cpp:1950-2162` + `part4.cpp:2-312`)

10 hunks total. Categories: **(a)** mechanical API migration, **(b)** behavioural change,
**(c)** capability removal, **(d)** new DX12-only code.

| Hunk | Change | Category |
|---|---|---|
| 1 | archive version pre-check added (S1) | **(d)** new DX12-only code — a workaround, not a port |
| 2 | `wiResourceManager::Serialize` block commented (S2) | **(c)** capability removal + stream desync |
| 3 | `prev_transforms.Serialize` commented (S3) | **(c)** |
| 4 | 4× `aabb_*.Serialize` commented (S4) | **(c)** |
| 5 | `if (!...textures[a].resource)` → `.resource.IsValid()` | **(a)** mechanical — `shared_ptr` → handle wrapper |
| 6 | `BLENDMODE_FORCEDEPTH`→`BLENDMODE_OPAQUE`, `BLENDMODE_ALPHANOZ`→`BLENDMODE_ALPHA` (S5/S6) | **(b)** behavioural — enum values removed upstream, the remap was retargeted to whatever still compiled |
| 7 | `#endif // WICKEDPARTICLESYSTEM` inserted at part3 EOF, `#ifdef` re-opened at part4 top | **(a)** mechanical — consequence of the `1fdcd66f` file split |
| 8 | `ec->SetVisible(false)` in `LoadWPE` commented (S7) | **(c)** |
| 9 | actions 5/6/7/8 commented (S8-S11) | **(c)** |
| 10 | `bFindFloor`/`bFollowCamera` block → `if (false)` (S12-S14) | **(c)** |
| 11 | `IsVisible`/`IsActive` guards in `GetVisibleWEmitters` commented (S15) | **(c)** — and **(b)**, since the return value silently changed meaning |

Everything in `WickedCall_ParticleEffectPosition`, `WickedCall_ParticleEffectPositionRotation`
and `WickedCall_CreateEmitter` is **byte-identical** to the DX11 original (they appear in no hunk).
Note `WickedCall_CreateEmitter` still uses `BLENDMODE_ADDITIVE` and the local-`Scene` + `Merge`
pattern unchanged — it was never exercised by the compiler because it touches no removed API.

---

## 4. GIT ARCHAEOLOGY — DX12 REPO

Full history of `wickedcalls_part3.cpp` / `wickedcalls_part4.cpp`:

```
26f4268f 2026-07-31 FPS-plummet fix set: capped LOS rays + static-mesh BVHs + Intersect diagnostics
4aff35da 2026-07-30 Character dedicated sun shadows: GG bridge to the engine's slot mechanism
77ce8b2f 2026-07-24 Editor: fix dead 'Frustum/Apparent Culled' readout (was hardcoded 0)
1fbb4ec6 2026-07-23 Pick cache: bypass while a mouse button is held
21071c32 2026-07-23 Perf P.3: editor CPU 45->61 FPS (TESTPRO1), visuals identical
ea449c02 2026-07-18 Editor: hide brush ring in single-tree tools + true DX12 VRAM readout
2021f76b 2026-07-18 Enable staggered shadow-cascade refresh (Wicked delta 1.11, DX11 parity)
0d8b185e 2026-07-18 Shadow distance parity
241bd64b 2026-02-16 Fix DX12 lighting brightness to match DX11 levels
f78e98f3 2026-02-16 Add scene interrogation harness commands and fix point light intensity
f189857b 2026-02-15 Fix wi::Archive messagebox errors from old DX11 particle effect files   <-- S1
f7a6c090 2026-02-14 Phase 5: ImGui migrated to DX12 backend
d3ae5996 2026-02-13 Fix all compilation errors for WickedEngine API migration (741 -> 0 errors)  <-- S2..S16
1fdcd66f 2026-02-13 Split humongous files into smaller part files
```

**Only two commits ever touched WPE behaviour.** `35bed006` (linker errors), `7214580e` (Phase 6 crash
loop) and `393e7be5` (Phase 4) did **not** touch these files — confirmed by their absence from this list.

### 4.1 `d3ae5996` — Lee Bamber, Fri Feb 13 23:50:10 2026 +0000

> Fix all compilation errors for WickedEngine API migration (741 -> 0 errors)
>
> Comprehensive update of 134 files to match the current WickedEngine API.
> Key changes: struct member renames (TextureDesc, GPUBufferDesc, RasterizerState,
> DepthStencilState, BlendState, SamplerDesc, Viewport), removed ShaderStage params
> from Bind* calls, LightComponent/AnimationComponent/ObjectComponent API updates,
> Format/BindFlag/Usage/ResourceState enum scoping, RenderPassAttachment value types,
> tinyddsloader code disabled, removed API stubs, and /FS /bigobj compiler flags.

134 files, 2557 insertions / 2821 deletions. This is the **"compile error fixed by deleting the call"**
commit the pattern-search was looking for. Every single WPE capability removal (S2–S4, S5–S6, S7–S16)
landed here in one sweep. Verbatim from `git show d3ae5996 -- ".../wickedcalls_part4.cpp"`:

```diff
 					case 5:
 					{
-						ec->SetVisible(true);
+						//ec->SetVisible(true); // SetVisible not in EmittedParticleSystem
 						break;
 					}
 					case 6:
 					{
-						ec->SetVisible(false);
+						//ec->SetVisible(false); // SetVisible not in EmittedParticleSystem
 						break;
 					}
 					case 7:
 					{
-						ec->SetEmitPaused(true);
+						//ec->SetEmitPaused(true); // SetEmitPaused not in EmittedParticleSystem
 						break;
 					}
 					case 8:
 					{
-						ec->SetEmitPaused(false);
+						//ec->SetEmitPaused(false); // SetEmitPaused not in EmittedParticleSystem
 						break;
 					}
```

```diff
 		//PE: If bFollowCamera , find InDoor , OutDoor , UnderWater.
 		//PE: bFindFloor ONLY if ec->bFollowCamera
-		if (ec && (ec->bFindFloor || ec->bFollowCamera))
+		//if (ec && (ec->bFindFloor || ec->bFollowCamera)) // bFindFloor/bFollowCamera not in EmittedParticleSystem
+		if (false) // disabled: bFindFloor/bFollowCamera not in EmittedParticleSystem
```

```diff
-							if (ec->bFollowCamera)
+							if (false) //if (ec->bFollowCamera) // bFollowCamera not in EmittedParticleSystem
...
-							if (ec->bFindFloor && ec->bFollowCamera)
+							if (false) //if (ec->bFindFloor && ec->bFollowCamera) // bFindFloor/bFollowCamera not in EmittedParticleSystem
```

```diff
 		wiEmittedParticle& ec = scene.emitters[i];
-		if (!ec.IsVisible())
-			continue;
-		if (!ec.IsActive())
-			continue;
+		//if (!ec.IsVisible()) // IsVisible not in EmittedParticleSystem
+		//	continue;
+		//if (!ec.IsActive()) // IsActive not in EmittedParticleSystem
+		//	continue;
 		total_visible++;
```

and in `wickedcalls_part3.cpp`:

```diff
 		//PE: Keeping this alive to keep serialized resources alive until entity serialization ends:
-		wiResourceManager::ResourceSerializer resource_seri;
-		if (archive.GetVersion() >= 63)
-		{
-			wiResourceManager::Serialize(archive, resource_seri);
-		}
+		//wiResourceManager::Serialize removed from API
+		//wiResourceManager::ResourceSerializer resource_seri;
+		//if (archive.GetVersion() >= 63)
+		//{
+		//	wiResourceManager::Serialize(archive, resource_seri);
+		//}
...
 		scene2.transforms.Serialize(archive, seri);
-		scene2.prev_transforms.Serialize(archive, seri);
+		//scene2.prev_transforms.Serialize(archive, seri); // removed, replaced by matrix_objects_prev
...
-		scene2.aabb_objects.Serialize(archive, seri);
+		//scene2.aabb_objects.Serialize(archive, seri); // aabb vectors no longer have Serialize
...
-			ec->SetVisible(false);
+			//ec->SetVisible(false); // SetVisible removed from EmittedParticleSystem
```

Note the commit message never mentions particles. The WPE capability loss was collateral inside a
741-error compile sweep — nobody made a decision to disable WPE; it was disabled one `//` at a time
to get a clean build.

### 4.2 `f189857b` — Lee Bamber, Sun Feb 15 16:07:32 2026 +0000

Full message and diff quoted in §0.4. Two days after `d3ae5996`, the symptom of the *real* problem
(35 blocking modal message boxes during Test Level) was suppressed rather than diagnosed. This is
the commit that turned "WPE effects throw errors" into "WPE effects silently do nothing", which is
why the breakage has been invisible since.

---

## 5. SUMMARY / WHAT A RE-ENABLE COSTS

Three independent layers must all be fixed; fixing any one alone changes nothing.

1. **Data layer.** `.PE` v5076 ≠ archive v93. Needs an offline converter or a legacy read path.
   Also needs `WickedCall_LoadWiSceneDirect`'s component read order restored/versioned (§0.6),
   and note `FLAG_EMIT_PAUSE`'s bit `1<<7` now means `FLAG_COLLIDERS_DISABLED` upstream, so raw
   `_flags` cannot be carried across.
2. **Engine layer.** ~35 GG-local members + 16 shader uniforms + the two-line emitter cull gate must
   be re-ported into `wi::EmittedParticleSystem` and the modern (bindless, push-constant) emit/simulate
   shaders. `distance_sort_bias` additionally needs the two `wiRenderer.cpp` sort sites.
   Some of it maps onto new upstream features (`fadein_time` → `opacityCurveControl*`;
   `bActive` → partially `IsInactive()`/`active_frames`).
3. **Game layer.** Un-comment S7–S15, and fix the pre-existing `>`-for-`<` bug at `part4.cpp:161`
   at the same time.

**Confidence labels:** §0.1–0.5, §1, §2, §3, §4 are **proof** (file contents, byte reads, commit
diffs). §0.6 (stream desync if the version gate were lifted) and the `FLAG_EMIT_PAUSE` bit-collision
consequence are **inference from code reading** — well-supported, but not observed at runtime because
nothing currently gets that far.
