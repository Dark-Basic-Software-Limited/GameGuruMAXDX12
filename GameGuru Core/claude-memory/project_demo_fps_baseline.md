---
name: project-demo-fps-baseline
description: "FPS baseline for all 19 hub demos. ★★ 08-14: an ENTIRE daytime sweep read −30-50% and was refuted in 6 min by same-binary probes (probe_one.sh) — validate any sweep anomaly with an immediate same-binary probe BEFORE bisecting; contamination tells inside. ★ 2026-08-08 REGRESSION SWEEP (engine 2.13 118e19d8 / game 82959a2b, post gpup-campaign + state-safe customDraw hooks): 19/19 CLEAN — zero load failures, zero crash logs, POLYS BIT-IDENTICAL on all 19, 4 GB gate holds (worst in-game Aztec GK 3982, Amazon 3857), visual pass 38 shots = 19 MATCH / 0 REGRESSION / 1 MINOR (Snowy's steam plume narrower = the corrected spawn cadence; GPUP_AGES showed pool 96% full, staggered, extras=0 on ALL 10 emitters, cadence 0.99x). ★★ DO NOT BANK the +16.1% editor sum: converted to time it is a UNIFORM −1.21 ms/frame sd 0.45 across all 19 INCLUDING particle-free levels (Bounty/Trapped/Zombie gained 1.4-1.6 ms and carry no gpup content) = the same ambient-drift term that hit −1.5 ms/frame on 07-31; read it as 'no regressions', never as a win. Prior: 2026-08-06 SWEEP v4 (engine a229fffe / game 50ca7c28, the 2.07g build), RUN TWICE (+0.5% between runs, no demo moved 10% = tight instrument). Editor sum +1.3% vs 08-01. ★ EVERY DEMO NOW UNDER 4 GB AT DEFAULTS: max 3607 MB, median 3002 MB, zero over 4096, every level -2.0 to -6.6 GB (Z Island 10038->3390). Limiter is geometry HARDER than before: Spearman polys -0.95 vs VRAM -0.53 (was -0.85/-0.66) - VRAM halved and FPS did not follow. Five lowest: Foggy Forest 63.4, Aztec Teaser 68.2, Island Showdown 72.9, Canyon Offensive 72.9, Disruption 85.4. ★ The four 'regressions' (Trapped -17%, Switch -14%, Zombie -12%, Snowy -12%) are the 30 s soak sampling a WARM-UP, not a loss: lazy object PSOs went default-ON after 08-01, so levels gain 10-12% between 30 s and 180 s and are flat by 300 s (Trapped 132->148, Switch 122->135->134, Zombie 120->133->133). Use a 180 s soak for numbers comparable with pre-08-03 baselines. Horseshoe 'in-game 3 FPS' was a fake collapse AGAIN - 'RASTERIZING NAVIGATION MESH' overlay, which GET_SCREEN_TEXT CANNOT SEE; real figure 71.8. v4a gates on FPS RECOVERY not text. Older: 2026-08-01 SWEEP v3 (engine 53481336 / game 0627f986, streaming ON): 19/19 clean, NO REGRESSIONS vs 07-31 (sum +5.1%). Five lowest by EDITOR FPS: Canyon Offensive 59.9, Foggy Forest 60.4, Aztec Teaser 63.3, Island Showdown 67.6, Aztec Game Kit 74.7 (field 59.9-154.3, median 95.6). KEY FINDING: the limiter is POLYGON COUNT, not VRAM - Spearman vs editor FPS is polys -0.85 vs VRAM -0.66; the 3 slowest are the 3 highest-poly levels, while Z Island runs 74.9 on the hub's LARGEST VRAM footprint (9.8 GB). So VRAM work buys headroom, not framerate. RANK ON EDITOR FPS - the in-game column caps at 60 (Zombie Cellar: 144.9 editor vs 59.9 game proves it). Sweep script tools/demo_fps_sweep.sh gates on the PREPARING overlay clearing; without that gate the 07-31 run recorded a fake 3.7 FPS for Horseshoe Bend. Repo GameGuru Core/DEMO_FPS_SWEEP.md is the authority; run cost ~2h50m."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-14T16:14:36.753Z
---

# ★★★ 2026-08-14: THE HALVED SWEEP — a per-LAUNCH leak, not day-drift (and my first verdict was WRONG)

The 0814_2.50 sweep read editor FPS down 30–50% on 17/19 demos (Switch 154→80, Aztec GK 91→48;
~constant +6 ms editor / +10 ms game). Two same-binary probes read AT/ABOVE baseline, and I
published "transient machine load — no binary regression". **That verdict was FALSE.** The re-run
sweep reproduced the slow numbers per-demo (Kit 47.9 vs 47.8) while probes stayed fast — the truth
was **launch-BIMODAL, not time-varying**: game 2.50 let hub/storyboard tutorial videos actually
LOAD on DX12; the widget pauses its video only from its own per-frame draw, so leaving the section
within seconds orphans a decoding MF session; and `iVideoChanged` was never consumed, so
UpdateAllAnimation re-converted the SAME stale frame at render rate forever — profiler:
`Logic - ConstantNonDisplay 11.70 ms` of a 20.5 ms frame. Whether a launch loaded a video decided
fast-vs-slow; videotrace.txt lines matched the fast/slow ledger 37/37. **FIXED game 2.51
(`56377809`): consume iVideoChanged before convert + tutorial-widget watchdog frees stale slots.**
Pre-fix 6/6 slow → post-fix 4/4 fast.

**Lessons, in order of weight:**
1. **"Same binary, fast elsewhere" does NOT exonerate the binary** — bimodal launch state fooled
   two clean probes. EXONERATION NEEDS REPRODUCTION UNDER THE FAILING CONTEXT, not a passing probe
   in a different context. The re-run's per-demo REPRODUCTION (47.9 vs 47.8) was the tell.
2. The instrument chain that cracked it in ~40 min: probe_one.sh (same-binary refute) →
   `ENABLE_PROFILER` + GET_PERF_DATA `PROFILER_DATA` (named the 11.7 ms range) → videotrace.txt
   (correlated every slow launch with a video load). Repo tools: `GameGuru Core/tools/probe_one.sh`.
3. Slow-launch fingerprint for THIS class: CPU_FRAME >> GPU_FRAME, `SUBMIT_STALL` ~0 with all
   scene counters identical, driver_usage +30-50 MB with census identical (the MF session).
4. POLYS/VRAM/no-crash gates of a poisoned sweep remain valid (load-state, not timing).
5. A "restored feature" fix (2.50 videos) must be swept for LIFECYCLE leaks: who STOPS the thing
   when its UI goes away? Same family as the modal-error and paired-accessor rules.

# 2026-08-08 REGRESSION SWEEP (engine 2.13 `118e19d8` / game `82959a2b`) — repo `DEMO_FPS_SWEEP.md`

Run to prove the gpup campaign + the 2.13 state-safe `customDraw_*` hook boundaries (a rebind after
EVERY custom draw, terrain included) broke nothing. Baseline = the 08-07 run. **19/19 loaded, zero
failures, zero crash logs, prep=0s everywhere, POLYS bit-identical on all 19, 4 GB gate holds**
(worst in-game Aztec GK 3982 = ~114 MB headroom, Amazon 3857; all deltas within ±16 MB).

**★★ THE +16.1% EDITOR SUM IS NOT OURS.** Per-frame it is a uniform **−1.21 ms/frame, sd 0.45**
across all 19, independent of level speed, poly count and *whether the level has particles*:
Bounty/Trapped/Zombie Cellar carry no gpup content and gained 1.4–1.6 ms like everything else, and
nothing in these commits removes a fixed cost from a particle-free level. Same uniform ~1.5 ms/frame
term this rig produced on 07-31 in the negative direction (exonerated as day-drift then). Only Switch
Escape in-game +30% has a candidate mechanism (split-arm retirement halves gpup sim time >140 FPS)
and even that is confounded. ★ RULE restated: cross-run absolutes carry a ±1.5 ms/frame uniform term
— same-day A/Bs only.

**Visual pass, 38 shots:** luma/blown-pixel probe (`scratchpad/shotstats.ps1`) flat on every demo
(largest move Snowy editor −2.6) = white-out class and lighting-character change both ruled out;
then 10 agents compared each pair → **19 MATCH, 0 REGRESSION, 1 MINOR**. MINOR = Snowy's editor steam
plume narrower (~180 px vs ~350 px envelope, and the whole of that −2.6 luma — *darker*, opposite of
white-out). Settled with `GPUP_AGES` on all 10 emitters rather than from stills: e0 alive 3912/4096
(96%), e1–e9 471–592, top age bin 7–25% (staggered, no cohort), **extras = 0 on every emitter**,
cadence 0.99×, `arm_split=0`, `max_time` 3.199 inside its 3.2 bound → the thinner plume is the
over-churn leaving, not an under-populated pool. DX11 density parity still needs the user's eye (#118).
Tools: `scratchpad/compare_sweep.sh <oldtag> <newtag>` joins two result files and flags FPS/VRAM/POLYS;
`scratchpad/shotstats.ps1` does the luma pass. ⚠ awk `split(s,a,"|")` needs `/\|/` — `"|"` is a regex
alternation and silently returns garbage; and `getline < f` on the SAME filename twice reads EOF, so a
self-compare A-vs-A reports zero rows and is not a valid test of the script.

> **08-07 run (results_0807, engine 2.10/2.11 build `98a1df21`/`5829d665`): all 19 OK, 4 GB
> gate HOLDS (max Aztec Game Kit 3965.8).** gpup re-enable adds +754 MB total across the hub
> (deltas track emitter counts: Aztec +161, Teaser +128, Z Island/Snowy/Switch ~+63); game
> FPS −4-8% only on the three emitter-heavy demos (restored DX11 content, not a regression);
> editor FPS up broadly (cross-run variance + warm caches). Light-curve change showed NO FPS
> cost. Horseshoe game 70.1 (the 08-06 "3.0" row was the prep-loop artifact). Per-demo
> editor+game screenshots of the new light look: scratchpad demo_fps/shots0807/.


# 2026-08-06 SWEEP v4 (engine `a229fffe` / game `50ca7c28`) — repo `DEMO_FPS_SWEEP.md` is THE authority
**Run TWICE** (22:13 + 23:03): sum +0.5% between runs, **no demo moved 10%** — so 08-01 deltas are real.
Run cost is now ~50 min, not 2h50m (levels load far faster). Editor sum 1886.9 → **1912.0 (+1.3%)**.

**★ ALL 19 DEMOS FIT 4 GB AT DEFAULTS — max 3607 MB, median 3002 MB, zero over 4096, no preset,
no per-level checkbox.** ⚠ **Judge the gate IN-GAME, not in the editor**: test-game runs 250-590 MB
HIGHER on every demo (v4 records both now). Worst in game: **Operation Amazon 3840** (only ~256 MB
of headroom — the demo to watch), Aztec GK 3805, River Raiders 3756, Z Island 3730. Still all <4096. Every level −2.0 to −6.6 GB vs 08-01 (Z Island 10038→3390; it was 13.0 GB
in July). The 08-02→08-04 campaign landed across the whole hub. **Spearman: polys −0.95, VRAM −0.53**
(was −0.85 / −0.66) — VRAM halved, FPS didn't follow, so the geometry verdict is now near-monotonic.

Big movers vs 08-01: Z Island +50%, RPG Template +31%, Aztec GK +27%, Jungle Fever +22%,
Canyon Offensive +22% (its old 59.9-in-both-columns was a cap, now 72.9/74.6).

**★ THE FOUR "REGRESSIONS" ARE A MEASUREMENT ARTIFACT, NOT A LOSS.** Trapped −17%, Switch −14%,
Zombie −12%, Snowy −12% are the four FASTEST levels — signature of a fixed per-frame cost, and both
runs reproduced it. Cause: **lazy object PSOs went default-ON (engine 1.82) AFTER the 08-01 baseline**,
so PSOs compile on first use and a level keeps speeding up for 2-3 min. Measured 30s→180s→300s:
Trapped 132.0→148.0, Switch Escape 122.4→135.5→134.4, Zombie 119.9→132.7→132.7 (+10-12%, flat by 300s).
Steady state lands within ~10% of 08-01 = inside this rig's ±15% drift. **Raise the soak to 180 s for
numbers comparable with pre-08-03 baselines** (+~48 min/run); 30 s is still valid as "what a user feels
just after a level opens", but it is NOT steady state any more.

**★ HORSESHOE "3 FPS IN GAME" = FAKE COLLAPSE AGAIN, NEW MASK.** Not the 07-31 case: the screenshot
says **"RASTERIZING NAVIGATION MESH — 73\100 Complete"**, and **`GET_SCREEN_TEXT` CANNOT SEE that
banner** (it is `printscreenprompt`, not ImGui text) so any text gate returns 0 s and samples a loading
screen. Real figure **71.8** (08-01: 70.4). v4a now gates on **FPS RECOVERY** (sub-20 → poll until it
climbs out) — text gating is not trustworthy for this. ⚠ Open, load-time only: Horseshoe rebuilt its
navmesh on BOTH runs an hour apart; cache is alive at
`Documents\GameGuruApps\GameGuruMAX\Files\navcache` but sits **exactly at its 20-file prune cap** with
17 of 20 written by tonight's sweeps — 19 demos may be thrashing it, or the vertex-soup hash is not
stable run to run. One two-visit test settles it.

Side finding: every demo gained exactly +1 command list vs 08-01 (14→15/16→17/18→19) = the 08-05
outline restore running its mask + composite every editor frame. Gating it on "anything highlighted"
is worth only **0.055 ms/frame** (Trapped 148.0 vs 146.8) — kept, but it does NOT explain the light-level
drop (the warm-up above does). ⚠ I attributed it to the outline first and was wrong; the three-arm A/B
(ON → OFF → ON) is what caught it, because the first arm was cold.

# 2026-08-01 21:48 SWEEP v3 (engine `53481336` / game `0627f986`, streaming ON) — repo `DEMO_FPS_SWEEP.md`
19/19 clean. **NO REGRESSIONS: every demo flat or faster vs 07-31, sum 1795->1887 (+5.1%)**
(RPG +13.0, Trapped +9.1, Jungle +8.6, Canyon +7.9; only Aztec Teaser -1.1 = noise).
**FIVE LOWEST (editor FPS): Canyon Offensive 59.9, Foggy Forest 60.4, Aztec Teaser 63.3,
Island Showdown 67.6, Aztec Game Kit 74.7.** Field 59.9-154.3, median 95.6.
**THE LIMITER IS POLYGONS, NOT VRAM — Spearman vs editor FPS: polys -0.85, VRAM -0.66.**
The 3 slowest = the 3 highest-poly levels (8.8M/10.2M/10.3M tris). Counter-examples settle it:
Z Island has the MOST VRAM in the hub (9.8 GB) yet runs 74.9 on 0.70M polys; Jungle Fever holds
7.4 GB at 101 FPS on 0.07M polys. **So the streaming/census VRAM work buys headroom + loading
smoothness, NOT framerate** — to move these five, attack draw/geometry load (LOD, occlusion,
instancing on dense architectural sets), not bytes.
**RANK ON EDITOR FPS — the in-game column caps at 60.** Canyon Offensive, Aztec Teaser and
Zombie Cellar all read exactly 59.9 in game; Zombie Cellar proves it's a CAP not a ceiling
(144.9 editor vs 59.9 game). Editor is uncapped (Trapped 154), so it's the only comparable metric.
**v3 = the PREPARING gate** (now `GameGuru Core/tools/demo_fps_sweep.sh`): polls GET_SCREEN_TEXT
until the "PREPARING TEST LEVEL - N/100" overlay clears before sampling. Horseshoe Bend now reads
a healthy **70.4** (it was the fake 3.7 below) and prep = 0s on every demo post-navmesh-cache.
Run cost ~2h50m (~9 min/demo) — budget for it.

# 2026-07-31 18:26 SWEEP v2 (build 1.68 `388628e5`/`26f4268f`) — first sweep WITH a test-game phase
19/19 clean, editor + in-game FPS per demo (results_0731.txt; script demo_fps_sweep2.sh adds
CLICK test_level + 3 game samples + RAYS counters). **Editor sum 2105→1795 (-14.7%) vs 07-30 —
EXONERATED as ambient day-drift, NOT a regression:** the dip is a uniform ~1.5ms/frame (fast demos
lose most %), and a same-day rebuild at PRE-sky commits (`92b7654d`/`71ba0a69`) read the SAME as the
new build on Switch Escape (135-150 vs sweep's 139; 07-30 recorded 179.8). Editor absolutes swing
±15-20% day-to-day — only same-day A/Bs are valid (the ±8 note below is optimistic). IN-GAME: 17/19
healthy (game ≥ editor on 10 demos; Zombie Cellar + Aztec Teaser pin at exactly 59.9 = frame cap);
**Horseshoe Bend "3.7 FPS in game" = SOLVED, NOT A PERF BUG (night session, wall-gap tracer,
screenshot-proven): the sweep sampled during the "PREPARING TEST LEVEL - N\\100" loading loop,
which takes ~60-90s on this level (~0.5-1s per prep step, one forced progress-render between
steps via StartForceRender — M-GridEditB_part6.cpp:1624). After 100/100: 62-67 FPS healthy
gameplay. The wall-gap tracer (engine `af201789` + game `b2876674`, PERMANENT tooling: >100ms
frame gaps dump a per-segment ledger to Files/gap_trace.txt + GAPS: line in GET_PERF_DATA)
pinned the void to RunCustom-tail→forced-render, killing en route: rays, VT FreeSort (~0.3ms
real; the '42.5ms' profiler reading was job-thread wall-time — TRAP: CPU ranges on job threads
read scheduling delay as work), submit tail, WndProc, message pump, PSO compiles (~0), video
theory. LESSONS: (1) sweep in-game sampling must WAIT for the PREPARING text to clear
(GET_SCREEN_TEXT gate) or long-prep levels read as fake collapses; (2) profiler averages
cannot see one-frame hitches — use the tracer. FOLLOW-UP worth having: WHY 100 prep steps ×
0.5-1s on Horseshoe (physics/AI/entity init per step?) — a LOAD-TIME improvement, not FPS.**
1.68 raycast fix validated in-game across all character demos (Island Showdown 73.6, Aztec
GK 72.2, RPG 57.5 — no collapses).

# 2026-07-30 07:40 RE-SWEEP (build through delta 1.61) vs the 07-29 baseline below
19/19 clean again. **Sum FPS 2197→2105 (-4.2%), median -3.5.** WINNERS (post-load dip fix paying off
on big terrains at measure time): Snowy +28.8, Horseshoe +13.2, River Raiders +12.0, Indian +8.5.
LOSERS = shadow-fidelity price on caster-dense scenes: Teaser -62.7(!), Disruption -19.6, Bounty -18.7,
Amazon -18.2; mid-pack -1..-6. VRAM +100-885MB/level (atlases doubled+true-2048). TEASER ATTRIBUTION
(live A/B 07:50): dedicated slots OFF = +0.4, throttle-all anims = +0.6 — character systems EXONERATED;
profiler: GPU 12.5ms, Opaque 4.6 (feathered PCF sampling on full-screen character pixels), shadow
RENDER only 0.56ms. KEY FRAMING: pre-1.58b the '2048' sun setting silently packed at 1024 — the
regression is largely the price of DELIVERING the resolution the setting always claimed. USER LEVER:
Sun res panel setting 1024 ≈ old perf while keeping D32+feather+dedicated slots. Full re-sweep rows in
scratchpad demo_fps/results.txt (baseline preserved as results_baseline_0729.txt).

# Demo-level FPS baseline — 2026-07-29 (dev rig, editor, start camera)

Method: fresh MAX launch per demo → hub → `SELECT_DEMO` → `CLICK edit_game` → `CLICK_ONLY_LEVEL`
→ 30s soak → 3× `GET_PERF_DATA` (4s apart, ImGui rolling-avg FPS) → screenshot. Game `2aef3325`,
engine `b93f6df8` (post light-shafts 1.56). **All 19 demos loaded with ZERO failures** — includes
the Canyon family (old TDR suspect). Editor FPS swings ±8 between launches ([[project-performance]]),
so treat <10% deltas as noise when comparing.

| Demo (first level) | avg FPS | VRAM MB |
|---|---|---|
| Trapped | 184 | 6263 |
| Switch Escape | 181 | 6140 |
| Escape from the Zombie Cellar | 180 | 6399 |
| Bounty | 163 | 6853 |
| Aztec Game Kit Teaser | 133 | 7720 |
| Snowy Mountain Stroll | 121 | 7009 |
| Disruption | 120 | 6483 |
| A Grand Canyon Adventure | 120 | 6714 |
| River Raiders | 119 | 7985 |
| Operation Amazon | 118 | 7842 |
| Jungle Fever | 114 | 10427 |
| Indian Strike Force | 107 | 8018 |
| The Mystery of Z Island | 84 | **12976** |
| Aztec Game Kit | 83 | 10661 |
| Horseshoe Bend | 82 | 7692 |
| RPG Template | 81 | 7816 |
| Island Showdown | 75 | 6950 |
| Foggy Forest | 68 | 8361 |
| Canyon Offensive | 63 | 9419 |

Slowest tier (63-84): Canyon Offensive, Foggy Forest, Island Showdown, RPG Template, Horseshoe
Bend, Aztec Game Kit, Z Island — the perf-work test set. VRAM hogs: Z Island 13.0 GB, Aztec Game
Kit 10.7 GB, Jungle Fever 10.4 GB. Sweep script: scratchpad `demo_fps_sweep.sh` pattern (rewrite
from this recipe if scratchpad gone); per-demo perf blobs + screenshots timestamped 21:35-22:05
in `Files/screenshots/`. Demo-tab harness flow: [[project-harness-open-my-games]].

Related: [[project-performance]], [[project-harness-open-my-games]].

## 0826b (3.24, game `3aaff10d`) — gate CLEAN 19/19, POLYS identical to 0825 twice running

⚠⚠ **Rig drift, not code**: editor FPS sum 4013 → 3463 → 3282 across 0825/0826/0826b, **−18.2%**, on a workload 3.22–3.24 cannot touch. Game −11.0% over the same span and game-minus-editor positive on **19/19** (mean +8.9 pts) — right sign, right size, but cross-day diff-in-diff over two workloads and NOT evidence. Raw: `tools/sweep_0826b_3.24.txt`.

## ★★ Before trusting ANY FPS number in this file (3.24b/c, 2026-08-26)

| comparison | safe? |
|---|---|
| POLYS / VRAM, anywhere | **yes** — deterministic |
| demo vs demo WITHIN one sweep | **yes** — one machine state; position/drop correlation −0.04 |
| FPS ACROSS sweeps | **only if both were taken after a reboot** |
| a code change's cost | **never from a sweep** — same-session interleaved A/B only |

Every sweep now records `# UPHOURS=<hours since boot>` in its results file. **Check both files' UPHOURS before believing any cross-sweep FPS delta.** All FPS columns predating 2026-08-26 are unstamped session-state samples — including the 0825 'start of day' baseline, which post-reboot measurement showed was itself 2.8% low.
