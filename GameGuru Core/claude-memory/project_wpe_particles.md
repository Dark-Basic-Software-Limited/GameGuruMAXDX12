---
name: project-wpe-particles
description: WPE (.PE) particle system - was dead in DX12 (archive 5076/5077 vs ceiling 93), RE-ENABLED 2026-08-04 via a legacy reader + engine delta 2.00; 2.01 fixed overlapping repeat effects (unserialized emitter fields broke Entity_Duplicate clones)
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-05T02:11:18.504Z
---

**IMPLEMENTED 2026-08-04** (game `4c012b17`, engine `fa852fe1` = delta **2.00**). Was investigated
first (game `d05df91a`); the plan's phases 0a-4 are now shipped and verified live. **Repo `GameGuru Core/PARTICLE_SYSTEM_PLAN.md` is
THE authority**; forensics + a working `.PE` parser live in `GameGuru Core/tools/particle_forensics/`.

**THREE SEPARATE PARTICLE SYSTEMS — do not confuse them:**
- **WPE** = `.PE` files in `Files\particlesbank\wpe` + `gamecore\decals\<n>\wpe.pe`. Wicked
  emitters (`wi::EmittedParticleSystem`). Code: `wickedcalls_part3.cpp:1950-2162` +
  all of `wickedcalls_part4.cpp`. **This is the one under review.**
- **gpup** = `.arx` files. `Guru-WickedMAX\GPUParticles*.cpp` + `GPUP_*.hlsl`. Custom
  render-to-texture, NOT Wicked. Out of scope (user instruction 2026-08-04).
- **ravey_particles** = `GameGuru\Source\M-Particles.cpp`. Legacy DarkSDK. Out of scope.

**ROOT CAUSE (proven 3 ways): version-space collision.** The DX11 fork set
`__archiveVersion = 5077` to gate ~35 GameGuru-private emitter fields at versions 5072–5077.
All 27 shipped `.PE` are 5076 (25) or 5077 (2). DX12 ceiling is **93** → hard reject +
messagebox. Confirmed live: 47 `Skipped non-Wicked archive (version=5076, expected 22-93)`
lines in `Guru-MapEditor.log`. Not a decision — `d3ae5996` (API migration) commented out 16
WPE capabilities to get a clean build; `f189857b` then added a silent skip to stop ~35 modal
boxes.

**`D:\max\WickedRepo` is the DX11-era Wicked fork = the visual gold standard.** READ ONLY,
same rule as `D:\max\GameGuruMAX` (see [[feedback-dx11-reference-readonly]]). Still actively
maintained (last emitter change 2025-08-08) so pin a hash for parity work.

**Format is FROZEN and must be READ, not converted.** `Tools\Particle Editor\particle_editor.exe`
(AppGameKit app, 2022) ships with MAX, still writes 5076/5077, and **its source is nowhere on
disk**. A one-time corpus conversion would orphan every user's effect library.

**Landmines**
- **Do NOT raise `__archiveVersion`** — version gates are compared across ~40 serializers
  engine-wide; v91/92 gates also expect a thumbnail header old files lack.
- ~~If the emitters component-library version must move, **use 4 — upstream already took 3**
  for `burst_on_create`.~~ **DONE 2026-08-05: it is now 4** (delta 2.01). Next move → 5.
- DX11 `FLAG_EMIT_PAUSE` is bit 7; DX12 bit 7 is `FLAG_COLLIDERS_DISABLED`. No shipped effect
  sets it, but user content will.
- **Biggest parity trap: the alpha curve.** `xEmitterOpacity = material.baseColor.w` and
  shipped content sets it up to **20**, so `saturate(linear × 20)` is flat-then-cliff, NOT a
  fade. Reimplementing it as a linear fade looks wrong on most of the corpus.
- Sprite mirroring is dead code in DX11 (`(bool<<31) & 0x10000000` == 0) — do not "fix" it.
- **Neither engine has turbulence/noise** — do not add curl noise, it is not needed for parity.
- `random_position == 0` everywhere → `% 0` divide-by-zero in emitCS is load-bearing.

**`bWPE` boundary:** `bWPE == true` ⟺ a DECAL that found a `wpe.pe`. Entity particle markers
are ALWAYS the `.arx` path. Single fork point `M-Entity_part5.cpp:239` (+ twin at `:159`).
Three entry points bypass it: weapon GUNSPEC `wpeeffect`/`wpeexplosion`, LUA
`WParticleEffect*`, editor preview.

**Phase 0a (NOW APPLIED, game `4c012b17`):** `preload_wicked_particle_effect`
(`M-Entity_part5.cpp:2-140`) returns `true` at `:139` even when no emitter was created, so
`bWPE` stays true with `emitterid == -1` and the legacy `.arx` fallback is skipped too — the
12 stock decals with a `wpe.pe` render **nothing of either kind**. Returning `false` when
`emitterid == -1` restores legacy effects. Shipped.

Verification instruments: `tools/particle_forensics/pe_decode.py` (parses all 27 files; the
numeric gate) and the harness's existing `SCENE_EMITTERS` / `VISIBLE_EMITTERS` in
`GET_PERF_DATA`.

Related: [[project-wicked-engine-changes]], [[project-vram-audit]] (re-enabling loads 27
effects' textures incl. a 5.4 MB flipbook — re-run the sweep).

## STATE AFTER IMPLEMENTATION (2026-08-04 ~17:20)

**It works.** Verified live on TESTPRO1 test game: `SCENE_EMITTERS` **0 -> 95**,
`Skipped non-Wicked archive` log events **47 -> 0**, LUA load+show moved
`VISIBLE_EMITTERS` 0 -> 1, FPS 83 -> 87, frame clean.

What shipped:
- **Legacy .PE reader** in `wickedcalls_part3.cpp` (`WickedCall_LoadLegacyWPE`). Reads the
  frozen 5072-5077 layout directly; the engine's `__archiveVersion` was NOT touched.
  **The hook is in `WickedCall_LoadWiScene`, not `WickedCall_LoadWPE`** - LoadWPE has only 3
  callers, while the decal preload and the LUA loader both call LoadWiScene directly. Cost me
  a build cycle to find; do not "tidy" the hook back into LoadWPE.
- **Engine delta 2.00**: ~35 emitter members, 16 CB fields, emitCS + simulateCS math,
  `IsVisible()`/`IsActive()` cull gate. `FLAG_EMIT_PAUSE` is **bit 10** (bit 7 is upstream's
  COLLIDERS_DISABLED). `fadein_time` defaults **-1** = stock opacity-curve path, so non-WPE
  emitters are untouched.
- **Phase 0a** + game-layer restore (actions 5-8, follow-camera/find-floor, LUA
  `WParticleEffectVisible`, the `a > size()` loop bug).

**NOT yet done - phase 5, needs the user:**
- Visual parity has NOT been judged. No DX11 reference captures exist (that was plan phase 0b
  and I skipped it to get the system running). Effects load and render; whether they look
  identical to the DX11 product is unverified.
- No 19-demo VRAM re-sweep. 27 effects now load textures incl. a 5.4 MB flipbook, and the
  4 GB gate was only just met - **this could regress it**.
- No FPS sweep. `downpour`/`heavy-rain3` request 25,000 particles each.

Verification instruments: `tools/particle_forensics/pe_decode.py` (parses all 27 files) and
`pe_seqcheck.py` (mirrors the C++ sequential walk; confirms it lands on the emitter block at
exactly the offset an independent signature scan finds, 14/14).

## BUG FIX 2026-08-04 ~18:35 - "preview shows nothing" (game `3969159a`, engine `8b5b236a`)

User repro: Trigger Zone + WPE Area script, Preview ticked. DX11 showed the effect, DX12
showed nothing. **Root cause was NOT loading or rendering - both worked. It was the transform.**

**LANDMINE: `Scene::Component_Attach(child, parent)` defaults to
`child_already_in_local_space = false`**, which multiplies the child's local transform by
`inverse(parent.world)`. DX11 never did this - it deserialised `hierarchy` as raw data
(parentID + layerMask_bind) and left transforms alone.

It cancels out at load, so it looks fine. It does not stay cancelled: **the .PE files carry a
leftover authoring position on the ROOT** - `firearea.pe` and `Steam.pe` at
`(330, 241.55, -18265)`, `downpour.pe` at `(-993, 288, -19312)`. The attach baked a
`(-330, -241.55, +18265)` offset into the emitter. The instant the preview repositioned the
root onto the selected entity, particles rendered **18,265 units away**.
Fix: pass `child_already_in_local_space = true` when attaching archive transforms.

Two more DX11 divergences fixed at the same time (engine 2.00b):
- **`Burst(0)` means "fire burst_amount"**, not "burst zero". The fork has
  `if (num <= 0) num = burst_amount;`. Editor preview, `WParticleEffectAction(1)` and the
  weapon/decal paths all call `Burst(0)`, so on stock upstream every burst-only effect
  (explosions, blood, impacts - `count == 0`) fired nothing.
- **`UpdateCPU` emission is not `emit += count*dt`** - it carries the `spawn_random` gust
  model (randpause/randemit) and fractional burst release (`burst_split`, `burst_delay` in
  MILLISECONDS). Now a verbatim port; reduces to upstream behaviour when both are 0.

**Harness diagnostics added - use these first next time:**
- `DUMP_EMITTERS` - per emitter: vis/act gates, **WORLD position**, live GPU alive count,
  blend mode, basecolor alpha, texture validity. Separates "not loading" from "not
  simulating" from "simulating somewhere you cannot see".
- `WPE_PREVIEW <file>` - replicates the editor Preview checkbox exactly and hands the root to
  `RenderPreviewEmitter`.

Full write-up: `PARTICLE_SYSTEM_PLAN.md` Appendix B.

## ALPHA PARITY FIX 2026-08-04 ~19:45 (engine `d5220b88` = 2.00c, game `f406d016`)

User: "particles look a bit weak, say 50% opacity of the DX11 shot."

**Material alpha was being applied TWICE - the particles were alpha-squared.**
`emitCS:35` bakes it into the particle (`baseColor = GetBaseColor() * location.color`,
packed at `:219`), then `simulateCS` computed `opacity = saturate(lerp(1,0,lifeLerp) *
GetBaseColor().a)` - containing it again - and finished `particleColor.a *= opacity`.
Shipped `baseColor.a` is 0.33, so effects rendered at 0.11: a **3.03x** under-render.

DX11 never multiplies. Its VS **overwrites** the alpha byte
(`emittedparticleVS.hlsl:103`: `Out.color = rgb | (uint(opacity*255) << 24)`), so the fix is
`particleColor.a = opacity;` - assignment, gated on the legacy path. **Exact for every effect
regardless of its authored baseColor.a**; a blanket 2x multiplier would only have been right
for one of them. Verified same-camera A/B: faint wisp -> dense golden cloud.

**Rule of thumb this establishes: when a DX11 WPE value looks "half strength", suspect a
double-apply, not a missing gain.** Three of the four bugs in this port were the same shape -
a value applied twice, or applied where DX11 assigned.

## OVERLAPPING EFFECTS FIX 2026-08-05 (engine `4bb6cc7a` = 2.01, game `f60c6f5b`)

User: "when I shoot a material I see the effect, but no second/third instance until the first
has completely finished. DX11 could overlap them for rapid fire."

**Root cause: `Scene::Entity_Duplicate` copies an entity by SERIALIZING it, and the ~35
GameGuru emitter fields added in 2.00 were never added to `EmittedParticleSystem::Serialize`.**
`preload_wicked_particle_effect` (`M-Entity_part5.cpp:27-136`) builds a 5-slot
`ready_decals[decal_id][]` round-robin clone cache per decal exactly that way — load the `.PE`
once, `Entity_Duplicate` the root four times. So clones 1-4 came back with every field at its
struct default, **`burst_amount = 0`** above all. `Burst(0)` resolves `num = burst_amount`
(the 2.00b change), so **four of every five cache slots emitted nothing while still taking
their turn in the round-robin**. One shot in five showed an effect; two could never overlap.

Fix: serialize the same set the DX11 fork did (`WickedRepo\...\wiEmittedParticle.cpp:953-1003`,
its 5072-5077 bands), minus `wpe_filler_1/2/3` and `spawn_pause`/`spawn_pause_random`.
**Emitters component-library version 2 → 4** (3 skipped, upstream owns it). Per-component
version is stored in the archive, so older scenes read unchanged; the shared archive ceiling
was NOT touched.

**GENERAL RULE THIS ESTABLISHES — applies to every Wicked component, not just emitters:**
**any field added to a component struct MUST also be added to that component's `Serialize`.**
`Entity_Duplicate` is a serialization round-trip, so an unserialized field silently becomes a
default on every clone — no error, no warning, just wrong behaviour in the copies only. Worth
auditing the other GGMAX-extended components for the same shape.

**Harness `WPE_CLONETEST <relative .pe path>` is the regression gate.** Loads an effect,
`Entity_Duplicate`s it four times exactly as the preload does, diffs every field, prints
PASS/FAIL. Editor-only, no test game, ~5 s. A/B proof: guard disabled → master
`burst_amount` 2.93 vs clones 0.000, FAIL 4/4 on impact/blood/explosion; enabled → PASS 0/4.

**`spawn_pause`/`spawn_pause_random` gap ASSESSED 2026-08-05:** parsed and discarded
(`wickedcalls_part3.cpp:2327`), but **corpus-checked — every shipped `.PE` (14 particlesbank +
12 decal) has both at 0**. Zero effect on shipped content; only user-authored pause-gust
effects would notice. Port the two members + UpdateCPU pause phase only if a user reports it.

## FPS QUESTION 2026-08-04 ~20:50 - NOT the particle work (measured)

User saw 40-50 FPS in the editor with a WPE-scripted Trigger Zone present (Preview OFF) and
~90 FPS after deleting it, and asked whether the particle work caused it.

**Measured answer: no.** Restored their pre-deletion level from
`Documents\...\mapbank\_automatedbackups\island_150.fpm` (18:11, zone confirmed present in the
screenshot AND in my own capture), loaded it, and got **91.8 FPS with the trigger zone sitting
on the rock and `SCENE_EMITTERS: 0`**. Same 8982 objects. Their own file was backed up by md5
and restored + verified before and after (`island.fpm: OK`).

Why the particle work cannot be it: with Preview off no .PE is ever loaded, so
`SCENE_EMITTERS == 0`, and every per-frame path added by 2.00 is zero-iteration -
`WickedCall_UpdateEmitters` loops over 0 emitters, the wiRenderer cull gate has 0 emitters to
test, `UpdateCPU` runs 0 times. A 12-agent adversarial workflow hunted four independent
hypotheses and **refuted all 8 verified candidates, 0 survivors**.

Two unrefuted SPECULATIVE leads, both PRE-EXISTING (not from the particle work):
- marker entities may be force-flagged transparent, so a placed zone draws as an alpha-blended
  box in the editor transparent pass every frame (would be overdraw-heavy over dense foliage) -
  though my 92 FPS capture had the diamond covering much of the screen, which argues against it;
- the properties panel being OPEN on the entity (measured by an agent at only ~10-40 us/frame,
  refuted as sufficient).

**Remaining difference is almost certainly camera/view** - the user said they "moved the camera
for a better view", and their slow shots have a close character + dense flowers.
NEXT DIAGNOSTIC IF IT RECURS: show the profiler overlay and read `Logic - common_loop` (the same
instrument that solved the corpse-ray bug). Fat common_loop = CPU/game logic; flat = GPU/view.
