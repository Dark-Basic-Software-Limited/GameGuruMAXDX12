---
name: project-vram-retention
description: "VRAM accumulating across level loads: SEVEN fixed defects all of one shape (a per-level value in a process-global), the named VRAM census as the instrument, and the method lessons that cost the most."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-20T00:57:17.639Z
---

**2026-09-19/20. Lee: "find where the VRAM is being retained across level loads. We need zero VRAM
leaks!"** Notes §3.65-§3.68. Closed.

★★★ **THE INSTRUMENT IS `DUMP_VRAM`** (census, engine 1.70): every D3D12MA allocation recorded WITH
ITS DEBUG NAME at create, erased in `~Resource_DX12`. Load A -> B -> A in one session and diff the
two A dumps: content is constant, so anything extra is genuinely retained. Turned "2 GB
unaccounted for" into a named list in one run. Tools: `tools/vramleak_probe.sh`,
`tools/vram_union_sweep.sh` (19 demos + a repeat of demo 1 at load 20), `tools/union_analyse.py`,
`tools/compare_runs.py`.

**RESULT: per-load accumulation eliminated.** 19-demo single session went from
**+90.1 MB/load (r2 0.71) to +5.2 MB/load (r2 0.01)** at 3.66, and 3.68 then took the three
remaining named suspects.

★★★ **ALL SEVEN DEFECTS WERE THE SAME SHAPE: a per-level value living in a process-lifetime
global, which the next level only overwrites if it happens to supply one.** This is now the FIRST
thing to check when state seems to bleed between levels, and it is not only a memory bug - the
stale terrain paint map also fed the blend/slot mapping.

1. **Stale terrain paint map, +142 MB.** `pMaterialMap` is persistent 4096x4096; restored only
   `if (FileExist("<size>.ptd"))` with **no else**, so an unpainted level inherited the previous
   level's paint and instantiated materials for it.
2. **Orphaned material slot entities.** Auto slots called `Entity_Remove`; painted/layer-1 did not.
3. **`GPUP_DeleteTexture` was an EMPTY STUB, +28 MB.** ⚠ Half a fix: `RenderPassAttachment` holds a
   Texture BY VALUE, so the 5 render passes must be released too.
4. **Depth-chain keep-alive, +67 MB.** A 2.05 diagnostic labelled "Remove once the hunt closes"
   whose hunt closed on something else. Gated on `dred.txt` = every dev machine.
5. **Shadow packer containing size (3.68), -255.6 MB on a light level after a heavy one.**
   `wi::rectpacker::State::clear()` keeps width/height by design, `add_rect` only grows, `pack()`
   only DOUBLES. Fixed at the call site in `wiRenderer.cpp`, ⚠⚠ **NOT in `State::clear()`** -
   `wiFont`'s glyph atlas is the other user and wants the ratchet.
   ★★ **This and 3.66's armed shrink are ONE FIX IN TWO HALVES** - the reset fixes what the packer
   ASKS FOR, the arm lets the atlas ANSWER, and either alone measures as a no-op. That is why 3.66
   was written up as "the atlas is exonerated".
6. **Grass material cache (3.68), ~26 MB per session.** `g_grassMaterials[]` lazy, never reset.
   Released per level load; safe wholesale because the chunk create path COPIES the material into
   the entity (`scene.materials.Create(e) = *mat`), so nothing holds a pointer into the array.
7. **MSAA outline RTs (3.68), ~12 MB.** Created under an MSAA guard in `ResizeBuffers`, never
   released when MSAA went off - and MSAA is per-level.

★★ **Releasing the shadow atlas at a level load DOES NOT WORK** (tried, measured, reverted):
release and create land on the SAME FRAME, before the incoming level's visuals are applied.
Replaced by `GG_ArmShadowAtlasShrink()`, a one-shot permission that is order-independent.

★★★ **Method, all paid for on this hunt:**
- **A two-level A/B/A cannot separate two leaks that both predict "current union previous".** I
  reported a working fix as doing nothing because of this. Use **A -> B -> C -> A**.
- ★★★ **A bound derived from an instrument is only as sound as the instrument.** §3.67 argued
  12288 was "unreachable from the rects" using `DUMP_SHADOWRECTS`, which divided the sun's WIDTH by
  cascade_count and not its HEIGHT - and the same section had already listed that accessor as
  needing repair. **Never close an argument with an instrument you have queued for fixing.**
- **Four plausible theories died to measurement in one session.** Reasoning generated all four.
- **A one-shot diagnostic reports the wrong moment** - make it periodic; the number that mattered
  existed 30 s after the arm.
- **Empty-bodied `*Delete*/*Free*/*Release*` functions are worth sweeping for as a class.**
- **A deliberate, labelled, temporary diagnostic is a leak with a calendar on it.**
- ⚠ **`wi::backlog` writes log.txt only from its DESTRUCTOR** - a taskkill loses everything.
- ⚠ **First launch after an ENGINE build needs a long warm budget** (900, not 300), AND a `sleep`
  before the first `alive` check or the probe declares failure before the process exists.
- ⚠ **Blank-frame detection by BYTE SIZE is wrong** - conflates "blank" with "dark". Use variance.
- ⚠ **`rm -rf` on an output dir deletes the LOCKFILE of a run still using it.**
- ⚠ **An unmatched `sed` silently passes text through** - assert the substitution matched.

⚠ **Largest remaining shadow-memory item (NOT a leak, a layout cost):** the sun cascade strip is
one row, so 6 cascades at 2048 make a 12288x2048 rect and the packer needs 12288x4096 for it plus
the locals. Two rows of three would be 4096x4096 for the same pixels, ~130 MB back on the heaviest
demos - but it needs the engine rect layout AND every shader that indexes into it.

Related: [[project-single-session-soak]], [[project-vram-floor]], [[project-measuring-rules]],
[[project-rules-rendering-dx12]], [[project-playgame-crash]], [[project-prealpha-readiness]]
