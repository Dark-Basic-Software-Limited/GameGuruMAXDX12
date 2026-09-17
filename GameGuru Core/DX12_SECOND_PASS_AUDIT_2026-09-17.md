# DX12 Port — Second-Pass Structural Audit

## Bottom line

**The port is essentially complete.** After a full per-commit walk of all 338 DX11 commits (pass 1, fixes shipped) and this structural pass — which ignored commits entirely and instead hunted for the port's characteristic defect shape, *one half of a feature landing and the other not* — the result is **12 distinct surviving gaps**, heavily overlapping in root cause.

Collapsed to root causes, that is really **six things**:

| # | Root cause | Real user impact |
|---|---|---|
| 1 | `MoveInventoryItem` never sets `collected` / never refreshes key state | **Progression-blocking** in levels using chest keys |
| 2 | `bit32` missing from the Lua 5.4 compat shim | **Latent app-parking hang**, masked only by an untracked hand edit |
| 3 | The whole custom-shader parameter pipeline (`customShaderParam1-7` → `userdata`) was started and abandoned | Authoring-only, but **destroys authored data** |
| 4 | Three custom shaders compile against the *engine* `objectHF.hlsli`, so their GG bodies are not in the build | Three dropdown entries silently do nothing |
| 5 | Four visual features whose engine API no longer exists (PP Snow, transparent shadows, bloom strength, gamma fade) | Cosmetic; three need an engine port, one does not |
| 6 | Diagnostics severed at both ends (`enablepixmarkers`, GFX debug log, `_ConvertFormat`) | Zero player impact |

**Only two of these can hurt a player**: #1 and #2. Everything else is authoring-surface, cosmetic, or developer-facing. That is a good result for a port of this size, and I am not going to dress it up as anything worse.

The most important structural observation is not any individual finding. It is that **three of the six were incomplete applications of fixes this port had already decided to make** — 3.45's chest-key port landed 3 of 4 hunks; 3.40's Lua compat shim covered 5 of 6 removed names; the `userdata` migration wrote the TODO comments and never wrote the code. The port's failure mode is no longer "we forgot DX11 had this." It is "we started it and stopped one step short."

---

## Act on these

### 1. `MoveInventoryItem` — chest/shop keys never flag as collected

**Confidence: CERTAIN. Impact: HIGH. Fix: ~7 lines, no engine change.**

`DarkLUA_part4.cpp` around **:1022-1031** (three sweeps gave slightly different anchors — 1022-1027, 1024-1030, 1024-1031; re-read the `if (item.e > 0)` block before patching) kept only DX11's `active = 1 / active = 0` and dropped five statements:

- `collected = 1` when destination container is 0 (player)
- `collected = 2` when destination is 1 (hotkeys)
- `collected = 0` when moving out to shop/chest
- `darklua_refreshhaskeystatefor(name)` on **both** branches

DX11 source: `DarkLUA.cpp:8887-8902`, added by **51197b43** ("Keys correctly flagged inside Chests", 22-Dec-2025), live at DX11 HEAD.

**This is an incomplete port of a port.** DX12's 3.45 explicitly set out to port 51197b43 — the note at `DarkLUA_part4.cpp:825-829` quotes the symptom. That commit touches `DarkLUA.cpp` in four hunks; DX12 landed three. The hunk that actually sets the flag the fix was written for never landed.

Proof by call-site census: `darklua_refreshhaskeystatefor` has **5 call sites in DX11** (`DarkLUA.cpp:1445, :1471, :8894, :8901`, `M-LUA-Entity.cpp:1196`) and **3 in DX12** (`DarkLUA_part0.cpp:1612, :1643`, `M-LUA-Entity_part0.cpp:1200`). The two absent are exactly the transfer pair. Tree-wide, DX12 contains **no literal `collected = 1` or `collected = 2` anywhere at all**.

The reader is live and correct: `M-LUA.cpp:1262` and `:1289` accept `collected == 1 || collected == 2`. Items sitting in a container carry `collected = 3` (`hud0.lua:333`), which never satisfies it.

**Repro:** put a key in a chest, assign it as a door's USE KEY, Test Game, open the chest, press TAKE ALL (`hud0.lua:1257` → `MoveInventoryItem(...)` with no following `SetEntityCollected`). Key is in inventory; door stays locked. Dropping it back never re-locks either, because `lua.haskey` is a sticky cache only re-evaluated while it reads 0 (`M-LUA.cpp:1241`) and the missing refresh call is what resets it.

**Blast radius — one honest disagreement between sweeps.** One sweep narrowed this: `hud0.lua:659/667` (the hotkey drag path) calls `SetEntityCollected` *after* the move, which reaches `entity_lua_collected` and does refresh — so that path is Lua-covered and is **not** broken. Another sweep listed the hotkey moves as broken. Uncontested and definitely broken: **`hud0.lua:653`** (plain container↔player drag) and **`hud0.lua:1257`** (transfer-all). Ground pickup still works (`DarkLUA_part0.cpp:1641`), which is exactly why this hides — it presents as "keys work when picked up, not when taken from a container."

Secondary damage: `M-LUA-Entity_part0.cpp:1055` treats `collected != 0` as "already collected, leave alone", and `G-Entity_part2.cpp:1602` guards `entity_monitorloot` on `collected == 0` — so an item in the player's inventory is still tracked as a world item.

### 2. `bit32` missing from `GGLua_InstallCompatShim`

**Confidence: CERTAIN. Impact: HIGH (latent). Fix: one clause in an existing function.**

DX11 ships its own Lua **5.2** with `lbitlib.c` registered, so `bit32` is a global in every script. DX12 has **no vendored Lua at all** under GameGuru Core and links WickedEngine's Lua **5.4.8**, where `bit32` was removed.

3.40 recognised the hazard and added `GGLua_InstallCompatShim` (`DarkLUA_part7.cpp:23`, correctly invoked at `:1521` and `:1616` before any script runs) — but it aliases only `math.atan2`, `math.mod`, `math.pow`, `math.log10`, `unpack`. Its own comment asserts *"math.atan2 is the only removed name any current script uses."* That is the wrong half of the claim.

Grep of all 1211 tracked scripts finds exactly two users, and they are the worst possible two:

- `Scripts/scriptbank/perlin_noise.lua:8` — `local band = bit32.band` **at file scope** (fails at load, not on a rare branch)
- `Scripts/scriptbank/gameplayercontrol.lua` — `require "scriptbank\\perlin_noise"` on line 2, plus 12 `bit32.band` calls

`gameplayercontrol.lua` is added to **every level's** collection by the engine (`M-MapFile_part1.cpp:445`). Both repo copies are byte-identical to DX11's.

**Why nobody has hit it:** an **untracked hand edit** in the deploy tree injects `if not bit32 then bit32 = {band=...,bor=...,bxor=...,lshift=...,rshift=...} end` into both deployed files. `git log` on the repo files shows no such commit and `build.bat` has no script-copy step — **nothing in the tracked build reproduces it.** Any redeploy from the repo, a fresh build area, or a checkout on the laptop loses the patch.

**Failure mode when it does fire:** per this port's own 3.40 note (`DarkLUA_part7.cpp:1-8`), an "attempt to call a nil value" raises a **modal MessageBox that parks the application at zero CPU**.

> ⚠ That is the same signature as the recorded Test Level freeze. I want to be careful here: the memory notes record that freeze on the **pre-port 08-29 alpha too**, which argues against `bit32` being the cause, and the deploy tree currently carries the masking patch so it could not be firing there today. **Treat this as a lead worth ten minutes, not a diagnosis.** The cheap check is to launch with `producelogfiles` on and look for a Lua load error before Test Level, and separately to ask Lee whether a *human* clicking Test Level sees the freeze.

Also noted in passing: the deployed `gameplayercontrol.lua` is **eleven globals behind** the repo copy (`g_gameplayercontrol_rotate45` onward). That deploy tree is stale in general and is not a safe fallback for anything.

Adding a `bit32` clause to the shim closes the class permanently and lets both hand-patched files revert to match DX11.

---

## The Custom Shaders cluster

Seven candidates across five sweeps all describe one abandoned migration. Treating them as one item, because fixing any one in isolation leaves the feature dead.

**Reachability caveat up front, so this is not over-prioritised:** **zero shipped .fpe files use `customshaderid` or `customshaderparam`** (4248 scanned). The only way a user reaches any of this is the Model Importer's Custom Shaders dropdown, which is live and populated. So this is an *authoring-surface* defect, not a player-facing one — with two exceptions noted below.

### 3a. The parameter pipeline: `customShaderParam1-7` → `userdata` was never finished

**CERTAIN. Impact: HIGH for authoring, and it silently destroys data.**

The engine replaced seven floats with `uint4 userdata` (`wiScene_Components.h:270`), and does plumb it to the GPU (`wiScene_Components.cpp:426` → `ShaderInterop_Renderer.h:434`). **Nothing in DX12 game code ever writes it.** Grep over `Guru-WickedMAX` + `GameGuru/Source` returns only TODO comments and commented env-probe fossils.

What landed vs what did not:

- ✅ **Landed, verified correct:** .fpe parse (`M-Entity_part1.cpp:685-724`), defaults (`:205-211`), .ele write (`M-Entity_part4.cpp:555-561`), .ele read (`M-Entity_part3.cpp:723-729`), all seven accessors with profile fallback (`M-Entity_part5.cpp:1091-1147`), `customShaderID` delivery itself, the combo box, the caption tables, the shaderID==5 veto, the nine-arg overload.
- ❌ **Severed:** all three apply sites (`wickedcalls_part1.cpp:496-512`, `:584-600`, `:672-688`) — the seven getters are still *called* into locals `iCustomShaderParam1..7` that are then never read (21 orphan locals total, the classic residue that makes a file look finished).
- ❌ **Severed:** `WickedCall_SetShaderParameter` (`wickedcalls_part4.cpp:326-360`) — body commented from `:348`, but the **caller was left in**. `M-LUA.cpp:1181` is byte-identical to DX11's `M-LUA.cpp:903` and runs every logic tick: `WickedCall_SetShaderParameter(t.tobj, 1, 1.0f - health)`. It walks the meshes, sets `bChanged`, writes nothing.
- ❌ **Gone:** all seven Importer sliders (`M-Importer_part6.cpp:1364-1452`, banner: *"Slider UI for custom shader params disabled until userdata migration"*), and the per-shader defaults that fire on combo selection. Both live calls pass `0.0f × 7` (`:1249`, `:1295`), and the callee discards them again (`M-Importer_part7.cpp:503-510`).
- ☠️ **Actively harmful:** `M-Importer_part7.cpp:69-76` stores **literal `0.0f`** into `edit_grideleprof->WEMaterial.customShaderParam1-7` where DX11 reads them back off the material. That function has **15 live call sites** (entity selection at `M-GridEdit_part1.cpp:6308/6315/6373`, runtime refresh at `G-Entity_part2.cpp:1705/1747`, `M-GridEditB_part9.cpp:401/888`, `M-GridEditB_part23.cpp:897`, and others). Selecting or refreshing an entity **zeroes its authored params in the profile**, and the still-intact serialiser then persists the zeros. Authored values are destroyed, not merely ignored.

Live GPU consequences, precisely: the only two shaders whose body is in their own entry file are `objectPS_grid.hlsl` and `damageBloodPS.hlsl`. Both read `asfloat(GetMaterial().userdata.*)` = `0.0f`. The grid object draws no lines (zero thickness, zero fade, zero min alpha). Blood damage draws zero coverage, and `splatterScale = 0` additionally **divides by zero** at `damageBloodPS.hlsl:187`. `maxBlood`/`brightness` are hardcoded to 1.0 with an in-source admission that C++ packing is missing.

One sweep flagged the grid shader as inert in *both* trees (its `bEmptyLevelGrid` overlay caller is dormant in DX11 too), so the genuinely live consequence is the blood shader alone.

**One claim from an earlier sweep is fabricated and should not be carried forward:** the "sheen/clearcoat/transmission/thickness lost on glTF import via KHR extensions" story. `grep -n KHR` over DX11's `M-Importer.cpp` returns **nothing**. The default blocks are keyed to the combo *selection index* (i==1 tree wind, i==3 glass, i==4 grid, i==5 blood). Drop that claim.

### 3b. Water / Glass / Tree-Animate compile to the stock engine shader

**CERTAIN, and proven from the shipped build rather than by inference. Impact: MED.**

Three of the five custom shaders are pure stubs — `objectVS_common_tree.hlsl` is four lines: a compile macro, a layout macro, `#define TREEANIMNATE`, `#include "objectHF.hlsli"`. The only `#ifdef TREEANIMNATE` in existence lives in the **GG-local** `GGTerrain/CustomShaders/objectHF.hlsli`. `CustomShaders.cpp:62-63` pushes `engineShaderDir` onto `include_directories` **before** `customDir`, and `wiShaderCompiler.cpp` hands DXC a `DxcBuffer` with no source-directory context — so the quoted include binds to the **engine** header and every define is inert.

Empirical proof from `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\shaders`:

- `objectPS_custom_water.cso` **==** `objectPS_transparent_glass.cso` (md5 `e63f489723b1696e46d885a488963b94`) — two shaders that should differ completely are the same binary
- `objectVS_prepass_trees.cso` **==** `shadowVS_alphatest_tree.cso` (`29a503392ecda3f2d89bbf00e8b5ed1a`)
- `objectVS_common_tree.cso` **==** `damageBloodVS.cso` **==** the engine's own `objectVS_common.cso` (`4de1a058a821f7511badf8891ef993b1`)
- `objectPS_custom_water.wishadermeta` lists 25 dependencies, **every one** under `WickedEngineDX12\WickedEngine\shaders`, plus only the entry stub. Not one GG CustomShaders header appears.

DX11 never has this problem because it never compiles at runtime — `CustomShaders.cpp` there `LoadShader()`s prebuilt `.cso` produced offline by `<FxCompile>` (`Template_Windows.vcxproj:849-1073`), where the local directory wins. That is precisely why the local header set exists in that folder.

Compounding: the engine renamed the layout macros to `OBJECTSHADER_LAYOUT_SHADOW_TEX` / `_PREPASS_TEX` (`objectHF.hlsli:138,153`) while the stubs still pass DX11's `_POS_TEX` / `_POS_PREVPOS_TEX`, which now match nothing.

**Unverified risk worth one eyeball:** each of these PSOs is registered with only one stage assigned (`descwater.ps` only, `descglass.ps` only). Whether the draw even survives PSO creation under DX12 was not checked at runtime.

Note this is **not** the GGTrees pipeline, which is separate and unaffected — almost certainly why nobody has spotted it.

The earlier hypothesis that the local headers *shadow* the engine's and cause silent CB offset mismatch is **refuted** by the dependency manifests. The truth is the opposite and worse: the entire GG-local header set (`objectHF.hlsli`, `globals.hlsli`, `brdf.hlsli`, `lightingHF.hlsli`, `ShaderInterop_Renderer.h`, all dated Feb 2026) is **dead code that nothing includes**.

**Order of operations matters here.** Fixing the `userdata` packing without also fixing the include order buys you nothing for water/glass/tree — their bodies are not in the build. Fixing the include order without the packing gives you shaders that run with all knobs at zero. And restoring the sliders without restoring `WickedCall_SetShaderParameter`'s body leaves damage-driven blood static. This is one job with four parts or it is not worth starting.

---

## Cosmetic / visual gaps

### 4. Level-load gamma fade-in is dead — **MED, and this one is cheap**

The ramp is fully intact and drives nothing. `visuals_newlevel` sets `g_fGlobalGammaFadeIn = -2` (`M-Visuals_part0.cpp:599`, called from LOADMAP at `M-GridEdit_part7.cpp:809-810` and the two new-level paths), `Wicked_Update_Visuals` sets the destination (`M-GridEditB_part3.cpp:2042`), `lighting_loop` still runs the `+= 0.05f` ramp every frame — and both consumer lines in `G-Lighting.cpp:341-346` are a bare `;` with a TODO.

Verified the DX11 value is real, not cosmetic: `GAMMA(x) = pow(abs(x), 1.0/g_xFrame_Gamma)` (`globals.hlsli:60`) applied in `tonemapCS.hlsl:257` — `SetGamma(0)` → exponent +INF → black, and `-2 → 2.2` is ~84 frames of fade from black.

DX12 already has the replacement lever in hand: `master_renderer->setBrightness`, already used by the gamma slider at `M-GridEditB_part23.cpp:1866`. **No engine port needed.**

Important: DX12's *other* fade (`t.postprocessings.fadeinvalue_f`, an ImGui black rect) is gated on `t.game.gameisexe == 1 || bFakeStandaloneTest` (`M-Game_part1.cpp:1639`), so it does **not** cover the editor or Test Game. Nothing replaces the gamma fade there — which also means level-load construction artefacts DX11 hid are now visible.

### 5. Environment-probe release fade pops — **MED, no engine change needed**

The interesting half of this is **not** the shader alpha (whose only consumer lives in the dead CustomShaders `lightingHF.hlsli` — see 3b). It is that in commenting out the ramp, DX12 left `g_bEnvProbeTrackingUpdate[iRealProbeIndex] = false;` firing **unconditionally** on both the acquire branch (`GGTerrain_part0.cpp:9526`) and the release branch (`:9571`).

In DX11 that flag stays TRUE until the 0..255 ramp completes, which is what keeps the multi-frame `probe->range -= (fMovementDelta * 20.0f)` fade-down running before the slot is parked. In DX12 the range decrements once and the probe is parked on the very next statement (`:9573`) — so the release fade is dead even though the decrement line is still live. Probes pop instead of fading as the player leaves an indoor volume.

### 6. Four features whose engine API no longer exists — **MED to LOW, all need engine work**

| Feature | Status |
|---|---|
| **PP Snow / Dust** (`bPPSnow`, `bpp_disable_indoor`) | Genuinely absent at every layer of the DX12 engine (`grep -rni snow` returns only `DirectXColors.h`, `STENCILREF_SNOW`, a terrain material name). All four call sites stubbed; both checkboxes commented. DX11 side confirmed real: `SetPPSnowEnabled` → `PP_SNOW` → `wiRenderer.cpp:4802` `OPTION_BIT_SNOW_ENABLED` → `tonemapCS.hlsl:213`. **Residue:** `bPPSnow` still gates an every-15th-frame indoor `IntersectAllEx` raycast at `M-Game_part3.cpp:331-345` whose result is discarded with `(void)iHitObj;` — that raycast should go regardless of whether the feature returns. |
| **`pp_alpha` / `voxel_steps`** | WeatherComponent members gone from the engine. Only meaningful alongside PP Snow. Note the neighbours in the same DX11 block (windDirection, windSpeed, windWaveSize, windRandomness, tree_wind, tree_sss) are **all still live** — only these two dropped. |
| **`bTransparentShadows`** | `wiRenderer::SetTransparentShadowsEnabled` absent. Already logged in the 2026-07-28 UI audit. **Not a pure orphan:** `M-GridEditB_part3.cpp:1332-1335` still runs the change-detect every frame and `bTransparentChanged` still feeds the shadow-refresh test below it — so toggling the now-hidden value forces a shadow rebuild that changes nothing. Trees unaffected (synced separately in `Wicked_Update_Shadows`). |
| **`fsetBloomStrength`** | `wiRenderPath3D.h` has `bloomThreshold`/`bloomEnabled` with getters and setters, but no `bloomStrength`. Already in the UI audit. Easy to miss precisely because it sits between two surviving siblings — Enabled and Threshold both still work. |

All four keep their fields defaulted, saved, parsed and copied, so **level-file compatibility is intact** and a DX11-authored level round-trips its values untouched. That was a deliberate choice and it was the right one.

### 7. Procedural-preview fog — **LOW, LIKELY (not certain)**

DX11 forces `weather->fogColorAndOpacity.w = 0.0` in the terrain snapshot camera when `skyindex==0` and the skybox is not disabled, deliberately without touching `t.visuals.FogA_f` (`M-TerrainNew.cpp:8810`). DX12's branch is a bare `;` (`M-TerrainNew_part5.cpp:888`). Replacement field exists (`weather->gg_fog_opacity`, `M-GridEditB_part3.cpp:1456`), so it is a one-liner.

**Why LIKELY and not CERTAIN:** DX12's sibling else-branch now also only writes `t.visuals.FogA_f` and relies on the same `bVisualUpdated`-gated refresh, so the preview's fog ramp may be inert in *both* branches rather than only one. Verify before patching.

---

## Diagnostics only — zero player impact

### 8. `enablepixmarkers` + GFX debug log in crash reports — **LOW**

`g_iEnablePIXMarkers` is defined (`M-GridEdit_part0.cpp:341`), parsed in two places (`Common_part1.cpp:1206`, `Common_part3.cpp:1081`), and read **nowhere**. The shipped `setup.ini` in the build area carries `enablepixmarkers=1` (line 78) against a key nothing reads.

Alongside it, `CrashLogger.cpp` keeps the orphan `static char g_pDebugExtraInfo[10240]` (`:21`) and `GetCrashHandlerDebugLogRef()` (`:393-396`) with zero callers, not declared in the header, and the report body (`:329-340`) prints only `g_CrashContext`.

**Two sweeps disagreed on scope; the narrower one is right.** DX11 has two readers and only one is portable:
- `CrashLogger.cpp:405-409` stamps "PIX Markers Enabled!" into crashlog.txt — pure GameGuru code, no engine dependency, **~4 lines to restore**.
- `main.cpp:218/222` passes the flag to `wiRenderer::GetDevice()->SetSpecialGGDebugLog` — **not portable**. That API does not exist anywhere in `WickedEngineDX12`, and in the DX11 engine it gates only the `PIXBeginEvent`/`PIXEndEvent` pair in PresentBegin/PresentEnd — it is not a general marker switch even there.

Restoring only the print would emit an empty "GFX Debug Log:" header, since nothing can fill the buffer. **Marker emission itself is not a regression:** `GraphicsDevice_DX12::EventBegin` emits PIX events unconditionally, a superset of DX11 — you only lose the ability to turn them off. Already logged as knowingly deferred at `DX11_PARITY_DEEP_AUDIT_2026-09-16.md:83`.

### 9. `_ConvertFormat` — ported, correct, and bypassed by a numerically wrong cast — **LOW**

DX12 ported `_ConvertFormat` and correctly re-typed it to `wiGraphics::Format` (`wickedcalls_part3.cpp:1625`). Its only caller then dropped it for `static_cast<DXGI_FORMAT>(Format)` under the comment *"values match DXGI_FORMAT"* (`:1877-1878`).

**They do not.** `wiGraphics.h:218` is `enum class Format : uint8_t { UNKNOWN, R32G32B32A32_FLOAT, ... }` — compact and sequential with no TYPELESS entries — whereas DXGI_FORMAT interleaves them: `R32G32B32A32_FLOAT` is 1 vs 2, `R8G8B8A8_UNORM` 24 vs 28, `BC1_UNORM` ~60 vs 71. The mis-cast value feeds `getBitsPerPixel(int)`/`getImageformat(int)`, which switch on real DXGI constants.

Reachable only via `setup.ini memgeneratedump=1` → `DumpImageList` → `Wicked_Memory_Use_Textures`. **BC1 falls through to the default 32 bpp, so a BC1 atlas is reported ~4× too large** — relevant because the VRAM campaign reads that log. Same function has a second independent break: `MaterialGetSRV` is stubbed so `lpTexture` is always nullptr, and the `already_registred` de-dup then suppresses every texture after the first. Fix both or neither.

### 10. GPU particle interleave — **LOW, already a documented decision. Do not act on this.**

DX11 keeps a per-command-list cursor (`uint32_t g_emitterCurrentIndex[256]`) so GPU particles emit *between* transparent meshes. DX12 reduced it to a scalar set to 0 and never advanced, and `gpup_draw_bydistance` early-returns. Particles sort as one batch per frame, so an effect can sort wrongly against a window or glass mesh.

The 20-line comment at `GPUParticles_part0.cpp:762-771` states plainly that restoring it needs an engine callback inside the DX12 `RenderMeshes` transparent loop — an engine delta that was deliberately not taken. Listed for completeness only. **Do not attempt a game-side fix.**

---

## Swept and found CLEAN

A trustworthy negative is a result. These were examined in this pass, some adversarially, and are **fine** — you can stop worrying about them.

**Rejected candidates (facts checked out, conclusion did not):**

- **Ragdoll bone `WickedCall_PresetObjectCreateOnDemand`** — the two commented calls bracket a block that creates no Wicked object. Both trees are inside `if (g_bDebugRagdoll == false)` and the first statement after is `dbproRagDollBoneID = 0; // not using debug capsule`. Between the pairs: `GetAngleFromPoint()` and a `btMatrix3x3 setEulerYPR`. Pure maths. Presets are sticky and apply to the next object created; there is no next object.
- **`RAY::bIgnoreNearestTriangle`** — the field really is absent from the DX12 engine, but DX11's semantics are not what they look like: `wiScene.cpp:4780-4790` sets `localResult` from whatever triangle hit *first* then sets `bEarlyExit`, returning a position/normal/subset that is **not necessarily the nearest**. The callers are the editor mouse pick and consume position/normal/entity. Not a parity loss worth chasing.
- **`cloudScale`** — rejected.
- **`GGTerrain_GetTriangleListHighQuality`** — genuinely absent from DX12, and **that absence is correct**. `22703eef` (HEAD-1) is an explicit bisected revert: 3.46+navmesh_hq = 0/6, 3.47 minus navmesh_hq = 6/6, cause a >60 s main-thread stall entering Test Game on large maps under DX12's native SVT terrain. Reporting it would have someone re-port code that was deliberately removed 24 hours ago.
- **Deployed `door.lua` v31 vs repo v33** — true, but not a port gap. Full diff of the two repo copies shows only DX12's deliberate 3.21 divergence (conditional `SetEntityAlwaysActive` gated on `use_switch`, with its rationale comment). DX12 is missing nothing. *(It does confirm the deploy tree is stale, which matters for finding #2.)*
- **Stale CustomShaders headers causing CB offset mismatch** — refuted decisively by the compiler's own dependency manifests. The premise was backwards: those headers are not compiled against at all. That refutation is what surfaced finding 3b.

**Verified present and correct in DX12:**

- The `darklua_refreshhaskeystatefor` **body itself** — full 73 lines at `DarkLUA_part4.cpp:830`, matching DX11 function-for-function including the multi-key `;` parsing and the `stricmp(...) == NULL` guard.
- Three of its four call sites: both `SetEntityCollectedEx` sites and the `entity_lua_collected` site, including the local `extern void` declaration.
- The `M-LUA.cpp` half of DX11 51197b43 — both SINGLE and MULTIPLE key branches widened correctly (`:1262`, `:1289`).
- The Lua compat shim's **installation** — both call sites (`DarkLUA_part7.cpp:1521`, `:1616`) are real and precede script execution, and the five existing aliases are exact. The 3.40 fix is sound; only the `bit32` clause is absent.
- The **entire** custom-shader persistence layer: .fpe parse, defaults, .ele read *and* write, all seven accessors with element-vs-profile fallback, `customShaderID` delivery and readback, the `#ifdef CUSTOMSHADERS` gate.
- Custom Shaders **UI scaffolding**: the combo box populated from `wiRenderer::GetCustomShaders`, the per-shader caption tables for IDs 1-5, the shaderID==5 double-sided/"eyeglasses" veto, the `if (i == 4) continue` skip, the nine-argument object-targeted `importer_set_all_material_shader_id` overload and its caller.
- **Crash-log breadcrumbs** — `GG_CRASH_CONTEXT` gated by `g_iDisableCrashLogSystem`, ring buffer printed under `---- BREADCRUMBS ----`. The 3.44 fix landed. `disablecrashlogsystem` is parsed **and** consumed.
- **Device-removal post-mortem is better than DX11's**, not missing: DRED with AutoBreadcrumbs, PageFault and BreadcrumbContext forced on, written to `dred_report.txt`, plus a minidump. DX11 has no DRED at all.
- `realsha...`, `UpdateEntity`, and the weather siblings `windDirection` / `windSpeed` / `windWaveSize` / `windRandomness` / `tree_wind` / `tree_sss` — all live.
- `bloomEnabled` and `bloomThreshold` — both wired; only `bloomStrength` is gone.
- The **GGTrees pipeline** — separate from the custom-shader tree stub and entirely unaffected.

---

## What this method still cannot see

Be sceptical of the shape of this result, not just its contents. The sweep hunts *asymmetry between two trees*, which means it is blind to whole categories:

1. **Anything DX11 also lacks.** If a feature was already broken in DX11, or if both trees carry the same bug, nothing here fires. This is a parity audit, not a correctness audit.
2. **Anything DX12 rewrote wholesale.** Native SVT terrain has no DX11 counterpart to diff against. The same goes for any subsystem where the port took a different architecture rather than porting line-for-line — those are exactly the places where the interesting bugs live and this method says nothing.
3. **Numeric and behavioural divergence where both sides have code.** Different defaults, different ordering, different precision. The fp16-overflow-at-1.66 km class of defect (which has bitten this project **three times**) is completely invisible to a structural sweep — both trees have the line, it just means something different.
4. **Everything runtime.** Races, device loss, ordering, performance. None of this was executed. Every finding above is static evidence from source, plus build-output evidence (the `.cso` md5 collisions and `.wishadermeta` dependency lists) for 3b. **Nothing was verified by launching the app.**
5. **Clean removals.** The sweep leans on residue — commented-out lines, orphan locals, TODO markers, fields written but never read. A feature deleted tidily with no comment and no leftover variable leaves nothing to grep for and will not appear here. There is no way to estimate how much falls into this category.
6. **Repo vs deploy-tree drift.** Finding #2 caught an untracked hand edit in the build area **by accident**, while chasing something else. There was no systematic diff of the deploy tree against the repo, and the stale `gameplayercontrol.lua` and `door.lua` suggest more drift exists. That is a separate sweep worth running.
7. **Assets and content.** Shader source beyond the CustomShaders folder, and the ~1200 shipped Lua scripts beyond the three candidates specifically checked, were not systematically compared.
8. **Adversarial refutation trades false positives for false negatives.** This pass rejected 6+ candidates. That discipline is why the surviving list is trustworthy — but the same conservatism will have discarded real defects whose evidence was merely thin.

---

## One caution before acting on any of this

**A byte-faithful port of a DX11 function can still be wrong for DX12, because the engine underneath is different.**

You have direct evidence from yesterday. `GGTerrain_GetTriangleListHighQuality` was ported into 3.47 diff-clean against DX11 — identical code, correct by every static measure this audit uses — and it **blocked the main thread for 60+ seconds** entering Test Game on a large map under DX12's native SVT terrain, and had to be reverted (`22703eef`). Bisected: 3.46+navmesh_hq = 0/6, 3.47 minus navmesh_hq = 6/6. The commit message records that the port was byte-identical and still wrong.

Every recommendation in this report is "DX11 does X, DX12 does not." **That is an argument that a gap exists, never an argument that DX11's implementation is the right fix for DX12.** Applied to the findings above:

- **Findings 1 and 2** are the safe ones. Both are pure game-side logic with no engine coupling — five statements setting an int field and calling an existing function; five entries in an existing table. Neither touches the renderer, the scene graph, or terrain. These are as close to risk-free as this codebase gets.
- **Finding 3b carries real unknown risk.** Changing include order in `CustomShaders.cpp` will recompile three shaders against a header set last touched in Feb 2026, against an engine that has since renamed the layout macros the stubs pass. It may not compile; it may compile and produce garbage; the single-stage PSO registration may fail outright. Do it behind a build you can revert, and check the `.cso` md5s afterwards to prove the bodies actually landed — that is the same instrument that found the bug.
- **Findings 4 and 5** use replacement APIs that already exist and are already used elsewhere in DX12 (`setBrightness`, `gg_fog_opacity`). Low risk, but verify the fog one first — both branches may be inert.
- **Finding 6** needs engine work. Given ~40 s engine builds, that is not the barrier; the barrier is that porting `PP_SNOW` means reasoning about the DX12 tonemap path, not transcribing DX11's.

**Recommended order:** #1, then #2 (both small, both safe, both the only player-facing items), then #4 (cheap, visible, replacement lever already in hand), then decide whether the custom-shader cluster is worth a day given that no shipped content reaches it — with the caveat that its data-destroying readback at `M-Importer_part7.cpp:69-76` is worth neutralising on its own even if the rest is deferred.