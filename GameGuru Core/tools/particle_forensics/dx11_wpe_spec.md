# DX11 WPE (Wicked Particle Effect / `.PE`) Behavioural Spec

Forensic reconstruction of how `.PE` particle effects looked and behaved in the shipping
GameGuru MAX **DX11** product.

**Source trees (both READ-ONLY, never mutated during this analysis):**

| Role | Path |
|---|---|
| DX11 game code | `D:\max\GameGuruMAX\GameGuru Core\` |
| DX11-era Wicked Engine fork ("gold standard") | `D:\max\WickedRepo\WickedEngine\` |
| Shipped `.PE` corpus (27 files) | `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\Files\` |

Link established by `D:\max\GameGuruMAX\GameGuru Core\Guru-WickedMAX\Template_Windows.vcxproj:214`
(`AdditionalIncludeDirectories` = `../../../WICKEDREPO/WickedEngine`).

> **Scope note.** `GPUParticles.cpp` / the `gpup_*` / `.arx` system is a *different*, unrelated
> render-to-texture particle system and is explicitly **out of scope** here. It is referenced only
> where the game code branches between the two (`newparticletype::bWPE`).

---

## 0. Executive summary

A `.PE` file is **a whole Wicked Engine `wiArchive` scene dump** (same serialiser as `.wiscene`),
containing 1–5 `wiEmittedParticle` components, their `MaterialComponent`s, `TransformComponent`s,
`HierarchyComponent`s and `NameComponent`s — and nothing else. There is no bespoke "PE format";
the format *is* `Scene::Serialize`.

The simulation is a classic Wicked GPU particle system: persistent structured particle buffer +
alive/dead free-lists + indirect dispatch, emit CS → simulate CS → indirect instanced draw of
camera-facing billboards (4-vertex triangle strips). GameGuru's fork adds ~30 emitter fields
(burst, fade-in, end-colour, per-axis spawn velocity, randomisers, follow-camera, find-floor)
serialised behind private archive versions **5072–5077**.

Six things dominate visual fidelity and are easy to get wrong:

1. **`baseColor.w` (emitter opacity) is routinely >1 in shipped content — up to 20.**
   The VS does `saturate(lerp(1,0,lifeLerp) * xEmitterOpacity)`, so an opacity of 20 holds the
   particle fully opaque until `lifeLerp > 0.95` and then cliff-drops. This *is* the "non-linear
   alpha over life" curve. §5.4.
2. **`material.emissiveColor.w` multiplies colour by `(1 + emissive)`** and reaches 20 in shipped
   content → 21× HDR boost. §6.4.
3. **Alpha over life is otherwise strictly linear** and **there is no colour ramp texture** — colour
   over life is a single `lerp(startColor, endcolor_rgb, lifeLerp)` in the VS. §5.4.
4. **Size over life is `lerp(sizeBegin, sizeEnd, lifeLerp)`, and `sizeEnd` is routinely negative**
   in shipped content (`scaleX` down to −4.12), so particles shrink through zero and *invert*. §5.2.
5. **Every non-volume emitter gets a constant sub-unit XZ offset** because of a modulo-by-zero in
   the emit CS that the whole shipped corpus triggers. §5.1 / §9.1.
6. **The game-side WPE branch honours almost none of the per-instance override sliders** — speed,
   opacity, size, colour, lifespan, bounciness, floor, looping, offsets, rotation and scale are all
   silently dropped for `bWPE == true`. §8.4.

---

## 1. Provenance: GameGuru-local vs stock upstream Wicked

`D:\max\WickedRepo` has a squashed root commit, so "upstream" here means *"present at the fork
point"*. Attribution by `git blame` (read-only).

| Commit | Date | Author | Meaning |
|---|---|---|---|
| `0988337` "Initial Files for DX12 Wicked Repo" | 2022-11-27 | Lee Bamber | fork point == stock Wicked of that era |
| `c7e9b3b` "Finished revert to DX11" | 2022-12-06 | Lee Bamber | DX11 revert |
| `983e8f7` "PE: … New particle system opdates." | 2024-12-02 | Preben Eriksen | **GG-local** wave 1 |
| `eefbe33` "PE: Wicked Particle system additions." | 2025-01-07 | Preben Eriksen | **GG-local** wave 2 |
| `54b3715` "PE: Bug Fix - WParticle simulation running even if no effect is active." | 2025-05-21 | Preben Eriksen | **GG-local** |
| `c503138` "PE: … spawn per sec 'R' randomness … Render bias …" | 2025-06-15 | Preben Eriksen | **GG-local** |
| `d85fe6e` "PE: New emitter features for projectiles effects." | 2025-06-20 | Preben Eriksen | **GG-local** |
| `61a5407` "PE: … resolve all possible debugbreaks." | 2025-08-08 | Preben Eriksen | **GG-local** |

The GG-local emitter fields are additionally self-marked in the header by a (commented-out) guard,
`D:\max\WickedRepo\WickedEngine\wiEmittedParticle.h:113` … `:179`:

```cpp
//#ifdef GGREDUCED
	float restitution = 0.70f; // 0.98f; // if the particles have collision enabled, then after collision this is a multiplier for their bouncing velocities
	...
	//#endif
```

and by the `//PE: Special ggm version.` comments on archive gates ≥5072 in
`wiEmittedParticle.cpp:953, 960, 966, 984, 992, 997`.

---

## 2. Emitter field list — every member, default, units, provenance

Source: `D:\max\WickedRepo\WickedEngine\wiEmittedParticle.h`.
"Prov." column: **U** = present at fork point (stock upstream of that era), **GG** = GameGuru-local
(with the introducing commit).

### 2.1 Private state (not serialised, not authorable)

| Member | h:line | Default | Meaning |
|---|---|---|---|
| `statistics` | 33 | `{}` | GPU readback of `ParticleCounters` |
| `statisticsReadbackBuffer[BufferCount+3]` | 34 | — | staging ring for the above |
| `particleBuffer`, `aliveList[2]`, `deadList`, `distanceBuffer`, `sphPartitionCellIndices`, `sphPartitionCellOffsets`, `densityBuffer`, `counterBuffer`, `indirectBuffers`, `constantBuffer` | 36–45 | — | GPU buffers, allocated by `CreateSelfBuffers()` |
| `float emit` | 48 | `0.0f` | fractional accumulator of particles to spawn this frame |
| `int burst` | 49 | `0` | pending burst count |
| `bool buffersUpToDate` | 50 | `false` | `Restart()` clears this → full buffer rebuild |
| `uint32_t MAX_PARTICLES` | 51 | `1000` | pool size; **serialised** (see §4) |

### 2.2 Serialised, stock-upstream (U)

| Member | h:line | Default | Units / range | Meaning |
|---|---|---|---|---|
| `uint32_t _flags` | 76 | `FLAG_EMPTY` | bitfield | see §2.5 |
| `PARTICLESHADERTYPE shaderType` | 78 | `SOFT` (0) | enum 0–3 | pixel shader selector; **serialisation order fixed** (`h:21` "order of enums shouldn't change!") |
| `wiECS::Entity meshID` | 80 | `INVALID_ENTITY` | entity | optional emit-from-mesh source |
| `float FIXED_TIMESTEP` | 82 | `-1.0f` | seconds | `<0` → variable timestep (`g_xFrame_DeltaTime`); `>=0` → forced fixed dt |
| `float size` | 84 | `1.0f` | world units | particle start size (billboard half-extent multiplier) |
| `float random_factor` | 85 | `1.0f` | — | **DEAD** — uploaded as `xParticleRandomFactor` (`cpp:339`) but **no shader reads it** |
| `float normal_factor` | 86 | `1.0f` | world units/s | scales the random spawn direction (see §5.1) |
| `float count` | 87 | `0.0f` | particles/second | continuous emission rate |
| `float life` | 88 | `1.0f` | seconds | base lifetime |
| `float random_life` | 89 | `1.0f` | fraction | lifetime randomness (±50 % × this) |
| `float scaleX` | 90 | `1.0f` | multiplier | **end** size multiplier (`sizeEnd = sizeBegin * scaleX + …`) |
| `float scaleY` | 91 | `1.0f` | — | **DEAD** — serialised and Lua-exposed (`wiScene_BindLua.cpp:2083`) but **never uploaded to the CB** |
| `float rotation` | 92 | `0.0f` | turns/60 | rotational velocity; uploaded as `rotation * PI * 60` (`cpp:343`) |
| `float motionBlurAmount` | 93 | `0.0f` | — | view-space velocity stretch of the billboard |
| `float mass` | 94 | `1.0f` | — | **DEAD** — written into `Particle.mass` (`emitCS:111`) but the simulate CS never divides force by mass |
| `float random_color` | 95 | `0` | 0–1 | per-channel random darkening of the tint |
| `XMFLOAT3 velocity` | 97 | `{0,0,0}` | world units/s | base velocity, **transformed by the emitter world matrix** (`cpp:389`) |
| `XMFLOAT3 gravity` | 98 | `{0,0,0}` | world units/s² | constant acceleration |
| `float drag` | 99 | `1.0f` | per-step multiplier | `velocity *= drag` **every simulation step** (frame-rate dependent — see §9.2) |
| `float SPH_h/SPH_K/SPH_p0/SPH_e` | 101–104 | `1.0f / 250.0f / 1.0f / 0.018f` | — | SPH fluid params; **never enabled in the shipped corpus** |
| `uint32_t framesX` | 107 | `1` | count | sprite-sheet columns |
| `uint32_t framesY` | 108 | `1` | count | sprite-sheet rows |
| `uint32_t frameCount` | 109 | `1` | count | frames actually used |
| `uint32_t frameStart` | 110 | `0` | index | first frame |
| `float frameRate` | 111 | `0` | frames/second | `0` → stretch whole sheet over life; `>0` → wrap at that rate |

### 2.3 Serialised, GameGuru-local (GG)

| Member | h:line | Default | Commit | Units / meaning |
|---|---|---|---|---|
| `float restitution` | 114 | `0.70f` | `983e8f7` | depth-collision bounce multiplier (`simulateCS:93`) |
| `float fadein_time` | 115 | `0.1f` | `983e8f7` | fraction of life over which alpha ramps 0→1 (`VS:86`) |
| `float burst_amount` | 116 | `0` | `983e8f7` | particles per burst when `Burst(0)` is called |
| `float burst_split` | 117 | `0` | `eefbe33` | if >0, the pending burst is released in `burst/burst_split` chunks per frame |
| `float burst_delay` | 118 | `0` | `983e8f7` | **milliseconds**; delay before a burst starts draining |
| `float normal_factor_x/_y/_z` | 119–121 | `0.0f` | `983e8f7` | per-axis **random** velocity (`sin(rand*2π)`, `rand−0.5`, `cos(rand*2π)`) |
| `float burst_factor_x/_y/_z` | 123–125 | `0.0f` | `eefbe33` | per-axis **index-correlated** velocity (`sin(DTid.x)`, `rand−0.5`, `cos(DTid.x)`) |
| `float burst_delay_timer` | 127 | `0` | `983e8f7` | runtime countdown (**not** serialised) |
| `XMFLOAT3 startpos` | 128 | `{0,0,0}` | `983e8f7` | ring/scatter spawn radii (X,Z = circular radius, Y = random band) |
| `float normal_random` | 130 | `1` | `eefbe33` | scales the random component of the spawn direction |
| `float rotation_random` | 131 | `0` | `983e8f7` | ± randomness added to rotational velocity |
| `float size_random` | 132 | `0` | `983e8f7` | ± fractional randomness on start size |
| `float scaling_random` | 133 | `1` | `eefbe33` | ± absolute randomness on end size |
| `float spawn_random` | 135 | `0` | `983e8f7` | stutter/gust length for continuous emission (§5.6) |
| `float spawn_pause` | 136 | `0` | `983e8f7` | **DEAD** — serialised, never read anywhere in either tree |
| `float spawn_pause_random` | 137 | `0` | `983e8f7` | **DEAD** — same |
| `uint endcolor_red/_green/_blue` | 139–141 | `255` | `983e8f7` | 0–255 target colour at end of life |
| `bool bFindFloor` | 143 | `false` | `eefbe33` | game-side: snap emitter root to terrain height (§8.3) |
| `float burst_factor_speed` | 144 | `1.0f` | `eefbe33` | angular speed of the `startpos` ring rotation |
| `float start_rotation` | 145 | `0.0f` | `eefbe33` | velocity-aligned billboard rotation weight (§5.5) |
| `bool bFollowCamera` | 146 | `false` | `eefbe33` | game-side: pin emitter root to camera each frame (§8.3) |
| `float random_position` | 148 | `0.0f` | `eefbe33` | seed modulus for the fixed XZ spawn offset (§9.1) |
| `float random_position_scale` | 149 | `1.0f` | `eefbe33` | magnitude of that XZ offset |
| `uint32_t total_emit_count` | 150 | `0` | `eefbe33` | running total, feeds `xTotalEmitCount` (not serialised) |
| `bool bTriggerOutDoor` | 152 | `true` | `eefbe33` | **DEAD** — never read, never written, **never serialised** |
| `bool bTriggerInDoor` | 153 | `false` | `eefbe33` | **DEAD** — same |
| `bool bTriggerUnderWater` | 154 | `false` | `eefbe33` | **DEAD** — same |
| `float randemit` | 156 | `0` | `c503138` | runtime state for the stutter emitter (not serialised) |
| `uint32_t randpause` | 157 | `0` | `c503138` | runtime countdown for the stutter emitter (not serialised) |
| `float distance_sort_bias` | 158 | `0` | `c503138` | added to emitter→camera distance for draw ordering (`wiRenderer.cpp:6122, 6252`) |
| `float wpe_filler_1/_2/_3` | 159–161 | `0` | `c503138` | reserved padding, unused |
| `DWORD64 emittimer` | 163 | `0` | `54b3715` | last-emit timestamp (ms); drives auto-deactivate (not serialised) |
| `bool bVisible` | 167 | `true` | `983e8f7` | culling gate (§7.1); **not serialised** |
| `bool bActive` | 171 | `true` | `54b3715` | auto-managed culling gate (§5.7); **not serialised** |
| `bool bStatActive` | 175 | `false` | `54b3715` | enables CPU readback of statistics; **not serialised** |

### 2.4 Non-serialised runtime attributes

`wiEmittedParticle.h:186-189`:

```cpp
	// Non-serialized attributes:
	XMFLOAT3 center;
	uint32_t statisticsReadBackIndex = 0;
	uint32_t layerMask = ~0u;
```

`center` = `transform.GetPosition()` refreshed each `UpdateCPU` (`cpp:202`) and used for emitter
draw sorting. `layerMask` is copied from the entity's `LayerComponent` each frame
(`wiScene.cpp:4338-4341`).

### 2.5 Flags

`wiEmittedParticle.h:64-75`:

```cpp
	enum FLAGS
	{
		FLAG_EMPTY = 0,
		FLAG_DEBUG = 1 << 0,
		FLAG_PAUSED = 1 << 1,
		FLAG_SORTING = 1 << 2,
		FLAG_DEPTHCOLLISION = 1 << 3,
		FLAG_SPH_FLUIDSIMULATION = 1 << 4,
		FLAG_HAS_VOLUME = 1 << 5,
		FLAG_FRAME_BLENDING = 1 << 6,
		FLAG_EMIT_PAUSE = 1 << 7,
	};
```

`FLAG_EMIT_PAUSE` is **GG-local** (`h:74`, commit `d85fe6e`); all other bits are upstream.

| Flag | Effect |
|---|---|
| `FLAG_PAUSED` | `UpdateCPU` returns immediately; `UpdateGPU` skips emit+simulate but still sorts and copies statistics |
| `FLAG_SORTING` | per-particle back-to-front GPU sort via `wiGPUSortLib` (`cpp:616`) |
| `FLAG_DEPTHCOLLISION` | compiles the `DEPTHCOLLISIONS` branch of the simulate CS |
| `FLAG_SPH_FLUIDSIMULATION` | runs the 6-pass SPH chain, plus hard-coded floor/box collision in simulate |
| `FLAG_HAS_VOLUME` | uses `emittedparticle_emitCS_volume` (spawn inside the transform's unit cube) |
| `FLAG_FRAME_BLENDING` | PS cross-fades current/next sprite frame |
| `FLAG_EMIT_PAUSE` | `UpdateCPU` zeroes `emit` and `burst` (`cpp:253-258`) — simulation continues, emission stops |

---

## 3. Frame pipeline

### 3.1 CPU, once per frame per emitter

`Scene::RunParticleUpdateSystem`, `D:\max\WickedRepo\WickedEngine\wiScene.cpp:4325-4346`.
Note this was **moved off the job system to the main thread** by the fork:

```cpp
		//PE: Try running this on main thread only. to prevent CORRUPTED_MULTITHREADING
		for (uint32_t i = 0; i < emitters.GetCount(); i++)
		{
			...
			emitter.UpdateCPU(transform, dt);
		}
```

### 3.2 Culling

`wiRenderer.cpp:4353-4370` — GG added the two gates:

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

There is **no frustum test** for emitters — only visible/active/layer.

### 3.3 GPU simulate

`wiRenderer.cpp:5547-5568`, for every `visibleEmitters` entry:

```cpp
			//PE: Bug Fix - Depth buffer was lost after first emitter.
			device->BindResource(CS, &depthBuffer_Copy1, TEXSLOT_DEPTH, cmd);

			emitter.UpdateGPU(transform, material, mesh, cmd);
```

`UpdateGPU` (`wiEmittedParticle.cpp:316-680`) issues, in order:

1. `UpdateBuffer(constantBuffer)` — the whole `EmittedParticleCB` (§4.3).
2. **kickoffUpdateCS**, `Dispatch(1,1,1)` — computes `realEmitCount = min(deadCount, xEmitCount)`,
   fills both indirect dispatch args, copies `aliveCount_afterSimulation → aliveCount`, resets it.
3. **emitCS** (or `emitCS_VOLUME` if `IsVolumeEnabled()`, or `emitCS_FROMMESH` if a `MeshComponent`
   resolves), `DispatchIndirect(ARGUMENTBUFFER_OFFSET_DISPATCHEMIT)`, 256 threads/group.
4. *(optional)* SPH chain: partition → GPU sort → offsets reset → offsets → density → force.
   Never used by shipped content.
5. **simulateCS** (one of 4 permutations by `FLAG_SORTING` × `FLAG_DEPTHCOLLISION`),
   `DispatchIndirect(ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION)`, 256 threads/group.
6. *(if sorted)* `wiGPUSortLib::Sort` over `distanceBuffer`.
7. **finishUpdateCS**, `Dispatch(1,1,1)` — writes `DrawInstancedIndirect` args
   `uint4(4, particleCount, 0, 0)`.
8. `CopyResource(counterBuffer → statisticsReadbackBuffer[…])`.

The double-buffered alive list is swapped on the **CPU** side, `wiEmittedParticle.cpp:262-263`:

```cpp
	// Swap CURRENT alivelist with NEW alivelist
	std::swap(aliveList[0], aliveList[1]);
```

### 3.4 GPU draw

Two entry points, both GG-local plumbing:

* `DrawSoftParticles_Init` (`wiRenderer.cpp:6100-6147`) sorts emitters by
  `DistanceEstimated(emitter.center, camera.Eye) + distance_sort_bias`, packed as
  `hash = (i & 0xFFF) | ((uint)(distance*2.0f) & 0xFFFFF) << 12`, sorted **descending**
  (far first). Cap `emitterCount > 4095 → 4095`.
* `DrawSoftParticles_Distance(vis, distortion, cmd, distance)` is then called *interleaved with the
  transparent object queue* so particles sort correctly against windows etc.
  (`wiRenderer.cpp:3546`, `RenderPath3D.cpp:2027`).

`wiEmittedParticle::Draw` (`cpp:683-732`) binds
`PSO[material.GetBlendMode()][shaderType]`, the base-colour texture (or `wiTextureHelper::getWhite()`
if absent) at `TEXSLOT_ONDEMAND0`, the emitter CB to VS and PS, the material CB to PS, then
`DrawInstancedIndirect`. `TRIANGLESTRIP`, 4 vertices, `particleCount` instances.

> **Mesh-shader path is dead.** `wiEmittedParticle.cpp:49` — `static bool ALLOW_MESH_SHADER = false;`
> so `emittedparticleMS.hlsl` is never compiled in or dispatched.

---

## 4. The `.PE` file format

### 4.1 It is a `wiArchive` scene

`WickedCall_LoadWPE` (`wickedcalls.cpp:7702`) → `WickedCall_LoadWiScene` (`:7690`) →
`WickedCall_LoadWiSceneDirect` → `Scene::Serialize`. So the "format" is
`D:\max\WickedRepo\WickedEngine\wiScene_Serializers.cpp:1291-1362` (`Scene::Serialize`) plus each
component's `Serialize`.

**Byte encodings** (`D:\max\WickedRepo\WickedEngine\wiArchive.h:49-160`) — these are the trap:

| C++ type | Bytes on disk |
|---|---|
| `bool` | **4** (`uint32`, `true` iff `== 1`) |
| `int` / `unsigned int` / `uint32_t` / `long long` / `Entity` | **8** (widened to `int64`/`uint64`) |
| `char` / `unsigned char` / `uint8_t` | **1** |
| `float` | 4 |
| `XMFLOAT2/3/4` | 8 / 12 / 16 |
| `std::string` | `uint64 len = length+1`, then `len` bytes (**includes** the NUL) |
| `std::vector<T>` | `uint64 count`, then `count` × T |

File header: `uint64 version` written by `wiArchive::CreateEmpty()`
(`wiArchive.cpp:45-55`). Current writer version is `__archiveVersion = 5077`
(`wiArchive.cpp:7`), barrier `22` (`wiArchive.cpp:9`).

`Scene::Serialize` layout (`wiScene_Serializers.cpp:1291`):

```
uint64  version                      (written by wiArchive itself)
uint64  reserved (=0)
        wiResourceManager::Serialize  (version >= 63)  -- embedded texture blobs
        names, layers, transforms, prev_transforms, hierarchy,
        materials, meshes, impostors, objects, aabb_objects,
        rigidbodies, softbodies, armatures, lights, aabb_lights,
        cameras, probes, aabb_probes, forces, decals, aabb_decals,
        animations, EMITTERS, hairs, weathers,
        sounds(>=30), inverse_kinematics(>=37), springs(>=38), animation_datas(>=46)
```

Each `ComponentManager::Serialize` (`wiECS.h:215-262`) is
`uint64 count`, then `count` components, then `count` entity ids (uint64 each).

**Empirically confirmed** by writing an exact parser and walking all 27 shipped `.PE` files
(27/27 parsed cleanly, byte-exact): a `.PE` contains **only** `names`, `layers`, `transforms`,
`prev_transforms`, `hierarchy`, `materials`, `emitters` (+ optionally embedded resources).
`meshes` … `animations` are all count-0 — verified as a 15-manager run of zero `uint64`s
immediately preceding the emitters block in every file. Consequences:

* **`emittedparticle_emitCS_FROMMESH` is never used by shipped content.** `meshID` *is* non-zero in
  23 of 27 files, but it is remapped through `SerializeEntity` with `allow_remap = true`
  (`wiECS.h:122-140`) to a freshly-created entity that owns no `MeshComponent`, so
  `scene.meshes.GetComponent(emitter.meshID)` returns `nullptr` and the plain/volume emit CS runs.
* Force fields affecting WPE particles can only come from the **host** GameGuru scene, never from
  the `.PE`.

### 4.2 `wiEmittedParticle::Serialize` — version map

`D:\max\WickedRepo\WickedEngine\wiEmittedParticle.cpp:896-1098`. Read and write sides are
symmetric; quoting the read side.

**Unconditional (all versions ≥ barrier 22)** — `cpp:900-919`:

```cpp
		archive >> _flags;                 // 8 bytes
		archive >> (uint32_t&)shaderType;  // 8
		wiECS::SerializeEntity(archive, meshID, seri);  // 8
		archive >> MAX_PARTICLES;          // 8
		archive >> FIXED_TIMESTEP;         // 4
		archive >> size;                   // 4
		archive >> random_factor;
		archive >> normal_factor;
		archive >> count;
		archive >> life;
		archive >> random_life;
		archive >> scaleX;
		archive >> scaleY;
		archive >> rotation;
		archive >> motionBlurAmount;
		archive >> mass;
		archive >> SPH_h;
		archive >> SPH_K;
		archive >> SPH_p0;
		archive >> SPH_e;
```

| Gate | Fields | Bytes | Source |
|---|---|---|---|
| `>= 45` | `framesX, framesY, frameCount, frameStart` (8 each), `frameRate` (4) | 36 | `cpp:921-928` |
| `== 48` | one `uint8_t shadingRate`, discarded (`// no longer needed`) | 1 | `cpp:930-934` |
| `>= 64` | `velocity`(12), `gravity`(12), `drag`(4), `random_color`(4) | 32 | `cpp:936-942` |
| `< 64` fallback | if SPH enabled: `gravity = (0, -19.6, 0); drag = 0.98f;` | — | `cpp:943-950` |
| `>= 5072` **GG** | `restitution, fadein_time, burst_amount, burst_delay` | 16 | `cpp:953-959` |
| `>= 5073` **GG** | `normal_factor_x/_y/_z` | 12 | `cpp:960-965` |
| `>= 5074` **GG** | `normal_random, rotation_random, size_random, spawn_random, scaling_random, spawn_pause, spawn_pause_random` (4 each = 28), `endcolor_red/_green/_blue` (8 each = 24), `burst_split` (4), `burst_factor_x/_y/_z` (12) | 68 | `cpp:966-983` |
| `>= 5075` **GG** | `startpos`(12), `bFindFloor`(4), `burst_factor_speed`(4), `start_rotation`(4), `bFollowCamera`(4) | 28 | `cpp:984-991` |
| `>= 5076` **GG** | `random_position, random_position_scale` | 8 | `cpp:992-996` |
| `>= 5077` **GG** | `distance_sort_bias, wpe_filler_1, wpe_filler_2, wpe_filler_3` | 16 | `cpp:997-1003` |

Fixed record sizes: **296 bytes at v5076, 312 bytes at v5077** (verified against the corpus).

**Never serialised** (must be reconstructed as defaults on load): `bVisible`, `bActive`,
`bStatActive`, `emittimer`, `randemit`, `randpause`, `total_emit_count`, `burst_delay_timer`,
`emit`, `burst`, `center`, `layerMask`, and the three dead
`bTriggerOutDoor/bTriggerInDoor/bTriggerUnderWater` booleans.

### 4.3 `EmittedParticleCB` — CPU→GPU mapping

`D:\max\WickedRepo\WickedEngine\shaders\ShaderInterop_EmittedParticle.h:35-116`; populated at
`wiEmittedParticle.cpp:329-420`. Non-obvious mappings:

| CB field | Filled from | cpp:line |
|---|---|---|
| `xEmitterWorld` | `transform.world` | 330 |
| `xEmitCount` | `(uint32_t)emit` (truncated!) | 331 |
| `xEmitterRandomness` | `wiRandom::getRandom(0,1000) * 0.001f` (new every frame) | 335 |
| `xParticleRotation` | `rotation * XM_PI * 60` | 343 |
| `xParticleColor` | `wiMath::CompressColor(float4(material.baseColor.rgb, 1))` — **8-bit, saturated** | 344 |
| `xParticleEmissive` | `material.emissiveColor.w` | 345 |
| `xEmitterOpacity` | `material.GetOpacity()` == `material.baseColor.w` — **not clamped here** | 346 |
| `xParticleSinPos` | `startpos` | 354 |
| `xParticleNormalFactorX/Y/Z` | **`burst_factor_x/_y/_z`** (names are crossed over!) | 355-357 |
| `xParticleNormalFactor2X/Y/Z` | **`normal_factor_x/_y/_z`** | 361-363 |
| `xParticleBurstFactorDpeed` | `burst_factor_speed` (sic, typo in engine) | 359 |
| `xParticleVelocity` | `XMVector3TransformNormal(velocity, transform.world)` | 389 |
| `xEmitterTexMul` | `float2(1/framesX, 1/framesY)` | 385 |
| `xEmitterFixedTimestep` | `FIXED_TIMESTEP` | 349 |

`CompressColor` saturates (`wiMath.cpp:439-449`), so a `baseColor.rgb > 1` is clipped to white
before reaching the GPU — only `baseColor.w` survives unclamped.

---

## 5. Simulation maths — exact formulas

### 5.1 Spawn position

`emittedparticle_emitCS.hlsl`. Seeding preamble (`:27-28`):

```hlsl
		float2 uv = float2(g_xFrame_Time + xEmitterRandomness, (float)DTid.x / (float)THREADCOUNT_EMIT);
		float seed = 0.12345;
```

with (`globals.hlsli:148-153`)

```hlsl
inline float rand(inout float seed, in float2 uv)
{
	float result = frac(sin(seed * dot(uv, float2(12.9898, 78.233))) * 43758.5453);
	seed += 1;
	return result;
}
```

`rand` is **stateful** — every call bumps the seed, so *the order of `rand()` calls is part of the
spec*. Any reimplementation must issue them in exactly the same sequence.

**Point emitter (default)** — `:90-95`:

```hlsl
		// Just emit from center point:
		float3 pos = mul(xEmitterWorld, float4(0, 0, 0, 1)).xyz;

        float fixedseed = xTotalEmitCount % (uint) xParticleRandomPos;
        pos.x += (rand(fixedseed, float2(321.123, xParticleRandomPosScale)) - 0.5f) * xParticleRandomPosScale;
        pos.z += (rand(fixedseed, float2(xParticleRandomPosScale, 456.32)) - 0.5f) * xParticleRandomPosScale;
```

See §9.1 — this is unconditional and the shipped corpus always divides by zero here.

**Volume emitter** (`FLAG_HAS_VOLUME`) — `:87`:

```hlsl
		float3 pos = mul(xEmitterWorld, float4(rand(seed, uv) * 2 - 1, rand(seed, uv) * 2 - 1, rand(seed, uv) * 2 - 1, 1)).xyz;
```

i.e. uniform inside the transform's ±1 cube; the *transform scale* is the volume extent. Note the
`xParticleRandomPos` lines are inside the `#else`, so **volume emitters do not get the §9.1
offset**.

**Ring / scatter offset**, applied to every emitter type (`:103`):

```hlsl
        pos += float3(sin((float) (g_xFrame_Time * xParticleBurstFactorDpeed) + DTid.x) * xParticleSinPos.x,
                      (rand(seed, uv) - 0.5f) * xParticleSinPos.y,
                      cos((float) (g_xFrame_Time * xParticleBurstFactorDpeed) + DTid.x) * xParticleSinPos.z);
```

`xParticleSinPos = startpos`. X/Z trace a circle whose phase advances with wall-clock time ×
`burst_factor_speed` and is offset per dispatch-thread index; Y is a uniform random band.

### 5.2 Initial size

`emitCS:105, 121`:

```hlsl
        float particleStartingSize = xParticleSize + (xParticleSize * ((rand(seed, uv) - 0.5f) * xParticleSizeRandom));
...
        particle.sizeBeginEnd = float2(particleStartingSize, (particleStartingSize * xParticleScaling) + ((rand(seed, uv) - 0.5f) * xParticleScalingRandom));
```

So `sizeBegin = size * (1 + (r−0.5)·size_random)` and
`sizeEnd = sizeBegin · scaleX + (r−0.5)·scaling_random`.
`scaleX` is negative in a lot of shipped content (down to −4.12), which is legal here — the
billboard passes through zero size and flips.

### 5.3 Initial velocity

`emitCS:112-115` — **three separate additive terms**, in this order:

```hlsl
        particle.velocity = xParticleVelocity + (nor + (float3(rand(seed, uv), rand(seed, uv), rand(seed, uv)) - 0.5f) * xParticleNormalRandom) * xParticleNormalFactor;
//        particle.velocity += float3(sin(rand(seed, uv) * PI2) * xParticleNormalFactorX, (rand(seed, uv) - 0.5f) * xParticleNormalFactorY, (cos(rand(seed, uv) * PI2)) * xParticleNormalFactorZ);
		particle.velocity += float3(sin((float) rand(seed, uv) * PI2) * xParticleNormalFactor2X, (rand(seed, uv) - 0.5f) * xParticleNormalFactor2Y, cos((float) rand(seed, uv) * PI2) * xParticleNormalFactor2Z);
		particle.velocity += float3(sin((float) DTid.x) * xParticleNormalFactorX, (rand(seed, uv) - 0.5f) * xParticleNormalFactorY, cos((float) DTid.x) * xParticleNormalFactorZ);
```

Remembering the CB crossover (§4.3):

* Term 1 (`normal_factor`): `velocity_world + (nor + (rand3 − 0.5)·normal_random) · normal_factor`.
  `nor` is 0 for non-mesh emitters, so this reduces to a random vector in a
  `±0.5·normal_random·normal_factor` cube.
* Term 2 (`normal_factor_x/y/z`): `(sin(2π·r)·nfx, (r−0.5)·nfy, cos(2π·r)·nfz)` — **independent
  random angles for X and Z**, so this is *not* a coherent direction; it is an axis-aligned scatter.
* Term 3 (`burst_factor_x/y/z`): `(sin(DTid.x)·bfx, (r−0.5)·bfy, cos(DTid.x)·bfz)` — X/Z are
  functions of the *dispatch thread index in radians*, giving a deterministic pseudo-ring spray
  across a burst.

Note `sin((float) rand(seed, uv) * PI2)`: the cast binds to `rand(...)` before the multiply, so it
is `sin(r · 2π)` as intended.

### 5.4 Rotational velocity, lifetime, colour bits

`emitCS:118-130`:

```hlsl
        particle.rotationalVelocity = xParticleRotation + ((rand(seed, uv) - 0.5f) * xParticleRotationRandom);
		particle.maxLife = xParticleLifeSpan + xParticleLifeSpan * (rand(seed, uv) - 0.5f) * xParticleLifeSpanRandomness;
		particle.life = particle.maxLife;
        particle.sizeBeginEnd = ...
		particle.color_mirror = 0;
		particle.color_mirror |= ((rand(seed, uv) > 0.5f) << 31) & 0x10000000;
		particle.color_mirror |= ((rand(seed, uv) < 0.5f) << 30) & 0x20000000;

		uint color_modifier = 0;
		color_modifier |= (uint)(255.0 * lerp(1, rand(seed, uv), xParticleRandomColorFactor)) << 0;
		color_modifier |= (uint)(255.0 * lerp(1, rand(seed, uv), xParticleRandomColorFactor)) << 8;
		color_modifier |= (uint)(255.0 * lerp(1, rand(seed, uv), xParticleRandomColorFactor)) << 16;
		particle.color_mirror |= xParticleColor & color_modifier;
```

`maxLife = life · (1 + (r−0.5)·random_life)`.
Bits 28/29 of `color_mirror` are random **UV mirror** flags (note the deliberately odd
`<<31 & 0x10000000` / `<<30 & 0x20000000` — the shift overflows and the mask is what actually
selects the bit; reproduce the *result*, i.e. a 50/50 flag each, not the shift).
Per-particle tint is `baseColorRGB8 AND randomDarken8` — a **bitwise AND**, not a multiply.

### 5.5 Per-step integration

`emittedparticle_simulateCS.hlsl:24`, `:32-55`, `:100-109`, `:158`:

```hlsl
		const float dt = xEmitterFixedTimestep >= 0 ? xEmitterFixedTimestep : g_xFrame_DeltaTime;
...
			for (uint i = 0; i < g_xFrame_ForceFieldArrayCount; ++i)
			{
				...
					particle.force += dir * forceField.GetEnergy() * (1 - saturate(dist * forceField.GetRange()));
			}

            particle.force += xParticleGravity;
...
			// integrate:
			particle.velocity += particle.force * dt;
			particle.position += particle.velocity * dt;

			// reset force for next frame:
			particle.force = 0;

			// drag:
			particle.velocity *= xParticleDrag;
...
			particle.life -= dt;
```

Semi-implicit Euler. `force` is an **acceleration** (mass is never divided out). Drag is applied
*after* integration and is a raw per-step multiplier, **not** `pow(drag, dt)` — see §9.2.

Depth collision (`FLAG_DEPTHCOLLISION`), `simulateCS:57-98` — reprojects the particle into the
**previous** frame's depth buffer, treats a 1.5-unit-thick shell around the surface as solid, builds
a normal from two neighbouring depth taps, and reflects:

```hlsl
					if (dot(particle.velocity, surfaceNormal) < 0)
					{
						particle.velocity = reflect(particle.velocity, surfaceNormal) * xEmitterRestitution;
					}
```

(`* xEmitterRestitution` is the GG change, commit `983e8f7`; upstream used a hard-coded `0.98f`.)

SPH-only floor/box collision (`simulateCS:121-154`) uses a hard-coded floor at y=0 and box extent
`float3(40, 0, 22)` with `elastic = 0.6` — legacy demo code, never reached by shipped content.

Sorting key (`simulateCS:168-173`): `distanceBuffer[particleIndex] = -dot(eye, eye)`.

### 5.6 Emission rate on the CPU

`wiEmittedParticle::UpdateCPU`, `cpp:193-300`.

```cpp
	emit = std::max(0.0f, emit - floorf(emit));   // keep only the fractional remainder
	center = transform.GetPosition();

	if (randpause == 0)
	{
		randpause = ((wiRandom::getRandom(0, 1000) * 0.001f) * spawn_random);
		if (randpause < (spawn_random * 0.6))
			randpause = 0;
		else
			randpause -= (spawn_random * 0.5);
		if(randpause > spawn_random)
			randpause = 0;

		emit += (float)(count) * dt;
		randemit = count;
	}
	else
	{
		emit += randemit * dt;
		randemit *= 0.05;
		randpause--;
	}
```

This is the "spawn per sec R randomness" behaviour added by `c503138`. With `spawn_random == 0`
`randpause` stays 0 and it degenerates to plain `emit += count * dt`. With `spawn_random > 0` the
emitter enters random-length pauses during which the rate decays by ×0.05 per frame — gusty
emission. `randpause` is `uint32_t` assigned from a float; the truncation is part of the behaviour.

Burst draining, `cpp:231-251`:

```cpp
	if (burst_delay_timer > 0)
	{
		burst_delay_timer -= 1000 * dt;
		if (burst_delay_timer < 0) burst_delay_timer = 0;
	}
	else
	{
		if (burst_split > 0)
		{
			float amount = burst / burst_split;
			emit += amount;
			burst -= amount;
			if (burst < 0) burst = 0;
		}
		else
		{
			emit += burst;
			burst = 0;
		}
	}
```

`burst_delay` is therefore in **milliseconds**. `Burst()` (`cpp:301-309`):

```cpp
void wiEmittedParticle::Burst(int num)
{
	if (IsPaused())
		return;
	if (num <= 0)
		num = burst_amount;
	burst_delay_timer = burst_delay;
	burst += num;
}
```

The game always calls `Burst(0)` (§8.2), so the quantity always comes from the authored
`burst_amount`.

Emit pause and total, `cpp:253-260`:

```cpp
	if (IsEmitPaused())
	{
		//PE: Just let it timeout later.
		emit = 0;
		burst = 0;
	}

	total_emit_count += (uint32_t)emit;
```

### 5.7 Auto-deactivation (GG-local, `54b3715`)

`cpp:285-297`:

```cpp
	if (emit <= 0)
	{
		DWORD64 maxLifeMil = (DWORD64) ((life + life * (0.5f) * random_life) * 1000.0);
		if (GetElapsedMilliseconds() - GetTimer() > (maxLifeMil * 2))
		{
			SetActive(false);
		}
	}
	else
	{
		SetTimer(GetElapsedMilliseconds());
		SetActive(true);
	}
```

Once an emitter has produced nothing for `2 × worst-case particle lifetime`, `bActive` goes false
and the emitter drops out of `visibleEmitters` — no simulate, no draw. This is what lets
fire-and-forget decal/impact effects self-terminate (`M-Decal.cpp:849`
`//PE: No need to stop effect it will end by itself.`). `Restart()` does **not** reset `bActive`;
only a non-zero `emit` does.

---

## 6. Rendering

### 6.1 Pipeline state

`wiEmittedParticle::Initialize`, `cpp:816-893`.

```cpp
	rs.FillMode = FILL_SOLID;
	rs.CullMode = CULL_NONE;
	rs.FrontCounterClockwise = true;
	rs.DepthClipEnable = false;
	...
	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_GREATER_EQUAL;
	dsd.StencilEnable = false;
```

Depth **test on** (reverse-Z, `GREATER_EQUAL`), depth **write off**, no backface culling, near/far
clipping disabled.

Blend states (`cpp:852-887`):

| Mode | Src / Dest (colour) | Src / Dest (alpha) |
|---|---|---|
| `BLENDMODE_ALPHA` (3) | `SRC_ALPHA` / `INV_SRC_ALPHA` | `ONE` / `INV_SRC_ALPHA` |
| `BLENDMODE_ADDITIVE` (5) | `SRC_ALPHA` / `ONE` | `ZERO` / `ONE` |
| `BLENDMODE_PREMULTIPLIED` (4) | `ONE` / `INV_SRC_ALPHA` | `ONE` / `ONE` |
| `BLENDMODE_OPAQUE` (0) | blending disabled | — |

> **Quirk:** `blendStates[]` is sized `BLENDMODE_COUNT` (= 7, `wiEnums.h:4-16`) but only those four
> entries are ever assigned. `BLENDMODE_ALPHANOZ` (1), `BLENDMODE_FORCEDEPTH` (2) and
> `BLENDMODE_MULTIPLY` (6) keep a default-constructed `BlendState` → blending **disabled**. A `.PE`
> material authored with one of those renders opaque. No shipped `.PE` does this (§7.2), but the
> replacement must not "fix" it silently.

`MaterialComponent::GetBlendMode()` (`wiScene.h:263`) upgrades `OPAQUE` to `ALPHA` when the material
is flagged transparent.

### 6.2 Billboard construction

`emittedparticleVS.hlsl:6-11, 29-79`:

```hlsl
static const float3 BILLBOARD[] = {
	float3(-1, -1, 0),	// 0
	float3(1, -1, 0),	// 1
	float3(-1, 1, 0),	// 2
	float3(1, 1, 0),	// 4
};
...
	float3 quadPos = BILLBOARD[vertexID];
	quadPos.x = particle.color_mirror & 0x10000000 ? -quadPos.x : quadPos.x;
	quadPos.y = particle.color_mirror & 0x20000000 ? -quadPos.y : quadPos.y;
	float2 uv = quadPos.xy * float2(0.5f, -0.5f) + 0.5f;
...
	// rotate the billboard:
	float2x2 rot = float2x2(
		cos(rotation), -sin(rotation),
		sin(rotation), cos(rotation)
		);
	quadPos.xy = mul(quadPos.xy, rot);

	// scale the billboard:
	quadPos *= size;

	// scale the billboard along view space motion vector:

	quadPos += dot(quadPos, velocity) * velocity * xParticleMotionBlurAmount;

	// copy to output:
	Out.pos = float4(particle.position, 1);
	Out.pos = mul(g_xCamera_View, Out.pos);
	Out.pos.xyz += quadPos.xyz;
	Out.P = mul(g_xCamera_InvV, float4(Out.pos.xyz, 1)).xyz;
	Out.pos = mul(g_xCamera_Proj, Out.pos);
```

**Pure view-space camera-facing quads.** The particle centre is transformed to view space and the
quad offset added there, so the billboard is always screen-aligned — never velocity-aligned, never
axis-locked. Size is a **half-extent in view/world units** (the quad spans ±size).

Rotation, `VS:27` and `:51-57`:

```hlsl
	float rotation = lifeLerp * particle.rotationalVelocity;
...
	//PE: Rotate to fit direction if top up view.
    float3 velocity = mul((float3x3) g_xCamera_View, particle.velocity);

    if (particle.velocity.x != 0 || particle.velocity.z != 0)
    {
        rotation += (tan(normalize(velocity)) * xParticleStartRotation);
    }
```

`rotation = lifeLerp · rotationalVelocity` — i.e. the sprite's angle is proportional to *fraction of
life elapsed*, **not** to absolute time, so the total spin over a particle's life is fixed
regardless of lifetime. The `start_rotation` term is GG-local (`eefbe33`) and is mathematically
odd: `tan()` of a normalised `float3` yields a `float3` which is then implicitly truncated to
`.x` when added to a scalar. Reproduce as `rotation += tan(normalize(viewVelocity).x) * start_rotation`
and flag it — this is almost certainly not what the author intended, but it is what shipped.
Only 1 emitter in the corpus has `start_rotation != 0`.

### 6.3 Sprite-sheet frame advance

`VS:36-49`:

```hlsl
	// Sprite sheet UV transform:
	//PE: Changed to always move flibook forward.
	const float spriteframe = xEmitterFrameRate == 0 ?
		lerp(xEmitterFrameStart, xEmitterFrameCount, lifeLerp) :
		((xEmitterFrameStart + (particle.maxLife - particle.life) * xEmitterFrameRate) % xEmitterFrameCount);
	const uint currentFrame = floor(spriteframe);
	const uint nextFrame = ceil(spriteframe);
	const float frameBlend = frac(spriteframe);
	uint2 offset = uint2(currentFrame % xEmitterFramesXY.x, currentFrame / xEmitterFramesXY.x);
	uv.xy += offset;
	uv.xy *= xEmitterTexMul;
	uint2 offset2 = uint2(nextFrame % xEmitterFramesXY.x, nextFrame / xEmitterFramesXY.x);
	uv2.xy += offset2;
	uv2.xy *= xEmitterTexMul;
```

* `frameRate == 0` → the whole sheet is stretched across the particle's life.
* `frameRate > 0` → frames advance at `frameRate` fps from birth and **wrap** (`%`), which is the
  GG change (`eefbe33`): upstream used the elapsed-life form without the modulo, so flipbooks could
  run backwards.

Frames are laid out row-major, `framesX` per row.

### 6.4 Pixel shaders

`emittedparticlePS_soft.hlsl` is the only real one; `_soft_lighting` and `_soft_distortion` are
one-line `#define` + `#include` wrappers (`emittedparticlePS_soft_lighting.hlsl:1-3`,
`emittedparticlePS_soft_distortion.hlsl:1-2`).

```hlsl
[earlydepthstencil]
float4 main(VertextoPixel input) : SV_TARGET
{
    float4 color = texture_color.Sample(sampler_linear_clamp, input.tex.xy);

	[branch]
	if (xEmitterOptions & EMITTER_OPTION_BIT_FRAME_BLENDING_ENABLED)
	{
	    float4 color2 = texture_color.Sample(sampler_linear_clamp, input.tex.zw);
		color = lerp(color, color2, input.frameBlend);
	}

	float2 pixel = input.pos.xy;
	float2 ScreenCoord = pixel * g_xFrame_InternalResolution_rcp;
	float4 depthScene = texture_lineardepth.GatherRed(sampler_linear_clamp, ScreenCoord) * g_xCamera_ZFarP;
	float depthFragment = input.pos.w;
	float fade = saturate(1.0 / input.size*(max(max(depthScene.x, depthScene.y), max(depthScene.z, depthScene.w)) - depthFragment));
	...
	float opacity = saturate(color.a * inputColor.a * fade);

	color.rgb *= inputColor.rgb * (1 + xParticleEmissive);
	color.a = opacity;
```

* **Soft-particle depth fade** is on for all three `SOFT*` shader types, using a 2×2
  `GatherRed` of linear depth and taking the **max** (farthest) tap; the fade width is
  `1 / particleSize`.
* **Emissive** multiplies RGB by `(1 + emissiveColor.w)` — up to **21×** in shipped content.
* `SOFT_DISTORTION` additionally does `color.rgb = color.rgb - 0.5f;` (`:39`) so the sprite acts as
  a signed normal map for the distortion pass.
* `SOFT_LIGHTING` builds a fake hemispherical normal from the unrotated quad UV and runs full tiled
  lighting (`:48-72`):
  ```hlsl
		N.x = -cos(PI * input.unrotated_uv.x);
		N.y = cos(PI * input.unrotated_uv.y);
		N.z = -sin(PI * length(input.unrotated_uv));
		N = mul((float3x3)g_xCamera_InvV, N);
  ```
* `SIMPLE` is **not** a real particle shader in this fork — `emittedparticlePS_simple.hlsl` is
  three lines returning flat grey:
  ```hlsl
  float4 main() : SV_TARGET
  {
  	return float4(0.8f, 0.8f, 0.8f, 1.0f);
  }
  ```
  It is only used for wireframe (`PSO_wire`, `cpp:805`). No shipped `.PE` selects `SIMPLE`.

### 6.5 The alpha-over-life curve (the important one)

`VS:26` and `:85-103`:

```hlsl
	float opacity = saturate(lerp(1, 0, lifeLerp) * xEmitterOpacity);
...
	//PE: Fade in particle.
	float normalizedTime = clamp(lifeLerp / xEmitterFadeinTime, 0, 1);
	opacity = saturate(lerp(0, opacity, normalizedTime));

	//PE: Endcolor
    uint red = particle.color_mirror   & 0x000000FF;
    ...
    red = lerp(red, xParticleEndColorRed, lifeLerp);
    green = lerp(green, xParticleEndColorGreen, lifeLerp);
    blue = lerp(blue, xParticleEndColorBlue, lifeLerp);

    Out.color = (red + (green << 8) + (blue << 16)) | (uint(opacity * 255.0f) << 24);
```

Full alpha model, with `t = lifeLerp = 1 − life/maxLife`:

```
alpha(t) = saturate( min(t / fadein_time, 1) * saturate( (1 - t) * emitterOpacity ) )
final_alpha = saturate( texture.a * alpha(t) * softDepthFade )
```

Because `emitterOpacity` (= `material.baseColor.w`) is often ≫1 in shipped content (median values
of 1.0–3.5, maxima of 10, 20), the decay term stays clamped at 1 until
`t > 1 − 1/emitterOpacity` and then falls linearly. With `emitterOpacity = 20` the particle is fully
opaque for the first 95 % of its life. **This asymmetric hold-then-cliff is the characteristic WPE
look and must be reproduced exactly** — a naive linear fade will look completely different.

`fadein_time` is a *fraction of life*, not seconds (median 0.1, i.e. first 10 % of life).

Colour over life is a plain `lerp` from the per-particle tint to `endcolor_rgb`, done in **8-bit
integer space in the vertex shader** and passed through a packed `uint` — the interpolated colour is
therefore quantised to 256 levels per channel and constant across the quad
(`nointerpolation uint color`, `emittedparticleHF.hlsli:9`).

---

## 7. The shipped `.PE` corpus — measured ground truth

27 `.PE` files, 70 emitters, parsed byte-exactly with a purpose-built reader
(scripts left in the scratchpad: `pe_dump.py`, `pe_mat.py`; raw output `pe_corpus.txt`,
`pe_table.txt`).

### 7.1 Archive versions

| Version | Files |
|---|---|
| 5076 | 25 |
| 5077 | 2 (`gamecore\decals\sparks\wpe.pe`, `gamecore\decals\splash_large\wpe.pe`) |

So `distance_sort_bias` is only *present* in 2 files; the other 25 get its default of 0 on load.

### 7.2 Aggregate emitter statistics (70 records)

| Field | min | max | median | distinct |
|---|---|---|---|---|
| `MAX_PARTICLES` | 1000 | 25000 | 1000 | 4 |
| `FIXED_TIMESTEP` | −1 | −1 | −1 | 1 (always variable) |
| `size` | 0.23 | 100 | 3.315 | 40 |
| `count` (per sec) | 0 | 10944 | 0 | 12 |
| `life` (sec) | 0.25 | 16 | 1.9 | 36 |
| `random_life` | 0 | 3.89 | 1 | 10 |
| `scaleX` | **−4.12** | 30 | 2.18 | 28 |
| `rotation` | −0.022 | 0.107 | 0 | 13 |
| `motionBlurAmount` | 0 | 0.00136 | 0 | 12 |
| `velocity.y` | −125 | 300 | 5.885 | 16 |
| `gravity.y` | −1200 | 30 | −8.63 | 19 |
| `drag` | **1** | **1** | 1 | **1** |
| `restitution` | 0.1 | 1.498 | 0.7 | 12 |
| `fadein_time` | 0.01 | 1 | 0.1 | 21 |
| `burst_amount` | 0 | 6000 | 90.73 | 25 |
| `burst_delay` (ms) | 0 | 2691 | 0 | 11 |
| `normal_factor` | 0 | 300 | 35.9 | 26 |
| `spawn_random` | 0 | 700 | 0 | 5 |
| `endcolor_red/green/blue` | 0 | 255 | 117 / 83.5 / 90.5 | ~23 each |
| `random_position` | **0** | **0** | 0 | **1** |
| `random_position_scale` | **1** | **1** | 1 | **1** |
| `mass`, `scaleY`, `SPH_*`, `spawn_pause*` | — | — | at default | 1 |

`shaderType`: 58 × `SOFT`, 10 × `SOFT_DISTORTION`, 2 × `SOFT_LIGHTING`, **0 × `SIMPLE`**.

`_flags` values observed: `0` (13), `4 SORTING` (19), `8 DEPTHCOLLISION` (12),
`12 SORTING+DEPTHCOLLISION` (4), `32 HAS_VOLUME` (9), `36 SORTING+HAS_VOLUME` (8),
`40 DEPTHCOLLISION+HAS_VOLUME` (4), `44 all three` (1).
**`FLAG_SPH_FLUIDSIMULATION`, `FLAG_FRAME_BLENDING`, `FLAG_PAUSED`, `FLAG_EMIT_PAUSE` and
`FLAG_DEBUG` never appear.** Frame blending being universally off means the `uv2`/`frameBlend`
half of the VS output is dead in practice.

Sprite sheets: `1×1` (37), `8×8/64` (21), `5×5/25` (11), `20×4/80` (1). `frameStart` always 0.
`frameRate != 0` in 8 files only (max 35 fps).

`bFollowCamera` true in 5 emitters across `birds.pe`, `downpour.pe`, `heavy-rain3.pe`.
`bFindFloor` true in 9 emitters (the 6 explosion decals, `Explosion Burst.pe`, `Steam.pe`,
`downpour.pe`).

Emitters per file: 1 (7 files), 2 (8), 3 (3), 4 (7), 5 (2).

### 7.3 Materials

All 70 materials parsed. Blend modes: **44 × `ADDITIVE`, 26 × `ALPHA`**. Nothing else — no
`MULTIPLY`, no `PREMULTIPLIED`, so the un-initialised blend-state hazard of §6.1 is never hit by
shipped content.

`baseColor.w` (→ `xEmitterOpacity`) values seen include `0.10, 0.20, 0.21, 0.25, 0.33, 1.0, 1.11,
1.25, 1.46, 1.5, 1.93, 3.17, 3.48, 8.85, 9.13, 10.66, **20.0**`.
`emissiveColor.w` values include `0, 0.2, 0.42, 0.5, 0.73, 1.0, 1.33, 1.39, 1.74, **20.0**`.
`alphaRef` is `1.0` throughout (unused by the particle PS).

Each material's `BASECOLORMAP` name is exactly `<peStem><index>_color.png` — e.g.
`Steam0_color.png`, `wpe0_color.png` … `wpe4_color.png`. This is the sibling-texture convention
(§8.5).

---

## 8. The DX11 game-side WPE layer

All line numbers below are in `D:\max\GameGuruMAX\GameGuru Core\`.

> The DX11 tree has a **single monolithic** `Guru-WickedMAX\wickedcalls.cpp` (285 KB). The
> `wickedcalls_part0..4.cpp` split exists only in the DX12 tree
> (`D:\max\GameGuruMAXDX12\...`), where the WPE features are additionally `#if false`'d out.
> DX11 `wickedcalls.cpp` is the authoritative version.

### 8.1 Feature gate

`GameGuru\Include\preprocessor-moreflags.h:116-120`:

```c
	#define NEWPROJSYSWORKINPROGRESS
	#define DETECTANDUSENEWPARTICLEDECALS

	#define WICKEDPARTICLESYSTEM
	#define CUSTOMSHADERS
```

Line 119 is `#define WICKEDPARTICLESYSTEM` — confirmed. `REMOVE_WICKED_PARTICLE` **does not exist**
anywhere in `D:\max\GameGuruMAX`, `D:\max\GameGuruMAXDX12` or `D:\max\WickedRepo` (zero hits).

### 8.2 Loading and control

`WickedCall_LoadWPE`, `Guru-WickedMAX\wickedcalls.cpp:7702-7733` (verified verbatim):

```cpp
uint32_t WickedCall_LoadWPE(char* filename)
{
	Scene& scene = wiScene::GetScene();
	uint32_t root = 0;
	uint32_t count_before = scene.emitters.GetCount();

	char path[MAX_PATH];
	strcpy(path, filename);
	GG_GetRealPath(path, 0);

	WickedCall_LoadWiScene(path, false, NULL, NULL);
	uint32_t count_after = scene.emitters.GetCount();
	if (count_before != count_after)
	{
		Entity emitter = scene.emitters.GetEntity(scene.emitters.GetCount() - 1);
		if (scene.emitters.GetCount() > 0)
		{
			HierarchyComponent* hier = scene.hierarchy.GetComponent(emitter);
			if (hier)
			{
				root = hier->parentID;
			}
		}
		wiEmittedParticle* ec = scene.emitters.GetComponent(emitter);
		if (ec)
		{
			ec->Restart();
			ec->SetVisible(false);
		}
	}
	return root;
}
```

Behaviour to preserve:

* Path goes through `GG_GetRealPath(path, 0)` (`Dark Basic Pro SDK\Shared\File\CFileC.cpp:337`) —
  absolute-ises, normalises separators, redirects install-folder paths into the writable/docs folder.
* The whole archive is merged into the live scene every call. **No caching in `LoadWPE` itself**;
  the pooling lives in `preload_wicked_particle_effect` (`M-Entity.cpp:8577`) with
  `ready_decals[MAXUNIQUEDECALS=100][MAXREADYDECALS=5]` (`M-Entity.cpp:31-34`).
* The returned handle is the **`.PE`'s own authored parent entity**, found by taking the *last*
  emitter in the merged scene and following `parentID` exactly **one** level. Multi-level `.PE`
  hierarchies would return the wrong root.
* Only the **last** emitter gets `Restart()` and `SetVisible(false)`. Sibling emitters in the same
  `.PE` keep whatever `bVisible` they were constructed with (`true`). The effect is therefore
  *loaded hidden only in part* until action 5 is issued.
* Texture resolution happens one level down in `WickedCall_LoadWiSceneDirect`
  (`wickedcalls.cpp:7645-7657`), which calls `WickedCall_LoadImage` for every non-empty texture
  name below `EMISSIVEMAP`. The `userBlendMode` enum remap at `:7660-7675` is guarded by
  `pestrcasestr(filename, ".wiscene")` and therefore **does not apply to `.PE`**.

`WickedCall_PerformEmitterAction(int iAction, uint32_t emitter_root)`,
`wickedcalls.cpp:7735-7797`. Documented by its own header comment (`:7735`):

```cpp
//iAction = 1 Burst all. 2 = Pause. - 3 = Resume. - 4 = Restart - 5 - visible - 6 = not visible. - 7 = pause emit - 8 = resume emit
```

It linearly scans **all** emitters in the scene and applies the action to every one whose
`HierarchyComponent::parentID == emitter_root` — so one root fans out to all emitters of the `.PE`.

| Action | Call | Line |
|---|---|---|
| 1 | `ec->Burst(0);` | 7755 |
| 2 | `ec->SetPaused(true);` | 7760 |
| 3 | `ec->SetPaused(false);` | 7765 |
| 4 | `ec->Restart();` | 7770 |
| 5 | `ec->SetVisible(true);` | 7775 |
| 6 | `ec->SetVisible(false);` | 7780 |
| 7 | `ec->SetEmitPaused(true);` | 7785 |
| 8 | `ec->SetEmitPaused(false);` | 7790 |

`Burst(0)` always → quantity comes from the authored `burst_amount`.

**Ordering matters:** the correct sequence used at runtime is 3 (Resume), 4 (Restart), 5 (Visible),
1 (Burst) — `M-Entity.cpp:8875-8878`. The editor preview path issues 1 before 4
(`imgui_gg_dx11.cpp:7847-7849`), and `Restart()` clears `buffersUpToDate`, so the burst is
effectively wiped there.

Other WPE entry points, all `wickedcalls.cpp`:

| Function | Line | Behaviour |
|---|---|---|
| `WickedCall_ParticleEffectPositionRotation(root, x,y,z, xa,ya,za)` | 7799 | `ClearTransform(); RotateRollPitchYaw(deg→rad); Translate(); UpdateTransform();` on the **root** transform |
| `WickedCall_ParticleEffectPosition(root, x,y,z)` | 7820 | same minus rotation |
| `WickedCall_UpdateEmitters(void)` | 7835 | per-frame; see §8.3 |
| `WickedCall_CreateEmitter(...)` | 7936 | **dead code**, zero call sites |
| `GetVisibleWEmitters(void)` | 8027 | **dead code**, zero call sites, not in the header |

`WickedCall_UpdateEmitters` is called exactly once per frame from
`Guru-WickedMAX\master.cpp:2154-2158`, after terrain/grass update, immediately before
`GuruLoopRender()`.

### 8.3 `bFollowCamera` / `bFindFloor` — what the DX12 port stubbed out

`wickedcalls.cpp:7883-7932`:

```cpp
		//PE: If bFollowCamera , find InDoor , OutDoor , UnderWater.
		//PE: bFindFloor ONLY if ec->bFollowCamera
		if (ec && (ec->bFindFloor || ec->bFollowCamera))
		{
			HierarchyComponent* hier = scene.hierarchy.GetComponent(emitter);
			if (hier)
			{
				if (hier->parentID != wiECS::INVALID_ENTITY)
				{
					bool bAlreadySet = false;
					for (int a = 0; a > parent_used.size(); a++)
					{
						...
					}
					if (!bAlreadySet)
					{
						parent_used.push_back(hier->parentID);
						TransformComponent* root_tranform = scene.transforms.GetComponent(hier->parentID);
						if (root_tranform)
						{
							if (ec->bFollowCamera)
							{
								float fX, fY, fZ;
								fX = CameraPositionX();
								//PE: PlayerHeight might have to be removed in GGM
								fY = CameraPositionY(); // -PlayerHeight;
								fZ = CameraPositionZ();
								root_tranform->ClearTransform();
								root_tranform->Translate(XMFLOAT3(fX, fY, fZ));
								root_tranform->UpdateTransform();
							}
							if (ec->bFindFloor && ec->bFollowCamera)
							{
								float fX = root_tranform->GetPosition().x;
								float fZ = root_tranform->GetPosition().z;
								float height = BT_GetGroundHeight(t.terrain.TerrainID, CameraPositionX(), CameraPositionZ());
								root_tranform->ClearTransform();
								root_tranform->Translate(XMFLOAT3(fX, height, fZ));
								root_tranform->UpdateTransform();
							}
						}
					}
				}
			}
		}
```

Net effect, exactly as shipped:

1. **`bFollowCamera`**: every frame, the emitter's *root* transform is reset and translated to the
   camera position (including camera Y — the `-PlayerHeight` is commented out). Rotation and scale
   are destroyed by `ClearTransform()`. This is how `downpour`/`heavy-rain3`/`birds` follow the player.
2. **`bFindFloor`**: only takes effect **when `bFollowCamera` is also true** (the inner condition is
   `&&`). It then re-plants the root at the terrain height sampled **at the camera's X/Z**, keeping
   the X/Z the follow-camera step just wrote.
3. The dedup loop is `for (int a = 0; a > parent_used.size(); a++)` — condition is `>`, so it
   **never iterates**. `bAlreadySet` is always false, `parent_used` is written but never read, and
   every emitter sharing a root re-applies the transform. `parent_used` is also a `std::vector`
   cleared and re-grown **every frame**.
4. `bTriggerOutDoor` / `bTriggerInDoor` / `bTriggerUnderWater` are promised by the comment but
   **no implementing code exists anywhere**, and they are not serialised. Purely vestigial.

### 8.4 Per-instance overrides: `newparticletype` and what WPE ignores

`newparticletype` is declared at `GameGuru\Include\Types.h:5422-5523` (not `gameguru.h`), 46
members. The `-123.0f` sentinel marks "original not yet captured" for revertable overrides.

Branch point: `newparticle_updateparticleemitter(...)`, `M-Entity.cpp:8717`; the WPE branch is
`if (pParticle->bWPE)` at `:8840`, the legacy `gpup_` branch is the `else` at `:8883`.
Shared show/hide gate, `M-Entity.cpp:8723-8729`:

```cpp
	bool bShowThisParticle = false;
	extern bool bImGuiInTestGame;
	if (bImGuiInTestGame == true)
		bShowThisParticle = pParticle->bParticle_Show_At_Start;
	else
		bShowThisParticle = pParticle->bParticle_Preview;
```

**The entire WPE branch body sits inside `if (pParticle->bParticle_Fire == true)`**
(`M-Entity.cpp:8847`). When the fire flag is clear the branch does nothing at all — not even
repositioning — so a WPE effect attached to a moving entity does not follow it.

When it *does* fire, it forces a camera-facing look-at on the root and then issues 3/4/5/1
(`M-Entity.cpp:8849-8879`):

```cpp
						XMVECTOR lookDir = XMVectorSubtract(cameraPos, emitterPos);
						lookDir = XMVectorSetY(lookDir, 0.0f);
						...
							XMMATRIX lookAtMat = XMMatrixLookToLH(emitterPos, lookDir, worldUp);
							XMMATRIX worldMat = XMMatrixInverse(nullptr, lookAtMat);
							root_tranform->MatrixTransform(worldMat);
						...
					WickedCall_PerformEmitterAction(3, iParticleEmitter); // Resume
					WickedCall_PerformEmitterAction(4, iParticleEmitter); // Restart
					WickedCall_PerformEmitterAction(5, iParticleEmitter); // Visible
					WickedCall_PerformEmitterAction(1, iParticleEmitter); // Burst All
					pParticle->bParticle_Fire = false;
```

**Override matrix** (legacy `gpup_` path shown for contrast):

| Property | legacy `gpup_` | **WPE branch** |
|---|---|---|
| speed (`bParticle_SpeedChange`, `fParticle_Speed`) | honoured, `M-Entity.cpp:8933-8944` | **ignored** |
| opacity (`bParticle_OpacityChange`) | honoured, `:8947-8958` | **ignored** |
| size (`bParticle_SizeChange`) | honoured, `:8961-8972` | **ignored** |
| colour (`bParticle_ColorChange`, R/G/B) | honoured, `:9008-9019` | **ignored** |
| lifespan (`bParticle_LifespanChange`) | honoured, `:9022-9033` | **ignored** |
| bounciness (`fParticle_BouncinessChange`) | honoured, `:8982-8993` (with ÷5 / ×5 scaling) | **ignored** |
| floor height (`iParticle_Floor_Active`) | honoured, `:8994-9005` | **ignored** (written by `M-Decal.cpp:250`, never read) |
| looping (`bParticle_Looping_Animation`) | honoured, `:8921-8924` | **ignored** (written by `M-Decal.cpp:252`, never read) |
| offsets (`bParticle_Offset_*`) | honoured, `:8887-8897` | **ignored** |
| local rotation (`bParticle_LocalRot_*`) | honoured, `:8901-8916` | **ignored** — and `M-Decal.cpp:545-549` *computes* a spray angle into `bParticle_LocalRot_Y` for the WPE path which is then discarded |
| global position (`fX/fY/fZ`) | every call, `:8886` | **only on fire** |
| global rotation (`fRX/fRY/fRZ`) | honoured, `:8917` | **ignored** (replaced by forced camera look-at) |
| global scale (`fScale`) | honoured, `:8918` (`100.0f + fScale`) | **ignored** |
| show/preview | two-way (`gpup_emitterActive(id,1/0)`, `:8927-8930`) | **one-way** — gates creation and firing, never hides |
| burst/fire (`bParticle_Fire`) | honoured, `:8975-8979` | **honoured** — the sole driver |
| `emittername`, `emitterid`, `bWPE`, `iMaxCache` | — | honoured |
| full-screen (`bParticle_Full_Screen`, `fParticle_Fullscreen_*`) | ignored | ignored (no runtime consumer found in either path) |

**Conclusion: everything a WPE effect looks like is baked into the `.PE` at authoring time.**
Any replacement must decide deliberately whether to keep that (fidelity) or fix it (usability).

WPE decals additionally must be **burst-only**: `preload_wicked_particle_effect` rejects any
emitter with `count > 0.1f` (`M-Entity.cpp:8680-8704`) and `M-Decal.cpp:258-262` then falls back to
the legacy `.arx` path with `t.decal[...].newparticle.bWPE = false;`.

### 8.5 Where `.PE` files live, and the sibling textures

* Editor browsing scans two roots, both `Files\particlesbank\wpe`
  (writable + docs), `imgui_gg_dx11.cpp:7232-7247`.
* Decals use a fixed per-decal filename: `M-Decal.cpp:222`
  ```cpp
	t.strwork = ""; t.strwork = t.strwork + "gamecore\\decals\\" + t.decal_s + "\\wpe.pe";
  ```
  Existence of `wpe.pe` opts a decal into the Wicked path; otherwise `M-Decal.cpp:266-276` falls
  back to `gamecore\decals\<name>\newparticle.arx` under `DETECTANDUSENEWPARTICLEDECALS`.
* Sibling textures, `M-MapFile.cpp:5616-5646` (`AddWPETextures`): the string is truncated at the
  first case-insensitive `".pe"` and then `0_color.png` … `6_color.png` are appended and registered
  with `addtocollection` for standalone export.

  ```
  <stem>0_color.png, <stem>1_color.png, ... <stem>6_color.png
  ```

  There is **no separator** before the digit. Index `N` corresponds to emitter/material `N` in the
  `.PE` — confirmed by parsing the materials (§7.3): `Steam.pe` → `Steam0_color.png`;
  `gamecore\decals\explosion\wpe.pe` → `wpe0_color.png` … `wpe4_color.png`.
  The hard-coded 0–6 range means **an 8th emitter's texture would be silently missing from
  standalone builds**.

---

## 9. Quirks that must be preserved (or consciously fixed)

### 9.1 Modulo-by-zero XZ spawn offset — hits 100 % of shipped content

`emittedparticle_emitCS.hlsl:93-95` runs unconditionally for every non-volume, non-mesh emitter:

```hlsl
        float fixedseed = xTotalEmitCount % (uint) xParticleRandomPos;
        pos.x += (rand(fixedseed, float2(321.123, xParticleRandomPosScale)) - 0.5f) * xParticleRandomPosScale;
        pos.z += (rand(fixedseed, float2(xParticleRandomPosScale, 456.32)) - 0.5f) * xParticleRandomPosScale;
```

**Every `.PE` in the shipped corpus has `random_position == 0` and `random_position_scale == 1`**
(§7.2). Therefore:

* `(uint)xParticleRandomPos == 0` and the integer `%` is a divide-by-zero. The D3D shader model
  defines integer divide-by-zero as returning `0xFFFFFFFF`, so `fixedseed` becomes a large constant
  — but this is **implementation-defined in practice** and I am flagging it as *not fully certain*:
  I have not captured the actual value on hardware.
* Both `rand()` inputs are then compile-time constants, so the two offsets are **constant** — the
  same value for every particle, every frame, for the life of the process. It is a fixed emitter
  origin displacement of up to ±0.5 world units in X and Z, **not** per-particle jitter.
* Volume emitters (`FLAG_HAS_VOLUME`, 22 of 70 records) are exempt — the lines live in the
  `#else` of `EMITTER_VOLUME`.

**Recommendation:** reproduce as "if `random_position == 0`, apply a fixed constant XZ offset
derived from a constant seed" and verify empirically against a DX11 capture before deciding whether
to drop it. Do not silently remove it — several effects will shift by up to half a unit.

### 9.2 Drag is per-step, not per-second

`simulateCS:109` `particle.velocity *= xParticleDrag;` runs once per simulation step, with a
**variable** `dt` (`FIXED_TIMESTEP == -1` in 100 % of the corpus). A `drag < 1` therefore decays
faster at high frame rates. The shipped corpus has `drag == 1` in **every single emitter**, so this
never bites in practice — but it means "no drag" is the only shipped behaviour and any port is free
to implement drag correctly.

### 9.3 `emit` is truncated to an integer for the GPU

`wiEmittedParticle.cpp:331` `cb.xEmitCount = (uint32_t)emit;` while `cpp:200`
`emit = std::max(0.0f, emit - floorf(emit));` carries the fraction to the next frame. Rates below
1/frame accumulate correctly; the burst path adds whole numbers.

### 9.4 Statistics readback is opt-in

`cpp:265` `if (IsStatActive())` (GG-local, `54b3715`) wraps the `Map`/`memcpy` readback, but
`cpp:661` still does the `CopyResource` into the staging ring unconditionally. `bStatActive`
defaults false and nothing in the DX11 game turns it on, so `GetStatistics()` returns zeros.

### 9.5 Dead fields to not bother porting

`random_factor`, `scaleY`, `mass`, `spawn_pause`, `spawn_pause_random`, `wpe_filler_1..3`,
`bTriggerOutDoor/InDoor/UnderWater` — all serialised (except the three booleans, which are not even
that) and all read by nothing. `SPH_*` are read but SPH is never enabled.

### 9.6 `emittedparticlePS_simple.hlsl` is not a particle shader

It returns flat grey. If a `.PE` ever selected `SIMPLE` the particles would render as untextured
grey quads. None do.

### 9.7 Mesh-shader path is compiled but never enabled

`wiEmittedParticle.cpp:49` `static bool ALLOW_MESH_SHADER = false;`.

---

## 10. Open questions / things I could not determine

1. **The exact value of `xTotalEmitCount % 0` on the shipping hardware** (§9.1). The D3D spec says
   `0xFFFFFFFF`, but I have not verified it in a capture, and the subsequent
   `sin(huge_value)` is precision-sensitive. The *magnitude* of the resulting offset is bounded by
   `±0.5 · random_position_scale`; the *sign and value* need a hardware capture to pin down.
2. **`bParticle_Full_Screen` and the `fParticle_Fullscreen_*` group.** I found serialisation and
   editor storage but **no runtime consumer in either particle branch**. Reporting as "no consumer
   found", not "definitively absent".
3. **Whether any user-authored (non-shipped) `.PE` in the wild uses `FLAG_FRAME_BLENDING`, `SIMPLE`,
   `MULTIPLY` blending, or a non-zero `random_position`.** The 27 shipped files use none of them,
   so those code paths are untested by the shipping product.
4. **`start_rotation`'s intended semantics** (§6.2). The `tan(normalize(float3))`-truncated-to-scalar
   expression is very likely a bug, but only one shipped emitter sets the field, so there is no
   strong visual reference to check against.
5. **Whether the `.PE` corpus was ever re-saved at v5077 for the whole set.** Only 2 of 27 files are
   v5077, which means `distance_sort_bias` is effectively unused shipped content.

---

## Appendix A — reproduction scripts

Left in the scratchpad alongside this document (they only *read* the game data folder):

| File | Purpose |
|---|---|
| `pe_dump.py` | Locates and decodes every `wiEmittedParticle` record in a `.PE` by anchoring on the 48 trailing zero bytes; prints all fields. |
| `pe_mat.py` | Full forward walk of the archive (resources → names → layers → transforms → prev → hierarchy → materials); prints blend mode, base colour, emissive and texture name per material. Parses 27/27 shipped files cleanly. |
| `pe_corpus.txt` | Full field dump of all 70 emitters. |
| `pe_table.txt` | One-line-per-emitter summary table. |
