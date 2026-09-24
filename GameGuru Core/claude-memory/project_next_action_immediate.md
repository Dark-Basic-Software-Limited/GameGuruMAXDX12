---
name: project-next-action-immediate
description: Current state and the exact next step on GameGuru MAX DX12 — read this FIRST when resuming
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-24T21:00:00.000Z
---

# ▶▶ RESUME HERE — 2026-09-24 evening: EXTERNAL TESTER RESULTS INCOMING

Lee signed off and made a MANUAL build for a small group of external human testers from game
`1e8cb135` / engine `680a30b0` (everything through 3.101, all pushed, tree clean). The next session
opens with their reports.

**First move:** read `GameGuru Core/RESUME_2026-09-24.md`, then the triage guide at the end of
`NIGHT_INVESTIGATIONS_2026-08-12.md` ("LEE SIGN-OFF FOR THE TESTER BUILD") — it lists what is
already known (so it is not re-investigated), the least-tested paths (weight reports there), and
what the newest changes touch (3.98 float skinned normals reach EVERY skinned mesh incl. static
DBO props via SKINDUMMY).

**Do-not list still in force:** no file cleanup (Lee does it himself); never Debug builds; DX11 repos
read-only; never save Lee's projects except TESTPRO2; ask before killing MAX while Lee is working.
