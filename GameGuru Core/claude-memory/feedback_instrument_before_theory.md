---
name: feedback-instrument-before-theory
description: "When hunting a crash or corruption, build the instrument that NAMES the culprit before reasoning about which code path could be at fault. Static analysis of a plausible-looking path repeatedly produced confident, wrong theories."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-01T17:08:30.785Z
---

For a crash, corruption, or any "something writes/reads the wrong thing" bug: **spend the first
build cycle on an instrument that identifies the culprit, not on reading code to work out which
path could be responsible.** Theory-first reads plausible and is repeatedly wrong; the instrument
is usually smaller than the analysis and settles it outright.

**Why:** 2026-08-01, the texture-streaming load crash (AV inside `memcpy`, killed two shipping
demos). I formed two confident theories from source reading — the background streaming thread's
partial file re-read, and the decrypt→read→re-encrypt cycle leaving a file encrypted when
streaming re-read it — and wrote both into the code comments and the notes as "prime suspects".
**Both were wrong.** I then spent a long stretch checking candidate paths by hand; every single
one's arithmetic checked out clean, which should itself have been the signal to stop.

What actually solved it, in two build cycles:
1. **A symbolized `StackWalk64` in the crash handler.** First repro showed the fault was on the
   MAIN thread inside the INITIAL upload — instantly demolishing the streaming-thread theory that
   hours of reading had reinforced. A crash inside a CRT routine like `memcpy` names the victim
   and never the caller; without a stack it is unfalsifiable guesswork.
2. **A per-load breadcrumb**, flushed per line, so the last line written when the process died
   named the exact texture (`DOOR1_surface.dds`, 500×500 DXT1) and its numbers.

Root cause was somewhere neither theory pointed: the mip reduction halved a block-compressed
texture's dimensions past block alignment, `GetCopyableFootprints` refused the desc and wrote
sentinel values instead of failing, and the guard meant to catch that is dead code on 64-bit.

**How to apply:**
1. Ask first: *"what artefact would name the culprit outright?"* — a stack trace, a per-item
   breadcrumb flushed before the dangerous call, a table dump of what the API expected vs what it
   was handed. Build that.
2. Prefer instruments that are **permanently worth keeping**. The stack walk now pays off for
   every future crash in this product; the cost was one edit.
3. Treat "I checked the arithmetic of five candidate paths and they were all fine" as evidence the
   model is wrong, not that the sixth path is the one.
4. Write theories down as *suspects*, never as findings, until an instrument confirms them — and
   go back and correct the notes when they turn out wrong.

Related: [[feedback-two-attempts-change-approach]] (two failures = wrong approach) and the grass
post-mortem in [[project-vram-census]] (a metric that cannot see the failure mode proves nothing).
