---
name: feedback-never-save-dx12testlevel
description: "TESTPRO2 > DX12TESTLEVEL must NEVER be saved, not even for a test round trip with a byte-for-byte restore - Lee's standing rule (relayed 2026-10-22 by the 3.124 session after 3.125 saved it twice). Test saves go on a copy / another level."
metadata:
  node_type: memory
  type: feedback
  originSessionId: aed3e3d8-e8e5-49ab-868e-e755b9560734
  modified: 2026-10-22T20:35:13.189Z
---

**DX12TESTLEVEL (TESTPRO2) must never be saved** - Lee's standing rule, relayed on 2026-10-22 by the parallel 3.124
session after the 3.125 session saved it twice for a save -> cold-reload test (restored byte for byte afterwards,
but the first save's backup rotation still deleted `_automatedbackups\DX12TESTLEVEL_7.fpm` - see
[[project-custom-terrain-textures]]).

**Why:** it is Lee's reference level for the terrain/grass/bake rounds; the general rule "never save Lee's projects
except TESTPRO2" did NOT cover it, and a save has side effects a restore cannot undo (backup rotation).

**How to apply:** a test that needs SAVE_LEVEL uses a COPY or another TESTPRO2 level node, never DX12TESTLEVEL - and
even then back up the .fpm/.lst, the `_automatedbackups\<level>.dat` counter and the oldest `<level>_N.fpm` first.
Loading DX12TESTLEVEL and changing it in memory is fine; never SAVE_LEVEL / Ctrl+S on it.
