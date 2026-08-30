---
name: feedback-mirror-complete-file-set
description: "Building from main repo while editing in worktree — every transitively-needed file must be mirrored, not just the obviously-edited ones."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 89bab6a9-6782-4447-8063-b03e17d3981b
---

When building from the main repo (`D:\max\GameGuruMAXDX12\GameGuru Core`) while editing in a worktree, **every transitively-referenced file** must be mirrored — not just the `.cpp` I'm consciously changing.

Concretely: if worktree has a new function declared in `Foo.h` and called from `Bar.cpp`, mirroring only `Bar.cpp` causes a silent feature regression. The build either:
- Fails (`identifier not found`) — easy case, you notice immediately, OR
- **Succeeds with stale incremental `.obj` artifacts** — disaster case, the EXE *appears* to build but the feature is silently broken. The user runs it, sees the feature gone, blames recent code changes. Hours wasted hunting the wrong cause.

**Why:** Most build sessions in this conversation came from the main repo via `Push-Location "D:\max\GameGuruMAXDX12\GameGuru Core"`. Main repo is on `main` branch, which lacks worktree-branch features (`SetBrushCursor` block + declaration). Mirroring only changed `.cpp` files for my current task left the supporting `.h` and other related `.cpp` files on stale main-branch versions. Compile picked up the old declarations; brush cursor silently regressed.

**How to apply:**
- Before EVERY build from main repo: run `git -C "D:/max/GameGuruMAXDX12/.claude/worktrees/<name>" diff main --name-only -- "GameGuru Core/"` to list every file the worktree branch touches relative to main, and mirror all of them. Don't just mirror the file you happened to edit this turn.
- Or simpler: build from the WORKTREE itself (`Push-Location` the worktree path) — slower setup (junctions + copied libs per [feedback_worktree_build.md](feedback_worktree_build.md)) but no mirror sync drift.
- If a feature mysteriously regresses without obvious code changes, immediately compare `git diff main -- "GameGuru Core/"` between worktree and main-repo working tree — drift between them is the most likely cause.
