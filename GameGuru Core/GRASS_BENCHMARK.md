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
