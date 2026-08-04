# Wicked Engine emitted-particle system — capability map

Scope: our vendored DX12 clone `D:\max\WickedEngineDX12` (engine version **0.71.858**, `WickedEngine\wiVersion.cpp:9-13`), compared to upstream `turanszkij/WickedEngine` master as of Aug 2026, and cross-referenced against the DX11-era GameGuru fork `D:\max\WickedRepo` (read-only) where it bears on the gap list.

Everything below is read from source. Nothing was run. Statements I could not verify are marked **UNVERIFIED**.

---

## 0. Executive summary of the blocking findings

1. **`.PE` files cannot be opened by our DX12 engine at all today.** The DX11 fork stamps its scene archives with `__archiveVersion = 5077` (`D:\max\WickedRepo\WickedEngine\wiArchive.cpp:7`). Our clone's `__archiveVersion` is **93** (`D:\max\WickedEngineDX12\WickedEngine\wiArchive.cpp:18`) and it hard-rejects any archive whose header version exceeds its own. Empirically, all 21 readable shipped `.pe` files in `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\Files\` carry header version **5076** (19 files) or **5077** (2 files). See §5.
2. **No turbulence / noise / curl-noise of any kind exists** in Wicked's emitter — not in our clone, not upstream. `grep -i "noise|turbulen|curl|vortex"` across `WickedEngine\shaders\emittedparticle*` and `ShaderInterop_EmittedParticle.h` returns zero hits. This is an engine-modification gap, not a tuning gap.
3. **Colour-over-life does not exist.** Only *opacity*-over-life exists, and only as a fixed trapezoid shape (smoothstep ramp-up / flat / smoothstep ramp-down) driven by two scalars. RGB is frozen at emit time. See §1.5 and §2.4.
4. **Size-over-life is a single linear lerp** from `size` to `size * scaleX`. No curve. See §2.3.
5. Sprite-sheet animation, per-effect textures, one-shot bursts, looping emission, and additive/alpha/premultiplied/multiply/inverse blending are all **native and exact**. See §1.9, §1.11, §3.

---

## 1. Full feature inventory — `EmittedParticleSystem` (= the "EmitterComponent")

There is no separate `EmitterComponent` type. The component *is* `wi::EmittedParticleSystem`, registered directly into the scene's component library:

```cpp
wi::ecs::ComponentManager<EmittedParticleSystem>& emitters =
    componentLibrary.Register<EmittedParticleSystem>("wi::scene::Scene::emitters", 2); // version = 2
```
`WickedEngine\wiScene.h:61`

An emitter entity **must** have a `TransformComponent` and (in practice) a `MaterialComponent`; a `MeshComponent` is optional and referenced by `meshID`. `Scene::Entity_CreateEmitter` (`WickedEngine\wiScene.cpp:1586-1604`) creates name + emitter (`count = 10`) + transform + `materials.Create(entity).userBlendMode = BLENDMODE_ALPHA`.

The scene update system force-overrides the material every frame (`WickedEngine\wiScene.cpp:5860-5875`):

```cpp
if (!material->IsUsingVertexColors()) material->SetUseVertexColors(true);
if (emitter.shaderType == SOFT_LIGHTING) material->shaderType = MaterialComponent::SHADERTYPE_PBR;
else                                     material->shaderType = MaterialComponent::SHADERTYPE_UNLIT;
```

So the emitter's *texture* is the material's texture set (BASECOLORMAP, and NORMALMAP), its *tint* is `material.GetBaseColor()`, its *blend mode* is `material.userBlendMode`, and its emissive boost is `material.GetEmissive()`.

### 1.1 Serialized scalar fields (declarations at `wiEmittedParticle.h:105-137`)

| Field | Type | Default | Units / meaning | Where consumed |
|---|---|---|---|---|
| `FIXED_TIMESTEP` | float | `-1.0f` | `-1` = variable timestep (uses `GetFrame().delta_time`); `>= 0` = forced fixed step in seconds. Editor slider range `-1 … 0.016`. | `cb.xEmitterFixedTimestep`, `simulateCS.hlsl:40` |
| `size` | float | `1.0f` | Starting particle radius, **world units** (billboard is `±size` before view rotation, i.e. `size` is the half-extent). Editor `0.01 … 10`. | `emitCS.hlsl:170` |
| `random_factor` | float | `1.0f` | Master randomness multiplier `[0..1]`. Drives *four separate* randomisations: start size, velocity jitter, start rotation, and rotation-velocity spread. There is no independent knob for any of them. | `emitCS.hlsl:170,177,178` |
| `normal_factor` | float | `1.0f` | Multiplier on the (surface-normal + jitter) velocity term, world units/sec. Editor `0 … 100`. | `emitCS.hlsl:177` |
| `count` | float | `0.0f` | **Emission rate in particles per second** (continuous / looping emission). Editor `0 … 10000`. | `wiEmittedParticle.cpp:343` |
| `life` | float | `1.0f` | Lifespan in **seconds**. Editor `0 … 100`. | `cb.xParticleLifeSpan` |
| `random_life` | float | `1.0f` | Lifespan randomness, fractional. Editor `0 … 2`. | `emitCS.hlsl:179` |
| `scaleX` | float | `1.0f` | **Size-over-life end multiplier.** Labelled "Scaling" in the editor. `sizeEnd = sizeBegin * scaleX`. Editor `0 … 100`. | `cb.xParticleScaling`, `emitCS.hlsl:181` |
| `scaleY` | float | `1.0f` | **DEAD FIELD.** Serialized (`wiEmittedParticle.cpp:1124,1202`) and Lua-exposed (`wiScene_BindLua.cpp:5207,5231`) but never written into the constant buffer and never read by any shader. Verified by grep: the only non-serialization/non-Lua hits for `scaleY` in the engine are `wiSprite`, an unrelated type. |
| `rotation` | float | `0.0f` | **Rotation *velocity*** (not a start angle), in half-turns/sec: `cb.xParticleRotation = rotation * XM_PI` (`wiEmittedParticle.cpp:486`), then per-particle spin `= xParticleRotation * (rand-0.5) * (1 + random_factor)`. Editor `0 … 1`, tooltip "Set the rotation velocity". **There is no start-rotation control** — start angle is always `(rand-0.5) * random_factor * 2π`. |
| `motionBlurAmount` | float | `0.0f` | Stretches the billboard along the view-space velocity vector. Editor `0 … 1`. | `simulateCS.hlsl:322` |
| `mass` | float | `1.0f` | Per-particle mass. **Only used by the SPH solver.** The non-SPH integrator ignores it entirely (`force` is added to `velocity` directly with no `/mass`). Editor `0.1 … 100`. |
| `random_color` | float | `0` | Per-channel RGB randomisation at emit: `c *= lerp(1, rand, random_color)`. Editor `0 … 2` (values > 1 extrapolate). |
| `opacityCurveControlPeakStart` | float | `0.1f` | Fade-in end, as a fraction of lifetime `[0..1]`. |
| `opacityCurveControlPeakEnd` | float | `0.5f` | Fade-out start, as a fraction of lifetime `[0..1]`. Clamped `>= peakStart`. |
| `velocity` | XMFLOAT3 | `{0,0,0}` | Initial velocity of every new particle, **in emitter local space** — transformed by `XMVector3TransformNormal(velocity, worldMatrix)` (`wiEmittedParticle.cpp:498`). World units/sec. |
| `gravity` | XMFLOAT3 | `{0,0,0}` | Constant acceleration, **world space**, world units/sec². Added to `force` every step. |
| `drag` | float | `1.0f` | **Per-simulation-step velocity multiplier** — *not* per second. `1.0` = no drag. Framerate-dependent unless `FIXED_TIMESTEP >= 0`. Editor `0 … 1`. |
| `restitution` | float | `0.98f` | Bounce multiplier, **only applied on depth-buffer collisions** (`simulateCS.hlsl:216`). Collider-entity bounces use a plain `reflect()` with **no** restitution. Editor `0 … 1`. |
| `SPH_h` | float | `1.0f` | SPH smoothing radius. |
| `SPH_K` | float | `250.0f` | SPH pressure constant. |
| `SPH_p0` | float | `1.0f` | SPH reference density. |
| `SPH_e` | float | `0.018f` | SPH viscosity constant. |
| `framesX` | uint32 | `1` | Sprite-sheet columns. |
| `framesY` | uint32 | `1` | Sprite-sheet rows. |
| `frameCount` | uint32 | `1` | Total frames used in the animation. |
| `frameStart` | uint32 | `0` | Starting frame index. **Constant, not random** — see §5 gap. |
| `frameRate` | float | `0` | Frames per second. **`0` is special: it means "play exactly one pass of the animation across the particle lifetime"** (tooltip, `Editor\EmitterWindow.cpp:213`). |
| `MAX_PARTICLES` (private) | uint32 | `1000` | Pool size. `SetMaxParticleCount` / `GetMaxParticleCount`. Editor `100 … 1,000,000`. |
| `meshID` | Entity | `INVALID_ENTITY` | Optional mesh emitter source. |
| `shaderType` | enum | `SOFT` | See §1.10. |

Non-serialized: `center` (emitter world position, used for the CPU per-emitter sort), `layerMask` (taken from the entity's `LayerComponent`), `worldMatrix`.

### 1.2 Flags (`wiEmittedParticle.h:85-98`)

```cpp
FLAG_EMPTY               = 0,
FLAG_DEBUG               = 1 << 0,   // debug visualiser (editor only)
FLAG_PAUSED              = 1 << 1,   // freeze the whole simulation
FLAG_SORTING             = 1 << 2,   // per-particle GPU back-to-front sort
FLAG_DEPTHCOLLISION      = 1 << 3,   // screen-space depth-buffer collision
FLAG_SPH_FLUIDSIMULATION = 1 << 4,   // SPH fluid solver
FLAG_HAS_VOLUME          = 1 << 5,   // emit inside a box volume instead of a point
FLAG_FRAME_BLENDING      = 1 << 6,   // cross-fade between sprite-sheet frames
FLAG_COLLIDERS_DISABLED  = 1 << 7,   // ignore scene ColliderComponents
FLAG_USE_RAIN_BLOCKER    = 1 << 8,   // internal, used by the built-in rain emitter
FLAG_TAKE_COLOR_FROM_MESH= 1 << 9,   // sample emitter mesh material/vcol/basecolor at emit
```

Note `FLAG_PAUSED` freezes *everything* (existing particles stop moving), because `UpdateCPU` and `UpdateGPU` both early-out on `IsPaused()` (`wiEmittedParticle.cpp:334, 437`). **There is no "stop emitting but let existing particles finish" flag** in our clone. The DX11 fork had exactly that: `FLAG_EMIT_PAUSE = 1 << 7` + `SetEmitPaused` (`D:\max\WickedRepo\WickedEngine\wiEmittedParticle.h:74, 205`). See §5.

### 1.3 Emitter shapes

- **Point** (default): `emitPos = 0`, then `mul(worldMatrix, float4(0,0,0,1))` — all particles start exactly at the transform origin (`emitCS.hlsl:156, 161`).
- **Volume** (`FLAG_HAS_VOLUME`, `emittedparticle_emitCS_volume.hlsl` → `#define EMITTER_VOLUME`): `emitPos = float3(rand*2-1, rand*2-1, rand*2-1)` then world-transformed (`emitCS.hlsl:153`). This is a **uniformly-filled box** of half-extent = the transform's scale. Not a sphere, not a shell, not a cone, not a disc.
- **Mesh** (`meshID` set, `emittedparticle_emitCS_FROMMESH.hlsl` → `#define EMIT_FROM_MESH`): picks a random geometry subset at LOD 0, then a random triangle, then random barycentrics with the standard `if (f+g > 1) { f = 1-f; g = 1-g; }` fold (`emitCS.hlsl:39-93`). Note this is **uniform over triangles, not over surface area** — large triangles are under-sampled. Surface normal seeds the velocity, and if `geometry.vb_pre` exists the surface motion `(emitPos - pre)` is added to the velocity (`emitCS.hlsl:102-112`). With `FLAG_TAKE_COLOR_FROM_MESH` it also multiplies in the mesh material base colour, vertex colour, and a `SampleLevel` of the mesh BASECOLORMAP at the emit UV (`emitCS.hlsl:114-147`).

### 1.4 Bursts

Three overloads (`wiEmittedParticle.h:72-74`):
```cpp
void Burst(int num);
void Burst(int num, const XMFLOAT3& position, const wi::Color& color = wi::Color::White());
void Burst(int num, const XMFLOAT4X4& transform, const wi::Color& color = wi::Color::White());
```
The plain `Burst(num)` adds to a `burst` accumulator folded into the next frame's normal emit location. The positional/matrix overloads push a **separate `EmitLocation`** with its own transform and **its own vertex colour** (`wiEmittedParticle.cpp:388-420`), each dispatched as its own `emitCS` dispatch (`wiEmittedParticle.cpp:576-581`). Note `location.transform` is *pre-multiplied by the emitter world matrix* (`wiEmittedParticle.cpp:440-448`), so burst transforms are emitter-relative.

`Restart()` (`wiEmittedParticle.cpp:421-425`) unpauses and nulls `counterBuffer`, which forces a rebuild with `aliveCount = 0, deadCount = MAX_PARTICLES` — i.e. instantly kills all live particles. **Warning:** it does *not* clear the vertex buffers, and it does not reset `emit`.

### 1.5 Colour / alpha handling

- **RGB is decided once, at emit, and never changes.** `emitCS.hlsl:35, 183-186`:
  ```hlsl
  float4 baseColor = EmitterGetMaterial().GetBaseColor() * unpack_rgba(location.color);
  ...
  baseColor.r *= lerp(1, rng.next_float(), xParticleRandomColorFactor);
  baseColor.g *= lerp(1, rng.next_float(), xParticleRandomColorFactor);
  baseColor.b *= lerp(1, rng.next_float(), xParticleRandomColorFactor);
  particle.color = pack_rgba(baseColor);   // RGBA8, stored in Particle::color
  ```
- **Alpha over life** is the only life-varying colour term. `simulateCS.hlsl:50, 281-283`:
  ```hlsl
  const float lifeOpa = opacityCurveTex.SampleLevel(sampler_linear_clamp, lifeLerp, 0);
  ...
  float opacity = saturate(lifeOpa * EmitterGetMaterial().GetBaseColor().a);
  float4 particleColor = unpack_rgba(particle.color);
  particleColor.a *= opacity;
  ```
- The pixel shader then multiplies the texture by the per-vertex colour (`emittedparticlePS_soft.hlsl:86`):
  ```hlsl
  color.rgb *= inputColor.rgb * (1 + material.GetEmissive());
  ```

**Consequence:** a colour ramp over life (fire orange → smoke grey, say) is *not* expressible. `endcolor_red/green/blue` existed in the DX11 fork (`D:\max\WickedRepo\WickedEngine\wiEmittedParticle.h:139-141`) — that is a GameGuru-local extension, not upstream Wicked.

### 1.6 Rotation / spin

- Packed as two halves in one uint: `Particle::rotation_rotationVelocity` (`ShaderInterop_EmittedParticle.h:12`).
- Start angle: `(rand - 0.5) * random_factor * 2π` radians — **always random, scaled by the shared `random_factor`**. Cannot be set to a fixed angle, and cannot be de-randomised without also de-randomising size/velocity/spin.
- Spin rate: `xParticleRotation * (rand - 0.5) * (1 + random_factor)` rad/sec, where `xParticleRotation = rotation * π`. Symmetric about zero, so half the particles spin each way.

### 1.7 Velocity, gravity, drag, mass

`emitCS.hlsl:177`:
```hlsl
particle.velocity = velocity + (nor + (float3(rng.next_float(), rng.next_float(), rng.next_float()) - 0.5f) * xParticleRandomFactor) * xParticleNormalFactor;
```
where `velocity` starts as the world-rotated `xParticleVelocity` (+ mesh surface motion, if any) and `nor` is the emitter-surface normal (zero for point/volume emitters).

Note the parenthesisation: for a **point or volume** emitter `nor == 0`, so the jitter term becomes `(rand3 - 0.5) * random_factor * normal_factor`. The jitter is therefore a **cube** in velocity space (not a sphere) and is scaled by `normal_factor` even though `normal_factor` is documented as a *surface-normal* factor. There is no separate speed-randomness knob.

`mass` never enters the non-SPH integration path — see §2.2.

### 1.8 Collision

Three independent mechanisms, all optional:

1. **Scene entity colliders and force fields** (`simulateCS.hlsl:94-183`), iterated per-particle over `forces()` entity list:
   - `ENTITY_TYPE_FORCEFIELD_POINT` — `force += dir * gravity * (1 - saturate(dist/range))`, linear falloff. Attractor if `gravity > 0`, deflector if negative (`wiScene_Components.h:1608`).
   - `ENTITY_TYPE_FORCEFIELD_PLANE` — `force += entity.GetDirection() * gravity * (1 - saturate(dist/range))`.
   - `ENTITY_TYPE_COLLIDER_SPHERE`, `_CAPSULE`, `_PLANE` — hard `reflect()` + penetration push-out. **No restitution applied here.**
   - Gated by `layerMask` and by `FLAG_COLLIDERS_DISABLED`.
2. **Depth-buffer collision** (`FLAG_DEPTHCOLLISION`, `#define DEPTHCOLLISIONS`, `simulateCS.hlsl:186-220`). Reprojects into the *previous* frame's depth history, reconstructs a surface normal from three depth taps, and bounces with `reflect(velocity, N) * xEmitterRestitution`. Hard-coded `surfaceThickness = 1.5f` world units.
3. **SPH self-collision** (`FLAG_SPH_FLUIDSIMULATION`) — see §2.6. Note `SPH_FLOOR_COLLISION` and `SPH_BOX_COLLISION` are **commented out** at `simulateCS.hlsl:28-29`.

### 1.9 Sprite-sheet animation

Fully native. `framesX × framesY` grid, `frameCount` frames used, `frameStart` offset, `frameRate` fps, plus `FLAG_FRAME_BLENDING` for a cross-fade between the current and next frame. Two modes — see the exact formula in §2.5. The VS/MS also compute `frameBlend = frac(spriteframe)` and the PS lerps two texture samples (`emittedparticlePS_soft.hlsl:41-45`). Frame blending applies to the normal map too (`:63-68`).

### 1.10 Shader types (`wiEmittedParticle.h:24-31`)

```cpp
enum PARTICLESHADERTYPE : uint32_t { SOFT, SOFT_DISTORTION, SIMPLE, SOFT_LIGHTING, PARTICLESHADERTYPE_COUNT };
```
- `SOFT` — textured, unlit, with depth fade. `emittedparticlePS_soft.hlsl`.
- `SOFT_DISTORTION` — same file with `#define EMITTEDPARTICLE_DISTORTION`; samples the **NORMALMAP** slot instead of BASECOLORMAP (`emittedparticlePS_soft.hlsl:8-12`), signed-biases it (`color.rgb -= 0.5`), and is rendered into a **separate distortion render target** in its own pass, later consumed by the tonemapper (`wiRenderPath3D.cpp:2417-2465, 2634`).
- `SIMPLE` — literally a flat grey constant, no texture at all:
  ```hlsl
  float4 main() : SV_TARGET { return float4(0.8f, 0.8f, 0.8f, 1.0f); }
  ```
  `emittedparticlePS_simple.hlsl`
- `SOFT_LIGHTING` — `#define EMITTEDPARTICLE_LIGHTING`; runs `TiledLighting` on a fabricated hemispherical normal + ambient + fog (`emittedparticlePS_soft.hlsl:94-139`). Forces the material to `SHADERTYPE_PBR`.

### 1.11 Blend modes — see §3.

### 1.12 Lighting / shadow interaction

- **Only `SOFT_LIGHTING` is lit.** The others are unlit; they only get `material.GetEmissive()` as a brightness multiplier and a saturation matrix.
- The fabricated normal for `SOFT_LIGHTING` (`emittedparticlePS_soft.hlsl:99-105`):
  ```hlsl
  N.x = -cos(PI * input.unrotated_uv.x);
  N.y =  cos(PI * input.unrotated_uv.y);
  N.z = -sin(PI * length(input.unrotated_uv));
  N.xz += normal.rg;                       // optional normal map perturbation
  N = mul((float3x3)GetCamera().inverse_view, N);
  ```
- **Particles do NOT cast raster shadow-map shadows in our clone.** `DrawSoftParticles` is only called from `RenderPath3D::RenderTransparents` (`wiRenderPath3D.cpp:2403, 2461`) and from the reflection pass (`:1559`). There is no shadow-pass PSO in `EmittedParticleSystem::Initialize` and no shadow pixel shader. **Upstream has since added exactly this** — see §4.
- **Raytraced shadows: yes, if RT is on.** Every emitter gets a BLAS (`wiEmittedParticle.cpp:274-301`) and a TLAS instance (`wiScene.cpp:5918-5946`), with `material->IsCastingShadow()` clearing `raytracing_inclusion_mask_shadow` when off. RT hits are shaded with `simple_lighting` because of `SHADERMESH_FLAG_EMITTEDPARTICLE` (`shaders\surfaceHF.hlsli:380-381`).
- Particles are **visible in planar reflections** (`wiRenderPath3D.cpp:1559`).

### 1.13 Sorting

Two levels:
1. **Per-emitter, CPU, always on** (`wiRenderer.cpp:6651-6668`). Distance from the camera is packed into the high 16 bits of a hash as a half float and sorted `std::greater` → farthest emitter drawn first.
2. **Per-particle, GPU, opt-in** (`FLAG_SORTING`). `simulateCS.hlsl:347-352` writes `distanceBuffer[prevCount] = -distSQ` (negated so the ascending bitonic sort produces back-to-front), then `wi::gpusortlib::Sort(...)` runs over the culled count (`wiEmittedParticle.cpp:776-779`). Editor tooltip: "This might slow down performance."

Frustum culling is per-particle, in the simulate CS (`simulateCS.hlsl:334-345`), producing `culledIndirectionBuffer` / `culledIndirectionBuffer2` and driving the indirect draw's instance count.

### 1.14 Memory footprint (relevant to the VRAM campaign)

Per particle, from `CreateSelfBuffers` (`wiEmittedParticle.cpp:58-215`) and the vertex format sizes in `wiScene_Components.h`:

| Buffer | Bytes/particle |
|---|---|
| `particleBuffer` (`sizeof(Particle)` = 12+4+12+4+12+4+8+4+4) | 64 |
| `aliveList[0]`, `aliveList[1]` | 8 |
| `deadList` | 4 |
| `culledIndirectionBuffer`, `culledIndirectionBuffer2` | 8 |
| `vb_pos` — `Vertex_POS32W` (16 B) × 4 verts | 64 |
| `vb_nor` — `Vertex_NOR` (4 B, R8G8B8A8_SNORM) × 4 | 16 |
| `vb_uvs` — `Vertex_UVS` (8 B, R16G16B16A16_UNORM) × 4 | 32 |
| `vb_col` — `Vertex_COL` (4 B, R8G8B8A8_UNORM) × 4 | 16 |
| `distanceBuffer` (only if `FLAG_SORTING`) | +4 |
| `densityBuffer` (only if SPH) | +4 |
| **Total** | **≈ 212 B** (216 sorted, 220 SPH+sorted) |

Plus fixed per-emitter overhead: `counterBuffer`, `indirectBuffers`, `constantBuffer` (`sizeof(EmittedParticleCB)`), a 2048×`R16_UNORM` opacity-curve 1D texture (4 KB), and `GraphicsDevice::GetBufferCount()` readback buffers. If SPH is enabled, `sphGridCells` is `SPH_PARTITION_BUCKET_COUNT (128*128*64 = 1,048,576) × sizeof(SPHGridCell) (8 B)` = **8 MB flat per SPH emitter** (`ShaderInterop_EmittedParticle.h:114`, `wiEmittedParticle.cpp:192-199`).

Default `MAX_PARTICLES = 1000` → ~212 KB. Editor max 1,000,000 → ~212 MB per emitter. `GetMemorySizeInBytes()` (`wiEmittedParticle.cpp:303-327`) reports the true total.

---

## 2. The simulation pipeline

### 2.0 Frame order

**CPU (`wi::scene::Scene::Update` → `RunEmitterUpdateSystem`, `wiScene.cpp:5853-5948`)**
material/layer fixup → `emitter.UpdateCPU(transform, dt)` → publish `ShaderGeometry` + `ShaderMeshInstance` + TLAS instance.

**GPU update (`wiRenderer.cpp:6247-6263`)** — inside the main render, before the transparent pass:
```cpp
for (uint32_t emitterIndex : vis.visibleEmitters) { ... emitter.UpdateGPU(instanceIndex, mesh, cmd); }
```

**GPU draw** — `wiRenderPath3D::RenderTransparents`:
- after opaque, after `DrawScene(DRAWSCENE_TRANSPARENT)`, after debug/wireframe/light visualizers:
  `wi::renderer::DrawSoftParticles(visibility_main, false, cmd)` (`wiRenderPath3D.cpp:2403`)
- then a **separate distortion render pass** (own RT, `LoadOp::CLEAR`, main depth buffer bound read-only): `DrawSoftParticles(visibility_main, true, cmd)` (`:2461`).

So: **transparent pass, after all transparent geometry, before lens flares and post-processing.**

### 2.1 Compute pipeline

Per emitter, in `UpdateGPU` (`wiEmittedParticle.cpp:427-837`), all under `EventBegin("UpdateEmittedParticles")`:

1. **Upload emit locations** — `AllocateGPU(sizeof(EmitLocation) * emit_locations.size())`, each location's transform pre-multiplied by the emitter world matrix.
2. **Fill + upload `EmittedParticleCB`**, barrier `COPY_DST → CONSTANT_BUFFER`.
3. **Bind 13 UAVs** (u0..u12): particleBuffer, aliveList[0], aliveList[1], deadList, counterBuffer, indirectBuffers, distanceBuffer, vb_pos, vb_nor, vb_uvs, vb_col, culledIndirection, culledIndirection2.
4. **Emit** — one `Dispatch((location.count + 63) / 64, 1, 1)` **per emit location**, with the buffer offset as a push constant. Shader is `emitCS` / `emitCS_VOLUME` / `emitCS_FROMMESH`. Pops from `deadList`, pushes to `aliveList[0]`, increments `ALIVECOUNT_AFTERSIMULATION`.
5. **KickOff** — `kickoffUpdateCS`, `Dispatch(1,1,1)`. Writes the simulate dispatch args and rolls the counters:
   ```hlsl
   indirectBuffers.Store3(ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION,
       uint3((aliveCount + THREADCOUNT_SIMULATION - 1) / THREADCOUNT_SIMULATION, 1, 1));
   counterBuffer.Store(PARTICLECOUNTER_OFFSET_ALIVECOUNT, aliveCount);
   counterBuffer.Store(PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION, 0);
   counterBuffer.Store<int>(PARTICLECOUNTER_OFFSET_DEADCOUNT, max(0, deadCount));
   counterBuffer.Store(PARTICLECOUNTER_OFFSET_CULLEDCOUNT, 0);
   counterBuffer.Store(PARTICLECOUNTER_OFFSET_CELLALLOCATOR, 0);
   ```
   `emittedparticle_kickoffUpdateCS.hlsl:10-26`. Barrier `UNORDERED_ACCESS → INDIRECT_ARGUMENT`.
6. **(optional) SPH chain** — 5 dispatches: `sphpartitionCS`, `sphcellallocationCS`, `sphbinningCS`, `sphdensityCS`, `sphforceCS`, all `DispatchIndirect` off `ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION` except cell-allocation which is a fixed `Dispatch(SPH_PARTITION_BUCKET_COUNT / 64)`.
7. **Simulate** — one of four variants (`simulateCS`, `_SORTING`, `_DEPTHCOLLISIONS`, `_SORTING_DEPTHCOLLISIONS`; the last three are one-line `#define` wrappers around the first), `DispatchIndirect`. Binds `opacityCurveTex` at `t0`. `THREADCOUNT_SIMULATION = 64` (because `SPH_USE_ACCELERATION_GRID` is defined; would be 256 otherwise — `ShaderInterop_EmittedParticle.h:116-120`).
8. **(optional) `wi::gpusortlib::Sort(MAX_PARTICLES, distanceBuffer, counterBuffer, PARTICLECOUNTER_OFFSET_CULLEDCOUNT, culledIndirectionBuffer, cmd)`.**
9. **FinishUpdate** — `finishUpdateCS`, `Dispatch(1,1,1)`. Turns the culled count into the indirect **draw** args:
   ```hlsl
   IndirectDrawArgsInstanced args;
   args.VertexCountPerInstance = 4;
   args.InstanceCount = particleCount;   // = culledCount
   args.StartVertexLocation = 0;
   args.StartInstanceLocation = 0;
   ```
   `emittedparticle_finishUpdateCS.hlsl:28-33`. (Mesh-shader branch instead writes `ThreadGroupCountX = (particleCount + 31) / 32`.)
10. **Statistics readback** — `CopyResource(statisticsReadbackBuffer[GetBufferIndex()], counterBuffer)`; `UpdateCPU` memcpy's it back with a frame-count delay.

**Draw** (`wiEmittedParticle.cpp:840-879`): binds the PSO for `[material.GetBlendMode()][shaderType]`, binds counterBuffer/particleBuffer/culledIndirection×2 as SRVs t0..t3, then `DrawInstancedIndirect(&indirectBuffers, ARGUMENTBUFFER_OFFSET_DRAWPARTICLES)`. Topology is `TRIANGLESTRIP` with 4 vertices per instance — one particle = one instance = one quad. The VS reads the *precomputed* vertex buffers written by the simulate CS; it does not expand the billboard itself.

`ALLOW_MESH_SHADER` is hard-coded `false` (`wiEmittedParticle.cpp:49`), so `emittedparticleMS.hlsl` is dead code today. (It also looks buggy — `Out.clip = dot(Out.pos, ...)` reads `Out.pos` *before* it is assigned, and it uses `particle.life * frameRate` rather than `(maxLife - life) * frameRate`.)

### 2.2 Emission accumulator (CPU) — `wiEmittedParticle.cpp:339-355`

```cpp
emit = std::max(0.0f, emit - std::floor(emit));   // carry only the fraction
center = transform.GetPosition();
emit += (float)count * dt;                         // count = particles/second
emit += burst;
burst = 0;
if ((uint)emit > 0)
{
    EmitLocation& location = emit_locations.emplace_back();
    location.transform.init();
    location.count = (uint)emit;
    location.color = wi::Color::White();
    active_frames |= 1;
}
```
Fractional carry means low rates are honoured exactly over time. `active_frames` is a shift register (`active_frames <<= 1` each frame, `|= 1` when there is work or live particles); `IsInactive()` is `active_frames == 0`, and an inactive emitter skips both `UpdateGPU` and `Draw`.

### 2.3 Per-particle integration (GPU) — `emittedparticle_simulateCS.hlsl:40-62`

```hlsl
const float dt = xEmitterFixedTimestep >= 0 ? xEmitterFixedTimestep : GetFrame().delta_time;
...
particle.life -= dt;

const float lifeLerp = 1 - particle.life / particle.maxLife;
const float particleSize = lerp(particle.sizeBeginEnd.x, particle.sizeBeginEnd.y, lifeLerp);
const float lifeOpa = opacityCurveTex.SampleLevel(sampler_linear_clamp, lifeLerp, 0);

// integrate:
particle.force += xParticleGravity;
particle.velocity += particle.force * dt;
particle.position += particle.velocity * dt;

// reset force for next frame:
particle.force = 0;

// drag:
particle.velocity *= xParticleDrag;
```

Semi-implicit (symplectic) Euler. Note precisely:
- `mass` does **not** divide the force. `mass` is only read by the SPH force shader.
- `drag` is applied once per *step*, so effective per-second drag is `drag^(1/dt)` — **framerate dependent** unless `FIXED_TIMESTEP >= 0`.
- `force` is a per-frame accumulator zeroed every step; force fields and SPH write into it before the integrate.
- `lifeLerp` is 0 at birth and 1 at death.

**Size over life:** `size(t) = lerp(sizeBegin, sizeEnd, lifeLerp)` where `sizeBegin = size + size*(rand-0.5)*random_factor` and `sizeEnd = sizeBegin * scaleX`. Strictly linear; there is no size curve.

**Rotation:** `emittedparticle_simulateCS.hlsl:266-268`
```hlsl
float2 rotation_rotationVel = unpack_half2(particle.rotation_rotationVelocity);
rotation_rotationVel.x += rotation_rotationVel.y * dt;
particle.rotation_rotationVelocity = pack_half2(rotation_rotationVel);
```
Half-precision, so the angle loses resolution as it accumulates past a few tens of radians. **UNVERIFIED** whether this is visible in practice.

**Kill path:** when `life <= 0`, the index is pushed to `deadBuffer` and all four vertex positions are zeroed (`:363-366`) — degenerate quad, so no stale geometry.

### 2.4 Opacity over life — the curve texture

CPU-baked into a 2048-entry `R16_UNORM` 1D texture (`wiEmittedParticle.cpp:881-922`):

```cpp
uint16_t data[2048];
int startup_length = int(peakStart * float(arraysize(data) - 1));
for (int i = 0; i < startup_length; ++i) {
    float t = smoothstep(0.0f, 1.0f, float(i) / (startup_length - 1));
    data[i] = uint16_t(t * 65535);
}
int keep_length = int((peakEnd - peakStart) * float(arraysize(data) - 1));
for (int i = 0; i < keep_length; ++i)
    data[i + startup_length] = uint16_t(65535);
for (int i = 0; i < (arraysize(data) - startup_length - keep_length); ++i) {
    float t = smoothstep(1.0f, 0.0f, float(i) / (arraysize(data) - startup_length - keep_length - 1));
    data[i + startup_length + keep_length] = uint16_t(t * 65535);
}
```

So the alpha envelope is exactly:

- `x ∈ [0, peakStart)`  → `smoothstep(0,1, x/peakStart)`  = `3u² − 2u³`, `u = x/peakStart`
- `x ∈ [peakStart, peakEnd)` → `1`
- `x ∈ [peakEnd, 1]` → `smoothstep(1,0, u)` = `1 − (3u² − 2u³)`, `u = (x − peakEnd)/(1 − peakEnd)`

Sampled with `sampler_linear_clamp` at `lifeLerp`, then `opacity = saturate(lifeOpa * material.baseColor.a)`.

**Only two degrees of freedom.** No arbitrary keyframed alpha curve.

**Latent divide-by-zero:** when `startup_length == 1` (i.e. `peakStart` in roughly `(0, 0.00073]`), the ramp-up loop divides by `startup_length - 1 == 0`. Same shape in the ramp-down if `startup_length + keep_length == 2047`. This code is **identical upstream**, so it is not a local regression. **UNVERIFIED** whether it produces a visible artefact — `uint16_t(inf * 65535)` is UB.

### 2.5 Sprite-sheet frame advance — `emittedparticle_simulateCS.hlsl:291-299`

```hlsl
// Sprite sheet frame:
const bool anim_over_lifetime = xEmitterFrameRate == 0;
const float spriteframe = anim_over_lifetime ?
    lerp(xEmitterFrameStart, xEmitterFrameCount, lifeLerp) :
    ((xEmitterFrameStart + (particle.maxLife - particle.life) * xEmitterFrameRate) % xEmitterFrameCount);
const uint currentFrame = floor(spriteframe);
const uint nextFrame = anim_over_lifetime ? min(ceil(spriteframe), xEmitterFrameCount - 1) : (uint(ceil(spriteframe)) % xEmitterFrameCount); // anim_over_lifetime doesn't wrap around
uint2 offset  = uint2(currentFrame % xEmitterFramesXY.x, currentFrame / xEmitterFramesXY.x);
uint2 offset2 = uint2(nextFrame    % xEmitterFramesXY.x, nextFrame    / xEmitterFramesXY.x);
```

Two exact modes:
- **`frameRate == 0` (one pass over lifetime):** `spriteframe = lerp(frameStart, frameCount, lifeLerp)` — note it lerps *to `frameCount`*, not `frameCount - 1`, so the last frame is only reached at the instant of death and `nextFrame` is clamped.
- **`frameRate > 0` (looping):** `spriteframe = (frameStart + age * frameRate) mod frameCount` where `age = maxLife - life`. `nextFrame` wraps.

UV mapping, `simulateCS.hlsl:305-312`:
```hlsl
float2 uv = quadPos.xy * float2(0.5f, -0.5f) + 0.5f;
float2 uv2 = uv;
uv.xy  += offset;   uv.xy  *= xEmitterTexMul;   // xEmitterTexMul = (1/framesX, 1/framesY)
uv2.xy += offset2;  uv2.xy *= xEmitterTexMul;
```
`xEmitterTexMul` is set on the CPU at `wiEmittedParticle.cpp:494`. `uv` goes to `.xy` of the UVS vertex attribute, `uv2` to `.zw`; the PS blends between them by `frameBlend = frac(spriteframe)` when `FLAG_FRAME_BLENDING` is set.

### 2.6 Billboard construction — `emittedparticle_simulateCS.hlsl:301-332`

```hlsl
float3 quadPos = BILLBOARD[vertexID];             // (±1, ±1, 0)
...
quadPos.xy = mul(quadPos.xy, rot);                // rot = 2x2 rotation by rotation_rotationVel.x
quadPos *= particleSize;
float3 velocity = mul((float3x3)GetCamera().view, particle.velocity);
quadPos += dot(quadPos, velocity) * velocity * xParticleMotionBlurAmount;   // motion blur stretch
quadPos = mul(quadPos, (float3x3)GetCamera().view);   // reversed mul == inverse camera rotation
vertexBuffer_POS[v0 + vertexID] = float4(particle.position + quadPos, 0);
vertexBuffer_NOR[v0 + vertexID] = float4(normalize(-GetCamera().forward), 0);
vertexBuffer_UVS[v0 + vertexID] = float4(uv, uv2);
vertexBuffer_COL[v0 + vertexID] = particleColor;
```

Always a **camera-facing screen-aligned billboard**. There is no velocity-aligned / stretched / axis-locked / horizontal billboard mode; the motion-blur term is a *non-uniform stretch of the camera-facing quad*, not a re-orientation. Note the motion-blur stretch is not normalised — `dot(quadPos, velocity) * velocity` scales with `|velocity|²`, so at high speed it explodes.

### 2.7 SPH

`SPH_USE_ACCELERATION_GRID` is defined, so SPH uses a spatially hashed grid (`SPH_GridHash`, three large primes, `ShaderInterop_EmittedParticle.h:122-130`) of `128*128*64` buckets, with a 27-neighbour stencil. Poly6 / Spiky / viscosity kernel constants are precomputed CPU-side (`wiEmittedParticle.cpp:530-541`):
```cpp
cb.xSPH_poly6_constant = (315.0f / (64.0f * XM_PI * h9));
cb.xSPH_spiky_constant = (-45.0f / (XM_PI * h6));
cb.xSPH_visc_constant  = ( 45.0f / (XM_PI * h6));
```
Not relevant to a legacy sprite-effect system; noted for completeness (and for its 8 MB/emitter grid cost).

---

## 3. Blend / render states

Specified **entirely by the material**, not by the emitter: `PSO[material.GetBlendMode()][shaderType]` (`wiEmittedParticle.cpp:853-854`). `MaterialComponent::GetBlendMode()` (`wiScene_Components.h:293`) returns `userBlendMode`, except that `BLENDMODE_OPAQUE` is upgraded to `BLENDMODE_ALPHA` when the material's filter mask says transparent.

Six modes (`wiEnums.h:9-15`), all built in `Initialize()` (`wiEmittedParticle.cpp:1040-1099`):

| Mode | src / dest (colour) | src / dest (alpha) | op |
|---|---|---|---|
| `BLENDMODE_ALPHA` | `SRC_ALPHA` / `INV_SRC_ALPHA` | `ONE` / `INV_SRC_ALPHA` | ADD |
| `BLENDMODE_ADDITIVE` | `SRC_ALPHA` / `ONE` | `ZERO` / `ONE` | ADD |
| `BLENDMODE_PREMULTIPLIED` | `ONE` / `INV_SRC_ALPHA` | `ONE` / `ONE` | ADD |
| `BLENDMODE_MULTIPLY` | `DEST_COLOR` / `ZERO` | `DEST_ALPHA` / `ZERO` | ADD |
| `BLENDMODE_INVERSE` | `INV_DEST_COLOR` / `ZERO` | `DEST_ALPHA` / `ZERO` | ADD |
| `BLENDMODE_OPAQUE` | blend disabled | — | — |

**Additive: yes.** **Premultiplied: yes.** Both native, no engine change needed.

**Depth** (`wiEmittedParticle.cpp:1032-1037`):
```cpp
dsd.depth_enable = true;
dsd.depth_write_mask = DepthWriteMask::ZERO;      // depth TEST yes, depth WRITE no
dsd.depth_func = ComparisonFunc::GREATER_EQUAL;   // reverse-Z
dsd.stencil_enable = false;
```

**Raster** (`:1007-1017`): solid fill, `CullMode::NONE`, `front_counter_clockwise = true`, **`depth_clip_enable = false`** (so particles straddling the near plane are not clipped), no MSAA-specific handling.

Also note `[earlydepthstencil]` on the soft PS (`emittedparticlePS_soft.hlsl:14`).

**Soft particles / depth fade: present.** `emittedparticlePS_soft.hlsl:76-84`:
```hlsl
[branch]
if (GetCamera().texture_lineardepth_index >= 0)
{
    float4 depthScene = texture_lineardepth.GatherRed(sampler_linear_clamp, ScreenCoord) * GetCamera().z_far;
    float depthFragment = input.pos.w;
    opacity *= saturate(1.0 / input.size * (max(max(depthScene.x, depthScene.y), max(depthScene.z, depthScene.w)) - depthFragment));
}
opacity = saturate(opacity);
```

Falloff is **linear**, and the fade distance is the particle's own current world-space size — larger particles fade over a longer distance. A `GatherRed` + `max` of the 4 taps is used (conservative: takes the farthest of the 4 neighbours). There is **no user knob** for the soft-fade distance; it is hard-wired to `1.0 / size`.

---

## 4. Upstream delta

### 4.1 Where our clone sits

- `wi::version` in the clone is **0.71.858** (`wiVersion.cpp:9-13`).
- The newest genuinely-upstream commit in the clone is `2f681cb7` "looks like exceptions were not disabled properly, now they are", **2025-11-23**. So we forked from upstream master around **late November 2025**.
- The newest upstream commit touching `wiEmittedParticle.cpp` that we carry is `334e5dbc` "added inverse blend mode (#1124)", 2025-06-11; for the `.h` it is `6e4ca467` "force enum to uint32_t for all compilers via enum-base (#1166)", 2025-07-13.
- **No local GGMAX delta touches any emitter file.** `git log` over `wiEmittedParticle.{cpp,h}`, `emittedparticle_simulateCS.hlsl`, `emittedparticle_emitCS.hlsl` filtered to non-upstream authors returns only `6e4ca467` (Dennis Brakhane, an upstream PR). The emitter subsystem is **stock upstream-as-of-Nov-2025**.

### 4.2 Upstream commits to `wiEmittedParticle.cpp` after our fork point

Fetched from `https://github.com/turanszkij/WickedEngine/commits/master/WickedEngine/wiEmittedParticle.cpp`:

| Date | Commit title | We have it? |
|---|---|---|
| 2026-07-14 | CreateBufferClearedWithType helper | No |
| 2026-06-14 | added "burst on create" for emitters | No |
| 2026-03-08 | Radix sort (#1579) | No |
| 2026-02-20 | **emitted particle shadowmap support (#1558)** | No |
| 2026-02-08 | Emitter, surfelgi, GPU sort refactor (#1541) | No |
| 2026-02-07 | emitter and surfelgi indirect arguments refactor | No |
| 2026-01-11 | Mac OS support (#1346) | No |
| 2025-12-13 | particles can also use wireframe overlay render mode | No |
| (and everything older) | — | Yes |

### 4.3 What changed, concretely

**(a) `burst_on_create` — one-shot effects that fire themselves.** Upstream `wiEmittedParticle.h` now has:
```cpp
int burst_on_create = 0;
```
and `UpdateCPU` gained:
```cpp
if (!bursted_on_create && burst_on_create > 0)
{
    bursted_on_create = true;
    Burst(burst_on_create);
}
```
Serialized behind `seri.GetVersion() >= 3` (upstream bumped the emitters component-library version from 2 to 3). **Directly relevant to one-shot burst effects** — it means an authored emitter asset can carry "spawn N particles the first frame you exist" without any game-side code. We would otherwise call `Burst(n)` from C++ after load, which is what `WickedCall_LoadWPE` is already positioned to do.

**(b) Shadow-map casting for particles (#1558).** New upstream files `emittedparticleVS_shadow.hlsl` and `emittedparticlePS_shadow.hlsl` (confirmed present via the GitHub contents API), new statics `vertexShader_shadow`, `shadowPS`, `blendState_shadow`, `rasterizerState_shadow`, `depthStencilState_shadow`, `PSO_shadow`, and a new public method:
```cpp
void DrawForShadowmap(const wi::scene::MaterialComponent& material, wi::graphics::CommandList cmd) const;
```
which early-outs on `!material.IsCastingShadow()` and draws with `offsetof(EmitterIndirectArgs, draw_all)` — i.e. **all alive particles, not the main-camera-culled subset**. The shadow PS writes a coloured-transmission value:
```hlsl
clip(opacity - 1.0 / 255.0);
color.rgb = lerp(1, color.rgb, opacity);
color.rgb *= 1 - opacity;
color.a = input.pos.z; // secondary depth
```
with `blendState_shadow` = `src ZERO / dest SRC_COLOR`, alpha `ONE/ONE` with `BlendOp::MAX` — a multiplicative coloured shadow accumulation. Also depth bias is applied in `rasterizerState_shadow`, branching on `IsFormatUnorm(wi::renderer::format_depthbuffer_shadowmap)`.

**Is it needed for particle-effect fidelity?** Only if the legacy effects were expected to darken the world (smoke plumes shadowing the ground). If the legacy system was pure additive/alpha billboards that never cast, this is optional. I could not determine what the legacy system did here — **UNVERIFIED**.

**(c) Indirect-argument refactor (#1541 / 2026-02-07).** Upstream's `ShaderInterop_EmittedParticle.h` **no longer contains** `PARTICLECOUNTER_OFFSET_*` or `ARGUMENTBUFFER_OFFSET_*` constants; they were replaced by a typed struct:
```c
struct EmitterIndirectArgs
{
    IndirectDispatchArgs dispatch;
    IndirectDrawArgsInstanced draw_culled;
    IndirectDrawArgsInstanced draw_all;
};
```
addressed with `offsetof(EmitterIndirectArgs, draw_culled)` etc. Note the extra `draw_all` slot exists specifically so the shadow pass can draw uncullled. This is a **plumbing** change, no behavioural difference for the culled main-camera draw.

**(d) Radix sort (#1579).** Upstream replaced/augmented `wi::gpusortlib` with a radix sort. Performance only, same ordering semantics. **UNVERIFIED** whether it changes tie-breaking.

**(e) Wireframe overlay.** Upstream `Draw` now uses `wi::renderer::GetWireframeMode()` with `WIREFRAME_ONLY` / `WIREFRAME_OVERLAY`, issuing a second indirect draw in overlay mode. Ours uses the older `wi::renderer::IsWireRender()` and has only the "wireframe replaces everything" path. Editor-only.

**(f) Mac OS support / `CreateBufferClearedWithType`.** Portability and buffer-creation plumbing. No behaviour change on Windows/DX12.

### 4.4 What did **NOT** change upstream

I fetched and byte-compared the important shader and interop content against ours:

- **`ShaderInterop_EmittedParticle.h`**: `struct Particle` — identical. `struct ParticleCounters` — identical. `EMITTER_OPTION_BIT_*` — identical (still 6 bits, no new ones). `EmittedParticleCB` — **identical field-for-field**, same order, same names. So no new simulation parameter has been added upstream at all.
- **`emittedparticle_simulateCS.hlsl`**: the integrate block, the drag, the rotation update, the sprite-sheet frame block and the billboard expansion loop are **verbatim identical** to ours. The only visible difference is ordering (upstream hoists the rotation update above the sprite-frame block) and the absence of an inline comment on the `nextFrame` line.
- `PARTICLESHADERTYPE` enum — identical, still 4 types.
- `FLAGS` enum — identical, still 10 flags, no new ones.
- `SetOpacityCurveControl` — **verbatim identical**, including the latent `startup_length - 1` divide-by-zero.
- Blend states — identical for all 6 modes; upstream only *added* `blendState_shadow`.

**Conclusion on the upstream delta: there is nothing upstream that we lack which would materially improve particle-effect fidelity, except `burst_on_create` (a convenience) and shadow-map casting (a new capability, orthogonal to sprite fidelity).** The simulation math is frozen. If Wicked's emitter cannot reproduce the legacy system today, pulling upstream will not change that.

---

## 5. Serialization — what happens to a `.PE` file

### 5.1 Our clone's `Serialize` (`wiEmittedParticle.cpp:1108-1243`)

Two independent version sources:
- `archive.GetVersion()` — the **file-format version** written into the archive header.
- `seri.GetVersion()` — the **per-component-manager version**, propagated from `ComponentLibrary::Register<T>(name, version)` (`wiECS.h:41, 51-56, 700-705`). For emitters this is **2** in our clone (`wiScene.h:61`) and **3** upstream.

Unconditional prefix, in this exact order:
```
_flags, shaderType(uint32), meshID(Entity), MAX_PARTICLES, FIXED_TIMESTEP,
size, random_factor, normal_factor, count, life, random_life,
scaleX, scaleY, rotation, motionBlurAmount, mass,
SPH_h, SPH_K, SPH_p0, SPH_e
```

Then the gates:

| Gate | Fields |
|---|---|
| `archive.GetVersion() >= 45` | `framesX, framesY, frameCount, frameStart, frameRate` |
| `archive.GetVersion() == 48` (read only) | one skipped `uint8_t shadingRate` |
| `archive.GetVersion() >= 64` | `velocity, gravity, drag, random_color`; **else** if SPH is on, defaults `gravity = (0, -19.6, 0)`, `drag = 0.98f` |
| `archive.GetVersion() >= 74` | `restitution` |
| `seri.GetVersion() >= 1` | `opacityCurveControlPeakStart`; **else** it is forced to `0` |
| `seri.GetVersion() >= 2` | `opacityCurveControlPeakEnd`; **else** `= opacityCurveControlPeakStart` |
| *(upstream only)* `seri.GetVersion() >= 3` | `burst_on_create` |

Archive constants in our clone (`wiArchive.cpp:18-20`):
```cpp
static constexpr uint64_t __archiveVersion = 93;
static constexpr uint64_t __archiveVersionBarrier = 22;
```

### 5.2 The blocking incompatibility

The DX11 fork's constants (`D:\max\WickedRepo\WickedEngine\wiArchive.cpp:7-9`):
```cpp
uint64_t __archiveVersion = 5077;
uint64_t __archiveVersionBarrier = 22;
```

That fork's emitter `Serialize` gates its GameGuru-specific fields on `archive.GetVersion() >= 5072 … >= 5077`, each commented `//PE: Special ggm version.` — the fork deliberately jumped the archive version into a private 5000-range so it could extend the emitter format.

Our clone's archive header check (`wiArchive.cpp:80-85`):
```cpp
if (header.version > __archiveVersion)
{
    wi::helper::messageBox("File is not supported!\nFile: " + fileName + "\nReason: The archive version ("
        + std::to_string(header.version) + ") is higher than the program's (" + std::to_string(__archiveVersion) + ")! ...", "Error!");
    Close();
    return;
}
```
This is **stock upstream code — no GGMAX patch**.

**Empirical confirmation.** Reading the first 8 bytes (the version field) of every `.pe` under `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\`:

```
count: 27 files (all 27 read)
version histogram:
     25  5076
      2  5077
```

So **every shipped `.PE` will trip the "File is not supported!" message box and `Close()` the archive.** `WickedCall_LoadWPE` (`GameGuru Core\Guru-WickedMAX\wickedcalls_part3.cpp:2130-2161`) calls `WickedCall_LoadWiScene(path, ...)` and then compares `scene.emitters.GetCount()` before/after — on a rejected archive the count is unchanged and it silently returns `0`. (That file already carries the comment `//ec->SetVisible(false); // SetVisible removed from EmittedParticleSystem` at line 2157, confirming this path is mid-port.)

### 5.3 What a `.PE` actually contains that we would drop

Even if the header check were bypassed, the field layout diverges after `random_color`:

- **DX11 fork writes** (at version ≥ 5072..5077, in order): `restitution, fadein_time, burst_amount, burst_delay`, then `normal_factor_x/y/z`, then `normal_random, rotation_random, size_random, spawn_random, scaling_random, spawn_pause, spawn_pause_random, endcolor_red, endcolor_green, endcolor_blue, burst_split, burst_factor_x/y/z`, then `startpos, bFindFloor, burst_factor_speed, start_rotation, bFollowCamera`, then `random_position, random_position_scale`, then `distance_sort_bias, wpe_filler_1/2/3`.
- **Our clone reads** at that point: `restitution` (gated `archive.GetVersion() >= 74`, which 5076 satisfies), then `opacityCurveControlPeakStart` / `PeakEnd` (gated on `seri.GetVersion()`).

So the first field, `restitution`, would line up by luck, and **everything after it would be read as garbage** — our `opacityCurveControlPeakStart` would consume the DX11 `fadein_time` float, and `PeakEnd` would consume `burst_amount`. Any bypass of the header check must therefore be paired with a bespoke reader, not a version bump alone.

### 5.4 Practical options (sketch, not a recommendation)

1. **Offline conversion**: a one-shot tool that parses `.PE` with the DX11 layout and re-emits a modern archive (or a plain JSON/ini) plus explicit `Burst()` calls. Safest — no engine change, no archive-version games.
2. **Bespoke `.PE` reader in `wickedcalls`**: open the file as raw bytes, skip the header, and hand-decode the emitter block, populating `EmittedParticleSystem` directly. Avoids touching `wiArchive` entirely, which matters because bumping `__archiveVersion` past 5077 would silently change the meaning of every `archive.GetVersion() >= N` gate throughout the whole engine (materials, meshes, terrain, …) and is **strongly inadvisable**.
3. Do **not** raise `__archiveVersion` to 5078. Version gates are compared numerically across ~40 component serializers.

---

## 6. Gap list against the legacy system

Legacy requirements as given: sprite-sheet animated billboards; per-effect textures; colour **and** alpha curves over life; turbulence/noise-driven motion; one-shot bursts and looping emission; additive/alpha blending.

I split gaps into three categories because the effort/risk profiles differ, per the coordinator's instruction:
- **[N] Native** — works today, exactly.
- **[C] CPU-side approximation** — achievable without engine changes, by baking or by driving values per frame from game code.
- **[R] Reinstate** — a GameGuru-local extension the DX11 fork `D:\max\WickedRepo` already had. Lower risk: the design is proven and the field names/semantics are known; but the DX12 emitter is *stock upstream*, so re-adding them means re-forking a file we have so far kept clean.
- **[E] Engine modification** — upstream Wicked never had it and the DX11 fork did not either. Highest risk.

### 6.1 Sprite-sheet animated billboards — **[N] Native, exact**

`framesX/framesY/frameCount/frameStart/frameRate` + `FLAG_FRAME_BLENDING`. Both "loop at N fps" and "one pass over lifetime" modes exist. Formula in §2.5.

Sub-gap: **random start frame is [E]**. `frameStart` is a constant for all particles. A per-particle random start requires either a per-particle field or deriving it from the particle index/RNG. Engine change: add `xEmitterFrameStartRandomness` to `EmittedParticleCB`, and in `emitCS` fold a random offset into an unused bit range — but `Particle` is a tight 64 bytes with no spare field, so this would need either a struct growth (VRAM impact, §1.14) or stealing bits from `rotation_rotationVelocity`. Cheapest workaround **[C]**: use several emitters with different `frameStart`.

### 6.2 Per-effect textures — **[N] Native**

The emitter uses its entity's `MaterialComponent` BASECOLORMAP (and NORMALMAP for the distortion type and for lighting perturbation). One material per emitter entity = one texture per effect. Tint is `material.GetBaseColor()`, brightness boost is `material.GetEmissive()`.

### 6.3 Alpha curve over life — **[C] partly, [E] for arbitrary curves**

- A **trapezoid** (fade-in / hold / fade-out with smoothstep shoulders) is native via `opacityCurveControlPeakStart` / `PeakEnd`.
- An **arbitrary** alpha curve is not expressible.
- **[C] approximation**: bake the alpha into the sprite sheet's own alpha channel and drive it with `frameRate == 0` (one pass over lifetime). This works well and costs nothing, but consumes the sprite sheet — you cannot have both an animated flipbook *and* a baked alpha curve on the same texture.
- **[E] engine change, small and low-risk**: `opacityCurveTex` is already a bindless 2048×`R16_UNORM` 1D texture sampled at `lifeLerp` (`simulateCS.hlsl:13, 50`). Replace `SetOpacityCurveControl(peakStart, peakEnd)` with an overload that takes an arbitrary 2048-sample array (or a keyframe list evaluated CPU-side), keeping the existing texture, sampler, binding and shader untouched. **No shader change at all.** This is the single highest value-per-risk change on this list.

### 6.4 Colour curve over life — **[E] engine modification** (with a **[R]** precedent)

Not expressible. RGB is baked at emit (§1.5) and the vertex colour is written once per frame from `particle.color` (`simulateCS.hlsl:282, 331`).

- **[R] precedent**: the DX11 fork had `endcolor_red/green/blue` (`D:\max\WickedRepo\WickedEngine\wiEmittedParticle.h:139-141`), i.e. a *start→end colour lerp*, not a full curve. Reinstating that specific feature is a known quantity.
- **[E] sketch, cheap version (start→end lerp)**: add `float3 xParticleEndColor` (or a packed `uint`) to `EmittedParticleCB`; in `simulateCS.hlsl` change
  ```hlsl
  float4 particleColor = unpack_rgba(particle.color);
  particleColor.a *= opacity;
  ```
  to lerp `particleColor.rgb` toward the end colour by `lifeLerp` before the alpha multiply. Two lines of HLSL, one CB field, no `Particle` struct growth, no VRAM cost. Sits right next to the existing opacity fetch.
- **[E] sketch, full version (RGB curve)**: promote `opacityCurveTex` from `R16_UNORM` to `R16G16B16A16_UNORM` (or add a second 1D texture), bake RGBA, and sample it in the same place. Costs 8× the curve texture (32 KB per emitter instead of 4 KB — negligible), one extra `SampleLevel` if a second texture, and touches only `simulateCS.hlsl`.
- **[C] approximation available today**: bake the colour ramp into the sprite sheet and play it with `frameRate == 0`. Same trade-off as §6.3 — it consumes the flipbook.
- **[C] second approximation**: split one legacy effect into two or three stacked emitters with different constant colours and staggered `opacityCurveControlPeak*` windows. Costs extra draw calls and extra particle pools (§1.14).

### 6.5 Turbulence / noise-driven motion — **[E] engine modification, the biggest one**

**Nothing exists.** Confirmed by grep across all `emittedparticle*` shaders and the interop header: zero occurrences of `noise`, `turbulen`, `curl`, `vortex`. Upstream has none either (the CB is field-identical, §4.4). The DX11 fork's header has no noise fields either — its motion extras are `normal_factor_x/y/z`, `burst_factor_x/y/z`, `random_position*`, which are *emission-time* randomisation, not *ongoing* turbulence.

What Wicked offers as the nearest thing:
- **`ForceFieldComponent`** (`wiScene_Components.h:1594-1618`) — Point (radial attract/deflect) or Plane, with linear falloff over `range`. These are *smooth, static, per-entity* fields, evaluated per particle per frame (`simulateCS.hlsl:132-139`). You could place a handful to fake swirl, but the cost is O(particles × forcefields) and the result is not noise.
- **`random_factor`** — one-time velocity jitter at birth only.

**[E] sketch — a curl/gradient-noise force.** The insertion point is exactly one place, `simulateCS.hlsl` between the forcefield loop and the integrate:
1. Add to `EmittedParticleCB`: `float xParticleTurbulenceStrength; float xParticleTurbulenceScale; float xParticleTurbulenceSpeed;` (there is room — the CB is not at a 16-byte-boundary limit, but adding 3 floats requires care with the existing `float3`/`float` packing pairs).
2. Add matching serialized fields to `EmittedParticleSystem` with a new `seri.GetVersion() >= N` gate, and bump `componentLibrary.Register<EmittedParticleSystem>("wi::scene::Scene::emitters", 2)` in `wiScene.h:61` to 3 (or 4, to stay clear of upstream's `burst_on_create` at 3 — **worth checking before choosing the number**, since a future upstream pull would otherwise collide).
3. In `simulateCS.hlsl`, before `particle.force += xParticleGravity;`:
   ```hlsl
   float3 np = particle.position * xParticleTurbulenceScale + GetFrame().time * xParticleTurbulenceSpeed;
   particle.force += curl_noise(np) * xParticleTurbulenceStrength;
   ```
   **A reusable noise function already exists and is already in scope**: `simulateCS.hlsl` includes `globals.hlsli`, which provides `float noise_gradient_3D(in float3 p)` at `WickedEngine\shaders\globals.hlsli:1168` (alongside `noise_simplex_2D` at `:1143` and `noise_voronoi` at `:1196`). Curl noise is 3 offset-pair taps of `noise_gradient_3D` — roughly 6 evaluations, no new noise implementation needed. A cheaper non-divergence-free version is a single 3-component gradient-noise sample.
4. No `Particle` struct growth needed (`force` is already a per-frame accumulator that is zeroed each step), so **no VRAM cost**.

Risk: it touches `simulateCS.hlsl`, which is a shared file compiled into 4 variants, and it grows `EmittedParticleCB` (a shader-interop struct). It also permanently forks a file our clone has so far kept identical to upstream, which matters for future pulls (see `WICKED_ENGINE_CHANGES.md` discipline).

**[C] approximation:** none that is faithful. Per-frame CPU driving cannot do per-particle turbulence — the CPU has no access to particle positions (they only exist in GPU buffers). You could jitter the *emitter transform* per frame to fake a wandering source, which reads as a completely different effect.

### 6.6 One-shot bursts — **[N] Native (call-driven), [C]/[upstream] for self-firing**

`Burst(n)`, `Burst(n, position, colour)`, `Burst(n, transform, colour)` all exist and each burst can carry its own transform *and its own vertex colour*. This is a genuinely good match for one-shot effects.

Gaps:
- **Self-firing on load** is upstream's `burst_on_create` (§4.3a) which we lack. **[C] trivially worked around**: `WickedCall_LoadWPE` already fetches the emitter component right after load (`wickedcalls_part3.cpp:2153-2158`) and calls `ec->Restart()`; adding `ec->Burst(n)` there is a one-line game-side change with zero engine risk. Alternatively cherry-pick the upstream commit.
- **Burst delay / burst split** (`burst_delay`, `burst_split`, `burst_delay_timer` in the DX11 fork, `D:\max\WickedRepo\WickedEngine\wiEmittedParticle.h:117-118, 127`) — **[R]**, but note these are pure CPU-side scheduling, so they are equally implementable **[C]** in game code as a timer that calls `Burst()`. Prefer [C].
- **`total_emit_count`** (DX11 fork, `:150`) — "emit N particles then stop", i.e. a finite one-shot emitter. **[R]/[C]**: implementable game-side by counting and zeroing `count`, but see §6.7 for the paused-emission problem.

### 6.7 Looping emission — **[N] Native**, but with a **[R]** gap on stopping

`count` (particles/second) with fractional carry (§2.2). Exact.

**Gap: there is no "stop emitting, let the existing particles die naturally".** `FLAG_PAUSED` freezes everything (§1.2). The DX11 fork had `FLAG_EMIT_PAUSE = 1 << 7` + `IsEmitPaused()`/`SetEmitPaused()` (`D:\max\WickedRepo\WickedEngine\wiEmittedParticle.h:74, 195, 205`). **[C] workaround with no engine change**: set `count = 0` and stop calling `Burst()`. That is behaviourally equivalent for looping emitters, and is what I would do first. **[R] alternative**: reinstate `FLAG_EMIT_PAUSE` — but note bit 7 is now `FLAG_COLLIDERS_DISABLED` upstream (`wiEmittedParticle.h:95`), so it would have to take bit 10 or higher, and the flags are serialized raw (`archive >> _flags`). Any `.PE` written by the DX11 fork with `FLAG_EMIT_PAUSE` set will be misread by our clone as **`FLAG_COLLIDERS_DISABLED`**. That is a concrete silent-corruption hazard for the `.PE` port and is worth flagging regardless of which route is taken.

### 6.8 Additive / alpha blending — **[N] Native, exact**

Both, plus premultiplied, multiply, inverse, opaque. Set via `MaterialComponent::userBlendMode` (§3). Depth-test on, depth-write off, reverse-Z.

### 6.9 Other DX11-fork extensions our clone lacks — all **[R]**

Listed for completeness of the *category*, not as a regression diff (another agent owns that):
`bFindFloor`, `bFollowCamera`, `IsVisible`/`SetVisible`, `IsActive`/`SetActive`, `IsStatActive`, `emittimer`/`SetTimer`/`GetTimer`, `GetEmit()`, `startpos`, `start_rotation`, `normal_random` / `rotation_random` / `size_random` / `scaling_random` / `spawn_random` (independent randomness knobs, versus our single shared `random_factor` — see §1.1), `spawn_pause` / `spawn_pause_random`, `random_position` / `random_position_scale`, `distance_sort_bias`, `bTriggerOutDoor` / `bTriggerInDoor` / `bTriggerUnderWater`, `fadein_time`.

Note especially **the independent randomness knobs**: our clone's single `random_factor` simultaneously drives start size, velocity jitter, start rotation and spin spread (§1.1, §1.6, §1.7). If the legacy effects tuned these separately — and `size_random`, `rotation_random`, `normal_random`, `scaling_random` in the DX11 header say they did — then **no CPU-side approximation can recover that**; it is an `emitCS.hlsl` + CB change (**[E]**, though with an **[R]** design precedent).

### 6.10 Summary table

| Legacy requirement | Verdict |
|---|---|
| Sprite-sheet animated billboards | **[N]** exact |
| — random start frame | **[E]** small (or **[C]** multiple emitters) |
| Per-effect textures | **[N]** exact |
| Alpha curve over life (trapezoid) | **[N]** exact |
| Alpha curve over life (arbitrary) | **[E]** *very small* — CPU-bake into the existing 1D curve texture, zero shader change |
| Colour curve over life | **[E]** small (2 lines HLSL for start→end lerp; a bit more for full RGB curve). **[R]** precedent = `endcolor_*` |
| Turbulence / noise motion | **[E]** **largest gap** — nothing exists anywhere; needs a CB extension + `simulateCS.hlsl` change |
| One-shot bursts | **[N]** exact (call-driven); self-firing is **[C]** one line or an upstream cherry-pick |
| Looping emission | **[N]** exact |
| — stop-emitting-but-finish | **[C]** `count = 0`; **[R]** `FLAG_EMIT_PAUSE` (bit collision hazard) |
| Additive / alpha blending | **[N]** exact |
| Independent per-attribute randomness | **[E]** (**[R]** precedent) — our single `random_factor` conflates four things |
| Velocity-aligned / stretched billboards | **[E]** — only camera-facing + a motion-blur stretch exists (§2.6) |
| Cone / sphere / disc emitter shapes | **[E]** — only point, axis-aligned box volume, mesh surface (§1.3) |
| **Loading `.PE` files at all** | **BLOCKED** — archive version 5076/5077 > our 93; see §5 |

---

## 7. Things I could not verify

- Whether the legacy system's effects relied on shadow-casting particles (upstream `DrawForShadowmap`, §4.3b) — I have no visibility into the legacy renderer's behaviour.
- Whether the half-precision rotation accumulator (§2.3) produces visible stepping for long-lived, fast-spinning particles.
- Whether the `SetOpacityCurveControl` divide-by-zero at `startup_length == 1` (§2.4) manifests visibly. The code is identical upstream, so it is not a local defect.
- Upstream's exact `emittedparticleVS_shadow.hlsl` contents — I confirmed the file exists via the GitHub contents API but did not fetch it.
- Whether upstream's radix sort (#1579) changes sort tie-breaking versus our bitonic `wi::gpusortlib`.
