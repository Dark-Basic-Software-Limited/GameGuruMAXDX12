---
name: feedback-check-main-repo-status
description: "Before cp-ing files from a worktree to the main repo, ALWAYS check the main repo's git status for uncommitted changes. The main repo often holds work-in-progress local edits (e.g. checkouts from feature branches that haven't been merged); overwriting them silently destroys hours of work."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 89bab6a9-6782-4447-8063-b03e17d3981b
---

When working from a worktree (e.g. `D:\max\GameGuruMAXDX12\.claude\worktrees\<name>`) and mirroring files to the main repo (`D:\max\GameGuruMAXDX12`) so the build can pick them up (the worktree path breaks the project's relative-include depth), ALWAYS first run:

```
git -C "D:/max/GameGuruMAXDX12" status --short
```

If anything shows as modified (` M file`) or has untracked entries (`?? file`) under source dirs, the main repo has work-in-progress that the user has put there but not committed. A plain `cp worktree/<file> main_repo/<file>` will **silently destroy** that work because the worktree's file is based on whatever the worktree branch is at (often a different baseline).

**Why:** On 2026-06-17 I overwrote the user's uncommitted Phase 4 grass implementation (5 commits' worth of code that lived on a feature branch, checked out into the main repo for local building). Symptoms: grass rendering vanished from MAX even after I reverted my own session edits. Took multiple build cycles, a clean rebuild, and finally finding the work on `claude/frosty-ritchie-f7efe6` to recover. The grass code had to be cherry-picked onto our branch and the user pushed it permanently to origin/main.

**How to apply:**
1. Before any `cp <worktree-file> <main-repo-file>`, run `git -C <main-repo> status --short` and surface any modified/untracked files in source dirs.
2. If there's anything ambiguous, ask the user: "your main repo shows X.cpp modified — is that work you want preserved? I'd be overwriting it." Don't assume.
3. Once a session has done a mirror+build cycle once safely, subsequent mirrors of the SAME files are fine — the risk is only on the first overwrite of files whose existing content is unknown.

Related: [[feedback-worktree-build]] explains the underlying reason the cp is needed at all (relative include paths).
