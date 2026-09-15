---
name: project-porting-clusters
description: "Porting a change between the DX11 and DX12 trees - why excluding a file does not exclude its cluster, and the checks that actually catch a half-port"
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-15T13:28:22.592Z
---

# ★★★ Excluding a file does NOT exclude the cluster

Learned the hard way on 2026-09-15 porting DX11 → DX12 (3.38 phases E–G).

`animsystem_weaponproperty` renames argument 2 from `readonly` to
`bFromCharacterCreator`. **Same arity.** I deliberately left `M-Importer.cpp` (the definition) out of a
batch for exactly that reason — and shipped the bug anyway, because its three CALLERS live in
`M-GridEdit`, `M-GridEditB` and `M-CharacterCreatorPlus`, which the batch DID include.

Result: every call site passed the new meaning while the definition still used the old one. And it
was not inert — the definition fed argument 2 straight into the UI's readonly slot, so a read-only
weapon dropdown became editable. **It compiled cleanly. No build gate can catch this.**

★★★ **The half that lands is the half that compiles.** A deferral is only real once you check what
the files you DID take reference. Before excluding anything, grep the batch for its symbols.

## The three checks, in the order they earn their keep

1. **Same-arity signature changes** — the only class a build cannot catch. Scan the unported diffs
   for `^[+-](void|int|bool|...)` lines containing `(`. An arity change is safe (compile error); a
   *rename* is not.
2. **Undefined symbols** — cheap static scan of added lines for `extern` decls and struct members.
   ⚠ Two blind spots, both paid for: it misses DOUBLY-defined symbols (a "does this resolve?" check
   cannot ask "twice?"), and it misses bare uses of an undeclared global, because it only follows
   explicit `extern` lines. Same shape as the `fopen` sweep in [[project-cwd-and-file-paths]] —
   **a pattern-based check reports only on what its pattern can see.**
3. **Dead knobs** — a setting whose *effect* lives in an unported file. DX11's `terrainsafegpu` ini
   knob placed cleanly in `Common.cpp` while its only consumer sits in the unported `GGTerrain.cpp`.
   Shipping it would have been a switch that saves, loads and does nothing — the 3.19 Texture Detail
   trap again. Removed the knob rather than ship it inert.

## Mechanics that matter

- ★★ **`patch -F0`, never default fuzz.** At default fuzz, hunks "succeeded with fuzz 3" against a
  part file that did not contain the target function at all — code dropped somewhere unrelated in a
  25,000-line file, still compiling. **A hunk that applies is not a hunk that belongs**: check the
  landing site is inside the right function.
- ⚠ **A partial patch is more dangerous than a rejected one.** `M-LUA` applied 5 of 7 hunks and both
  failures were the ones that mattered (the global and the implementation), because they anchored on
  code our 3.21 rework replaced. The button landed; its implementation did not.
- ⚠ **Name patch files by stem AND extension.** `wickedcalls.cpp` and `wickedcalls.h` both mapped to
  `wickedcalls.patch` and the second overwrote the first. `GGTerrain` collides the same way.
- ★ **Serialisation: check whether the record GREW or a filler was CONSUMED.** DX11 added
  `iAllowBuletHole` by swapping a reserved `WriteFloat(0.0f)` for `WriteLong(...)` — same 4 bytes, so
  offsets are unchanged and old levels still load. Take the same SLOT, and verify save and load have
  identical slot counts and types. See [[project-level-version-debt]].
- ⚠ A failed LINK deletes the exe. A failed COMPILE does not. If the build folder loses
  `GameGuruMAX.exe`, that is why — and the alpha pair is archived in `Max - 300826.zip`.
