---
name: project-review-2026-07-17
description: "Full project review (2026-07-17, Fable): state of play, strategy advice on the trees→performance plan, git cleanup plan, repo-doc sync plan. AGENDA for a discussion session with the user — advice items here are NOT yet approved."
metadata: 
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
---

# 2026-07-17 Project Review — things to remember + advice for discussion

Six-agent review (memory notes, repo docs, git archaeology, code-vs-notes check, strategy, housekeeping). Memory notes were cleaned the same day.

## EXECUTED same day (user granted full permission + had made local backups)

- **Git cleanup DONE** (except two items below): frosty-ritchie worktree removed; branches `claude/frosty-ritchie-f7efe6`, `claude/determined-chebyshev-bf0892` (origin), `phase4-dx12-init`, `phase5-imgui-dx12` deleted; pruned.
- **Repo-doc sync DONE** — commit `f6c0f149` (all 10 docs updated per the plan below).
- **Step 0 instrument-trust DONE** — commit `72b2de27`: .ele version logged on every load + LOUD mismatch warning (MessageBox suppressed when `g_bAutomationActive`); TREE TYPES DROPPED warning in GGTrees_SetData; stale comments fixed. **Verified: TESTPRO1 loads map.ele v341 with its 1 element — the baseline is currently CLEAN**; the tripwire arms the moment DX11 re-saves it as v342. The leftover 5233fb3c perf-panel diagnostic was already gone (c4d81543 removed it).

## STILL OPEN for user decision

1. **stash@{0}** — user skims `git stash show -p stash@{0}` once, then drop (verified redundant).
2. **This session's worktree** (`determined-chebyshev-bf0892`) + its local branch + the WickedEngineDX12 junction — remove from a session NOT running inside it (`git worktree remove --force`, `git branch -D`, `rmdir` the junction ONLY). **Future sessions should work directly in the main repo** — the mirror workflow is retired with the worktree.
3. **Step 1 sequencing** (trees: fresh A/B first, three pool fixes, tint test, then maybe billboards) — see below.

## State of play (verified)

**Solid:** Terrain arc done (blendmaps, camera-excursion, chunk_scale=80, DX11-style CPU blend `42e927b8` — ordering/gating verified in code). Grass complete through B.10. Automation harness is the project's best asset — all 23 WETEST.md commands verified against `AutomationHarness.cpp`; the ~50s cold-launch→TESTPRO1→screenshot A/B loop works. Git clean: main == origin/main, and the review proved byte-level that both worktrees, both `claude/*` branches, and stash@{0} contain NOTHING main lacks. Trees pool design (10K, nearest-N, impostors retired) matches its documentation exactly.

**Fragile:** (1) **The A/B instrument itself** — production DX11 confirmed writing `.ele` v342 on disk (`mapbank/island - Copy/map.ele` = 342) while DX12 reads max 341 and SILENTLY loads zero entities. One re-save of TESTPRO1 in DX11 invisibly corrupts every subsequent parity comparison. (2) Three latent tree-pool bugs (see [[project-trees-phase5]] review-corrections section): unstable slot→tree assignment (TAA ghosting), `draw_enabled` ignored by the Wicked path, setup latch race. (3) Green branch tint is a PERMANENT multiply, not a fallback — a live parity-colour variable. (4) Repo docs still teach "colored cylinder" trees and the cwd-trap build invocation — the whole Jul 12-13 arc lives only in memory.

**Unknown:** whether TESTPRO1's packed level data is already v342 (entities may already be missing from the baseline); actual size of the far-horizon tree delta vs DX11; the 24x AI cost gap; tree shadows/reflections parity; visual cost of GPU particles being fully disabled (`GPUParticles_part0.cpp:1891` unconditional early-out — recorded only in a frozen Feb doc).

## ADVICE: recommended sequencing (trees-then-performance is right, with a prelude)

**Step 0 — Trust-the-instrument (~half a day, do FIRST):**
1. Make the v342 silent entity-drop LOUD (one warning line at the `M-Entity_part3.cpp:42` else-branch) and read back TESTPRO1's actual loaded version. Non-negotiable before further A/B work.
2. If the baseline is affected, port the v341→v342 delta now (one version step; diff DX11's `entity_saveelementsdata`).
3. One-time warning when tree types >= 38 are dropped (`GGTrees_part2.cpp` type filter) so the A/B says when the palette port is needed.

**Step 1 — Trees continuation (reordered internally):**
1. Run the fresh A/B FIRST and measure the horizon delta before writing any billboard code — billboards may be unnecessary.
2. Land the three cheap pool-correctness fixes while in the file: stable slot assignment keyed by tree index (fixes motion vectors — parity AND perf item); make `GGTrees_WickedUpdate` respect `draw_enabled`/`hide_until_update`; on `GGTrees_SetData` failure set `iUpdateTrees = 5` explicitly.
3. A/B the green tint — try (1,1,1,1) now the impostor rationale is gone.
4. THEN billboards if the measured delta warrants → wind sway → types>=38 palette port (mirror grass B.9 pattern).

**Step 2 — Performance phase:** PERFORMANCE.md Active Targets + the tree-pool tax items in [[project-performance]]. The baseline re-measure is now due (grass+trees online). Trees-before-performance is correct because the perf profile is meaningless until scene contents stabilise — optimizing first means optimizing a moving target.

**Ship-readiness caveat:** TESTPRO1 parity ≠ shippable. A release-hardening list exists beyond all active plans: GPU particles disabled, ten unconditional debug hotkeys (U/I/O/P/1-8/G in `GGTerrainWicked_Update` — fire during normal typing), v342 for ALL production levels, ImGui shader variants/multi-viewport/VR deferred since Feb. Keep a living tech-debt list so "done" has a definition.

## ADVICE: git cleanup (verified safe, awaiting go-ahead)

All content checks done with `git cherry`/`range-diff`/blob hashes — nothing below can lose work:
- **SAFE:** remove worktree `determined-chebyshev-bf0892` (`git worktree remove --force` — its "modifications" are byte-identical to main HEAD) and worktree `frosty-ritchie-f7efe6` (clean); delete branches `claude/determined-chebyshev-bf0892` and `claude/frosty-ritchie-f7efe6` local + origin; delete merged Feb branches `phase4-dx12-init`, `phase5-imgui-dx12`; `git worktree prune`.
- **NEEDS USER:** stash@{0} ("MAIN-REPO WIP + accidental Claude edits 2026-06-18") — verified fully redundant (every functional piece is in main; untracked component is empty; ~135 stash-only lines are superseded early grass tuning), but the label says "review and split" and stashes are sacred: user skims `git stash show -p stash@{0}` once, then drop.
- **FOOT-GUN:** `.claude/worktrees/WickedEngineDX12` is a JUNCTION into the real engine tree. Remove with `rmdir` only — NEVER recursive-delete `.claude/worktrees` while it exists.
- Note: this review session runs INSIDE the determined-chebyshev worktree — do the worktree removal from a session that doesn't.

## ADVICE: repo-doc sync (~30 min, deferred because it commits to main)

Highest value first:
1. **SCRATCHPAD.md** — add milestone rows for `f5c4866a`, `7a842b78..18a95978`, `42e927b8`, `2e0ad1ef`; rewrite Phase 5 status from "colored cylinder ACTIVE" fossil to Stage 4.3; add a living Tech Debt section (GPU particles, ImGui shader variants, multi-viewport, VR, .ele v342, E_INVALIDARG).
2. **CLAUDE.md** — Active Work section to 2026-07-17 reality; replace the full-path build.bat guidance with the cd-first form (docs currently re-teach the cwd trap the memory rule exists to prevent); mark the mid-file pre-port terrain overview HISTORICAL.
3. **WETEST.md** — same build-command fix; add the implemented-but-undocumented `SET_GRASS` command row; status-stamp the stale Feb "point light too dim" open issue.
4. **PERFORMANCE.md** — refresh date; target 4 "now due"; drop the obsolete no-engine-mods caveat; append tree-pool tax items.
5. **WICKED_ENGINE_CHANGES.md** — add section 1.0 for pre-existing port-era engine deltas (customDraw hooks, SRV 16→64, accessors, /MTd) — without them a re-cloned engine won't render, and the doc claims to be the complete restore record.
6. One-line historical headers on TERRAINPORT.md (+ "zero Wicked modifications" superseded note), DX11_to_DX12_Shader_Porting_Plan.md (+ fix its internal Phase 3 contradiction), MIGRATION_PLAN.md, AUTOMATION_MAP.md, PROJECT_MAP.md (+ ImGui-on-DX12 addendum). DX12_AUDIT.md needs nothing.

## Durable facts surfaced by the review (also folded into topic files)

- Perf-panel duplicates bug: FIXED `c4d81543` 2026-03-28 → [[project-performance]].
- v342 confirmed on disk; TESTPRO1 lives in the same Files tree DX11 saves into → [[project-level-version-debt]].
- Y=0 is transient via the `iUpdateTrees` engine retry; permanent only when trees hidden → [[project-trees-phase5]].
- Green tint permanent; slot instability → ghosting; setup latch race → [[project-trees-phase5]].
- Junction hazard + cleanup plan → this file.
