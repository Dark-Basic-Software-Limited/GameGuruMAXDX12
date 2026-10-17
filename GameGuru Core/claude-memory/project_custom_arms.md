---
name: project-custom-arms
description: "Player Start \"Hands\" arm sets, the interactive swimming arms and the legacy arm replacement trick - how a set reaches a gun, why it went stale (3.122, tester report #7), how to test it"
metadata:
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-10-17T14:45:45.339Z
---

**Feature map (3.122, 2026-10-17, notes §3.122, TESTER_TRIAGE #7).** Arm sets = `Files/gamecore/hands/<set>/`
(arms.dbo + arms_*.dds). Player Start **Hands** dropdown (only for a new-weapon-system start weapon) -> `eleprof.texaltd_s`
-> `g_guns_customArms_s` at each game start (`gun_activategunsfromentities`). Two ways a set reaches a gun:
- `resources\hud.dat` weapons (max\Colt, lowpoly, aztec, tools): `gun_createhud` REBUILDS HUD.dbo, saved to the WRITABLE
  area (`Documents/GameGuruApps/GameGuruMAX/Files/gamecore/guns/<gun>/HUD.dbo` + `hudcustom.txt`), read before the root copy.
- `HUD-ORIG.dbo` weapons (enhanced\*, `interactive\swimming`): the **legacy arm replacement trick** in `gun_load` - set's
  arms.dbo in a second object at `gunbankextraobjoffset`+slot, own `LEGACYARMS` limb hidden, animations mirrored by the
  `gun_*` wrappers, synced in `gun_control`.
- Interactive arms = `gamecore/guns/interactive/swimming`, equipped mid-game by `scriptbank/global/swimming.lua`
  (ChangePlayerWeapon; its AddPlayerWeapon(string) is a no-op). Default swimming arms == Male Light (same file).

**The defect:** gun objects outlive a game (Test Game after Test Game, level after level); only `gridedit_clear_map`
deletes them. Mid-game-added guns were never rebuilt (`gun_selectandorload` loaded only `obj==0`), and set -> No
Preference never forced a rebuild. Fixed with `g_guns_loadedArms` (set each gun was built with) + rebuild on mismatch.
Gun code is IDENTICAL to DX11 - the staleness is latent there too; the tester's "worked in DX11" was a fresh session.

**Test:** harness `SET_PLAYERARMS`, `SET_PLAYERWEAPON`, `SET_ENTITYSCRIPT 3 global\swimming.lua` (all live-only),
`DUMP_GUNARMS` (built= / LEGACYARMS / second object texture); tools `arms_swim_ab.sh`, `arms_weapon_probe.sh`; deep water
in DX12TESTLEVEL at (3500,-80,7500); play `PlayGunAnimation(70,110)` to bring the arms on screen. ⚠ Never judge with Male
Light or (on enhanced) Combat Gloves Light - identical to the defaults. ⚠ The player drowns in ~30 s - shoot fast.
⚠ Testing writes `hudcustom.txt`/`HUD.dbo` into the writable area - list them for Lee, never delete (his rule).

**Rebuild side effects (review):** a rebuild of a gun already in memory re-runs ALL of gun_load: it used to orphan the gun's
whole sound set (fixed: `gun_freegunsounds` first); it re-parses gunspec (Lua/entity overrides of a carried weapon revert -
DX11 did the same); a third-party HUD-ORIG gun with gunspec rotx/roty/rotz gets its pivot rotated again (object reused; no
stock weapon has them). Prefer rebuilding ONLY on a real set change.

**Still open (pre-existing, also DX11):** hud.dat weapon + No Preference keeps the last custom writable HUD.dbo;
`gun_createhud("")` comes out WHITE (base_arms.dbo references missing .png), so the revert = deleting that cache.
[[project-rules-debugging]] [[project-object-library-preview]]
