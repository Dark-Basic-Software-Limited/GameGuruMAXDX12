---
name: project-next-action-immediate
description: Current state and the exact next step on GameGuru MAX DX12 — read this FIRST when resuming
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-10-11T01:40:00.000Z
---

# ▶▶ RESUME HERE — 2026-10-11: TESTER FIX CYCLE IN PROGRESS

2026-10-11: report #4 fixed (3.107, game `c8e5a62b`, ENGINE `c1f54bd9` - first engine change since the tester build): placing a character drew its never-written skinning buffer (splayed polygons) + held characters drew an old pose whenever an object was on the cursor. Awaiting Lee. Kill MAX freely; parallel review per fix.

## 2026-10-10 late

Read `GameGuru Core/RESUME_2026-09-24.md` FIRST (its top block is the 10-10 state). Tester reports #1-#3 fixed (3.102 + 3.103 Lee-confirmed; 3.104 awaiting), hub test 1010 CLEAN (3.105), grass 64-bit selection (3.106, awaiting). Engine unchanged since the tester build (680a30b0). Next: more tester reports via Lee -> TESTER_TRIAGE.md. Kill MAX freely during bug fixing. Run a parallel review workflow on every fix.

## Earlier: 2026-09-24 evening - EXTERNAL TESTER RESULTS INCOMING

Lee signed off and made a MANUAL build for a small group of external human testers from game
`1e8cb135` / engine `680a30b0` (everything through 3.101, all pushed, tree clean). The next session
opens with their reports.

**First move:** read `GameGuru Core/RESUME_2026-09-24.md`, then the triage guide at the end of
`NIGHT_INVESTIGATIONS_2026-08-12.md` ("LEE SIGN-OFF FOR THE TESTER BUILD") — it lists what is
already known (so it is not re-investigated), the least-tested paths (weight reports there), and
what the newest changes touch (3.98 float skinned normals reach EVERY skinned mesh incl. static
DBO props via SKINDUMMY).

**Tester reports so far:** #1 (2026-10-10) picking the last palette trees tore the terrain apart - FIXED 3.102 (`e7b11178`), Lee-confirmed. #2 storyboard Take Screenshot never reached the level node - FIXED 3.103 (`96b52e3a`), Lee-confirmed. #3 RPG HUD map blank + Save New Level thumbnail - FIXED 3.104 (`e715e5c3`, DX12 image loader now honours the writables redirect), awaiting Lee. Harness gained `TYPE_TEXT`. **Hub test 1010 (3.105, 2026-10-10 evening): effectively CLEAN** - first gate since 3.96; fresh POLYS 19/19 = 0924, C3 281 MB headroom, soak 19/19 / 0 restarts / 0 blank. Raw `tools/sweep_1010.txt`, `tools/soak_1010.txt`, prereg `tools/prereg_1010.txt`. Cross-review fixed two minor 3.102/3.104 regressions (3.105b, exe f20fd9b2). 3.106 (`a5a7bfdf`): grass palette selection is 64-bit end to end (unsigned JSON read + legacy repair, all-slot flag seeding, custom-grass OOB fixed); harness SET_PALETTEBITS / SET_GRASSCUSTOMSLOT. Both logged in `TESTER_TRIAGE.md` §6. Lee's level node is now `testpro2` (not `testpro2level`); `tools/harness/open_project_level.sh` defaults to it.

**Do-not list still in force:** no file cleanup (Lee does it himself); never Debug builds; DX11 repos
read-only; never save Lee's projects except TESTPRO2; ask before killing MAX while Lee is working.
