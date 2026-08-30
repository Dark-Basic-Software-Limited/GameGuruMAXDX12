---
name: feedback-stale-auto-command
description: "A harness run that dies mid-sequence leaves its last command in auto_command.txt, and the NEXT MAX launch executes it — silently contaminating the following experiment. Always delete the file when a run aborts."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-09T03:44:44.857Z
---

`auto_command.txt` is not consumed-and-cleared on process exit. If a harness script is killed
(bash timeout, MAX crash, Ctrl-C) after writing a command but before MAX reads it, the file sits
there — and **the next MAX launch executes that stale command during startup.**

Hit for real 2026-08-09: a timed-out A/B left `SET_WEAPONDEPTH 0` in the file. The next launch
disabled the weapon depth carve, and the following investigation read `forcedepth=0` and briefly
treated it as a real finding. It was self-inflicted.

**Why:** the state it sets is invisible in screenshots and survives into every later measurement in
that session, so it looks like a property of the build rather than of the harness.

**How to apply:**
- `rm -f "$D/auto_command.txt"` at the START of any harness script, and again in any abort path.
- When a readout disagrees with what the build should be doing (a knob reading 0 that defaults to 1),
  suspect harness contamination BEFORE suspecting the code — check the file's mtime against the
  launch time.
- Any long harness sequence should be idempotent about knob state: set every knob it depends on
  explicitly rather than assuming defaults.

Related: [[project-harness-open-my-games]], [[feedback-instrument-before-theory]].
