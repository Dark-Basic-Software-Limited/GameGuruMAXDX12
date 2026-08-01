# Demo FPS sweep — every hub demo, editor + in-game

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
