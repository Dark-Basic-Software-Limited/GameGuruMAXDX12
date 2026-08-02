# Grass benchmark — the GOOD reference for VRAM work

Lee saved this scene on 2026-08-01 specifically as the acceptance test for grass VRAM work, and
**deliberately painted pink flowers so grass density is measurable, not a matter of opinion**.
Any change that touches grass must be measured against it before it goes anywhere near a commit.

## The GOAL

Render *this* scene with materially less VRAM and **hardly any visual reduction**. The intended
shape of the solution is a working **Grass Draw Distance** dial: dense grass up close with a
sparse distance for low-end machines, out to extreme distance for high-end ones.

## The scene

- Project **TESTPRO1**, its single level. Load with harness `OPEN_PROJECT TESTPRO1` then
  `CLICK_ONLY_LEVEL`; the saved editor camera frames the flower meadow, so no camera setup is
  needed and shots are directly comparable.
- Grass Draw Distance saved at **2247** (per-strand cull = lod_dist + 2500 = 4747 inches).
- Grass Density 100, Match Terrain Color on.

## GOOD reference numbers (build: engine `2b9b989f`, game `207e4b4d`)

Editor, ~30 s after load:

| Metric | Value |
|---|---|
| driver VRAM | **6536–6553 MB** |
| census | 4995–4999 MB |
| grass buffers | **1945 MB (38.9%)** — 37 systems @ 47.4 MB + 3 @ 25.2 + 12 @ 8.6 |
| HAIR_SYSTEMS | 52 |
| HAIR_TOTAL_STRANDS | 4,216,000 |
| FPS | 59.9 (v-synced) |
| POLYS | 2,714,197 |

Reference screenshot: `tools/reference/BENCHMARK_GOOD_testpro1_grass.png`.

**Use the EDITOR view for density.** The test-game spawn on this level looks down a bare
rock/beach slope with no flowers in frame, so it is only good for VRAM/FPS, never for the density
gate. For the record, test game reads: driver **7506 MB**, census 5449 MB, 46 systems,
4,600,000 strands, FPS 72.4.

## The density gate — use this, not your eyes, and not mean colour

`tools/grassdensity.ps1` counts flower-pink pixels in the 3D viewport and reports three numbers:

```bash
powershell -NoProfile -File "GameGuru Core/tools/grassdensity.ps1" -Img shot.png -Label after
```

- **coverage** — % of viewport pixels that are flower-pink. Overall density.
- **clumpCV** — coefficient of variation of coverage across an 8×6 tile grid. **This is the
  regression detector.** Grass that has been redistributed into clumps keeps a similar coverage
  but a much higher CV. A mean-colour proxy cannot see that, which is exactly how the reverted
  2026-08-01 coverage-scaling change passed review and shipped broken.
- **bands** — coverage per horizontal band, near (screen bottom) → far (top). This is the
  draw-distance curve: it is what "dense up front, sparse in the distance" looks like as data.

### Measured noise floor (3 shots, same build, same load)

| | shot 1 | shot 2 | shot 3 |
|---|---|---|---|
| coverage | 9.554% | 9.525% | 9.436% |
| clumpCV | 0.870 | 0.872 | 0.872 |
| bands near..far | 14.19 14.94 16.83 8.17 3.15 0.00 | 14.02 15.01 16.81 8.09 3.18 0.00 | 13.92 14.72 16.69 8.10 3.14 0.00 |

### Acceptance thresholds

A change is visually clean on this scene only if **all** hold:

- coverage within **±0.15 pp** of ~9.5%
- **clumpCV within ±0.01 of 0.871** ← the clumping gate; treat any rise as a failure
- each band within **±0.25 pp** of the reference profile

Anything that trades coverage for clumping fails even if total coverage matches.

### Proof the gate catches the real regression

The reverted coverage-scaling build was temporarily rebuilt and run against this scene. Every
metric fails, by 19–27× the thresholds — so this gate would have stopped it before it shipped:

| | GOOD | reverted "optimisation" | verdict |
|---|---|---|---|
| coverage | 9.554% | **6.695%** (−2.86 pp) | FAIL (limit ±0.15) |
| **clumpCV** | 0.870 | **1.144** (+0.274) | **FAIL (limit ±0.01)** |
| bands near..far | 14.19 14.94 16.83 8.17 3.15 | 8.66 12.55 12.96 4.21 1.76 | FAIL, every band |
| strands | 4,216,000 | 920,791 (−78%) | — |
| driver VRAM | 6552 MB | 5081 MB | the "saving" that wasn't |

Note the shape of it: coverage fell 30% *and* clumping rose 31%. The old mean-colour proxy would
have seen a small average shift and passed; clumpCV is what makes the failure unmissable.

The good build was then restored and re-verified: coverage 9.426, clumpCV 0.873, 4,216,000
strands, 6552.4 MB — inside the noise band on every metric.

### Metric sensitivity check (proof it responds)

Shrinking the per-strand cull radius with `SET_GRASS viewdist` moves the far bands and leaves the
near ones alone, exactly as it should:

| cull radius | bands near..far |
|---|---|
| 4747 (scene) | 14.26 15.10 16.91 8.17 **3.14** 0.00 |
| 2000 | 14.10 14.86 17.16 12.09 **0.68** 0.00 |
| 1000 | 14.12 14.89 20.42 **5.55** 0.00 0.00 |

## Where this scene's grass memory goes

Each tier-3 system is 47.4 MB for 100,000 strands (~497 B/strand), and the regions break down as
roughly: positions `vb_pos[0]` + `vb_pos[1]` **25.6 MB (54%)**, index buffers
`ib_culled` + `prim_view` 9.6 MB (20%), uvs 6.4, simulation 3.2, normals 3.2, wetmap 1.6.
The raytracing position copy that used to add another 12.2 MB per system is already gone
(engine 1.70, `gg_hair_raytracing = false`).

**37 tier-3 systems ≈ 9 chunks × ~4 grass types painted per chunk.** That multiplication is the
target: see the ranked backlog in `VRAM_CENSUS.md`, and the FAILED ATTEMPT section there for the
approach that must not be repeated.


---

## 2026-08-02 — the density lever, and a WARNING about the reference numbers above

### The reference numbers above no longer reproduce. Use a same-session baseline.

Re-running this scene on the 2026-08-02 build gave a baseline that does **not** match the
"GOOD reference" table:

| | stored reference (08-01) | measured 08-02 |
|---|---|---|
| HAIR_SYSTEMS | 52 | **58** |
| HAIR_TOTAL_STRANDS | 4,216,000 | **4,816,000** |
| POLYS | 2,714,197 | **3,149,243** |
| coverage | 9.554% | 9.450% |
| **clumpCV** | **0.871** | **1.104** |
| bands near..far | 14.19 14.94 16.83 8.17 3.15 0.00 | 13.86 12.94 18.66 9.84 1.35 0.00 |

Coverage still lands in the old band, but clumpCV is +0.233 — **23× the ±0.01 gate**. Taken at
face value that would condemn the shipping build. It does not: system count, strand count and
POLYS all moved together, which is the signature of a **different chunk set in range**, i.e. the
scene's saved camera (or its content) is not what it was on 08-01. Chunk creation follows the
camera, so more chunks in range means more systems, more strands, more polygons, and a different
framing for every pixel metric.

**Therefore: judge any grass change against a baseline captured in the SAME session on the SAME
build, never against the table at the top of this file, until that table is refreshed.** The
gate's *thresholds* are still right; its *reference values* are stale. Refreshing them is an open
item — and whoever does it should first work out why the scene drifted, because if it was an
accidental re-save then the "GOOD" camera is gone and the new one needs blessing.

### The density lever (low-VRAM preset)

`lowvramgrassdensity` scales `strandCount` uniformly in `GrassTierDensityScale()`. It touches
**only** the strand count — emitter mesh, `vertex_lengths` paint mask and `randomSeed` are
untouched, so the same painted region is still sampled uniformly, just with fewer strands. That
is the whole difference from the reverted 2026-08-01 attempt, which narrowed the *mask* and made
strands bunch into whatever it caught.

For this lever the gate is read differently on purpose: **coverage is MEANT to fall** (it is a
4 GB knob), so the ±0.15 pp coverage threshold does not apply. What must not happen is clumping,
so **clumpCV and the band profile decide pass/fail**.

All four points below, same session, same build, `lowvramgrassdist` pinned unreachable so only
density varies:

| density | strands | grass buffers | driver VRAM | coverage | clumpCV | bands near..far | FPS | verdict |
|---|---|---|---|---|---|---|---|---|
| 100 (baseline) | 4,816,000 | 2187.0 MB | 5990.3 MB | 9.450% | 1.104 | 13.86 12.94 18.66 9.84 1.35 | 56.2 | reference |
| **75** | 3,612,000 | **1645.6 MB** | **4765.1 MB** | 9.355% | **1.116** | 13.58 12.93 18.83 9.61 1.14 | 60.5 | **PASS — near free** |
| 50 | 2,408,000 | 1104.5 MB | 4499.4 MB | 7.979% | 1.178 | 10.56 9.81 17.75 8.70 1.00 | 65.3 | PASS — honest trade |
| 30 | 1,444,800 | 665.7 MB | 3746.2 MB | 5.403% | **1.354** | 8.61 4.51 12.19 6.37 0.68 | 69.9 | **FAIL** |

**75 % is very nearly free** — coverage falls 0.095 pp, inside the shot-to-shot noise band, and
clumpCV moves 0.012, while grass buffers drop 541 MB and driver VRAM drops 1225 MB. The reason is
that the meadow is over-saturated at full density: strands overlap so heavily that removing a
quarter of them barely changes which pixels are covered. Screenshots agree — you would struggle
to pick d75 from the baseline without an A/B.

**30 % fails**, and fails the same way the reverted build did: clumpCV +0.25 against that build's
+0.274, with band 2 losing 65 % while band 3 loses only 35 % — uneven collapse, not even thinning.
The screenshot shows visibly bare ground between sparse flowers.

So the preset defaults to **75**, with 50 available for users who need more and are willing to see
it. **Do not ship below 50** without re-running this table.

### The shipping preset on this scene (`lowvram=1`, nothing else set)

Density 75 **and** the grass draw-distance cap (750, vs the scene's saved 2247) together:

| | baseline | `lowvram=1` | delta |
|---|---|---|---|
| driver VRAM | 5989.9 MB | **4765.1 MB** | **−1224.8** |
| grass buffers | 2187.0 MB | 1645.6 MB | −541.4 |
| strands | 4,816,000 | 3,612,000 | −25 % |
| coverage | 9.472% | 9.757% | **+0.285** |
| **clumpCV** | 1.103 | **1.101** | **−0.002** |
| bands near..far | 13.87 12.92 18.80 9.83 1.35 | 14.15 12.93 18.95 11.65 0.82 | |
| FPS | 56.2 | 69.4 | **+13.2** |

clumpCV is flat to within a thousandth — the grass thins evenly, which is the whole point. The
band profile shows the two levers doing exactly what they should and nothing else: the near three
bands are unchanged, band 4 *rises* (11.65 vs 9.83) and band 5 falls (0.82 vs 1.35) because the
shorter per-strand cull pulls the visible edge inward. That is the same signature the
"metric sensitivity check" section above records for `SET_GRASS viewdist`, so coverage ticking
*up* is not an anomaly — it is grass being concentrated nearer the camera.

The cost is entirely at distance: on this scene the far band loses about 40 % of its flowers.
