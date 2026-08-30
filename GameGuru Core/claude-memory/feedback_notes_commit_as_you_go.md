---
name: feedback-notes-commit-as-you-go
description: "User-confirmed workflow: update memory notes, repo docs, and commit+push as each piece of work lands — don't batch it for the end of a session or wait to be asked."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
---

# Notes + commits happen on the fly, not on request

2026-07-18, after the 60 FPS perf session: the user asked "do I need to remind you about updating notes and pushing to the repo?" and confirmed "It looks you are doing this on the fly which works for me."

**Why:** Sessions end unpredictably (context, time, crashes). Work that is verified but uncommitted, or committed but undocumented, is the thing that rots between sessions — the 2026-07-17 review found exactly that kind of drift. Landing each milestone as verify → commit+push → update notes while context is fresh means any session can stop at any moment with nothing dangling.

**How to apply:**
- After each verified milestone (not each tiny edit): commit + push to main, then refresh the affected memory notes and repo docs (WICKED_ENGINE_CHANGES.md status table, resume file commit chain) in the same breath.
- Engine changes commit+push in the Wicked clone (`origin/master`) separately from the game repo — see [[project-wicked-engine-changes]].
- End-of-session should be a no-op sweep (both repos clean, notes current), not a documentation session.
- Related: [[feedback-commit-and-push-means-main]], [[feedback-autonomous-testing]].
