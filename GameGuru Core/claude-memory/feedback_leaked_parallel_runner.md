---
name: feedback-leaked-parallel-runner
description: "Harness scripts that outlive their launch silently corrupt every later measurement — three copies once drove one MAX at once. pkill does NOT work under Git Bash on Windows; use ps -W + kill -9, and give every long script a PID lockfile."
metadata:
  node_type: memory
  type: feedback
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-09T19:21:58.798Z
---

Long harness scripts can outlive the tool call that launched them, and a second launch then runs
**concurrently** with the first. Both drive the SAME MAX through the SAME `auto_command.txt` and
append to the SAME log. Nothing errors. The data is garbage.

**Hit for real 2026-08-09** during the SET_SINGLEQUEUE sweep: THREE instances were live at once
(19:53, 19:56, 20:00, 20:03 launches). Symptoms, none of which look like contamination at first:
- 40-second arms "completing" 16 seconds apart
- a level's first arm read at **3.9 FPS / POLYS 154768** while its settled state was 106 FPS /
  POLYS 3438876 — one script sampled while the other's MAX was still loading
- the same demo header printed twice; results rows carrying the wrong demo name
- an apparent **+96%** win for the knob, which was pure artefact

Two mechanisms caused the leak, both worth knowing:
1. **`nohup ./script.sh &` inside a `run_in_background` Bash call.** The wrapper exits instantly,
   the harness reports exit 1, and the child either dies or (worse) survives unowned. Launch the
   script DIRECTLY with `run_in_background: true` — no `nohup`, no `&`.
2. **`pkill -f script.sh` silently does nothing under Git Bash on Windows.** It reports success
   and kills nothing, so a "cleanup" step gives false confidence.

**How to apply:**
- Kill with `ps -W | grep /usr/bin/bash` then `kill -9 <pid>` on the listed Git-Bash PIDs, and
  **verify the survivor count** before relaunching. Never trust `pkill`.
- Give every long harness script a **PID lockfile** that refuses to start a second instance:
  `if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK")" 2>/dev/null; then exit 3; fi; echo $$ > "$LOCK"`
  plus `trap 'rm -f "$LOCK"' EXIT INT TERM`. This is the only defence that does not depend on me
  remembering.
- **Sanity-check pacing before trusting a sweep**: demo headers should be minutes apart and arm
  timestamps should match the configured arm length. If they don't, suspect a second runner
  BEFORE interpreting any number.
- Related trap: a fixed soak is not a settle gate. Gate on POLYS stable AND FPS within 3% of the
  previous sample, or an arm can land mid-load. See [[project-switchescape-perf]].

Related: [[feedback-stale-auto-command]], [[feedback-dont-thrash-on-automation]],
[[project-performance]] (Stage P.6 already recorded a leaked-runner poisoning one A/B round).

**Hit AGAIN 2026-08-14** (the 2.50 sweep), with three new teeth on the same trap:

1. **The lockfile does not protect a RELAUNCH.** Launch #1 (no tag) wrote the lock; my launch #2
   OVERWROTE the lock with its own PID; "kill the lock PID" then killed #2's ancestor while #1
   kept running — invisible to the lock. The lock proves "someone is running", never "only me".
2. **`nohup ... &` prints the WRAPPER pid, not the script pid.** Killing what nohup reported
   left the actual `./demo_fps_sweep.sh` process alive.
3. **Grep-based liveness checks match the CHECKER ITSELF** (the Bash tool's wrapper carries the
   pattern in its own cmdline) — three consecutive false "STILL ALIVE" readings.

**The recipe that actually works:**
- kill/verify by EXACT argv: `/proc/$p/cmdline` split on NUL, `argv[1] == "./demo_fps_sweep.sh"`
  (transient `$(...)` forks briefly clone the parent's argv — a count of 2 for one runner is
  normal; judge liveness over two samples, or judge by behavior).
- after sterilizing, verify BEHAVIORALLY: one lock, one MAX, the EXPECTED results filename
  growing, and no OTHER results file gaining mtime.
- monitors must PIN the exact results file — `ls -t | head -1` follows whatever file the zombie
  touches and reports the wrong run's rows as if they were yours.
- the sweep's TAG is `$1` and DEFAULTS TO 0806 — always pass a tag, or the run silently
  overwrites `results_0806.txt`.
