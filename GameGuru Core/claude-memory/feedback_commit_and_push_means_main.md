---
name: feedback-commit-and-push-means-main
description: "When the user says \"commit and push\" they mean their local main branch updated, merged with whatever I made, and origin/main matching. Not a feature branch sitting on origin."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 89bab6a9-6782-4447-8063-b03e17d3981b
---

When the user says **"commit and push"** in this project, they mean ONE specific end-state:

1. Their **local `main` branch** in `D:/max/GameGuruMAXDX12` reflects the work.
2. **origin/main** matches their local `main`.
3. Their next build from `D:/max/GameGuruMAXDX12/GameGuru Core` picks up that work.

NOT: "commit on whatever branch I'm currently on and push that branch somewhere." If I'm working in a worktree on a feature branch, committing+pushing the feature branch is *not* what was asked. The work is invisible to them until it lands on `main`.

**Why:** They came back from a session expecting to see their cursor work, ran their normal build, got an EXE silently missing whole features. The cause was that I'd been "committing and pushing" to `claude/determined-chebyshev-bf0892` on origin while their local `main` was 30+ commits stale. They thought my "push" updated their main repo branch — it did not.

**How to apply:** Every "commit and push" must end with the sequence below (or call out clearly why I'm skipping a step and get confirmation):

```bash
# 1. Commit on the worktree branch first (preserves history of small commits)
git add ... && git commit -m "..."

# 2. Bring the main repo's working tree to a state I can pull/merge into
#    (stash any pending mirror M-files — they'll be reapplied by the merge)
git -C "D:/max/GameGuruMAXDX12" stash push -m "temp: pre-merge mirror M-files"

# 3. Bring main repo's local `main` up to origin/main
git -C "D:/max/GameGuruMAXDX12" pull --ff-only origin main

# 4. Fast-forward `main` to the worktree branch tip (or merge if non-FF needed)
git -C "D:/max/GameGuruMAXDX12" merge --ff-only <worktree-branch>

# 5. Push the updated main
git -C "D:/max/GameGuruMAXDX12" push origin main

# 6. Drop the temp stash (its content matches the merge)
git -C "D:/max/GameGuruMAXDX12" stash drop stash@{0}

# 7. Verify: `git -C ... log -3 --oneline` shows my commits at top of main,
#    working tree clean, the user's other stashes still listed.
```

**Never** touch any stash whose message does not start with "temp: " (mine). The user's prior WIP stashes are sacred — preserve their index across all of this.

If a non-fast-forward situation appears (merge conflicts, divergent histories) STOP and ask. Don't improvise destructively.
