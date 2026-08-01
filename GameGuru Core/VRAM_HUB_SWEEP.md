# VRAM sweep — every hub demo, opening scene

Measured 2026-08-01 on engine `2b9b989f` / game `c51b0d56` (i.e. *after* the dead-VT and
raytracing-copy fixes, and with the grass coverage change reverted). Each demo gets a **cold
launch** so its reading is not contaminated by the previous level; level opened in the editor,
35 s settle, then `GET_PERF_DATA` + `DUMP_VRAM`. Reproduce with the sweep script pattern in
`tools/` plus `DUMP_VRAM <tag>`.

"driver" is what the FPS panel shows as VRam — the number a user actually feels.

## Ranked by driver VRAM

| # | Demo | driver MB | census MB | hair systems | strands | polys | FPS |
|---|---|---|---|---|---|---|---|
| 1 | The Mystery of Z Island | **10087** | 8536 | 185 | 12,432,000 | 704,717 | 71.9 |
| 2 | Aztec Game Kit | **8117** | 6698 | 64 | 4,104,000 | 3,438,876 | 78.0 |
| 3 | Jungle Fever | **7651** | 5852 | 63 | 6,300,000 | 74,124 | 99.9 |
| 4 | Canyon Offensive | **7002** | 5310 | 62 | 5,298,000 | 8,816,163 | 59.9 |
| 5 | Operation Amazon | 6513 | 4832 | 26 | 2,354,000 | 5,496,922 | 86.2 |
| 6 | Foggy Forest | 6469 | 4920 | 30 | 3,000,000 | 10,195,894 | 59.9 |
| 7 | River Raiders | 5704 | 4076 | 23 | 1,726,000 | 1,906,072 | 112.4 |
| 8 | Indian Strike Force | 5690 | 3808 | 0 | 0 | 3,184,527 | 105.7 |
| 9 | Aztec Game Kit Teaser | 5650 | 4026 | 48 | 2,335,000 | 10,313,511 | 66.0 |
| 10 | Disruption | 5493 | 3602 | 6 | 436,000 | 4,665,184 | 88.2 |
| 11 | Bounty | 5384 | 3778 | 0 | 0 | 463,210 | 119.9 |
| 12 | Snowy Mountain Stroll | 5324 | 3516 | 7 | 700,000 | 81,081 | 131.8 |
| 13 | Horseshoe Bend | 5284 | 3368 | 0 | 0 | 2,133,269 | 95.7 |
| 14 | Island Showdown | 5240 | 3644 | 34 | 1,350,000 | 4,115,636 | 66.6 |
| 15 | A Grand Canyon Adventure | 5139 | 3447 | 48 | 1,942,000 | 2,272,361 | 102.7 |
| 16 | Escape from the Zombie Cellar | 4671 | 2925 | 0 | 0 | 28,048 | 139.4 |
| 17 | Switch Escape | 4645 | 2780 | 0 | 0 | 109,358 | 142.6 |

**Trapped** and **RPG Template** never reached the editor — see the failures note at the bottom.

## Category breakdown (MB)

| Demo | grass | SVT pool | mesh alloc | content tex | arrays | shadow-T | chunk maps | other |
|---|---|---|---|---|---|---|---|---|
| zisland | **4297** | 768 | 512 | 979 | 220 | 384 | 115 | 1261 |
| jungle | **2403** | 768 | 512 | 595 | 220 | 320 | 168 | 866 |
| aztec | **1919** | 768 | 1024 | 1114 | 220 | 320 | 115 | 1219 |
| canyonoff | **1883** | 768 | 512 | 781 | 220 | 160 | 115 | 871 |
| foggyforest | 1097 | 768 | 512 | 486 | 220 | **512** | 115 | 1210 |
| aztecteaser | 1041 | 768 | 512 | 478 | 220 | 160 | 115 | 732 |
| amazon | 963 | 768 | 512 | 754 | 220 | **512** | 115 | 987 |
| riverraiders | 728 | 768 | 512 | 722 | 220 | 160 | 115 | 851 |
| island | 602 | 768 | 512 | 452 | 220 | 160 | 168 | 762 |
| canyonadv | 503 | 768 | 512 | 442 | 220 | 160 | 115 | 727 |
| snowy | 333 | 768 | 512 | 440 | 220 | 320 | 115 | 808 |
| disruption | 208 | 768 | 512 | 352 | 220 | **512** | 115 | 915 |
| indian | 0 | 768 | 512 | 918 | 220 | 256 | 115 | 1018 |
| horseshoe | 0 | 768 | 768 | 476 | 220 | 160 | 115 | 861 |
| bounty | 0 | 768 | 512 | 521 | 220 | **512** | 115 | 1130 |
| cellar | 0 | 768 | 512 | 359 | 220 | 160 | 115 | 791 |
| switch | 0 | 768 | 512 | 312 | 220 | 160 | 115 | 692 |

## What the numbers say

**1. There is a hard floor of ~1.6 GB that every level pays, including a cellar.**
`SVT pool 768 + arrays 220 + chunk maps 115 + mesh allocator 512` is present and *identical* on
all 17 demos. Switch Escape draws 109 k polys with no grass and still needs 4.6 GB. Attacking
this floor improves **every** project in the product, not just the heavy ones.

- **SVT tile pool: 768 MB, invariant, on all 17.** Biggest single universal item. Measured
  residency is only ~26% of the atlas; `svtatlasheight=8192` halves it (−384 MB *per demo*,
  ~6.5 GB across the hub). Already wired, default off, needs a fast-travel soak.
- **Source texture arrays: 220 MB, invariant, on all 17.** At least **61 MB of that is
  `texGrass`, which is never written and never read** (all writers are disabled stubs; the only
  binder is the dead legacy draw path). That part is free.
- **Chunk maps: 115 MB** (168 on two) — terrain blend/wet/height maps, 841 chunks.

**2. Grass is the entire spread between demos.** 0 MB on six of them, 4.3 GB on Z Island. It
alone explains why #1 is 10 GB and #17 is 4.6 GB. Total across the hub: **~16 GB**. This is the
single biggest prize and the retry design (one hair system per chunk, bit-identical placement)
is specified in `VRAM_CENSUS.md`.

**3. The transparent shadow atlas is bigger than assumed on several demos** — 512 MB on Foggy
Forest, Operation Amazon, Disruption and Bounty (vs 160 on Island Showdown; it scales with the
shadow packer). RGBA16F → RGBA8 halves it, so on those four it is worth **256 MB each**, and it
applies to levels with no grass at all. Caveat unchanged: the alpha channel carries a depth value
compared against the shadow test, so confirm nothing actually renders into that atlas first.

**4. Polygons and VRAM are unrelated.** Aztec Teaser draws 10.3 M polys in 5.6 GB; Jungle Fever
draws 74 k in 7.7 GB. Do not use scene complexity as a VRAM proxy — measure.

## Suggested targeting order

| Target | Per-demo | Hub-wide | Risk |
|---|---|---|---|
| Grass one-system-per-chunk | up to −3 GB | ~−10 GB | design proven bit-identical; must pass the `GRASS_BENCHMARK.md` gate |
| SVT atlas halving | −384 MB × 17 | ~−6.5 GB | needs fast-travel soak; switch already exists |
| `texGrass` dead array | −61 MB × 17 | ~−1 GB | none — provably never written or read |
| Transparent shadow atlas RGBA8 | −80 to −256 MB | ~−3 GB | verify the alpha-depth test first |
| Mesh suballocator granularity | −0 to 512 MB | varies | fragmentation-dependent |

## Load failures found by the sweep

**Trapped** and **RPG Template** did not reach the editor. Both were retried individually; MAX
was no longer running afterwards, which points at a crash during level load rather than a slow
load. Diagnosed separately — see the session notes. These two are a correctness bug, not a VRAM
result, and should be treated as their own issue.
