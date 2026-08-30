---
name: project-shader-build-pipeline
description: "How GGMAX shaders actually (re)compile — the non-obvious split that decides whether a shader edit needs a game rebuild. ENGINE shaders (WickedEngine/shaders/*.hlsl) recompile AT RUNTIME on next launch — but the runtime's own freshness check (IsShaderOutdated/.wishadermeta) SILENTLY FAILS on stale/foreign metas (caused the 2026-07-23 stale-CSO grass bug); now backstopped by refresh_shaders.ps1 (engine a3f08948 / game ddde22be), a build-time source-newer-than-cso check wired into BOTH build_wicked.bat and build.bat that deletes stale engine .cso so they recompile. GGTerrain/game shaders (GGTerrain/Shaders/*.hlsl incl PBR/lightingHF.hlsli) do NOT runtime-recompile (source not on SHADERSOURCEPATH, no .wishadermeta) — they build via the GAME project's MSBuild FxCompile items, so editing them needs a GAME rebuild (build.bat Release). Touch the top-level .hlsl to force FxCompile if an include-only edit isn't picked up."
metadata:
  node_type: memory
  type: reference
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
  modified: 2026-07-23T21:43:41.322Z
---

# Shader (re)compile pipeline — two separate mechanisms

Decisive when planning any shader edit: does it need a rebuild, and which one?

## ENGINE shaders — `D:\max\WickedEngineDX12\WickedEngine\shaders\*.hlsl(i)`
- Auto-recompile **at runtime on next launch** when source is newer than the
  `.cso`. Path: `wiRenderer::LoadShader` -> `wi::shadercompiler::IsShaderOutdated`
  -> recompile from `SHADERSOURCEPATH` (= `SHADER_INTEROP_PATH`, baked from
  `__FILE__` = the engine `shaders/` dir) -> writes new `.cso` + `.wishadermeta`
  to the runtime `shaders/` folder. `.wishadermeta` records include deps, so
  editing an include (e.g. engine `lightingHF.hlsli`, `ShaderInterop_*.h`)
  marks every dependent outdated.
- So: edit engine shader/include -> just relaunch MAX. (A **C++/CB** change in
  `ShaderInterop_*.h` ALSO needs `build_wicked.bat Release` for the lib; the
  shader side auto-recompiles at runtime — both must land before running or the
  CPU/GPU struct layout mismatches = corruption.)

## GGTerrain / game shaders — `GameGuru Core\Guru-WickedMAX\GGTerrain\Shaders\*.hlsl(i)` (incl. `PBR\lightingHF.hlsli`)
- **Do NOT runtime-recompile** — their source isn't on `SHADERSOURCEPATH`
  (engine dir) and they ship no `.wishadermeta`, so `IsShaderOutdated` can't
  find/flag them; `LoadShader` just loads the existing `.cso`.
- They are compiled by the **GAME project's MSBuild `FxCompile` items** (66 of
  them in `Guru-WickedMAX\Template_Windows.vcxproj`, include dirs =
  `WickedEngine\shaders;GGTerrain\Shaders`). So a GGTerrain shader edit needs a
  **game rebuild** (`build.bat Release`) — the `.cso` land in the runtime
  `shaders/` folder.
- Include-only edits (e.g. `PBR/lightingHF.hlsli`) SHOULD trigger dependents via
  MSBuild's tracker, but to be safe `touch` the top-level `.hlsl` that includes
  it (e.g. `GGTerrainVirtualPBR_PS.hlsl`, `GGGrassPS.hlsl`) so FxCompile
  definitely rebuilds. Verify by checking the `.cso` timestamp afterwards.
- The terrain "sand under water" surface = `GGTerrainVirtualPBR_PS` (includes
  `PBR/lightingHF.hlsli`). `GGTerrainVirtualPS.cso` doesn't exist (variant not
  built) — don't chase it.

## Session findings that used this (2026-07-19)
## SEABED CAUSTICS — where they REALLY live (2026-07-20, cost hours to find)

**The live seabed caustics are in the ENGINE shader
`D:\max\WickedEngineDX12\WickedEngine\shaders\lightingHF.hlsli`** — the
`texture_caustics` block inside `DirectionalLight`, sampled at
`surface.P.xz * ocean.patch_size_rcp`. That `patch_size_rcp` is why the
**Water Tiling Patch Size** slider changes caustic size (and wave size with it).

**The GG copy `GGTerrain\Shaders\PBR\lightingHF.hlsli` is DEAD CODE for terrain.**
`GGTerrainVirtualPBR_PS.hlsl` includes it but line ~370 has
`//ForwardLighting(surface, lighting);` **commented out** and calls
`GGTiledLightingWithAmbient` (GGLighting.hlsli) instead, whose `DirectionalLight`
resolves to the ENGINE's. Proven by injecting a flat colour at the top of GG's
`DirectionalLight` — terrain never tinted. Hours were burned editing that file.

**Wicked delta 1.22 — COMMITTED (engine `fc8e310a`, game `05a38598`):
`ShaderOcean::caustic_scale`** (`ShaderInterop_Weather.h` + marshalled in
`wiScene.cpp` next to `patch_size_rcp`) scales ONLY the caustic lookup ->
caustic size independent of patch_length/waves. 1.0 = stock. GG drives it from
the Water panel "Caustic Size" slider (`visuals.fWaterCausticSize`, scale =
1/size). Measured: 1.0 fine stock speckle, 0.3 larger cells, 0.1 broad soft
blobs. **DEAD-CODE FOLLOW-UP — DONE (`e69a10c4`):** stripped the game's earlier
caustic plumbing (`14b90e0a`/`565f704b` — GGCustomFrameCB `causticScale`,
GGFrameCompat macro, game `PBR/lightingHF.hlsli` block); GGCustomFrameData
shrank 96->80 bytes. **Correction to the old claim:** the game
`PBR/lightingHF.hlsli` block was NOT on a dead code path — `GGLighting.hlsli`
calls this game `DirectionalLight` from every terrain/grass/tree PS. It was
inert because its gate `OPTION_BIT_WATER_ENABLED` (`1<<22` in the GG PBR
`ShaderInterop_Renderer.h`) **aliases the engine FrameCB's
`OPTION_BIT_DEBUG_NORMAL_VIS`** (`g_xFrame_Options = GetFrame().options`), which
is off in production — so the gate never passes. That same bit-22 collision
gates the other "underwater" GG effects OFF too (see below).

**METHOD LESSON (paid for in hours):** before tuning ANY shader effect, first
prove the code path is live — inject a flat unmistakable colour and confirm it
appears on screen. Then prove you can DISABLE the effect. Only then tune. Also:
comparing screenshots from different camera positions is worthless; pin the
camera or you will "verify" changes that never happened.

- **(superseded, kept for context)** the game-side caustic work in
  `GGTerrain\Shaders\PBR\lightingHF.hlsli` (its
  OWN procedural formula, NOT the engine's baked `texture_caustics` path):
  `ocean_uv = (surface.P.xz + surface.P.yy*0.25) * g_xFrame_CausticScale`.
  Stock 0.0045 was metre-authored -> tiled too tight on inch terrain. Now a
  **persisted "Caustic Size" water slider** (Water panel, `M-TerrainNew_part3`;
  visuals field `fWaterCausticSize`, default 3.0 -> shader scale 0.00067,
  ~3x bigger cells than the 0.0020 first pass). Commits `14b90e0a` (first pass
  const) + `565f704b` (slider). Same inch-scale class as ocean foam (1.10) /
  normals (1.1).
- **`GGCustomFrameCB` (register b4) is THE clean injection point for a GG-wide
  per-frame shader value** — GG-owned (NOT the engine FrameCB), already bound to
  every terrain/grass/tree shader, and rebuilds entirely in the game build so no
  engine change / no layout-mismatch risk. Recipe: add a field at the END of
  `GGCustomFrameData` in `GGCustomFrameCB.hlsli` (pad struct to a 16-byte
  multiple) -> add a `g_xFrame_*` macro in `GGFrameCompat.hlsli` -> fill
  `ggCustomFrameStaging.<field>` (GGTerrain_part0.cpp for ocean-derived,
  `M-GridEditB_part3.cpp` ~1917 for visuals-derived). causticScale was done this
  way; follow it for any future shared shader knob rather than touching the
  engine FrameCB. To PERSIST a knob per-level: add a `visuals.` field — water
  settings serialize as backward-compatible text KV (M-Visuals_part0.cpp save
  ~886 / load ~1506), + gamevisuals sync (M-GridEdit_part2 ~1794).
  **CRITICAL (fixed `2bbe09f6`): the b4 CB is uploaded ONCE per frame by
  `GGCustomFrame_Update` (GGTerrain_part0.cpp) — which for a long time only ran
  in the SKIPPED legacy tail of `GGTerrain_Update` (after the Wicked-terrain
  early return ~line 9958).** In Wicked mode (today's default) the CB was
  therefore NEVER uploaded and EVERY field read as zero in the GG shaders
  (waterHeight, water/fog colour, tree wind, cloud params, desaturate...). Proven
  with a `GET_PERF_DATA` WH_* trace: island water line 2.00 all the way to
  `scene.weather` but 0.00 in the CB, `GGCustomFrame_Update` run count 0. Fix =
  call `GGCustomFrame_Update(cmd)` before that early return. Side effect: this
  re-enabled tree wind (GGTreesConstants), GG distance-fog tint (`ApplyFogCustom`
  -> GetFogColor/GetFogOpacity in GGCommonFunctions) and the water-overlay tint,
  all silently zero before — correct per visuals; island beach A/B unchanged
  (near geometry), fog-heavy levels will now tint at distance. Harness readout
  `WH_GDEFAULT/COMPONENT/SCENE_WEATHER/GGCB` + `GGCustomFrame_GetWaterHeight()`.
- **RESOLVED (`a276db4c`) — bit-22 collision killed, underwater handed to Wicked.**
  The old GG underwater fog (`ApplyFogCustom` underwater branch, GGCommonFunctions.hlsli:8)
  was gated by `OPTION_BIT_WATER_ENABLED = 1<<22`, which aliases the engine FrameCB's
  `OPTION_BIT_DEBUG_NORMAL_VIS` (`g_xFrame_Options = GetFrame().options`). It only ever
  fired ACCIDENTALLY, by pressing the **'I' debug key** (GGTerrainWicked.cpp:1917 ->
  SetDebugNormalVis raises engine bit 22). Decision (user): retire the old GG underwater
  system, rely entirely on Wicked. Fix = force the branch `if(false)` + delete the
  colliding `1<<22` define. A future GG frame flag must use `ggCustomFrame.ggOptions` (b4),
  NOT engine `FrameCB.options`. **Wicked's underwater IS the system now and needs no enable:**
  `underwaterCS.hlsl` (fog + colour tint + depth extinction) is dispatched whenever
  `weather.IsOceanEnabled()` (wiRenderPath3D.cpp:2392) — GG sets that — and the below-surface
  test is PER-PIXEL in the shader (`world_pos.y < ocean surface`), no C++ camera check. Plus
  engine ocean-from-below+refraction (oceanSurfacePS.hlsl), seabed caustics (engine
  lightingHF.hlsli:149), godrays clamped to the ocean plane. Verified on the island via new
  harness `SET_OCEAN waterheight <y>` (raises the ocean line to submerge the editor camera):
  surface-from-below + caustics + tint all render; above-water unchanged.
  The legacy GG water plane / reflection-camera (M-Terrain_part2.cpp) is
  already excluded from the Wicked build; the DX11 underwater screen-wave post targets a never-loaded
  effect — both inert, left alone. Optional deferred: M-Visuals_part1.cpp reflectionmode=1 poke
  (LUA-gated SetUnderwaterOn, physics-coupled swim gravity — didn't touch).
- **DONE (delta 1.23, engine `f9b51982` / game `9dad2bff`) — underwater fog DECOUPLED + water-colour
  transparency bug fixed.** Two problems solved together: (a) the underwaterCS fog colour/density were
  welded to the SURFACE `water_color.rgb`/`.a`, and (b) GG's "Water Base Color" ImGui picker
  (M-TerrainNew_part3.cpp) read its alpha as a hard-coded `1.0` and wrote `WaterAlpha=255` on every
  edit → picking a colour forced the surface fully opaque ("lost transparency"). Fix: new
  `ShaderOcean::underwater_color` + `underwater_fog_density` (density 0 = stock fallback to
  `water_color`), driven by new Water-panel **"Underwater Color"** picker + **"Underwater Fog"** slider
  (`visuals.fUnderwaterColorR/G/B` + `fUnderwaterFog`; enable block M-GridEditB_part3.cpp:
  `density = fUnderwaterFog * 0.00015`, default 20 → 0.003); and the Water Base Color picker now
  preserves the existing alpha (opaque swatch for display). So surface stays transparent when the
  colour changes, and the submerged view fogs to its own colour/distance. **Density scale is tuned on
  the TESTPRO1 deep-water vista (the user's saved "water work" camera); shallow water is naturally much
  clearer at the same value — physically correct, not a bug.** Live-tune via harness `SET_OCEAN
  waterheight <y>` (submerge the editor camera) + `SET_OCEAN uwdensity <v>`. extinctionColor is still
  unset in GG (rides on the OceanParameters default (0,0.9,1) → blue absorption via wiScene.cpp:901
  inversion) — fine, leave unless a level wants custom absorption.
- **DONE (delta 1.24, engine `77869a44` / game `fa80df27`) — "Water Base Color" now tints the
  TRANSPARENT surface from above.** After 1.23 the picker preserved WaterAlpha (good) — but GG water is
  `WaterAlpha 0` (transparent so the seabed shows), and on the SURFACE the base colour only reaches the
  eye through opacity (albedo blended by `refraction.a = exp(-water_depth*color.a)`, all gated by
  `color.a`), so at alpha 0 setting the colour did NOTHING from above (diagnosed via new harness readout
  `WATER_COLOR/WATER_ALPHA/WATER_EXTINCTION`: colour reached the shader as (1,0,0) but alpha 0). **KEY:
  the SURFACE reads a DIFFERENT ocean CB than underwaterCS** — `OceanCB` in `ShaderInterop_Ocean.h`
  (`xOcean*`, filled in `wiOcean.cpp::GetOceanCBAtDim`), NOT the `ShaderOcean` in ShaderInterop_Weather.h.
  Fix = new `OceanCB::xOceanWaterColorDepth` (`OceanParameters::water_color_depth`); `oceanSurfacePS`
  lerps `surface.refraction.rgb` toward `xOceanWaterColor.rgb` by `saturate(abs(water_depth)*depth)`,
  alpha-independent → clear shallows, coloured depths. GG sets 0.005 (M-GridEditB_part3.cpp); default
  0 = stock. Tint scales with colour saturation. Live-tune: harness `SET_OCEAN wcr/wcg/wcb/wca` (set
  surface colour 0..1) + `wcdepth <v>`. NOTE: this is ON for all GG water-enabled levels now, so existing
  water shows its base colour with depth (improvement, but a global look change).
- **`imgui_terrain_loop_v2()` is DEAD CODE** (zero call sites). The live editor
  terrain panel is **`imgui_terrain_loop_v3()`** (`M-TerrainNew_part1.cpp:6`,
  called from `M-GridEdit_part1.cpp:6887`). The Tree Shadow LOD Distance /
  Tree Shadow Range sliders were stranded in v2; surfaced into v3's Paint Tree
  panel, commit `cd666bca`. See [[project-shadow-system]].

## 2026-07-23 — STALE-CSO grass bug: ROOT-CAUSED + FIXED (engine `a3f08948`, game `ddde22be`)

**The claim above ("ENGINE shaders auto-recompile at runtime when source newer than .cso") FAILED in
practice and silently broke grass rendering.** Observed: the runtime
`shaders/hairparticle_simulateCS.cso` was OLDER (20:50) than the engine source `.hlsl` (21:52) after an
edit, **yet the runtime kept using the stale `.cso`** across relaunches — `IsShaderOutdated` did not fire.
Because `hairparticle_simulateCS.hlsl` is heavily GG-customised (reads `xHairGrassType`, paint-mask
origin/scale, and the Stage B.10 per-strand filters — paint-type, slope `target.y<0.7`, altitude band —
from a constant buffer), a `.cso` compiled against an older constant layout read those from the WRONG
offsets → every strand `strand_length=0` → **grass simulates (~59ms GPU) but draws nothing.** Symptom the
user hit as "grass gone" on TESTPRO1 AND Island Showdown (mask/data present, no hair). Delta 1.25's
cull-skip (`3273b651`) is INNOCENT — grass renders with it ON and OFF; the recompile was the differentiator.

**ROOT CAUSE (confirmed by reading `wiShaderCompiler.cpp:795 IsShaderOutdated`):** it trusts a `.cso` as
fresh in TWO silent-fail cases — **(a)** the `.wishadermeta` is missing → returns `false` (up to date); and
**(b)** the meta exists but its recorded dependency paths (stored RELATIVE to the deploy `shaders/` folder
via `wi::Archive::GetSourceDirectory()` + `MakePathRelative`) don't resolve to existing files on this
machine → each unresolvable dep is silently SKIPPED (`if (FileExists) { check mtime }` else skip) → no dep
looks newer → `false`. hairparticle hit case (b): its historical meta (from a copied/foreign deploy) had
`../../../../../..` paths that didn't map here, so the genuinely-newer source was never stat'd. (Case (a)
is why the 59 GG*/test/BulletTracer game shaders — MSBuild-FxCompile'd, no meta — never runtime-recompile;
that's correct, they rebuild via the game build.)

**FIX — `refresh_shaders.ps1` in the engine repo root (does NOT touch engine C++ / IsShaderOutdated).**
A plain "is the compiled output older than its source?" check that CANNOT silently fail: for each deploy
`.cso` that HAS a matching `.wishadermeta` (= an engine shader the runtime can recompile; this cleanly
excludes the metaless game shaders, which would VANISH if deleted since the runtime won't rebuild them), it
deletes the `.cso`+meta when the `.cso` is older than its `.hlsl` OR any header that `.hlsl` **transitively
`#include`s** (resolved from disk with a memoized, cycle-guarded parser — real paths, never the fragile
meta). A missing `.cso` → `IsShaderOutdated`'s existence check → clean recompile from CURRENT source next
launch. **Selective** (touch `hairparticle_simulateCS.hlsl` → flags only its 1 cso; touch
`ShaderInterop_HairParticle.h` → flags exactly the 7 hairparticle shaders that include it; touch an
unrelated ocean header → flags only ocean/volumetric shaders, not the other 375) and **converging** (a
freshly recompiled `.cso` is newer than its sources, so never re-flagged; healthy tree = 0 deletions, no
mass recompile). Only follows `#include "..."` (project) not `<...>` (system) headers, and skips headers
outside the shader tree. `-All` forces a full engine-shader recompile; `-DryRun` previews.
**Wired into BOTH builds:** `build_wicked.bat` (after BUILD SUCCEEDED) and the game `build.bat` (guarded by
`if exist`) both call it, so any build — engine (catches ShaderInterop/CB-layout changes, which always come
with an engine rebuild) or game (catches a shader-body edit made without an engine rebuild) — clears the
stale `.cso` and the runtime recompiles it. Manual fallback still works: delete the runtime `.cso`.
**Verify only on a level with grass in view** (TESTPRO1's grass is in the meadow, not the saved alcove;
Island Showdown's opening shot has it but is NOT loadable via the harness — only TESTPRO1 is in the list).

Related: [[project-wicked-engine-changes]], [[project-shadow-system]],
[[project-terrain-blendmaps]], [[project-performance]].
