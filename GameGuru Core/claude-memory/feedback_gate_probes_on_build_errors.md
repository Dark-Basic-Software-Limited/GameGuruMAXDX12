---
name: feedback-gate-probes-on-build-errors
description: NEVER chain a probe after build.bat via a pipeline — tail eats the exit status and a failed build silently runs the probe on a STALE exe
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-15T18:51:59.774Z
---

`./build.bat Release 2>&1 | tail -2 && probe.sh` runs the probe EVEN WHEN THE BUILD FAILED —
the pipeline's exit status is tail's, not MSBuild's. In the 2.62 biome hunt this produced TWO
ghost rounds: probes ran a stale exe and "refuted" a fix that was never in the binary (and a
C2039 error scrolled away unseen because only the last 2 lines were kept).

**Why:** a false negative from a stale binary is indistinguishable from "the fix doesn't work" —
it sends the investigation back to theory-land while the code was actually right. Same failure
family as the stale `auto_command.txt` ([[feedback-stale-auto-command]]) and the
reproduction-under-failing-context rule: the experiment must PROVE it tested the new artifact.

**How to apply:**
- Build to a log, then gate on a REAL error count before running any probe:
  `./build.bat Release > build.log 2>&1; grep -cE "error C[0-9]+|fatal error|: error LNK" build.log`
  (pattern must not match `warning LNK4286` — plain `LNK[0-9]+` counts 60+ benign warnings).
- Belt-and-braces: check the EXE mtime advanced past the edit time before launching it.
- The exe dir is `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\GameGuruMAX.exe`.
