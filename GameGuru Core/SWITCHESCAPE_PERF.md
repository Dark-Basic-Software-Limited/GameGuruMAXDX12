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

### 12.1 2.19 hub sweep + VRAM gate: **CLEAN** (all 19)

`demo_fps_sweep.sh 0810b`, scored by `tools/sweepgate.sh`. Raw + scored:
`tools/sweep_0810b_2.19.txt`.

| criterion | result |
|---|---|
| **C1 LOAD** | **PASS** — 19/19 reached the editor |
| **C2 GEOMETRY** | **PASS** — POLYS identical to the 0809 reference on all 19 |
| **C3 VRAM** | **PASS** — worst of editor+game **3944.8 MB** (Aztec Game Kit in-game), **151.2 MB headroom** (was 133.5 on 2.18) |
| **C4 GAME** | **PASS** — all 19 reached gameplay |

C2 matters twice over here: it proves the ECS change still resolves every entity correctly, AND
that lazy growth reaches full pool size wherever trees exist — a pool that failed to grow would
have collapsed the tree-dominated poly counts (Island Showdown 4,114,598, Foggy Forest
10,195,894, Canyon Offensive 8,816,163 all unchanged).

**VRAM: essentially unchanged, as expected.** Mean editor delta −19.6 MB, mean in-game +2.4 MB,
with individual demos swinging ±84 MB in both directions — that is driver-usage noise, not a
saving. The only *structural* VRAM effect of dropping 6000 objects is the GPU instance array
(6000 × 256 B ≈ 1.5 MB), and nothing in this table is precise enough to see it. **Do not report
this as a VRAM win.** Worst case did improve 3962.5 → 3944.8 MB, which is worth having but is
inside the same noise band.

⚠ **Editor FPS was higher on 16 of 19 demos, and I am NOT claiming that as a result.** A
consistent sign across demos looks compelling, but these are two sequential whole-sweep runs, so
any systematic drift in the machine's state between them (thermal, clocks, background load)
produces exactly that pattern — and this rig demonstrably drifts: Switch Escape read 142.5,
149.3 and 159.6 across three launches of the SAME build earlier in the day. The evidence for
this work is the direct CPU measurement (`Scene::Update` 2.412 → 1.457 ms with the profiler on),
not a difference of two noisy FPS columns.

## ★★★ 13. SU-Hierarchy CONFIRMED as load imbalance (engine 2.20 instrument, 2026-08-10)

### 13.1 The DX11 object count — no code change is needed, and none was made
⚠ `D:\max\GameGuruMAX` and `D:\max\WickedRepo` are **read-only reference sources**, so the
"print `objects.GetCount()` from DX11 `master.cpp:2171`" plan in §10.5 was **not** carried out —
it would have meant editing and building that tree.

**It is unnecessary.** The DX11 editor **already prints these numbers**, in the very performance
panel this whole study started from — `GameGuru/Source/M-GridEdit.cpp:11840-11841` (and the
identical block at `M-TerrainNew.cpp:10679-10680`), immediately above the
`wiProfiler::GetProfilerData()` rows:

```cpp
ImGui::Text("Scene Transforms: %d", (int)pScene->transforms.GetCount());
ImGui::Text("Scene Hierarchy: %d",  (int)pScene->hierarchy.GetCount());
```

So the DX11 half of the comparison is a **read**, not a build. DX12 now prints the same two
quantities under matching labels (`SCENE_TRANSFORMS`, new `SCENE_HIERARCHY`) so the panels line
up. ⚠ I cannot drive the DX11 editor — it ships no automation harness — so the DX11 values
still need one human glance at that panel on Switch Escape.

**DX12 on Switch Escape, post-2.19:**

| | DX12 (2.19) | DX11 (read off its perf panel) |
|---|---|---|
| Scene objects | **1322** | — |
| Scene Transforms | **2437** | **?** ← the number to read |
| Scene Hierarchy | **1995** | **?** |

★ **A cross-check that strongly supports the §10.3(1) bigger-N story:** the pre-lazy-pool build
reported **8437** transforms and the post-lazy-pool build reports **2437**. 8437 − 2437 = **6000
exactly** — the tree pool, to the entity. The "6000 of the transforms are parked pool slots"
claim is now arithmetic rather than inference. What remains unconfirmed is only whether DX11's
loader produces the same ~2437 for the same level.

### 13.2 ★ SU-Hierarchy is a SCHEDULING problem, not a per-entity cost
New engine instrument (2.20) harvests the DFS's existing `processed` cycle-guard counter — one
relaxed CAS and one fetch_add **per root**, nothing in the hot loop — and reports:

```
HIER: roots=435  maxSubtree=1283  visited=1995  imbalance=0.643
```

**One subtree walks 1283 of the 1995 nodes — 64.3% of all the work in a single job**, while the
other 434 jobs split the remaining 36% and then idle at the stage's `Wait`. The fast path
Dispatches one job per subtree ROOT (`wiScene.cpp`, groupsize 8), so the system's wall clock is
the biggest single subtree, not the total.

That matches the measured shape exactly: `SU-Hierarchy` **0.47 ms** against `SU-Transform`
**0.02 ms** over a *larger* N (2437) with zero lookups. The per-node work is not the problem —
the critical path is.

**Ceiling if it were balanced:** the critical path would fall from 1283 nodes to ~1995/workers.
On this pool that is roughly 250 nodes, i.e. **~0.47 → ~0.1 ms, a further ~0.35 ms** off
`Scene::Update`. Treat that as an upper bound, not a promise — DFS carries parent state down the
chain, so splitting a subtree means either seeding sub-roots with their ancestors' accumulated
world matrix (cheap: the parent's final world is already computed) or a two-phase level-order
pass. Both are real engine work and neither is written.

⚠ **NOT ATTEMPTED IN THIS SESSION, deliberately.** The diagnosis is solid and cheap; the fix is
a change to the transform-propagation order, which is exactly the kind of thing that produces
subtle one-frame parenting glitches. It deserves its own build + POLYS gate + sweep, not the
last fifteen minutes of a session.

## ✗ 14. THE SU-HIERARCHY BALANCE FIX WAS BUILT, MEASURED, AND REJECTED (2026-08-10)

§13 predicted ~0.35 ms from balancing the hierarchy jobs. **It was implemented, it worked
mechanically and perfectly, and it made `Scene::Update` WORSE. Reverted.**

### What was built (engine 2.21, now reverted)
`g_hier_split`: any subtree over 128 nodes is expanded into its children — recursively — with
the nodes above the split points resolved serially in BFS order first. Work items carry their
own seed (parent's resolved world + layer mask), so a split sub-root is walked by exactly the
same job body as an original root.

**The mechanism did precisely what it was designed to do:**

| | split OFF | split ON |
|---|---|---|
| dispatch items | 435 | **1275** |
| largest single subtree | 1283 nodes | **13** |
| **imbalance** | **0.643** | **0.007** |
| POLYS | 109358 | **109358** (identical, every sample) |

### The result: +0.260 ms WORSE (+16.6%)
Three arms, within one launch, 10 samples each, on Switch Escape:

| arm | Scene::Update mean | sd | CPU frame |
|---|---|---|---|
| A1 split OFF | **1.570** | 0.124 | 3.40 |
| B split ON | **1.825** | 0.136 | 3.60 |
| A2 split OFF | **1.560** | 0.183 | 3.38 |

**Knob delta +0.260 ms; A-vs-A2 control drift 0.010 ms.** The regression is 26× the drift —
this is not noise, it is a real and sizeable loss.

### ★ Why the prediction was wrong — the lesson, and it was already written down
The ~0.35 ms estimate came from **`SU-Hierarchy` measured under `SET_SCENESERIAL`**. That mode
**removes cross-system overlap** — WETEST's own row for it says so, and warns "compare the SU-*
shares against EACH OTHER, never the total against a normal frame." In a normal frame Hierarchy
is dispatched *alongside* Mesh and Material inside stage S2 and closed by one Wait, so its
0.47 ms was largely **hidden underneath them**. Shortening a critical path that was not the
stage's binding constraint buys nothing — while the machinery to shorten it costs:
- an O(N) subtree-size pass (~4000 hash ops) in the snapshot job that this system **waits on**;
- a serial work-list build of 1275 items, each carrying a 64-byte matrix by value;
- ~3× the job count (1275 vs 435) and its dispatch overhead.

★ **RULE: a serialised per-system number is a SHARE, not a saving. Never size a real-frame
optimisation from a `SET_SCENESERIAL` reading — confirm the system is the stage's critical path
in a NORMAL frame first.**

### ⚠ A second trap, caught mid-test
The first A/B computed the subtree sizes **unconditionally**, so the "off" arm paid the new cost
too — the knob looked nearly free (+0.020 ms) because both arms carried the tax. Gating the size
pass on the flag turned +0.020 into the true **+0.260**. ★ **When adding a knob, verify the OFF
path is byte-identical to before, or the A/B measures the wrong thing.**

### Disposition
**Reverted to 2.20** (engine `b4ea718a`, game `75169d4c`) — the state that is swept, gated and
tagged `baseline-lazytreepool-20260810`. Nothing default-off was kept: known-worse machinery in
a hot engine path is a liability, and the A-arms here (1.565) also read above 2.20's 1.457 on
this level, so the refactor likely taxed the default path as well.

**SU-Hierarchy is therefore CLOSED as a target, not solved.** The imbalance is real and now
proven harmless on this scene. If it is ever revisited, the prerequisite is a measurement
showing Hierarchy is the S2 critical path **in a normal frame** — e.g. by shortening Mesh and
Material first and seeing whether S2 stops falling.

## ★★★ 15. THE DX11 SCENE POPULATION — READ OFF THE PANEL (2026-08-11, user-supplied)

§13.1 said the DX11 numbers needed one human glance at its performance panel. Here they are,
both builds on `switch escape.fpm`, DX12 on the post-2.19 build:

| counter | DX11 | DX12 | ratio |
|---|---|---|---|
| Scene **Meshes** | **316** | **1288** | **4.08×** |
| Scene **Materials** | **317** | **1250** | **3.94×** |
| Scene **Transforms** | **1176** | **2437** | **2.07×** |
| Scene **Hierarchy** | **681** | **1995** | **2.93×** |
| CPU Frame | 1.49 ms (peak 3.39) | 3.28 ms | +1.79 |
| FPS at capture | 337.8 | 219.3 | — |

⚠ **MY PREDICTION WAS WRONG.** §13.1 expected DX11 to report ~2437 transforms — i.e. that once
the 6000 tree-pool slots were gone, the two engines would be walking the same population. DX11
reports **1176, less than half**. The tree pool was real and worth removing, but it was **not the
whole of the bigger-N story** — DX12 still carries ~2× the transforms, ~3× the hierarchy links
and **~4× the meshes and materials** for the same level, with the pool already excluded.

★ **NEW TOP LEAD, and it is bigger than anything left on the list.** The mesh/material multiplier
is the striking one. Both engines sit at roughly 1:1 mesh:material (316:317 and 1288:1250), so
this is not DX12 failing to *share* materials — DX12 simply has ~4× as many mesh entities, each
bringing its own material. Those two counters feed `SU-Mesh` and `SU-Material` directly, and
`RunMeshUpdateSystem`/`RunMaterialUpdateSystem` iterate them every frame.
**Hypothesis to test (NOT yet verified): DX12 creates a MeshComponent per LIMB where DX11 creates
one per object with subsets, and/or DX11 reuses one MeshComponent across repeated props where
DX12 clones per placement.** Settle it by dumping mesh names/owners on both sides.

Validity notes: the two screenshots are from different camera positions, but all four counters
are whole-scene totals and camera-independent, and both title bars read `switch escape.fpm`.
DX11's panel prints `avg (peak)` per row — its `Update - Wicked: 0.22 ms (0.52 ms)` matches the
0.21 recorded in §1, confirming that row was the 20-sample average.

The CPU gap has also narrowed from the §1 measurement of +2.89 ms to **+1.79 ms**, which is
consistent with the ~0.95 ms of `Scene::Update` removed by 2.18/2.19 plus the 2.15 XInput fix —
though across different machine states, so treat that as corroboration, not a measurement.

## ★★★ 16. THE FLOOR IS WICKED'S NATIVE TERRAIN — root-caused (2026-08-11)

§15's "4× meshes" hypothesis (per-limb duplication of level props) is **REFUTED**. The user
deleted all level objects in both builds and read the panels; subtracting empty from full splits
the fixed floor from the content:

| | DX11 full/empty/**content** | DX12 full/empty/**content** |
|---|---|---|
| Meshes | 316 / 224 / **92** | 1288 / 1254 / **34** |
| Materials | 317 / 225 / **92** | 1250 / 1216 / **34** |
| Transforms | 1176 / 573 / **603** | 2437 / 1894 / **543** |
| Hierarchy | 681 / 300 / **381** | 1995 / 1672 / **323** |

**The content columns are comparable and DX12 is LOWER on all four.** The entire multiplier is
the empty-level floor: **+1030 meshes, +991 materials, +1321 transforms, +1372 hierarchy.**

### The cause, with an exact independent cross-check
**DX12 runs Wicked's native `wi::terrain` generator; DX11 has no such thing.** Verified:
`D:\max\WickedRepo\WickedEngine` contains **no `wiTerrain.cpp/.h`** and its `wiScene.h` has **zero**
`terrains` references. DX11's terrain is `GGTerrain.cpp` (11,707 lines) drawing a custom clipmap
from its own buffers — **0 ECS entities**. Same for its trees and grass.

`wi::terrain` mints one full ECS entity per chunk (object + mesh + material + transform, attached
to a chunk group) and one empty `props_entity` child per near chunk:

| creator | file:line | count | mesh | mat | xform | hier |
|---|---|---|---|---|---|---|
| terrain chunk entity | `wiTerrain.cpp:1108/1123/1131/1140` | **841** | 841 | 841 | 841 | 841 |
| `props_entity` (empty scaffold) | `wiTerrain.cpp:1413-1425` | **441** | 0 | 0 | 441 | 441 |
| chunk group + surface materials | `wiTerrain.cpp:626-634`, `:593-598` | 5 | 0 | 4 | 0 | 5 |
| GGTrees LOD library 38×3 | `GGTrees_part2.cpp:241-277` | 114/70 | 114 | 70 | 0 | 0 |
| brush cursor decal | `GGTerrainWicked.cpp:2347-2366` | 1 | 0 | 1 | 1 | 0 |
| **accounted** | | | **955** | **916** | **1283** | **1287** |
| **target** | | | 1030 | 991 | 1321 | 1372 |
| **unexplained** | | | 75 | 75 | 38 | 85 |

★★ **The counts are pinned by an EXACT cross-check.** `terrain.generation = 14`
(`GGTerrainWicked.cpp:2529`) → (2·14+1)² = **841**; `prop_generation = 10` (`wiTerrain.h:455`) →
(2·10+1)² = **441**. So the chunk-group subtree must hold 1 + 841 + 441 = **1283** — and the
`HIER` instrument built for §13 independently measured **`maxSubtree=1283`**. Two unrelated
routes to the same number; the model is not a guess.

**Verified as ZERO, do not re-chase:** the tree pool (lazy since 2.19), the 256 tree-chunk shadow
proxies (`validCount == 0 → return`), engine per-chunk grass (`SetGrassEnabled(false)`,
`GGTerrainWicked.cpp:2526`, corroborated by `SCENE_HAIRS: 0`), and engine terrain props
(`terrain.props` never populated). ⚠ GG **merged grass** is 0 only because Switch Escape has
none — on a grassy level it becomes a first-rank contributor.

### Why this matters more than the raw numbers
It is a **fixed per-level cost**, so it hurts small levels most — and it is 64% of the hierarchy
nodes `SU-Hierarchy` walks every frame (1283 of 1995), 441 of which are **empty placeholders
with no children and no pixels**. It also retro-explains §14: the hierarchy work I tried to
balance is overwhelmingly terrain scaffolding.

### Ranked, and the top one is nearly free
1. **`terrain.prop_density = 0`** beside the other params at `GGTerrainWicked.cpp:2526-2531` —
   **−441 transforms and −441 hierarchy, zero rendering change** (`terrain.props` is never
   populated, so these nodes draw nothing). Blocks creation at `wiTerrain.cpp:1420` and the
   existing epsilon branch at `:936-939` retro-deletes any already made. ⚠ Check no load path
   restores a serialized Terrain over it (`wiTerrain.cpp:2741`). **Best cost:risk on the list.**
2. **Lazy GGTrees LOD library** — −114 meshes, −70 materials on treeless levels. Same shape as
   the 2.19 lazy pool. ⚠ `BuildTreeMesh` calls `CreateRenderData()`, so deferring moves 114 GPU
   buffer creations to first-tree-sighting; prebuild at level load once `pAllTrees[]` is known.
3. **`terrain.generation` 14 → 12/10** — the ONLY lever that moves the mesh/material columns
   (−216/−400 of each counter). ⚠ Shortens terrain extent; must go through the DX11 parity
   baseline and the POLYS gate. Do not ship blind.

### Open
The ~75-mesh residual is unexplained, and part of it may be a **leak** rather than a design cost:
DX11 freed 92 meshes on delete-all, DX12 freed 34, and Wicked's `Entity_Remove` on an object does
not necessarily destroy the `MeshComponent` it referenced. **Cheapest test, zero code:** capture
`SCENE_*` + `HIER` on a delete-all level and on a freshly created blank level. Identical → no
leak. Different → that difference IS the leak, and its size.

## ★★★ 17. LEAK CHECK: A REAL ONE, AND IT IS THE TREE POOL (2026-08-11)

§16 suspected a ~75-mesh leak. The probe found something much larger, and it indicts my own 2.19
design. No harness command deletes all objects, so instead of a delete-all comparison the probe
loads the SAME level two ways (`tools/sceneupdate/suleak.sh`) — which tests the operation that
actually happens in use:

| Switch Escape | FRESH (loaded first) | AFTER Island Showdown | delta |
|---|---|---|---|
| SCENE_OBJECTS | 1322 | **7401** | **+6079** |
| SCENE_TRANSFORMS | 2437 | **8516** | **+6079** |
| SCENE_MESHES | 1288 | 1367 | +79 |
| SCENE_MATERIALS | 1250 | 1267 | +17 |
| POLYS | 109358 | 109358 | **0** |

Objects and transforms move in exact lockstep at +6079 and POLYS does not move at all, so none of
it draws. `DUMP_TREEPOOL` on Switch Escape *after* Island Showdown names it outright:

```
POOL built=6000 ceiling=6000 bound=6000 renderable=0 numTotalTrees=400000
```

★★ **The pool is not merely still built — all 6000 slots are still BOUND to the previous level's
trees, and `numTotalTrees=400000` shows Island Showdown's tree array is still resident.** On a
level with no trees at all.

⚠ **This is a direct consequence of the 2.19 design, and it caps that win.** GrowTreePool was
made deliberately one-way ("growth is one-way for the pool's lifetime, so a camera walking away
does not thrash entity create/destroy") — correct for camera movement, wrong for level changes.
**It is NOT a regression** (before 2.19 the pool was 6000 on every level unconditionally), but
the lazy-pool saving only survives until the session visits its first tree level, after which
every subsequent level pays the full 6000 objects + 6000 transforms again.

**Fix, NOT yet implemented:** release pool slots on LEVEL CHANGE rather than never. Level change
is a natural, infrequent boundary — nothing like the per-frame thrash the one-way rule was
protecting against. `GGTrees_WickedShutdown` already does exactly the right teardown and is
reachable only from `GGTerrainWicked_Shutdown`, which has zero callers; wiring a trim into the
level-load path is the smallest correct change. Gate it the same way: POLYS identity on a
tree level, and re-run `suleak.sh` — a fixed build must show FRESH == AFTER.

⚠ The residual +79 meshes / +17 materials is a genuine second, smaller leak (retained mesh
entities whose objects were removed) and is still unexplained.

## ★★ 18. GGMAX 2.22 — kill the engine's empty per-chunk prop scaffolding

`wi::terrain` creates one empty `props` child entity per chunk within `prop_generation`
(default 10 → 441) purely to parent scattered props under — and **GameGuru never populates
`terrain.props`** (grep for `.props` / `Prop` / `props.push_back` across `Guru-WickedMAX`
returns nothing). So all 441 were childless nodes drawing no pixels, walked every frame.

`terrain.prop_density = 0.0f` at `GGTerrainWicked.cpp:2526`. This both blocks creation (the
`prop_density > 0` gate at `wiTerrain.cpp:1420`) and retro-deletes any already made (the
`prop_density_current` mismatch branch at `:936-939`), so it is correct regardless of init order.

| Switch Escape | before | after |
|---|---|---|
| SCENE_TRANSFORMS | 2437 | **1996** (−441) |
| SCENE_HIERARCHY | 1995 | **1554** (−441) |
| `HIER maxSubtree` | 1283 | **842** (= 1 + 841 chunks, exactly) |
| `SU-Hierarchy` (serialised) | 0.47 | **0.31** (−34%) |
| `Scene::Update` | 1.577 | **1.323** (−0.25 ms) |
| POLYS / objects / meshes | 109358 / 1322 / 1288 | **identical** |

★ `maxSubtree` landing on exactly 842 = 1 + 841 is the model from §16 confirming itself: remove
the 441 props and the chunk subtree is precisely the chunk group plus its 841 chunks.

⚠ **Honesty on the ms figure:** 1.577 → 1.323 is cross-launch, and launch-to-launch drift on
this build has been measured at 0.12 ms (1.457 vs 1.577), so the −0.25 ms is about 2× the drift
— real, but the *structural* evidence is what is certain: −441 transforms, −441 hierarchy and
maxSubtree 1283 → 842 are exact counts, not statistics.

Correctness gate (POLYS identity, incl. terrain-heavy levels since this touches terrain only):
Island Showdown 4114598 ✔ · Trapped 11209 ✔ · Switch Escape 109358 ✔, objects unchanged on all.

### 18.1 2.22 hub sweep + VRAM gate: **CLEAN** (all 19)
`demo_fps_sweep.sh 0811`, scored by `tools/sweepgate.sh`. Raw + scored:
`tools/sweep_0811_2.22.txt`.

| criterion | result |
|---|---|
| **C1 LOAD** | **PASS** — 19/19 reached the editor |
| **C2 GEOMETRY** | **PASS** — POLYS identical to the 0809 reference on all 19 |
| **C3 VRAM** | **PASS** — worst of editor+game 3973.4 MB (Aztec GK in-game), 122.6 MB headroom |
| **C4 GAME** | **PASS** — all 19 reached gameplay |

⚠ **Worst-case VRAM headroom moved the WRONG way: 151.2 → 122.6 MB** (Aztec GK in-game
3944.8 → 3973.4). Mean deltas are +16.2 MB editor / −8.3 MB in-game, i.e. both directions, and
the established sweep-to-sweep noise band on this measurement is ±84 MB — so this is almost
certainly noise, not a cost of removing 441 empty entities (which cannot plausibly *add* VRAM).
**But it is the closest to the 4 GB limit any sweep has run**, so it is recorded rather than
waved away, and Aztec Game Kit should be watched on the next sweep.

## ★★★ 19. GGMAX 2.23 — tree-pool retention across level change: FIXED

§17 measured the pool surviving a level change (+6079 objects/transforms on a treeless level
loaded after a tree level). Fixed and verified in both directions.

**Cause.** The `draw_enabled` early-return in `GGTrees_WickedUpdate` hides pool slots and
returns — it never releases them. `bound=6000` on a treeless level was the diagnostic: Pass A
evicts stale bindings, so bindings surviving proved Pass A never ran, i.e. the update was taking
the park branch. Underneath that, 2.19's `GrowTreePool` is deliberately one-way — right for
camera movement, wrong for level changes.

**Fix.** `ReleaseTreePool()` destroys every built slot and drops all bindings, called from the
park branch after `GG_TREE_POOL_PARK_RELEASE_FRAMES` (600) consecutive parked frames. A frame
threshold rather than a level-change hook because `draw_enabled` is ALSO cleared transiently
during terrain regen and there is no single reliable "level changed" signal at that point; a
sustained park means the trees are gone for good, a regen park is short, and releasing a few
seconds late costs nothing because the slots are already invisible. The per-type tree
meshes/materials (114 + 70) are deliberately KEPT so a later tree level does not rebuild them.

**Verified, both directions, one session** (`tools/sceneupdate/suleak.sh`, now with a regrow arm):

| Switch Escape | before 2.23 | after 2.23 |
|---|---|---|
| objects FRESH → AFTER Island Showdown | 1322 → 7401 (**+6079**) | 1322 → **1401** (+79) |
| transforms | 2437 → 8516 | 1996 → **2075** |

| regrow arm — back to Island Showdown AFTER a release | |
|---|---|
| POLYS | **4114598** (exact reference) |
| objects | 8850 (= fresh load) |
| pool | `built=6000 bound=6000 renderable=6000` |

The residual +79 objects / +79 meshes / +17 materials is the separate smaller leak, unchanged
and still unexplained.

⚠ **Correction to §17:** `DUMP_TREEPOOL`'s `numTotalTrees=400000` is a COMPILE-TIME CONSTANT
(`GGTrees_part0.cpp:131`, the capacity of `pAllTrees[]`) and always reads 400000. It is NOT
evidence that the previous level's tree data was retained, as §17 implied. The real evidence was
`built=6000 bound=6000`, which stands.

### 19.1 2.23 hub sweep: **CLEAN** (all 19)
`tools/sweep_0811b_2.23.txt`. C1 LOAD 19/19 · C2 GEOMETRY POLYS identical on all 19 ·
C3 VRAM worst 3961.5 MB (Aztec GK in-game), **134.5 MB headroom** · C4 GAME all 19 reached play.
★ The §18.1 headroom dip (151.2 → 122.6) did NOT persist — it is back to 134.5, confirming it
was sweep-to-sweep noise rather than a cost of 2.22, exactly as recorded at the time.

---

## THE DAY IN ONE TABLE (2026-08-10 / 11, Switch Escape editor)

| build | change | Scene::Update | swept |
|---|---|---|---|
| 2.17 | baseline | 2.412 ms | — |
| 2.18 | ECS sparse lookup (DX11-parity restoration) | 1.750 | CLEAN |
| 2.19 | lazy tree-pool growth | 1.457 | CLEAN |
| 2.22 | kill 441 empty prop scaffolding nodes | **1.323** | CLEAN |
| 2.23 | release the pool on level change | 1.323 | CLEAN |

**−45% of `Scene::Update`, POLYS bit-identical on all 19 demos at every step**, and after 2.23
the saving persists across level changes instead of evaporating at the session's first tree level.

**Measured and REJECTED along the way** (recorded so they are not retried): the parallel
instance-array init (a wash, §11.2) and the SU-Hierarchy load-balance split (worked perfectly,
made things 16.6% worse, §14).

## ★★ 20. GGMAX 2.23b — the residual level-change leak, named and closed

§19 left +79 objects / +79 meshes / +17 materials retained across a level change. **Named by
diffing dumps, not by theorising** (`tools/sceneupdate/suleakname.sh`, new):

```
pool dump, Switch Escape FRESH : proxyChunks=0
pool dump, Switch Escape AFTER : proxyChunks=256
all 17 extra materials UNNAMED · ORPHAN_TOTAL 0 in both arms
```

**It was the merged billboard FAR-SHADOW PROXY chunks built for the previous level's trees.**
`GGTrees_SetShadowProxiesVisible` only calls `SetRenderable(false)` — a park hides them and
nothing removes them; the only teardown is in `GGTrees_WickedShutdown`, whose caller has no
callers. Exactly the same structural gap as the pool itself.

The arithmetic matched to the entity: **79 chunks that actually held trees** → 79 proxy objects
+ 79 proxy meshes; **17 distinct tree types present** → 17 billboard shadow materials.

The two supporting signals mattered as much as the counts: all 17 extra materials were UNNAMED
and `ORPHAN_TOTAL` was 0 in both arms, which ruled out stray level props and pointed at
internally-created entities.

**Fix:** `ReleaseTreePool()` also frees every proxy chunk, clears the proxy vectors and removes
the per-type `g_shadowProxyMaterial` entities. They must go with the pool — a proxy describes
where the PREVIOUS level's trees cast far shadows, so keeping it is stale data, not just wasted
ECS work. The per-type tree meshes/materials stay (level-independent assets).

| Switch Escape, FRESH vs AFTER Island Showdown | before 2.23b | after |
|---|---|---|
| objects | +79 | **0** |
| meshes | +79 | **0** |
| transforms | +79 | **0** |
| materials | +17 | **+1** |
| POLYS | identical | identical |

Regrow arm: back to Island Showdown after a release → POLYS 4114598, objects 8850,
`built=6000 bound=6000 renderable=6000 proxyChunks=256`. Proxies and trees both rebuild.

⚠ The residual **+1 material / +1 hierarchy** is NOT part of this leak — it is the long-known
player-start marker residue already recorded in WETEST's `DUMP_MATERIALS` row ("known: +3 per
reload = the start-with-ghost-human marker"). Bounded, pre-existing, deliberately left.

### 20.1 2.23b hub sweep: **CLEAN** (all 19) — the fifth consecutive clean sweep
`tools/sweep_0811c_2.23b.txt`. C1 19/19 · C2 POLYS identical on all 19 · C3 VRAM worst
3961.1 MB in-game (**134.9 MB headroom**) · C4 all 19 reached gameplay.

**LEVEL CHANGES ARE NOW CLEAN.** `Scene::Update` 2.412 → 1.323 ms (−45%) on Switch Escape, and
the saving persists across level changes instead of being handed back at the session's first
tree level.

## ★★★ 21. GGMAX 2.24 — tree-type assets built on demand, per level (and the first REAL VRAM win)

`GGTrees_WickedSetup` built all 38 tree types × 3 LOD meshes + materials at APP STARTUP — 114
meshes + 70 materials, before any level was known (§16's biggest single non-terrain floor item).

`EnsureTreeType(t)` builds one type on demand. **The driver is the spatial-grid rebuild's
`passes` filter**, which already walks every tree instance whenever a level's tree data changes,
so each level realises exactly the types it places, once, at load. `BindTreeSlot` and the
shadow-proxy build call it as safety nets; setup is now validation only. `ReleaseTreeTypes` runs
from `ReleaseTreePool`, because per-level assets kept across a level change would be the same
retention bug as §19/§20.

### Result — and tree levels save too, which the scoped estimate missed

| | meshes | materials | POLYS |
|---|---|---|---|
| Switch Escape (treeless) | 1288 → **1174** (−114) | 1250 → **1180** (−70) | identical |
| Island Showdown (trees) | 2410 → **2344** (−66) | 2362 → **2322** (−40) | **4114598 identical** |

Island Showdown builds **48 of 114 meshes = 16 of the 38 types**. The estimate in §16 was
"−114 on treeless levels"; the per-level design also saves ~58% of the library on tree levels.

### ★ THE FIRST REAL VRAM WIN OF THIS CAMPAIGN

| 2.23b → 2.24 | mean | median | range | direction |
|---|---|---|---|---|
| editor VRAM | **−74.7 MB** | −64.0 | −192 … −15 | **19/19 negative** |
| in-game VRAM | **−68.8 MB** | −62.5 | −176 … −31 | **19/19 negative** |

Worst-case headroom **134.9 → 183.1 MB** (Aztec GK in-game 3961.1 → 3912.9).

⚠ **Why this one IS a result when §18.1's was not.** The earlier ±84 MB swings were
BIDIRECTIONAL and had no mechanism, so they were correctly called noise. This is unidirectional
on all 19 demos in both columns AND has a mechanism — tree meshes carry real GPU geometry, and
the build now creates 58-100% fewer of them. Sign test alone on 19/19 is decisive; the mechanism
makes it causal rather than coincidental.

### 21.1 2.24 hub sweep: **CLEAN** — sixth consecutive
`tools/sweep_0811d_2.24.txt`. C1 19/19 · C2 POLYS identical on all 19 · C3 VRAM worst 3912.9 MB
in-game (183.1 MB headroom) · C4 all 19 reached gameplay.

⚠ **NOT MEASURED: the load-time hitch.** Moving up to 48 mesh builds (`CreateRenderData` → GPU
buffers) from app startup into the level-load grid rebuild is uninstrumented. It lands during a
loading screen and is fewer meshes than it replaced, but it is not proven. **If a load-time hitch
is ever reported on a tree level, look here first** (`EnsureTreeType` via the `passes` filter).

## ★★★ 22. GGMAX 2.25 — the terrain chunk RING, sized against the map for the first time

`terrain.generation` (`GGTerrainWicked.cpp`) is the radius, in chunks, of the wi::terrain ring:
`(2N+1)^2` chunk entities, each a mesh + material + transform + hierarchy node that
`Scene::Update` walks every frame. It shipped at **14 = 29x29 = 841 chunks**. §16 named this ring
as the DX12 entity floor; this section is the first time anyone measured what it is *for*.

**This change is a different KIND from 2.22/2.23/2.24.** Those removed things that drew nothing —
empty prop scaffolding, a retained tree pool, orphaned shadow proxies — so POLYS *had* to stay
bit-identical and that was the correctness gate. `generation` is a **draw-distance setting**: the
outer rings are scenery. So POLYS stops being an identity gate here and becomes the *visibility
detector* — POLYS unchanged means the rings removed were not drawing at that camera.

### 22.1 The arithmetic nobody had written down
One chunk spans `(chunk_width-1) * chunk_scale` = `66 * 80` = **5280 units**, so the ring reaches
`N * 5280`. ⚠ The two source comments at the assignment said "~10560 units/chunk" and "cover
~147840 units each direction" — both quote the FULL SPAN as a radius, i.e. 2x the truth. Fixed.

**1 unit = 1 inch** (`GGTerrain_MetersToUnits`). New instrument **`TERRAIN_RING`** in
`GET_PERF_DATA` prints `gen` / `chunks` / `ringMax` / `chunkU` / **`viewM`** / `centreToCam` /
`mapHalfM`.

★★ **`gen` is a VIEW DISTANCE, not map coverage — and that is the whole point.** GG sets
`SetCenterToCamEnabled(true)`, and the engine recomputes `center_chunk` from `camera.Eye` every
frame (`wiTerrain.cpp:776-780`), so **the ring travels with the camera**. Terrain always extends
`viewM` in every direction from wherever the player is, whatever the map size.
⚠ See §22.7 — the first version of this instrument got that wrong and it produced a false alarm.

| demo | mapHalf | shipped gen | chunks | viewM |
|---|---|---|---|---|
| Island Showdown | 250 m | 14 | 841 = ringMax | 1878 m |
| Switch Escape | 1270 m (default map) | 14 | 841 = ringMax | 1878 m |

★ Island Showdown's `editable_size` read 9842 against the 9842.5 recorded independently in
`TERRAINPORT.md:448` — that match is what validated the instrument was reading the right variable.

★ **The full ring is always built.** `chunks == ringMax` on both demos with a parked camera. The
"settles ~650-700 of the 841" note at `GGTerrainWicked.cpp` initial-build turbo is wrong for a
parked camera (annotated in place, not deleted — it may still hold for TESTPRO1 after travel).
A camera that has *moved* can read ABOVE ringMax (898 measured) because removal lags creation by
`removal_threshold = generation + 2 + gg_removal_margin(12)` rings.

⚠ Because the ring is camera-centred, there is NO arithmetic shortcut for how low `gen` may go.
The only criterion is whether the shorter horizon is visible — §22.2.

### 22.2 The visual gate — and why the first read of it was worthless
Four cold-launch arms on Island Showdown (`tools/sceneupdate/ringvisual.sh`, `setup.ini
terraingen=<N>`), two fixed views each, pixel-diffed with `tools/imgdiff.ps1`.

The first diff said 13.3% of pixels differ at gen 12. **That number meant nothing**: Island
Showdown has swaying palms, animated water and grass, so two cold launches differ on their own.
★ The fix is the rule this project already had and nearly skipped again — **take a same-setting
control**. Two independent gen-14 launches establish the floor:

| view | **noise floor** (gen14 A vs gen14 B) | gen 12 | gen 10 | gen 8 |
|---|---|---|---|---|
| default | 16.245% / **1.0765** | 13.274% / **0.5528** | 20.568% / 1.5619 | 20.521% / 1.5901 |
| horizon | 8.284% / **0.1892** | 2.603% / **0.1018** | 14.199% / 0.3830 | 13.891% / 0.6756 |

(`differing%` / `meanAbsDiff` out of 765.)

★★ **gen 12 is BELOW the noise floor on both views and both metrics** — it differs from run A by
less than an identical-setting rerun does. That is the strongest available form of "no detectable
change". **gen 10 is the first step that measurably alters the image** (mean 1.4x the floor on the
default view, 2.0x on the horizon); gen 8 is 3.6x on the horizon.

Corroborating, from the horizon camera: **POLYS = 19464 bit-identical at gen 14, 12 AND 10**,
moving only at gen 8 (18192, −6.5%). The two instruments disagree about *where* the cut starts
(pixels say between 12 and 10, POLYS says between 10 and 8) and both are right — they are
different cameras. The conservative reading is the pixel one, and it lands on **12**.

⚠ Why gen 10 and gen 8 differ from gen 14 by nearly the SAME amount (20.57/1.562 vs 20.52/1.590)
rather than progressively: a distant feature sits between the gen-10 reach (1341 m) and the gen-12
reach (1609 m). Both 10 and 8 lose it; 12 keeps it. That is the mechanism behind the verdict.

### 22.3 Counters — the mechanism, confirmed
Every ECS counter falls by exactly the ring delta, in lockstep (Island Showdown, horizon camera,
so absolute values include post-jump chunks):

| gen | chunks | meshes | materials | transforms | hierarchy |
|---|---|---|---|---|---|
| 14 | 898 | 2401 | 2385 | 15141 | 7659 |
| 12 | 674 | 2177 | 2161 | 14917 | 7435 |
| 10 | 482 | 1985 | 1969 | 14725 | 7243 |
| 8 | 322 | 1825 | 1809 | 14565 | 7083 |

14 → 12 on a parked camera is **841 → 625 = −216 of every counter**.

### 22.4 Scene::Update cost — one binary, both arms
`tools/sceneupdate/sugen.sh "Switch Escape" 14 12`, 6 profiler samples per arm, settle-gated.

| arm | chunks | SCENE_MESHES | Scene::Update mean | samples |
|---|---|---|---|---|
| gen 14 | 841 = ringMax | 1174 | **1.515 ms** | 1.36–1.67 |
| gen 12 | 625 = ringMax | 958 | **1.128 ms** | 1.07–1.24 |

**−0.387 ms, −25.5%.** The two sample ranges do NOT overlap, which is what lifts this clear of the
±2.5% "do not trust a single A/B on this rig" caution.

★ Knob-reaches-the-thing check, done BEFORE reading the timings: `chunks` 841 → 625 is exactly
`(2N+1)^2`, and `SCENE_MESHES` 1174 → 958 is exactly −216. Both arms are the SAME binary — the
`terraingen` key varies the ring, so no rebuild sits inside the measurement.

⚠ Today's gen-14 arm reads 1.515 ms where the 2.24 sweep recorded 1.323 ms for the same default.
That is cross-session drift, not a regression — which is exactly why the −25.5% is quoted from the
within-session pair and NOT as "1.323 → 1.128".

### 22.5 ⚠ PRE-REGISTERED before the 2.25 hub sweep ran: C2 does not apply as written
`sweepgate.sh` C2 is "POLYS identical on every demo". **That criterion was written for the ECS
lookup change**, where POLYS could only move if an entity resolved to the wrong component — a
pure correctness proof. It is NOT the right gate for a draw-distance change, which can legitimately
remove drawn far terrain. Recording the amendment here, before the run, so the verdict cannot be
rationalised afterwards:

- **C2' GEOMETRY** — a POLYS *decrease* is permitted, but every demo that moves must be named and
  attributed to far-terrain removal. A POLYS *increase* is still an automatic fail (nothing in this
  change can add geometry). C1/C3/C4 are unchanged.
- Evidence standard for any demo that moves: the same-setting-control pixel method of §22.2, not an
  eyeball, and not a bare cross-run screenshot.

### 22.6 2.25 hub sweep — and the coverage finding that matters more than the saving
`scratchpad/demo_fps/results_0811e_2.25.txt`, scored by `sweepgate.sh` against the C2' amendment
pre-registered in §22.5.

**C1 LOAD PASS** 19/19 · **C3 VRAM PASS** worst 3772.2 MB in-game (Operation Amazon), **headroom
183.1 → 323.8 MB** · **C4 GAME PASS** 19/19 reached gameplay.

**C2' GEOMETRY PASS.** 18/19 demos POLYS bit-identical to the reference. One moved, and in the
permitted direction: **Aztec Game Kit Teaser 10 313 511 → 10 311 639 = −1 872 tris (−0.018%)**.
(`sweepgate.sh` prints this as a raw C2 FAIL because its criterion is literal identity — that is
the criterion §22.5 amended in advance, not a verdict rationalised afterwards.)

### ★★★ 22.7 RETRACTED — "two demos have 5 km maps and are CROPPED" was WRONG
**What I claimed, in this document, before checking it:** `TERRAIN_RING` reported
`needGen = ceil(mapHalfU / chunkU) = 19` for A Grand Canyon Adventure and Operation Amazon
(both 5 km maps) against a shipped gen of 12, so the outer ~891 m of their own *editable* map
supposedly had no terrain under it. It was written up as the headline finding of 2.25 with a
recommended `clamp(needGen, 12, 14)` fix and a DECISION-NEEDED flag to the user.

**It is false.** `needGen` silently assumed the ring is centred on the WORLD ORIGIN. It is not:
GG sets `SetCenterToCamEnabled(true)` (`GGTerrainWicked.cpp:2543`) and the engine recomputes
`center_chunk` from `camera.Eye` every frame (`wiTerrain.cpp:776-780`). **The ring follows the
camera.** A map can never outrun it, at any size, so nothing was ever cropped — not at gen 12,
not at gen 14, not at the 5 km slider maximum.

★ **What caught it: refusing to ship the claim on arithmetic.** The finding was already written
up and flagged to the user when I built `tools/sceneupdate/ringedge.sh` purely to turn the
inference into a photograph — fly A Grand Canyon Adventure out along +X and look. At **x=90000**
(2286 m, far past any origin-centred reach) there is **solid ground filling the frame**, and
`chunks` climbs 625 → 1020 as the ring rebuilds around the moving camera. Both observations are
flatly inconsistent with a fixed ring, and neither is visible from the maths.

⚠⚠ **THE LESSON, and it is the expensive kind: a derived metric can encode an assumption you
never consciously made.** Every input to `needGen` was measured correctly — `editable_size` was
even cross-validated against `TERRAINPORT.md` — and the formula was arithmetically right. The
defect was one unstated premise about what the ring is centred on. Measuring the inputs harder
would never have found it; only looking at the thing did.
★ RULE: **before a derived number becomes a finding, name the assumption that makes the formula
valid, then test THAT.** Here: "the ring is origin-centred" — one grep of `SetCenterToCamEnabled`
would have killed it in seconds.

**Consequences, all of them:** the instrument no longer computes `needGen` or `marginM` (it prints
`viewM` + `centreToCam` instead, with a comment forbidding reintroduction); the `clamp(needGen,
12, 14)` proposal is withdrawn; there is no decision pending for the user; and the two demos are
not defective. ⚠ There is genuinely **no map-size constraint on `gen` at all** — the only
criterion is the visible-horizon test of §22.2, which is what gen 12 was actually cleared by.

### 22.8 Discharging C2' — the one demo whose POLYS moved
**Aztec Game Kit Teaser, −1 872 tris (−0.018%).** The two dropped rings held distant scenery
1609-1878 m from the camera.

Whole-frame pixel diff could NOT settle it, and the reason is instructive:

| Aztec, whole frame | noise floor (gen14 A vs B) | gen 12 |
|---|---|---|
| default view | 36.483% / **11.0340** | 34.796% / **13.4571** |
| horizon view | 15.073% / 0.3098 | 4.219% / 0.1100 |

That default-view floor of **mean 11.03** is the highest measured anywhere in this campaign — the
level is half waving grass. gen 12 reads 13.46: *below* the floor on differing-pixel count but ~22%
*above* it on mean. One control sample cannot separate that, and it would have been wrong to call
it either way.

★ **The fix is to diff the region where the effect must appear, not the whole frame.**
`tools/regiondiff.ps1` (new) restricted to the upper viewport band — sky line + distant hills,
above the grass:

| Aztec, band 250,105–1283,310 | differing | meanAbsDiff |
|---|---|---|
| noise floor (gen14 A vs gen14 B) | 14.209% | **0.3365** |
| gen 12 | 4.605% | **0.0767** |

**4.4x BELOW the floor** on the only band that could show far-terrain loss. C2' discharged: the
movement is real, tiny, and produces no detectable change where it would have to be visible.

★★ **RULE — a whole-frame diff is the wrong instrument when the effect is localised and the noise
is not.** Grass animation is bottom-of-frame; far terrain is top-of-frame. Diffing everything let
a 0.018% geometry change hide inside an 11.03 mean grass floor. Pick the band first.

## ★★★ 23. THE "~75 UNEXPLAINED MESHES" — NAMED. It is the decal element pool (2026-08-12)

§16 closed with a residual it could not identify: 955 of 1030 floor meshes attributed, **75
unexplained**, with a note that some of it might be a leak. This section names it.

### 23.1 Method: enumerate, do not subtract
§16 attributed by ACCOUNTING — sum the known creators, subtract from the measured total, call the
difference unexplained. That can prove a remainder exists but never say what it IS, and it is the
same shape of reasoning that produced the retracted §22.7 finding. New harness command
**`DUMP_MESHES [filter]`** (GGMAX 2.26) instead walks every live `MeshComponent` and groups it:
- **by GEOMETRY SIGNATURE** (verts/tris/subsets) — generator families cluster exactly even when
  unnamed, which matters because GG names nearly every loaded mesh `node_mesh`;
- **by OWNER** (the referencing object's name — the real identity);
- **ORPHANs** — meshes no `ObjectComponent` references;
- `DUMP_MESHES <filter>` then itemises one family with parent, renderable flag and world position.
⚠ It rides `AutoHarness_StandaloneCommands`; a new link in the main dispatch chain re-hit C1061.

### 23.2 What three demos show
| | Switch Escape | Trapped | Zombie Cellar |
|---|---|---|---|
| meshes | 958 | 977 | 961 |
| terrain chunks (v=4489) | 625 | 625 | 625 |
| **`plane` v=6 tri=2** | **108** | **113** | **110** |
| `Body` / `new limb` (characters) | 17/17 | 9/9 | 22/22 |
| **ORPHANS** | **0** | **0** | **0** |

★ **ORPHANS = 0 on all three: there is NO mesh leak** on a loaded level. §16's leak hypothesis
("Entity_Remove on an object may not destroy its MeshComponent") is not supported here — closed.
★ Mesh COUNT barely moves across three levels whose POLYS differ 10x (109k / 11k / 28k). Mesh
count is dominated by fixed cost, not content.
★ The 625 chunks carry **8 298 750 of ~8 600 000 triangles — 96%** of all mesh geometry.

### ★★★ 23.3 The answer: a preallocated pool of 100 hidden decal quads
`DUMP_MESHES plane` itemised them: **every one is `renderable=NO`, at world (0,0,0), 6 verts /
2 tris.** Non-renderable placeholders parked at the origin — the same shape as the 441 empty prop
nodes (2.22) and the retained tree pool (2.23).

`decal_init()`, **`M-Decal.cpp:41-65`**:
```
//  Precreate elements as each is unique (UV writing)
for ( t.f = 1 ; t.f <= g.decalelementmax; t.f++ )
{ ... MakeObjectPlane (t.tobj, 100, 100); ... HideObject (t.tobj); }
```
`g.decalelementmax = 100` (`Common_part0.cpp:1380`, `REDUCEMEMUSE` is defined at `:28`). One hidden
quad per slot, built at startup, on **every level whether or not a decal is ever placed**. Cost:
**100 objects + 100 meshes + 100 materials + 100 transforms walked by Scene::Update every frame.**
The census's 108/113/110 is the 100-slot pool plus a handful of other `MakeObjectPlane` sites
(gun decals `G-Gun_part2.cpp:1411/1453`, explosions, the EBE tool plane).

★★ **The author had already seen the symptom and could not explain it.** Directly above the pool
size, `Common_part0.cpp:1378`:
> `//PE: For now until we found out why wicked use all that "update" time on non visible objects.`

and the value was walked down **499 → 199 → 100** as a workaround. That is this entire campaign's
root cause, observed from the other end: **Scene::Update walks every ECS entity regardless of
visibility.** The pool is a textbook instance, and the shrink was treating the symptom.

### 23.4 ⚠ §16's "content = 34" was wrong, and so was its floor
§16 read content by delete-all: full 1288 meshes, empty 1254, so "content = 34". But the census
shows Switch Escape carrying hundreds of unmistakable content meshes — `railings1`,
`CorrugatedRoof2`, `ConcreteWarehouse6`, `WindowFrame2`, `warehouse07`, `zombie_male_body_01`.
Deleting the objects plainly did not free them, so **1254 was never a floor** — it was the floor
plus retained content, and every figure derived from it (including the "75") was inflated.
Cross-demo differential is the sound substitute: families with near-identical counts across
unrelated levels are the true fixed floor. On that basis the real per-level fixed floor is
**~625 chunks + ~100 decal quads + ~8 misc + 2 VR controller meshes ≈ 735**, not 1254.

### 23.5 The fix, not yet taken
Same lever that worked twice already: **lazy pool growth** (2.19 tree pool, 2.24 tree types).
Allocation is a linear scan for `active == 0` (`M-Decal.cpp:506`) over a fixed array, so slot N's
object can be created on first use instead of at init, with the 2.19 per-frame growth cap.
Worth ~100 objects/meshes/materials/transforms per level, on every level.
⚠ Decals are allocated at RUNTIME (bullet impacts), so growth must be safe mid-frame — this needs
its own build + POLYS gate + 19-demo sweep, and is deliberately left as a separate change.
⚠ Do NOT "fix" it by shrinking `decalelementmax` further; that is what the original workaround did
and it trades decal capacity for frame time.

## ★★ 24. GGMAX 2.27 — decal pool: PREWARM + GROW (the §23 fix)

§23 named the largest non-terrain floor item: `decal_init()` built all 100 pool quads at startup
and hid them, so every level carried 100 objects + meshes + materials + transforms that draw
nothing. 2.27 builds only a prewarm slice and grows the rest on demand.

**Why prewarm and not pure lazy.** Decals are allocated at RUNTIME from bullet impacts, so a fully
lazy pool moves object creation into a firefight. `decal_init()` is called **once at app startup**
(`Common_part2.cpp:1292`), never per level — so the prewarm is paid where nothing is running, and
normal play then allocates nothing at all. Growth beyond it is batched
(`GG_DECAL_GROW_STEP = 8`) so a burst costs a bounded number of grow events, not one per impact.

**What the hitch risk is NOT.** The expensive thing in this engine is the lazy-PSO compile
(1-30 ms), and it is already deferred: `pso_validate` builds the pipeline **at first BIND**
(`wiRenderer.cpp:208-215`) and hidden objects never bind. The first decal *drawn* has always paid
that compile — 2.27 does not move it by a frame. What 2.27 adds per slot is a 6-vertex mesh, four
ECS components and one small GPU buffer. The design also already mutates the mesh per placement
(`SetObjectUVManually`, `M-Decal.cpp:1024` — the "each is unique (UV writing)" comment), so
placement was never free.

**Safety, verified before writing the change:** only the allocator can see an unbuilt slot.
`decal_hide()` guards on `ObjectExist()`; the per-frame control loop and `M-Entity_part4.cpp:1894`
both gate on `active == 1`, which an unbuilt slot can never be.
Allocator semantics preserved on both ends: the random start (spreads reuse so the same quad is
not recycled every time) is kept but scaled to the built range, and when the pool is genuinely
exhausted `t.d` is left `>= decalelementmax` so the existing test drops the decal exactly as before.

### 24.1 ⚠ PRE-REGISTERED gate criteria (written before the run)
`tools/sceneupdate/decalhitch.sh`, two cold-launch arms on ONE binary:
CONTROL `decalprewarm=99` (== the pre-2.27 eager pool) vs PREWARM `decalprewarm=24` (shipping).
Each: settle → `HITCH_RESET` → 4 x `DECAL_BURST 120` → read `HITCH:`.

- **C1 KNOB REACHES** — pre-burst `DECALPOOL: built=` must DIFFER between arms. Identical means
  the key never reached the pool and both arms measured the same thing (the §2 `SET_TREES` trap).
- **C2 GROWTH EXERCISED** — the PREWARM arm's post-burst `built` must exceed its pre-burst `built`.
  If the pool never grew, the burst did not stress the new path and the run says nothing.
- **C3 HITCH TAIL** — PREWARM must not add frames to the over-buckets vs CONTROL, and `worst_ms`
  must not exceed CONTROL's by more than ~2 ms. Both arms run an identical burst, so any excess in
  PREWARM IS the growth cost. ⚠ Judge the TAIL; a 1-30 ms stall is invisible in mean FPS, which is
  precisely why the 1.82 hitch instrument exists.
- **C4 SAVING REAL** — PREWARM `SCENE_OBJECTS`/`SCENE_MESHES` must be ~75 below CONTROL pre-burst.

⚠ Honest limitation: this is a synthetic burst in the EDITOR, denser than real weapon fire, and it
is a stress test rather than a gameplay sample. That errs conservative (harder than reality),
which is the right direction for a gate — but it is not a substitute for a play session.

### 24.2 The gate: FOUR runs, three of them failures — and each failure was a real defect
Every run used the SAME script and the SAME pre-registered criteria (§24.1). Nothing was relaxed
after seeing data; what changed each time was the implementation, against a NAMED measured cause.

| run | change under test | C2 grew? | CONTROL worst | PREWARM worst | over(16.7) | verdict |
|---|---|---|---|---|---|---|
| 1 | batch grow, no per-frame cap | 24 → **99** | 9.5 | **23.9** | **1** | ❌ C3 |
| 2 | cap 4, budget refilled in `decalelement_control()` | 24 → **24** | 9.0 | 7.8 | 0 | ❌ **C2 — void** |
| 3 | cap 4, budget self-refills off the frame counter | 24 → 40 | 8.6 | **13.4** (+4.8) | 0 | ❌ C3b |
| 4 | **cap 2** | 24 → 32 | 7.8 | **8.2 (+0.4)** | 0 | ✅ **PASS** |

**Run 1 — the answer to "will it pause?": YES, as first written.** Batching grow CALLS bounds
nothing; a dense burst just calls the allocator repeatedly in one frame, so all 75 slots were built
in a single frame: **worst_ms 9.5 → 23.9, one frame over budget**. The tree pool already knew this
(`GG_TREE_POOL_GROW_PER_FRAME`, 2.19) — growth needs a PER-FRAME cap, not a per-call one.

★★ **Run 2 is the important one.** It read `worst_ms 7.8, over=0` — a clean pass on every timing
number — while the pool sat at **built=24 and never grew**. The budget was refilled in
`decalelement_control()`, which does not tick in the editor, so growth was silently dead and the
"no hitch" measured a code path that never ran. **C2 ("the pool must actually grow") was added as a
formality and is the criterion that caught it.** Had C2 not existed this would have shipped with
growth broken in some app states — and the user-visible symptom would have been decals silently
not appearing. ★ RULE: **"did the mechanism execute?" is a SEPARATE question from "what did it
cost?", and the timing numbers look perfectly healthy when the answer to the first is no.**
Fix: the budget now self-refills off the device frame counter inside `decal_growbudgeted`, so it
cannot be disabled by another function failing to tick.

**Run 3** priced a slot build at **~1.2 ms** — expensive for a 6-vertex quad, because the cost is
the DBP `CreateNewObject` path plus a GPU buffer, not the geometry. 4/frame = +4.8 ms worst-frame,
which breached the +2 ms clause even though no frame passed 16.7 ms. ⚠ Predicted cap 2 would give
~+2.4 ms; it actually gave **+0.4 ms**, so the relationship is not linear and the cap-4 worst frame
had coincident work. The prediction was wrong in the favourable direction — recorded because a
lucky miss is still a miss.

### 24.3 ⚠ What this costs, stated plainly
- **Decals dropped under an extreme burst.** With the pool below demand the allocator drops rather
  than stalls: `deferred` 448 (PREWARM) vs 381 (CONTROL) out of 480 requests. That 67-decal
  difference is the price of the cap. It is self-correcting — the pool grows 2/frame — and real
  weapon fire is spread over frames where a synthetic 120-in-one-frame burst is not. `deferred=`
  in `DECALPOOL:` makes it visible rather than silent. ⚠ The CONTROL number also matters: the
  pre-2.27 build ALREADY dropped 381 of 480: drop-when-full is long-standing, not new.
- **The saving decays with decal use.** The pool never shrinks, so a level that places many decals
  climbs back toward 99. This reliably helps the EDITOR (no decals placed — and every
  Scene::Update measurement in this campaign is an editor measurement) and light-decal play; in a
  sustained firefight the entities come back. It is NOT "−75 everywhere".
- **Not a gameplay sample.** A synthetic editor burst, denser than real fire. Conservative, but a
  play session is still the real test.

### 24.4 C4 — the saving, measured with the §23 census (not inferred from the pool count)
Fresh launch, no bursts, pool sitting at the prewarm. `DUMP_MESHES` on Switch Escape:

| | eager (pre-2.27) | prewarm 24 | delta |
|---|---|---|---|
| meshes | 958 | 882 | **−76** |
| objects | 1106 | 1030 | **−76** |
| materials | 964 | 888 | **−76** |
| transforms | 1780 | 1628 | **−152** |
| hierarchy | 1338 | 1262 | **−76** |
| `plane` family (v=6 tri=2) | 108 | **32** | −76 |

★ **transforms fall by 2x the object count** — each DBP object carries a limb/frame transform as
well as its own, so the per-frame Scene::Update saving is larger than the mesh column implies.
The residual 32 planes = 24 pool slots + ~8 non-pool `MakeObjectPlane` sites (gun decals,
explosions, the EBE tool plane), exactly as §23 predicted.
⚠ C4 was written as "SCENE_OBJECTS/MESHES ~75 below control" and the gate script only captured
the pool count, which is a proxy. Measured with the census instead rather than passing the
criterion on the proxy.

**VERDICT: 4/4 pre-registered criteria PASS** (run 4, `GG_DECAL_GROW_PER_FRAME = 2`).
⚠ **STILL OWED before release: the 19-demo hub sweep.** Every other change this campaign got one;
this one has had the hitch gate the work was scoped to, and nothing more. POLYS is expected
identical (hidden pool quads never drew) but that is a prediction, not a measurement.

### 24.5 The 19-demo sweep 2.27 owed — RUN, and the prediction holds
`tools/sweep_0812_2.27.txt`. **C1 PASS** 19/19 · **C3 PASS** worst 3768.1 MB in-game (Aztec Game
Kit), headroom **327.9 MB** · **C4 PASS** 19/19 reached gameplay.

**C2: POLYS bit-identical to the 2.25 shipped state on all 19 demos.** `sweepgate.sh` prints one
mismatch — Aztec Game Kit Teaser 10 311 639 vs "ref" 10 313 511 — but that is **the same −1 872
already attributed to 2.25's ring change** and cleared in §22.8; the script's C2 reference is the
stale 0809 sweep, which predates 2.25. Against the correct baseline the deviation is zero:
Canyon 8 816 163, Foggy Forest 10 195 894, Operation Amazon 5 496 922 … all unchanged.

★ This closes the debt recorded in the 2.27 commit, which said POLYS-identical was *"a prediction,
not a measurement"*. It is now a measurement, and the prediction was right — expected, since
hidden pool quads never drew, but expected is not the same as verified.
⚠ Note for whoever next runs `sweepgate.sh`: **its C2 reference needs re-baselining to 2.25**, or
it will keep reporting this one demo as a failure forever.
