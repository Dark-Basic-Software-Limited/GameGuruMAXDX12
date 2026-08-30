---
name: feedback-edit-worktree-path
description: "When session cwd is a worktree, every Edit/Write must use the worktree absolute path — never the main-repo path. The Edit tool takes literal absolute paths and will silently land on the wrong tree if the path is wrong. This has corrupted user WIP twice now."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 89bab6a9-6782-4447-8063-b03e17d3981b
---

When the session cwd is `D:\max\GameGuruMAXDX12\.claude\worktrees\<name>\...`, **every Edit/Write/Read I issue must use the worktree absolute path**, never the main-repo path `D:\max\GameGuruMAXDX12\GameGuru Core\...`. Bash commands run in the worktree by default, but `Edit` takes a literal absolute path — if I absent-mindedly type the main-repo path, the edit silently lands there.

**Why this matters:** the main repo frequently holds the user's WIP (uncommitted local work). My edits stack on top of that WIP without warning. The user's diff blows up by hundreds of lines and theirs gets mixed with mine, making clean separation expensive. We already had two incidents — the May 29 mirror-overwrite ([feedback_check_main_repo_status](feedback_check_main_repo_status.md)) and the 2026-06-18 wrong-path edit during the per-grass-type texture work.

**How to apply:**
- Before *any* Edit/Write in a worktree session, mentally verify the path begins with the worktree prefix (`.claude\worktrees\<name>\`). If it doesn't, fix it before sending the tool call.
- A symptom that I'm editing the wrong tree: my changes "vanish" — `grep` for a unique string from the new code returns no matches in the file I expected. That means I edited a different file with the same basename.
- Read-before-edit (the harness already enforces this) is a partial safeguard, but it doesn't help if I read the main-repo file too — the Read just teaches me to consistently edit the main-repo copy. The actual fix is to **construct the path from the cwd**, not from muscle memory.
- When the user's main repo has WIP and I notice mid-task that I've polluted it, the recovery is `git -C "<main-repo>" stash push -u -m "user WIP + accidental Claude edits <date> (review and split)"`. That preserves both sides for the user to triage later.

Relates to [[feedback-worktree-build]] (worktree build setup) and [[feedback-check-main-repo-status]] (always check `git status` on the main repo before any cross-tree action).
