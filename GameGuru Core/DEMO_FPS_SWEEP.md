# Demo FPS sweep — every hub demo, editor + in-game

## 2026-08-06 SWEEP v4 — engine `a229fffe` / game `50ca7c28` (the 2.07g light/shadow build)

**Run TWICE back to back** (22:13 and 23:03) to separate signal from scatter: sum +0.5% between
the two runs and **no demo moved 10%** — so the deltas below are real. Sweep now costs ~50 min,
not the ~2h50m of the v3 runs, because levels load far faster than they did on 08-01.

| Demo | ed 08-01 | ed 08-06 | Δ% | game 08-01 | game 08-06 | Δ% | VRAM 08-01 | VRAM 08-06 | ΔMB | Mpolys |
|---|---|---|---|---|---|---|---|---|---|---|
| Foggy Forest | 60.4 | **63.4** | +5.1 | 69.2 | 68.9 | −0.5 | 6532 | 3085 | −3447 | 10.20 |
| Aztec Game Kit Teaser | 63.3 | **68.2** | +7.6 | 59.9 | 64.2 | +7.1 | 5666 | 2984 | −2682 | 10.31 |
| Island Showdown | 67.6 | **72.9** | +7.8 | 76.8 | 84.4 | +9.8 | 5192 | 2716 | −2476 | 4.11 |
| Canyon Offensive | 59.9 | **72.9** | +21.7 | 60.0 | 74.6 | +24.4 | 7002 | 3348 | −3654 | 8.82 |
| Disruption | 90.0 | 85.4 | −5.1 | 98.5 | 95.9 | −2.6 | 5493 | 3002 | −2491 | 4.67 |
| Operation Amazon | 88.3 | 85.9 | −2.7 | 107.7 | 98.5 | −8.5 | 6528 | 3551 | −2977 | 5.50 |
| Horseshoe Bend | 95.6 | 94.0 | −1.6 | 70.4 | 71.8 | +2.0 | 5284 | 3064 | −2220 | 2.11 |
| Aztec Game Kit | 74.7 | **95.1** | +27.3 | 76.9 | 80.4 | +4.5 | 8117 | 3607 | −4510 | 3.44 |
| RPG Template | 75.7 | **99.1** | +30.9 | 65.0 | 82.4 | +26.8 | 6159 | 3195 | −2964 | 3.24 |
| Indian Strike Force | 107.6 | 102.5 | −4.8 | 98.8 | 100.9 | +2.1 | 5706 | 3057 | −2648 | 3.18 |
| A Grand Canyon Adventure | 106.1 | 102.8 | −3.1 | 110.9 | 107.4 | −3.2 | 5139 | 2811 | −2328 | 2.27 |
| River Raiders | 115.8 | 111.5 | −3.7 | 100.4 | 94.5 | −5.8 | 5655 | 3297 | −2359 | 1.91 |
| The Mystery of Z Island | 74.9 | **112.4** | +50.1 | 81.9 | 107.8 | +31.6 | 10038 | 3390 | −6648 | 0.70 |
| Bounty | 119.9 | 117.6 | −1.9 | 139.7 | 119.5 | −14.5 | 5335 | 2677 | −2658 | 0.46 |
| Snowy Mountain Stroll | 136.7 | 119.8 | −12.3 | 97.7 | 97.9 | +0.2 | 5372 | 2745 | −2628 | 0.08 |
| Jungle Fever | 101.3 | **123.8** | +22.2 | 96.7 | 119.1 | +23.2 | 7603 | 2949 | −4654 | 0.07 |
| Escape from the Zombie Cellar | 144.9 | 127.1 | −12.3 | 59.9 † | 59.9 † | 0.0 | 4624 | 2588 | −2035 | 0.03 |
| Trapped | 154.3 | 128.2 | −16.9 | 149.1 | 134.3 | −9.9 | 4813 | 2543 | −2271 | 0.01 |
| Switch Escape | 149.8 | 129.3 | −13.7 | 141.2 | 121.7 | −13.8 | 4645 | 2393 | −2252 | 0.11 |

Editor sum 1886.9 → **1912.0 (+1.3%)**, median 102.5, field 63.4–129.3. † = in-game 60 cap.

### ★ EVERY DEMO NOW FITS 4 GB AT DEFAULTS — max 3607 MB, median 3002 MB

Zero demos over 4096 MB, with **no low-VRAM preset and no per-level checkbox**. Every level
dropped 2.0–6.6 GB versus 08-01: Z Island **10038 → 3390** (−6.6 GB, and it was 13.0 GB in July),
Aztec Game Kit −4.5 GB, Jungle Fever −4.7 GB. That is the 08-02→08-04 campaign (SVT atlas
default, merged grass, lazy PSOs, on-demand legacy atlases) showing up across the whole hub.
The 4 GB min-spec goal is met by the DEFAULT configuration, not by a fallback path.

### The limiter is geometry, harder than ever: Spearman −0.95 (polys) vs −0.53 (VRAM)

The 08-01 sweep measured −0.85 / −0.66. Now that VRAM has been cut roughly in half across the
board and framerate did NOT follow it, the correlation with bytes has weakened while the
correlation with triangles has tightened to almost perfectly monotonic. **The three slowest
levels are still the three highest-poly levels.** VRAM work buys headroom and load speed —
not frames. To move Foggy Forest (10.2 M tris) attack draw/geometry, not memory.

### The four "regressions" are the 30 s soak reading a WARM-UP, not a loss

Trapped −16.9%, Switch Escape −13.7%, Zombie Cellar −12.3%, Snowy −12.3% — all four are the
*fastest, lightest* levels in the hub, which is the signature of a fixed per-frame cost, and
both runs reproduced them. They are a measurement artifact:

| Level | 30 s (what the sweep samples) | 180 s | 300 s | 08-01 |
|---|---|---|---|---|
| Trapped | 132.0 | 148.0 | — | 154.3 |
| Switch Escape | 122.4 | 135.5 | 134.4 | 149.8 |
| Escape from the Zombie Cellar | 119.9 | 132.7 | 132.7 | 144.9 |

All three gain 10-12% between 30 s and 180 s and are flat by 300 s — the same shape on three
unrelated levels.

**Lazy object PSOs went default-ON (engine 1.82) AFTER the 08-01 baseline.** PSOs now compile on
first use, so a level keeps getting faster for ~2-3 minutes; the 08-01 run's 30 s soak sampled a
fully-built pipeline, tonight's samples mid-warm-up. Steady state lands within ~10% of 08-01,
which is inside this rig's ±15% day-to-day editor drift. Heavy levels hide the same warm-up
because ~1 ms is noise on a 12 ms frame.

**If you need numbers comparable with pre-08-03 baselines, raise the soak to 180 s** (+~48 min on
the run). The 30 s figure is not wrong — it is what a user feels shortly after a level opens —
but it is not steady state any more.

### Horseshoe Bend "3 FPS in game" — the fake collapse came back, with a new mask

Run A recorded 3.0 FPS in game. Not a perf bug and not the 07-31 case either: the screenshot
shows **"RASTERIZING NAVIGATION MESH — 73\100 Complete"**. The v3 gate only waits out
"PREPARING TEST LEVEL", and worse, **`GET_SCREEN_TEXT` cannot see the navmesh banner at all**
(it is `printscreenprompt`, not ImGui text), so the gate returns 0 s and samples a loading
screen. Real figure once it finishes: **71.8 FPS** (08-01: 70.4).

v4a therefore gates on **FPS recovery**, not text: any sub-20 FPS reading triggers a poll until
the level climbs out (no hub demo runs below 20 in real gameplay), then re-samples. Both the
widened text pattern and the FPS poll are in `tools/demo_fps_sweep.sh`.

Open question, load-time only: Horseshoe rasterized its navmesh on BOTH runs an hour apart,
where 08-01 recorded an instant cached start. The cache is alive
(`Documents\GameGuruApps\GameGuruMAX\Files\navcache`) but sits **exactly at its 20-file cap**
with 17 of 20 entries written by tonight's two sweeps — so with 19 demos it may simply be
thrashing its own prune limit, or the vertex-soup hash is not stable across runs. Worth one
two-visit test.

### Also measured: the selection-outline idle gate (game-side, this build)

The sweep spotted that every demo gained exactly one command list versus 08-01 (14→15, 16→17,
18→19) — the outline restore (08-05) runs three mask draws on their own command list plus three
`Postprocess_Outline` composites every editor frame, selected or not. Gating that on "is
anything highlighted" (upstream Wicked's condition, left commented out in the port) is worth
**0.055 ms/frame on Trapped** (148.0 vs 146.8, lists 15→14) — real but small; the initial
attribution of the light-level drop to this pipeline was WRONG, the warm-up above is the
explanation. Kept anyway: it removes six full-screen passes per idle frame, which is worth more
on a min-spec GPU than on this one. Toggle with `OUTLINE_GATE 0|1`.

---

## 2026-08-01 sweep (previous baseline)

Measured 2026-08-01 on engine `53481336` / game `0627f986` (texture streaming ON by default).
Reproduce with `tools/demo_fps_sweep.sh`. Method matches the 07-29/07-30/07-31 baselines: cold
launch per demo → first level → 30 s soak at the start camera → 3 editor samples 4 s apart →
`CLICK test_level` → **wait for the PREPARING overlay to clear** → 15 s settle → 3 game samples.
Full run takes ~2h50m (~9 min per demo).

## Ranked by editor FPS (slowest first)

| # | Demo | editor | in-game | Mpolys | VRAM GB |
|---|---|---|---|---|---|
| 1 | **Canyon Offensive** | **59.9** | 60.0 †| 8.82 | 6.84 |
| 2 | **Foggy Forest** | **60.4** | 69.2 | 10.20 | 6.38 |
| 3 | **Aztec Game Kit Teaser** | **63.3** | 59.9 †| 10.31 | 5.53 |
| 4 | **Island Showdown** | **67.6** | 76.8 | 4.12 | 5.07 |
| 5 | **Aztec Game Kit** | **74.7** | 76.9 | 3.44 | 7.93 |
| 6 | The Mystery of Z Island | 74.9 | 81.9 | 0.70 | 9.80 |
| 7 | RPG Template | 75.7 | 65.0 | 3.24 | 6.01 |
| 8 | Operation Amazon | 88.3 | 107.7 | 5.50 | 6.38 |
| 9 | Disruption | 90.0 | 98.5 | 4.67 | 5.36 |
| 10 | Horseshoe Bend | 95.6 | 70.4 | 2.13 | 5.16 |
| 11 | Jungle Fever | 101.3 | 96.7 | 0.07 | 7.43 |
| 12 | A Grand Canyon Adventure | 106.1 | 110.9 | 2.27 | 5.02 |
| 13 | Indian Strike Force | 107.6 | 98.8 | 3.18 | 5.57 |
| 14 | River Raiders | 115.8 | 100.4 | 1.91 | 5.52 |
| 15 | Bounty | 119.9 | 139.7 | 0.46 | 5.21 |
| 16 | Snowy Mountain Stroll | 136.7 | 97.7 | 0.08 | 5.25 |
| 17 | Escape from the Zombie Cellar | 144.9 | 59.9 †| 0.03 | 4.52 |
| 18 | Switch Escape | 149.8 | 141.2 | 0.11 | 4.54 |
| 19 | Trapped | 154.3 | 149.1 | 0.01 | 4.70 |

Editor: min 59.9, median 95.6, max 154.3. **† = pinned at the in-game 60 FPS cap.**

## Rank on EDITOR FPS — the in-game column caps at 60

Three demos read exactly 59.9 in game. That is a cap, not a ceiling: **Zombie Cellar runs 144.9
in the editor and still reads 59.9 in game.** The editor is uncapped (Trapped hits 154), so it is
the only comparable metric across the whole set. For the three capped demos the in-game figure
means "at or below 60" and nothing more.

## The limiter is GEOMETRY, not memory

Across all 19 demos, Spearman correlation with editor FPS:

| Predictor | correlation |
|---|---|
| **polygon count** | **−0.85** |
| VRAM | −0.66 |

The three slowest levels are the three highest-polygon levels (8.8M / 10.2M / 10.3M triangles).
The counter-examples are decisive: **Z Island carries the most VRAM in the hub (9.8 GB) and still
runs 74.9 FPS on 0.70M polys**, and **Jungle Fever holds 7.4 GB at 101 FPS on 0.07M polys**.
Foggy Forest is slow on a merely mid-pack 6.4 GB.

**Implication for the VRAM work:** the streaming/census reductions buy memory headroom and
loading smoothness, *not* framerate. To move these five, target draw and geometry load — LOD
aggressiveness, occlusion culling, instancing on the dense architectural sets — not bytes.

## Versus the 07-31 sweep: no regressions

Every demo flat or faster; **sum 1795 → 1887, +5.1%**. Biggest gains RPG Template +13.0%,
Trapped +9.1%, Jungle Fever +8.6%, Canyon Offensive +7.9%, Switch Escape +7.6%. Only movement
the other way is Aztec Teaser at −1.1%, well inside noise.

Treat cross-day absolutes with care — editor FPS drifts ±15-20% day to day (see
[[project-demo-fps-baseline]]); a uniform positive shift like this is consistent with both real
improvement and ambient drift. The safe claim is **nothing regressed**.

## The PREPARING gate (why v3 exists)

On 07-31 this sweep recorded **Horseshoe Bend at 3.7 FPS in game** and cost a night chasing a
phantom. It was sampling during the `PREPARING TEST LEVEL - N/100` overlay — a loading screen,
not gameplay. `tools/demo_fps_sweep.sh` now polls `GET_SCREEN_TEXT` until the overlay clears
before sampling, and records the wait. Horseshoe now reads a healthy **70.4**, and prep is 0 s on
every demo (the navmesh cache, game `9c42a455`, did its job).

**Never sample in-game FPS without that gate.**
