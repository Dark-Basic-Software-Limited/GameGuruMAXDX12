---
name: project-switchescape-perf
description: "Switch Escape DX11-vs-DX12 frame study and the campaign it started (08-09 to 08-11). FINAL: Scene::Update 2.412 -> 1.323 ms (-45%) + VRAM -75 MB hub-wide, six clean 19-demo sweeps, POLYS identical every time. Floor root-caused = Wicked native wi::terrain (DX11 has none). Three level-change leaks fixed, all one shape. Two optimisations measured WORSE and reverted. Editor frame is GPU-fence-bound: judge by ms, never FPS."
metadata:
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-11T21:45:00.000Z
---

# Switch Escape perf study (2026-08-09 → 2026-08-11)

## ★★★ 2026-08-11 FINAL: Scene::Update −45%, VRAM −75 MB, SIX CLEAN SWEEPS
Repo `SWITCHESCAPE_PERF.md` **§10–§21** is THE authority. Validated tag
**`baseline-lazytreetypes-20260811`** (game `029c7043`, engine `04ac483c`).

| build | change | Scene::Update |
|---|---|---|
| 2.17 baseline | — | 2.412 ms |
| 2.18 | ECS sparse lookup (DX11-parity restoration) | 1.750 |
| 2.19 | lazy tree-pool growth | 1.457 |
| 2.22 | `prop_density=0` — 441 empty scaffolding nodes | **1.323** |
| 2.23 / 2.23b | release pool + far-shadow proxies on level change | 1.323 |
| 2.24 | tree-type assets built PER LEVEL | 1.323 |

★★ **ROOT CAUSE of the whole DX12 entity floor: Wicked's NATIVE `wi::terrain`.** DX11 has no
`wiTerrain.cpp/.h` at all and creates ZERO ECS entities for terrain/trees/grass. Empty-level
panels proved content is comparable (DX11 92 meshes vs DX12 34) and the multiplier is entirely
the FLOOR. Exact cross-check: generation=14 → 841 chunks + prop_generation=10 → 441 props gives
1+841+441 = **1283** = the independently measured `HIER maxSubtree`.

★★ **THREE LEAKS, ONE SHAPE — all fixed.** The park path only HID entities
(`SetRenderable(false)`) and their only teardown sat behind `GGTrees_WickedShutdown`, **which has
zero callers**: the 6000-slot pool (2.23), the 79 far-shadow proxy chunks + 17 materials (2.23b),
and the per-type assets once they became per-level (2.24). Level changes now return the scene to
its true fresh-load size (only the known +1 player-start marker remains).
★ **If another tree-system entity ever leaks, look at that function first.**

★ **2.24 is the campaign's FIRST REAL VRAM WIN**: mean −74.7 MB editor / −68.8 MB in-game,
**negative on all 19 demos in both columns**, worst-case headroom 134.9 → **183.1 MB**.
★★ RULE: **a VRAM delta is a result only if it is UNIDIRECTIONAL across demos AND has a
mechanism.** Earlier ±84 MB swings were bidirectional and mechanism-free — correctly called noise.

⚠ **TWO THINGS MEASURED AND REJECTED** (do not retry): the parallel instance-array init (a wash)
and the SU-Hierarchy load-balance split — which fixed the imbalance perfectly (0.643 → 0.007) and
made Scene::Update **16.6% WORSE**. ★★ The lesson: **a `SET_SCENESERIAL` per-system number is a
SHARE, not a saving** — serialising removes the cross-system overlap that was hiding the cost.

## ★★★ 2026-08-10 ACTED ON IT — engine 2.18: ECS sparse lookup = −27% of Scene::Update
Repo `SWITCHESCAPE_PERF.md` **§11** is THE authority. Revert tag **`baseline-sceneupdate-20260810`**
(game `04b6a3da`, engine `07b616db`), pushed both repos.

| change | Scene::Update | verdict |
|---|---|---|
| baseline 2.17 | 2.412 ms | — |
| **2.18 `wiECS.h:445` LOOKUP_SPARSE** | **1.750** | ★ **−0.662 ms (−27.4%)** SHIPPED |
| `SET_INSTINIT 1` parallel blank pass | 1.765 | **wash (+0.9%)** — default OFF |
| `setup.ini treepool=1` (blunt probe) | ~1.62 | −0.13 more; default UNCHANGED |
| **2.19 LAZY TREE-POOL GROWTH** | **1.457** | ★★ **cumulative −0.955 ms (−40%)**, CPU frame 4.49→3.55 |

## ★★ 2.19 (2026-08-10): lazy tree-pool growth — the real fix, shipped
`GGTrees_part2.cpp`: `g_treePoolSize` is now only a CEILING; new `g_treePoolBuilt` counts slots
that actually exist, starting at 0, grown on demand by `GrowTreePool()` from `poolFill` (the
nearest-N count the selection pass already computes). Per-frame growth cap 2048 so the creation
of thousands of entities spreads over a few load frames instead of one hitch; growth is one-way
for the pool's lifetime so a camera walking away does not thrash create/destroy.
★ **`SCENE_OBJECTS` 7322 → 1322 on Switch Escape with NO knob set** — the level's real object
count was always ~1322. Loops that walk EXISTING slots (park, cascade, rebind, Pass A, shutdown,
dump) bound on `built`; loops expressing DESIRED N (`nth_element` cap, ring coverage) keep the
ceiling, because that is what they mean.
⚠ `DUMP_TREEPOOL` first line is now `POOL built=N ceiling=M ...`; **`built=0` on a treeless level
is CORRECT, not a broken pool.**
⚠ `SET_TREES pool` is no longer fully dead: RAISING the ceiling permits growth, but LOWERING it
still cannot shrink an existing pool. Real reduction = `setup.ini treepool=<N>`.

- **The win is a DX11-parity restoration**, not a new idea — DX11 has had the sparse array since
  `WickedRepo b40f00a`; the port reverted to upstream stock. One `#define`; the implementation
  was already sitting unused at `wiECS.h:486-593`. Audited before enabling (value-initialising
  block allocator; `erase()` only reachable for present entities).
- ⚠ **It has a BUILD cost**: extra template instantiations pushed `DarkLUA.cpp` past the COFF
  section limit (C1128) and Camera's browse DB past BSCMAKE BK1520. Fixed with `/bigobj` on
  DarkLUA + Release-only browse-info off on Camera. ★ **If another TU hits C1128, add `/bigobj`
  — do NOT revert the ECS change.**
- ⚠ **MY HYPOTHESIS WAS REFUTED BY MEASUREMENT**: the 1.87 MB single-threaded instance-array
  blank pass (`wiScene.cpp:332-340`) looked like the S1 pole and parallelising it is a **wash**.
  It is an `Execute` overlapping the parallel dispatches, so it was never the critical path.
- ★ **`setup.ini treepool=<N>` (new) PROVES the pool is real**: `SCENE_OBJECTS` 7322 → **1323**.
  It works because `GetSetupIniEarly()` runs before the one-shot `GGTerrainWicked_Init` — which
  is exactly what the dead runtime knob could not do. **Next real win = LAZY POOL GROWTH**
  (re-bound the ~10 `for i < g_treePoolSize` loops in `GGTrees_part2.cpp` to an allocated count);
  do NOT just lower the default, it would cripple tree levels.
- ⚠ **FPS did not move (142.5 → 143.5) and that is the EXPECTED result** — the frame is
  GPU-fence-bound. **Judge this work by the `Scene::Update` ms column, never by FPS.**
- ⚠ Machine was GPU-slower on 08-10 than on 08-09 (142 vs 201 FPS on the same scene) while CPU
  Frame matched (4.49 vs 4.58). **Same-day arms only; never compare FPS across these two days.**
- Method + scripts: `tools/sceneupdate/` (`sumeasure.sh`, `suarms.sh` 3-arm within-launch,
  `susmoke.sh` POLYS-identity gate).
- ★★ **FULL 19-DEMO SWEEP + VRAM GATE: CLEAN** (`tools/sweep_0810_2.18.txt`, scored by new
  `tools/sweepgate.sh` with criteria fixed BEFORE the run). 19/19 load; **POLYS identical to the
  0809 reference on all 19** (the real correctness proof for an engine-wide ECS lookup change —
  11k to 10.3M polys, a mis-resolved entity would have moved the counter); VRAM worst
  **3962.5 MB** (Aztec GK in-game), 133.5 MB headroom, 0/19 breach; all 19 reach gameplay.
- ⚠ **In-game VRAM is HIGHER than editor VRAM on every demo** (Aztec GK 3811.4 vs 3962.5).
  My first `sweepgate.sh` read only the editor column and would have passed a build breaching
  4 GB where players actually run. **Always gate BOTH columns.**
- ⚠ **FPS variance today was 11% launch-to-launch on the SAME build and scene** (Switch Escape
  142.5 / 149.3 / 159.6), vs 201 on 08-09 at a matched CPU frame. Reinforces the standing rule:
  **never gate on FPS across launches, let alone across days.**

# Original study (2026-08-09, 2 h autonomous)

Repo `GameGuru Core/SWITCHESCAPE_PERF.md` is **THE authority**. Engine 2.15 `33335382`,
game `883d2a49`. **Revert point: tag `baseline-switchescape-20260809` in BOTH repos.**

## The gap
DX11 **302.8 FPS / 3.30 ms** vs DX12 **200.8 / 4.98 ms** (warm 180 s soak, opening camera).
CPU 1.69 → 4.58, GPU 2.28 → 3.68. **75% of the CPU gap is ONE range: `Scene::Update`
0.21 → 2.39 ms** — modern Wicked walking 7322 objects / 8437 transforms through ~30
subsystems every frame. Everything else (Logic +0.28, Compose +0.20, Render +0.16,
Terrain +0.11) is small change.

## ★★★ SOLVED (engine 2.16): the level is GPU-FENCE-BOUND; SET_SINGLEQUEUE = +23.9 FPS
The rolling `SUBMIT_STALL_WINDOW` (new, `SET_SUBMITSTATS 1` to reset) shows the CPU waits on
the GPU frame fence in **99.4% of frames, mean 0.89 ms**. Proof of mechanism: **ADDING**
0.25 ms of CPU work (`SET_XINPUT 0`) **SHRANK** the stall 0.893 → 0.711 with FPS unchanged.
Less GPU work (`SET_RESSCALE 0.5`) halved it (0.407) and gained 25 FPS.
⚠ The earlier `stall=0.00` snapshots were sampling luck — `SUBMIT_PHASES_MS` is LAST-FRAME.
★ This retro-explains every zero below: they are all on the wrong side of the wall.

**~1 ms of the 3.68 ms GPU frame is in no named pass** (lists=14 batches=12 deps=9) = cross-queue
bubble. `SET_SINGLEQUEUE 1` removes it. **Six demos, three arms each (0/1/0), editor, warm —
6/6 POSITIVE, and max stall improves on 5/6:**

| demo | off | on | delta | stall mean | stall max |
|---|---|---|---|---|---|
| Trapped | 211.7 | **251.1** | **+18.6%** | 0.663→**0.064** | 1.30→0.92 ✔ (stalled 98%→**22%**) |
| Zombie Cellar | 193.7 | **219.8** | **+13.5%** | 0.856→0.382 | 1.73→1.53 ✔ |
| Switch Escape | 201.3 | **225.3** | **+11.9%** | 0.90→0.42 | — |
| Island Showdown | 91.6 | 95.1 | +3.8% | 2.12→1.82 | 3.37→**6.71** ✘ |
| Aztec GK Teaser | 83.5 | 85.9 | +2.9% | 4.82→4.50 | 6.49→6.06 ✔ |
| Foggy Forest | 76.7 | 78.2 | +2.0% | 4.13→4.05 | 6.06→5.60 ✔ |

★ **Gain scales INVERSELY with GPU load** — the bubble is fixed per-frame overhead, so it is a
big share of a 5 ms frame and a small share of a 12 ms one. A/A2 control drift ≤1.3 FPS.
`SET_LEANASYNC 1` = +5.5% on Switch Escape (weaker sibling; single-queue supersedes it).

## ★★★ SHIPPED: DEFAULT FLIPPED ON (engine 2.17 `b7bfe180` / game `e01263ef`)
Full 19-demo hub sweep, 3 arms each (0/1/0), 40 s settle-gated arms, criteria fixed BEFORE the
data existed. **18/19 positive, mean +5.07%, POLYS bit-identical on all 19, no tail regression.**
Trapped +16.7 / Switch Escape +15.8 / Zombie Cellar +11.9 / RPG Template +9.5 / Jungle Fever +6.4
/ Op Amazon +6.0 / Disruption +5.7 / Canyon Offensive +4.6 / Island Showdown +4.2 … Aztec GK +0.5
/ **Horseshoe Bend −0.5 (only negative, inside its own drift)**. Raw:
repo `tools/singlequeue_sweep_0809_full.txt`.
★ The Island Showdown max-stall outlier that blocked the flip (3.37→6.71 ms on a 22 s arm) did
NOT reproduce over 40 s arms — it was a lazy-PSO compile.
**Post-flip verified clean-install:** default 234.8/233.2/232.5 (= sweep ON), `singlequeue=0`
202.4/199.9/201.7 (= sweep OFF), screenshots identical (0.019% px, luma −0.00).
**Revert = `setup.ini singlequeue=0` (new key, persistent) or `SET_SINGLEQUEUE 0` (live).**

⚠ **METHOD TRAPS that nearly produced fake results** (both now guarded in `sqfull.sh`):
1. A fixed soak is NOT a settle gate — 90 s captured one demo's first arm at 3.9 FPS / POLYS
   154768 mid-load vs a settled 106 FPS / POLYS 3438876 = a fake **+96%**. Gate on POLYS stable
   AND FPS within 3% of the previous sample.
2. THREE script copies ran concurrently on one MAX → impossible-but-plausible data. See
   [[feedback-leaked-parallel-runner]]. All of it discarded.

⚠⚠ **This INVERTS [[project-performance]] Stage P.6** ("single-queue −4.7 FPS on TESTPRO1,
submission overhead is a dead end, do NOT re-chase"). I expected a light-vs-heavy crossover and
tested for it — **there isn't one, it wins on both**. That verdict was 2026-07-26 on a much
older engine; treat it as STALE, not as law. ★ RULE: queue-structure verdicts have a shelf
life — re-measure after renderer changes instead of citing the old number.
(The "DEFAULT NOT FLIPPED / Island Showdown tail" caveat that stood here is RESOLVED — the
outlier did not survive longer arms, and the flip shipped as 2.17. See the section above.)
★ **BOTH REMAINING GAPS NOW CLOSED (task #131):**
- **TEST-GAME mode** (different queue mix, no ImGui): mean **+5.38%** over 6 scorable cases —
  Trapped **+22.1%**, Switch Escape +6.2%, Foggy Forest +5.0%, Island Showdown +1.7%,
  Horseshoe Bend −0.1%. Gains are at least as good as the editor's.
  ⚠ **Zombie Cellar excluded, not counted neutral: test-game honours the per-level VSync
  setting and pins BOTH arms at exactly 60.0.** ★ RULE: check for a vsync pin before reading
  any in-game FPS A/B — and note a 60 Hz frame IS 16.67 ms, so the over-16.7 ms counter is
  useless there too.
- **TESTPRO1 is NOT an exception.** First 3-arm pass read **−2.5%** and looked like a genuine
  content-dependent regression. 5-arm repeat (0/1/0/1/0) = **+0.2%** (OFF 118.4/119.3/119.6,
  ON 119.8/118.8). The OFF baseline itself moved 122.2 → 119.1 between launches — the same
  magnitude as the "regression". ⚠ **Do not re-raise TESTPRO1 without a 5-arm run**, and treat
  any single 3-arm result near ±2.5% on this rig as unresolved rather than real.

**Net: 26 cases measured (19 editor + 6 test-game + TESTPRO1), ZERO confirmed regressions.**
Raw: repo `tools/singlequeue_testgame_0809.txt`.

## ★★ Earlier finding (now explained by the above): CPU cuts do not convert to FPS here
A **verified 0.25 ms of real main-thread work was deleted and FPS did not move** (201.1 vs
201.2, three arms). The CPU frame absorbed 0.19 ms of the 0.25 as extra wait somewhere else.
Five levers, five zeros:

| lever | delta |
|---|---|
| `SET_RESSCALE` 1.0→0.5 | **+23.6 FPS** — the only mover; pixels ≈ 0.7 ms of frame |
| `SET_HIERLO` 1→0 | −1.2 (so the delta-1.36 fast path IS engaged; that lead is CLOSED) |
| `SET_XINPUT` 60→0 | 0.0 |
| `SET_TREES pool` 6000→1 | 0.0 |
| `SET_HAIRDEPTH` 1→0 | 0.0 |

⚠ **Before doing ANY more CPU optimisation on this level, find what absorbs the slack.**
On this evidence even a successful 1 ms CPU saving would show zero. Same shape as the
Stage P.4 rule in [[project-performance]].
★ **Live lead:** `SUBMIT_PHASES_MS` `stall` read **0.00 three times and 0.38 once** across
today's four captures (`APP_SUBMIT_PRESENT_MS` 0.32/0.45/0.36/0.67 tracking it). One outlier
in four is not proof, but it is the right shape. Cheapest next step = make SUBMIT_PHASES_MS
report **mean+max over N frames** instead of a single snapshot, THEN `RP3D-RenderWait`
(0.30 ms) and the jobsystem waits. ⚠ MY ERROR to avoid repeating: I first wrote "not the
GPU stall" off ONE snapshot reading 0.00 — a single sample of a variable quantity is not an
exoneration.

## Shipped
- **Engine 2.15 XInput throttle** — a genuine DX11-parity regression: `XInputGetState` on an
  EMPTY slot is a driver round-trip and upstream polls all 4 every frame (0.29 ms of a
  4.62 ms frame with no pad attached). **The DX11 fork already had this fix** —
  `WickedRepo/WickedEngine/wiXInput.cpp:18`, `//PE: XInputGetState slow ,so delay if not
  connected.` Connected pads still polled every frame. `SET_XINPUT <frames>`, 0 = stock.
  Verified `Input 0.29 → 0.04`. FPS unmoved (see above) — kept anyway: real work, zero risk.
- **`SET_SCENESERIAL 0|1` instrument.** Scene::Update was un-attributable because every
  `RunXUpdateSystem(ctx)` only DISPATCHES and the cost lands in the one stage-closing
  `jobsystem::Wait`. Serialising gives per-system `SU-*` ranges. First result:
  **SU-Hierarchy 0.81** (pole), SU-Object 0.57, SU-Mesh 0.29, SU-Material 0.11,
  **SU-Transform 0.07** (★ refutes the obvious guess), SU-Animation 0.04, SU-Physics 0.00.
  ⚠ Serialised TOTALS inflate (4.56 → 4.84) — compare shares only.

## ★★★ 2026-08-10: WHY Scene::Update is 11× — investigated, and two recorded findings OVERTURNED
Repo `SWITCHESCAPE_PERF.md` **§10** is THE authority. Read-only sweep of both trees, 31 agents,
26/39 findings survived adversarial verification, 5 load-bearing ones re-verified by hand.

**The 0.21-vs-2.39 comparison is VALID** — every artifact theory died. Both profilers are plain
inclusive begin→end wall clock, no child subtraction (DX11 `wiProfiler.cpp:142`, DX12 `:410`).
⚠ But the ROW LABEL was wrong: **there is no `Scene::Update` range anywhere in DX11.** The 0.21
is `Update - Wicked` (DX11 game `master.cpp:2171`) wrapping ALL of `RenderPath3D::Update` — a
strict SUPERSET of `scene->Update()`. So DX11's scene update is *below* 0.21 and the true delta
is ≈2.25–2.30 ms. The comparison understated the gap; it did not inflate it.

**It is NOT the "modern Wicked infrastructure."** Surfel/DDGI/VXGI/impostor/TLAS/Script/Spring/
Character/Expression/Humanoid all iterate EMPTY arrays here: **<0.01 ms combined**. Both engines
run essentially the same system list. Three real causes:
1. **Bigger N** — 6000 of 7322 objects and 6000 of 8437 transforms are parked tree-pool slots
   (`GGTrees_part2.cpp:284-307`). **DX11 creates ZERO ECS entities for trees** (grep count 0 in
   its `GGTrees.cpp`; draws via `DrawIndexedInstanced`). DX11's scene ≈1322 objects.
2. **The port dropped a GameGuru ECS optimisation** — DX11 `wiECS.h:413` is a flat sparse array
   ("no more lookup.find(entity)"); DX12 is stock `LOOKUP_BUCKET_HASH` (`wiECS.h:446`), a
   hash find on EVERY `Contains`/`GetComponent`. ★ `LOOKUP_SPARSE` is already implemented and
   commented out at DX12 `wiECS.h:445` / `:486-593`.
3. **Genuinely relocated** — the GPU scene mirror (`wiScene.cpp:332-340` 1.87 MB single-threaded
   blanket init + `:5318` per-object 256 B) which DX11 did at draw time, billed to its `Render`.
⚠ **But relocation explains little: DX12's `Render` row is MORE expensive (0.87 vs 0.71), so the
credit never appeared, and DX11's whole CPU frame is 1.69 ms — 2.18 ms cannot hide in it.**

## ★★★ 2026-08-11: THE FLOOR IS WICKED'S NATIVE TERRAIN + a REAL leak (repo §16-18)
User photographed both perf panels, full AND after deleting all objects. Subtracting splits
floor from content and the answer is unambiguous — **the content columns are comparable and DX12
is LOWER on all four; the ENTIRE multiplier is the empty-level floor** (+1030 meshes, +991
materials, +1321 transforms, +1372 hierarchy).
★ **CAUSE: DX12 runs Wicked's native `wi::terrain`; DX11 has NO wiTerrain.cpp/.h at all** (zero
`terrains` refs in its wiScene.h) — its terrain/trees/grass are custom draws creating **ZERO ECS
entities**. `wi::terrain` mints a full entity per chunk + an empty props child per near chunk.
★★ **EXACT cross-check:** generation=14 → (2·14+1)²=841 chunks, prop_generation=10 → 441 props,
so the chunk subtree = 1+841+441 = **1283** — and the HIER instrument had independently measured
`maxSubtree=1283`. Two unrelated routes, same number.
⚠ This REFUTED my "per-limb mesh duplication" guess (§15). Content meshes: DX11 92, DX12 34.

## ★★ 2.22 SHIPPED (game `a1c05af9`): `terrain.prop_density = 0`
441 empty per-chunk "props" scaffolding nodes killed — GG **never populates `terrain.props`**
(grep-verified). Transforms 2437→**1996**, hierarchy 1995→**1554**, maxSubtree 1283→**842**
(=1+841 exactly), SU-Hierarchy 0.47→**0.31**, Scene::Update 1.577→**1.323**. POLYS/objects/meshes
identical. **Sweep CLEAN 19/19** (`tools/sweep_0811_2.22.txt`). ⚠ Watch: worst VRAM headroom
151.2→122.6 MB (Aztec GK) — inside the ±84 MB noise band, but the closest to 4 GB yet.
**Cumulative Scene::Update on Switch Escape: 2.412 → 1.323 ms = −45%.**

## ★★★ LEAK FOUND — THE TREE POOL SURVIVES LEVEL CHANGES (caps my own 2.19 win)
`tools/sceneupdate/suleak.sh` loads the SAME level two ways (no delete-all command exists):
Switch Escape FRESH vs AFTER Island Showdown → **objects 1322→7401, transforms 2437→8516
(+6079 each), POLYS unchanged**. `DUMP_TREEPOOL` names it:
`POOL built=6000 ceiling=6000 bound=6000 renderable=0 numTotalTrees=400000` — all 6000 slots
still BOUND to the previous level's trees, its 400k tree array still resident, on a treeless level.
⚠ **Direct consequence of 2.19's deliberate one-way growth** (right for camera movement, wrong for
level changes). NOT a regression (pre-2.19 was 6000 everywhere always) but **the lazy-pool win
only survives until the session visits its first tree level.**
**FIX NOT DONE:** release slots on LEVEL CHANGE. `GGTrees_WickedShutdown` already does the right
teardown and has zero callers. Gate: POLYS on a tree level + re-run `suleak.sh`, which must then
show FRESH == AFTER. ⚠ A second smaller leak remains: +79 meshes / +17 materials, unexplained.

## Refuted / do-not
- ⚠⚠ **RETRACTED 2026-08-10: the "tree pool is NOT the cost" A/B tested NOTHING.**
  `SET_TREES pool N` writes only `g_treePoolSize` (`AutomationHarness.cpp:5656`), read at build
  time inside `GGTrees_WickedSetup`, latched by `g_wickedTreesSetup`. The latch clears only in
  `GGTrees_WickedInit` (← `GGTerrainWicked_Init`, called ONCE from `GameGuruMain.cpp:171`, app
  startup, **not per level**) and `GGTrees_WickedShutdown` (← `GGTerrainWicked_Shutdown`, which
  has **ZERO callers**). **The pool is built once per process and never rebuilt, not even on
  level load** — both arms ran the same 6000 slots. `SCENE_OBJECTS` staying 7322 is proof the
  KNOB did nothing, not that the pool is free. **And the pool entities ARE in that count** —
  each slot does `objects.Create` + `transforms.Create`. Cost = UNMEASURED, not zero.
  ★ RULE: before trusting any A/B, prove the knob REACHES the thing — a latched one-shot setup
  makes a runtime knob a no-op that still produces plausible-looking numbers.
- ⚠ **The terrain tickbox the user offered was deliberately NOT built.** It already exists
  ("Completely Empty Level", `M-TerrainNew_part5.cpp:2126`, reachable only under theme 8) and
  terrain here already costs 0.14 ms CPU / 0.01 ms GPU with `TERRAIN_DRAW_COUNT 0` — ceiling
  ~3%. Enabling it also forces water/trees/grass off and resizes the universe to 25 km.
- ⚠ **The hub is vsync-locked to exactly 60.0 FPS** — never benchmark there. The editor is
  unlocked (201).
- ⚠ `GET_FPS` is not a command; FPS comes from `GET_PERF_DATA`'s `FPS:` line, which is
  `ImGui::GetIO().Framerate` (a 120-frame rolling average — stable, but smooths fast changes).

## 300 FPS verdict
**Not reachable by incremental tuning.** Closing 1.68 ms needs pixels (≤0.7 ms, resolution
only), CPU items (measured zero), or the structural `Scene::Update` 11× — which is engine
work gated on resolving the slack question first.

Related: [[project-performance]], [[project-demo-fps-baseline]], [[project-transparency-parity]].
