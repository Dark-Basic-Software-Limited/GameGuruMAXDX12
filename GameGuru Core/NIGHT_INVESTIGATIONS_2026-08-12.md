# Overnight investigations, 2026-08-12

Four user-reported issues, investigated by reading the code (the 19-demo sweep for 2.27 was
running on the only MAX instance, so no builds or live repro until it finished). Each entry:
what was OBSERVED, what the code SAYS, the root cause where one was found, and the fix shape.

⚠ Standing rule applied throughout after the §22.7 retraction: **name the assumption a conclusion
rests on and test THAT**, and do not report an inference as a finding.

---

## A. Impact smoke flume loops forever; only 5 can exist — ROOT-CAUSED

**Observed (user):** "When I shoot the floor, I see the particle effect of the impact, but I also
see a flume of smoke looping over and over, and it does not go away. I can create five of them
before the oldest is re-used for the new impact effect."

### The "five" is exact
`#define MAXREADYDECALS 5` — `GameGuru/Source/M-Entity_part0.cpp:31`, backing
`uint32_t ready_decals[MAXUNIQUEDECALS][MAXREADYDECALS]`. Each decal type gets a **5-slot
round-robin cache** of preloaded WPE emitters, handed out at `M-Entity_part5.cpp:178-184`:
```
iParticleEmitter = pParticle->emitterid = ready_decals[decal_id][decal_count[decal_id]];
decal_count[decal_id]++;  if (decal_count[decal_id] >= MaxCachedDecals) decal_count[decal_id] = 0;
```
The 6th impact wraps to slot 0 — "the oldest is re-used", exactly as reported.

### ★ Root cause: a WPE decal emitter is NEVER told to stop
Decal expiry, `M-Decal.cpp:973-980`:
```
if (t.decal[decalid].newparticle.bWPE) {
    if (framedelay >= 100) {
        //PE: No need to stop effect it will end by itself.
        active = 0;  framedelay = 0;  newparticle.emitterid = -1;
    }
}
```
It abandons the emitter on the assumption that the effect self-terminates. **That assumption is
false for a continuous/looping emitter** — a smoke flume never ends by itself.

Compare the sibling branch immediately below (`:983-997`), the non-WPE gpup path, which DOES tear
down: `newparticle_deleteparticleemitter(emitterid)` then `active = 0`. And
`newparticle_deleteparticleemitter` has exactly **one caller in the codebase** — that line. Nothing
ever stops or deletes a WPE decal emitter.

Meanwhile the emitter was started fully hot (`M-Entity_part5.cpp:258-264`):
Resume(3) → Restart(4) → Visible(5) → **Burst All(1)**.

The non-WPE path has an off switch — `gpup_emitterActive(id, 0)` (`:313-316`). **The WPE path has
no equivalent anywhere.**

### ★★ The codebase already knows this class of bug
`wickedcalls_part4.cpp:41-44`, on the actions restored in GGMAX 2.00:
> "Action 6 is the most-called of all of them — without it editor previews could never be hidden,
> and **weapon trails (action 7) never stopped emitting**."

Actions 6 (`SetVisible(false)`) and 7 (`SetEmitPaused(true)`) exist *because* emitters were failing
to stop. **The decal path was simply never updated to call them.**

### Fix (small, and it matches the requested behaviour)
In the WPE expiry branch, before clearing `emitterid`, issue **action 7 = `SetEmitPaused(true)`**.
That stops NEW particles while letting the ones already alive finish their lifetime — i.e. the
flume *dissipates* rather than vanishing, which is what was asked for. Action 2 (full pause) or 6
(hide) would both cut it off mid-air instead.
⚠ Reuse is already safe: the round-robin hands back an emitter and the `bParticle_Fire` path
Restarts + Bursts it, so a recycled slot is re-primed. The defect is only the abandoned run.
⚠ Do NOT delete the emitter here — it belongs to the `ready_decals` cache and must survive for
reuse; deleting it would strand a cache slot pointing at a dead entity.

### ⚠ NO interaction with the 2.27 decal pool — hypothesis checked and dropped
I flagged a possible interaction (stuck decals exhausting the smaller prewarm pool). **It does not
apply.** The decal ELEMENT is correctly freed (`active = 0`) on the same line that abandons the
emitter, so the element pool reclaims normally. The leak is the EMITTER, which lives in a separate
5-slot cache. The two systems do not touch.

**Status: root-caused, fix identified, NOT yet applied** (see the implementation log at the end).

---

## C. Shadow pops away when the caster leaves the camera frustum — ROOT CAUSE IDENTIFIED

**Observed (user):** "as soon as an object is not in the camera frustum, it is removed from casting
its shadow... when the object is overhead and casts a shadow in front of you, then the object
passes out of the visible frustum, the shadow disappears."

### First, what is NOT happening
The shadow caster gather does **not** use the camera-culled visible set. `wiRenderer.cpp:8017`
iterates `vis.scene->aabb_objects` — every object in the scene. So nothing rejects a caster merely
for being off-screen. The rejection is one line later: `shcams[cascade].frustum.CheckBoxFast(aabb)`
(`:8053`) — the caster must be inside the CASCADE volume.

### ★ Root cause: a metres-scale constant in an inch-scale world
`CreateDirLightShadowCams` builds two volumes per cascade. The second is explicitly the
off-screen-caster allowance (`wiRenderer.cpp:3544-3556`):
```
// culling extrusion for frustum:
//  It is coarser to allow far away casters to be drawn. Far away casters can be outside real
//  projection, and their depth will be clamped (depth clip is off)
float ext = abs(_center.z - _min.z);                     // = cascade sphere radius
ext = std::max(ext, std::min(2000.0f, farPlane) * 0.5f); // <-- 1000 WORLD UNITS
```
In light-view space Z **is** the light axis, so this extrusion is precisely the caster→shadow
displacement budget. `min(2000, farPlane) * 0.5` = **1000 units**, and **1 GG unit = 1 inch**
(`GGTerrain_MetersToUnits`), so the allowance is **25.4 metres**. A caster displaced further than
that along the sun ray is culled and its shadow vanishes.

### Why it bites the NEAR shadows specifically
`ext = max(cascade_radius, 1000)`, and GG's cascades are `{380, 950, 7500, 30000, 500000}` units
(`master_part1.cpp:344`):
| cascade | distance | radius (approx) | effective ext |
|---|---|---|---|
| 0 | 380 u (9.7 m) | small | **1000 u (25 m)** ← floor governs |
| 1 | 950 u (24 m) | ~500 u | **1000 u (25 m)** ← floor governs |
| 2 | 7500 u (190 m) | ~4000 u | 4000 u (100 m) — self-extends |
So the far cascades are fine and the near ones — where the player's own surroundings are shadowed —
get 25 m. That matches the report: it is nearby shadows that pop.

★★ **Corroboration that this is a missed rescale, not a design choice:** `wickedcalls_part3.cpp:1460`
already says of the very same function's inputs — *"light's cascade_distances, whose stock default
{8,80,800} is ~20 metres"* — and GG replaced them with inch-scale values. The project knew stock
Wicked constants are metre-scale and rescaled the cascade distances; **the culling extrusion inside
`CreateDirLightShadowCams` was missed.**

### Fix shape
Make the extrusion floor a GG-scaled, tunable value instead of the stock `2000.0f`. Something like
a `gg_shadow_caster_extrude` (units, default ~6000-8000 u = 150-200 m) used in place of
`min(2000, farPlane) * 0.5`, with a harness knob so the pop distance can be A/B'd directly.
★ Widening the CULL is the sanctioned fix, not a hack: the stock comment states depth clip is off
and out-of-projection casters clamp correctly, so the culling frustum is the only thing rejecting
them — exactly what it is there to control.
⚠ **COST**: a wider cull selects more casters → more shadow draw calls in the near cascades, which
are the most expensive. This MUST be measured (shadow pass ms + the 19-demo sweep), not assumed.
⚠ **NOT YET EMPIRICALLY CONFIRMED.** The mechanism and the arithmetic are solid, but "this is what
the user saw" is still an inference. The knob is also the test: set it high, and if the pop stops,
the diagnosis is proven. Do that before believing it (§22.7).

---

## D. Ragdoll death does nothing — characters T-pose and float — ★ ROOT-CAUSED, ONE COMMENTED LINE

**Observed (user):** the two death options are a canned animation or ragdoll; "the second... currently
does not work, and the characters simply perform a T pose and float there."

### First, a premise correction
The user expected Bullet3. **Both are true and they are different systems:**
- **Wicked's** physics is now **Jolt** (`wiPhysics_Jolt.cpp`; there is no `wiPhysics_Bullet.cpp`).
- **GG's own** ragdoll is still **Bullet**, entirely separate:
  `Dark Basic Public Shared/.../Shared/Bullet/` + `Include/BulletPhysics.H` + `Lib64/.../Bullet.lib`,
  driven from `M-Ragdoll.cpp` via the `BPhys_*` wrapper.
So GG's ragdoll does NOT depend on the Jolt migration. Nothing was lost there.

### What is intact
- The whole `Shared/Bullet/Ragdoll/` directory is present in DX12 with the same file set as DX11.
- `BPhys_RagDollBegin` (`BulletPhysics_part1.CPP:1742`) is **byte-identical** to DX11.
- `DBProMotionState.cpp` and `DBProRagdollManager.cpp` are **byte-identical** to DX11.
- `M-Ragdoll.cpp` is in the build (`Template_Windows.vcxproj:1690`), unguarded, and its call site
  `BPhys_RagDollBegin(...)` is at line 187 in BOTH trees.
So the ragdoll IS being created and Bullet IS simulating it.

### ★★ Root cause: the bone→limb WRITEBACK was commented out during the port
`Shared/Bullet/Ragdoll/DBProRagDoll.cpp:697` — the only meaningful diff against DX11:
```
DX11:  WickedCall_OverrideLimbWithCombined(pObject, pFrame, bIncludeTranslation);
DX12: //WickedCall_OverrideLimbWithCombined(pObject, pFrame, bIncludeTranslation); DX12
```
…together with its include, three lines up:
```
DX11:  #ifdef WICKEDENGINE
       #include ".\..\..\..\..\Guru-WickedMAX\wickedcalls.h"
DX12: //#ifdef WICKEDENGINE DX12
      //#include ".\..\..\..\..\Guru-WickedMAX\wickedcalls.h"
```
That call is what pushes each simulated bone's combined matrix onto the VISIBLE limb. With it
commented out, Bullet simulates a ragdoll nobody can see, and the character keeps whatever pose it
had when `ragdoll_create` ran `StopObject` — **a T-pose that does not move.** Exactly the report.

Same treatment in `DBProRagDollBone.cpp` (lines 15-17, 158-159, 174-175): the include and
`WickedCall_PresetObjectRenderLayer` / `WickedCall_PresetObjectCreateOnDemand` around the debug
bone capsules are commented with the same `DX12` marker. Those are debug-visual only, but they are
part of the same disabling pass.

### The target function is alive and maintained in DX12
`WickedCall_OverrideLimbWithCombined` is declared at `Guru-WickedMAX/wickedcalls.h:149` and
implemented at `wickedcalls_part2.cpp:1202`. Nothing needs porting — the fix is to restore the
include and the call.
⚠ **BUT it currently has ZERO live callers in DX12**, i.e. it is dead code and therefore
**untested on this renderer**. Restoring the call may expose follow-on work inside it (the animation
bridge changed a lot — see `GGAnimBridge.h:33`, which explicitly lists
"WickedCall_RotateLimb, OverrideLimbWithCombined, etc." as bridge-managed). Treat "uncomment and
it works" as the hypothesis, not the outcome.

### Fix shape
1. Restore the `wickedcalls.h` include + the `WickedCall_OverrideLimbWithCombined` call in
   `DBProRagDoll.cpp`.
2. Optionally restore the two `DBProRagDollBone.cpp` preset calls (debug capsules only).
3. Verify against the DX11 behaviour: shoot a ragdoll-flagged character and watch it fall.
⚠ **Cannot be harness-verified** — it needs a kill in test-game, which requires driving the player.
This one ends with the user's eye, like the 2.14 walk test.

---

## B. 2D HUD elements missing (grey box behind health) — NOT root-caused; leads + one false lead killed

**Observed (user):** "the 2D HUD elements such as the grey semi-transparent box behind the players
health score are missing and not drawn." Scoped by the user as investigation-for-future-work.

**Status: NOT root-caused.** Code reading alone did not converge, and the live repro needed
(enter test-game, look at the HUD, bisect the draw path) could not run — the 19-demo sweep owned
the only MAX instance all night. Recording the map so the next session starts from here rather
than from scratch.

### ⚠ FALSE LEAD, killed before it was reported
`CSpritesC_part0.cpp:1192` — the 4-argument overload is a completely empty stub:
```
DARKSDK void PasteSprite ( int iID, int iX, int iY, int iDrawImmediately ) { }
```
That looks damning. **It is not the cause.** DX11's copy (`CSpritesC.cpp:1187`) is byte-identical
and equally empty, and the overload has **zero callers** in the DX12 tree. Pre-existing dead code.
★ Checked before writing it up — this would have been the third false finding of the session had
it gone in unverified.

### Established
- The HUD is a **storyboard screen node**: `iHUDScreenNodeID = 13`
  (`M-GridEditB_part17.cpp:1264`, reset at `:1590`), so the elements are authored in the Screen
  Editor and are data, not hard-coded.
- The 3-arg `PasteSprite` (`CSpritesC_part0.cpp:1152`) is fully implemented and reaches
  `m_SpriteManager.DrawImmediate` — the generic sprite path is NOT stubbed.
- Loader/font/menu sprites do draw through it (`Common_part3.cpp:95/268/732…`), so 2D sprite
  rendering works in general. Whatever is broken is specific to the HUD screen's elements.
- HUD *entity* layers (`ishudlayer`, `hud_scanforhudlayers`, `hud_updatehudlayerobjects`,
  `M-HUD.cpp`) are a DIFFERENT system — 3D objects pinned to the camera (jetpack etc.). Do not
  confuse the two; the grey box is not one of these.

### Next steps (in the order I would take them)
1. **Repro and bisect live**, which needs test-game: does ANY screen-node HUD element draw, or
   only the box? If text draws and images do not, the split is image-loading vs sprite-drawing.
2. Find who consumes screen node 13 at runtime and whether it is invoked at all in test-game —
   grep outward from `iHUDScreenNodeID`. A node that is never walked would explain everything.
3. Check whether the HUD screen's images LOAD (a failed `loadinternalimage` would give a valid
   sprite with no texture — invisible, no error).
4. Compare against DX11 the same way §24/D was done: diff the screen-editor runtime files
   between trees and look for `DX12`-tagged comment-outs. That single technique found D in
   minutes and is the highest-yield next move.
⚠ Do NOT assume the sprite system is broken — it demonstrably is not (see Established).

---

# IMPLEMENTATION LOG (GGMAX 2.28)

All four were investigated first, as asked. Three had a root cause solid enough to act on; B did
not, and was left as notes rather than a speculative change.

| | issue | root cause | action |
|---|---|---|---|
| A | smoke flume loops forever | WPE decal emitter never told to stop | **FIXED** — action 7 on expiry |
| B | HUD grey box missing | not found | **notes only** (+1 false lead killed) |
| C | shadow pops when caster exits view | metres-scale cull constant in an inch world | **FIXED + knob** |
| D | ragdoll T-pose/float | bone→limb writeback commented out in the port | **FIXED** (1 line + 1 decl) |

### A — `M-Decal.cpp`, WPE decal expiry
Issue `WickedCall_PerformEmitterAction(7 /*SetEmitPaused(true)*/, emitterid)` before abandoning
the emitter. Chosen over pause(2)/hide(6) so live particles finish their lifetime and the flume
**dissipates** instead of snapping out. The emitter is NOT deleted — it belongs to the
`ready_decals` round-robin and must survive for reuse.

### C — `wiRenderer.cpp` / `wiRenderer.h`, engine
New `wi::renderer::gg_shadow_caster_extrude` (default **4000 u ≈ 101 m**, was an effective
1000 u = 25 m) replaces the stock floor inside `CreateDirLightShadowCams`' culling extrusion.
`0` restores stock exactly. Live harness knob **`SET_SHADOWEXTRUDE <units>`**.
⚠ Only lifts the NEAR cascades — `ext` still wins wherever the cascade is naturally larger, so
cascades 2+ are untouched.

### D — `DBProRagDoll.cpp`, Bullet ragdoll
Restored `WickedCall_OverrideLimbWithCombined(...)`. The symbol is **forward-declared** rather
than re-enabling the commented `#include`, for two reasons: the commented path climbs 4 directory
levels but this file sits one deeper than the sibling that uses 4 (it needs 5), and pulling
`wickedcalls.h` into a Bullet translation unit mixes two libraries' math types — very likely the
real reason the port dropped it. `sObject`/`sFrame` come from `CObjectsC.h`, already included.

## ⚠ VERIFICATION STATUS — read before trusting any of this
Everything above is **built-and-reasoned, not yet proven**. The 19-demo sweep owned the only MAX
instance for the whole investigation window, so nothing here has been run.
- **A** — needs a shot at the floor; watch the flume fade instead of loop. Harness cannot fire a
  weapon, so this ends with the user's eye.
- **C** — needs someone standing where a shadow pops. **`SET_SHADOWEXTRUDE` is the test as well as
  the fix**: if raising it stops the pop, the diagnosis is confirmed; if not, the mechanism is
  wrong and the change should be reverted, not tuned. ⚠ Cost must come from the shadow pass + a
  hub sweep before this default ships.
- **D** — needs a ragdoll-flagged character killed in test-game. ⚠ The restored function has
  **zero other live callers in DX12**, so it is untested on this renderer; "uncomment and it
  works" is the hypothesis, not the result.

---

# C — COST MEASUREMENT, and a measurement trap worth more than the result

`tools/sceneupdate/shadowextrude.sh`, Island Showdown, three arms in ONE launch
(STOCK 0 → NEW 4000 → **STOCK again** as the drift control).

### Attempt 1 (no caster counter) — INCONCLUSIVE, and it would have read as a win
| arm | mean shadowmap ms |
|---|---|
| STOCK_1000 | 1.478 |
| NEW_4000 | 1.124 |
| STOCK_REPEAT (identical to arm 1) | 0.856 |
The two identical arms differ by 0.62 ms and the new setting lands BETWEEN them. Without the
repeat arm this reads "the change made shadows 24% FASTER", which is nonsense. **Drift exceeded
the effect; nothing could be concluded.**

### The missing instrument
`VISIBLE_OBJECTS` is blind here — it counts CAMERA visibility, and this change alters CASTER
selection. So there was no evidence the cull had widened at all: exactly the failure that voided
gate run 2 of the decal pool (§24.2). Added **`SHADOW_CASTERS: <n> extrude=<u>`** to
`GET_PERF_DATA`, counted where an object survives the cascade cull.

### Attempt 2 (with the counter) — two real findings
| arm | casters | shadowmap ms |
|---|---|---|
| STOCK_1000 | 43 ×4, **856** ×1 | 0.65–0.89, **1.47** |
| NEW_4000 | **44** ×5 | 0.71–1.00 |
| STOCK_REPEAT | 43 ×2, **856** ×2 | 0.68–0.75, **1.55 / 1.89** |

**1. Executed-check PASSES: 43 → 44.** The knob genuinely reaches the cull. But it is only **+1
caster** on Island Showdown, so the cost here is negligible — and this level is a weak test
(an island; few tall casters, no low-sun canyon geometry). Do not generalise "free" from it.

**2. ★★ The "noise" was not noise — it is the STAGGERED CASCADE REFRESH.** Caster count is
bimodal, **43 or 856**, and every high-ms sample is an 856 frame. Most frames gather only the
cascades due for refresh (`ggDelayedShadows` / `delayedShadowState.update[cascade]`); periodically
all of them refresh. My three arms happened to catch 1, 0 and 2 full-refresh frames — which is
the entire explanation for "stock is slower than stock".
★ **RULE: any shadow-pass A/B on this engine must control for the refresh stagger**, e.g. by
bucketing samples on `SHADOW_CASTERS` before averaging, or by sampling long enough to average the
phase out. A five-sample mean measures the refresh phase, not the change.

**Verdict on C's cost: cheap on this level (+1 caster), and NOT yet measured on a level that
actually stresses it.** The hub sweep is the broader check; the pop itself still needs the eye.

---

# 2.28 hub sweep — CLEAN
`tools/sweep_0812b_2.28.txt`. **C1 PASS** 19/19 · **C3 PASS** worst 3768.8 MB in-game, headroom
**327.2 MB** (2.27 was 327.9 — flat) · **C4 PASS** 19/19 reached gameplay.
**C2: POLYS bit-identical to 2.27 on all 19 demos.** The single mismatch `sweepgate` prints is the
same Aztec Teaser −1 872 that 2.25 introduced and §22.8 cleared; its C2 reference is the stale
0809 sweep.
★ POLYS being unchanged is the RIGHT result for these three fixes, and worth stating as a check
rather than a shrug: A stops an emitter (no main-pass geometry), C alters shadow-CASTER selection
(not the camera pass), D only runs when a ragdoll is created. Any POLYS movement here would have
meant one of them was doing something unintended.

# What the user needs to do (none of this is harness-verifiable)
| | how to test | instant revert |
|---|---|---|
| A | shoot the floor; the flume should thin out and vanish instead of looping | — (revert = rebuild) |
| C | stand where a shadow pops as its caster leaves view; `SET_SHADOWEXTRUDE 0` vs `4000` | `SET_SHADOWEXTRUDE 0` / `setup.ini shadowextrude=0` |
| D | kill a ragdoll-flagged character in test-game; it should collapse, not T-pose and float | — (revert = rebuild) |
⚠ For C the knob is BOTH the fix and the experiment. If raising it does not stop the pop, the
diagnosis in section C is wrong and the change should be **reverted, not tuned** — the mechanism
would be something else entirely.

---

# ★ USER VERIFICATION 2026-08-12 (played the 2.28 build)

| | user's verdict |
|---|---|
| **A** | **PARTIAL — fix works, but I introduced a REGRESSION.** "the flume now dissipates after a while but now after I get 5 impacts I do not get any more as though I have used them all up. Other impacts (say stone) work but shooting the ground I only get 5 now." |
| **C** | ✅ **CONFIRMED FIXED** — "looks good, no surprise shadow pops." The §22.7-style diagnosis held: it really was the metres-in-inches cull constant. |
| **D** | ❌ **DID NOT WORK** — still broken after the writeback restore. |
| **B** | Parked by the user: "its root cause is further back in the screen editor." |

## ★★ A REGRESSION — root-caused, my fault, one line to fix
`SetPaused` and `SetEmitPaused` are **two different flags**, and the reuse path only clears the
first. The fire/reuse sequence (`M-Entity_part5.cpp:262-265`) is:
```
3 = SetPaused(false)  "Resume"
4 = Restart()
5 = SetVisible(true)
1 = Burst(0)
```
**There is no action 8 (`SetEmitPaused(false)`) anywhere in that path** — the only action-8 call in
the whole codebase is `M-Weapon.cpp:1372`, for weapon trails. So my expiry fix sets
`SetEmitPaused(true)` and *nothing ever clears it*. Once all 5 cached emitters for a decal type
have each expired once, all 5 are permanently emit-paused → that decal type stops appearing.
Exactly the report: ground dies at 5 (its cache cycled), stone still works (its cache has not).

**FIX (do this first after the compact):** add `WickedCall_PerformEmitterAction(8, ...)` to the
fire sequence, before Restart, so a reused emitter is un-emit-paused as well as un-paused.
⚠ Put it in the FIRE path, not the expiry path — the expiry pause is what makes the flume
dissipate and must stay.
★ Lesson: "Resume" (3) reads like it resumes everything and does not. The weapon-trail code knew
to pair 7 with 8; the decal path did not, and I copied the half that suited me.
⚠ My hitch/burst gate could never have caught this — `DECAL_BURST` exercises allocation, not the
emitter state machine across a full expire→reuse cycle. A repro needs N+1 impacts of ONE decal
type with expiry in between.

## D — still broken; what the restore did and did not prove
The writeback restore was necessary but not sufficient. It compiled, linked and shipped, so the
symbol resolves — but the character still T-poses and floats. That eliminates "the call is
missing" and leaves the stages I never verified:
1. **Is `ragdoll_create` even reached?** The death path is `G-Entity_part2.cpp:386`, gated on
   `t.entityprofile[].ragdoll == 1`.
2. **Does it bail before building anything?** `M-Ragdoll.cpp:140-147` requires
   `getlimbbyname(obj,"Bip01_Pelvis")` AND `"Bip01_Spine")` to both resolve, trying prefixes
   `Bip01` then `Bip002`. A DX12 importer that names limbs differently = silent no-ragdoll.
   ⚠ Note this branch also contains the `StopObject` — so a bail leaves the character animating,
   NOT T-posed. The observed T-pose therefore suggests the branch IS entered. Worth confirming.
3. **Is the writeback loop reached?** It is gated on `pbFramePartOfBoneMovement[iF] == true`.
4. **Does `WickedCall_OverrideLimbWithCombined` do anything on this renderer?** It had ZERO other
   live callers before this change, so it has never run in DX12. `GGAnimBridge.h:33` lists it as
   bridge-managed and the animation bridge changed substantially in the port.
★ **Next move is an instrument, not another guess** — a `DUMP_RAGDOLL` reporting: ragdoll_create
calls, which prefix matched, pelvis/spine limb ids, bones added, and writeback invocations per
frame. That names the dead stage in one run instead of bisecting four hypotheses. Same approach
that settled §23 and the shadow caster count.

## B — parked by the user
Root cause is upstream in the screen editor. Leads in section B stand; the recommended technique
(diff screen-editor runtime vs the DX11 tree for `DX12`-tagged comment-outs) is unchanged.

---

# GGMAX 2.29 — the A regression fixed, and D instrumented instead of guessed

## A — FIXED: the fire path now clears BOTH pause flags
`M-Entity_part5.cpp:262` (the WPE fire/reuse sequence) gains one line:

```
3 = SetPaused(false)      // was already here
8 = SetEmitPaused(false)  // GGMAX 2.29 — NEW
4 = Restart
5 = SetVisible(true)
1 = Burst(0)
```

Verified against the engine before writing it: `Restart()` does **not** touch `FLAG_EMIT_PAUSE`
(`wiEmittedParticle.h:204` — only an explicit `SetEmitPaused(false)` clears it), and
`wiEmittedParticle.cpp:398` is the single place the flag is read. So nothing else in the reuse
sequence could have cleared 2.28's expiry pause, which is why the fifth impact of a decal type
killed it permanently.
The expiry `SetEmitPaused(true)` at `M-Decal.cpp:996` is **kept** — that is what makes the flume
dissipate rather than loop forever, and it is now correctly paired, exactly as the weapon-trail
path has always paired 7 with 8 (`M-Weapon.cpp:1693` / `:1372`).
★ Reuse carries `bParticle_Fire = true` and the cached `emitterid` (`M-Decal.cpp:657-664`), so the
fire block genuinely runs for a recycled emitter — checked, not assumed.

## D — instrumented, and the chain turned out to have THREE more dead links
`DUMP_RAGDOLL` (2.29) reports every stage: CREATE → UPDATE → WRITEBACK, plus a bone read-back
and a survival counter. Driven by `RAGDOLL_TEST [entity]`, which ragdolls a character on demand
because the harness cannot make the player shoot one. Measured on Escape from the Zombie Cellar,
in test game (ragdolls only update there — `ragdollManager::Update()` runs off the Bullet step).

### The four stages I could not previously verify are ALL alive
```
RAGDOLL_CREATE: calls=1 obj=70002 frames=67 prefix=Bip01 pelvis=2 spine=3 existed=0
                produced=1 limbsTagged=66
RAGDOLL_UPDATE: calls=322 bones=13 framesMoved=66 animateFrom=1 writebackCalls=20930
RAGDOLL_WB:     entered=20930 ... channel=13524 applied=13524 splitTarget=0
```
So: create IS reached, the Bip01 gate DOES open, 66 limbs are tagged onto 13 bones, Bullet IS
driving them, and the writeback IS called. Every hypothesis in the 2.28 list was wrong.

### ★ Dead link 2 — the function 2.28 restored the call to was itself hollow
`WickedCall_OverrideLimbWithCombined` computed the pose and then applied it through two
`GGAnimBridge_SetPreFrame` lines that were commented out (`////`) — as is **every other
PreFrame call site in the file**. 2.29 replaces them with a single merged call (DX11 sets the
POS and ROT channels separately; the DX12 bridge keys preframes by `channel->target`, and both
channels of a bone share one target, so enabling the two lines as written would have had the
second overwrite the first — rotation applied with an identity translation, mode 2 lerping every
bone toward its parent's origin. Probably why they were disabled rather than fixed.)
`splitTarget=0` on every run confirms the merge is valid.

### ★ Dead link 3 — the preframe CONSUMER is disabled too
`GGAnimBridge_PostUpdate`, the only reader of that preframe map, is commented out in
`MasterRenderer::PostUpdate` (`master_part1.cpp:617`, disabled with PreUpdate in 89873913 as
"too slow"). Routing through the bridge would have applied nothing while the counter happily
read "applied" — so 2.29 writes the bone's local transform directly from the writeback, which
runs in the game-logic phase and therefore lands before that frame's `Scene::Update`.

### ★★ Dead link 4 — and the animation clip took the pose straight back
With the direct write in, the first read-back said **HELD** — and that was an artefact: the
harness runs inside `GuruLoopLogic`, the same phase as the Bullet step, so the dump was reading
its own write. The phase-independent counter (compare the tracked bone at the NEXT writeback)
convicted it:
```
clobbered=225 survived=0     wroteT=(16.79,7.52,8.03) vs liveT=(0.70,33.36,2.85)
```
Cause: `DBProRagDoll::Update` ends with `LoopObject(objectID, 0, 1)` — "must keep object playing
so modified bones can affect the wicked animation system". True on DX11, where the override was
applied *inside* animation evaluation. On DX12 a running clip just re-samples over the bone every
frame. 2.29 skips that re-play (create already stopped the clip), gated on the same knob.

### Result, same instrument, same scene, same session
| | clobbered | survived | tracked bone |
|---|---|---|---|
| clip looping (2.28 behaviour) | **225** | 0 | dT=47.11 dR=1.59 OVERWRITTEN |
| clip stopped (2.29) | **0** | **249** | dT=0.0000 dR=0.0000 HELD |

⚠ **What this does NOT prove: that it LOOKS right.** The plumbing now delivers the ragdoll pose
to the mesh and it survives; whether the resulting pose is correct is the user's eye. A visual
check could not be automated — the ragdolled body is never in frame at the player spawn,
`SET_CAMERA` is editor-only (`ERROR: SET_CAMERA only works in the level editor`), and the harness
cannot drive the player. The first attempt at a screenshot A/B produced two 0.018%-different
images of an empty cellar, which is not evidence of anything.
⚠ Also unproven: whether writing translation on the root only (2.29 diverges from DX11 here, see
the code comment) is right. If the corpse looks stretched or pinned, that is the first suspect.

### Method notes worth keeping
- ★ **A read-back sampled in the same phase as the write can only ever agree with itself.** HELD
  was a false pass; the next-writeback comparison is what settled it. Any "did my value stick"
  check needs to observe from a different phase than the one that wrote it.
- ⚠ The first version of that counter tracked "the last bone written" — a different bone every
  call — so it never compared anything and reported a clean-looking `0/0`. Latching one bone
  fixed it. **A survival counter that never fires looks exactly like a survival counter that
  always passes.**
- ⚠ `ragdoll_create` needs the ambient global `t.tobj` set to the same object as `t.tphyobj`: it
  reads limbs from one and writes them back with `RotateLimbQuat(t.tobj, ...)`. Both live death
  paths happen to set it (`G-Entity_part1.cpp:30`, `G-Entity_part2.cpp:79` — both checked, NOT a
  live bug), but omitting it in the harness crashed MAX in `AnglesFromMatrix`.
- ⚠⚠ **Three runs were lost to overlapping script instances**, after I deleted a lockfile to
  "unstick" a run. Leaked runners truncate and interleave the shared log, and the result reads as
  a plausible failure of the thing under test rather than of the harness. Same lesson as
  2026-08-11, learned again the same way: never remove the lock, kill the runner.

---

# GGMAX 2.30 — the invisible collectable pistol (TESTPRO2 / spotshadowtest)

Shipped: `SET_OCCLUSION <0|1>` and `WHYNOTDRAWN <name-substr>` (closes backlog #125).
`WHYNOTDRAWN` walks every gate `UpdateVisibility` + `DrawScene` + `RenderMeshes` apply to the main
camera, in engine order, and names the first one that rejects the object.

## The one hard, reproducible finding: pickups carry a 2 km bounding box
```
W_MK19T (pistol)  aabb=(-700,42,153)-(100001,100001,100001) center=(49650,50021,50077) r=86750
44ammo  (pickup)  center=(50103,50026,49319) r=86926
candle_holder     center=(-679,48,167)       r=7.9     <- healthy prop
Rectangle001      center=(-130,7,-413)       r=28.5    <- healthy prop
```
Clean class split: every PICKUP is corrupt, every ordinary prop is fine — which is why the level
shows THREE orphan shadows (pistol + two ammo pickups), not one.
★ Where the number comes from, arithmetically: `SET_ENTITY_VIS` reports TWO objects named
`W_MK19T` — the placed pickup, and the hidden library master parked at (100000,100000,100000)
(`M-Entity_part0.cpp:1468`). Midpoint of those two = (49654,50021,50080), half-diagonal = 86745,
against measured (49650,50021,50077) / 86750. The placed pickup's bounds are the UNION of itself
and the parked master. Its transform is untouched (`rawT`==`compT`==true position) — which is
exactly why the shadow lands correctly.

## ⚠⚠ THE AABB IS NOT THE CAUSE. Four mechanisms proposed, four killed.
| # | mechanism | killed by |
|---|---|---|
| 1 | lazy object PSO | live A/B: `lazypso=0`, still invisible |
| 2 | apparent-size cull (delta 1.30) | live: `SET_APPARENTSIZE 0`, still invisible. Arithmetic agrees — the cull's own `distSq > radiusSq` guard cannot even be entered when radius≈distance |
| 3 | occlusion query false-negative | `WHYNOTDRAWN`: `occlusion=visible(hist=ffffffff)`, `inVisibleSet=yes` |
| 4 | distance-fade dither skip (`wiRenderer.cpp:4170`) | `WHYNOTDRAWN`: `fadeDistance=3.4e38` (FLT_MAX) so dither=0.0000 |

Final state for the pistol: `renderable=1 layerOk=1 frustum=PASS apparentCull=pass
occlusion=visible inVisibleSet=yes drawReject=no dither=0.0000` — **it passes every CPU gate in
the engine and is in the camera's visible set, and still does not appear.**

★★ METHOD NOTE WORTH MORE THAN THE RESULT: I announced mechanism 3, then mechanism 4, and the
instrument killed both within minutes of being built. Both were coherent, both were quantitatively
consistent with the corrupt AABB, and both were wrong. **A measurement that names the culprit
class (here: pickups have broken bounds) does not license a story about what that class does
downstream.** Build the gate-by-gate instrument BEFORE narrating the mechanism, not after.

## Where the fault must now be
Downstream of the CPU visibility path — inside `RenderMeshes` or in the pixels:
* a per-SUBSET reject (material filter mask) rather than per-object;
* a missing PSO VARIANT for `RENDERPASS_MAIN`/`PREPASS` while `RENDERPASS_SHADOW` exists — note
  this is independent of `lazypso` (a never-built variant is skipped outright);
* ★ the amplifier: the opaque main pass is depth-compare EQUAL with depth-write OFF
  (`wiRenderer.cpp:2937-2940`, `:2449`), so losing ONLY the Z-prepass draw yields exactly
  "no mesh, perfect shadow". The per-OBJECT reject lists of prepass and main are provably
  identical, so any such loss must be per-subset or per-PSO-variant.
* ⚠ Still unexplained by any of the above: the click repair. The one draw-level change selection
  makes is forcing an INSTANCED-BATCH BREAK — and the placed pickup shares mesh 2821 with the
  parked master, so the two are instancing candidates. That is the next thread to pull.

## Next instrument (do this before theorising again)
Extend `WHYNOTDRAWN` past the queue into the batch: per-subset filter masks vs the pass filter,
whether a PSO exists for MAIN/PREPASS/SHADOW for that material's variant, and which instanced
batch the object landed in (with its instance count and the other members). If the pistol is
batched with the parked master, that is the answer.

## 2.31 — the draw tracer, and how far it narrowed the pistol
Engine-side per-MESH tracer inside `batch_flush` (`wiRenderer.cpp`), armed by `WHYNOTDRAWN`:
batches / instances / subsets / draws / nofilter / nopso / nobuffer, per RENDERPASS. Zero cost
when `gg_dbg_watch_mesh` is -1. Answers "was the draw issued?" from the only place that knows.

| object | MAIN | PREPASS |
|---|---|---|
| candle_holder (visible control) | batches=1142 draws=1142 | batches=1142 draws=1142 |
| **W_MK19T (the pistol)** | **0** | **0** |

Zero batches in EVERY pass — the mesh never reaches `batch_flush` at all. And yet, measured on the
same frames, the object:
* is in `visibility_main.visibleObjects` (`inVisibleSet=yes`);
* passes renderable / layerMask / frustum / apparent-size / occlusion / draw-distance / dither;
* has `notVisibleInMainCamera=0 foreground=0 filterMask=00000001 notInReflections=0` — byte for
  byte the same as the control that draws 1142 times;
* has `mesh_index` (the renderer's batch key) == `meshes.GetIndex(meshID)` == 195, so the tracer
  is watching the right mesh and the key is not stale.

★ The fault is therefore inside a very small gap: between `renderQueue.add(...)`
(`wiRenderer.cpp:8668`) and `batch_flush`. Every input on both sides is measured and matches a
working control. ⚠ NOT CLOSED — do not narrate a mechanism for this gap without measuring it.

### Next step, and it is one counter
Add a tracer hook at the `renderQueue.add` call itself (count insertions for the watched mesh, per
pass). That splits the remaining gap in two: if insertions are 0 the object is being dropped by a
DrawScene reject that is NOT one of the eight already printed; if insertions are non-zero and
batches are still 0, the loss is in the queue sort/flush and the batch key is a lie.

### ⚠⚠ A SECOND instance of the same instrument bug in one day
The tracer's first run printed zeros for the pistol. It also printed zeros for a control object
that was plainly on screen — which is the only reason it was caught. Cause: `armedMesh` was a
LOCAL, so every invocation re-zeroed the counters microseconds before printing them.
This is the 2.29 latch bug again in a new costume. **RULE: a counter that is re-zeroed on read is
indistinguishable from a counter that never fires — and both look like a finding. ALWAYS arm a new
tracer on a KNOWN-GOOD object first and require it to show non-zero before trusting a zero.**
