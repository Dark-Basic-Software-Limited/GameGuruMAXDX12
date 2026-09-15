# DX11 → DX12 parity audit, 2026-09-14

What the DX11 tree (`D:\max\GameGuruMAX`, HEAD `3e21f674` "Particle System Upgrade", 2026-08-13)
has that this DX12 tree does not, how much of it can be ported mechanically, and what needs a
decision. Produced overnight at Lee's request; **read §6 first if you only read one section.**

---

## 1. The shape of the gap

The two repos **share history** — the DX12 repo already has the DX11 remote as `upstream`, and
DX11's commits are reachable here. So this is a real diff, not a guess.

| | |
|---|---|
| fork point (merge base) | `ca32a143` — *Update M-Entity.cpp*, 2025-11-21 |
| DX11 HEAD | `3e21f674` — *Particle System Upgrade*, 2026-08-13 |
| DX11 commits not in DX12 | **338** (~9 months) |
| files DX11 changed since the fork | 244 |
| — of those, DX12 never touched | **194** (safe to take) |
| — of those, DX12 *also* changed | **50** (need triage) |

⚠ **The DX12 tree is SPLIT and DX11's is not.** `M-GridEditB.cpp` is a 26-line stub here (the real
code is `_part0…_part24`), `M-Entity.cpp` is 7 lines, `master.cpp` is 3. **No DX11 hunk in those
files can be applied by path** — each has to be placed by content into the right part file.

## 2. Method — how "portable" was measured

For every conflicting file: extract DX11's change since the fork, then test-apply it against each
candidate DX12 part file with **`patch -F0`, exact context, zero fuzz.**

★ Zero fuzz is deliberate. A first pass with the default fuzz reported hunks "succeeded with fuzz 3"
in a file that does not contain the function at all — a loose match that would have dropped code
into the wrong place in a 25,000-line file and still compiled. **A hunk that applies is not a hunk
that belongs.** Every number below is an exact-context match, and even those still need a semantic
read before they are trusted.

## 3. Portability matrix

"place" = hunks that land on exact context. Remainder = DX12 has diverged there and needs hand-work.

| file | hunks | place | % | where they land |
|---|---:|---:|---:|---|
| DarkLUA.cpp | 351 | 88 | 25% | parts 0-6 |
| M-GridEditB.cpp | 106 | 92 | 86% | 19 different part files |
| M-GridEdit.cpp | 33 | 30 | 90% | parts 0,1,4,6,8 |
| M-Entity.cpp | 29 | 16 | 55% | parts 1-5 |
| GGTerrain.cpp | 21 | 9 | 42% | part0 |
| Types.h | 14 | 7 | 50% | — |
| master.cpp | 13 | 4 | 30% | part0 |
| G-Gun.cpp | 13 | 10 | 76% | parts 1,3 |
| M-Game.cpp | 12 | 8 | 66% | parts 1,2 |
| G-Entity.cpp | 11 | 9 | 81% | parts 1,2,3 |
| imgui_gg_dx11.cpp | 9 | 8 | 88% | parts 3,4,5 |
| M-MapFile.cpp / Common.cpp | 8 / 8 | 6 / 6 | 75% | |
| M-LUA.cpp | 7 | 5 | 71% | |
| **M-Visuals.cpp** | 6 | 6 | **100%** | parts 0,1 |
| **M-CharacterCreatorPlus.cpp** | 6 | 6 | **100%** | parts 1,2 |
| M-LUA-Entity.cpp | 6 | 3 | 50% | |
| CrashLogger.cpp | 6 | 3 | 50% | |
| cStr.cpp | 6 | 0 | 0% | |
| DBDLLCore.cpp | 5 | 0 | 0% | |
| **wickedcalls.cpp** | 4 | 4 | **100%** | part0 |
| **M-Decal.cpp** | 4 | 4 | **100%** | |
| M-Titles.cpp | 4 | 3 | 75% | |
| CInputC.cpp | 4 | 1 | 25% | |
| **M-TerrainNew / M-Importer** | 3 / 3 | 3 / 3 | **100%** | |
| **CObjectsC / DBOBlock / BulletPhysics** | 2/1/1 | all | **100%** | |
| GGTrees.cpp | 2 | 0 | 0% | |
| Sample_TileMesh / GameGuruMain | 2 / 2 | 0 | 0% | |
| 6 one-hunk files | 1 each | 0 | 0% | |

★ The two lowest scores are the reassuring ones: **`GGTrees.cpp` 0/2 and `GGTerrain.cpp` 9/21** are
exactly where DX12 diverged hardest (terrain bake, far-tree billboards, reduction scale). Low
placement there is the analysis working, not failing — those must not be taken wholesale.

## 4. Dependency clusters — things that cannot be ported alone

Several changes look like single files and are not:

1. **Crash-log cluster** — `CError.cpp` (+376, human-readable runtime errors with Lua file:line)
   needs `GetCrashHandlerDebugLogRef()` from `CrashLogger.cpp`, declared in `CrashLogger.h`, called
   from `main.cpp`. `CObjectsC.cpp` additionally needs the `GG_CRASH_CONTEXT` macro, which DX11
   defines in `wickedcalls.h`. **Five files, one feature.**
2. **Warning-rename cluster** — `g_bDisplayWarnings` → `g_bDisplayObjectAndLimbWarnings`, defined in
   `wickedcalls.cpp`, set in `Common.cpp`, read in `CommonC.cpp` and `CObjectsC_part4.cpp`
   (17 references across 3 DX12 files). Cosmetic; all-or-nothing.
3. **`animsystem_weaponproperty` semantic change** — DX11 renames argument 2 from `readonly` to
   `bFromCharacterCreator`. ⚠ **Same arity**, so the four DX12 call sites still COMPILE while
   passing the wrong meaning. `M-Importer_part1.cpp` + `M-CharacterCreatorPlus_part2.cpp` +
   `M-GridEditB_part12.cpp` + `M-GridEdit_part1.cpp` must change together.
4. **Nav-mesh focus view** — `DetourDebugDraw.cpp` uses globals defined in `Sample_TileMesh.cpp`.
   *(Done — see §5.)*

## 5. What was DONE

Every phase gated on `grep -cE "error C[0-9]+|fatal error|: error LNK"` == 0, never on exit code.

### Phase A — `d1a9d09e` (committed, pushed, builds clean)

- **177 content files**: 96 Lua behaviours, 78 behaviour icons, the Behaviors guide, the MaxLua and
  Groups `.tps`, `document.fpe`, two `.byc`. Also mirrors DX11's deletion of `border_maps.*`
  (checked first — referenced by nothing but itself).
- **The 4 missing Lua commands**, which were the gate on all of the above. Comparing the registered
  C++ Lua API of both trees: DX11 has exactly four DX12 lacks — `GetSunColorRed/Green/Blue`,
  `ForceMouseXYClick` — and DX12 has **none** DX11 lacks. Any shipped behaviour calling those would
  simply have failed here. (Registered against `lua2` in this tree, not `lua`.)
- **8 safe code files**: recursive_mutex deadlock fix (Bullet ×2), multi-monitor UI placement,
  workshop skip-list, `undosys_clearall`, linear audio volume, `gdisablefulldecaleffects`,
  ImGui constants.
- **Nav-mesh focus view**, hand-ported. ⚠ I first took DX11's whole `Sample_TileMesh.cpp` and it
  CLOBBERED DX12's navmesh cache (`loadNavMeshFromFile`, −30 lines). Reverted; moved only the five
  globals and the `handleRender` hunk. **A file being in the "DX11 changed it" set does not make it
  safe — it has to be in the DX11-only set, and that one was not.**
- Content also deployed into the build folder (176 files, manifest kept outside it) so it is
  testable; repo and build verified byte-identical afterwards.

### Phase B — Particle System Upgrade (DX11 `3e21f674`, the newest DX11 commit)

The editor-side particle work: a **Particle States** panel (looping, animation speed, opacity),
WPE emitter preview in the library, and `.pe` effects using their own shipped `.jpg` thumbnail
instead of a generated one.

19 hunks against `M-GridEditB.cpp`, which does not exist here as a compiled file:

| | |
|---|---|
| auto-placed, exact context | **15** — 7 into `_part9` (`process_entity_library_v2`), 8 into `_part12` (`DisplayFPEBehavior`) |
| hand-placed | **3** — #3 and #10 (`.pe` JPG thumbs), #19 (preview-emitter guard, into `_part24`) |
| skipped | **1** — #9 adds a blank line |

★ All three hand-placed hunks failed to auto-apply for the *same* reason: DX11 wraps
`GG_SetWritablesToRoot` in a `Storyboard.gamename`/`customprojectfolder` test that this tree does
not have — a separate, still-unported DX11 change. Only the particle intent was taken; DX12's
existing line was left alone.

★ Hunk #19 wraps the whole of `RenderPreviewEmitter` in `if (bExternal_Entities_Window == false)`.
Ported as an **early return** instead — verified there is no code after that block, so it is exactly
equivalent, and re-indenting 30 lines is risk with no behavioural gain.

## 6. ★★★ What still needs YOUR decision

*(filled in at the end)*
