---
name: project-second-level-atlas
description: The 2026-09-18 second-level corruption - a treeless first level left the billboard atlas "ready" but never created; found by bisection after eight refuted theories; the rules it taught
metadata:
  type: project
---

Full account: `GameGuru Core/NIGHT_INVESTIGATIONS_2026-08-12.md` §3.53. Related: [[project-far-tree-billboards]], [[project-testgame-freeze]], [[project-rules-debugging]].

## The bug (Lee's litmus: River Raiders -> hub -> Island Showdown -> editor -> Test Game)
2.99's `GGTrees_EnsureBillboardAtlases` marked the billboard atlas READY for a treeless level without creating `texTree`, once per process (`g_ftAtlasesReady` had no reset). River Raiders is treeless; Island Showdown then bound a texture that did not exist. Symptom changed with the era's renderer: device removed at 2.99-08-25, blank/stale compose at 08-28->HEAD. Island-first, same-level-twice and Island->River all worked, which is why it looked like "level loading" and was not.

## Fixed (GGTrees_part0.cpp): per-level atlas invalidation on `GGTrees_SetData`/`RepopulateInstances`; `Ready()` also requires `texTree.IsValid()`; `numValid` published after its buffer exists.

## Rules this cost a day to learn
- ★★★ **"It worked a few weeks ago" = BISECT FIRST.** Nine builds named it; a morning of instruments named eight other real defects and not the cause.
- ★★★ **A kill-switch sent after level 1 tests nothing about level 1.** Send switches BEFORE the first load or the exoneration is false (this is how the trees were wrongly cleared at 12:00).
- ★★ Bisect hygiene on two repos: date-match engine to game; restore build scripts and behaviour-neutral shim headers from main; verify BOTH trees clean vs HEAD before building (an interrupted checkout left one file at another commit); PID-lock the build area; never reuse a SKIP verdict; `say()` to stderr never stdout.
- ★ `STATE: editor` is the harness FALLBACK (not hub/storyboard/loading/game); a screenshot is the criterion. The sweep relaunches MAX per demo, so a second-load bug is invisible to 19/19.
