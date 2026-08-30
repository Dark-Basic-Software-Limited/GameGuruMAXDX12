---
name: project-measuring-rules
description: How to measure on GameGuru MAX DX12 - the hard-won rules about instruments, windows, A/B protocol and when a null result is the test's fault
metadata:
  node_type: memory
  type: project
---

# Measuring on GameGuru MAX DX12

Moved out of MEMORY.md verbatim (2026-08-25) because the index had grown past its size limit and
was only partially loading. Nothing here was edited; the index now carries one-line hooks to it.
Section numbers refer to `GameGuru Core/NIGHT_INVESTIGATIONS_2026-08-12.md`.
Related: [[project-next-action-immediate]], [[project-performance]], [[project-lowspec-offswitches]].

- ★★★ **Size the window to the rarest thing you could be WRONG about, not to the bugs you are testing.** 3.19 reported "one layout hash across 12 dumps" and that was true — yet a third shifting mechanism was live the whole time at **1 dump in 31** (a job's `thread_local` parent flipping, moving a row 82 lines). It took 78 dumps and a check written for a DIFFERENT feature to see it. A clean result bounds only the failure modes you had in mind. ★ Companion: **a specification can be satisfied exactly while the goal is missed** — 3.20's tick box met Lee's constraint to the letter and hid 3 rows of 127. Report the gap with numbers and leave the re-spec to him; do not quietly substitute your own rule. Notes §3.20.

- ★★ **Caching is a RISK CALCULUS, not a technique — judge it by the failure mode.** Same session, two caches, opposite calls: 3.16 REFUSED to cache `entity_loopanim`'s skip decision (invalidation surface = every entity-property edit; failure = **animations silently stop**), while 3.18 SHIPPED the footfall keyframe cache (failure = **a wrong footstep sound** — cosmetic, cannot crash). Ask “what breaks, how loudly, and would I notice?” before “can I cache it?” ★ 3.18 also: keep **ONE copy of the semantics** driven from either source — two copies drift (that is how 3.04 happened) — and make the OFF knob the TRUE old path, not a rebuild-every-frame straw man that flatters the result (0.49 vs the real 0.33). ★ VARIANCE corroborates: cached samples sat in a 0.03 ms band vs 0.18 ms for the pointer chase. Notes §3.18.

- ★★★ **Count the WORK, not the CALLERS.** 3.17: `entity_loopanim`'s footfall scan ran for all 243 dynamic entities (ffEnt) and restricting it to the 44 characters looked like free money — but the second counter, ffSets (animation-set nodes actually walked), went **2523 → 2508**. The 199 non-characters contributed ~15 nodes between them; the cost is 44 characters × ~57 sets. Measured CPU 5.63 vs 5.64 = nothing, so the guard was **NOT shipped**. Had I instrumented only the caller count I would have changed behaviour for zero gain. ★ Always pair a “how many callers” counter with a “how much work” counter. Notes §3.17.

- ★★★ **3.15: the editor's hottest CPU row was a per-frame ENGINE RAY-INTERSECT over every waypoint node.** `waypoint_mousemanage` ran `PositionObject`+`HideObject`+`IntersectObject` per node, every frame, just to hover-test the mouse (plus `CameraPositionX/Y/Z` per node for a constant). Fixed with an analytic closest-approach-to-segment pre-reject: **P2-mainfunc 0.56 → 0.01 ms, +6.4% FPS**. ★ The reject is a conservative BOUND (radius 25 vs the proxy's true 21.65 half-diagonal) — safe direction, never used as a position. ★★ **Both sub-splits refuted my prediction**: I expected the 330-line rotation block and got 0.00, twice over. Bisect, never guess, even inside one function. `SET_WAYPOINTFAST`. ⚠ not interactively verified — nobody has clicked a waypoint on this build. Notes §3.15.

- ★★★ **3.14: the pole inside a hot system was a REBUILD of something already correct.** `SET_SCENESERIAL 1` named it in one reading (SU-Hierarchy = 71% of Scene-S2), but the fix was upstream of the walk: `StartBuildTopDownHierarchy` repopulated all 8,774 topdown entries EVERY frame. ★ The consumer already validated the snapshot from (mutation stamp + count) — **if a pair is good enough to TRUST a cache, it is good enough to SKIP REBUILDING it**. +3.0% FPS on the canyon level, ~80% of rebuilds gone. ⚠ Caching escalates what an invalidation counter must be right about: a missed increment used to self-correct (rebuilt from live data), now it leaves a STALE snapshot — so audit every mutation site before caching (found one: `wiScene_Serializers.cpp` deserialize did not bump it). ★ Report the paired FPS separation, not the mean CPU delta, when the within-condition spread exceeds the delta. Notes §3.14.

- ★★★ **The CPU panel printed a call TREE as a flat alphabetical list, so parents and children read as separate costs.** Lee caught it: `Update - Logic (Total)` 3.32 + `Logic - common_loop` 3.56 + `CL-GameLoop` 3.55 = 10.4 ms of a **7.99 ms** frame — one cost at three depths. 3.13: `wiProfiler` records depth/parent from a **thread_local** stack and prints a DFS tree with `[self]` and `[worker]`. ★ `[self]` is the number that answers “is this double counting”. ⚠ Two traps: “CPU Frame” is itself a CPU range and parents every main-thread row (excluding it from the cache orphaned the WHOLE main thread); and every row is a **20-frame rolling average on its own counter**, so children never sum exactly to the parent — report the residual as signed averaging skew, never as “unattributed”. ★★ 3.19 then had to make the SAME panel hold still twice over: the tree printer re-introduced 1.67's vanishing rows by skipping `num_hits == 0`, and cost order could not be stabilised at all (see the noisy-ordering rule above) — zero rows now print, siblings sort by NAME. Notes §3.13, §3.19.

- ★★ **A single cell over the noise floor is closed ONLY by an INTERLEAVED same-session A/B of the two BUILDS.** Alternate round-by-round so each rides the same thermal ramp; all-A-then-all-B rebuilds the drift confound you are testing for. A mechanistic argument is a reason to RUN the test, never a substitute. ★ Keep a same-session reading of the REVERTED build — on 08-19 the verdict came from the reverted binary drifting 92.7→85 over four hours, not from the A−B delta. Notes §2.92b.

- ★★★ **An ordering derived from a noisy measurement CANNOT be made stable — stop tuning and remove the dependency.** 3.19: the perf panel's rows are 20-frame averages of sub-ms work that swing 10× between reads (`CL-EntityProps` 0.02→0.23 over ten samples). A latched sort key with a deadband was written and MEASURED first: row COUNT went stable 133/133, name-order hash still differed on all ten dumps. Sorting by NAME fixed it outright. ★ Try the clever fix, but measure it before believing it, and be willing to throw it away for the dull one. Notes §3.19.

- ★★★ **A cache or fast path justified by an INVARIANT can be tested against the thing it replaced, offline — do not wait for a human to exercise the UI.** 3.19 closed two owed verifications in an hour that had been waiting days on Lee: `TEST_WAYPOINTFAST` ran both the 3.15 analytic reject and the engine ray test on every node over 222,750 cases (0 false negatives, tightest margin 7.17 of 25), and `TEST_ELANIMFFCACHE` compared the 3.18 footfall cache to the live list element-by-element and IN ORDER over 29,076 values in editor AND test game (0 stale-accepted). ★ Both produce a number a click never could — how much headroom the bound has, how many values were checked. ★ Write the test that names the INVARIANT, not the test that reproduces the gesture. ⚠ A matching COUNT is a weak witness: a short, stale or mis-ordered array totals the same (3.18's original evidence). Notes §3.19b–c.

- ★★★ **A texture-resolution change can only be measured where the texture is sampled near its TOP MIP — choose the camera before believing the null.** 3.19: a pure-ground crop at the level's start camera said Texture Detail did not touch the terrain (7.372 → 7.343 edge energy, −0.4%). At that pose the ground runs away at a glancing angle so the sampler is already deep in the mip chain. Pitch down 55° and the same measurement reads −12%, obvious by eye. Third costume this month for "audit the TEST, not the code".

- ★★★ **Driver VRAM cannot measure a texture-size change — sum the DESCRIPTORS.** 3.19 read VRAM 2709→2773→3061 MB while dividing textures 1→4→2 and nearly shipped that as a regression. Two confounds: the allocator keeps freed heaps, and editor streaming had already walked the textures below the divided size. Summing `ComputeTextureMemorySizeInBytes` over what was actually swapped gave the truth: 218.8 → 59.6 MB. Notes §3.19.

- ★★ **When an effect is ~0.3 ms on a 4 ms frame, measure the ROW you changed, not the frame** — and ask WHERE it is: main thread vs worker changes what the win means.

- ★★ **A "total" is not a "sum of parts".** Wicked's `GPU Frame` is a wall-clock SPAN (holds unranged passes, clear/store/resolve, idle) and tends to the frame period. 2.91 adds `GPU Busy` (**UNION** of children — a SUM double-counts because ranges NEST) and `GPU Idle + unranged`. Notes §2.91.

- ★★★ **A PIX/debug marker is NOT a profiler range — an idle bucket is an ACCOUNTING HOLE, not evidence of idling.** `device->EventBegin/EventEnd` is invisible to `wi::profiler`, so a marker-only pass is GUARANTEED to read as idle. Hid 3.96 ms/frame for the whole project. Use `DUMP_GPUGAPS` (2.94c), which names the ranges BRACKETING each hole. PIX was never needed.

- ★★ **A POP is TEMPORAL — a static A/B screenshot pair cannot show it**, both endpoints are the same picture. 3.03's pair came out 104 bytes apart and proved nothing; POLYS was the signal that worked. Ask whether the defect lives BETWEEN frames, then find a scalar the mechanism must move.

- ★★ **An instrument that bypasses the suspect path can only ever exonerate it** — the debug probe sphere has no parallax, so it could never show a parallax defect (a night lost). Live-scene whole-frame diffs have a 3-5% animation floor — use a predicted-region mask.

- ★★★ **Never name a tuning knob from reading the code — measure which DIRECTION it moves things first.** 3.07: I named a `lerp` constant as the softening dial; lowering it made the shade 2× darker. ★ Offline analysis of the source ASSETS (python over the actual DDS + the shader maths) ranked five candidate levers in minutes — a first-class instrument here, not just in-engine A/B.

- ★★★ **Before believing a null result, audit the TEST, not the code.** Twice in one night: the 3.08 switches measured zero on a level with no shadow/bloom/AO row to remove, and Object Detail Distance measured zero at 8000–40000 units on a level 200 m across (it worked instantly at 500). Ask “is the thing I am switching off even RUNNING here, and is my knob's range inside the content's scale?” ★ Corollary: grep the **getter** (`getShadowsEnabled()`), not the member — five working levers looked dead because every call site uses the accessor.

- ★★ **When an instrument rules out a whole STAGE, believe it and move on** — do not go back to reading the picture. ⚠ And check each bisect row actually REACHED its path: `SET_TREEMESHFADE` applies at pool BIND, so reading it without churning the pool passed VACUOUSLY.

- ★★★ **A sparse sample can show that something HAPPENS; it can never show that something NEVER happens — and I used it for the second.** §3.20 reported "32 rows print 0.00 in all 25 dumps" and Lee made a design decision on it; the true figure was **12**, because 25 dumps two seconds apart is one frame in ~540 and the other rows do cross the threshold, just not on the frames I caught. Fix: instrument the ENGINE to carry the extremum over every frame (`Hits::peak_time`, `DUMP_IDLEPEAKS`) rather than reconstructing it from dumps. ★ Same day, opposite end: §3.19's "one hash across twelve dumps" missed a **1-in-31** re-parenting event (§3.20). Too few samples to SEE a rare event, too few to RULE ONE OUT — both are the same mistake. Ask what the sampling rate is against the thing's period before quoting a count. Notes §3.20, §3.20a.

- ★★★ **Before reporting the SHAPE of a behaviour, ask how many periods of it your window contains.** §3.20a watched a threshold for **24 seconds**, saw "71, 72, 74, 75", called it CHURN and recommended against shipping. §3.20b watched the same thing for **three minutes** and every one of eleven changes was an **INSERTION** — monotone growth to an equilibrium, a completely different and perfectly acceptable behaviour. The short window was not noisy, it was too short to show a curve. ⚠ **Third window failure in two days**, all different costumes: too few samples to SEE a rare event (§3.19, 1-in-31), too few to RULE ONE OUT (§3.20a, 12 rows reported as 32), too few to see a TREND. Notes §3.20b.

- ★★★ **When a change can only affect ONE of two measured workloads, the other one is a drift gauge — use it before attributing anything.** §3.22a's sweep read game FPS −6.3% and it looked like a regression from the change that had just shipped. But the EDITOR read −13.7% on **19 of 19** demos, and the change cannot touch the editor at all (entity logic does not run there). So the rig had moved, the game had held up **+7.3 points better than the unaffected workload on 18 of 19**, and the apparent regression was the opposite of what happened. ⚠ Then do NOT promote that into proof: it is cross-day difference-in-differences over two different workloads, so it corroborates the same-session interleaved A/B and never replaces it. Notes §3.22a.

- ★★★ **Big-O is a claim about the LIMIT, not about your data — measure at the size you actually have, and get that size from the running game.** §3.24 replaced a linear `strcmp` scan with `std::unordered_set<std::string>` and it was **2.7x SLOWER** at the real size (231 ns vs 85 ns at 39 entries; it only won past ~110). `find()` on a `char*` builds a temporary `std::string` — past SSO, so a heap allocation — and hashes it every lookup, losing to a handful of strcmps that fail on the first character. It compiled, it was in sync, the game ran, and every instinct said it was an improvement; only the benchmark disagreed. ★ The size assumption was wrong too: "the list is empty in a healthy project" turned out to be **39**. ⚠ Third costume in two days for the same lesson — §3.17's caller-count guard did nothing, §3.22's obvious suspect was 5.6 us of 531, and this. Notes §3.24.

- ★★★ **Ask whether the drift is WITHIN the run or BETWEEN runs — they have opposite consequences, and the data you already have can tell you.** §3.24b: a hub-wide −18.2% editor drop looked like it might be a 50-minute sweep cooking the machine, which would have made even demo-vs-demo comparison inside one run unfair. Correlation between a demo's POSITION in the run and its drop was **−0.04** (first half −17.5%, second half −17.8%) — the full loss was already there on demo 1. So it accumulates between SESSIONS: within a sweep everything is measured in one machine state and is fair; across sweeps FPS is meaningless. ★ A reboot recovered **all** of it (+34.9%), which also proved the earlier read that the editor — a workload the change could not touch — was a valid drift gauge. ⚠ And it showed the 'clean' baseline was not clean: post-reboot sat **+2.8% above** the start-of-day sweep. **Reboot before any sweep whose FPS you intend to compare.** Notes §3.24b.


## ★★★ 20. THE REPORT IS THE FEATURE — and it must state a FACT, not an intention (3.25)

Nine defects across the Terrain/Water Bake work and **not one announced itself**: every one
produced correct counters, no crash and no log entry. `DUMP_BAKE` found them all. Nothing else
would have. Build the report before the feature.

But an instrument that reports an INTENTION is worse than none, because it agrees with you:
- the bake report printed `CurrentWaterColor()` **evaluated at print time**, so it showed magenta
  whether or not one magenta vertex had ever reached the vertex buffer. Fixed by printing
  `g_lastColor` — the buffer's actual contents — plus `updates / rebuilds / draws` counters. Those
  three numbers reading healthy are what finally proved the fault lay outside my code.
- a "solid-colour bisect" that forces a colour into the VERTEX still runs the pixel shader's own
  maths. Mine still ran `ApplyFogCustom`, so fog could have absorbed the entire test. **A bisect
  that runs the shader's maths is not a bisect.**
- `0 chunks promoted` could not distinguish "found no entities" from "found entities but nothing
  overlapped". One extra field named the cause instantly.

Same family as "a count of CALLS is not a count of SUCCESSES" (§2.96).

## ★★ 21. SIZE THE TEST TO THE FAILURE, AND CHECK IT REACHED IT (3.25g)

A bake-cycle test with generous waits passed cleanly four times and proved nothing — every build
succeeded in **1 attempt with 0 chunks not ready**, so it never entered the failing state at all.
The giveaway was in the instrument's own output. The real test moved the CAMERA between toggles to
keep chunks generating, and the bug appeared instantly.

**A clean result from a test that cannot reach the bug is worse than no test**, because it reads as
confirmation. Ask what state the bug needs, then check the run actually got there.

## ★ 22. WHEN EVERY INSTRUMENT SAYS YOUR CODE IS RIGHT, WIDEN THE FRAME (3.25f)

Magenta → no. Depth test ALWAYS → no. Fog bypassed → no. Real buffer contents and counters → all
healthy. At that point the answer was not a fourth instrument, it was **comparing against the two
custom draws that DO work**, which named the cause (`gpup_draw` clobbering the camera CB) in one
pass. Then a predicted A/B confirmed it in one screenshot.


## ★★★ 23. A CRITERION IS ONLY AS DETERMINISTIC AS THE STATE IT SAMPLES (3.25p)

The sweep gate's C2 (POLYS identical to reference) was the criterion I trusted MOST, and I had
written in the sweep banner that "C1-C4 exclude FPS and are unaffected" by machine state. True of
the criteria's INTENT, false of C2's IMPLEMENTATION: POLYS is deterministic only once a scene has
finished streaming, and at 14 h uptime a 30-second soak is not always enough. C2 failed on three
demos that direct re-measurement showed were perfectly intact.

Two corollaries paid for the same evening:
- **All three failures were UNDER the reference, never over.** A one-sided error distribution is a
  signature of a measurement artifact, not of a defect - real geometry loss has no reason to be
  monotone. Read the SIGN of a set of discrepancies before believing them.
- **The obvious hardening did not work.** Taking the MAX of the three samples closes the case where
  only the last sample is early, but re-deriving the failing sweep from its own saved samples
  changed nothing: all three were identically low. Verify a fix against the data that produced the
  failure before claiming it addresses it.


## ★★★ 24. A RUNTIME A/B CANNOT TEST A SETTING THAT IS ALSO LIVE DURING CONSTRUCTION (3.25s)

Reduction Scale made POLYS non-deterministic (Horseshoe Bend settled anywhere from 81,302 to
4,038,923 - steady within a session, different between them). I A/B'd it early by toggling
`SET_ANIMREDUCTION` at runtime, saw the same value at 25 and at 1, and **cleared the right
suspect with a test that could not convict it.**

The damage was done during LEVEL LOAD: a newly created armature was eligible for holding before
its phase came up, so for up to period-1 frames it was never posed, its objects carried un-posed
bounds, and culling/LOD/occlusion settled the scene on those bounds - permanently.

**If a setting is live while the scene is being CONSTRUCTED, it must be A/B'd across LOADS, not
within one session.** Flipping it afterwards cannot unwind a decision already taken.

Corollaries from the same day:
- ★★ **Bisect beats reasoning.** Five builds x four fresh sessions took ~90 minutes and settled
  in one pass what two days of argument had not. When a regression is real but not obvious, build
  the old version.
- ★★ **A one-sided error set suggests artifact; a two-sided one does not.** All-under looked like
  unsettled scenes (and I said so); the 2.55x OVER-count on the next run broke that read and was
  the signal to stop theorising.
- ★ **The gate's value is failing on what you were not looking for.** C2 (POLYS identical) was
  never designed to catch an animation-throttle defect. It caught one, because a geometry
  invariant is violated by anything that perturbs geometry.
