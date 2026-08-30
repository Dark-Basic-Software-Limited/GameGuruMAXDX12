---
name: feedback-dont-thrash-on-automation
description: "When automation misses on the first or second attempt, stop and hand control back to the user — do not keep swapping demos or techniques hoping the next one works."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
---

When asked to launch MAX and screenshot a specific baseline (e.g. TESTPRO1 island level), if the automation harness can't reach that scene in ONE clean attempt, stop and hand it back to the user manually. Do not fall back to "let me try a different demo" or "let me try a different approach". The user is watching a clock — they want the result on the intended level, not a proof-of-concept on some substitute.

**Why:** Burned ~15 minutes on 2026-07-12 bouncing between TESTPRO1 → Foggy Forest → Zombie Cellar (residual state) → Aztec Game Kit Teaser → Island Showdown while chasing a screenshot of the Stage 2 trunk meshes. User interrupted with "Seems you are BAD at launching and grabbing the screenshot yourself. I will do it manually for now." The right move after the second miss was to stop and say so.

**How to apply:**
- ONE clean attempt at the intended baseline.
- If it fails, name the constraint (e.g. "harness can't reach My Games tab projects") and hand back to the user.
- Do NOT substitute a nearby demo for the intended baseline — the visual A/B is what the user cares about, and a substitute doesn't count.
- Do NOT keep the app cycling through recovery+load sequences.

**This generalises to CODE debugging too.** 2026-07-12 impostor-colour session: after the initial fix failed I burned two more speculative attempts (recapture countdown, green baseColor fallback) that produced no visible change, still on my own guesses about the Wicked internals rather than instrumented diagnosis. User called it: "You go off on seemingly random solutions and just corrupt the progress we have made." Same pattern. Same rule applies: after ONE failed guess-fix, stop, write down what was observed vs expected, and either (a) instrument for real signal or (b) hand back for direction. Do NOT keep committing "attempt N" hoping the next guess sticks.

## Concrete constraint — since resolved

The constraint documented here (`CLICK edit_game` silently no-ops for My Games projects) was resolved the same day by the `OPEN_PROJECT` command — canonical details in [[project-harness-open-my-games]]. The behavioural rule above is what remains durable.
