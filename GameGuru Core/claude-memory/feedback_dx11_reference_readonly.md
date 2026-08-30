---
name: feedback-dx11-reference-readonly
description: "The production DX11 GameGuru MAX source lives at D:\\max\\GameGuruMAX as a READ-ONLY reference. Never write, edit, or run git-mutating commands there — reading any file is encouraged for porting work."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
---

# DX11 reference repo — READ ONLY

The user keeps the latest production DX11 GameGuru MAX source at `D:\max\GameGuruMAX` (added 2026-07-18). It is the authoritative reference for porting scenarios — file-format version bumps (like .ele v342), feature diffs, shader behaviour, upstream sync candidates.

**The user's exact framing: "It is VITAL that you do not WRITE anything into this repo."**

**How to apply:**
- Read/Grep/Glob freely — that is what it is for. Its monolithic files map to DX12's split parts (e.g. DX11 `M-Entity.cpp` = DX12 `M-Entity_part0..4.cpp`).
- NEVER Edit/Write any file under `D:\max\GameGuruMAX`. NEVER run git commands that mutate it (commit, checkout, stash, pull). No build commands there either.
- When a DX12 loader hits an unknown newer file version, diff the relevant DX11 save/load functions here first — the v342 port ([[project-level-version-debt]]) is the worked example of the recipe.
