---
name: project-machine-migration
description: "2026-08-31 new machine — VS 2022 path, BUILDS junction, and the untracked .lib set that breaks the build on any migration"
metadata: 
  node_type: memory
  type: project
  originSessionId: 509f3c47-3d86-4b9b-a337-23ada2c00769
  modified: 2026-08-30T23:42:25.047Z
---

# Machine migration 2026-08-31 — what broke and where the pieces are

Lee moved to a new box ("i have updagted"). Repos and the versioned memory survived; **everything
untracked did not**. First fresh build took SIX link failures to get green. All fixed same session.

## The environment deltas (this machine)
- **Visual Studio**: `C:\Program Files\Microsoft Visual Studio\2022\Community` — the old
  `...\18\Community` (VS 2026) is GONE. `build.bat`, `build_wicked.bat` fixed and committed.
- **Build area**: the vcxproj now outputs to `D:\DEV\BUILDS\GameGuru Wicked MAX Build Area\Max`
  (BUILDS, plural). ★ A **junction `D:\DEV\BUILD` → `D:\DEV\BUILDS`** (`mklink /J`) keeps every
  tools script, memory doc and WETEST path valid — do NOT mass-edit them. Undo: `rmdir D:\DEV\BUILD`.
- The old original repo lives at **`D:\DEV\GAMEGURUMAXREPO`** — it is the donor for every
  untracked binary. `D:\max\GameGuruMAX` / `D:\max\WickedRepo` (DX11 references) are NOT here.
- `VULKAN_SDK` env var is EMPTY; DXSDK June 2010 is installed and `DXSDK_DIR` is set.

## ★★★ The untracked .lib set (`.gitignore:17` ignores `*.lib` — POLICY, keep it)
A fresh clone/copy CANNOT link until these exist. Restored this session:
- `GameGuru Core/SDK/OpenXR/lib64/{Debug,Release}/openxr_loader.lib` ← GAMEGURUMAXREPO same path
- `GameGuru Core/Dark Wicked Shared/Lib64/` ← steam_api64, OptickCore, assimp, 3× Photon
  (`*_vc14_release_windows_mt_x64`), D3DX11 (from DXSDK `Lib\x64` — vcxproj never searches
  DXSDK_DIR), ogg/vorbis/vorbisfile statics (see below)
- `GameGuru Core/SDK/DirectXTex/DirectXTex/Bin/Desktop_2022/x64/Release/DirectXTex.lib`
- `.../Dark Basic Pro SDK/Shared/BaseClasses/STRMBASE.lib` (referenced by explicit path)
- `WickedEngine_Windows.lib` — not a copy: **build the engine first** on any new box.

## ★★★ C1047: /GL libs are compiler-version-locked
The 2024-built ogg/vorbis statics died with C1047 against fresh objects. **Rebuilt from the
tracked source** via `GameGuru Core/SDK/OGG/build_ogg.bat` (committed) — `/t:Rebuild`,
`WholeProgramOptimization=false` so they can never version-lock again. If another old lib C1047s
(candidates: DirectXTex, OptickCore, STRMBASE, openxr_loader), rebuild it the same way, do not
hunt binaries.

## ★★ Traps re-paid this session
- `cp -r` gives outputs FRESH mtimes → msbuild sees them up-to-date → **/t:Build is a silent
  no-op on copied trees**. md5sum proved three "rebuilt" libs identical. Force `/t:Rebuild`.
- `cmd //c "call ... && msbuild ..."` from Git Bash ran NOTHING, silently — same lesson as the
  vacuous-test rule: write a .bat and run that.
- A stale intermediate tree copied from another machine C1047s against fresh code with the blame
  pointed at the WRONG side (named the lib, stale side was the .obj). One clean rebuild disproved it.

## Still unverified on this machine
- MAX has not been LAUNCHED here yet; harness, demos, sweepgate all untested.
- FPS/VRAM baselines are OLD-MACHINE numbers; new GPU unknown — re-baseline before any A/B claim.
