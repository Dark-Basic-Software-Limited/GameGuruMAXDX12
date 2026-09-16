# DX11 → DX12 Parity Report
**Scope:** all 338 DX11 commits, Nov 2025 → 2026-09-16 (`ca32a143 .. 3e21f674`). 12 audit slices, every MISSING claim adversarially re-verified, plus a whole-subsystem critic pass.

---

## Bottom line

**DX12 is close to parity, but not at it.** Roughly 305 of 338 commits landed in full. **33 commits carry a gap — 34 discrete defects: 5 HIGH user impact, 24 MED, 5 LOW.** Nothing whole-subsystem is missing. No commit was skipped wholesale by accident.

The honest characterisation is worse than the count suggests, though, and it is one specific failure mode:

> **Almost every gap is a HALF-PORT, not an omission.** One side of a feature landed and the other did not. The knob saves and loads and does nothing. The flag is written in four places and read in none. The breadcrumb is recorded and never printed. The cache directory is swept and never populated.

That matters because these are invisible to the compiler, invisible to `sweepgate.sh`, and invisible to a code reader — the file *looks* like the feature is there. Out of the 34 defects, **at least 12 are textbook severed chains** where DX12 contains dead code that reads as done. Two of them (`bAbortRestOfGunModeCode`, `bOnlyOneButtonMouseRelease`) are single orphan variable declarations that would fool any future audit, including a repeat of this one.

**Ease is the good news: 27 of 34 are EASY (a hunk or a handful of lines), 7 are MEDIUM. Nothing is HARD.** This is a day or two of work, not a campaign.

---

## Port order

### Tier 1 — Broken user-facing behaviour, cheap to fix (do these first)

| What the user gets back | Port | Risk |
|---|---|---|
| **Storyboard intro video auto-advance** (`8315c88c`) — today a splash video plays to the last frame and *sits there*; Escape doesn't skip it; the player must click the video. DX12 ported the four writes of `bTriggerVideoNextScreen` and **zero of the four reads** — *and* ported the other half of the commit that makes the game boot straight into that screen. Self-inflicted. | 4 `if` wrappers in `M-GridEditB_part22.cpp` around :1738–:1790 | **Low.** Purely additive conditions; the automation `TITLE_CLICK` block DX12 inserted at :1777 is why the hunk never auto-placed — re-place by hand around it. |
| **Save-game reload restores spawned objects** (`c9cb9074`, hunk 1) — load a save and any entity the player spawned during play comes back invisible and walk-through. DX12 handles only the `health<=0` respawn queue; the `spawnatstart==2 && alive` else-branch is absent. | `M-LUA-Entity.cpp` `entity_lua_spawn` | **Low.** Self-contained else-branch: `phyalways=0; entity_lua_show(); entity_lua_collisionon();` |
| **Entity Sound Slot 4 controllable from Lua** (`f74a6116`) — the editor/file-format half shipped (v342 record), so users *can* set Sound4 now and it *does* persist, but every Lua path returns slot **5**, and any `SetEntityString` call on **any** slot does an unconditional `soundset4 = 0`, orphaning the handle. Slot 4 also leaks on level change and never silences on AI death. | `DarkLUA.cpp` :2413/:2459/:5265/:5348, `M-LUA-Entity.cpp`, `M-DAINew.cpp`, `G-Entity.cpp` | **Low-med.** Six small sites, but the unconditional-zero removal must be exact or you break slots 1–3. |
| **Crash log actually gets written for Lua runtime errors** (`75d4eecf`) — DX12 ported the caller (`CrashHandler(NULL)` from `CError.cpp`) **without the four `if (pExceptionInfo)` guards**. So a Lua runtime error AVs inside the crash handler, re-enters, hits `if (!SymInitialize(...)) return 1;` and produces **no log at all** — for exactly the error class the feature exists to report. | `CrashLogger.cpp` :175, :187, :204, :218–222, :249 | **Low.** Four null guards. This is a bug you shipped, not a missing feature. |
| **Keys looted from Chests unlock doors** (`51197b43`) — DX12 took the reader (`collected==2` accepted) and nothing that *sets* `collected`, plus the whole 74-line `darklua_refreshhaskeystatefor()` is absent (0 hits in the tree). Player loots key, door stays locked. Removing a key never re-locks either. | `DarkLUA.cpp` :8693 + call sites :1444/:1470/:8894/:8901; `M-LUA-Entity.cpp` :1195 | **Med.** New function plus four call sites; anchors all verified present and unmodified, so it drops in clean. |

### Tier 2 — Dead knobs: the UI exists, the setting does nothing

These are the most damaging to trust, because the user *sees* the control.

- **`animchoicemode` GUNSPEC field** (`5514c107`) — all four runtime consumers ported; the **parser never assigns it**, so it is stuck at 0 and modes 1/2 are unreachable. Fix is two `cmpStrConst` lines in `M-Gun_part0.cpp` ~:543. *EASY.*
- **Ultrawide Save/Load double-selection, issue 6283** (`7bf6ff5b`) — `bOnlyOneButtonMouseRelease` declared and written twice, **never read**. One click still registers on every overlapping widget. One `if` at `M-GridEditB_part22.cpp`:1772. *EASY.* (Same function as the video bug above — port them together.)
- **Editor grid below 1.0, issue 6278** (`38a6252f`) — the popup accepts 0.5; the toolbar's second live copy of the widget clamps it back, and `Master::InitializeSecondaries()` resets it on every boot. Six sites in DX11, DX12 took two. Remaining: `DBDLLCore_part0.cpp` :784/:924/:1043–45, `master_part0.cpp`:469. *EASY.*
- **"Show Object Debug Visuals"** (`1ee57a00`) — field exists, exported to Lua, shipped scripts act on it, **the checkbox that is the only writer was never ported**. Cannot be turned on. Same commit: `WickedCall_GetObjectPlaying` exists with zero callers, so `GetObjectAnimationFinished` can hang a script forever on an anim that stops short. *EASY.*
- **`disablejustgrasssystem=1`** (`49b9719d`) — parsed in *two* places, sets the global, **nothing reads it**. Loses you a triage lever (skip 6 shader loads + the grass-map alloc at boot) that would be genuinely useful on the current hang hunt. One guard around `GGGrass_Init()`. *EASY.*
- **WPE preview OffsetX / OffsetZ** (`11b3678b`) — sliders write `fPreviewXOffset`/`fPreviewZOffset`, the runtime honours them in-game, **the editor preview reads only Y**. Preview doesn't match the level. One line at `M-GridEditB_part24.cpp`:1431. *EASY.*
- **`terrainsafegpu=1`** (`76731c3c`) — neither half landed, so there is **no escape hatch** forcing chunk generation + GPU buffer creation onto the main thread. See Tier 3.

### Tier 3 — Crash / hang hardening (cheap, and directly relevant to your live DEVICE_HUNG history)

This cluster deserves to be read as one thing. After 3.35b–h you already know this tree can hang on a terrain/streaming race, and **six separate mitigations DX11 shipped for exactly that class are absent here.**

- **`terrainlock` is still a plain `std::mutex`** (`8c478170`) — DX11 converted three locks to `recursive_mutex` because the optimiser inlining a locking helper into a lock-holding caller self-deadlocks ("freezes everything", PE's words). DX12 took **both Bullet locks and not the terrain one** — the one that matters most here, given `GGGrass_UpdateInstances` takes it twice per instance across 400k instances and DX12 has since added bake/readback/invalidate paths that also take it. One line, `GGTerrain_part0.cpp`:9283. *EASY.* **Port this one today.**
- **300 ms `Sleep(1)` spin held inside `terrainlock`** (`633f216f`) — DX11 moved the LOD wait loop out of the lock and its own commit message names the old shape as the suspected cause of device-lost. DX12 still blocks every other terrain consumer for up to 300 ms in a frame. `GGTerrain_part0.cpp`:9899–9915. *MEDIUM* (lock-scope restructure — measure after).
- **`terrainsafegpu` opt-in** (`76731c3c`) — `AddChunk` still hands `GenerateGPUBuffers` (a D3D resource creation) to a worker thread with no way to disable it. Your own rule set says resource creation belongs on the main thread. DX12's `AddChunk` is byte-identical to DX11's pre-image, so the hunk applies verbatim. *EASY.*
- **`pPage` null guard in `GGTerrain_CheckPageShift`** (`01d27f07`) — `assert(pPage)` compiles out under NDEBUG in Release, so an exhausted SVT free-page list is a null deref in the shipping build. **Your own author already guarded the other three PopItem sites**; this is the one left bare. `GGTerrain_part0.cpp`:8197. *EASY.*
- **Crash log survives a failed `SymInitialize`** (`0471a9a3`) — DX12 does `if (!SymInitialize(...)) return 1;` and writes **nothing**. DX11 retries with `invade=FALSE` and carries on with `bSymbolsAvailable`. Worst possible outcome for a tester report. *MEDIUM.* (Note the DX12 crash logger is a genuine 3.38 rewrite, not a stale copy — the stack-walk half is arguably better than DX11's. It's the failure paths that are missing.)
- **The breadcrumb ring buffer is never printed** (`1aacfb90`) — DX12 carefully records breadcrumbs (`CError.cpp`:385/:435, `HideLimb`/`ShowLimb`) into `g_CrashContext`, and **`CrashLogger.cpp` never reads it**. Every breadcrumb the port installed is thrown away. Two lines. Also missing: the four Lua breadcrumbs naming the script function, and standalone suppression of object/limb warning popups (`g_bDisplayObjectAndLimbWarnings` defined, never read). *EASY, and it makes every future crash report better.*
- **`cStr` +8-byte slack** (`9ae89fac`) — DX12 fixed the one reported heap corruption but not the general hardening (`BUFFERINCREASESIZE 8`) protecting every other `.x → .dbo` extension-swap caller. Near-zero memory cost (`STRMINSIZE` is 128). *EASY, LOW impact, but it's crash-hardening against a bug that already bit.*

### Tier 4 — Editor-vs-game and WYSIWYG breaks

- **Additive blending survives Test Game** (`29a22b33`) — AvengingEagle-style light/glow decals (`decal_animate1_additive.fx`) render additive in the editor and go flat alpha-blended with dark edge boxes the instant you test. `entity_init()` re-applies `SetObjectTransparency` over the blend mode; the re-assert block is absent from `G-Entity_part0.cpp`:163. *EASY, MED.*
- **Bullet holes on doors, issue 6236** (`c1269dd7`) — DX12 added the `iAllowBuletHole` opt-in but **kept the old `isimmobile` clause DX11 removed**, so the paired checkbox (`f460d1be`, ported) is decorative and doors still collect decals that then hang in mid-air. `G-Entity_part2.cpp`:989. *EASY.*
- **Start Marker underwater** (`1d49f004`) — DX12 still teleports the player to waterline+20, making an underwater-opening level impossible. Comment out the clamp at `M-Physics_part0.cpp`:1681. (Audio half of this commit **is** in.) *EASY.*
- **Remote-project thumbnails** (`1ad98209`) — the guard is ported *everywhere except the three sites in `entity_load()`* (`M-Entity_part0.cpp` :842/:932/:948), so remote-project entities show stale/shared thumbs — the Discord issue, unfixed. *EASY.*
- **Ctrl+Z across a level load** (`b807cfd7`) — one of two `undosys_clearall()` sites landed. The missing one is the storyboard/next-level load path, so undo replays events from the *previous* level. One line at `M-GridEdit_part1.cpp`:197. *EASY.*
- **Case-insensitive custom decals** (`71954585`) — 1 of 3 sites ported. Still case-sensitive: the Lua `SetCustomExplosion()` and `weapon_projectileresult_make()`. Wrong-cased name silently falls back to the default explosion. *EASY.*

### Tier 5 — Performance, AI quality, content

- **Nav mesh built at full resolution across the whole map** (`15cbd06c`) — DX12 got the debug-visual half; `GGTerrain_GetTriangleListHighQuality` **does not exist**. AI pathing is high-detail only near wherever the camera happened to be when Build Nav Mesh ran. *MEDIUM.*
- **Standalone nav-mesh vertex cache** (`fd09784f`) — DX12 ships the hunk that **deletes** `navbank\` and neither `saveVertices()` nor `loadVertices()`. The shipped build clears a cache nothing populates, and every standalone level load rebuilds the whole terrain triangle soup. This is the "Make Standalone Loading Faster" commit, inverted. Port with `15cbd06c` — same function, independent changes. *EASY.*
- **No terrain update during standalone export** (`2224088a`) — DX12 still runs `GGTerrain_Update`/`GGTrees_Update`/cull/`GGGrass_Update` while the saver cycles, so exports are slower and do needless GPU work. One `if (bExport_Standalone_Window == false)` around `master_part1.cpp`:747–786. *EASY.*
- **Delayed Shot** (`634ed6a6`) — bow/crossbow support: hold the shot until the release frame. **None of the behaviour is present**, and the port left exactly one orphan line (`bool bAbortRestOfGunModeCode = false;` at `G-Gun_part1.cpp`:1582) that makes the file look done. *MEDIUM.*
- **door.lua is two versions behind** (`5a36a87c` v32 + `3eaf6bba` v33) — DX12 is still v31 because the file was locally modified by your 3.21 logic gating, so Phase A skipped it. Missing: `SOUND_SYNC` (sliding doors play the close sound at the wrong time) and `OPEN_ANIMATION`/`CLOSE_ANIMATION` dropdowns (any door model whose anims aren't literally named open/close is unusable). Merge both onto the DX12 local change together. The `animsetlist` token is already supported by the DX12 property parser. *MEDIUM — the only hand-merge on the list.*
- **"Add New Particle" button is inert** (`76731c3c`, second half) — all the code is ported; **the three content files are not in the build area**: `Files/editors/uiv3/entity_new_particle.png`, `entity_new_particle2.png`, `Files/entitybank/_markers/NewParticles.fpe`. Blank icon, click places a nonexistent .fpe. Needs an asset drop from you, not a code change. *EASY, but blocked on you.*
- **Diagnostics and cosmetics (LOW):** `Loading progress: N%` breadcrumb absent from `loadingpageprogress()` (`0251d4fe`) — you cannot narrow "it hangs on the loading screen" from a tester log; 5 interior `timestampactivity` steps inside `GGTerrain_RemoveAllFlatAreas` (`f32ed7db`) — relevant to the current hang hunt; two stale HUB URLs pointing at retired sub-pages (`a013bd2f`); `wiProfiler::SetEnabled(false)` when TABTAB hides, so the profiler stops costing frames (`c9cb9074` hunk 2).

---

## Deliberately skipped — and should stay skipped

These are **not applicable**, not backlog:

- **PIX `BeginCommandList(QUEUE, "name")`** (`633f216f`) — DX12's engine emits PIX markers itself via `device->EventBegin`; our `BeginCommandList` has no name parameter. Take the terrainlock half only.
- **`01d27f07`'s D3D11 readback plumbing** — `RESTOREDEC2025TERRAINSYSTEM`, `ID3D11Query`, `texReadBackStaging`. DX11-only. Only the `pPage` guard is portable.
- **`SetSpecialGGDebugLog`** (`0471a9a3`) — the engine API does not exist in the DX12 Wicked fork (`D:\max\WickedEngineDX12`). Would need an engine change for marginal value.
- **`ForceCrash_AccessViolation()`** — dev-only test helper, no user value.
- **Front Shadows Priority tooltip** (`0251d4fe`) — the checkbox is deliberately commented out at `M-GridEditB_part24.cpp`:1359 by your 2026-07-28 UI audit because the global has no reader. Unreachable by design.
- **`DX11_PARITY_AUDIT_2026-09-14.md` §6 parks `GGTrees.cpp` and `GGTerrain.cpp`** pending a per-commit read. That parking is still sound for the GPU-race/readback work — but note §6 does **not** cover `cStr.cpp`, `door.lua`, or the terrain `recursive_mutex`, all of which fell out silently rather than by decision.

---

## Confidence and blind spots

**Where I'm confident:** every MISSING claim above was re-verified independently against exact line numbers in both trees, with the anchor context read in full — not grep-and-assume. Several candidate findings died in that pass. The zero-length refuted list is a *result* of that verification, not an absence of it.

**Where this could be wrong:**

1. **Static evidence only.** Nothing here was run. "The checkbox can never be set" is a code-reading conclusion; a runtime path I didn't find could set it. I'd rate the HIGH items as near-certain and a few MEDs as ~85%.
2. **Assets are effectively unaudited.** Neither repo tracks binaries and there is no DX11 build area on this machine, so the `NewParticles` finding came from a `find` over your build area and could not be diffed against a deployed DX11 install. **If DX11 added other art/models/sounds in these 338 commits, this audit would not have seen them.** That is the single largest hole.
3. **Commit-by-commit slicing misses cross-commit features.** The critic pass caught 6 more gaps *after* all 12 slices reported clean — including the highest-impact one (`8315c88c`). That tells you the per-commit method has a real miss rate. My estimate: **1–3 more gaps of this shape remain**, most likely in the storyboard/screen-editor code (three findings already cluster in `M-GridEditB_part22.cpp`, all failing to auto-place for the same reason: DX12 inserted its own automation block into that context) and in `DarkLUA.cpp`.
4. **Parked files not fully read.** `GGTerrain.cpp` and `GGTrees.cpp` were audited opportunistically, not exhaustively. Three findings came out of them anyway; there may be more.
5. **Out of scope entirely:** regressions DX12 introduced on its own, and DX11 changes that *predate* Nov 2025.

**Suggested first move:** the Tier 1 five plus the terrain `recursive_mutex` one-liner. That is six edits, all low-risk, and it clears every HIGH-impact defect plus the hardening most relevant to the freeze you're already chasing.