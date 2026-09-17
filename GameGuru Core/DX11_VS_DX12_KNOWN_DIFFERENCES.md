# DX11 vs DX12 — known and intended differences

**For manual side-by-side testing.** Written 2026-09-17, after two full parity audits and 32 fixes.

If you are comparing DX12 against the latest DX11 build and find a difference, check here first.
Everything below is **known** — either a deliberate decision, an engine-level impossibility, or a
place DX12 is intentionally *better*. Anything you find that is **not** on this list is worth
reporting.

Parity state: all 338 DX11 commits since 2025-11-21 (`ca32a143`..`3e21f674`) audited per-commit,
then a second structural pass. Hub demos 19/19 in the editor and 19/19 in Test Game.

---

## 1. Behaviour that intentionally differs from the DX11 you last used

### Bullet holes on non-static, immobile objects — CHANGED, verify this one
DX11 commit `c1269dd7` (issue 6236) **removed** the `isimmobile` clause. DX12 had kept it. As of
3.44 we match DX11: a non-static but immobile object (a door) no longer collects bullet decals
**unless** its "Allow Bullet Holes ?" tickbox is set.

Why DX11 did it: doors collected decals that then hang in mid-air the moment the door swings, and
keeping the clause made the opt-in tickbox decorative.

⚠ This is the one change that alters behaviour you personally tested on 2026-09-16. If it reads
wrong to you, the clause is a two-line revert.

### Shadow defaults on a NEW level
Delayed Shadows now defaults **ON**, cascade resolution defaults **1024** (was off / 2048), matching
DX11 `d836fcab`. Existing levels restore their own saved values, so this only shows on a new level.

⚠ Any FPS baseline taken before this is not comparable with one taken after.

### Start Marker underwater
The clamp that teleported an underwater Start Marker to waterline+20 is gone (DX11 `1d49f004`), so
an underwater-opening level is now possible. DX11's own note: *"new request wants to start
underwater if the marker is underwater"*.

---

## 2. DX11 features DX12 does NOT have, on purpose

| feature | why |
|---|---|
| **Nav-mesh high-quality extraction** (`15cbd06c`) | Ported byte-identically, then **reverted**. It blocks the main thread 60+ seconds on a large map under DX12's terrain system. Bisected both ways on Horseshoe Bend. DX12 keeps the LOD-limited nav mesh, so AI path detail varies with where the camera sat when the mesh was built. Needs a different shape (worker thread, or explicit Build Nav Mesh only), not a different patch. |
| **`terrainsleep` lock restructure** (`633f216f`) | **DX11 walked this back.** `01d27f07` restored the original lock-held block unconditionally; the restructure is dead code in DX11's `#else` arm. DX11 today compiles what DX12 already has. |
| **PIX `BeginCommandList(QUEUE, "name")`** | DX12's engine has no name parameter and emits PIX markers itself. |
| **`SetSpecialGGDebugLog`** | The API does not exist in the DX12 Wicked fork. |
| **PP Snow, transparent shadows, bloom strength, gamma fade** | Engine API no longer exists. Three of the four need an engine-side port, not a game-side one. |
| **Custom shader parameters** (`customShaderParam1-7`) | The engine replaced seven floats with `uint4 userdata` and the migration was never finished. Authoring-surface only — **zero shipped `.fpe` files use them** (4248 scanned). As of 3.50 your authored values are at least no longer *destroyed* on entity selection; they simply have no effect yet. |
| **Water / Glass / Tree-Animate custom shaders** | Their bodies are not in the build: the stubs `#include "objectHF.hlsli"` and bind to the **engine** header because `CustomShaders.cpp` pushes the engine dir first. Proven from the shipped `.cso` files — `objectPS_custom_water.cso` and `objectPS_transparent_glass.cso` are the same binary. DX11 never hits this because it compiles shaders offline. |

---

## 3. Where DX12 is deliberately different, and better

- **`GetObjectAnimationFinished`** has an `AnimationComponent` null check DX11 lacks.
- **`door.lua`** is DX11 v33 **plus** the GGMAX 3.21 logic gating: `SetEntityAlwaysActive` is now
  conditional on `use_switch` instead of unconditional. Every door in a level used to opt out of the
  750-unit logic freeze; now only switch-operated doors do.
- **Lua 5.4 compat shim** — DX11 ships Lua 5.2. DX12 links the engine's 5.4.8 and shims the removed
  surface (`math.atan2`, `mod`, `pow`, `log10`, `unpack`, and the whole `bit32` library) in C++
  before any script runs. Script behaviour should be identical; the mechanism is not.
- **Crash handler** tolerates a NULL exception record and a failed `SymInitialize`, and prints the
  breadcrumb ring buffer. DX11 does none of the three.

---

## 4. Not verified by anyone — most likely place to find a real bug

These were fixed and shown to build and not regress, but the behaviour itself has never been
exercised. If you want to spend your testing time where it is most likely to pay:

| what | how to reach it |
|---|---|
| Storyboard video auto-advance | a non-looping splash video linked to a screen — it should advance itself at the last frame |
| Save-game reload of spawned objects | spawn something during play, save, reload — it should be visible and solid |
| Chest keys | put a key in a chest, assign it as a door's USE KEY, take it, try the door |
| Lua Sound slot 4 | an entity using Sound4 from Lua — it should play slot 4, not slot 5 |
| `animchoicemode` 1 and 2 | a GUNSPEC using them — they were unreachable before |
| Ultrawide Save/Load | click near where two slot boxes meet |
| Grid size below 1.0 | set 0.5, restart MAX, check it survived |
| WPE preview offsets | an emitter with an X/Z offset — preview should match the placed result |
| Delayed Shot | bow / crossbow |
| Additive blending | a glow decal — it should stay additive when you Test Game |

---

## 5. Known blind spot

⚠ **Assets were never audited.** Neither repo tracks binaries and there is no DX11 build area on
this machine, so any art, model or sound DX11 added since Nov 2025 is invisible to every pass run.
If you spot missing or wrong *content* rather than wrong *behaviour*, that is why.

New content files added on the DX12 side live in `GameGuru Core/content-additions/` and are deployed
by `tools/deploy_content_additions.sh` — run it after rebuilding the build area.
