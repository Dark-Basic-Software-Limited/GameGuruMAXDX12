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
- **Not GPU end-of-frame stall.** `SUBMIT_PHASES_MS` reads `stall=0.00`.
- **Not measurement noise.** Three arms, five samples each, drift +0.2 FPS.

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

1. **Resolve §5 first — find what absorbs CPU slack.** Instrument `RP3D-RenderWait` (0.30 ms)
   and the jobsystem waits; until a CPU cut demonstrably converts, no CPU work is worth doing.
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
