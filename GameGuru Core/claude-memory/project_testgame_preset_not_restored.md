---
name: project-testgame-preset-not-restored
description: "OPEN DEFECT: entering Test Game applies SetGlobalGraphicsSettings and nothing restores it on return, so every level loaded afterwards keeps that quality preset and can lose 35% of its triangles"
metadata:
  node_type: memory
  type: project
---

# ⚠ OPEN as of 2026-09-23 — found, named, deliberately NOT fixed

Three steps, silent, in the build today:

1. open any level, **Test Level**, **ESC** back to the editor
2. load a **different** level
3. it renders with up to **35% of its triangles missing**, for the rest of the process

Operation Amazon **486602 → 315943** triangles, scene objects **1850 → 1671** — 179 objects never
created, ~950 triangles each, i.e. trees.

```
M-GridEdit_part2.cpp:1045   if (pref.iTestGameGraphicsQuality != 2)
M-GridEdit_part2.cpp:1046       SetGlobalGraphicsSettings( pref.iTestGameGraphicsQuality );
M-GridEdit_part2.cpp:1790   //PE: restore SetGlobalGraphicsSettings here.   <-- A COMMENT
```

**The restore is a comment**, inside a ~120-line block that restores every other field from
`t.gamevisuals` one at a time. It is the one setting needing a FUNCTION CALL rather than a field
copy, and it is the one nobody wrote. Grass alone is handled (saved 1043, restored 1858).
`ggtrees_global_params.lod_dist` stays at the Test Game preset: **3000 HIGH healthy → 1000 LOW**.

**Fix shape:** mirror grass (snapshot tree/terrain params on entry, restore on exit), or re-assert
the editor's own quality on return. ⚠ It sits in the editor↔game transition BOTH sweeps run
through, so it needs a gate sweep after landing.

**Scope:** gated `!= 2`, shipped default is High(2), pref lives in
`%LOCALAPPDATA%\TGC\gamegurumax.pref` — OUTSIDE the build area, so testers inherit nothing.
⚠⚠ **This machine has it at Low**, so **every POLYS figure in a SOAK run in this repo is a
LOW-preset number.** Fresh-launch sweeps relaunch per demo and are unaffected.

## ★★★ The durable lessons

- **Third defect at that one call site** (2.39 was the same preset stamping authored shadow
  settings). **When a call site has produced one "applied and never undone" bug, audit everything
  else it writes** — do not fix the instance. See [[project-porting-clusters]].
- **A wrong explanation written into a comment stops anyone measuring it again.**
  `demo_soak_sweep.sh` recorded this as under-settling; it sat there from 09-19 and is why the
  09-20 pre-alpha report shipped without noticing. Refuted in minutes once questioned: the WARM run
  settles LONGER than the COLD run that reads HIGHER, all three samples are identical in both, and
  the warm figures reproduce BIT-IDENTICALLY against a soak on a DIFFERENT build. **A race does not
  reproduce to the digit** — that single observation converts "noisy measurement" into "state".
- **A plausible mechanism is not the mechanism.** The tree pool explained the symptom perfectly.
  `SET_TREEPOOLCAP 0` in the degraded state built 6000 pooled trees and POLYS did not move one
  triangle. One cheap test killed it; without it I would have fixed the wrong subsystem.
- **A knob tested in the state where it has nothing to do proves nothing.** I ran that same uncap
  in a HEALTHY session first and read "no change". Same mistake as 3.84's first light sweep.
- **Change ONE variable.** Probe 1 changed two (load count AND loading the subject first) and could
  name neither.

See [[project-measuring-rules]], [[project-rules-debugging]], [[project-second-level-atlas]]
(same family: per-level state in a process global), `PREALPHA_REPORT_2026-09-23.md` §3.3,
notes §3.86.
