---
name: project-demo-shadow-copy
description: "A read-only hub demo loads a SAVED COPY of its level from the writable mapbank first - why one tester sees a level problem nobody else can reproduce (3.123, tester #8); Lee's own Switch Escape copy"
metadata:
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-10-22T15:04:23.704Z
---

**A "(read only)" hub demo is read-only only for its STORYBOARD** (`project_readonly` = where
`projectbank\<demo>\project.dat` was found). The level node is the relative `mapbank\<level>.fpm`, opened via
`GG_GetRealPath`, which checks the writable area FIRST (custom write folder, then
`Documents\GameGuruApps\GameGuruMAX\Files\mapbank\`), then the install copy. Level Save has no read-only guard and
always writes the writable copy - one Save (or "Yes" to "save first?") on a demo level shadows that demo for good.
DX11 Steam MAX shares the folder (same exe name, same `GG_GetRealPath`), so a DX11 save shadows the DX12 demo too.
Measured 3.123 (2026-10-22): crafted copy present -> `GET_TREE_LOADSTATS fpm=` the Documents path, `readonly=1`.

**Why:** tester #8 got "TREE TYPES DROPPED: 15015" on Island Showdown; Lee/testers did not - the stock .fpm is
clean everywhere, the tester loads their own saved copy. The 3.101 note ("reopening the demo gives the original")
was WRONG - corrected in TESTER_TRIAGE §3.

**How to apply:** a level symptom on ONE machine with a stock demo -> first ask for
`Documents\GameGuruApps\GameGuruMAX\Files\mapbank\<level>.fpm` (+ `_automatedbackups`, OneDrive redirection,
custom write folder). Before any hub-demo sweep/FPS baseline, check the writable mapbank holds no demo-named .fpm.
⚠ Lee's desktop has `switch escape.fpm` there (DX12 save 09-24 01:17, non-stock trees/entities/visuals) - hub
Switch Escape numbers since 09-24 measured it; reported to Lee 10-22, his to remove.
Tools: `tools/treetypes_scan.py` (opens the encrypted .fpm, password 'mypassword'), `tools/tre_craft.py`.
[[project-writable-area]] [[project-trees-phase5]] [[project-measuring-rules]]
