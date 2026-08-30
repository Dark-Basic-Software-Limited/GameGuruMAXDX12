---
name: feedback-worktree-build
description: How to build/run GameGuru MAX from a git worktree (junction + copy gitignored libs + direct cmd /c). Needed whenever the cwd is under .claude/worktrees/.
metadata: 
  node_type: memory
  type: feedback
  originSessionId: e8952cbe-c08c-485a-a50c-e1debb54656f
---

Building GameGuru MAX from a git **worktree** (path under `.claude\worktrees\<name>\`) needs three one-time setup steps the main-repo build does not. A fresh worktree contains only git-TRACKED files and sits 3 levels deeper than the main repo, which breaks the project's relative paths.

1. **Junction for WickedEngineDX12.** Source files include `"../../../../WickedEngineDX12/..."`; from a worktree those resolve to `D:\max\GameGuruMAXDX12\.claude\worktrees\WickedEngineDX12`. One junction fixes ALL such relative paths (source includes AND the lib dir `$(SolutionDir)..\..\WickedEngineDX12\BUILD`), because they all target the directory CONTAINING the repo root:
   `New-Item -ItemType Junction -Path 'D:\max\GameGuruMAXDX12\.claude\worktrees\WickedEngineDX12' -Target 'D:\max\WickedEngineDX12'`
2. **Copy gitignored prebuilt `*.lib`.** `.gitignore` excludes `*.lib`, so a worktree has NONE of the prebuilt libs (openxr_loader.lib, DirectXTex, Photon, steam_api64, WickedEngine_Windows, etc.) and linking fails with `LNK1181: cannot open input file`. Copy them from the MAIN repo's `GameGuru Core` into the worktree's `GameGuru Core` at the same relative paths, **copy-if-missing** (don't clobber sublibs the worktree compiles itself).
3. **Run build.bat via DIRECT `cmd /c`** — NOT PowerShell `Start-Process` nor `Set-Location`+cmd. Those fail with `'build.bat' is not recognized` even with the correct CWD: PowerShell does not propagate its location to a child cmd, and `Start-Process -NoNewWindow` breaks batch command resolution. Use a wrapper .bat that `cd /d`s with an ABSOLUTE path then `call`s build.bat, invoked as `cmd /c 'D:\max\<wrapper>.bat'`. (This does NOT conflict with the main-repo "never `cmd //c`" rule in MEMORY.md — that rule bans relying on bash's cwd propagating into build.bat; the wrapper here does its own absolute `cd /d`, so cwd never matters.)

**Why:** Discovered the hard way (2026-05-29, grass feature) after ~6 failed build launches. As of 2026-07-17 this is still not in CLAUDE.md, which documents only the main-repo build.

**How to apply:** Any build/run request while the cwd is a worktree. Release `OutDir` is the absolute `$(GG_MAX_BUILD_PATH)\Max`, so a worktree build still produces the shared runtime EXE at `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max` — launch/test exactly as for a main build. Clean up junctions with `rmdir`/`Remove-Item` on the junction itself, NEVER `rm -rf` (which would follow into and delete the real target). Relates to MEMORY.md → CRITICAL BUILD RULES (Release-only; run from the EXE dir).
