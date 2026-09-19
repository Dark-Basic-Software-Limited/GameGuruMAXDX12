---
name: project-weather-wpe-swap
description: "Rain/snow run on the legacy ravey particle system; the WPE .pe path that will replace them already has camera-follow and is correct — the blocker is a texture-name mismatch inside Lee's new .pe files."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-19T16:34:11.306Z
---

**DONE 3.59, 2026-09-19 (game `06ca6404`).** Rain/snow now load `Light Rain.pe` / `Light Snow.pe`.
★ THREE defects, in the order they bit — all three are the general lesson:
 1. assets mis-authored (" MAX" missing from the PNG names) — **renamed the four PNGs**;
 2. ⚠ the load path must NOT carry a `Files\` prefix — emitter names are relative to `Files/`
    (cf. `"particlesbank/default"`), and `LoadWPE` returned 0 **silently** because
    `producelogfiles=0` means there is no log to read;
 3. ⚠⚠ the ravey **self-repair** in `update_env_particles`
    (`if (ObjectExist(raveyoffset+1)==0) { ravey_particles_init(); reset_env_particles(); }`)
    destroyed the effect four lines after the dispatcher created it, **every frame**, because
    `reset_env_particles` is exactly where the WPE teardown belongs. Now gated on a ravey
    emitter actually existing.
★ Diagnose with `GET_WEATHER` — it reports `latch / updateCalls / setCalls / lastResult /
lastPath`, which located all three in three runs. Also `SET_WEATHER <0-5>`,
`SET_WEATHERINTENSITY <0-100>`.
⚠ Editor only runs weather when **"Display Weather in Editor"** is ticked (`bEnableWeather`,
default false); Test Game is ungated.
⚠ Intensity scales `ec->count` from a base captured at load — never `SetMaxParticleCount`.
⚠ Indoor cutout (#5917) kept, as **emit-pause** not count-zero.

★★★ **3.60 — .PE VERSION IS NOW A SANITY BOUND, NOT A CEILING** (`e64b6e0f`). The reader rejected
`ver > 5077`, so the next particle-editor bump would have killed rain and snow.
⚠ **Raising the ceiling alone would be WRONG** — `ReadEmitter` branches on version and appends
fields at 5072/5073/5074/5075/5076/5077, so 5077 rules on a 5078 file under-read each record and
desync the trailing id array: it "loads" and produces garbage. ★ The survey that made the fix
possible: **all six** GameGuru-era bumps only ever appended to the emitter record (materials stop
at v68, resources at v63) — one evolution point, and it is skippable.
⚠⚠ **The assumption I had to throw away:** deriving the record size as
`(bytes between the emitter array and the id array) / emCount` assumes the id array ends the file.
**It does not** — shipped v5077 files have a **48-byte trailer after the id array**
(`emStart=48751 afterRecs=49375 afterIds=49391 size=49439`). Caught only because the diagnostic
printed stride (336) and known (312) side by side and they disagreed.
**The fix**: for an unknown newer version, *solve* for the record size — every emitter has a
TransformComponent, so each trailing id must already be in `trEnt`; step candidates in 4-byte units
and take the first where all ids resolve. Known versions skip the solver and are bit-identical.
Fails CLOSED for changes outside the emitter tail (the sixteen always-zero manager counts trip),
and `gg_wpe_lastError` says which. Verified with a synthetic v5078 (+16 B/record): loads, stride 328.

**Original investigation follows.** Lee: replace the DX11-era rain/snow with the new
Wicked particle effects `Light Rain.pe` / `Light Snow.pe` in
`Max/Files/particlesbank/wpe/Weather Effects/`. Explicitly NOT a visual-parity job.

**Today:** weather is the legacy **ravey** system (fully alive, not stubbed).
`M-Particles.cpp` dispatcher ~:1573-1622 is edge-triggered on
`environment_weather != t.visuals.iEnvironmentWeather`. Modes: 0 none, 1 light rain, 2 heavy rain,
3 light snow(+test), 4 heavy snow, 5 test. Particles are DBP quad objects (id 180001+) that are
**never deleted, only hidden**, with a full CPU vertex lock + material re-texture **per raindrop**.
Camera-follow is three explicit `xPos/yPos/zPos = CameraPosition*()` writes at :1645/:1675/:1691 —
NOT `parentObject` (the `//Always follow camera` comment sits on a line that means "no parent").
⚠ **Keep the indoor cutout** (:1651-1667, every 15th frame, upward `IntersectAllEx` → emitters off;
GameGuruRepo #5917). Editor path is gated on `bEnableWeather`; the in-game path is not.
⚠ The engine-side `setRainTextures` (`master_part1.cpp:462`) is **dead** — `POSTPROCESSRAIN` is
commented out. Unrelated to any of this despite the name.

★★★ **The WPE delivery mechanism is already complete and correct** — this is the good news.
`WickedCall_LoadWPE` (`wickedcalls_part3.cpp:2813`) reads the legacy .PE archive; **`followCamera`
and `findFloor` are properties stored IN the .pe** (`:2743-2744`), and `wickedcalls_part4.cpp:155+`
already repositions such effects to the player each frame with indoor/outdoor/underwater handling
(restored in GGMAX 2.00). `WickedCall_PerformEmitterAction` (`wickedcalls_part4.cpp:3-69`) walks
every emitter under a root — 4=Restart, 5=Visible, 7/8=pause/resume emit.

⚠ **BLOCKER — Lee's two new .pe files are mis-authored.** Their materials ask for
`Light Rain MAX0/1_color.png` and `Light Snow MAX0/1_color.png`; the shipped PNGs are
`Light Rain0/1_color.png` etc. — the literal **" MAX"** is the whole difference, and no matching
file exists anywhere. The loader takes the path **verbatim** and prefixes only the .pe's own
directory (`wickedcalls_part3.cpp:2575`); it does **not** derive `<name><N>_color.png`. Fix by
renaming the four PNGs or re-saving from the editor — **not** by fuzzy-matching in the loader.
★ A missing basecolour gives **exactly grey untextured squares**: `emittedparticlePS_soft.hlsl:33`
does `half4 color = 1;` and only overwrites it when the descriptor is valid.

⚠ Other facts worth keeping:
- Both new files are archive **version 5077**; every other shipped .pe is 5076, and 5077 is the
  reader's inclusive ceiling — one more editor bump makes them silently unloadable.
- `Light Rain.pe` is a re-save of `heavy-rain3.pe` with only texture names changed — **9,494/s,
  25,000 cap**, i.e. as dense as heavy rain and 35% denser than `downpour.pe`.
- **No teardown catches a WPE root loaded outside Lua.** `CleanUpEmitterEffects` sweeps only
  `vWickedEmitterEffects`, whose sole push site is the Lua loader — so a weather root would leak
  across a level swap. (The recurring [[project-customdraw-hook-inventory]] failure shape.)
- `WickedCall_LoadWPE` hides only the **last** emitter; both new files have **two**, so emitter 0
  starts spraying at load.
- Intensity must drive `ec->count`, **never** `SetMaxParticleCount` — the latter blanks
  `counterBuffer` and forces a buffer recreate.
- ⚠ Latent landmine: `wickedcalls_part0.cpp:510` passes `IMPORT_NORMALMAP` on **every** texture
  load. If `IMPORT_BLOCK_COMPRESSED` is ever also set, alpha is forced to 1 game-wide.

Related: [[project-wpe-particles]], [[project-gpup-particles]], [[project-customdraw-hook-inventory]]
