---
name: feedback-kill-max-after-10min
description: "If a pending build/test needs MAX closed and the user hasn't exited it manually within ~10 minutes, assume they are AWAY and kill MAX yourself — proceed autonomously. Waiting politely cost an hour once."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-07-27T16:40:45.315Z
---

If work is blocked on GameGuruMAX.exe being closed (game rebuild needs the EXE unlocked) and the
user has not exited it within **~10 minutes** of being asked, **assume they are NOT at the machine
and `taskkill` MAX yourself**, then build/relaunch/test autonomously.

**Why:** 2026-07-27 — a watcher waited for a voluntary exit (out of caution for an unsaved level,
`island.fpm*`); the user had walked away, the watcher timed out silently, and a full hour was lost.
User verbatim: "if I have not exited manually after 10 minutes, assume I am NOT THERE and exit it
yourself so you can perform more autonomously. We just lost a good hour."

**How to apply:** When needing MAX closed: announce it in chat, arm a ~10-minute watcher; on manual
exit → proceed immediately; on timeout → `taskkill.exe //IM GameGuruMAX.exe //F` and proceed. This
is user-authorized even if a level has unsaved edits (their explicit standing instruction). After
the work, relaunch MAX and restore a useful state (e.g. OPEN_PROJECT + level load). Relates to
[[feedback-autonomous-testing]] and [[feedback-dont-thrash-on-automation]].

## ⚠ ADDENDUM 2026-08-26 — check liveness in a SEPARATE command from the kill

I killed Lee's MAX while it held UNSAVED changes (`spotshadowtest.fpm*`). I had checked the window
title correctly twice before and held off; the third time I combined the `tasklist` check and the
`taskkill` into one command "for speed", so I saw the asterisk only in the output AFTER the process
was already gone. The check existed and was useless because it could not influence the action.

**Rule: when the decision to kill depends on a check, the check must be its own command.** A test
whose result arrives after the irreversible step is not a test. Applies to Lee's interactive
sessions specifically — during authorised autonomous runs (he is asleep, no unsaved work) killing
freely is still correct and wanted.
