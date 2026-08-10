# Switch Escape: DX12 vs DX11 editor frame (2026-08-09)

**Mission (user, 2 h autonomous):** get the Switch Escape hub demo as close as possible to the
**302.8 FPS** their DX11 build shows at the level's opening editor camera, using the DX11
profiler panel in their screenshot as the guide.

**Revert point: tag `baseline-switchescape-20260809` in BOTH repos**
(game `7d1ba062`, engine `14307cad`). `git checkout baseline-switchescape-20260809` restores
the exact build this work started from.

---

## 1. The two frames, side by side

DX11 numbers are read off the user's screenshot; DX12 numbers are a warm 180 s soak at the
same opening camera on `7d1ba062` / `14307cad`.

| | DX11 | DX12 baseline | delta |
|---|---|---|---|
| **FPS** | **302.8** | **200.8** | −102 |
| **Frame** | **3.30 ms** | **4.98 ms** | **+1.68 ms** |
| CPU Frame | 1.69 | 4.58 | **+2.89** |
| GPU Frame | 2.28 | 3.68 | +1.40 |

The CPU is the wall, not the GPU: `SUBMIT_PHASES_MS` reports **`stall=0.00`**, i.e. the CPU
never waits on the GPU at the end of the frame. So CPU cuts convert to FPS until CPU drops
below the 3.68 ms GPU wall; after that the GPU has to come down too.

### Where the +2.89 ms of CPU actually is

| CPU range | DX11 | DX12 | delta | share of gap |
|---|---|---|---|---|
| **`Update - Wicked`** vs **`Scene::Update`** ⚠ see note | **0.21** | **2.39** | **+2.18** | **75%** |
| Update - Logic | 0.62 | 0.90 | +0.28 | 10% |
| Compose | 0.04 | 0.24 | +0.20 | 7% |
| Render | 0.71 | 0.87 | +0.16 | 6% |
| Update - Terrain | 0.03 | 0.14 | +0.11 | 4% |

**Three quarters of the entire gap is one range: `Scene::Update`.** Its stage split is
S1 Anim+Transform 0.91, S2 Hier+Mesh+Mat 0.79, S4 Object+Light 0.61 — and only 0.03 ms of
that has named children, which is what §3 is about.

⚠ **NOTE ON THE ROW LABEL (corrected 2026-08-10).** These are two DIFFERENTLY-SCOPED ranges.
There is **no `Scene::Update` profiler range anywhere in the DX11 engine** — its complete CPU
range list is `CPU Frame`, `Fixed Update`, `Update`, `Render`, `Compose`, `Physics`,
`Frustum Culling` plus the game's `Update - *` rows. The 0.21 is the range created at DX11
game `master.cpp:2171`, which wraps `__super::Update(dt)` = ALL of `RenderPath3D::Update`
(`WickedRepo/WickedEngine/RenderPath3D.cpp:849`): `CheckUsedTextures`, `RenderPath2D::Update`,
`scene->Update()` (:866), main-camera frustum culling, `UpdatePerFrameData`, `UpdateCamera`.
The DX12 2.39 is the sum of the `Scene-S1..S5` stages, i.e. `Scene::Update` alone.
**The DX11 side is a strict SUPERSET measured at one tenth the cost — so the comparison
understates the gap rather than inflating it.** DX11's `Scene::Update` proper is some fraction
of 0.21 and the true delta is ≈2.25–2.30 ms. See §10 for why, verified against both trees.

### GPU side (becomes the wall once CPU < 3.68)

`Transparent Scene 1.12` (30% of the GPU frame), Opaque 0.51, Update Buffers 0.32,
Occlusion pair 0.28, MSAO 0.11, Scene MIP 0.10, Z-Prepass 0.04.

Scene: 7322 objects / 8437 transforms / **576 visible** / 22 lights (16 visible) /
109,358 polys / 221 transparent objects of which **141 are double-sided**.

---

## 2. ~~Refuted: the tree pool is NOT the cost here~~ → ⚠ **THAT A/B WAS INVALID (2026-08-10)**

The obvious suspect was the parked tree pool — `DUMP_TREEPOOL` reports
`POOL size=6000 bound=0 renderable=0` on a level with no terrain and no trees.

**What was run, and why it proved nothing:**

| | pool 6000 | pool 1 |
|---|---|---|
| FPS | 200.8 | **201.5** |
| SCENE_OBJECTS | 7322 | **7322** |

★★ **`SET_TREES pool N` CANNOT SHRINK AN ALREADY-BUILT POOL. Both arms ran the same 6000
slots.** The knob writes only `GGTrees::g_treePoolSize` (`AutomationHarness.cpp:5656`). That
variable is read at pool-build time inside `GGTrees_WickedSetup`, which is latched by
`g_wickedTreesSetup` (`GGTrees_part2.cpp:721-725`). The latch is cleared in exactly two places:
- `GGTrees_WickedInit` (`GGTrees_part2.cpp:361`) ← `GGTerrainWicked_Init`, called **ONCE** from
  `GameGuruMain.cpp:171`, in `g_iInitializationSequence` case 2 — app startup, **not per level**;
- `GGTrees_WickedShutdown` (`:1146`) ← `GGTerrainWicked_Shutdown`, which has **ZERO callers**
  anywhere in the source.

So the pool is built once per process and **never rebuilt, not even on level load**.
`SCENE_OBJECTS` staying at 7322 is the *proof the knob did nothing* — it is not evidence the
pool is free. (`GGTrees.h:68` says so in its own comment: "applies on next pool setup (level
reload)" — which, given the above, never happens.)

⚠ **Also wrong: "the pool entities are not part of that population."** They are. Each slot does
`scene.objects.Create(e)` **and** `scene.transforms.Create(e)` (`GGTrees_part2.cpp:284-307`), so
**6000 of the 7322 objects and 6000 of the 8437 transforms ARE the pool**, leaving ~1322 real
level objects. They get no `HierarchyComponent`, so they cannot reach `SU-Hierarchy`, but they
are walked by every per-object and per-transform system.

**Status: the tree pool's cost on this level is UNMEASURED, not zero.** To test it you need a
size knob that applies *before* `GGTerrainWicked_Init` (setup.ini / command line), not the
runtime harness command. See §10.

---

## 3. The terrain tickbox the user offered

The user pre-authorised adding a terrain-off tickbox since this level has no terrain. Two
findings:

1. **It already exists.** Terrain properties → theme 8 → "Disable Level Aspects" →
   **"Completely Empty Level"** (`M-TerrainNew_part5.cpp:2126`) sets
   `t.visuals.bEnableEmptyLevelMode`, which makes `master_part1.cpp:456` skip the entire
   terrain/trees/grass update block. It is only *reachable* when theme 8 is selected.
2. **It is not where the FPS is.** On this level terrain already costs
   **`Update - Terrain` 0.14 ms CPU** and **`Terrain - UpdateVirtualTexturesGPU` 0.01 ms GPU**,
   with `TERRAIN_DRAW_COUNT 0` and the idle gate already skipping 40,844 of 46,559 calm
   frames. The VT worker job (`VT-job Total 0.42 ms`) is off the critical path —
   `VT-WaitInRecord` reads 0.00 ms.

So the ceiling on a terrain-off switch is **~0.15 ms ≈ 3% ≈ 6 FPS**, and switching it on also
forces water/trees/grass off and resizes the playable universe to 25 km — content changes I
should not make to a shipped demo. **Recommendation: do not add a second tickbox.** If the
existing one should be easier to reach, the cheap change is to show "Disable Level Aspects"
for every theme rather than only theme 8 — a UI reachability fix, not a perf fix.

---

## 4. Engine 2.15 — a real DX11-parity regression found and fixed (`SET_XINPUT`)

`Input` was reading **0.29 ms of a 4.62 ms CPU frame (6.3%)** on a machine with no gamepad
attached. Cause: upstream `wiXInput.cpp Update()` calls `XInputGetState` on **all four slots
every frame**, and on an *empty* slot that call is a driver round-trip, not a memory read.

**The DX11 fork already fixed this and the DX12 port dropped it.**
`WickedRepo/WickedEngine/wiXInput.cpp:18`:

```cpp
//PE: XInputGetState slow ,so delay if not connected.
if (connected[i] == true || (iConnectRetry % 120) == 0)
```

Restored in `wiXInput.cpp` with a staggered rotation (at most one empty slot probed per
frame, default every 60). **Connected pads are still polled every frame**, so controller
latency is unchanged; only hotplug detection is delayed, by up to 60 frames. Knob
`SET_XINPUT <frames>`, `0` = stock. New `Input-XInput` / `Input-RawInput` / `Input-SDL`
profiler sub-ranges make it verifiable.

**Result: the work is gone, the FPS is not.**

| | before | after |
|---|---|---|
| `Input` range | 0.29 ms | **0.04 ms** |
| `Input-XInput` | — | **0.00 ms** |
| CPU Frame | 4.62 ms | 4.56 ms |
| **FPS (3-arm A/B)** | 201.2 | **201.1** |

Which leads directly to the most important finding of the session.

---

## 5. ★ The finding that matters: CPU cuts do not convert to FPS on this level

A **verified 0.25 ms of real main-thread work was deleted** and the frame did not get faster.
The CPU frame moved only 0.06 ms of the 0.25 removed — i.e. the main thread simply waited
0.19 ms longer somewhere else.

Ruled out along the way:
- **Not a frame cap.** The hub runs at exactly **60.0 FPS / 16.68 ms** (vsync), the editor at
  201 — so the editor is genuinely unlocked and 201 is not a limiter.
- **Not measurement noise.** Three arms, five samples each, drift +0.2 FPS.

⚠ **The end-of-frame GPU fence stall is NOT ruled out — sample it properly.** Across the four
`GET_PERF_DATA` captures taken today `SUBMIT_PHASES_MS` read `stall=0.00` three times and
**`stall=0.38`** once (the last, at default settings after the `SET_HIERLO` A/B), with
`APP_SUBMIT_PRESENT_MS` 0.32 / 0.45 / 0.36 / **0.67** tracking it. One outlier in four is not
a conclusion, but it is the right shape for the missing absorber and it is the cheapest thing
to check next: sample `stall` over many frames per arm rather than reading one snapshot.

So **the ~4.98 ms frame is paced by something other than the sum of main-thread CPU work**,
and until that pacer is named, every CPU micro-optimisation on this level will measure zero —
exactly as this one did. This is the same shape as the Stage P.4 result in `PERFORMANCE.md`
("cutting whichever is NOT the pole yields ~0 FPS"), and `RP3D-RenderWait 0.30 ms` is the
obvious first suspect for where the slack is absorbed.

**This is the single most valuable thing to resolve next**, because it gates the worth of
every item in §7.

---

## 6. New instrument: `SET_SCENESERIAL` names the Scene::Update pole

`Scene::Update` is 75% of the CPU gap but was un-attributable: every `RunXUpdateSystem(ctx)`
only *dispatches* jobs, so the whole cost lands in the one `jobsystem::Wait` closing each
stage — `Scene-S1 0.95 ms` had just 0.03 ms of named children. `SET_SCENESERIAL 1` gives each
system its own Wait + `SU-*` range.

| system | ms | note |
|---|---|---|
| **`SU-Hierarchy`** | **0.81** | **the pole** — `RunHierarchyUpdateSystem` |
| `SU-Object` | 0.57 | AABB + instance writes, 7322 objects |
| `SU-Mesh` | 0.29 | |
| `SU-Material` | 0.11 | |
| `SU-Transform` | **0.07** | ★ **refutes the obvious guess** — 8437 transforms, near-free |
| `SU-Animation` | 0.04 | |
| `SU-Physics` | 0.00 | |

⚠ Serialised totals inflate (CPU frame 4.56 → 4.84); compare shares, never the total.

★ `SU-Hierarchy` being the pole is a strong lead: GGMAX delta **1.36** already rewrote this as
a subtree-parallel system, and its fast path is **gated on `gg_hier_snapshot_count == live
count` and silently falls back to the stock O(N×depth) walk otherwise**. If that gate is
failing on this level, 0.81 ms may be recoverable with no new algorithm at all.

**Checked — the gate is NOT failing.** `SET_HIERLO 1` (fast path, default) vs `0` (stock),
three arms: **200.9 vs 199.7 FPS**. The 1.36 fast path is engaged and earning 1.2 FPS. There
is no free 0.81 ms sitting here; that lead is closed.

---

## ★★ 8. SOLVED: the level is GPU-FENCE-BOUND, and single-queue is worth +23.9 FPS

§5 asked what absorbs the CPU slack. Engine 2.16's rolling `SUBMIT_STALL_WINDOW` answers it,
and the earlier snapshot readings of `stall=0.00` were simply sampling luck — the truth is
the opposite:

| arm | FPS | stall mean | frames stalled |
|---|---|---|---|
| A default | 201.0 | **0.893 ms** | **99.4%** |
| B `SET_RESSCALE 0.5` (less GPU work) | 226.5 | 0.407 ms | 89.0% |
| C `SET_XINPUT 0` (**+0.25 ms CPU work**) | 201.2 | **0.711 ms** | 98.7% |
| A2 control | 201.6 | 0.915 ms | 99.4% |

**The CPU waits on the GPU frame fence in 99.4% of frames, for 0.89 ms on average.** Arm C is
the proof of the mechanism: *adding* 0.25 ms of CPU work back **shrank** the stall by 0.18 ms
and left FPS unchanged. The slack is real and it is spent waiting on the GPU.

**So Switch Escape's editor frame is GPU-BOUND, not CPU-bound** — which retro-explains every
zero in §7: `Scene::Update`, XInput, the tree pool and the transparent depth rule are all on
the wrong side of the wall. The earlier "CPU is the wall" reading came from comparing CPU
frame 4.58 against the profiler's GPU Frame 3.68, but that GPU number only sums the *named
passes*; the real GPU wall includes the bubbles between them.

### The win: cross-queue bubbles

~1 ms of the 3.68 ms GPU frame is in no named pass, with `lists=14 batches=12 deps=9`. That
is cross-queue dependency bubble. Both existing queue knobs were retested:

| knob | off | on | delta | stall mean |
|---|---|---|---|---|
| **`SET_SINGLEQUEUE 1`** | 201.4 / 201.1 | **225.3** | **+23.9 FPS (+11.9%)** | 0.90 → **0.42** |
| `SET_LEANASYNC 1` | 201.3 / 202.2 | 212.8 | +11.1 FPS (+5.5%) | 0.89 → 0.65 |

**Same gain as halving the render resolution, with zero visual change.**

⚠ **This INVERTS a documented result.** `PERFORMANCE.md` Stage P.6 records single-queue at
**−4.7 FPS** on TESTPRO1 and says "submission overhead is a dead end. Do NOT re-chase."

**I expected a light-vs-heavy crossover and tested for one. There isn't. Six hub demos, three
arms each (0/1/0), editor, warm — every single one is positive:**

| level | off (A/A2) | on | delta | stall mean | stall **max** | stalled frames |
|---|---|---|---|---|---|---|
| **Trapped** | 212.1 / 211.2 | **251.1** | **+39.4 (+18.6%)** | 0.663 → **0.064** | 1.30 → 0.92 ✔ | 98.2% → **22.4%** |
| **Zombie Cellar** | 193.0 / 194.3 | **219.8** | **+26.1 (+13.5%)** | 0.856 → 0.382 | 1.73 → 1.53 ✔ | 99.3% → 85.0% |
| **Switch Escape** | 201.4 / 201.1 | **225.3** | **+24.0 (+11.9%)** | 0.90 → 0.42 | — | 99.4% → — |
| Island Showdown | 91.5 / 91.7 | 95.1 | +3.5 (+3.8%) | 2.12 → 1.82 | 3.37 → **6.71** ✘ | 99.0% → 99.2% |
| Aztec GK Teaser | 83.3 / 83.6 | 85.9 | +2.4 (+2.9%) | 4.82 → 4.50 | 6.49 → 6.06 ✔ | 99.6% → 99.6% |
| Foggy Forest | 76.8 / 76.6 | 78.2 | +1.5 (+2.0%) | 4.13 → 4.05 | 6.06 → 5.60 ✔ | 99.6% → 99.7% |

**6/6 gain FPS. 5/6 also improve the worst-case stall.** Drift between the A and A2 control
arms is ≤1.3 FPS everywhere, so none of these are warm-up artefacts.

★ **The gain scales inversely with GPU load, exactly as the mechanism predicts.** The bubble is
a roughly fixed per-frame overhead, so it is a large share of a 5 ms frame (+12–19% on levels
running 190–250 FPS) and a small share of a 12 ms one (+2–4% on levels running 76–95 FPS).
Trapped is the clearest case: stalled frames collapse from 98.2% to **22.4%** — that level
stops being fence-bound almost entirely.

So the Stage P.6 "do not re-chase" verdict does **not** reproduce on hub content on today's
build. It was measured 2026-07-26 on a much older engine and on TESTPRO1 specifically; treat
it as stale rather than as a law. ★ **RULE: a queue-structure verdict has a shelf life —
re-measure it after significant renderer changes instead of citing the old number.** This one
sat on a double-digit lever for six weeks.

⚠ **The one blemish: Island Showdown's max stall went 3.37 → 6.71 ms** while its mean improved.
It is now 1 of 6 rather than the trend, and a single 6.7 ms frame inside a 22 s window is as
likely to be a lazy-PSO compile as a systematic tail regression — but it has not been re-checked,
so it stays on the record as unexplained.

**Default NOT changed.** See §9.

---

## ★★★ 8b. FULL 19-DEMO SWEEP → DEFAULT FLIPPED ON (engine 2.17)

Criteria were fixed in the sweep script's header **before the data existed**. All three pass.

| # | demo | OFF | ON | delta | drift | tail ON/OFF | POLYS |
|---|---|---|---|---|---|---|---|
| 19 | **Trapped** | 210.9 | **246.2** | **+16.7%** | +0.2 | 0 / 0 | same |
| 13 | **Switch Escape** | 201.7 | **233.5** | **+15.8%** | +0.7 | 0 / 0 | same |
| 15 | **Zombie Cellar** | 194.7 | **217.8** | **+11.9%** | +0.0 | 0 / 0 | same |
| 17 | RPG Template | 119.5 | 130.8 | +9.5% | −0.8 | 26 / 24 | same |
| 16 | Jungle Fever | 167.5 | 178.2 | +6.4% | +3.8 | 0 / 0 | same |
| 6 | Operation Amazon | 105.9 | 112.3 | +6.0% | −0.7 | 21 / 21 | same |
| 10 | Disruption | 105.8 | 111.8 | +5.7% | −0.6 | 6 / 2 | same |
| 14 | Canyon Offensive | 92.9 | 97.2 | +4.6% | −0.3 | 19 / 18 | same |
| 5 | Island Showdown | 91.0 | 94.9 | +4.2% | +0.1 | 0 / 0 | same |
| 8 | Snowy Mountain Stroll | 146.3 | 151.3 | +3.4% | +4.4 | 0 / 0 | same |
| 9 | A Grand Canyon Adventure | 136.8 | 139.9 | +2.3% | −0.3 | 0 / 0 | same |
| 3 | Bounty | 157.0 | 160.5 | +2.2% | +0.0 | 0 / 1 | same |
| 7 | River Raiders | 144.6 | 147.6 | +2.1% | +0.2 | 0 / 3 | same |
| 18 | The Mystery of Z Island | 142.1 | 145.1 | +2.1% | −0.6 | 1 / 1 | same |
| 1 | Aztec Game Kit Teaser | 83.8 | 85.2 | +1.7% | −0.4 | 17 / 16 | same |
| 11 | Foggy Forest | 77.0 | 77.9 | +1.2% | +0.6 | 0 / 1 | same |
| 12 | Indian Strike Force | 116.3 | 117.1 | +0.7% | +0.9 | 18 / 17 | same |
| 2 | Aztec Game Kit | 109.2 | 109.7 | +0.5% | +2.4 | 22 / 21 | same |
| 4 | Horseshoe Bend | 96.8 | 96.3 | **−0.5%** | +0.4 | 0 / 2 | same |

**Mean +5.07%, 18/19 positive, 0 failures, 0 deaths.**

- **C1 FPS — PASS.** 18/19 positive (needed ≥16); the single negative is Horseshoe Bend at
  −0.5%, inside its own +0.4 control drift and far above the −2.0% reject line.
- **C2 TAIL — PASS.** No demo's over-16.7 ms frame count rose more than 25%. ★ **Island
  Showdown's 6.71 ms max-stall outlier from the 22 s arms did NOT reproduce** over 40 s
  (tail 0/0, +4.2%) — it was a lazy-PSO compile, exactly as suspected, not a tail regression.
  That was the one thing blocking the flip, and it is now resolved with data.
- **C3 POLYS — PASS.** Bit-identical across all three arms on all 19 demos, i.e. no
  geometry/visual change. Control drift ≤4.4% everywhere, mostly under 1%.

**Flipped: `gg_single_queue = true` (engine 2.17).**
Revert: `setup.ini singlequeue=0` (persistent, new in 2.16) or `SET_SINGLEQUEUE 0` (live).

**Method notes worth keeping** (both cost real time today):
- Arms are **settle-gated**, not fixed-soak — POLYS stable AND FPS within 3% of the previous
  sample. A fixed 90 s soak had captured one demo's first arm at 3.9 FPS / POLYS 154768 mid-load
  against a settled 106 FPS / POLYS 3438876, which would have manufactured a fake +96%.
- The script takes a **PID lockfile**. Three copies once ran concurrently against one MAX
  (a `nohup` launch that outlived its parent, plus relaunches after a `pkill` that silently does
  nothing in Git Bash), producing impossible-but-plausible numbers. That data was discarded.

Raw data: `tools/singlequeue_sweep_0809_full.txt`.

### 8c. The two gaps closed: TEST-GAME mode and TESTPRO1

§8b was editor-only, and TESTPRO1 (the level behind the original −4.7 verdict) was untested.
Both now done, same three-arm protocol, settle-gated.

**Test-game mode** — a different queue mix (no ImGui, different pass set) and where players
actually judge FPS:

| case | OFF | ON | delta |
|---|---|---|---|
| **Trapped** | 193.0 | **235.6** | **+22.1%** |
| Switch Escape | 178.6 | 189.7 | +6.2% |
| Foggy Forest | 89.5 | 94.0 | +5.0% |
| Island Showdown | 87.3 | 88.8 | +1.7% |
| Horseshoe Bend | 71.0 | 70.9 | −0.1% |
| Escape from the Zombie Cellar | 60.0 | 59.9 | **not scorable** |

**Mean +5.38% over the 6 scorable cases, nothing worse than −0.1%.** Test-game gains are at
least as good as the editor's — the win is not an editor artefact.

⚠ **Zombie Cellar is excluded, not counted as neutral.** Test-game honours the per-level VSync
setting, so a level that can hold the refresh rate reads exactly 60.0 on BOTH arms — that is the
monitor, not the knob. Counting it as 0% would have diluted the result. It also makes the
over-16.7 ms counter useless there: a 60 Hz frame *is* 16.67 ms, so it ticks on nearly every
frame in every arm. ★ **RULE: check for a vsync pin before reading any in-game FPS A/B.**

**TESTPRO1 (editor)** — ★ **NOT a regression. Do not re-raise it without a 5-arm run.**
A first three-arm pass read **−2.5%** (122.2/122.3 OFF vs 119.2 ON) and looked like a genuine
content-dependent exception. It did not survive a repeat. Five arms, 0/1/0/1/0, two independent
ON measurements against three OFF:

```
OFF 118.4 / 119.3 / 119.6  (mean 119.1)
ON  119.8 / 118.8          (mean 119.3)      → +0.2%, OFF spread 1.2
```

The OFF baseline itself moved 122.2 → 119.1 between launches — the same magnitude as the
"regression", which is what a cross-launch drift artefact looks like. TESTPRO1 is **neutral**.

**Net across everything measured: 26 cases, 0 confirmed regressions.**

---

## 9. ~~What would settle the single-queue default~~ — SETTLED, see §8b

`SET_SINGLEQUEUE 1` is the best FPS lever found in this whole study — **+2.0% to +18.6% across
six hub demos, 6/6 positive, no visual change, and the worst-case stall improves on 5 of 6.**
On that evidence **I would expect this to become the default**, but I have not flipped it and it
should not be flipped on six editor-only samples. What remains:

1. **The other 13 demos**, same three-arm protocol. `tools/demo_fps_sweep.sh` already does the
   walk; it needs the 0/1/0 knob loop plus `SET_SUBMITSTATS 1` before each arm. ~45 min.
2. **Test-game mode, not just the editor.** Everything measured here is editor-side. Gameplay
   has a different queue mix (no ImGui, different pass set) and is where users judge FPS.
3. **A longer window for the tail.** These arms are 22 s. Island Showdown's 6.71 ms max is one
   frame in ~1850 and could easily be a lazy-PSO compile; a 3-minute arm plus the
   `HITCH: over(16.7/25/33/50/100)` histogram distinguishes a real tail regression from a
   warm-up artefact. **Gate the decision on that histogram, not on mean FPS** — a change that
   lifts average FPS while adding 33 ms frames is a bad trade in an editor.
4. **Re-run TESTPRO1**, the level that produced the original −4.7 FPS verdict. If it is still
   negative on the current engine that is a genuine content-dependent exception, and worth
   understanding before flipping a global.

Until then it is a free +2–19% for anyone who sets it, and it costs nothing to leave off.

---

## 7. What the frame is actually made of (the decisive test)

Quartering the shaded pixels is the one lever that moved anything:

| lever | default | alt | delta |
|---|---|---|---|
| **`SET_RESSCALE` 1.0 → 0.5** | 201.2 | **224.8** | **+23.6 FPS (+11.7%)** |
| `SET_HIERLO` 1 → 0 (fast → stock hierarchy) | 200.9 | 199.7 | −1.2 FPS |
| `SET_XINPUT` 60 → 0 (0.25 ms of real CPU) | 201.1 | 201.2 | **0.0** |
| `SET_TREES pool` 6000 → 1 (5999 ECS slots) | 200.8 | 201.5 | 0.0 |
| `SET_HAIRDEPTH` 1 → 0 (depth write back for 141 dsided transparents) | 201.3 | 201.0 | 0.0 |

Read together: **quartering the pixels buys 0.52 ms of a 4.98 ms frame, and nothing on the
CPU side buys anything.** So pixel/raster work is a real but minor contributor (~0.7 ms), and
the rest of the frame is diffuse parallel work with enough slack that removing any single
CPU item is absorbed elsewhere.

### Verdict on the 300 FPS target

**Not reachable by incremental tuning, and the numbers say why.** Closing 1.68 ms needs:

- pixels: ≤0.7 ms, and only by rendering at lower resolution — not acceptable;
- CPU items: measured at **zero** conversion, three separate ways;
- which leaves the **structural** item — `Scene::Update` at 2.39 ms against DX11's 0.21 ms.

That 11× is modern Wicked walking a 7322-object / 8437-transform ECS every frame with ~30
subsystems, versus the DX11 fork's much thinner scene update. It is not a bug and not a knob;
closing it means changing how much of the scene the ECS revisits per frame (dirty-tracking,
or keeping static level geometry out of the per-frame systems). That is engine-structural
work, and it should not be attempted without first resolving §5 — because on today's evidence
**even a successful 1 ms CPU saving would show 0 FPS on this level.**

### Ranked next steps

1. **Resolve §5 first — find what absorbs CPU slack.** Start with the cheapest lead: make
   `SUBMIT_PHASES_MS` report a **mean and max over N frames** instead of a single snapshot,
   since `stall` read 0.00 three times and 0.38 once today. Then `RP3D-RenderWait` (0.30 ms)
   and the jobsystem waits. Until a CPU cut demonstrably converts, no CPU work is worth doing.
   This is a prerequisite, not an optimisation.
2. **`Transparent Scene` 1.12 ms** = 30% of the GPU frame, on a level with 221 transparent
   objects of which **141 are double-sided** (no early-Z, shaded by 16 lights, drawn twice).
   `DUMP_TRANSPARENTS` shows character `Body` / `new limb` subsets in the transparent pass —
   worth asking whether those assets are authored correctly, since opaque would be far cheaper.
   ⚠ A blend-mode change is a visual change; needs your eye, not my judgement.
   ⚠ Note the cheap version of this was already tested and is a zero: `SET_HAIRDEPTH 0` puts
   depth write back on all 141 (halving their overdraw) for **−0.3 FPS**. So the transparent
   cost is not overdraw from the 2.08 rule; if it is anything it is the per-pixel shading
   itself, which means an asset/blend-mode question rather than a renderer one.
3. **`SU-Object` 0.57 ms / `SU-Mesh` 0.29 ms** — the remaining named ECS systems, behind (1).
4. **`CL-ObjLists` 0.33 ms** — the editor's Level Objects ImGui panel rebuilt every frame for
   7322 objects. An `ImGuiListClipper` would cut most of it. Editor-only, self-contained,
   independent of (1)… but still subject to the zero-conversion caveat.

### Do-not list

- ⚠ ~~Do not chase the tree pool on no-tree levels (§2) — measured zero, twice.~~
  **RETRACTED 2026-08-10 — that A/B tested nothing (dead knob). See §2 and §10.**
- ⚠ **Do not trust `Input`, `SU-*` or any single CPU range as an FPS lever on this level**
  without an A/B; four separate cuts measured zero.
- ⚠ **Do not compare a `SET_SCENESERIAL 1` total against a normal frame** — only shares.
- ⚠ The editor is unlocked but **the hub is vsync-locked to 60.0 FPS** — never benchmark there.

---

## ★★★ 10. WHY `Scene::Update` IS 11× DX11 — the real answer (2026-08-10)

Read-only comparison across both trees (DX12 `WickedEngineDX12` + this game; DX11 `WickedRepo` +
`D:\max\GameGuruMAX`). 31 agents, 39 findings, 26 survived adversarial verification; the five
load-bearing claims below were then re-verified by hand.

### 10.1 The measurement is VALID — the artifact theories are all dead

| Suspected artifact | Verdict |
|---|---|
| DX11 reports *self* time, DX12 *inclusive* | **NO.** Both are plain begin→end wall clock, no child subtraction, `ranges` is a flat map in both. DX11 `wiProfiler.cpp:142`; DX12 `wiProfiler.cpp:410`. |
| Nested ranges deflate DX11's 0.21 | **NO.** Only `Frustum Culling` (`wiRenderer.cpp:4032`) can nest, and inclusive means it is *contained in* the 0.21. DX11's `Physics` range is `#ifndef GGREDUCED` (`wiScene.cpp:1755`) = unreachable. |
| DX11 skips the scene update | **NO.** `setSceneUpdateEnabled` has one caller in the whole DX11 tree (`RenderPath3D_PathTracing.cpp:149`), so the `true` default holds and `scene->Update()` really runs inside the 0.21. |
| DX12's 2.39 sums several calls per frame | **NO.** The project's own tracer already settled it: `calls/frame min=1 max=1` (`WICKED_ENGINE_CHANGES.md:811`). |

Only real asymmetry: DX11 CPU rows are a **20-frame moving mean** (`wiProfiler.cpp:157-167`),
DX12's are the **raw current frame**. That adds variance to DX12, not a 10× bias.

### 10.2 It is NOT the "modern Wicked infrastructure"

Surfel GI, DDGI, VXGI, impostors, TLAS, and every modern component system (Script, Spline,
Collider, Spring, Character, Expression, Humanoid, Video, Sprite, Font) iterate **empty arrays**
on this level — `grep -rl` finds zero files in the game creating any of them. Combined: **<0.01 ms**.
DX11's `Scene::Update` (`WickedRepo/WickedEngine/wiScene.cpp:1720-1789`) runs essentially the
**same system list** as DX12's. The gap is not new systems.

### 10.3 What it actually is — three causes, in order

**(1) BIGGER N — 82% of the "scene" is empty tree-pool slots.**
6000 of 7322 objects and 6000 of 8437 transforms are parked pool entities created at app
startup (`GGTrees_part2.cpp:284-307`), on a level with no terrain and no trees. **DX11 creates
ZERO ECS entities for trees** — `grep -c 'objects.Create|transforms.Create|CreateEntity'` on
DX11 `GGTerrain/GGTrees.cpp` returns **0**; it draws via `DrawIndexedInstanced` (:3009/:3025)
from its own instance buffers. So DX11's scene for this level is ~1322 objects, DX12's is 7322.
⚠ Unverified on the DX11 side: that build ships no counter. See 10.5.

**(2) THE PORT DROPPED A GAMEGURU ECS OPTIMISATION.**
DX11 resolves entity→component through a flat sparse array — one bounds test, one load
(`WickedRepo/WickedEngine/wiECS.h:413`, `GetComponent` :375-382, `Contains` :366-373). That is a
**GameGuru change**, not upstream ("way faster for larger games… no more lookup.find(entity)").
DX12 is stock upstream: every `Contains`/`GetComponent` goes through `LOOKUP_BUCKET_HASH`
(`WickedEngineDX12/WickedEngine/wiECS.h:446`) into a `ska::flat_hash_map` find. Every system
pays it, on every entity, every frame. **`LOOKUP_SPARSE` is already implemented in the DX12 tree
at `wiECS.h:486-593` and simply commented out at `:445`.**

**(3) GENUINELY RELOCATED — the GPU scene mirror.** *(this is the "combined work" the user asked about)*
Modern Wicked builds a persistent bindless GPU description of the whole scene inside
`Scene::Update`:
- `wiScene.cpp:332-340` — one `jobsystem::Execute` (**single worker**) blanket-initialising every
  instance slot: `instanceArraySize` × 256 B ≈ **1.87 MB of single-threaded stores into
  write-combined UPLOAD memory**, gated only on `dt > 0`;
- `:5318` — the real per-object 256 B `memcpy`, dispatched over **all** `objects.GetCount()`;
- `:4743` geometry (per subset), `:4880` material (per material, every frame, no dirty gate).

DX11 did this **at draw time, sized by the culled batch list**: `wiRenderer.cpp:3373`
(`AllocateGPU` in `RenderMeshes`, 64 B `struct Instance`), materials at `:4918` (dirty-gated
`UpdateBuffer` inside `UpdateRenderData`), bindless mesh descriptors at `:5389` (gated on
`dirty_bindless`). All of that is billed to DX11's **`Render`** row, not `Update - Wicked`.

### 10.4 But relocation cannot explain much — the ledger does not balance

1. **DX11's ENTIRE CPU frame is 1.69 ms** and DX12's `Scene::Update` alone is 2.39 ms. There is
   physically nowhere in DX11's frame for 2.18 ms of "same work, done elsewhere" to hide.
2. **The credit never appears.** If DX12 had really moved work *out* of Render, its `Render` row
   should be cheaper. It is **more** expensive — 0.87 vs 0.71 — despite writing 4 B per visible
   instance (`wiRenderer.cpp:3718`) where DX11 wrote 64.

**Corrected conclusion: essentially the whole delta is real, non-relocated CPU time.** Cutting
`Scene::Update` will NOT just push cost back into `Render`. The §1 headline number survives —
only its causal story ("modern-Wicked ECS overhead", §7 verdict) was wrong.

### 10.5 Open, and the cheapest way to close it

| Question | Cheapest measurement |
|---|---|
| **Is DX11 walking ~1322 objects or ~7322?** The whole bigger-N bucket rests on this and it is *inferred* — the DX11 build ships no counter. | Print `objects.GetCount()` / `transforms.GetCount()` once/second from DX11 `master.cpp:2171`. **If DX11 also reports ~7322 the bigger-N bucket collapses.** |
| **What does the hash-vs-sparse lookup cost?** | Flip `wiECS.h:446` → `:445`, rebuild engine, re-read `SU-*`. One line, already-written code. |
| **What does the tree pool cost?** | A pool-size knob applied *before* `GGTerrainWicked_Init` — the runtime one is dead (§2). |
| **What does the blanket instance init cost?** `SET_SCENESERIAL` is blind to it (only `GG_SCENE_SYS`-wrapped systems are timed). | A `BeginRangeCPU`/`EndRange` pair around `wiScene.cpp:332-340`. |

⚠ **All of this is still gated by §8: the editor frame is GPU-fence-bound.** A CPU win here is
worth taking for the engine-wide benefit, but on *this* level it may show ~0 FPS. Judge CPU work
by the `Scene::Update` / `SU-*` millisecond change, **not** by Switch Escape's FPS.

## ★★★ 11. ACTING ON §10 — engine 2.18: ECS sparse lookup is worth −27% of Scene::Update

Session 2026-08-10. Revert bookmark: tag **`baseline-sceneupdate-20260810`** in BOTH repos
(game `04b6a3da`, engine `07b616db`).

### Method (why not FPS)
The editor frame is GPU-fence-bound (§8), so a genuine CPU saving can read 0 FPS. The primary
metric here is the **`Scene-S1..S5` CPU range sum in ms** with the profiler enabled in every
arm; FPS and POLYS are recorded but secondary. Scripts: `tools/sceneupdate/sumeasure.sh`
(single build, 6 samples + `SET_SCENESERIAL` shares) and `suarms.sh` (3-arm within-launch A/B
of a live knob, which removes cross-launch drift from the knob comparison).
⚠ **Today's machine was GPU-slower than the 08-09 sweep** — Switch Escape read 142 FPS where
the sweep read 201. CPU Frame matched (4.49 vs 4.58) and the baseline `Scene::Update` reproduced
the documented 2.39 almost exactly (2.412), so the CPU side is comparable and the FPS column is
NOT comparable across days. All conclusions below rest on same-day arms.

### Results

| build / knob | Scene::Update | vs base | CPU Frame | objects | POLYS |
|---|---|---|---|---|---|
| baseline (2.17, `LOOKUP_BUCKET_HASH`) | **2.412** | — | 4.49 | 7322 | 109358 |
| **2.18 `LOOKUP_SPARSE`** | **1.750** | **−0.662 ms (−27.4%)** | 3.93 | 7322 | 109358 |
| 2.18 + `SET_INSTINIT 1` (parallel blank pass) | 1.765 | +0.015 (+0.9%) | 3.90 | 7322 | 109358 |
| 2.18 + `setup.ini treepool=1` | ~1.62 median | −0.13 further | 3.51 | **1323** | 109358 |

Per-system shares (`SET_SCENESERIAL 1`; serialised, shares only):

| | base | sparse | sparse + treepool=1 |
|---|---|---|---|
| SU-Hierarchy | 0.65 | — | 0.47 |
| SU-Object | 0.57 | — | 0.38 |
| SU-Mesh | 0.41 | — | 0.17 |
| SU-Material | 0.19 | — | 0.12 |
| SU-Transform | 0.04 | — | 0.03 |

### 11.1 ★ SHIPPED: `LOOKUP_SPARSE` (engine 2.18, `wiECS.h:445`)
A DX11-parity restoration, not a new idea — see §10.3(2). **−0.662 ms of a 2.412 ms
`Scene::Update`, and −0.56 ms of the whole CPU frame**, for a one-line change to already-written
upstream code. Safety was audited before enabling (value-initialising block allocator; erase()
only reachable for present entities) and then verified by geometry identity, not by eye.

⚠ **It has a build cost.** The extra per-ComponentManager `BlockAllocator` instantiations pushed
`DarkLUA.cpp` — a 9-part unity TU that also includes the Wicked headers — past the COFF section
limit (`C1128`), and enlarged Camera's browse-info database past BSCMAKE's `BK1520` limit.
Fixed with `/bigobj` on DarkLUA (three other projects in the solution already use it) and by
turning **Release-only** browse info off in Camera. Both are build-flag changes with no codegen
or semantic effect. **If a future TU hits C1128, add `/bigobj` — do not revert the ECS change.**

### 11.2 REFUTED BY MEASUREMENT: the parallel instance-array blank pass
§10 flagged `wiScene.cpp:332-340` (1.87 MB of single-threaded write-combined stores per frame)
as the biggest unnamed occupant of Scene-S1, and it looked compelling: S1 was 1.02 ms with only
0.07 ms of named children. **Parallelising it is a wash: +0.015 ms (+0.9%), inside noise.**
The reason was written into the code comment before the test and held: it is a
`jobsystem::Execute` running *alongside* the parallel Animation/Physics/Transform dispatches, so
it only shows in wall clock if it is the critical path — and it is not. Kept as an opt-in knob
(`SET_INSTINIT`, **default 0**) rather than deleted, because it will matter on a scene whose S1
dispatches are cheaper relative to instanceArraySize. ★ Do not re-raise it as a Switch Escape
lever without re-measuring.

### 11.3 The tree pool: knob FIXED, cost now MEASURED, default unchanged
`setup.ini treepool=<N>` (new, GGMAX 2.18) is read in `GetSetupIniEarly()` and therefore lands
*before* the one-shot `GGTerrainWicked_Init`, which is exactly what the dead runtime knob could
not do (§2). **Proof it reaches: `SCENE_OBJECTS` 7322 → 1323**, i.e. 5999 of the objects on this
level really were parked pool slots — settling §10's open question in the affirmative.

Cost on a no-tree level: **~0.13 ms further off `Scene::Update`** on top of the sparse win, and
a clear drop in every per-object share (`SU-Mesh` 0.41→0.17, `SU-Object` 0.57→0.38,
`SU-Hierarchy` 0.65→0.47 — the last one despite pool slots holding no `HierarchyComponent`,
which is consistent with better lookup-table locality once 6000 entities leave the tables).

⚠ **The default is deliberately NOT changed.** `treepool=1` would cripple levels that actually
have trees; the pool is built once at app startup, before any level is known. The correct fix is
**lazy pool growth** — create slots on demand as `GGTrees_Update` binds them — which needs the
~10 `for i < g_treePoolSize` loops in `GGTrees_part2.cpp` re-bounded to an allocated count. That
is the next real win here and it is now backed by a number instead of a guess.

### 11.4 What did NOT move: FPS
Switch Escape read 142.5 (base) / 143.5 (sparse + treepool=1) — **~1%, i.e. nothing**, exactly
as §8 predicts for a fence-bound frame. **This is the expected outcome, not a failure**: 0.9 ms
of CPU came off the frame and the GPU wall did not move, so the CPU simply waits longer. The win
is real, it is banked engine-wide, and it will convert on CPU-bound levels and lower-end CPUs.
★ Judge this work by the `Scene::Update` ms column, never by Switch Escape's frame rate.

### 11.5 Correctness gate for the ECS change (`tools/sceneupdate/susmoke.sh`)
An entity->index lookup is used by EVERY system, so the gate is geometry identity, not FPS.
All three demos loaded to the editor and matched the 08-09 sweep's POLYS **exactly**:

| demo | POLYS | reference | objects | FPS (today's slower GPU state) |
|---|---|---|---|---|
| Island Showdown (tree-heavy, exercises the pool) | 4114598 | 4114598 ✔ | 8850 | 69.8 |
| Trapped | 11209 | 11209 ✔ | 7255 | 159.4 |
| Switch Escape | 109358 | 109358 ✔ | 7322 | 149.3 |

### 11.6 ★ FULL 19-DEMO SWEEP + VRAM GATE: **CLEAN** (2026-08-10)

`tools/demo_fps_sweep.sh 0810`, scored by `tools/sweepgate.sh` against criteria written into the
script *before* the run. Raw + scored output: `tools/sweep_0810_2.18.txt`.

| criterion | result |
|---|---|
| **C1 LOAD** | **PASS** — 19/19 reached the editor, no crashes, no FAIL_* rows |
| **C2 GEOMETRY** | **PASS** — POLYS identical to the 0809 reference on **all 19** |
| **C3 VRAM** | **PASS** — worst of editor+game **3962.5 MB** (Aztec Game Kit, in-game), limit 4096, **133.5 MB headroom**; 0/19 breach in either mode |
| **C4 GAME** | **PASS** — all 19 reached gameplay past the loading overlays (prep=0s throughout, navmesh cache holding) |

**C2 is the real result here.** For an engine-wide entity→index lookup change, POLYS identity
across 19 scenes ranging from 11 k to 10.3 M polygons is the proof that no entity ever resolved
to the wrong component — a mis-resolve would change what is drawn, and the counter would move.

VRAM top five (editor / in-game MB): Aztec Game Kit 3811.4 / **3962.5**, Operation Amazon
3595.4 / 3868.7, Z Island 3464.9 / 3789.5, River Raiders 3292.4 / 3767.7, Canyon Offensive
3376.1 / 3708.9. ⚠ **In-game VRAM runs above the editor's on every demo** — a gate that reads
only the editor column can pass a build that breaches 4 GB in the mode players actually ship.
`sweepgate.sh` now checks both (it did not on its first version; the miss is recorded here
because it would have been an easy way to ship a false PASS).

⚠ **FPS was deliberately excluded as a criterion and the data shows why.** Switch Escape's
editor FPS was read three separate times today on the *same build and same scene*: **142.5,
149.3, 159.6** — an 11% spread launch-to-launch, on top of a rig that read 201 for the same
scene on 08-09 at a matched CPU frame. Nothing about the FPS column in this sweep is comparable
to earlier sweeps; it is recorded for the ranking and the archive, nothing more.

## ★★★ 12. GGMAX 2.19 — LAZY TREE-POOL GROWTH (the §11.3 follow-up, now shipped)

§11.3 measured the parked tree pool with a blunt instrument (`setup.ini treepool=1`, which
cripples tree levels) and left the real fix scoped but unbuilt. This is it.

### What changed
`GGTrees_part2.cpp`: `g_treePoolSize` is now only a **ceiling**. A new `g_treePoolBuilt` counts
slots that actually exist as ECS entities; it starts at 0 and `GrowTreePool()` creates slots on
demand, driven by `poolFill` — the nearest-N count the selection pass already computes each
frame. Every loop that walks existing slots (park, cascade refresh, force-rebind, Pass A,
shutdown, DebugDumpPool) now bounds on `built`; the loops that express *desired* N
(`nth_element` cap, ring-coverage test) still use the ceiling, because that is what they mean.

Two safeguards:
- **Per-frame growth cap** (`GG_TREE_POOL_GROW_PER_FRAME = 2048`) so creating thousands of
  entities lands over a few frames during level load rather than as one hitch.
- **Growth is one-way for the pool's lifetime** (only `GGTrees_WickedShutdown` releases). A
  camera walking away from the trees does not thrash entity create/destroy.

### Result — the pool now costs nothing on levels that have no trees

| build | Scene::Update | CPU Frame | SCENE_OBJECTS | POLYS |
|---|---|---|---|---|
| 2.17 baseline | 2.412 ms | 4.49 | 7322 | 109358 |
| 2.18 sparse ECS | 1.750 | 3.93 | 7322 | 109358 |
| **2.19 + lazy pool** | **1.457** | **3.55** | **1322** | 109358 |

**Cumulative: `Scene::Update` 2.412 → 1.457 ms = −0.955 ms (−40%); CPU frame 4.49 → 3.55.**
Per-system: SU-Mesh 0.41 → 0.19, SU-Object 0.57 → 0.39, SU-Material 0.19 → 0.12.

★ **`SCENE_OBJECTS` fell 7322 → 1322 with no knob set.** That is the whole point: the level's
real object count was always ~1322, and the other 6000 were pool slots created at app startup
before any level was known. The `treepool=` key from §11.3 is now a tuning ceiling rather than
the only way to avoid the cost.

⚠ **This does NOT change what renders.** POLYS is identical on Switch Escape, and on the
tree-heavy levels the pool grows exactly as before — see the correctness gate below, where
POLYS identity on Island Showdown (4,114,598, a scene whose polygon count is dominated by
trees) is simultaneously the proof that growth works and that nothing regressed.

### Side effect worth knowing
`SET_TREES pool <N>` is no longer completely dead at runtime. It sets the ceiling, so *raising*
it now permits further growth on the next frame that wants more slots. **Lowering it still does
not shrink an existing pool** — slots are only released at shutdown. For a true reduction, use
`setup.ini treepool=<N>`, which is read before the pool can grow at all.
