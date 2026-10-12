# Tester triage — pre-alpha, external testers (opened 2026-09-24)

One place to log what the testers send, recognise what is already known, and know where to look
first. Every fix made during the test period follows the usual cycle: its own notes section in
`NIGHT_INVESTIGATIONS_2026-08-12.md`, a `CHECKLIST_CHRONOLOGICAL.md` row, one commit, push — and the
report row below gets the commit hash.

## 1. What the testers have

| | |
|---|---|
| Game code | `7e4fe3b0` (3.99b) — every later commit is notes/tools only |
| Engine | `680a30b0` (3.97 frame-constant hook + 3.98 float skinned normals) |
| `GameGuruMAX.exe` | 31,975,424 bytes · built 2026-09-24 19:20:38 · md5 `90b817e3a5acab37f47a2ea62fd79579` |
| `GameGuruMAX.pdb` | 148,779,008 bytes · md5 `4ddece2d6273497ffcc309ac0004a561` — ships, so crash logs symbolise |
| `shaders/skinningCS.cso` | **9,264 bytes** · md5 `56ab544dd32d34dbe08d3d4d0381658f` — the 3.98 float version. The old one is 9,772 bytes; if a tester reports black faces inside boxes / on flat props, check this file FIRST |
| Diagnostics armed | `dred.txt` present (unless Lee removed it at packaging) — a device removal writes `dred_report.txt` |

★ If a report's exe md5 differs, it is not this build — ask before investigating.

## 2. Ask every reporter for

1. What they did, step by step, and which level / object (demo name, or their own).
2. A screenshot (or short video) of the problem.
3. GPU model and VRAM, Windows version, screen resolution / scaling.
4. After a crash or hang: `Guru-Crash.log`, `crashdump.dmp`, `dred_report.txt` from the Max folder.
5. Graphics preset and any Graphics and Performance switches they changed.

⚠ All baselines here are from ONE machine (RX 9060 XT, GPU-idle at ~60%). A tester's FPS number is
a new data point, not a regression, until it is compared on the same hardware.

## 3. Known issues — recognise these, do not re-investigate

| issue | status | ref |
|---|---|---|
| PLAY GAME (standalone) holds 950–1520 MB MORE VRAM than Test Game on the same level | open, cause not found | 09-20 §3.6, [[project-standalone-vram]] |
| PLAY GAME standalone and Export Game only ever hand-tested | least-tested paths — weight reports here highest | 3.101 |
| Operation Amazon ships baked at the MED terrain preset | Lee's decision pending | 3.101 |
| Editing a shipped demo saves to Documents, not over the demo; reopening the demo gives the original | by design — tester note | 3.101 |
| Water reflection default 1024 + blur 2 unverified on a low-end AMD card | open — first suspect for low-end FPS reports | 3.91 |
| Library preview lighting-stage defaults are provisional | open | 3.98 |
| "Lower Animation" panel checkbox `bEnableAnimationCulling` controls nothing | open | 3.94b |
| A zombie that "cannot find the nav mesh" is standing on a non-walkable spot — generation works | explained | 3.100 |
| Z Island's poly count wanders by ~4000 between runs | by nature, not a regression | 3.95 |
| Soak (many levels in one session) VRAM climbs ~90 MB per level load | accumulation, advisory | 3.66 |

## 4. Symptom → where to look first

| symptom | first suspect | first check |
|---|---|---|
| Black faces on props / characters (esp. flat faces, box interiors) | 3.98 skinning — static DBO props are SKINNED (`SKINDUMMY`) | `skinningCS.cso` size (§1); harness `SET_TANGENTVIS 2` — exact black = NaN normal |
| Library hover preview wrong: white-out, dark, level colours leaking in | `master_part2.cpp` GGObjectPreview (3.97/3.98) | `DUMP_OBJPREVIEW` → `studio=1`, `key=`, `env=`; `SET_OBJPREVIEW_LIGHT 0 4 45 0` turns isolation off for A/B |
| Full-size preview background black / cut in half | 3.99 backdrop state | `DUMP_OBJPREVIEW` → `backdropFixes=` should rise once per open / combo change |
| Level looks lower quality after a Test Game, or after saving | 3.96 terrain snapshot/restore | read the level's `ggterrain.dat` fields (notes 3.86/3.96 list the LOW/MED signatures) |
| Post-processing lost when entering Test Game | 3.87 call-site restore (`M-GridEdit_part2.cpp`) | compare `DUMP_POSTFX` in editor vs game |
| Water reflection shimmer / blur / FPS cost | 3.89–3.91 Water Reflection Size / Blur | ask them to try Size = Auto, Blur = 0 |
| Distant characters frozen, jittering, head/feet out of place | Reduction Scale 3.92/3.93 | `DUMP_ANIMREDUCTION`; ask them to untick Lower Animation |
| AI idle / "no nav mesh" | 3.100 | `RUN_LUA return RDIsWithinMesh(x,y,z)` at the character's position |
| Test Game "hang" at 0% CPU | a modal (Lua MessageBox) owns the message pump | [[project-testgame-freeze]] — look for a dialog before a lock |
| Crash / device removed | read the logs before theorising | `Guru-Crash.log` names file + line; `dred_report.txt` for GPU faults |
| Out of VRAM on a 4 GB card | standalone gap (§3), or level size | `DUMP_VRAM` named census; ask whether it was PLAY GAME or Test Game |
| Second level loads blank or hangs | billboard atlas per-level rebuild (solved 09-18) | confirm build md5 first |
| Trees / grass missing or popping | billboard handover, tree pool cap | [[project-far-tree-billboards]] |
| Big FPS drop on a weak GPU | reflection default (§3) first, then the Brutal off-switches | ask for Graphics and Performance settings |
| A thumbnail / image that should come from a capture never appears, or vanishes on reopen | DX12 image layer: relative load of a writables file fails, `SaveImage` writes nothing (3.103) | is the file on disk in `Documents/GameGuruApps/GameGuruMAX/Files/...`? then it is the load, not the capture |
| A character splays into huge sheets for a frame or few when placed or spawned; distant characters flick between two poses | 3.107 skinning ping-pong under Reduction Scale (fixed after the tester build) | `SET_SKINFRESHGUARD` -> `drawnGarbage` and `drawnStale` must stay 0; ask them to untick Lower Animation |
| A tree pops lighter (its self-shadow vanishes) as the camera backs away | 3.108 - fixed after the tester build; if seen on a dev build, the slider is below the reach (it still switches there) | `SET_SHADOWDISTFADE` -> `fadingCasters` > 0 near trees; slider at "Max"; `DUMP_POSTFX` -> `treelod` |
| Particle Library hover shows nothing, or the Object Library hover preview stopped working after the Particle Library was opened | 3.110 (fixed after the tester build): the BackBufferParticlesMode latch; the particle preview pass | `DUMP_OBJPREVIEW` -> `bbParticlesMode=` must be 0 when nothing is hovered; while hovering `particle=1`, `particleFrames=` rising, `wpeSim=` > 0 for a .pe |
| Terrain torn into white sheets / spikes, trees floating, after clicking palette trees | 3.102 billboard atlas slice overrun (fixed after the tester build) | `SET_FARTREES` (no argument = read-only) -> `ATLAS (3.102): sliceCopiesRefused=` must stay 0 |

## 5. Reproducing — the tool belt

- `tools/harness/lib.sh` — `send`, `wait_state`, `shot`, `click` (CR-safe; screenshots go to
  `$GGMAX_OUT`, never the build folder).
- `tools/harness/open_demo.sh "Switch Escape" testgame` — shipped demo → editor → Test Game.
- `tools/harness/open_project_level.sh TESTPRO2 testpro2 library` — My Games project → level →
  Object Library open.
- In game: `RUN_LUA return <expr>` reads live Lua state; `PRESS_ESCAPE` leaves Test Game.
- `TYPE_TEXT <text>` types into the focused ImGui text field (3.104) - e.g. naming a new level.
- `tools/harness/place_burst.sh <pickX> <pickY> <placeX> <placeY> <frames> <tag>` - place an object while
  capturing consecutive frames (3.107); `BURST_FRAMES` alone for any other one-frame glitch.
- `tools/harness/tree_shadow_sweep.sh` / `tree_shadow_sweep_game.sh` - dolly the editor camera / teleport the
  player straight back from a tree and log its crown brightness per step (3.108). In Test Game
  `SetFreezePosition(x,y,z)` + `TransportToFreezePosition()` via `RUN_LUA` moves the player - put it 60 u
  above `GetTerrainHeight`, or it falls through.
- `tools/harness/particle_preview_probe.sh <tag> x0 y0 x1 y1 [neighbour rect]` - with the Particle Library
  open, hover a tile and require five spaced shots to differ (3.110). Works for any live hover preview.
- Debug views (`SET_TANGENTVIS n`, 0 = off): 2 vertex normal · 3 bumped normal · 1 world tangent ·
  4 handedness · 16 world-position grid · 23 albedo. Exact black in 2/3 = a NaN normal.
- `FIND_OBJECT x y z r` lists scene objects near a point; `SET_CAMERA x y z ax ay` moves the editor
  camera; `UNDO` reverts a placement. `PRESS_KEY` cannot type text into ImGui fields.
- ⚠ Never save Lee's projects (TESTPRO2 excepted) and never touch the DX11 repos.

## 6. Report log

| # | date | tester | GPU | build md5 ok? | area | symptom (short) | known? (§3) | status | notes § / commit |
|---|---|---|---|---|---|---|---|---|---|
| 1 | 2026-10-10 | via Lee | - | reproduced by Lee on the dev build | Terrain > Trees palette | picking the last trees tears the terrain into white sheets | no | **fixed, Lee-confirmed** (also: selection wipe, 64-bit bitfield load, atlas rebuild for new types) | 3.102 / `e7b11178` |
| 2 | 2026-10-10 | Lee | - | dev build | Storyboard > level node > Take Screenshot | snapshot taken but the level node thumbnail never changes | no | **fixed, Lee-confirmed** (also: UI in the capture, thumb lost on reopen, map PNG) | 3.103 / `96b52e3a` |
| 3 | 2026-10-10 | Lee | - | dev build | RPG HUD on-screen map + Save New Level | the HUD map shows nothing after a Map Snapshot; a generated level gets no thumbnail | no | **fixed** (DX12 image loader now uses the writables redirect) | 3.104 / `e715e5c3` (+3.105b guards) |
| 4 | 2026-10-11 | Lee | RX 9060 XT | dev build | Level editor > placing a character (Christmas Zombie, TESTPRO2 > testpro2) | for ONE frame as the character is put down its polygons splay out, then it looks fine | no | **fixed, Lee-confirmed** (also: distant characters drew an old pose whenever an object was on the cursor) | 3.107 / `c8e5a62b` (engine `c1f54bd9`) |
| 5 | 2026-10-11 | Lee | RX 9060 XT | dev build | Test Game > tree shadows (TESTPRO2 > testpro2snow) | backing the camera away from a tree, its shadowing changes and the whole tree pops lighter; tied to Tree Shadow LOD Distance (7000 shows it, 750 moves it closer) | no | **fixed, Lee-confirmed** (options 1 + 3, then 2 as 3.108b - any slider value fades; slider 750..2000, default 2000 as 3.108d: the mesh shadow dissolves with the mesh; honest slider stopping at the preset's reach, "Max") | 3.108 / `81aaba31` + `3bbddd5e` (engine `d1e972a6`) + `81320752` (engine `090ad5c7`) + `013e30d4` (3.108d) |
| 6 | 2026-10-12 | Lee | RX 9060 XT | dev build | Particle Library (Object Tools > Particles > New Particle) | hovering a particle thumbnail does nothing; DX11 played the effect in it | no | **fixed** (also: opening the Particle Library latched a flag that froze its hover code and killed the Object Library live preview for the session) | 3.110 / `3ca93575` |
| | | | | | | | | | |
