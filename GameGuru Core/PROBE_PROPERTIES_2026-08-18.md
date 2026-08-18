# Env Probe marker properties — audit and fix (2026-08-18, GGMAX 2.90)

Lee's report: *"probe range and probe brightness in the properties of the env probe do not
seem to do anything. Size XYZ works fine — I can see half the fish using the local rather
than the global env map — but I suspect the other 2 properties do nothing (or nothing
useful)."*

Both were dead. They were dead for **different reasons**, and only one was a DX12 port
regression. This document is the audit and what shipped.

---

## 1. Probe Brightness — a real port regression, one line from working

### The chain

| Stage | Location |
|---|---|
| Panel slider | `GameGuru/Source/M-GridEditB_part12.cpp` (`##fLightfProbeBrightness`) |
| → `eleprof.light.fProbeBrightness` | serialized in the .ele (`M-Entity_part4.cpp:569` / `M-Entity_part3.cpp:732`) |
| → passed to terrain | `G-Lighting.cpp:232` → `GGTerrain_AddEnvProbeList(..., fProbeBrightness)` |
| → stored | `GGTerrain_part0.cpp:9278` `item.brightness = brightness` |
| → **dead end** | `GGTerrain_part0.cpp:9500` `//probe->SetBrightness(g_envProbeList[p].brightness);` |

`item.brightness` had exactly one writer and one reader, and the reader was commented out.

### Why it was commented out

The DX12 Wicked engine **deleted the feature**. DX11 had it end to end:

- `WickedRepo/WickedEngine/wiScene.h:1031,1034` — `float filterBrightness = 1.0f;` + `SetBrightness`
- `wiRenderer.cpp:9092` — `cb.filterBrightness = probe.filterBrightness;`
- `filterEnvMapCS.hlsl:43` — `col.rgb *= filterBrightness;`

In the DX12 engine the member, the setter, the CB field and the shader multiply were all
absent, so the port had no choice but to stub the call.

### What shipped (engine delta 2.90)

Restored DX11's design — the brightness is **baked into the cube** during BRDF mip
filtering, so it costs nothing at shading time:

| File | Change |
|---|---|
| `wiScene_Components.h` | `float filterBrightness = 1.0f;` on `EnvironmentProbeComponent` + `SetBrightness()` |
| `ShaderInterop_Renderer.h` | `filterBrightness` in `FilterEnvmapPushConstants` — took the **free padding slot**, so struct size and layout are unchanged |
| `wiRenderer.cpp` | `push.filterBrightness = probe.filterBrightness;` in the filter loop |
| `filterEnvMapCS.hlsl` | `accum.rgb *= push.filterBrightness;` after the ray average |
| `GGTerrain_part0.cpp:9500` | the call site uncommented |

Two deliberate departures from a literal DX11 copy:

1. **`SetBrightness` self-dirties, but only on change.** Brightness is baked, so a change
   must force a re-capture — and the caller runs on every probe-tracking update, so an
   unconditional `SetDirty()` would re-bake the cube forever. `if (value != filterBrightness)`
   gives exactly one re-bake per slider move. (`SetDirty()` → `DeleteResource()` is a no-op
   for GG's runtime-baked probes: it only clears asset-sourced ones, where `resource` is
   valid. No black-cube flash.)
2. **`filterBrightness` is NOT serialized**, sitting with `position`/`range` in the
   non-serialized block. The source of truth is the GG entity profile — which *is* saved —
   and `GGTerrain_EnvProbeWork` re-pushes it every tracking update.

### ⚠ Known gap: mip 0

The filter loop is `for (i = mip_levels-1; i > 0; --i)` — **mip 0 is never filtered**, it is
a straight `CopyResource` of the unfiltered render. So brightness covers mips 1..N-1 and
mirror-sharp (roughness ≈ 0) reflections ignore the slider; a chrome prop at roughness 0.1
samples between mip 0 and 1 and gets a partial effect. **DX11 had the identical gap**, so
this is parity, not a new defect. Closing it needs a scale pass over `envrenderingColorBuffer`
mip 0 before `GenerateMipChain` — a separate dispatch, deliberately not done here.

### The global probe stays on the 1.55 shader knob

`GGTerrain_part0.cpp:~9770` (`probe->SetBrightness(GetEnvProbeBrightness())`) is left
commented **on purpose**. The Visuals panel's "Env Probe Brightness" already drives
`wi::scene::gg_envprobe_brightness`, which multiplies inside `EnvironmentReflection_Global`
(`lightingHF.hlsli:727`, engine delta 1.55). Baking it as well would apply the same slider
twice — squared. One owner per knob; the shader-side one also responds live with no re-bake.

---

## 2. Probe Range — never did anything, in DX11 either

GG writes it (`GGTerrain_part0.cpp:9482` `probe->range = range`), but the engine
**overwrites it every frame**:

```cpp
// wiScene.cpp:5734, Scene::RunProbeUpdateSystem
probe.range = std::max(scale.x, std::max(scale.y, scale.z)) * 2;
```

`range` is a non-serialized derived attribute, recomputed from the transform on every
`Scene::Update`. GG's value is clobbered before anything reads it. **The DX11 engine has the
identical line** (`WickedRepo/WickedEngine/wiScene.cpp:4190`), so this is not a port
regression — the slider was inert there too.

The probe volume comes solely from `pTransform->Scale(fSX, fSY, fSZ)` — Size X/Y/Z
(`GGTerrain_part0.cpp:9508`; DX11 `GGTerrain.cpp:9413`, same). That is exactly why Size XYZ
works on the fish and Range does nothing. The shading path never reads a probe's range at
all; the only GPU consumer is the coarse tile-cull sphere (`wiRenderer.cpp:5682` →
`lightCullingCS.hlsl:337`), already derived from the scale.

The UI tooltip promised *"base range which is then later scaled XYZ"*. That was never
implemented in either renderer — the range is not multiplied into the XYZ scale anywhere.

### What the value actually is

A **flag, not a magnitude**. `fLightHasProbe` is literally "has probe", and every consumer
is a `>= 50.0f` boolean test:

| Site | Meaning |
|---|---|
| `G-Lighting.cpp:236` | admits the marker to the probe list at all |
| `M-GridEditB_part12.cpp:16`, `M-GridEditB_part13.cpp:1072` | `bIsLightProbe = true` → shows the probe panel |
| `M-GridEdit_part7.cpp:1604,1660`, `part3.cpp:1476,1596,1659`, `part1.cpp:4076` | probe-vs-light branching |

The slider's own minimum was 50, so every reachable value 50–500 behaved identically.

### What shipped (game, 2.90)

1. **The slider is removed** from the marker properties panel
   (`M-GridEditB_part12.cpp`). Size X/Y/Z remain — they are the real control.
2. **The value is canonicalised to 50 on load** (Lee-authorised: *"you are free to replace
   any level-set value with one that works better"*), so the field carries one meaning
   instead of arbitrary noise:
   - `M-Entity_part3.cpp:461` (.ele level load) — `if (>= 50) = 50`
   - `M-Entity_part1.cpp:1163` (.fpe `lightprobescale`) — the flag is canonicalised to 50
     while **X/Y/Z keep the authored value**, since those are the actual probe volume.

   Values below 50 mean "not a probe" and are left untouched.

### ⚠ If Probe Range is ever revived

Making it real would mean multiplying it into the OBB scale. Note the interaction with the
2.89 fp16 fix: Range 500 × Size 500 puts a *local* probe at half-extent 250,000, well past
the 37,820 parallax ceiling. The shipping `probeparallax=3` guard would catch it and
silently drop parallax on that probe. It would also change every existing level's probe
volumes.

---

## 3. Harness

`SET_PROBEMARKERBRIGHTNESS <f>` (2.90) drives the **per-probe** panel slider on every probe
marker in the level and raises `g_bLightProbeScaleChanged`, exactly as the panel does — the
probe list rebuilds, `SetBrightness` self-dirties and the cube re-bakes. Distinct from
`SET_ENVPROBE_BRIGHTNESS`, which is the global shader knob.
