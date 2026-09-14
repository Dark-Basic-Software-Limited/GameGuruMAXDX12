---
name: project-machine-migration
description: "TWO machines now - the RX 9060 XT desktop and the GTX 1050 laptop. Identify which one you are on BEFORE trusting any path, GPU or perf number."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-14T12:57:22.246Z
---

# ⚠ There are TWO machines. Check which one you are on FIRST.

The 2026-08-31 session set up a **second** box (a laptop) and wrote this file as though the project
had *migrated*. It had not — Lee went back to the desktop on 2026-09-14 and the laptop was never
used after setup. Five statements in that original version are FALSE on the desktop. Everything
below is now machine-scoped; do not read a row without checking the column.

## ★★★ Identify the box in one command

```bash
[ -d /d/DEV/BUILDS ] && echo LAPTOP || echo DESKTOP
```

Corroborate with the GPU if it matters:
`powershell.exe -NoProfile -Command "Get-CimInstance Win32_VideoController | % Name"`

| | **DESKTOP** (primary) | **LAPTOP** (set up 08-31, unused since) |
|---|---|---|
| GPU | **RX 9060 XT** (+ Intel UHD) | **GTX 1050**, driver budget 3406 MB |
| Visual Studio | `...\18\Community` **and** `...\2022\Community` both present | `...\2022\Community` only; 18 absent |
| build area | `D:\DEV\BUILD\...` — a **real directory** | `D:\DEV\BUILDS\...` + junction `D:\DEV\BUILD` → it |
| DX11 references | `D:\max\GameGuruMAX`, `D:\max\WickedRepo` **PRESENT** (read-only) | **absent** |
| donor repo | — | `D:\DEV\GAMEGURUMAXREPO` (source of untracked binaries) |
| perf baselines | ★ every FPS/VRAM number in memory was measured here | ★★ **all of them INVALID here** — re-baseline |

⚠ `build.bat` / `build_wicked.bat` are **committed pointing at VS 2022**, which the laptop needed.
Verified 2026-09-14: both still build clean on the desktop, because it has VS 2022 too. If either
box ever loses that install this becomes a merge conflict waiting to happen — the honest fix would
be `vswhere` rather than a hardcoded path, not done.

## ★★★ The untracked .lib set (`.gitignore:17` ignores `*.lib` — POLICY, keep it)

This is the durable part, and it applies to **any** fresh box. A clone CANNOT link until these
exist; the 08-31 laptop build took six link failures to get green:

- `GameGuru Core/SDK/OpenXR/lib64/{Debug,Release}/openxr_loader.lib`
- `GameGuru Core/Dark Wicked Shared/Lib64/` — steam_api64, OptickCore, assimp, 3× Photon
  (`*_vc14_release_windows_mt_x64`), D3DX11 (from DXSDK `Lib\x64` — the vcxproj never searches
  `DXSDK_DIR`), ogg/vorbis/vorbisfile statics
- `GameGuru Core/SDK/DirectXTex/.../Desktop_2022/x64/Release/DirectXTex.lib`
- `.../Dark Basic Pro SDK/Shared/BaseClasses/STRMBASE.lib` (referenced by explicit path)
- `WickedEngine_Windows.lib` — **not a copy: build the engine first on any new box**

## ★★★ C1047: /GL libs are compiler-version-locked

The 2024-built ogg/vorbis statics died with C1047 against fresh objects. **Rebuilt from tracked
source** via `GameGuru Core/SDK/OGG/build_ogg.bat` (committed) — `/t:Rebuild` and
`WholeProgramOptimization=false` so they can never version-lock again. If another old lib C1047s
(candidates: DirectXTex, OptickCore, STRMBASE, openxr_loader) rebuild it the same way; do not hunt
for binaries.

## ★★ Traps re-paid on 08-31 (machine-independent)

- `cp -r` gives outputs FRESH mtimes → msbuild sees them up to date → **`/t:Build` is a silent
  no-op on a copied tree.** md5sum proved three "rebuilt" libs identical. Force `/t:Rebuild`.
- `cmd //c "call ... && msbuild ..."` from Git Bash ran NOTHING, silently — same family as the
  vacuous-test rule in [[project-measuring-rules]]. Write a .bat and run that.
- A stale intermediate tree C1047'd against fresh code with the blame pointed at the WRONG side
  (it named the lib; the stale side was the .obj). One clean rebuild disproved it.
- ⚠ Harness commands sent during hub transitions can time out yet still execute later — gate on
  the LOAD (`GET_STATE`), never on each command's reply.

## Crashes from the laptop's one day of use

1. **Startup AV in a LoadShaders job — FIXED**, engine 3.36 `edce8393`. `CreatePipelineState` read
   `to_internal(desc.xs)->shadercode` for seven stages with no null check; a stage whose LoadShader
   lost the startup race has null `internal_state` → AV at null+0x20. The fast desktop always won
   that race; the laptop lost it once in three launches. Now refused **with the stage named**.
   ⚠ Benign sibling, do not chase: `shadowClearPS.cso` is the one .cso with no embedded RTS0, so
   `wiGraphicsDevice_DX12.cpp:4666` logs one E_INVALIDARG per boot — harmless for a PS.
2. ★ **`LoadAssImpObject` AV (`DBOAssImp_part0.cpp:1142`) — OPEN, seen ONCE on the laptop, never
   reproduced** (0 for 4 test-game entries). Physics loading a companion `.obj`/`_COL.obj`
   collision mesh; AV READ at `0xFFFFFFFFFFFFFFFF` walking the sibling-frame list. `szName` is 256
   (MAX_STRING) so a simple name overflow is unlikely. The same content and code ran for months on
   the desktop. Suspects: different v143 codegen, or that box's memory pressure on a starved card.
   ⚠ **Only ever seen on the GTX 1050** — do not assume it reproduces on the desktop.
   `Guru-Crash.log` names the exact line if it recurs; add the `.obj` filename as the first
   instrument THEN, not before.

See [[project-next-action-immediate]] for current state.
