---
name: project-save-crash-group-thumbnails
description: "RESOLVED 2026-07-24 (game 8ed85a83): saving a level that contains SMART-OBJECT GROUPS crashed mid-save. Root cause: entity_saveelementsdata's v319 block (ent==1) writes each group's thumbnail to groupimg<n>.png via SaveImage, but under the Wicked/DX12 backend those thumbnail images have a NULL lpTexture, and SaveImageCore did m_imgptr->lpTexture->QueryInterface(...) on null -> access violation. Fixed with a null-guard in SaveImageCore (CImageC_part1.cpp:1216). Camera change in the user's report was incidental; ANY save of a groups level crashed."
metadata:
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
  modified: 2026-07-24T02:25:35.288Z
---

# Save crash on levels with smart-object groups — RESOLVED (game `8ed85a83`)

**Symptom:** "change the camera angle and save the level -> crash." The crash log (Guru-MapEditor-last.log)
ended mid `SAVETESTMAP: Save elements`. The camera change was INCIDENTAL — any save of this level crashed.

**Root cause (deterministic):** `gridedit_save_map` (M-GridEdit_part7.cpp) -> `gridedit_save_test_map` ->
`entity_saveelementsdata` (M-Entity_part4.cpp, a two-pass EntityWriter). Its **v319 block, which runs ONLY
for ent==1**, saves each smart-object group's thumbnail to `groupimg<n>.png` via `SaveImage(file, imgID)`
(line ~439). Under the Wicked/DX12 backend those group-thumbnail images are valid image SLOTS
(`ImageExist`/`UpdatePtrImage` return true, `m_imgptr` set) but carry **NO D3D texture — `lpTexture == NULL`**.
`SaveImageCore` (CImageC_part1.cpp:1216) then did `m_imgptr->lpTexture->QueryInterface<ID3D11Texture2D>(...)`
on the null pointer -> access violation -> whole save died. The image IDs were large (119000, 119002, 119003
for groups 6/30/38) — normal for Wicked-backed thumbnails, not corruption.

**Fix (`8ed85a83`, game-only):** null-guard `m_imgptr->lpTexture` in `SaveImageCore` before the
QueryInterface — if null, log (throttled) + `return false` instead of dereferencing. Robust for ANY
texture-less image. Verified: SAVE_LEVEL now runs to `SAVETESTMAP: Complete`, MAX responsive, log shows the
guard skipping images 119000/119002/119003 gracefully.

**Not my regression** — the save/entity code is stable/pre-existing; my shadow work never touched the save
path. This is a latent DX12-port bug that only bites levels with smart-object groups.

**KNOWN LIMITATIONS / follow-ups (NOT crashes, left as-is):**
1. **Group thumbnails don't persist in DX12** — because the Wicked backend never populates `lpTexture` for
   these thumbnail images, `SaveImage` always skips them, so `groupimg<n>.png` isn't written. Group preview
   images in the left panel may not survive a reload. A proper fix = make group-thumbnail images have a
   readable texture under Wicked (deeper, separate task).
2. **SaveImage is called inside the two-pass writer loop** (v319, both pass 0 AND pass 1) — a heavy file-I/O
   side-effect that (a) shouldn't run in the counting pass and (b) runs twice for valid thumbnails. Harmless
   now (guarded), but structurally wrong; could gate the image-save block to `pass == 1`.

**Tooling added this session (kept):** harness `SAVE_LEVEL` command (runs `gridedit_save_map`) for automated
save-path testing. The crash was pinpointed with a crash-surviving `save_dbg.txt` written per-entity /
per-field-block (overwrite each iteration; last value = crash site) — a reusable technique for no-debugger
crash bisection. Related: [[project-next-action-immediate]].
