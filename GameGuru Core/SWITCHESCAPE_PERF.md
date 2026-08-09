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
| **Update - Wicked (Scene::Update)** | **0.21** | **2.39** | **+2.18** | **75%** |
| Update - Logic | 0.62 | 0.90 | +0.28 | 10% |
| Compose | 0.04 | 0.24 | +0.20 | 7% |
| Render | 0.71 | 0.87 | +0.16 | 6% |
| Update - Terrain | 0.03 | 0.14 | +0.11 | 4% |

**Three quarters of the entire gap is one range: `Scene::Update`.** That is the modern-Wicked
ECS walking the whole scene every frame. Its stage split is S1 Anim+Transform 0.91,
S2 Hier+Mesh+Mat 0.79, S4 Object+Light 0.61 — and only 0.03 ms of that has named children,
which is what §3 is about.

### GPU side (becomes the wall once CPU < 3.68)

`Transparent Scene 1.12` (30% of the GPU frame), Opaque 0.51, Update Buffers 0.32,
Occlusion pair 0.28, MSAO 0.11, Scene MIP 0.10, Z-Prepass 0.04.

Scene: 7322 objects / 8437 transforms / **576 visible** / 22 lights (16 visible) /
109,358 polys / 221 transparent objects of which **141 are double-sided**.

---

## 2. Refuted: the tree pool is NOT the cost here

The obvious suspect was the parked tree pool — `DUMP_TREEPOOL` reports
`POOL size=6000 bound=0 renderable=0` on a level with no terrain and no trees.

**Measured and refuted.** Fresh load with `SET_TREES pool 0` (pool clamps to 1):

| | pool 6000 | pool 1 |
|---|---|---|
| FPS | 200.8 | **201.5** |
| SCENE_OBJECTS | 7322 | **7322** |

Removing 5999 pool slots changed nothing, and `SCENE_OBJECTS` did not move at all — the pool
entities are not part of that population, and unbound slots (no mesh, not renderable) are
close to free in the ECS. A lazy-pool change was written and then **reverted** rather than
shipped on the strength of a benefit that does not exist on this level.

⚠ Do not re-chase the tree pool for no-tree levels. (It remains the right lever on *tree*
levels — that is the separate, already-documented Stage P.4 result.)

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

## 9. What would settle the single-queue default (the one job worth doing next)

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

- ⚠ **Do not chase the tree pool on no-tree levels** (§2) — measured zero, twice.
- ⚠ **Do not trust `Input`, `SU-*` or any single CPU range as an FPS lever on this level**
  without an A/B; four separate cuts measured zero.
- ⚠ **Do not compare a `SET_SCENESERIAL 1` total against a normal frame** — only shares.
- ⚠ The editor is unlocked but **the hub is vsync-locked to 60.0 FPS** — never benchmark there.

---
