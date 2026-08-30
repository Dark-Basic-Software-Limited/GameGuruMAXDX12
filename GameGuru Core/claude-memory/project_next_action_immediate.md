---
name: project-next-action-immediate
description: Current state and the exact next step on GameGuru MAX DX12 — read this FIRST when resuming
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-26T04:10:52.131Z
---

# ▶▶ RESUME HERE — state as of 2026-08-29 (3.35b-h: Lee's five reports)

**Everything committed and pushed. Both repos clean, 0 unpushed.**
Engine `a159b93e` · game `91b5c146`. Notes: `NIGHT_INVESTIGATIONS_2026-08-12.md` §3.35b–j.
**Gate: sweep `0829a` CLEAN 19/19** — POLYS identical to the 0825 reference, worst VRAM 3975.1 MB
(120.9 MB headroom). ★ Also diffed per-demo vs 0828e: VRAM deltas bidirectional in ~16 MB steps,
means −2.4 / −1.7 MB — restoring streaming feedback cost nothing. A gate answers “is it allowed”,
not “did it change”.

## ★★★ 3.35b–h — five reports from Lee, and the one that was a DEVICE HANG

| | what | state |
|---|---|---|
| 3.35b | Super Quick defaults to rung **3**, not 1 | shipped |
| 3.35c | Super Quick was overruling **Texture Detail** (killed mip-streaming feedback → “Full” looked worse than “Half”) | fixed |
| 3.35d/e | tooltips ran off the right of the screen | fixed, **NOT eyeballed by me** |
| 3.35f | baked water gets a Schlick distance opacity ramp | shipped, tunable |
| 3.35g | **GPU page fault on a freed texture → DEVICE_HUNG** | fixed, corroborated NOT proven |
| 3.35h | build folder cleanup, 3.19 GB | done |

### ★★★ The one to re-read: 3.35g
`gg_ApplyTextureDivideLive` raced the streaming thread. That thread applies replacements with a
bare `resource->texture = replace.texture` — no retention — while the divide swaps the same
`ResourceInternal`. A replacement queued before the rebuild lands after it and drops the fresh
texture while descriptors still point at it. **The guard already existed**: `GGReloadGuardBegin/End`
(1.44, for level reload). The divide never asked for it; it does now.
⚠ 42 Texture Detail toggles survived, but the fault was intermittent and I never reproduced it on
the OLD build. **Consistent with a fix, not evidence of one.** If it recurs, that is the first
place to look, and `dred_report.txt` + `Guru-Crash.log` (kept deliberately) are the instruments.

### ★★★ THE RULE THAT HAS NOW COST FIVE DEFECTS
**When a fast path drops a texture, substitute a sane value for EVERY channel that texture
supplied.** Never leave the multiply's identity — for a modulation map the identity is the TOP of
the range, the worst possible default. Paid for by: emissive, ORM/albedo, roughness, the streaming
feedback, and SVT.

### ★★★ A crash path must not depend on RTTI
3.26's device-lost handler used `dynamic_cast`; `__RTDynamicCast` threw and turned a handled
device loss into a **silent close**. Now virtual on the base. By the time a crash path runs, the
thing it reasons about is already broken — exactly when reading a vtable is least likely to work.

### ★★ 3.35i — every debug file now lands BESIDE THE EXE
`wi::helper::GetDiagnosticPath(name)` = `GetDirectoryFromPath(GetExecutablePath()) + name`.
RUNTIME-resolved via GetModuleFileName, **not hardcoded** — Lee pushed back on the word “absolute”
and was right to. Routed: screenshots, `log.txt`, 5 engine writers, 20 game `DUMP_*`/trace writers
(via `GGDiagFopen`). ⚠ `setup.ini` and `GG_fopen("files/treebank/...")` deliberately untouched —
PRODUCT files, and a sweeping regex would have taken them.
Lee's reason: all AI-generated temp/debug files then sit in one set in the root, easy to delete
before shipping. The device-lost dialog no longer points at `Files/log.txt`.

### ★★★ 3.35j — SHIP THE PDB with the alpha (Lee's call)
`CrashLogger.cpp:148` `SymInitialize(process, NULL, TRUE)` finds the pdb BESIDE THE EXE — it is the
only source of function names and file:line in `Guru-Crash.log`. **A rebuild makes a pdb that no
longer matches a shipped exe**, so deleting it would make every tester crash address on this alpha
permanently undecodable. Shipping it means testers' logs arrive already symbolised.
★ Verified via `DUMP_SCENEUPDATE` (same dbghelp path as the crash logger) — resolves fully.

### ★★ Method: a sweep is only as wide as its pattern
3.35i routed 20 writers matching `fopen("name"` and reported success. It never looked at
`fopen_s(&f, "name", ...)` — nine more, found only by RUNNING the feature and seeing where the file
landed. One of them (`"Files\\hairkill_dump.txt"` written while CWD is already `Max\Files`) is what
created the 1.28 GB nested `Max/Files/Files/` tree. **Enumerate by DESTINATION, not call pattern.**

### Open, written down, NOT fixed
- ⚠ **Screenshots and the engine log resolve against the process CWD**, which file dialogs change.
  Evidence: 8 byte-identical blank PNGs in the exe root over six months; `log.txt` in two folders
  in one session. Makes the device-lost dialog's own “see Files/log.txt” wrong ~half the time.
- Tooltip wrap width (42 em) is built and audited but never visually confirmed — ask Lee.
- ⚠ **Alpha packaging is DONE - do not re-tidy the build folder from memory of an older note.**
  3.38 GB of debris was removed on 08-29. What remains that LOOKS like debris and must NOT be
  deleted: `ffmpeg.exe` (two live editor features need it) and `GameGuruMAX.pdb` (**ships on
  purpose**, so testers' crash logs arrive symbolised; a rebuild cannot reproduce a matching one).
  Only remaining optional weight is ~3 GB of tutorialbank + Guides. See [[project-alpha-packaging]].

---

## Previous milestone — 3.34–3.35 (Super Quick made real)

**Everything committed and pushed. Both repos clean, 0 unpushed.**
Engine `544c6ccb` · game `73d6df22`.

## ★★★ 3.34-3.35 — WHERE THE 16 ms IN OPAQUE SCENE GOES, AND WHAT SUPER QUICK IS NOW

Lee's AMD card: Opaque Scene **16.36 ms**, Z-Prepass **8.39** on the four half-million-poly
buildings, where DX11 did ~4 and ~2. Super Quick (3.31) did **nothing** — correctly, because all
it collapsed was exotic material permutations and that scene is already base PBR.

### The answer, measured on TESTPRO2 city view (RX 9060 XT; Lee's card runs ~4x these)
```
Opaque Objects, stock                            3.92 ms   1320 draws/frame
  pixel shading                                  1.53   (39%)
      remaining maps + decals + screen-space     0.56
      tiled lighting + shadows                   0.94
      the albedo texture itself                  0.03   <- FREE
  FLOOR: a shader that fetches NOTHING           2.39   (61%)
      the fat COMMON vertex layout              ~1.09     <- 3.35 took most of this
      irreducible geometry submission           ~1.30     <- only a bake reaches this
```
- **Opaque Scene 3.96 is 3.92 objects.** Not terrain (0.03), not sky (0.01).
- **Not fill-bound**: 6.25x fewer pixels removes only 13% of the floor.
- After 3.35: stock → FLAT **−52%**, stock → LIT **−33%**. On Lee's card 16.4 → ~11 ms at LIT, ~7.9 at FLAT.

### What shipped
Three real cut-down opaque shaders behind the tickbox, picked by a slider (1 Shapes only /
2 Shapes and colour / 3 + light), on a **matched reduced VS+PS layout** so the vertex shader can
stop exporting what the pixel shader ignores. Plus the instruments that made it answerable:
`Opaque Scene` and `Z-Prepass` split into Objects / Terrain-Trees-Grass / Sky rows, `DUMP_DRAWS`
(draws per frame per pass), and debugvis **23 = surface.albedo**, **24 = surface.F**.

### ★★★ THE RULE THAT COST THREE DEFECTS IN ONE CHANGE
**When a fast path drops a texture, substitute a sane value for everything that texture supplied.**
Never leave the multiply's identity behind — for a modulation map the identity is the TOP of the
range, the worst possible default. Bitten three times in 3.34: emissive (white constant + dropped
black map = every surface an emitter, bloom veiled the whole frame), ORM `surfaceMap = 1`
(metalness/reflectance → material constants → **albedo ZERO** → black interiors), and roughness
(→ Wicked's 0.2 default → glossy plastic). All three fixed in 3.35, one place.

### ★★ Method, earned the hard way
- **A rendering defect has a measurable LOCATION.** Three theories reached by reading the code
  (metalness, roughness, lightmaps) were all wrong and each cost a build cycle. Every channel view
  was right first time. Build the missing view instead of the next theory. [[project-measuring-rules]]
- **A single test viewpoint is not a test.** The exterior looked perfect at every rung while the
  same build rendered the interior BLACK — outdoors is direct sun, indoors is almost entirely
  indirect diffuse and has nothing but albedo to show. Gate shading changes at both.
- ⚠ **`tail` in a pipeline withholds a background script's output until it exits, and killing the
  `tail` DISCARDS the buffer.** Lost a completed 10-minute run this way. Redirect to a FILE.

### Still open, in order
1. **~1.30 ms of geometry submission** — 1320 draws. Only fewer draws/triangles reach it: the bake.
2. `aztec_wall_152` is **1,530,144 verts for 510,048 tris — 3.0 verts/tri, completely unwelded.**
   Content-side, and it multiplies everything.
3. Delayed Shadows still off by default — still the largest single unclaimed saving (below).

⚠ **3.33 broke the GGTerrain shader build and no sweep noticed** — the engine builds clean and a
sweep on stale GGTerrain `.cso` passes anyway. **Prove an engine shader-header edit with a GAME
build.** Fixed in 3.34.

---

## Previous milestone (3.28-3.33) — gate: sweep `0828d_final` CLEAN 19/19
★ Read the **CLOSING SUMMARY at the END of `NIGHT_INVESTIGATIONS_2026-08-12.md`** first — it has
the ranked next actions, the clean negatives, and the instrument failures for the perf chase.

## ★★★ DO THIS FIRST NEXT SESSION
**Ask Lee whether he ticked "Delayed Shadows" and how it looked.** DX11 shipped it ON at both
layers, DX12 ships OFF. Biggest single saving found (−59% sun shadow, −25% whole-frame GPU) and
it is one click away in Graphics & Performance. ⚠ Default deliberately NOT flipped — the DX12
port regrouped the stagger to kill a cascade-blend flicker, so only his eye can approve it.
If he accepts the distant-shadow lag, **flip both defaults to match DX11**.

⚠⚠ **Before ANY test-game measurement: the in-game instruments are broken.** `DUMP_PROFILER`
serves a FROZEN snapshot and `GET_GPUMS` returns NOTHING in game state (editor varies
1.93/0.81/0.85; game gave 0.82 six times running). `SHADOW_LOCAL_RENDERED` via `GET_PERF_DATA`
is the only one that still works. ⚠ `PRESS_KEY` cannot drive gameplay either — it posts
WM_KEYDOWN and GameGuru reads gameplay keys through DirectInput.
Game `3d19c23f` (branch `main`) · Engine (branch **`master`**).
Narrative: `GameGuru Core/NIGHT_INVESTIGATIONS_2026-08-12.md` §3.13–**§3.25l**, which ends with a
CLOSING SUMMARY of the whole milestone.

## ✅ 3.25 SHIPPED — the low-spec bake milestone

Terrain Bake, Water Bake, Reduction Scale, per-level Brutal switches, two panel removals.
★ Full detail and the nine hard-won rules: [[project-terrain-bake]]. Method lessons 20-22:
[[project-measuring-rules]].

Gate: sweep `0826c_325` **CLEAN 19/19**, POLYS identical to the 0825 reference on all nineteen.
⚠ That sweep ran at `506b9022`; everything after it is bake-path work, default-off, verified by
`tools/bakestress.sh` / `baketier.sh` rather than by a re-sweep. **A fresh 19-demo sweep is the
first thing to run if anything here is about to ship.**

## ★★★ 3.28-3.33 — the DX11-vs-DX12 performance campaign (2026-08-28)

All Lee-confirmed good on the old AMD card up to 3.30. Narrative: `NIGHT_INVESTIGATIONS` §3.28-§3.33.

### The four fixes that landed, each with the mechanism

1. **3.28 Occlusion Cull Delay** — `IsOccluded()` needed **32 CONSECUTIVE** occluded frames, and a
   query slot is only allocated inside the frustum-visible branch, so anything sweeping through the
   frustum refilled its history and could NEVER be culled. Rotating: 602 draws/frame vs 55 parked,
   and occlusion ON vs OFF made no difference at all. Per-level slider; **Lee runs it at 2**.
2. **3.29 Shadow Detail Steps** — atlas rects sized from `min(1, range/dist)`, a CONTINUOUS function
   of camera distance, and the shadow cache keys on the rect — so walking re-rendered every local
   shadow every frame. Quantised (CEIL, so never blurrier than stock). Moving 4.21 → 1.84 ms.
   **Lee runs it at 4.**
3. **3.30 per-light invalidation** — one moving caster re-rendered EVERY local shadow. ★★★ An
   adversarial review found the naive fix unsafe: `dirty => cleared` but NOT `dirty => rendered`
   (the render loop bails after the clear pass at `:8427` etc.), and a cleared-not-rendered rect
   reads FULLY LIT and persists under `LoadOp::LOAD`. **Two optimisations stacked closed both of the
   hole's self-heal paths** — each safe alone. Shipped with a round-robin **refresh floor** so any
   residual hole is bounded (~0.8 s) rather than permanent.
4. **3.31 Super Quick Objects** + **3.32/3.33** — see below.

### ★★★ THE BIG ONE, NOT YET DEFAULTED: delayed sun cascades

**DX11 shipped them ON at both layers; DX12 ships OFF at both.** DX11 averaged **2.19 cascades per
frame**, DX12 renders all **5.0** — 2.28x the sun work, from a feature that is present and correct
in the port and merely defaulted off.

MEASURED (sun only, control first AND last): shadowmap **2.15 → 0.89 ms (-59%)**, whole-frame GPU
busy 4.39 → 3.28 (-25%). On Lee's card ~6 ms → ~2.5 ms.

⚠ **Default deliberately NOT changed** — the DX12 port regrouped the stagger on purpose to kill a
cascade-blend flicker, so it is a visual-risk call. The tickbox already exists ("Delayed Shadows").
**ASK LEE TO TICK IT AND LOOK.** `SET_DELAYEDSHADOWS 0|1` to A/B.

### Answers to Lee's four questions

- **C (sun 6 ms)**: the delayed-cascade default, above. ⚠ Cascade count and resolution are at
  PARITY (DX11 `SHADOWRES_2D=2048`, `SHADOWCOUNT_2D=5`) — do NOT re-chase those.
- **B (half-million-poly buildings)**: same cause. Both engines cull casters past cascade 3
  identically, but with the stagger live DX11 draws a building 3.83x/frame vs DX12's 5.0. +31%.
- **D (sun + 12 points superlinear)**: mechanism FOUND — one shared atlas, and the sun's cascade
  cache keys on the atlas layout (`forceAll |= localAtlasFullClear`), so points invalidate the sun
  AND the sun's full clear destroys every cached local. DX11 used three separate arrays, no
  coupling possible. ⚠ **NOT reproduced on this PC**: 16 granted lights, `pack_scale` pinned at
  1.0, opaque rose LINEARLY (0.91/1.15/1.20/1.43 at caps 0/4/12/32). Needs light density.
  ★ 3.30 should already have reduced this a lot, since it makes `localAtlasFullClear` rare.
- **A (3000 objects)**: only PARTIALLY answered. PSO lookup is a hash in DX12 vs a flat array index
  in DX11 (~0.3-1.0 ms of recording), plus a per-pixel streaming-feedback atomic DX11 never had.
  ⚠ Neither shown to dominate; the deciding measurement is whether the frame is CPU-recording-bound.

### ★★ Method lessons paid for tonight

- **Before A/B-ing a switch, measure whether the thing it switches is even PRESENT.**
  `DUMP_MATERIALTYPES` showed GameGuru content is ~100% base PBR (COLLAPSIBLE **0** of 3307), so the
  first Super Quick design was worth nothing. Without it the null A/B reads as "the switch is
  broken" and costs a debugging session.
- **A saving that scales with N cannot be measured at N=3.** Super Quick measured inside the noise
  at the default viewpoint (3 granted lights) and **-30% opaque** in the light cluster (16).
- ⚠ **In TEST GAME `DUMP_PROFILER` is a FROZEN snapshot and `GET_GPUMS` returns NOTHING.** Editor
  varies 1.93/0.81/0.85; game gave 0.82 six times running. `SHADOW_LOCAL_RENDERED` via
  `GET_PERF_DATA` is the one in-game instrument that still works.
- ⚠ `PRESS_KEY` posts WM_KEYDOWN; GameGuru reads gameplay keys via DirectInput, so **it does not
  trigger in-game actions**. The ack proves a message was posted, nothing more.
- ⚠ An engine build **clears `Max/shaders/*.cso`** — do not build the engine during a sweep.
  Prove a shader edit landed by the **.cso SIZE** (objectPS 92056 → 92256), never the timestamp.

## ✅ 3.26 — the 08-27 crash, fixed and gated (2026-08-28)

Engine `cd559901` / game `e9f3117f`. Sweep `0828a_crashfix` **CLEAN 19/19** (C3 3830.3 MB).
Narrative: `NIGHT_INVESTIGATIONS` §3.26 and §3.26a.

★★★ **The pattern, not the fix, is the thing to remember: `dx12_check` RETURNS the HRESULT and
asserts on it, and the assert compiles out under NDEBUG.** So `dx12_check(x)` is `(void)x` in every
release build. EIGHT sites in one chain discarded a failure and dereferenced the null it left.

★★ **A guard whose condition can never be false is worse than no guard** — `is_window_active` had
ZERO writers in either repo and every WM_SIZE went straight to a swapchain teardown. ⚠ Deliberately
still not wired: `wiApplication` reads the same flag to skip updates, so restoring the writers would
pause MAX on focus loss — a product decision. ⚠ Side effect nobody had hit: minimise was driving
`CreateSwapChain` with a 0x0 canvas on a HEALTHY device.

★★ **`Application::Run()` genuinely re-enters** — `StartForceRender` pumps the queue then calls
`RunCustom()`, from five call sites, during ORDINARY level loading. Any RAII guard here must
save/restore, never clear to a constant (my own first version got this wrong).

★★★ **THE PRIMARY CAUSE IS STILL OPEN.** Six `INVALID_CALL` removals since 08-07, all inside
level-load bursts. `Close() == E_INVALIDARG` is NOT diagnostic — this driver returns it for every
Close once the device is gone. ⚠ I wrongly called the 08-26 render-pass fix an "exact precedent";
11:07 the same day is AFTER it. A static audit of every recording surface found NOTHING.
**The next occurrence now answers itself**: `GetDeviceRemovedReason()` is logged before the Close
loop, the failing lists are named as a set, and the validation layer is drained in Release.
Suspects if it recurs, all load-path: `CopyAllocator`, aliasing resources, and ⚠ **`gg_single_queue`
(default TRUE)** routing COMPUTE/COPY onto the graphics queue.

★ `CLOSEFAIL: n` in `GET_PERF_DATA`, always printed including the zero — `Max/Files/log.txt` is
LOST under `taskkill //F`, so a harness run otherwise proves nothing about what was logged.

## ✅ 3.25x — animation near distance 500 → 1000 (gated CLEAN 19/19, `0827d_near1000`)

Lee: "strangeness as characters run towards me". ★ The reduction holds the POSE but not the
TRANSFORM, so a held runner SKATES — worst head-on, where angular size grows fastest and the feet
are in view. The constant is BOTH cutoff and pro-rata reference, so moving it stretches the whole
curve; moving only the cutoff would have stepped a crossing character from period 1 to 6 at once.
Cost ~1.5 points of hold rate.

## ✅ GATED — sweep `0827c_grace` CLEAN 19/19 (2026-08-27, latest)

Game `2f67fc4a` / engine 3.25v, both pushed. C1 19/19 · **C2 POLYS identical on all nineteen** ·
C3 3975.2 MB (120.8 MB headroom) · C4 all in-game. Reduction Scale defaults to **25**.

Two independent guards now protect the animation reduction, and they do NOT subsume each other:
- **posed-once** (3.25t) — never hold an armature that has not been posed since the level loaded.
  This is what fixed the POLYS non-determinism; only C2 ever caught it.
- **a 10-second grace** (3.25v/w) — hold nothing at all for the first 10 s of a level, re-armed on
  both edges of `bImGuiInTestGame` because entering the test game is not a level load but does
  spawn characters and start their scripts. Lee's "Horseshoe Bend flickered on load, unticking
  fixed it, re-ticking did not bring it back" — the signature of load-time-only damage.

★★★ **It took three tries to count ten seconds, and every wrong one failed the SAME direction.**
Subtracting `Update()`'s dt gave 10.0→3.1 over 16 real seconds; subtracting a real steady_clock
delta clamped to 0.25 s/step gave 9.7→1.8; a **deadline** gives 9.9→0.0. Both failures
UNDER-counted, so the grace silently lasted ~2.3x longer than advertised — which keeps the feature
switched OFF while looking like it works, and **has no symptom at all**. Accumulating deltas was
the wrong shape from the start: it depends on how often the code runs and on every clamp in the
chain. A claim about the wall clock must be read off the wall clock.
★ The re-arm COUNTER is what made this tractable — it proved the trigger was firing correctly and
sent me at the countdown instead of hunting a phantom repeated reset.

## ⚠⚠ THE ONE THING THAT MATTERS NEXT

**None of 3.25 has been measured on the hardware it was built for.** Every number in the notes is
from a fast card and is corroboration only. Lee has a 6-year-old AMD card on another PC. Ask him
for, in this order:
1. Terrain Bake on/off frame ms (he measured the real terrain at 10 ms there)
2. whether the 7 ms `GPU Idle + unranged` decay survives Water Bake (NOT reproduced here)
3. whether Reduction Scale moves `Skinning and Morph`. ★ MECHANISM VERIFIED 3.25n: at scale 25
   it holds **93% of 352 armatures** per frame. But `Skinning and Morph` is **0.01 ms here**, so
   the FPS sweep is noise and the WORTH is still unmeasured. ★★ The saving SATURATES at ~25 —
   50 and 100 add ~1% between them, so recommend 20-25, not the top of the slider.
4. what near-tier detail he settles on — 4096 vs 8192 is purely a memory choice

## Known-open, honest list

- Editor overlay lines (waypoints/zones) looked absent over baked terrain in one screenshot.
  UNVERIFIED - one glance would settle it.
- `GGTerrain_Draw_Transparent` has the same latent camera-CB exposure `gpup_draw` caused for the
  water plane. Editor-only debug geometry, which is the only reason it has never shown.
- ⚠ I killed Lee's MAX with UNSAVED changes (title showed `*`) by combining the check and the kill
  into one command. **Check liveness in a SEPARATE command from the kill, always.**

## Next actions, in order

1. **Lee must test on the AMD card** — that is the only place these numbers mean anything.
   Ask him for: terrain bake on/off frame ms, whether the 7 ms water decay survives, whether
   Reduction Scale moves `Skinning and Morph`, and whether the Water Bake shoreline edge (hard
   and speckled where the ocean had soft foam) bothers him enough to fix.
2. **Terrain Bake follow-ups if he wants them**: BC1 compression (would cut the 220 MB to ~28 MB;
   needs a block-compress pass since UAVs on BC formats are prohibited), and normals/roughness are
   currently a flat dielectric guess rather than baked.
3. `masterinterpreter` at 38.8 µs of Lua per patrol character — the thing that scales with content.
4. Texture Detail + streaming composition (⚠ blocked on the `data_offset` absolute-mip path that
   caused the Trapped/RPG load crash).

## Do-not list

- ⚠ Do NOT re-sort the profiler tree by cost; do NOT re-read `parent`/`main_thread` per frame.
- ⚠ Do NOT judge Texture Detail (or the bake) by driver VRAM alone.
- ⚠ Do NOT chase hub-wide FPS drift; it is ambient. **Reboot before any FPS-comparing sweep.**
- ⚠ Do NOT re-derive the CPU panel's nesting — parents and children legitimately overlap.
- ⚠ Engine pushes go to **`master`**. Engine builds take **~40 seconds**.
- ⚠ Long heredocs truncate and `\n` inside a bash-quoted python `-c` **collapses into a real
  newline inside C string literals** (C2001). Write patch scripts to a FILE with the Write tool.
- ⚠ `wiProfiler.cpp` is **LF**, `wiProfiler.h` is **CRLF**; `GameGuruMain.cpp` is CRLF.
  A patch script must DETECT, not assume.
- ⚠ This ImGui has no `BeginDisabled`/`EndDisabled`.
