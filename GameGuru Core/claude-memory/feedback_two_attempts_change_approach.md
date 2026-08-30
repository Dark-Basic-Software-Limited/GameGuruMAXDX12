---
name: feedback-two-attempts-change-approach
description: "When two attempts at a problem fail, don't try a third variation of the same approach — step back and look for a fundamentally different technique or vantage point."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: b0ef787c-20f3-4f00-b034-5ecbac249822
---

If two attempts at the same problem don't work, that's a signal the whole approach is wrong, not that the parameters need tuning. Don't iterate on the current technique a third time — step back and ask what fundamentally different angle exists.

**Why:** Told 2026-07-10 during Stage B.10 slope-filter work. First attempt: CPU cell-scan with `GGTerrain_GetNormal` (failed — coarse DX11 normal map). Second attempt: CPU cell-scan with three local `GetHeight` samples (failed — a strand from a flat paint cell can still land on the adjacent cliff triangle via random-barycentric distribution). User's exact framing: *"Seems you are missing something fundamental here. two attempts means you are on the wrong track. Do you have a third approach to this, perhaps based on the original code but allowing for how our new grass system under Wicked performs?"* The right answer was to move the filter into the compute shader on the face normal of the exact triangle each strand sits on — a completely different place in the pipeline.

**How to apply:** After two failed attempts at the same problem:
1. Stop iterating on the current technique.
2. Explicitly ask *"what is the fundamentally different way to attack this?"* — different layer of the stack (CPU vs GPU vs shader), different data source (cell resolution vs triangle resolution), different vantage point (paint-time vs render-time).
3. Reference-check the user's hints — they often point at the shift in perspective, not the tuning.
4. If nothing obviously different presents itself, explicitly say so and ask; don't reach for attempt 3 in the same family.

Doesn't apply to routine iteration (adjusting a threshold, fixing a syntax error, addressing a compile error) — only when the *structure* of the solution isn't landing.
