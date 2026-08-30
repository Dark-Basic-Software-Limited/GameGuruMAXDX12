---
name: project-level-version-debt
description: "RESOLVED 2026-07-18 (commit ed919e44) — .ele v342 (eleprof.soundset4a_s) ported from DX11 source; DX12 reads AND writes v342, verified on the TESTPRO1 baseline. Version tripwire from 72b2de27 stays armed for future version bumps. Kept for the porting recipe + remaining Sound4 runtime/UI gap."
metadata: 
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
---

## RESOLVED 2026-07-18 — v342 ported (commit `ed919e44`)

The v341→v342 delta was ONE field: `eleprof.soundset4a_s` (new Sound4 audio slot string). Ported from the READ-ONLY DX11 reference repo at `D:\max\GameGuruMAX` (`M-Entity.cpp` — load 5581-5588, save 7198-7205, FPE parse 2201-2202, replacement loop 5783-5817). DX12 now reads and WRITES v342 (round-trips both ways). Verified: TESTPRO1 v342 baseline loads "version 342, supported max 342, 2 elements", Brick Pyramid renders.

**Remaining gap (deliberate):** runtime playback of the Sound4 slot and its editor property-grid UI are NOT ported — the field is preserved through load/save/clone/export so nothing is lost; the feature arrives with a future upstream code sync.

**The porting recipe below is retained** — the next DX11 version bump (343+) follows the same steps, and the `72b2de27` tripwire will announce it.

## The original bug (historical)

`c_entity_loadelementsdata` in [M-Entity_part3.cpp](GameGuru Core/GameGuru/Source/M-Entity_part3.cpp) has:

- `t.versionnumbersupported = 341` (line 17)
- Line 35 always sets `g.entityelementlist = iElementsInFile` from the file
- Line 42 only allocates + reads entities if `versionnumberload <= 341`
- Line 904 loop runs regardless, iterating `1..entityelementlist` and dereferencing `t.entityelement[t.e].bankindex`

For files saved with version > 341, `entityelement` is stale (still sized for the previous level) but the loop iterates the new bigger count → access violation at line 906.

## The temporary guard (applied 2026-07-12)

Line 904 now reads:
```cpp
for (t.e = 1; t.failedtoload == 0 && t.e <= g.entityelementlist && t.e <= g.entityelementmax; t.e++)
```

The `failedtoload == 0` term is the key one — skips the loop entirely when the version check failed. The `<= entityelementmax` term is defensive against any other counter-desync.

**Effect:** loading a too-new .ele no longer crashes — but the level's entities silently don't load. Terrain, trees (`pAllTrees[]`), grass, and paint data all still load through their own paths (they don't share the .ele version check). Enough to continue tree-rendering work.

## Root-cause fix (deferred to pre-release)

Before final release the DX12 port has to actually load newer .ele files. Steps:

1. Production DX11 writes `versionnumbersave = 342` — **confirmed on disk** (2026-07-17 review: version int in TESTPRO1's `map.ele` at `C:/Users/leeba/Documents/GameGuruApps/GameGuruMAX/Files/mapbank/island - Copy/map.ele` = 342). Diff what fields 342 adds over 341 in DX11's save code and extend the DX12 reader.
2. Bump `t.versionnumbersupported` in [M-Entity_part3.cpp:17](GameGuru Core/GameGuru/Source/M-Entity_part3.cpp:17) to match.
3. Look at DX11's `entity_saveelementsdata` — every version above 341 that adds a new field needs a matching read in DX12's `c_entity_loadelementsdata` load loop (starts at line 88), otherwise field offsets go out of sync mid-record and the entities load with corrupted data.
4. Also match `t.versionnumbersave = 341` in [M-Entity_part4.cpp:15](GameGuru Core/GameGuru/Source/M-Entity_part4.cpp:15) — DX12 must write the new version too, or a round-trip corrupts the save.
5. Similar version-compat audits are probably lurking in `map.ent`, `header.dat`, `map.way`, etc. — GG's other per-level files. Grep for `versionnumbersupported` across all `M-*.cpp` files.

## How to apply

When we hit final visual-parity testing and the temporary guard's "entities silently missing" behaviour blocks progress on levels that need entity props (spawn points, doors, etc.) — that's the trigger to do the root-cause fix. Until then, tree/grass/terrain work can proceed on the guarded build.

**Escalation RESOLVED (2026-07-17, commit `72b2de27`):** the drop is now LOUD — every load logs `entity elements file: <path> (version N, supported max M, K elements)` to Guru-MapEditor.log via timestampactivity; a mismatch logs `ENTITY LOAD SKIPPED` and pops a MessageBox (suppressed while `g_bAutomationActive` so the harness never hangs). **Verified same day: TESTPRO1 island loads map.ele v341 with 1 element — baseline currently CLEAN.** The tripwire fires the moment a DX11 re-save bumps it to 342; that's the trigger for the root-cause port. Grep pattern for A/B sessions: `grep "entity elements file\|ENTITY LOAD SKIPPED" Guru-MapEditor.log`.
