---
name: project-rules-debugging
description: "Durable debugging habits — dead-code chains, calls vs successes, deferred controls, dead-knob taxonomy, edge-triggered switches, content levers. Read when a symptom makes no sense."
metadata: 
  node_type: memory
  type: project
  originSessionId: 509f3c47-3d86-4b9b-a337-23ada2c00769
  modified: 2026-08-30T23:43:22.862Z
---

# Durable debugging habits (verbatim from the index; each paid for at least once)

- ★★★ **FIVE layers of "this is dead, skip it" hid one feature, each citing the others** (the far-tree chain). When a comment says "this is dead", check whether the thing that killed it is itself a bug.
- ★★ **A count of CALLS is not a count of SUCCESSES — a bool return you discard is a diagnostic you threw away.** `atlasSlices=38` read as success while all 76 uploads failed; counting successes gave 0 and one captured reason string named the cause instantly. ★ Companion: a SOLID-COLOUR bisect (force the PS to one colour, KEEP the real transform) clears geometry, transform, camera CB, depth, blend and root signature in ONE reading. ⚠ size the debug primitive — a fullscreen quad × 98,410 instances killed the app.
- ★★★ **A control that is CORRECT-BUT-DEFERRED is indistinguishable from a broken one.** Texture Detail (3.12) applied at load time by design and said so in its tooltip; Lee reported it as doing nothing. Nobody reads a tooltip that explains why the thing they just clicked did not respond. Either make it act now or make the deferral visible in the UI. Notes §3.19.
- ★★ **When a scan loop deliberately SKIPS the items it already owns, the explicit refresh is the ONLY path that can ever update them — so it must fire on ANY change, not just the direction you had in mind.** 3.19's Object Detail Distance re-tightened on `< s_prevCull`, so the slider worked downwards and was inert upwards. Same family as "a visibility test in one pass and not its partner".
- ★★ **A dead UI knob fails in TWO ways** — *severed chain* (call commented out) vs *clobbered value* (something re-derives the field every frame). Ask **"who writes it LAST, and what derives it?"** [[project-env-probes]]
- ★★ **A shader edit is NOT LIVE until a VISIBLE change proves it** — and when it "isn't live", suspect the C++ VALUE PLUMBING first (an INI bool-ization, not the pipeline). Prove a knob's INI and LIVE paths equivalent. ★ ini knobs are INT PASSTHROUGH, never bool-ized.
- ★★ **An edge-triggered switch cannot be driven twice in one command** — a frame must observe the intermediate state. `SET_TERRAINGEN` set a flag on and off in one handler and a whole ladder reported the same value at every rung. **An edge-triggered teardown latch must clear on BOTH edges, and clearing state ≠ re-running the pass** — the 2.94 grass latch was one-way, and even fixed, grass would not regrow because `ProcessGrassChunks` is gated on dirty/sig/settle/nudge/camera-moved and a PARKED camera fires none (`g_grassPassNudge = true`).
- ★★ **"The element's object" ≠ "the entity's geometry"** — GG allocates MORE THAN ONE object per marker (a %probe = inner sphere + a separate 'root' shell). `DUMP_OBJENT` the object RANGE first. [[project-env-probes]]
- ★ **"Make X the size of Y" wants the HALF-EXTENT** — `AABB::getRadius()` is the half-diagonal (√3×). Before scaling something the engine draws for ALL of a class, read the loop: harmless at scale 1 ≠ harmless at scale 60.
- **Map record ≠ live entity — validate LIVENESS** before trusting a scene lookup. **A port diff must include the CALLER'S dt clamp regime.** **Never verify a time-dependent system on a short window**; `frac(sin(big·seed))` degrades with seed magnitude.
- ⚠ **Don't reach for the obvious content lever — three of four are LEVEL-DATA MUTATORS**: `GGTrees_HideAll` writes 400K instance words, `GGGrass_RemoveAll` zeroes the 16 MB painted-grass map, `visuals->bWaterEnable` is re-derived every load. `SetTerrainVisible(false)` is the fourth trap: chunks persist and `Generation_Update` keeps making more.
