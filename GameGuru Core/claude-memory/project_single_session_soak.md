---
name: project-single-session-soak
description: The 19-demo sweep relaunches MAX per demo and so cannot see accumulation; the single-session soak that can found real per-load VRAM retention and a gate that had silently stopped checking two demos.
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-19T19:52:51.247Z
---

**2026-09-19, game `7402c58e`.** Lee asked for a full demo sweep in ONE MAX session to see the
cumulative effect. **19/19 editor, 19/19 Test Game, 0 blank frames, 0 restarts**, 49 min, no
degradation with session length (last demo 42 s / 255 FPS). Tools:
`tools/demo_soak_sweep.sh`, `soak_analyse.py`, `soak_settle_repeat.sh`,
raw `tools/soak_0919_single_session.txt`. Notes §3.64.

★★★ **Two sweeps answer different questions - keep BOTH.** `demo_fps_sweep.sh` relaunches MAX per
demo (fair FPS, blind to accumulation); `demo_soak_sweep.sh` is one launch through
hub -> editor -> Test Game -> ESC -> hub x19 (sees accumulation, FPS still not cross-run comparable).
The per-demo-relaunch blind spot is what hid [[project-second-level-atlas]] for four weeks.

★★★ **VRAM ACCUMULATES ACROSS SEQUENTIAL LEVEL LOADS - real, ~93 MB/load, mechanism unknown.**
⚠ The 19-demo regression (3108 -> 5121 MB, slope +90 MB/load, r2 0.71, 16/19 over the 4096 gate) is
**CONFOUNDED and is not the evidence** - each level loads once, so order and content cannot be
separated and the list gets heavier toward the end. The evidence is the **same level twice in one
session**: Operation Amazon 3385.0 -> 3659.3 MB (+274), RPG Template 3276.3 -> 3372.6 (+96), POLYS
exact on all four. ⚠ They disagree 3x and resource COUNT moved +92 vs +5, so the mechanism is not
uniform - a direction, not a diagnosis.
⚠⚠ **Do NOT call this a min-spec failure.** C3's 4096 MB gate is defined per FRESH LAUNCH and all 19
still pass that way. This rig is 16 GB with a 15416 MB driver budget so `driver_usage_mb` is an
upper bound on a 4 GB card. Fair claim: *the figure C3 gates on rises ~90 MB per sequential load,
and C3 has never been measured across sequential loads* - which is what a shipped game does between
levels. See [[project-vram-floor]].

★★ **`sweepgate.sh` C2 held 17 demos, not 19** (fixed). The 3.53c comment was appended to the END of
the Aztec Teaser line and swallowed `"Aztec Game Kit"` and `"Bounty"`; a missing ref printed
`polys?` **without setting `c2 = False`** while the summary still said "all 19" because it prints
`len(rows)`. Third instance of *the scope of a check shrank and nothing announced it*. Comments now
go ABOVE entries; missing ref = `POLYS_NO_REFERENCE` + hard fail.

⚠ **A raw `sweep_*.txt` is a RECORD, not a REFERENCE.** I built a comparison from
`sweep_0826b_3.24.txt` and inherited its stale pre-far-tree Aztec Teaser 6454117; the curated dict
in `sweepgate.sh` (1413604 since 3.53c) is the authority.

⚠ **POLYS needs a 60 s settle, not 25 s.** 11 of 19 read low - **every one vegetation-heavy, every
sparse/indoor level exact**, which is the tell (under-settling lands on whatever streams last, it is
not random). A longer ladder returned the reference EXACTLY cold and warm: Operation Amazon
315943 -> 486602, RPG Template 351661 -> 540778. The 25 s/30 s boundary is sharper than a gradual
fill explains, so the variable may be FRAME COUNT not wall time - not pinned.

⚠ **Blank-frame detection by BYTE SIZE is wrong** - it conflates "blank" with "dark". Zombie
Cellar's editor frame is a fully rendered windowless interior at 977 KB that false-failed a 1 MB
threshold (luminance std 36.7, eyes-on confirmed). Threshold on **variance**.

Related: [[project-second-level-atlas]], [[project-vram-floor]], [[project-measuring-rules]],
[[project-demo-fps-baseline]]
