---
name: project-alpha-packaging
description: What the MAX build folder must and must not contain for the alpha - including two files that look like debris and are not
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-30T22:22:13.884Z
---

The 2026-08-29 cleanup removed **1,885 files / 3.38 GB** of genuine debris from
`D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max` (harness screenshots, a nested `Files/Files/`
tree, loose diagnostic dumps, rolling `Guru-*.log`, `auto_log.txt`, `alloc_tripwire.txt`,
`GameGuruMAX.map`, Optick). Manifest of every removed file was kept outside the build dir.

**Why:** ⚠ Two of the biggest remaining files look exactly like debris and MUST NOT be deleted.
A future session tidying the folder will reach for both. Do not.

## KEEP — `ffmpeg.exe` (76 MB)

Two live EDITOR features shell out to it, and deleting it makes both silently do nothing:

- project PNG icon → `.ico` for standalone export — `M-GridEditB_part19.cpp:1446`
- the **Import Music And Sound** button — `M-GridEditB_part9.cpp:5322`

Both build the path from `g_pAbsPathToConverter` (`Common_part0.cpp:233`) and string-replace
`\Guru-Converter.exe` → `\ffmpeg.exe`. ★ `Guru-Converter.exe` does not exist anywhere and never
runs; the variable survives purely as a root-folder anchor. Verified: a standalone exported game
does NOT need ffmpeg — `mapfile_savestandalone_stage4` never copies it.

## KEEP — `GameGuruMAX.pdb` (148 MB), and it SHIPS with the alpha

Lee's call, 2026-08-29. `CrashLogger.cpp:148` `SymInitialize(process, NULL, TRUE)` finds the pdb
**beside the exe** — it is the only source of the function names and `file:line` in
`Guru-Crash.log`. ★★★ **A rebuild produces a pdb that no longer matches a shipped exe**, so
deleting it makes every tester crash address on this alpha permanently undecodable. Shipping it
means testers' logs arrive already symbolised. Verified live via `DUMP_SCENEUPDATE`, which
symbolises through the same dbghelp path.

## Still optional, still there

`Files/tutorialbank` (2.4 GB of tutorial .mp4) and `Guides` (578 MB) — ~3 GB of learning content
internal testers plausibly do not need. Dropping both is the remaining lever; everything else in the
root is shipping runtime.

**How to apply:** exclude from the PACKAGE, do not delete from the build folder — a deleted asset
costs a rebuild or a re-download, and the pdb cannot be regenerated to match.

See [[project-next-action-immediate]] for current state and
`NIGHT_INVESTIGATIONS_2026-08-12.md` §3.35h–j for the audit and the CWD finding behind it.
