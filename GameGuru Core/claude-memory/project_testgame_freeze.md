---
name: project-testgame-freeze
description: "Entering Test Level parks the whole app at ZERO CPU - measured on the pre-port alpha too, so not from 3.38. Unknown whether it affects interactive use."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-15T16:51:44.122Z
---

# ⚠⚠ Test game freezes the application — cause unknown, blame NOT 3.38

Found 2026-09-15 while trying to verify the bullet-hole port.

## The measurement

Process CPU time sampled externally every 10 s. Editor is the control.

| run | 10s | 20s | 30s | 40s | 50s | 60s |
|---|---|---|---|---|---|---|
| editor (control) | +22.2 | +22.5 | +21.8 | | | |
| 3.38 exe, unfocused | +12.8 | +10.2 | **+0.0** | +0.0 | +0.0 | +0.0 |
| **2026-08-29 alpha exe** | +13.1 | +9.5 | **+0.0** | +0.0 | +0.0 | +0.0 |
| 3.38 exe, window FOREGROUND-verified | **+0.0** | +0.0 | +0.0 | +0.0 | +0.0 | +0.1 |

**Exactly zero CPU**, process alive, no crash log, `auto_log.txt` records
`CLICK test_level -> OK` and then nothing ever again. That is a thread parked on a wait — a modal
message pump or a lock — not throttling (which reduces CPU) and not starvation (which progresses).

## ★★★ What is ruled OUT, with reasons

- **Not 3.38.** The archived pre-port alpha exe (extracted from `Max - 300826.zip`) behaves
  identically. Same content, same level, same protocol, only the binary differs.
- **Not focus.** It froze FASTEST with the window verified foreground via `GetForegroundWindow`.
- **Not the harness poll.** `AutoHarness_CheckForCommand()` is the first line of `GuruLoopLogic()`,
  which runs every frame from `MasterRenderer::Update`.
- **Not CWD.** `AutoHarness_InitPaths()` uses `GetModuleFileNameA` — absolute.
- **Not the focus throttle.** The bypass is wired and correct: the harness sets
  `g_bAutomationActive` on the first command consumed (never reset), `main.cpp:318` turns that into
  `bKeepActiveEvenInBackground`, and that branch calls `master.RunCustom()` identically.

## ⚠ THE OPEN QUESTION — ask Lee first

Every run went through the automation harness. **Nobody has established whether test game freezes
when a human clicks Test Level.** That distinction is everything:

- freezes for a human too → **the shipped alpha locks up on first playthrough**, tester-blocking
- freezes only under automation → an automation limitation, and interactive use is fine

A two-minute manual check settles it. Do that before investigating further.

## Next instruments if it IS universal

Zero CPU means a wait. Attach a debugger and read the main thread's stack, or add a watchdog that
dumps `CaptureStackBackTrace` for every thread when no frame completes for N seconds. ⚠ Do NOT
repeat the window-enumeration check the way I first wrote it — see below.

## ★★★ The method failure this cost

My first attempt enumerated top-level windows looking for a MessageBox (class `#32770`). It
returned **nothing — including in the editor**, where there is unambiguously a window. I reported
"no dialog found" when the correct reading was "my check does not work". A test that cannot produce
a positive on a known-good case proves nothing.

**Verify the instrument on the control before trusting it on the subject.** Same family as the `bc`
vacuous-test rule in [[project-rules-environment]] and the `fopen` sweep in
[[project-cwd-and-file-paths]]. The fix was to use `Get-Process | MainWindowTitle`, confirm it
printed `GameGuru MAX` on a live run, and only then measure.
