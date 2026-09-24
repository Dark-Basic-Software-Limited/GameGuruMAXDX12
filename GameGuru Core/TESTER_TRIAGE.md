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

## 5. Reproducing — the tool belt

- `tools/harness/lib.sh` — `send`, `wait_state`, `shot`, `click` (CR-safe; screenshots go to
  `$GGMAX_OUT`, never the build folder).
- `tools/harness/open_demo.sh "Switch Escape" testgame` — shipped demo → editor → Test Game.
- `tools/harness/open_project_level.sh TESTPRO2 testpro2level library` — My Games project → level →
  Object Library open.
- In game: `RUN_LUA return <expr>` reads live Lua state; `PRESS_ESCAPE` leaves Test Game.
- Debug views (`SET_TANGENTVIS n`, 0 = off): 2 vertex normal · 3 bumped normal · 1 world tangent ·
  4 handedness · 16 world-position grid · 23 albedo. Exact black in 2/3 = a NaN normal.
- `FIND_OBJECT x y z r` lists scene objects near a point; `SET_CAMERA x y z ax ay` moves the editor
  camera; `UNDO` reverts a placement. `PRESS_KEY` cannot type text into ImGui fields.
- ⚠ Never save Lee's projects (TESTPRO2 excepted) and never touch the DX11 repos.

## 6. Report log

| # | date | tester | GPU | build md5 ok? | area | symptom (short) | known? (§3) | status | notes § / commit |
|---|---|---|---|---|---|---|---|---|---|
| | | | | | | | | | |
