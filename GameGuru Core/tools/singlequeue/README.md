# A/B sweep harness (built 2026-08-09 for the single-queue decision)

These are reusable for **any** knob A/B, not just single-queue — swap the `SET_*` command.
They encode four traps that each cost real time on 2026-08-09; keep them.

| script | what it does |
|---|---|
| `sqfull.sh [arm_s] [start_idx]` | All 19 hub demos, editor, three arms (0/1/0). Resumable. |
| `sqanalyse.sh` | Scores `sqfull_results.txt` against pre-registered criteria. |
| `sqgame.sh [arm_s]` | TESTPRO1 (editor) + a test-game pass over a demo subset. |
| `sqgameanalyse.sh` | Scores the above; **excludes vsync-pinned cases**. |
| `sqtp1.sh` | Five-arm (0/1/0/1/0) single-level repeat, for settling a borderline result. |

## The four traps these guard against

1. **Never launch with `nohup … &` inside a background tool call, and never trust `pkill`**
   (it silently does nothing under Git Bash on Windows). Three copies of a sweep once ran
   concurrently against one MAX, all writing the same log: 40 s arms "completing" 16 s apart,
   duplicate headers, and a fake **+96%**. Every script here takes a **PID lockfile**. Kill
   strays with `ps -W | grep /usr/bin/bash` + `kill -9`, and verify the survivor count.
2. **A fixed soak is not a settle gate.** A 90 s soak once sampled an arm mid-load at
   3.9 FPS / POLYS 154768 against a settled 106 FPS / POLYS 3438876. `wait_stable()` gates on
   **POLYS stable AND FPS within 3% of the previous sample**, twice running.
3. **Cross-launch drift on this rig is ~2.5%.** Always run **three** arms (A/B/A) so the
   A-vs-A2 control drift is visible, and treat any lone 3-arm result near ±2.5% as unresolved.
   `sqtp1.sh` is the five-arm tiebreak: TESTPRO1 read −2.5% on three arms and **+0.2%** on five.
4. **Test-game honours the per-level VSync setting.** A level that holds the refresh rate reads
   exactly 60.0 on *both* arms — that is the monitor, not the knob. `sqgameanalyse.sh` reports
   those as `vsync` and excludes them rather than counting them as neutral. It also means the
   over-16.7 ms hitch counter is meaningless there (a 60 Hz frame *is* 16.67 ms).

## Method

- Write the pass/fail criteria into the script header **before** running it, so the decision
  cannot be rationalised after seeing the data. `sqfull.sh` does this.
- Judge the fence stall from `SUBMIT_STALL_WINDOW` (rolling, reset per arm with
  `SET_SUBMITSTATS 1`), **never** from the `SUBMIT_PHASES_MS` snapshot — that is last-frame only
  and read 0.00 three times against a true mean of 0.89 ms.
- Always check POLYS matches across arms; that is the cheap proof of no content/visual change.

Results these produced: `../singlequeue_sweep_0809_full.txt` (19 demos, editor) and
`../singlequeue_testgame_0809.txt` (test-game + TESTPRO1). Analysis: `../../SWITCHESCAPE_PERF.md`.
