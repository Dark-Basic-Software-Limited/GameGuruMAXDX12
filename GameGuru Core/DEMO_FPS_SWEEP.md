# 2026-08-23 SWEEP (0823) - FAR-TREE BILLBOARDS + POOL CAP - game `42277a53`

Gate for 2.96 (distant-tree billboards restored) and 2.97 (tree pool radius cap).

VERDICT: **19/19, zero failures, hub-wide +43.1%. ⚠ POLYS deliberately CHANGED on 13 of 19 -
the standing bit-identical test does not apply to this feature and needs replacing.
⚠⚠ WORST IN-GAME VRAM 4026.6 MB on Aztec Game Kit - 69 MB under the 4096 gate.**

| Demo | 0822a | 0823 | delta | ed VRAM | POLYS 0822a | POLYS 0823 |
|---|---|---|---|---|---|---|
| Aztec Game Kit Teaser | 83.1 | 120.0 | +44.4% | 3042 | 10,330,135 | 6,458,677 |
| Aztec Game Kit | 121.8 | 160.1 | +31.5% | 3746 | 3,438,876 | 527,382 |
| Bounty | 158.5 | 227.7 | +43.7% | 2641 | 469,906 | = |
| Horseshoe Bend | 124.9 | 142.1 | +13.8% | 3123 | 2,168,281 | 1,583,122 |
| Island Showdown | 90.3 | 139.8 | +54.8% | 2831 | 4,125,704 | 1,655,768 |
| Operation Amazon | 103.8 | 174.6 | +68.3% | 3419 | 5,504,271 | 507,604 |
| River Raiders | 170.3 | 264.3 | +55.2% | 3404 | 2,362,345 | 267,366 |
| Snowy Mountain Stroll | 188.9 | 226.5 | +19.9% | 2835 | 81,369 | = |
| A Grand Canyon Adventure | 139.4 | 167.8 | +20.3% | 2902 | 2,279,506 | 2,132,292 |
| Disruption | 108.3 | 177.3 | +63.7% | 2947 | 4,677,579 | 153,309 |
| **Foggy Forest** | 74.6 | **155.9** | **+109.1%** | 3032 | 10,220,589 | 1,283,316 |
| Indian Strike Force | 127.2 | 182.6 | +43.5% | 3052 | 3,229,699 | 303,737 |
| Switch Escape | 251.1 | 318.7 | +26.9% | 2276 | 109,358 | = |
| Canyon Offensive | 91.6 | 156.3 | +70.7% | 3406 | 8,838,008 | 517,948 |
| Escape from the Zombie Cellar | 244.9 | 306.9 | +25.3% | 2345 | 28,048 | = |
| Jungle Fever | 179.2 | 223.4 | +24.7% | 3031 | 76,157 | = |
| RPG Template | 140.6 | 214.2 | +52.4% | 3174 | 3,247,629 | 552,000 |
| The Mystery of Z Island | 147.4 | 191.8 | +30.1% | 3384 | 722,872 | 326,002 |
| Trapped | 279.5 | 335.0 | +19.9% | 2346 | 12,768 | = |

## The shape confirms the mechanism
The biggest gains are on the demos with the most tree geometry: Foggy Forest +109% (10.2 M ->
1.28 M polys), Canyon Offensive +71% (8.8 M -> 0.52 M), Operation Amazon +68% (5.5 M -> 0.51 M).
Levels with no trees to cap are flat: Trapped, Switch Escape, Zombie Cellar, Jungle Fever and
Snowy Mountain Stroll all show POLYS unchanged and gains in the 20-27% band that is just the
2.94d/2.94f terrain work already banked. **Six demos have bit-identical POLYS, and all six are
the treeless ones** - which is exactly the control this table needed.

## ⚠ Two things this gate does NOT clear

**1. POLYS bit-identical is no longer a valid acceptance test for tree work.** It changed on 13
of 19 by design - the cap removes near-field mesh triangles the billboards now cover. A
replacement gate is needed: POLYS-excluding-far-trees plus a separate billboard instance count.
Until then, tree changes have no automatic regression test.

**2. Aztec Game Kit in-game VRAM is 4026.6 MB against a 4096 gate - 69 MB of headroom.**
It was 3947.9 before today, so the ~78 MB billboard atlas took most of the remaining margin.
This is the demo to watch and it is the one that would break first on a 4 GB card.
Mitigations, cheapest first: drop `texTreeNormal` (26.1 MB - billboards could use a flat normal),
or halve the atlas to 512x512 (52.2 -> 13 MB, and 544 of the 625 ring chunks already ship at
256 px so the quality floor is lower than it sounds).

## Visual state
Distant forest restored and matching DX11's arrangement. The swap seam has DX11's noise-dithered
discard on the billboard side; the mesh side has no matching far-dissolve (our meshes simply stop
at the pool cap instead), so a hard edge is possible where a near tree passes the cap. Not seen
in the shots taken so far. **Wants Lee's eye while flying.**

---

# 2026-08-22 SWEEP (0822) - 2.94d/2.94f GATE - engine `68db4c1b` / game `bfaf4e13`

Purpose: regression gate for the day's two engine changes - 2.94d (VT tile-request round trip
every 4th frame) and 2.94f (terrain idle gate extended to the engine's Generation_Update).

VERDICT: **PASS 1 CLEAN, 19/19, zero failures, POLYS bit-identical 19/19 vs the 0816 baseline,
4 GB gate HOLDS (worst editor VRAM 3635 MB). PASS 2 ABORTED after 5 demos - not a code defect,
a wedged MAX process. See the incident section.**

## Pass 1 (0822a) - editor FPS vs the 2.91 baseline (0819)

| Demo | 2.91 ref | 0822a | delta | VRAM MB | POLYS |
|---|---|---|---|---|---|
| Aztec Game Kit Teaser | 63.0 | 83.1 | +32.0% | 3046 | 10,330,135 |
| Aztec Game Kit | 92.2 | 121.8 | +32.1% | 3635 | 3,438,876 |
| Bounty | 103.0 | 158.5 | +54.0% | 2639 | 469,906 |
| Horseshoe Bend | 103.5 | 124.9 | +20.6% | 2981 | 2,168,281 |
| Island Showdown | 68.8 | 90.3 | +31.1% | 2680 | 4,125,704 |
| Operation Amazon | 77.5 | 103.8 | +34.0% | 3356 | 5,504,271 |
| River Raiders | 109.7 | 170.3 | +55.2% | 3228 | 2,362,345 |
| Snowy Mountain Stroll | 134.9 | 188.9 | +40.1% | 2740 | 81,369 |
| A Grand Canyon Adventure | 96.2 | 139.4 | +44.9% | 2743 | 2,279,506 |
| Disruption | 80.3 | 108.3 | +34.9% | 2773 | 4,677,579 |
| Foggy Forest | 60.0 | 74.6 | +24.4% | 2890 | 10,220,589 |
| Indian Strike Force | 90.0 | 127.2 | +41.4% | 2990 | 3,229,699 |
| Switch Escape | 141.9 | 251.1 | +76.9% | 2277 | 109,358 |
| Canyon Offensive | 68.8 | 91.6 | +33.2% | 3233 | 8,838,008 |
| Escape from the Zombie Cellar | 138.8 | 244.9 | +76.5% | 2313 | 28,048 |
| Jungle Fever | 113.8 | 179.2 | +57.5% | 2856 | 76,157 |
| RPG Template | 95.0 | 140.6 | +48.0% | 3048 | 3,247,629 |
| The Mystery of Z Island | 99.7 | 147.4 | +48.0% | 3290 | 722,872 |
| Trapped | 152.9 | 279.5 | +82.9% | 2362 | 12,768 |

**Hub-wide +45.7%.** POLYS bit-identical 19/19 against `results_0816.txt`. Worst editor VRAM
3635 MB (gate 4096) - HOLDS. Worst IN-GAME VRAM 3947.9 MB on Aztec Game Kit, which is close
enough to the 4 GB line to be worth watching.

### Is +45.7% believable across days?
The standing rule says trust nothing under ~10% cross-day. These are 20-83%, far outside the
drift band, and the SHAPE is the confirmation: **the gains are largest on the CHEAPEST levels.**
Trapped (12,768 polys) +82.9%, Zombie Cellar (28,048) +76.5%, Switch Escape (109,358) +76.9%,
against Foggy Forest (10.2 M polys) +24.4% and Aztec Teaser (10.3 M) +32.0%. That is exactly
what removing a FIXED ~4 ms/frame cost predicts - it is a larger fraction of a cheap frame.
2.94c measured that fixed cost directly (WritebackTileRequests 3.96 ms on a 26,813-poly scene).
Mechanism and measurement agree, so this is real, not drift.

## ⚠ INCIDENT: pass 2 aborted - a wedged, unkillable MAX process

Pass 2 completed Aztec Game Kit Teaser and Aztec Game Kit, then every subsequent demo returned
FAIL_HUB. Diagnosis, in order:
- Not a crash. `Guru-Crash.log` is dated 2026-08-14 and never changed tonight.
- Not the build. The identical binary had just completed 19/19 in pass 1.
- **A MAX instance (PID 23040) from the Aztec Game Kit test-game phase wedged**, holding
  6.9 GB, still listed by `tasklist`, and would not die: `taskkill //F //PID`, `taskkill //F
  //IM`, PowerShell `Stop-Process -Force` and `Process.Kill()` all reported success or "no
  running instance" while the process remained. Almost certainly blocked in a driver call.
- Every later launch stalled at ~51 MB and never initialised, because the wedged instance held
  the GPU. That is the FAIL_HUB.

★★ **`taskkill` reporting SUCCESS does not mean the process died, and `ps -W` and `taskkill`
disagree with each other.** Both lied here in opposite directions. `tasklist //FI "IMAGENAME
eq ..."` was the only reliable reading. Any sweep that kills-and-relaunches should verify the
process is GONE with tasklist before launching, not trust the kill's exit status.

⚠ Pass 2's two completed demos DO agree with pass 1 (Aztec Teaser |A-B| 0.4%, Aztec Game Kit
2.5%, POLYS identical both), which is consistent with the previously measured ~1% same-session
noise floor - but two demos is not a noise-floor calibration.

## What is still owed
Pass 2 needs re-running once the machine is clear (the wedged PID may need a reboot). The gate
is therefore **provisionally clean on one pass**, not the usual two-pass standard.


---

# 2026-08-19 SWEEP (0819) — 2.91 REGRESSION GATE — engine `ace9088a` / game `1d766a63`

Purpose: Lee-requested confirmation that the 2.91 GPU-accounting work broke nothing.

VERDICT: **CLEAN. 19/19 in BOTH passes, zero failures, POLYS bit-identical on all 19 across
both passes AND against 2.90, 4 GB gate holds (worst editor VRAM 3588 MB). 2.91 is +2.4%
hub-wide vs yesterday — recovery, not a gain.**

## Method change: TWO passes, so the noise floor is MEASURED not assumed

Yesterday's 0818b table read as a −13.4% rout against a cross-day baseline and was pure
machine drift. Rather than repeat that, this gate runs **two identical passes back to back on
the same build**. Pass A vs Pass B is therefore a direct measurement of today's run-to-run
noise, against which the 2.91-vs-2.90 delta can be judged.

★ **The result is a genuinely useful calibration: |A−B| median 0.8%, mean 0.9%, worst 4.1%.**
That is an order of magnitude tighter than the "±8% between launches" figure this project has
been carrying. The ±8 band is the CROSS-DAY band; back-to-back same-session runs are far more
precise than that. Future A/Bs can trust a ~1% resolution if they are run in one session — and
must NOT trust anything under ~10% across days.

Also note 2.91 is **zero-cost by construction** in this gate: `BeginRangeGPU` early-returns on
`!ENABLED`, the profiler defaults off and the sweep never enables it, so the new ranges and the
Busy/Idle union are dead code here. The game-side snapshot move is gated on `IsEnabled()` too.
The +2.4% is therefore ambient — and specifically it is RECOVERY: the three biggest gainers
(Aztec Game Kit +7.0%, Zombie Cellar +6.9%, Trapped +5.9%) are the same cells 0818b flagged as
band-edge drops. They dropped most yesterday and gained most today, which is the drift
signature, not a code effect.

| Demo | 2.90 (08-18) | 2.91 A | 2.91 B | A vs B | mean vs 2.90 | VRAM MB | POLYS |
|---|---|---|---|---|---|---|---|
| Aztec Game Kit Teaser | 61.7 | 61.7 | 64.2 | +4.1% | +2.0% | 2999 | ✓ |
| Aztec Game Kit | 86.1 | 92.7 | 91.6 | -1.2% | +7.0% | 3588 | ✓ |
| Bounty | 101.9 | 102.9 | 103.0 | +0.1% | +1.0% | 2514 | ✓ |
| Horseshoe Bend | 101.8 | 104.0 | 103.1 | -0.9% | +1.7% | 2981 | ✓ |
| Island Showdown | 68.4 | 68.7 | 69.0 | +0.4% | +0.7% | 2664 | ✓ |
| Operation Amazon | 76.5 | 77.2 | 77.7 | +0.6% | +1.2% | 3341 | ✓ |
| River Raiders | 111.7 | 109.5 | 109.9 | +0.4% | -1.8% | 3230 | ✓ |
| Snowy Mountain Stroll | 131.9 | 136.1 | 133.7 | -1.8% | +2.3% | 2724 | ✓ |
| A Grand Canyon Adventure | 93.2 | 95.7 | 96.7 | +1.0% | +3.2% | 2760 | ✓ |
| Disruption | 79.1 | 79.9 | 80.7 | +1.0% | +1.5% | 2840 | ✓ |
| Foggy Forest | 59.9 | 60.0 | 59.9 | -0.2% | +0.1% | 2860 | ✓ |
| Indian Strike Force | 89.7 | 89.9 | 90.0 | +0.1% | +0.3% | 2975 | ✓ |
| Switch Escape | 139.1 | 142.4 | 141.4 | -0.7% | +2.0% | 2292 | ✓ |
| Canyon Offensive | 68.2 | 69.1 | 68.4 | -1.0% | +0.8% | 3274 | ✓ |
| Escape from the Zombie Cellar | 129.8 | 140.0 | 137.5 | -1.8% | +6.9% | 2281 | ✓ |
| Jungle Fever | 110.0 | 113.9 | 113.7 | -0.2% | +3.5% | 2857 | ✓ |
| RPG Template | 94.3 | 94.6 | 95.4 | +0.8% | +0.7% | 3049 | ✓ |
| The Mystery of Z Island | 97.9 | 99.2 | 100.1 | +0.9% | +1.8% | 3260 | ✓ |
| Trapped | 144.3 | 153.3 | 152.4 | -0.6% | +5.9% | 2346 | ✓ |

**Measured noise floor (same build, back-to-back): |A−B| median 0.8%, mean 0.9%, worst 4.1%.**
2.91 (mean of A,B) vs 2.90 hub-wide: **+2.4%**. POLYS bit-identical 19/19. Worst editor VRAM 3588 MB — 4 GB gate **HOLDS**.
Raw: `scratchpad/gate291_A`, `scratchpad/gate291_B`. Editor phase only, same shape as 0818b
(load → settle 30 s → screenshot → 3 FPS samples → VRAM), which is what makes A/B comparable.

# 2026-08-18 SWEEP (0818b) — 2.90 ACCEPTANCE — engine `bde205ad` / game `26482fe6`

Purpose: Lee-requested full-hub confirmation ahead of the first tester alpha, covering the
2.90 probe-property work (per-probe brightness restored engine-side, Probe Range slider
removed, fLightHasProbe canonicalised on load). EDITOR phase only.

VERDICT: **CLEAN. 19/19 loaded, zero failures, POLYS bit-identical on all 19, 4 GB gate
holds (worst editor VRAM 3586 MB, Aztec Game Kit).**

⚠ **Read the FPS columns with the same-day control, not the 0816 one.** Against 0816 this
table looks like a uniform −13% rout. It is not: the **2.89 gate run at 07:27 THIS MORNING
— before a line of 2.90 existed — was already −11.2% below 0816**. Isolated properly,
2.90 evening vs 2.89 morning is **−2.5% hub-wide with two demos UP** (River Raiders +2.7%,
Indian Strike Force +0.3%), comfortably inside the documented ±8% cross-launch band and
matching §2.75a's finding that this machine drifts slower as the day goes on. The three
largest single-cell drops (Aztec Teaser −9.4%, Aztec Game Kit −8.1%, Zombie Cellar −5.7%)
are the usual band-edge cells, and 2.90 cannot physically cost frame time: it adds one
float multiply inside `filterEnvMapCS`, which runs only when a probe RE-BAKES, plus one
float in a push constant that reused an existing padding slot. Allocation-neutral too,
which is why VRAM is flat.

| Demo | ed 0816 | ed 2.89 am | ed 2.90 pm | pm vs am | ed VRAM MB | POLYS |
|---|---|---|---|---|---|---|
| Aztec Game Kit Teaser | 63.8 | 68.1 | 61.7 | -9.4% | 2948 | 10330135 ✓ |
| Aztec Game Kit | 97.3 | 93.7 | 86.1 | -8.1% | 3586 | 3438876 ✓ |
| Bounty | 141.0 | 105.7 | 101.9 | -3.6% | 2447 | 469906 ✓ |
| Horseshoe Bend | 120.1 | 105.1 | 101.8 | -3.1% | 2915 | 2168281 ✓ |
| Island Showdown | 77.1 | 69.0 | 68.4 | -0.9% | 2630 | 4125704 ✓ |
| Operation Amazon | 84.8 | 76.9 | 76.5 | -0.5% | 3308 | 5504271 ✓ |
| River Raiders | 123.3 | 108.8 | 111.7 | +2.7% | 3211 | 2362345 ✓ |
| Snowy Mountain Stroll | 156.4 | 137.9 | 131.9 | -4.4% | 2690 | 81369 ✓ |
| A Grand Canyon Adventure | 104.7 | 93.3 | 93.2 | -0.1% | 2677 | 2279506 ✓ |
| Disruption | 86.9 | 79.4 | 79.1 | -0.4% | 2806 | 4677579 ✓ |
| Foggy Forest | 62.5 | 60.1 | 59.9 | -0.3% | 2841 | 10220589 ✓ |
| Indian Strike Force | 99.4 | 89.4 | 89.7 | +0.3% | 2942 | 3229699 ✓ |
| Switch Escape | 162.6 | 141.1 | 139.1 | -1.4% | 2243 | 109358 ✓ |
| Canyon Offensive | 74.1 | 68.8 | 68.2 | -0.9% | 3241 | 8838008 ✓ |
| Escape from the Zombie Cellar | 162.2 | 137.7 | 129.8 | -5.7% | 2278 | 28048 ✓ |
| Jungle Fever | 128.4 | 112.9 | 110.0 | -2.6% | 2823 | 76157 ✓ |
| RPG Template | 104.3 | 96.0 | 94.3 | -1.8% | 2967 | 3247629 ✓ |
| The Mystery of Z Island | 105.1 | 98.4 | 97.9 | -0.5% | 3225 | 722872 ✓ |
| Trapped | 176.6 | 150.5 | 144.3 | -4.1% | 2327 | 12768 ✓ |

Hub-wide editor FPS: **2.90 evening vs 2.89 THIS MORNING = -2.5%**; 2.89 morning vs 0816 = -11.2%.
Worst editor VRAM 3586 MB (4 GB gate = 4096) — **HOLDS**. POLYS bit-identical 19/19 (✓).
Method note: this is the EDITOR phase only (load → settle 30 s → screenshot → 3 FPS
samples → VRAM), the same shape as the 2.89 gate, which is what makes the same-day
comparison valid. VRAM here is the editor-phase figure from `GET_PERF_DATA` (the 0816
table's VRAM column is the IN-GAME phase, so the two are not comparable — do not diff them).

★ Lesson re-learned the hard way, again: **the first table I generated compared against
0816 and read as a −13.4% regression across all 19 demos.** The tell was uniformity — a
real regression hits the demos that exercise the changed path, not every demo equally. The
fix was the control that already existed on disk: this morning's run on the same machine.
Cross-day FPS baselines are not evidence. (MEMORY: "Editor FPS ±8 between launches —
within-session A/Bs only".)


# 2026-08-16 EVENING SWEEP (0816b) — 2.73-2.75 ACCEPTANCE — engine 2.75 `5ee09abc` / game `bb4c4712`

Purpose: Lee-requested full-hub confirmation that the day's changes (2.73 probe pool
re-bake, 2.74 terrain roughness dry-look floor content lift, 2.75 matte probe ball +
preview sphere) broke nothing. Compared against the same-day 0816 morning table.

VERDICT: **CLEAN. 19/19 loaded, POLYS bit-identical on all 19, 4 GB gate holds with the
worst case DOWN again (Aztec Game Kit 3852 -> 3838 MB), VRAM within +-17 MB everywhere.**
FPS hub-wide +4.6% editor / +4.1% in-game vs the morning — net positive, inside the drift
band. Single flag: Aztec Game Kit editor -14% (97.3 -> 83.9) with its in-game phase -5%
and VRAM/POLYS identical — the same band-edge single-cell class the morning table itself
carried (Aztec Teaser -12%), not a regression signature. Several demos UP double digits
(River Raiders +22%/+16%, Island Showdown +16%/+10%, Operation Amazon +12%/+14%) —
consistent with §2.75a's finding that today's machine drifted low mid-evening and
recovered. Raw: tools/sweep_0816b_2.75.txt.

# Demo FPS sweep — every hub demo, editor + in-game

## 2026-08-16 SWEEP (0816) — THE MILESTONE DDS CONVERSION SWEEP — engine 2.71 `c2c9ef44` / game 2.72 `bfad4185` — 19/19 CLEAN, all gates hold

Run immediately after the full stock DDS conversion (§2.72 in NIGHT_INVESTIGATIONS: 1641
files brought to full mip chains, +1431 MB disk, originals mirrored at D:\max\mipbackup).
This is the acceptance sweep the MILESTONE doc requires, AND the fresh per-demo FPS + VRAM
readings Lee asked for ahead of the internal tester build. Comparison base: 0814c on 2.51.

**Verdict: 19/19 loaded, zero crashes, POLYS BIT-IDENTICAL on all 19 (texture-only change
proven), 4 GB gate HOLDS with MORE headroom — and in-game VRAM fell on EVERY demo** (−22 to
−135 MB; worst case Aztec Game Kit 3987 → 3852, headroom 109 → 244 MB). Hub-wide FPS drift:
editor −4.6%, game −3.9% — inside the documented ±8-15% ambient cross-run band. Texture
streaming remains DEFAULT OFF; the conversion is what makes revisiting it possible (task #37). **[CORRECTED 2026-08-18: this is WRONG — streaming is and was ON by default; it defaulted 0 for one day (08-01) and was restored the same day by delta 1.73. See STREAMING_STATUS_2026-08-18.md.]**

| Demo | editor FPS | in-game FPS | in-game VRAM MB (Δ vs 0814c) | POLYS |
|---|---|---|---|---|
| Aztec Game Kit Teaser | 63.8 | 61.3 | 3239 (−41) | 10330135 |
| Aztec Game Kit | 97.3 | 84.4 | 3852 (−135) | 3438876 |
| Bounty | 141.0 | 157.5 | 2841 (−54) | 469906 |
| Horseshoe Bend | 120.1 | 90.2 | 3190 (−87) | 2168281 |
| Island Showdown | 77.1 | 94.5 | 3088 (−72) | 4125704 |
| Operation Amazon | 84.8 | 84.1 | 3583 (−71) | 5504271 |
| River Raiders | 123.3 | 97.3 | 3498 (−70) | 2362345 |
| Snowy Mountain Stroll | 156.4 | 102.6 | 2987 (−55) | 81369 |
| A Grand Canyon Adventure | 104.7 | 60.0 | 3073 (−70) | 2279506 |
| Disruption | 86.9 | 83.3 | 2975 (−112) | 4677579 |
| Foggy Forest | 62.5 | 74.8 | 3129 (−117) | 10220589 |
| Indian Strike Force | 99.4 | 99.3 | 3102 (−86) | 3229699 |
| Switch Escape | 162.6 | 136.6 | 2455 (−39) | 109358 |
| Canyon Offensive | 74.1 | 72.9 | 3416 (−86) | 8838008 |
| Escape from the Zombie Cellar | 162.2 | 59.9 | 2509 (−39) | 28048 |
| Jungle Fever | 128.4 | 131.8 | 3143 (−55) | 76157 |
| RPG Template | 104.3 | 86.7 | 3435 (−22) | 3247629 |
| The Mystery of Z Island | 105.1 | 94.9 | 3499 (−100) | 722872 |
| Trapped | 176.6 | 179.8 | 2577 (−39) | 12768 |

Notables: Bounty +16-18% and Grand Canyon in-game +28% (46.8 → 60.0) are genuine
improvements; Aztec Teaser (−12%/−10%) and ISF in-game (−11%) sit at the edge of the drift
band — worth one eyeball on Lee's rig but consistent with the 08-03 lesson that 30 s soaks
sample lazy-PSO warm-up. Nothing crosses into regression territory, and the memory story
(VRAM down everywhere, gate headroom up) is exactly what full mip chains were meant to buy.

## 2026-08-14 SWEEP (0814c) — engine `07a192f2` (2.47) / game 2.51 `56377809` — 19/19 CLEAN, and the story of the day

THREE sweeps ran today on what was nominally the same content. The first two were POISONED by a
2.50 leak and are recorded here only as a warning; 0814c below is the only valid record.

**The leak (fixed as game 2.51):** 2.50 made hub/storyboard tutorial videos load on DX12. The
widget pauses its video only from its own per-frame draw, so leaving the section within seconds
orphaned a live MF session — and `iVideoChanged` was never consumed, so UpdateAllAnimation
re-converted the SAME stale frame every render frame (`Logic - ConstantNonDisplay` 11.70 ms).
17/19 launches of the first sweep paid ~+6 ms editor / +10 ms game. A same-binary probe pair
read clean and produced a WRONG "machine load" exoneration — the re-run's per-demo REPRODUCTION
(Kit 47.9 vs 47.8) was what broke the false story. Crack chain: probe_one.sh -> ENABLE_PROFILER
(named the range) -> videotrace.txt (37/37 load-lines matched fast/slow). Verify: pre-fix 6/6
slow launches, post-fix 4/4 fast.

**0814c verdict vs the 08-13 baseline:** 19/19 loaded, zero crashes, POLYS bit-identical on all
19, 4 GB gate HOLDS (worst in-game Aztec Game Kit 3987.3 — ~109 MB headroom, in line with its
08-07/08-08 readings). Editor mean +9.3% (every demo -1.3%..+17.8% = the usual ambient cross-run
term, read as "no regressions", not a win). Game mean -0.1%.

Game-phase rows explained, marked (c) below:
- **A Grand Canyon Adventure -65%: its intro CUTSCENE (videobank introtolevel1.mp4) now actually
  plays** — 2.50 working as intended; all three samples landed inside the movie. Presentation,
  not gameplay. (Post-cutscene rate unmeasured this run.)
- **Bounty -8.8%: same class** — bountyintrocs.mp4 plays at level start; samples land near it.
- Operation Amazon game 89.5 vs 106.0: NO video involved (trace-verified). Its game phase has
  ranged 97-116 across the last week's sweeps; single-run mild outlier, watch item only.

| Demo | ed 08-13 | ed 08-14c | d% | game 08-13 | game 08-14c | d% | gVRAM 08-14c | POLYS |
|---|---|---|---|---|---|---|---|---|
| Aztec Game Kit Teaser | 64.5 | 72.6 | +12.5 | 61.7 | 68.5 | +10.9 | 3279.7 | SAME |
| Aztec Game Kit | 90.9 | 100.4 | +10.5 | 89.2 | 91.0 | +2.0 | 3987.3 | SAME |
| Bounty (c) | 118.9 | 119.9 | +0.9 | 148.9 | 135.7 | -8.8 | 2895.3 | SAME |
| Horseshoe Bend | 123.5 | 122.0 | -1.3 | 89.6 | 89.1 | -0.6 | 3277.0 | SAME |
| Island Showdown | 76.0 | 78.1 | +2.7 | 92.1 | 93.8 | +1.8 | 3159.9 | SAME |
| Operation Amazon | 86.4 | 89.9 | +4.1 | 106.0 | 89.5 | -15.6 | 3653.7 | SAME |
| River Raiders | 127.5 | 130.4 | +2.3 | 99.5 | 102.1 | +2.6 | 3568.5 | SAME |
| Snowy Mountain Stroll | 158.9 | 166.6 | +4.8 | 113.9 | 107.6 | -5.5 | 3042.2 | SAME |
| A Grand Canyon Adventure (c) | 109.5 | 112.1 | +2.4 | 134.5 | 46.8 | -65.2 | 3142.8 | SAME |
| Disruption | 82.3 | 92.0 | +11.8 | 98.2 | 91.0 | -7.3 | 3087.2 | SAME |
| Foggy Forest | 61.5 | 67.2 | +9.3 | 72.7 | 79.8 | +9.8 | 3246.5 | SAME |
| Indian Strike Force | 92.8 | 109.0 | +17.5 | 96.8 | 111.0 | +14.6 | 3188.1 | SAME |
| Switch Escape | 153.7 | 173.0 | +12.6 | 141.7 | 151.2 | +6.7 | 2493.6 | SAME |
| Canyon Offensive | 71.4 | 80.1 | +12.2 | 71.2 | 79.4 | +11.6 | 3502.1 | SAME |
| Escape from the Zombie Cellar | 145.4 | 168.4 | +15.9 | 59.9 | 59.9 | +0.1 | 2548.0 | SAME |
| Jungle Fever | 120.2 | 140.4 | +16.7 | 137.5 | 146.4 | +6.5 | 3197.5 | SAME |
| RPG Template | 98.8 | 108.9 | +10.2 | 87.3 | 93.8 | +7.4 | 3457.3 | SAME |
| The Mystery of Z Island | 102.0 | 115.4 | +13.1 | 90.6 | 99.5 | +9.8 | 3598.7 | SAME |
| Trapped | 158.6 | 186.9 | +17.8 | 162.5 | 190.3 | +17.1 | 2616.2 | SAME |

## 2026-08-08 SWEEP — engine `118e19d8` (2.13) / game `82959a2b` — REGRESSION CHECK, 19/19 CLEAN

Run to answer one question: did the gpup particle campaign and the 2.13 hook-boundary change
break anything across the hub? Baseline is the 08-07 run (`results_0807`), the last sweep before
these landed:

- **engine 2.13** state-safe `customDraw_*` hook boundaries — the hub-wide risk, since it adds a
  camera-CB + common-resource rebind + command-list invalidate after EVERY custom draw, terrain included
- gpup: seed fix (`8d0c75a7`), split-arm retirement (`039f244d`), shared-CB copy race (`82959a2b`)
- TracerManager shared-CB fix (same race class)

| Demo | ed 08-07 | ed 08-08 | Δ% | game 08-07 | game 08-08 | Δ% | gVRAM 08-07 | gVRAM 08-08 | POLYS |
|---|---|---|---|---|---|---|---|---|---|
| Aztec Game Kit Teaser | 69.7 | 78.8 | +13 | 65.6 | 75.4 | +15 | 3498 | 3466 | identical |
| Aztec Game Kit | 100.2 | 104.8 | +5 | 78.8 | 79.4 | +1 | 3966 | **3982** | identical |
| Bounty | 124.3 | 151.2 | +22 | 129.1 | 155.5 | +20 | 2973 | 2973 | identical |
| Horseshoe Bend | 91.8 | 92.3 | +0 | 70.1 | 68.1 | −3 | 3369 | 3385 | identical |
| Island Showdown | 78.7 | 88.3 | +12 | 86.5 | 85.0 | −2 | 3362 | 3378 | identical |
| Operation Amazon | 89.8 | 101.9 | +13 | 104.3 | 108.0 | +4 | 3873 | 3857 | identical |
| River Raiders | 119.7 | 139.7 | +17 | 95.9 | 96.2 | +0 | 3772 | 3771 | identical |
| Snowy Mountain Stroll | 132.5 | 140.6 | +6 | 89.0 | 92.2 | +4 | 3164 | 3164 | identical |
| A Grand Canyon Adventure | 109.8 | 130.7 | +19 | 116.3 | 119.2 | +2 | 3185 | 3187 | identical |
| Disruption | 89.6 | 102.3 | +14 | 99.7 | 111.6 | +12 | 3315 | 3316 | identical |
| Foggy Forest | 66.6 | 73.6 | +11 | 75.2 | 85.2 | +13 | 3450 | 3450 | identical |
| Indian Strike Force | 107.6 | 116.9 | +9 | 95.8 | 95.0 | −1 | 3392 | 3391 | identical |
| Switch Escape | 143.3 | 176.1 | +23 | 132.5 | 172.2 | +30 | 2714 | 2714 | identical |
| Canyon Offensive | 77.3 | 89.6 | +16 | 79.7 | 81.4 | +2 | 3712 | 3712 | identical |
| Escape from the Zombie Cellar | 133.1 | 168.9 | +27 | 60.1 † | 60.0 † | −0 | 2770 | 2769 | identical |
| Jungle Fever | 134.7 | 163.6 | +22 | 120.0 | 124.1 | +3 | 3233 | 3233 | identical |
| RPG Template | 103.1 | 119.6 | +16 | 86.2 | 100.6 | +17 | 3660 | 3660 | identical |
| The Mystery of Z Island | 119.6 | 140.6 | +18 | 110.9 | 107.8 | −3 | 3794 | 3794 | identical |
| Trapped | 140.9 | 179.9 | +28 | 142.6 | 180.2 | +26 | 2836 | 2820 | identical |

† the in-game column caps at 60 on this demo — a frame cap, not a ceiling.

**19/19 loaded, zero failures, zero crash logs, prep gate 0 s on every demo.** No demo regressed
outside the noise band; the three small minuses are inside this rig's run-to-run spread.

**POLYS bit-identical on all 19** — the standing acceptance gate for a pipeline change, and the one
that matters most here, because 2.13 touches every custom draw. Nothing moved.

**4 GB gate holds.** Worst in-game Aztec Game Kit 3982 MB (~114 MB of headroom, still the demo to
watch), then Operation Amazon 3857. Every delta within ±16 MB = noise, not a footprint change.

### ★ DO NOT BANK THE +16% — it is the ambient-drift signature, not a win we earned

Editor sum +16.1%, game +8.6%, and it is tempting to credit the particle work. Don't. Converted to
per-frame time the change is a near-**uniform −1.21 ms/frame, sd 0.45**, across all 19 demos —
independent of level speed, poly count, and *whether the level contains particles at all*. Bounty,
Trapped and Zombie Cellar carry no gpup content and gained 1.4–1.6 ms exactly like the rest, and
nothing in these commits removes a fixed cost from a particle-free level. This is the same uniform
~1.5 ms/frame ambient swing this rig produced on 07-31 in the *negative* direction (and which was
exonerated as day-drift then). **Read this run as "no regressions", not as a speedup.** Only Switch
Escape's in-game +30% has a candidate mechanism (split-arm retirement, which halves gpup sim time
above 140 FPS, and Switch Escape both carries emitters and sits in that band) — and even that is
confounded by the drift. ★ RULE, restated: editor and in-game absolutes on this rig are only
comparable within a same-day A/B; cross-run sums carry a ±1.5 ms/frame uniform term.

### Visual pass — 38 screenshots, 19 pairs, zero regressions

Mean-luma + blown-pixel probe (`scratchpad/shotstats.ps1`) over every editor and in-game shot vs the
08-07 pair: every demo flat, largest move Snowy editor −2.6 luma. That rules out the white-out class
(#120) and any lighting-character change from the 2.13 rebinds. Ten agents then compared each pair
directly for missing geometry, corrupt textures and absent emitters: **19 MATCH, 0 REGRESSION, 1 MINOR.**

The MINOR is **Snowy Mountain Stroll**: its editor steam plume is visibly narrower than 08-07's
(~180 px vs ~350 px envelope), which is also the whole of that −2.6 luma (less bright smoke over a
dark interior — *darker*, the opposite of the white-out signature). Chased with `GPUP_AGES` rather
than accepted from stills, across all 10 emitters on the live level:

| emitter | alive | top age bin | foreign births |
|---|---|---|---|
| e0 | 3912/4096 (96%) | 25% | 0 |
| e1–e9 | 471–592 (11–14%) | 7–8% | 0 |

Pool full, age band cleanly staggered (no cohort), **extras = 0 on every emitter**, sim cadence
0.99× real time, `arm_split=0`, `max_time` 3.199 inside its 3.2 bound. So the thinner plume is the
corrected spawn cadence, not an under-populated pool — the over-churn leaving. Whether the corrected
density matches DX11 is still a question only the user's eye settles (task #118).

---

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

**Judge the gate on the IN-GAME figure, not the editor one.** v4 records both for the first time,
and test-game sits 250–590 MB *above* the editor reading on every demo. Worst cases: Operation
Amazon **3840 MB**, Aztec Game Kit 3805, River Raiders 3756, Z Island 3730 — all still inside
4096, but Amazon has only ~256 MB of headroom, so it is the demo to watch when anything adds
per-level memory. Lightest in game: Switch Escape 2651, Zombie Cellar 2753, Trapped 2806.

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

**Follow-up, ANSWERED: the navmesh cache was thrashing its own prune cap.** Horseshoe rasterized
its navmesh on BOTH runs an hour apart, where 08-01 recorded an instant cached start. Two visits
back to back settled it: with its entry still resident, Horseshoe hit the cache **both times**
(10 s below 20 FPS = ordinary game-entry cost, no `.gnav` written, nothing evicted). The
mechanism was never broken — **the cap was 20 files and the hub has 19 demos**, so playing
through every level evicted each entry with the ~18 writes that followed it, and the next lap
paid the full ~35 s recast rebuild again. Fixed in `GGRecastDetour.cpp`: prune now bounded by
**48 files AND 512 MB** (entries measured 0.07–27 MB; 20 of them came to 109 MB), with a guard
so a level bigger than the byte budget can never prune away its own entry.

Verified live: stash every real entry aside (so the level under test is a guaranteed miss), prime
the directory with exactly 20 dummies on staggered timestamps, then enter test-game on Switch
Escape. Result **21 files, 1 newly written, the oldest dummy untouched** — the old cap would have
left 20 with that dummy deleted. (First attempt at this test was junk: it guessed the level's
entry by picking the largest `.gnav`, which belonged to a different demo, so the level hit its
cache and wrote nothing. Don't guess which hash belongs to which level — stash them all.)

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
