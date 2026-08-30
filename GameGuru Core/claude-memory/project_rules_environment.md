---
name: project-rules-environment
description: "Environment/tooling rules — taskkill lies, bc missing, OPEN_PROJECT scope, modal error paths, CWD, C1061, heredocs, phantom tails. Read before writing harness or shell automation."
metadata: 
  node_type: memory
  type: project
  originSessionId: 509f3c47-3d86-4b9b-a337-23ada2c00769
  modified: 2026-08-30T23:43:37.832Z
---

# Environment / tooling rules (verbatim from the index; each paid for at least once)

- ★★★ **`taskkill` reporting SUCCESS does NOT mean the process died.** 08-22 a MAX instance wedged at 6.9 GB survived `taskkill //F` by PID and IM, PowerShell `Stop-Process -Force` and `Process.Kill()`, all reporting success, while it held the GPU and stalled later launches. **Only `tasklist //FI "IMAGENAME eq ..."` is reliable** — verify GONE, never trust the kill's exit status.
- ★★★ **`bc` DOES NOT EXIST in this Git Bash — and a test that never reaches the path PASSES VACUOUSLY.** `$(echo "$X+1000" | bc)` yields EMPTY, `SET_CAMERA` silently takes malformed args, and "I flew around, all fine" is worthless. Use `awk 'BEGIN{printf "%.1f", a+b}'`; ALWAYS echo the reply and READ BACK the state you changed.
- ⚠★ **`OPEN_PROJECT` only sees MY GAMES projects** — the 19 hub demos need `SELECT_DEMO` + `CLICK edit_game` + `CLICK_ONLY_LEVEL` (`tools/probe_one.sh`); a My Games project needs `OPEN_PROJECT` + `CLICK_ONLY_LEVEL`. A full ladder once ran on the hub's EMPTY scene and produced a plausible, worthless table. Gate on the LOAD, never on exit 0.
- **Legacy error paths are MODAL** (`RunTimeError` = MessageBox) — headless reads them as hangs. `timestampactivity` BUFFERS (loses tail under `taskkill //F`); hang forensics need open-append-close per line.
- **Runtime `fopen` files land in the game's `Files/` CWD**, not the exe dir. `wi::platform::Exit()` **RETURNS**. (Known writers now exe-anchored — see [[project-cwd-and-file-paths]].)
- ★ **Never leave working files in the build OUTPUT dir** — it is packaged and un-versioned. Scratchpad for throwaway, repo `tools/` for keepers. ⚠ its 5 loose `run_*.sh`/`perf_test.sh` are stale, not mine; `dxdiagsystemspecs.bat` is a PRODUCT file.
- ⚠ MSVC **C1061** in harness dispatch — hoist new commands into helper chains, don't extend the ladder. ⚠ Combined heredocs in one Bash call fail with "unexpected EOF" — split them.
- ⚠ `SELECT_ENTITY` is by index, not visibility. ⚠ Verify subagent claims by grep; Windows `tail -f` emits phantom partial lines. ⚠ `tail` in a pipeline withholds a background script's output until it exits, and killing the tail DISCARDS the buffer — redirect to a FILE.
- ⚠ **`cmd //c "call ... && msbuild ..."` from Git Bash can run NOTHING, silently** — write a .bat file and run that (2026-08-31, OGG libs). ⚠ **`cp -r` gives outputs fresh mtimes, so msbuild `/t:Build` on a copied tree is a silent no-op** — force `/t:Rebuild`, prove with md5sum.
