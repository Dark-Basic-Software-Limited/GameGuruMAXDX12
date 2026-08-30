---
name: project-playgame-crash
description: "PLAY GAME (standalone) crash campaign 2026-08-05 — SVT material_index OOB device-hang, savegames Lua fix, standalone harness commands"
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-05T21:05:30.483Z
---

# PLAY GAME crash campaign (2026-08-05 evening)

**Repo `GameGuru Core/PLAYGAME_CRASH_2026-08-05.md` is THE report.** User's triple crash on
Aztec Game Kit Teaser PLAY GAME→START: (1) DEVICE_HUNG during LOADING LEVEL, (2) silent
relaunch crash, (3) LUA ERROR gamedata.lua:288.

## Root causes (all found same evening)
1. **DEVICE_HUNG = SVT tile-render material_index OOB** (wiTerrain.cpp "Render Tile
   Regions"): `scene->materials.GetIndex(entity)` = SIZE_MAX for dead entity → 0xFFFFFFFF →
   CS reads ShaderMaterial out of bounds → garbage descriptor → deterministic page fault.
   Feeder bug: `SetupWickedTerrainMaterials` never truncated `materialEntities` on a set
   swap — stale tail slots died with the old level while chunk blendmaps still spanned
   those layers. 6/6 soak repro (~45 s into idle play, fault when VT residency reached the
   dead layer); "mat18 freed" in DRED = the corpse at the faulted VA, not the actor.
2. **Silent crash** = `wi::platform::Exit()` is PostQuitMessage and RETURNS — GraphicsDevice
   ctor failure (driver mid-TDR-recovery) fell through to null `SetName` (24 such sites).
3. **Lua save crash** = `savegames/` folder existed NOWHERE; io.open nil → io.close(nil).

## Fixes (game `00b60ee0`+follow-ups, engine 2.03 `104a585d` + 2.04)
- Engine 2.03: ctor fail-clean (gg_fatal_device_init), block_allocate → OnDeviceRemoved +
  clean exit, DRED context loop fix, raw markers when armed (NOTE: neither ANSI nor
  UNICODE embedded markers produce DRED context strings on this NVIDIA driver — identify
  hung passes by OP SHAPE instead; shapes documented in the report).
- Engine 2.04: bounds-clamp both material_index holes + one-shot backlog culprit line.
- Game: WaitForGPU + materialEntities truncation in SetupWickedTerrainMaterials;
  savegames dir created in game_masterroot_initcode; gamedata.save nil guard (fix BOTH
  `Scripts/titlesbank/gamedata.lua` = deployed truth AND the stale `GameGuru
  Core/GameGuru/titlesbank` copy; build area deployed by hand).

## Standalone harness (WETEST.md section)
- Hub `CLICK play_game` → exe RELAUNCHES as `project=2` standalone (identity Guru-Game,
  CWD Files) — harness goes silent ~15 s then answers from the NEW process.
- `TITLE_CLICK start` fires the storyboard START widget through the real click path.
- `GET_STATE` → standalone_title / standalone_loading / standalone_playing + raw flags.
- Soak: scratchpad `playgame_soak.sh` (traps: dred growth, crash-log growth, process
  death). ⚠ `RUN_LUA` HANGS the harness in standalone mode — editor/test-game only.
- ⚠ In standalone play, the game can exit BY DESIGN (idle player killed → flow end →
  relaunches editor): "process died" without dred/crash growth ≠ crash.

## ★ CLOSED: faulted resource NAMED by keep-alive bisection (2026-08-05 late)
**Terrain material DDS set (`Files/terraintextures/matNN/*.dds`), freed at the
level-load material swap; reader = SVT "Render Tile Regions" CS (sole consumer).**
Rounds: A depth-pin → persists (depth exonerated; DRED freed-match names are heap
corpses that rotate); B leakall.txt → CLEAN; C leakterraintex.txt ONLY → CLEAN.
~85%/cycle → 0/9. Silent no-log deaths were the same fault presenting differently
(vanished under the pins). 2.04 clamp never fired (indices valid — staleness is a
GPU-side ShaderMaterial texture descriptor); WaitForGPU at swap did NOT help (persistent
data, survives a full drain). **SHIP FIX: double-buffered retention of the outgoing
texture set in SetupWickedTerrainMaterials (released at next swap).**
- Engine 2.05 keeps the three hunt knobs flag-gated default-OFF (dred + leakall.txt +
  leakterraintex.txt + depth keep-alive under DRED).
- Residual root-root (non-blocking, unreachable with retention): WHICH ShaderMaterial
  entry holds the dead descriptor — suspect the terrain generator's deep-copied material
  snapshot (Generation_Restart) or an orphaned entity's buffer slot.
- ⚠ Booting <25 s after a TDR can crash INSIDE D3D12CreateDevice (driver-side AV, no app
  guard possible) — soak sleeps 25 s after any trapped cycle.
- wilog_messagebox has no automation suppression (modal boxes block the game).
