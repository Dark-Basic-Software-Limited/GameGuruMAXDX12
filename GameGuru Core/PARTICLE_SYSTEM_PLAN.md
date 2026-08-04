# WPE Particle System — History, Post-Mortem and Upgrade Plan

**Investigation complete 2026-08-04. No code changed — this document is the plan.**

Scope: the **WPE particle system** — effects stored as `.PE` files in `Files\particlesbank\wpe`
and `Files\gamecore\decals\<name>\wpe.pe`. These are Wicked Engine emitter effects
(`wiEmittedParticle` in the DX11 fork, `wi::EmittedParticleSystem` today).

**Explicitly OUT of scope** — do not touch:

| System | Code | Data |
|---|---|---|
| Legacy hard-coded particles | `GameGuru\Source\M-Particles.cpp` (`ravey_particles_*`), DarkSDK `CParticleC` | — |
| Legacy GPU particles ("gpup") | `Guru-WickedMAX\GPUParticles*.cpp`, `Particles\Shaders\GPUP_*.hlsl` | `particlesbank\*.arx` + `_sx/_g/_r.png` |
| Decal system proper | `GameGuru\Source\M-Decal.cpp` | decal textures/projection |

---

## 1. Executive summary

**The WPE particle system is completely dead in DX12.** Not degraded — dead. Every `.PE`
file is rejected at its first 8 bytes and never reaches the engine.

The cause is a **version-space collision**, not a rendering bug. The DX11 GameGuru fork set
its archive version to **5077** so it could gate ~35 GameGuru-private emitter fields behind
versions 5072–5077. Shipped `.PE` files declare **5076** (25 files) or **5077** (2 files).
The modern engine's archive version is **93**, and it hard-rejects anything higher.

Nobody decided to switch it off. It happened in two commits, neither of which set out to:

1. **`d3ae5996`** (Lee Bamber, 2026-02-13, *"Fix all compilation errors for WickedEngine API
   migration (741 → 0 errors)"*) — the modern engine has none of the GameGuru emitter
   members, so every call to them was commented out to get a clean build. **16 separate
   capability removals landed in this one sweep**, each carrying a comment naming the missing
   symbol. The commit message never mentions particles.
2. **`f189857b`** (Lee Bamber, 2026-02-15) — the rejected archives were popping ~35 modal
   error boxes during Test Level, so a pre-flight version check was added that silently skips
   the files.

Step 2 turned a loud failure into a silent one, which is why this has been invisible for
almost six months.

**There is also a one-line bug worth fixing immediately, independent of everything else**
— see Phase 0a in §9. It is currently suppressing the *legacy* fallback too, so the 12 stock
decals that ship a `wpe.pe` render **no particle effect of either kind**.

---

## 2. Proof

**2.1 — The files.** All 27 `.PE` files in the build area carry a GameGuru-fork archive
version: 25 at **5076**, and `gamecore\decals\sparks\wpe.pe` + `gamecore\decals\splash_large\wpe.pe`
at **5077**. The reader must cover the whole 5072–5077 band, not just 5076.

**2.2 — The version spaces do not overlap.**

| Tree | File | Constant | Value |
|---|---|---|---|
| DX11 fork | `D:\max\WickedRepo\WickedEngine\wiArchive.cpp:7` | `__archiveVersion` | **5077** |
| DX12 engine | `D:\max\WickedEngineDX12\WickedEngine\wiArchive.cpp:18` | `__archiveVersion` | **93** |

`wiEmittedParticle::Serialize` in the fork gates on `GetVersion() >= 5072…5077`, each tagged
`//PE: Special ggm version.` (`WickedRepo\WickedEngine\wiEmittedParticle.cpp:953-1003`).

**2.3 — Measured in a live run.** The current build's `Guru-MapEditor.log` contains **47**
skip events from the last session:

```
40407 : Skipped non-Wicked archive: ...\Files\gamecore\decals\blood\wpe.pe (version=5076, expected 22-93)
```

Distinct effects skipped: `sparks` (15×), `dust` (6×), `blood` (6×), `splinters` (5×),
`splash_large` (5×), `impact` (5×), `explosion` (5×) — i.e. weapon impacts, blood,
explosions, sparks and water splashes.

Rejection path: `WickedEngineDX12\wiArchive.cpp:80-85` (message box + `Close()`),
short-circuited by the game's pre-check at `wickedcalls_part3.cpp:1961-1979`.
`WickedCall_LoadWPE` then merges an empty scene, sees `count_before == count_after`, and
**returns root 0**; every later `WickedCall_PerformEmitterAction(action, 0)` matches nothing.

---

## 3. History

| When | What |
|---|---|
| 2022-06-17 | `particle_editor.exe` built — the authoring tool shipped in `Tools\Particle Editor` |
| 2022-11-27 | `0988337c` — Wicked Engine vendored into `D:\max\WickedRepo` |
| 2022-12-06 | `c7e9b3b0` — "Finished revert to DX11" |
| 2024-12-02 | `983e8f7c` — Preben Eriksen, first large batch of GameGuru emitter extensions |
| 2025-01-07 | `eefbe334` — burst factors, `bFollowCamera`, `bFindFloor` |
| 2025-05-21 | `54b37150` — emitter timer, `bActive`, stat gating |
| 2025-06-15 | `c5031382` — spawn-randomness rework, `distance_sort_bias` (archive 5077) |
| 2025-06-20 | `d85fe6e8` — `FLAG_EMIT_PAUSE`, projectile-effect features |
| 2025-08-08 | `61a5407e` — last emitter change in the DX11 fork |
| 2025-11-23 | our DX12 clone's upstream fork point (Wicked 0.71.858) |
| **2026-02-13** | **`d3ae5996` — DX12 API migration; all WPE capabilities commented out** |
| **2026-02-15** | **`f189857b` — `.PE` files silently skipped to stop the message boxes** |
| 2026-02-01 | `842a0a0` — DX11 fork still receiving merges (`PE-2026-1`) |

**The DX11 fork is a live, moving target.** Pin parity work to a named `WickedRepo` commit.

---

## 4. What a `.PE` file is

A complete **Wicked scene archive**, not a particle-specific format. Loaded by
`WickedCall_LoadWPE` → `WickedCall_LoadWiScene` → `WickedCall_LoadWiSceneDirect` → `Scene::Merge`.

### 4.1 Container layout (verified byte-by-byte against `Steam.pe`)

Primitive widths (`WickedRepo\WickedEngine\wiArchive.h`) — note the 64-bit promotion, which is
the main trap:

| C++ type | Bytes | Note |
|---|---|---|
| `bool` | 4 | written as `uint32` |
| `int` / `unsigned int` / `uint32_t` | **8** | promoted to 64-bit |
| `size_t`, `Entity` | 8 | |
| `float` | 4 | |
| `XMFLOAT3` | 12 | |
| `std::string` | 8 + N | length **includes** the null terminator (modern engine excludes it) |

Stream order (`D:\max\GameGuruMAX\GameGuru Core\Guru-WickedMAX\wickedcalls.cpp:7543`):

```
uint64 version
uint64 reserved
resource block (version >= 63): uint64 count, then { string name, uint64 flags, uint64 len, bytes }
names, layers, transforms, prev_transforms, hierarchy, materials, meshes,
impostors, objects, aabb_objects, rigidbodies, softbodies, armatures, lights,
aabb_lights, cameras, probes, aabb_probes, forces, decals, aabb_decals,
animations, EMITTERS, hairs, weathers, sounds, inverse_kinematics, springs, animation_datas
```

Each manager is `uint64 count`, then `count` components, then `count` entity IDs.

> The DX12 port **deleted six of these reads** (`wiResourceManager::Serialize`,
> `prev_transforms`, four `aabb_*`). Even with the version gate lifted, the cursor would
> desynchronise from `hierarchy` onward.

### 4.2 The emitter record

296 bytes at v5076 (`WickedRepo\WickedEngine\wiEmittedParticle.cpp:896`):

```
u64  _flags, shaderType, meshID, MAX_PARTICLES
f32  FIXED_TIMESTEP, size, random_factor, normal_factor, count, life, random_life,
     scaleX, scaleY, rotation, motionBlurAmount, mass, SPH_h, SPH_K, SPH_p0, SPH_e
>=45   u64 framesX, framesY, frameCount, frameStart; f32 frameRate
>=64   f32x3 velocity; f32x3 gravity; f32 drag, random_color
>=5072 f32 restitution, fadein_time, burst_amount, burst_delay      <-- GameGuru from here
>=5073 f32 normal_factor_x, normal_factor_y, normal_factor_z
>=5074 f32 normal_random, rotation_random, size_random, spawn_random, scaling_random,
       spawn_pause, spawn_pause_random; u64 endcolor_red/green/blue;
       f32 burst_split, burst_factor_x/y/z
>=5075 f32x3 startpos; bool bFindFloor; f32 burst_factor_speed, start_rotation; bool bFollowCamera
>=5076 f32 random_position, random_position_scale
>=5077 f32 distance_sort_bias, wpe_filler_1..3
```

A working decoder is at `tools/particle_forensics/pe_decode.py`; it parses all 27 shipped files cleanly and
is the numeric gate for Phase 1.

### 4.3 The shipped corpus — measured, all 27 files / 70 emitters

**A `.PE` only ever contains 7 component types.** Across all 27 files, `meshes`, `impostors`,
`objects`, `rigidbodies`, `softbodies`, `armatures`, `lights`, `cameras`, `probes`, `forces`,
`decals`, `animations` and the four `aabb_*` managers are **all count-0** — a run of fifteen
zero `uint64`s before the emitters block. Only `names`, `layers`, `transforms`,
`prev_transforms`, `hierarchy`, `materials`, `emitters` (+ optional embedded texture blobs)
carry data.

*This is what makes a standalone parser realistic* — it needs to understand 7 component
serialisers, not 30. (Corollary: `emittedparticle_emitCS_FROMMESH` is never used by shipped
content, even though `meshID` is non-zero in 23/27 files — `SerializeEntity` remaps it to a
fresh entity that owns no mesh.)

Measured ground truth across the 70 emitters:

| Property | Observed |
|---|---|
| `drag` | **1.0 everywhere** |
| `FIXED_TIMESTEP` | **−1 everywhere** (variable timestep) |
| `random_position` / `random_position_scale` | **0 / 1 everywhere** |
| `mass`, `scaleY`, `SPH_*`, `spawn_pause*` | default everywhere |
| `scaleX` | **−4.12 … 30** — negative values are real; particles shrink through zero and invert |
| `shaderType` | 58 SOFT, 10 SOFT_DISTORTION, 2 SOFT_LIGHTING, **0 SIMPLE** |
| Blend mode | **44 ADDITIVE, 26 ALPHA** — nothing else |
| Sprite sheets | 1×1 (37), 8×8 (21), 5×5 (11), 20×4 (1) |
| Flags seen | 0, 4, 8, 12, 32, 36, 40, 44 — **SPH, FRAME_BLENDING, PAUSED, EMIT_PAUSE, DEBUG never appear** |

Consequences:

- **No shipped effect sets flag bit 7**, so the `FLAG_EMIT_PAUSE (1<<7)` ↔
  `FLAG_COLLIDERS_DISABLED (1<<7)` collision does not bite the shipped corpus — but it will
  bite user-authored effects, so remap explicitly.
- **`FRAME_BLENDING` is never used**, so the whole `uv2` / `frameBlend` second-sample half of
  the vertex and pixel shaders is dead in practice. Lower priority for the re-port.
- **SPH is never used** — skip it entirely, along with its 8 MB-per-emitter grid.
- **Multi-emitter effects are the norm** (up to 4). `WickedCall_LoadWPE` only grabs
  `emitters[count-1]` and `Restart()`s that one, relying on the shared hierarchy root.
- Texture naming follows `<peStem><N>_color.png` where N is the emitter index. Embedded
  resource paths name the authoring tool:
  `..\..\..\ParticleEditorBeta\ParticleEditor\files\tools\particlebank\Additive\scorch_03.png`.
- `distance_sort_bias` only exists in the 2 v5077 files, so it is effectively unused content.

---

## 5. The engine regression

The fork's `wiEmittedParticle.h` is 218 lines; `wi::EmittedParticleSystem` is 181. The fork
wrapped its additions in a commented `//#ifdef GGREDUCED` marker (h:113–179).

**~35 GameGuru members are absent from the modern engine:**

| Group | Members |
|---|---|
| Fade | `fadein_time` |
| Burst | `burst_amount`, `burst_split`, `burst_delay`, `burst_delay_timer`, `burst_factor_x/y/z`, `burst_factor_speed` |
| Direction | `normal_factor_x/y/z` |
| Randomisers | `normal_random`, `rotation_random`, `size_random`, `scaling_random`, `random_position`, `random_position_scale` |
| Emission rhythm | `spawn_random`, `spawn_pause`, `spawn_pause_random`, `randemit`, `randpause`, `total_emit_count` |
| Colour | `endcolor_red/green/blue` |
| Placement | `startpos`, `start_rotation`, **`bFindFloor`**, **`bFollowCamera`** |
| Environment | `bTriggerOutDoor/InDoor/UnderWater` |
| Render | `distance_sort_bias` (consumed at `WickedRepo\wiRenderer.cpp:6122,6252`) |
| State | `emittimer`, **`bVisible`**, **`bActive`**, `bStatActive`, `FLAG_EMIT_PAUSE` |

`restitution` was retuned **0.70** in the fork; the modern engine restores stock **0.98** —
bounciness differs even for effects that otherwise survive.

**All 16 GameGuru shader uniforms are gone** (zero references in the DX12 shader tree):
`xParticleEndColorRed/Green/Blue`, `xParticleNormalFactorX/Y/Z`, `xParticleNormalFactor2X/Y/Z`,
`xParticleStartRotation`, `xParticleRandomPos(Scale)`, `xParticleNormalRandom`,
`xParticleRotationRandom`, `xParticleSizeRandom`, `xParticleScalingRandom`,
`xParticleBurstFactorDpeed`, `xEmitterFadeinTime`, `xEmitterOpacity`, `xParticleSinPos`,
`xTotalEmitCount`, `color_mirror`.

**The renderer lost the cull gate.** Fork `wiRenderer.cpp:4357-4369` honoured `IsVisible()` /
`IsActive()`; DX12 `wiRenderer.cpp:4156-4164` keeps only `layerMask`. Un-commenting the
game-side calls achieves nothing on its own.

**One important conflation:** the modern engine has a single `random_factor` driving start
size, velocity jitter, start rotation *and* spin spread. The fork had `size_random`,
`rotation_random`, `normal_random`, `scaling_random`, `spawn_random` as independent knobs, and
the shipped effects tune them separately. No CPU-side trick recovers this — it needs an
`emitCS` + constant-buffer change.

### 5.1 The `Particle` struct — good news

Identical size and layout; only two fields changed meaning:

```
DX11 fork                          DX12
float rotationalVelocity;   -->    uint rotation_rotationVelocity;  (packed half2: start rot + rot vel)
uint  color_mirror;         -->    uint color;
```

The modern packed field is actually a natural home for the fork's `start_rotation`, which had
to be bolted on in the vertex shader.

---

## 6. The DX11 visual model — what "exactly the same" means

The GameGuru shader delta is small and concentrated: **~62 lines** across four shaders
(emit 30, VS 23, simulate 7, PS_soft 2).

**Emission** (`emittedparticle_emitCS.hlsl`). `rand()` is **stateful** (`globals.hlsli:148`,
`seed += 1` per call), so **the order of calls is part of the specification** — reordering them
changes every particle.
- Position jitter: `pos.xz += (rand(...) - 0.5) * random_position_scale`, seeded by
  `total_emit_count % random_position` (l.93-95). Volume emitters skip this.
- Orbital offset: `pos += float3(sin(t*burst_factor_speed + id)*sinPos.x, (rand-0.5)*sinPos.y, cos(t*burst_factor_speed + id)*sinPos.z)` (l.103).
- Size: `sizeBegin = size * (1 + (rand - 0.5) * size_random)` (l.105);
  `sizeEnd = sizeBegin * scaleX + (rand - 0.5) * scaling_random` (l.121).
- Velocity — three additive terms (l.112-115):
  ```
  v  = velocity_world + (nor + (rand3 - 0.5) * normal_random) * normal_factor
  v += (sin(2π·r)*A.x, (r-0.5)*A.y, cos(2π·r)*A.z)     // independent angles, NOT coherent
  v += (sin(DTid.x)*B.x, (r-0.5)*B.y, cos(DTid.x)*B.z) // thread-index ring
  ```
- Lifetime: `maxLife = life * (1 + (rand - 0.5) * random_life)` (l.119).
- Spin: `rotationalVelocity = rotation*π*60 + (rand - 0.5) * rotation_random` (l.118) — note
  the `×π×60` applied CPU-side.
- Tint: per-particle colour is `baseColorRGB8 & randomDarken8` — a **bitwise AND**, not a
  multiply (l.126-130).

> **TRAP — the constant-buffer names are crossed over.** In `wiEmittedParticle.cpp:355-363`,
> `xParticleNormalFactorX/Y/Z` is fed from **`burst_factor_*`**, and
> `xParticleNormalFactor2X/Y/Z` is fed from **`normal_factor_*`**. Reading the shader alone
> gives the mapping backwards. Verify against the CPU upload, not the HLSL identifier.

**Per-frame appearance** (`emittedparticleVS.hlsl`), `lifeLerp = 1 - life/maxLife`:
- `size = lerp(sizeBegin, sizeEnd, lifeLerp)`.
- `rotation = lifeLerp * rotationalVelocity` (l.27) — **a fraction of life, not elapsed time.**
- **Sprite sheet:** `frameRate == 0` → `lerp(frameStart, frameCount, lifeLerp)`; else
  `(frameStart + (maxLife - life) * frameRate) % frameCount` — the *"PE: Changed to always
  move flipbook forward"* fix (l.38-40).
- **Colour over life:** `lerp(startColourRGB8, endcolor_rgb, lifeLerp)` computed in **8-bit
  integer space in the VS** and passed as a `nointerpolation uint` (l.98-103). No ramp texture.
- Velocity-aligned rotation: `rotation += tan(normalize(viewVelocity)) * start_rotation` (l.56).
- Billboards are **pure view-space camera-facing quads** — never velocity-aligned or
  axis-locked. `size` is a half-extent.

**Simulation** (`emittedparticle_simulateCS.hlsl`) — semi-implicit Euler:
```
force += forceFieldTerms; force += gravity;
velocity += force * dt;  position += velocity * dt;  force = 0;
velocity *= drag;        // PER STEP, not pow(drag, dt) - framerate dependent
life -= dt;
```
GameGuru deltas: gravity applied *before* the depth-collision block, and the hard-coded bounce
constant `0.98` replaced by `xEmitterRestitution`. Depth collision reflects against the
**previous** frame's depth buffer through a 1.5-unit shell.

**CPU emission** (`wiEmittedParticle.cpp:200-297`): the stutter/gust model driven by
`spawn_random` via `randpause`/`randemit`; burst drain with `burst_delay` in **milliseconds**
and `burst_split` chunking; and an **auto-deactivate** — after `2 × worst-case lifetime` with
no emission, `SetActive(false)` drops the emitter from both simulate and draw. **That is how
fire-and-forget decal effects self-terminate**, and it depends on `bActive`, which the modern
engine does not have.

### 6.1 The single most likely thing to get wrong: the alpha curve

```hlsl
float opacity = saturate(lerp(1, 0, lifeLerp) * xEmitterOpacity);     // VS:26
float normalizedTime = clamp(lifeLerp / xEmitterFadeinTime, 0, 1);    // VS:86
opacity = saturate(lerp(0, opacity, normalizedTime));                 // VS:87
```

Two details that invert the intuition:

1. **`xEmitterOpacity` is `material.baseColor.w`, and shipped content routinely sets it far
   above 1 — values up to 20.** Because of the `saturate`, an opacity of 20 does not mean
   "20× brighter"; it means the particle sits at **alpha 1.0 for the first ~95 % of its life
   and then cliff-drops**. Treating this as a linear fade — which is what the formula looks
   like at a glance — produces a visibly different, softer effect for most of the corpus.
2. **`fadein_time` is a fraction of lifetime, not seconds.**

Final alpha is `saturate(texture.a * opacity * softDepthFade)`. Separately, emissive is
`color.rgb *= inputColor.rgb * (1 + xParticleEmissive)`, and shipped content pushes that as
high as **21×**. Any HDR/exposure difference between the two renderers will show up here first.

### 6.2 Blend states — the fork only ever initialised four

`wiEmittedParticle.cpp:816-893` builds blend states for **ALPHA, ADDITIVE, PREMULTIPLIED and
OPAQUE** only. `ALPHANOZ`, `FORCEDEPTH` and `MULTIPLY` fall through to a default
`BlendState` with **blending disabled**. Since the shipped corpus is 44 ADDITIVE / 26 ALPHA,
the DX12 port's mangled blend remapping matters less than it first appears — but the
*replacement* must reproduce "unlisted mode ⇒ blending off", not silently map it to ALPHA.

Depth: test **on** (`GREATER_EQUAL`, reverse-Z), write **off**, `CULL_NONE`,
`DepthClipEnable = false`. Soft fade is a 2×2 `GatherRed` of linear depth taking the **max**
tap, with width `1/size`.

### 6.3 Scope reducers — three things we must NOT "improve"

**Sprite mirroring never fires.** The emit shader writes:

```hlsl
particle.color_mirror |= ((rand(seed, uv) > 0.5f) << 31) & 0x10000000;
particle.color_mirror |= ((rand(seed, uv) < 0.5f) << 30) & 0x20000000;
```

`1 << 31` is `0x80000000`; `& 0x10000000` is **always 0**. Same for the second line. The VS
reads those bits (l.31-32) to flip the billboard, so the feature is dead in the shipping
product. Faithful parity means leaving it off.

**Neither engine has turbulence or noise.** A grep for `noise|turbulen|curl|vortex` across all
`emittedparticle*` shaders returns nothing in the DX11 fork, our clone, or upstream. The DX11
"swirl" look comes from the deterministic sin/cos orbital term. **Do not add curl noise** — it
is the largest engine gap and it is not needed for parity.

**Do not "fix" the divide-by-zero at emitCS l.93.** `random_position` is **0 in every shipped
effect**, so `xTotalEmitCount % 0` is an integer divide by zero. D3D specifies `0xFFFFFFFF`,
which makes both `rand()` inputs compile-time constants — the result is a *constant* (not
per-particle) XZ displacement bounded by ±0.5 world units, applied to every point emitter.
Removing the lines would shift every effect's origin slightly. Reproduce the behaviour, or
verify in a capture first. *(Flagged as the one item in this spec not confirmed on hardware.)*

**Dead fields — read by nothing in the fork:** `random_factor` (uploaded to
`xParticleRandomFactor` but no shader reads it — GameGuru replaced it with the separate
`*_random` knobs), `scaleY`, `mass` (stored but never divided out, so force *is* acceleration),
`spawn_pause`, `spawn_pause_random`, `wpe_filler_*`, and all three `bTrigger*` booleans (which
are not even serialised). Note that `random_factor` is precisely the single master knob the
modern engine still uses for four separate behaviours — reinstating the fork's independent
randomisers is therefore mandatory, not cosmetic.

---

## 7. Constraints

**7.1 — `.PE` is a live authoring format, not just legacy data.**
`Tools\Particle Editor\particle_editor.exe` (6 MB, 2022-06-17) ships with MAX, presence-gated
by `g_bParticleEditorPresent` (`master_part0.cpp:700`). It is an AppGameKit application (no
Wicked strings in the binary; `media/bytecode.byc`), **and its source is not anywhere on
disk.** It writes archive version 5076/5077 and will keep doing so.

→ **A one-time conversion of the shipped corpus is not sufficient.** The runtime must *read*
5072–5077 `.PE`, or every user's existing effect library and every future export breaks.

**7.2 — Do NOT raise `__archiveVersion`.** Version gates are compared numerically across ~40
component serializers engine-wide (materials, meshes, terrain…); moving the ceiling silently
changes the meaning of every `archive.GetVersion() >= N`. Additionally the v91/92 gates read a
thumbnail/properties header old `.PE` files do not contain (`wiArchive.cpp:86-97`).

**7.3 — If the emitters component-library version must be bumped, use 4, not 3.** Upstream has
already claimed **3** for its new `burst_on_create` field; colliding would corrupt any future
upstream pull.

**7.4 — Pulling upstream Wicked buys nothing for fidelity.** Direct comparison against
upstream master: `struct Particle`, `ParticleCounters`, `EmittedParticleCB`, the flags, the
blend states, and the whole simulate integrate/drag/rotation/sprite-frame/billboard block are
**verbatim identical** to ours. Post-fork additions are `burst_on_create` (convenience),
particle shadow-map casting (new orthogonal capability), a radix sort, and an indirect-args
refactor that **deletes** `PARTICLECOUNTER_OFFSET_*`. None of it helps sprite-effect parity;
the indirect-args refactor would actively cost us a rebase.

---

## 8. The `bWPE` boundary — where the line is drawn

`newparticletype::bWPE` (`Types.h:5493`, default `false`) is written in exactly one function,
`decal_load()` in `M-Decal.cpp` (`:221` false, `:253` true when
`gamecore\<name>\wpe.pe` exists and preload succeeded, `:262` false when preload rejected it).

> **`bWPE == true` ⟺ a DECAL that found a `wpe.pe`.** Every entity particle marker
> (`ismarker == 10`) is unconditionally the legacy `.arx` path. The single fork point is
> **`M-Entity_part5.cpp:239`** (`if (pParticle->bWPE) { …WPE… } else { …gpup… }`) with its
> creation-side twin at `:159`. A replacement backend touches only the `true` arms.

**Three WPE entry points bypass `newparticletype` entirely:**

1. **Weapon projectiles** — `WPE_Effect` / `WPE_Explosion` / `WPE_Root` (`Types.h:1407/1409/1562`),
   GUNSPEC keys `wpeeffect` / `wpeexplosion` (`M-Weapon.cpp:970-987`).
2. **LUA** — `vWickedEmitterEffects` (`DarkLUA_part5.cpp:997`).
3. **Editor preview** — `PreviewWPERoot` (`imgui_gg_dx11_part0.cpp:70`).

**Decal coupling is one-directional and shallow.** `M-Decal.cpp` only tests `FileExist`, sets
`bWPE`, calls `preload_wicked_particle_effect` / `newparticle_updateparticleemitter`, and
special-cases a 100 ms teardown. It never touches `wiEmittedParticle` or any `WickedCall_*`
symbol, so it can stay byte-identical provided the replacement preserves the
`preload_wicked_particle_effect(newparticletype*, int)` signature and its `false` = "fall
back to legacy" contract, `emitterid` as an opaque handle (`-1` none, `-2` force-reload), and
the `ready_decals[][]` / `decal_count[]` cache convention.

### 8.1 Other integration facts worth knowing

- **LUA API** (`DarkLUA_part7.cpp:1036-1042`): `WParticleEffectLoad`, `WParticleEffectPosition`,
  `WParticleEffectVisible`, `WParticleEffectAction`. **`WParticleEffectVisible` is a silent
  no-op** (`DarkLUA_part5.cpp:1192`), and **all five shipped scripts**
  (`Scripts\scriptbank\particles\wpe_area|wpe_blast|wpe_impact|wpe_rain|wpe_zone.lua`) use it
  for show/hide. Emitter actions 5/6/7/8 are likewise dead; only 1–4 work.
- **Editor:** there is no dedicated WPE panel. The "Particle Values" panel
  (`M-GridEditB_part12.cpp:1244-1795`) is the `.arx` panel. WPE rides the generic script
  property `wpefile` (`imgui_gg_dx11_part5.cpp:206-289`). The `effectlist` dropdown
  (`imgui_gg_dx11_part4.cpp:864-897`) is the only place `particlesbank\wpe` is enumerated and
  has **zero users**.
- **Paths:** `.PE` loads from the **build-area** tree. `GG_GetRealPath` redirects to Documents
  only when the file actually exists there (`CFileC_part0.cpp:400-417`);
  `Documents\...\Files\particlesbank` exists but is empty. There is no skybank-style special
  case — same machinery, different data.
- **Save/load:** `bWPE` and `emitterid` are **not persisted** (recomputed each session). The
  `.ele` `newparticletype` block is positional at versions **320/321** — any new field needs a
  new version guard or every later block desyncs.
- **Kill switch:** `setup.ini` key `disablewparticlesystem` → `g_iDisableWParticleSystem`.
- **Dead code:** `WickedCall_CreateEmitter` and `GetVisibleWEmitters` have zero callers;
  `CreateEmitter` is also broken (local `Scene scene;` shadows the global scene at `:209`, and
  `XMMATRIX& transformMatrix = XMMatrixIdentity();` binds a reference to a temporary).
- **Latent bug:** `M-Decal.cpp:541-545` computes a camera-facing yaw into
  `bParticle_LocalRot_Y` for WPE decals, but the WPE arm never reads it — the port took the
  *commented-out* older variant. DX11 `M-Entity.cpp:8834-8873` has a live
  `//PE: Make emitter always face camera.` block. Directional sprays will not orient to
  camera even after the format fix.
- **Memory:** ~212 bytes per particle. `downpour`/`heavy-rain3` request 25 000 → ~5.3 MB per
  emitter. SPH additionally allocates a flat 8 MB grid per emitter (no shipped effect uses SPH).

---

## 9. The upgrade plan

### Phase 0a — The one-line safety net (ship immediately, independent of everything else)

**Verified by reading `M-Entity_part5.cpp:2-140`.** In `preload_wicked_particle_effect`, the
`bWPE` branch calls `WickedCall_LoadWiScene` (`:46`), which the archive gate turns into a
no-op. So `count_before == count_after` (`:50`), the population block is skipped, `root` stays
`0`, the `if (root != 0)` block at `:91` never runs, and `pParticle->emitterid` remains `-1`.
Execution then falls through to an **unconditional `return true;` at `:139`.**

`decal_load()` reads that as success and leaves `bWPE == true` with `emitterid == -1`, which
**skips the `if (!bWPE)` legacy fallback at `M-Decal.cpp:266` and every legacy `else` branch**
— the 3–4 blood splats, the 8-element splash composite, `explosion_custom`.

**Consequence: the 12 stock decals that ship a `wpe.pe` currently render no particle effect of
either kind.**

The `false` = "fall back to legacy" contract already exists and is already used — `:125`
returns `false` when the effect turns out not to be a burst-only emitter, and `M-Decal.cpp:262`
responds by clearing `bWPE`. The fix simply honours the same contract for the load-failed case:

```cpp
// at the end of the bWPE branch, before the final return
if (pParticle->emitterid == -1)
    return false;   // nothing was created - let the caller fall back to the legacy .arx path
```

Note the existing early-out at `:10-13` (`g_iDisableWParticleSystem`) already returns `false`,
so this is consistent with how the function signals "not handled".

*Risk:* very low — it can only affect the path that currently produces nothing.
*Exit test:* firing a weapon produces impact/blood/spark effects again (legacy look), and the
`Skipped non-Wicked archive` log lines are accompanied by visible legacy effects.

### Phase 0b — Instrumentation and the parity bed

- The harness already reports `SCENE_EMITTERS` / `VISIBLE_EMITTERS` in `GET_PERF_DATA`
  (`AutomationHarness.cpp:1111,1151`). Add `LOAD_WPE <file>`, `EMITTER_ACTION <n>` and
  `DUMP_EMITTERS` (per-emitter field dump) so effects can be exercised headlessly.
- Build a parity level in TESTPRO1: one instance of each of the 14 `wpe` effects on a flat
  grey plane, fixed camera, fixed time-of-day.
- **Capture the DX11 reference before touching anything.** Build the DX11 product from
  `D:\max\GameGuruMAX` + `D:\max\WickedRepo` (pin the hash), load the parity level, record a
  fixed set of frames per effect at fixed simulation times. Without this, "looks the same" is
  unfalsifiable.

*Exit test:* DX12 reports `SCENE_EMITTERS: 0` on the parity level — today's truth, recorded.

### Phase 1 — Legacy `.PE` reader

Two viable designs. **Recommended: (B).**

**(A) Legacy band inside `wi::Archive`.** Accept 5000–5077, set a `legacy_gg` flag, treat the
effective upstream version as 64, skip the v91/92 header, restore the null-terminated string
convention, reinstate the six deleted component reads, and add the 5072–5077 blocks to
`EmittedParticleSystem::Serialize`.
*Downside:* touches the engine's shared archive semantics — the exact thing §7.2 warns about.

**(B) A standalone `.PE` parser in the game layer** (`wickedcalls`), which never touches
`wiArchive`. It reads the 5072–5077 layout directly (the format is fully documented in §4 and
already implemented in `pe_decode.py`), pulls out embedded resources, materials and emitter
records, and constructs the scene entities through the normal `Scene` API.
*Upside:* zero risk to every other serializer; no version-ceiling change; the DX11 format is
frozen so the parser never needs to track upstream. **Recommended.**

Either way: remap `_flags` explicitly — legacy bit 7 is `FLAG_EMIT_PAUSE`, which the modern
engine would read as `FLAG_COLLIDERS_DISABLED`. Retire the pre-flight skip at
`wickedcalls_part3.cpp:1961-1979` once the reader works (keep it for genuinely unknown versions).

*Risk:* medium, confined to load paths.
*Exit test — numeric, not visual:* `DUMP_EMITTERS` must match `pe_decode.py` field-for-field
across all 27 files. This is the POLYS-bit-identical equivalent for this work.

### Phase 2 — Re-port the emitter members (engine CPU side)

- Add the ~35 GameGuru members with the fork's defaults (including `restitution = 0.70`).
- Re-add `bVisible` / `bActive` / `FLAG_EMIT_PAUSE` (as a *new* free bit, not bit 7) and accessors.
- Restore the two cull-gate lines in `wiRenderer.cpp` and the `distance_sort_bias` term at the
  two sort sites.
- Port `UpdateCPU`'s GameGuru logic: burst delay/split countdown, spawn-pause/gust model,
  `total_emit_count`, `emittimer`.

*Risk:* medium; additive, stock behaviour unchanged at default values.
*Exit test:* effects load and emit; counts and lifetimes plausible.

### Phase 3 — Re-enable the game layer

Un-comment and repair the 16 stub sites (full list in `tools/particle_forensics/dx12_wpe_archaeology.md`):

- `wickedcalls_part4.cpp:42/47` — actions 5/6 (visible / not visible). Action 6 has 8 call
  sites; editor previews currently cannot be hidden.
- `:52/57` — actions 7/8 (pause emit / resume emit). Without these, weapon trails never stop
  emitting (`M-Weapon.cpp:1693`).
- `:152-200` — the `bFollowCamera` / `bFindFloor` block, currently `if (false)`. This is what
  makes rain, snow, downpour and dust volumes track the player.
- `wickedcalls_part3.cpp:2157` — `SetVisible(false)` on load, so effects do not pop on.
- `:303-306` — `GetVisibleWEmitters` currently returns the total.
- **Fix the loop bug** at `:161`: `for (int a = 0; a > parent_used.size(); a++)` — `>` should
  be `<`. Present in the DX11 original too (`GameGuruMAX\...\wickedcalls.cpp:7893`), so not a
  port regression, but it becomes reachable again.
- Restore the mangled blend-mode mappings (`BLENDMODE_FORCEDEPTH` → `OPAQUE`, and the
  `ALPHANOZ` → `ALPHA` remap that became a no-op).
- Restore the live camera-facing block from DX11 `M-Entity.cpp:8834-8873` (§8.1).
- Wire `WParticleEffectVisible` to a real implementation — all five shipped scripts depend on it.

*Risk:* low-medium; pure restoration.
*Exit test:* all 14 effects visible in the editor; preview show/hide works; rain follows the
camera; explosions fire once rather than looping.

### Phase 4 — Re-port the shader math (fidelity)

Re-implement the ~62 lines of GameGuru shader logic. The DX12 shaders were rearchitected
upstream (macro bindings → explicit `register(u0..u4)` + `PUSHCONSTANT` + bindless
`load_geometry()`), so **these cannot be cherry-picked — they must be rewritten** against §6.

- `emittedparticle_emitCS.hlsl`: random position, sin/cos orbital offset, the independent
  size/rotation/scaling/normal randomisers, the two additive normal-factor terms.
- `emittedparticleVS.hlsl`: fade-in, colour-over-life to `endcolor_*`, the forward-only
  flipbook rule, velocity-aligned rotation, `xEmitterOpacity`.
- `emittedparticle_simulateCS.hlsl`: gravity ordering, `xEmitterRestitution`.
- Extend `ShaderInterop_EmittedParticle.h` with the 16 uniforms.
- **Colour over life is cheap:** add `float3 xParticleEndColor` to `EmittedParticleCB` and lerp
  `particleColor.rgb` toward it by `lifeLerp` in `simulateCS.hlsl` just before the alpha
  multiply — two lines of HLSL, one CB field, no `Particle` growth, no VRAM cost.
- **The alpha curve is the highest-risk item in the entire plan** (§6.1). The modern
  `opacityCurveTex` (2048-entry `R16_UNORM`, sampled at `lifeLerp`) is a `smoothstep`
  trapezoid; DX11's is `saturate(linear × baseColor.w)` with `baseColor.w` up to **20**, which
  produces a near-flat-then-cliff shape, not a fade. Bake the DX11 formula into the curve
  texture (which needs no shader change at all) rather than approximating with peakStart /
  peakEnd. Verify numerically against `pe_decode.py` values — do not eyeball it.
- **Watch the crossed-over constant-buffer names** when porting the velocity terms — see the
  TRAP note in §6.
- **`rotation` is `lifeLerp × rotationalVelocity`** (fraction of life), and the CPU uploads
  `rotation × π × 60`. Both halves must be reproduced.
- **Do not implement sprite mirroring, do not add turbulence, and do not "fix" the
  `% random_position` divide-by-zero** (§6.3).
- Deprioritise `FRAME_BLENDING` (the `uv2` second-sample path) and SPH — no shipped effect
  uses either (§4.3).

*Risk:* **high** — this is where "looks slightly different" lives.
*Exit test:* frame-by-frame comparison against the Phase 0b DX11 captures.

### Phase 5 — Parity sweep and sign-off

- All 14 `wpe` effects and 12 decal effects against the DX11 captures.
- In-game triggers: weapon impact, blood, sparks, explosion, water splash, projectile trail.
- **Re-run the 19-demo VRAM sweep.** 27 effects newly loading textures (including the 5.4 MB
  `firearea` flipbook) is a real delta, and this campaign just spent weeks getting all 19 demos
  under 3450 MB. Budget for it; a lowvram lever may be needed.
- Re-run the FPS sweep — `downpour` and `heavy-rain3` request 25 000 particles each.

*Exit test:* user sign-off on visual parity; no demo regressed past the 4 GB gate.

---

## 10. Verification method

Two gates, deliberately different in kind:

1. **Numeric (phases 1–3).** `DUMP_EMITTERS` must match `pe_decode.py` field-for-field. Zero
   subjectivity; catches serialisation drift instantly.
2. **Visual (phases 4–5).** Fixed-camera, fixed-time captures against the Phase 0b DX11
   reference. Particle systems are stochastic, so compare at matched simulation times and
   judge envelope — extent, colour ramp, density falloff — rather than per-pixel equality.

---

## 11. Risks

| Risk | Severity | Mitigation |
|---|---|---|
| **Alpha curve reimplemented as a linear fade** — the natural reading of the formula, but wrong for most of the corpus because `baseColor.w` reaches 20 | **Highest** | Bake the exact DX11 formula into `opacityCurveTex`; check against measured per-effect `baseColor.w` |
| Phase 1 desync yields plausible garbage rather than failing | High | Numeric gate against `pe_decode.py` before trusting anything |
| Shader re-port lands "close but not identical" | High | Phase 0b reference captures; per-effect sign-off |
| Emissive up to 21× interacts with DX12 exposure/tonemapping differently | Medium | Compare bright additive effects (explosions, sparks) early, not last |
| `rand()` call order changed during the re-port, altering every particle | Medium | Port the emit shader statement-by-statement, preserving order (§6) |
| 27 effects' textures blow the 4 GB VRAM budget | Medium | Re-run the sweep in Phase 5; lowvram lever if needed |
| 25 000-particle rain costs FPS | Medium | FPS sweep; `MAX_PARTICLES` is already a knob |
| Particle Editor emits something unanticipated | Medium | Reader covers the whole 5072–5077 band |
| Future upstream pull collides with our emitter changes | Medium | Use component-library version **4** (§7.3); log every change in `WICKED_ENGINE_CHANGES.md` |

---

## 12. Recommendation

**Ship Phase 0a now** — one line, essentially no risk, and it restores *some* particle effect
to the 12 stock decals that currently show nothing at all.

Phases 0b–3 are worth doing regardless: mostly restoration of deleted code, numerically
verifiable, and they turn a silently dead subsystem back on. Phase 4 is the expensive,
judgement-heavy part, and it is where the "no embellishments, users expect their particles to
look the same" requirement actually bites.

**Do not start Phase 1 before Phase 0b's DX11 reference captures exist.** Everything after
depends on having something to compare against.

---

## Appendix A — field-by-field mapping, DX11 fork → modern engine

Action codes: **KEEP** = exists in both, nothing to do · **RETUNE** = exists but default or
semantics differ · **CPU** = add member + `UpdateCPU` logic, no shader change ·
**SHADER** = add member + constant-buffer field + HLSL · **SKIP** = dead in the fork, do not
port · **RENDER** = needs a `wiRenderer.cpp` change.

### Stock fields — present in both engines

| Field | Action | Note |
|---|---|---|
| `shaderType`, `meshID`, `MAX_PARTICLES`, `FIXED_TIMESTEP` | KEEP | |
| `size`, `count`, `life`, `random_life`, `scaleX` | KEEP | `scaleX` can be **negative** in the corpus (−4.12 … 30) |
| `normal_factor`, `motionBlurAmount` | KEEP | |
| `velocity`, `gravity`, `drag` | KEEP | `drag` is per-step in both; corpus is 1.0 everywhere |
| `framesX/Y`, `frameCount`, `frameStart`, `frameRate` | KEEP | `frameRate == 0` special case matches |
| `SPH_h/K/p0/e` | SKIP | no shipped effect enables SPH |
| `rotation` | RETUNE | fork uploads `×π×60`; VS applies `lifeLerp × rotVel`, not time-based |
| `restitution` | RETUNE | fork default **0.70**, modern **0.98** |
| `random_factor` | RETUNE | **dead in the fork**; the modern engine uses it as a single master knob for size + velocity + rotation + spin. Must be neutralised once the independent randomisers below are in |
| `scaleY`, `mass` | SKIP | dead in the fork (`scaleY` never uploaded; `mass` never divided out) |
| `_flags` | RETUNE | remap legacy bit 7 (`FLAG_EMIT_PAUSE`) away from `FLAG_COLLIDERS_DISABLED` |

### GameGuru fields — absent from the modern engine

| Field | Action | Where it is consumed |
|---|---|---|
| `fadein_time` | SHADER | VS alpha ramp — see §6.1, highest-risk item |
| `endcolor_red/green/blue` | SHADER | VS colour-over-life lerp in 8-bit space |
| `normal_factor_x/y/z` | SHADER | emit velocity term 2 (**uploaded as `xParticleNormalFactor2*`**) |
| `burst_factor_x/y/z` | SHADER | emit velocity term 3 (**uploaded as `xParticleNormalFactor*`**) |
| `burst_factor_speed` | SHADER | phase rate of the sin/cos orbital offset |
| `normal_random` | SHADER | emit velocity jitter |
| `rotation_random` | SHADER | emit spin spread |
| `size_random` | SHADER | emit start size |
| `scaling_random` | SHADER | emit end size |
| `start_rotation` | SHADER | VS velocity-aligned rotation (see caveat below) |
| `random_position`, `random_position_scale` | SHADER | emit XZ jitter — **0/1 in all shipped content**; preserve the divide-by-zero behaviour (§6.3) |
| `startpos` | SHADER | emitter-local spawn offset |
| `burst_amount`, `burst_split`, `burst_delay`, `burst_delay_timer` | CPU | `UpdateCPU` burst drain; `burst_delay` is in **milliseconds** |
| `spawn_random`, `randemit`, `randpause` | CPU | the gust/stutter emission model |
| `total_emit_count` | CPU | seeds the emit-time position jitter |
| `emittimer` + `SetTimer`/`GetTimer` | CPU | lifetime bookkeeping |
| `bActive` + auto-deactivate | CPU + RENDER | **load-bearing** — after 2× worst-case lifetime with no emission the emitter drops out of simulate *and* draw. This is how fire-and-forget decal effects self-terminate |
| `bVisible` | CPU + RENDER | restore the cull-gate line in `wiRenderer.cpp` |
| `FLAG_EMIT_PAUSE` + `IsEmitPaused`/`SetEmitPaused` | CPU | "stop spawning, let live particles die"; **allocate a new free bit, not bit 7** |
| `bFindFloor`, `bFollowCamera` | CPU (game-side) | consumed by `WickedCall_UpdateEmitters`, not the engine — can live in the game layer |
| `distance_sort_bias` | RENDER | two sort sites; only present in the 2 v5077 files |
| `bStatActive` | SKIP | statistics opt-in only |
| `spawn_pause`, `spawn_pause_random` | SKIP | dead in the fork |
| `bTriggerOutDoor/InDoor/UnderWater` | SKIP | dead — never even serialised |
| `wpe_filler_1/2/3` | SKIP | reserved padding |

**Caveat on `start_rotation`:** `rotation += tan(normalize(velocity)) * xParticleStartRotation`
takes `tan()` of a `float3` and implicitly truncates to `.x`. Almost certainly a bug, but only
**one** shipped emitter sets the field, so there is no strong visual reference either way.
Reproduce it verbatim and move on.

### Summary of effort

- **SHADER: 14 fields** — concentrated in `emitCS` (~30 lines) and the VS (~23 lines).
- **CPU: 12 fields** — `UpdateCPU`, plus 2 render-gate lines and 2 sort-site lines.
- **SKIP: 9 fields** — genuinely dead; porting them would be wasted work and added risk.

---

## Appendix B — implementation landmines found while shipping this (2026-08-04)

Three divergences that are invisible in code review and only show up as "the effect loads,
simulates, reports visible and active — and you see nothing".

### B.1 `Component_Attach` re-parents the child transform (cost: the whole preview path)

The DX11 loader deserialised `hierarchy` as **raw data** — `parentID` plus `layerMask_bind` —
and never touched the child's transform. The obvious DX12 equivalent, `scene.Component_Attach(child, parent)`,
defaults to `child_already_in_local_space = false`, which multiplies the child's local
transform by `inverse(parent.world)`.

That looks harmless because it cancels out at load time. It does not stay harmless: **the
`.PE` files carry a leftover authoring position on the root** — `firearea.pe` and `Steam.pe`
are both at `(330, 241.55, -18265)`, `downpour.pe` at `(-993, 288, -19312)`. So the attach
baked a `(-330, -241.55, +18265)` offset into the emitter's local transform. Everything looked
correct until the editor preview repositioned the root onto the selected entity — at which
point the particles rendered **18,265 units away** and the user saw an empty scene.

**Always pass `child_already_in_local_space = true` when attaching transforms that came
straight out of an archive.**

### B.2 `Burst(0)` means "fire burst_amount", not "burst zero particles"

DX11 fork:
```cpp
void wiEmittedParticle::Burst(int num)
{
    if (IsPaused()) return;
    if (num <= 0) num = burst_amount;   // <-- upstream has no equivalent
    burst_delay_timer = burst_delay;
    burst += num;
}
```
Upstream is a plain `burst += num`. The editor preview, `WParticleEffectAction(1)` and the
weapon/decal paths **all call `Burst(0)`**, so on stock upstream every burst-only effect
(explosions, blood, impacts — the ones with `count == 0`) fired nothing at all.

### B.3 The emission model is not `emit += count * dt`

The fork's `UpdateCPU` carries a gust/stutter model (`spawn_random` → `randpause`/`randemit`)
and a fractional burst release (`burst_split`, `burst_delay` in **milliseconds**). Both were
ported verbatim; with `spawn_random == 0` and `burst_split == 0` they reduce exactly to
upstream's behaviour, so nothing stock changes. Do not "simplify" them — the shipped effects
were tuned against these exact curves.

### Diagnosing this class of bug

`DUMP_EMITTERS` (added to the harness) prints, per emitter: the render gates (`vis`/`act`),
the **world** position, the live GPU `alive` count, and the material's blend mode / basecolor
alpha / texture validity. That one line separates "not loading" from "not simulating" from
"simulating somewhere you cannot see" — which is what this bug actually was.
