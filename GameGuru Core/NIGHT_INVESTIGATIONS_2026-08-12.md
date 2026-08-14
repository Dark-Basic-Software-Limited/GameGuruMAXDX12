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

---

# 2.32 — ★★★ THE PISTOL IS SOLVED: a half-float distance overflowed to infinity

## The chain, every link measured
1. A placed pickup's world AABB is the UNION of itself and the hidden library master parked at
   (100000,100000,100000) — arithmetic in the 2.30 section. Its CENTRE therefore sits **86,730
   units** from the camera while the object itself is 1 m away.
2. `RenderBatch` stores the queue distance as a **HALF** (`wiRenderer.cpp:474`, `:483`
   `XMConvertFloatToHalf`). A half's largest finite value is **65504**. 86,730 → **+INF**.
3. `RenderMeshes` computes `dither = max(transparency, max(0, GetDistance() - fadeDistance) / radius)`
   (`:4219`). `INF - FLT_MAX` is INF, so dither = INF, `dither > 0.99f` fires, `continue`.
4. The instance is therefore never counted, `instancedBatch.instanceCount` stays 0, and
   `batch_flush` returns at its first line. **No error, no warning, no draw.**
5. The SHADOW pass passes distance 0, so half(0) = 0, dither = 0 — the shadow renders perfectly.

## The measurement that located it
The 2.32 queue-build tracer made the two halves disagree, and the contradiction was the whole clue:
```
PISTOL   MAIN QUEUE seen=4572 ... ADDED=1143      MAIN batches=0    draws=0
CONTROL  MAIN QUEUE seen=4368 ... ADDED=1092      MAIN batches=1092 draws=1092
```
Added to the queue 1143 times,zero batches at the flush. Only one filter sits between those two
points, and it is the dither.

## Fix and result
`RenderBatch::Create` now clamps: `XMConvertFloatToHalf(std::min(distance, 65504.0f))`. Clamping
rather than widening the field keeps RenderBatch at 16 bytes and its `static_assert` intact; an
object sorting as "65504 away" is harmless.
| | MAIN batches | MAIN draws | on screen |
|---|---|---|---|
| before | 0 | 0 | pistol + 2 ammo pickups missing, shadows present |
| after | **1088** | **1088** | **all three render** |

## ⚠ Two lessons, both expensive
★★ **This is the SECOND fp16-range bug on this project** (2.07g was the light-attenuation range²
overflow). GameGuru is INCH-scale: 65504 units is ~1.66 km and trivially reachable. Upstream
Wicked is metres, where the same constant is 65 km and effectively infinite. **When porting a
metres-scale engine to an inch-scale game, every half-precision distance is a range bug waiting
to happen** — audit them as a class, not one at a time.
★★ **An instrument that mirrors engine logic must mirror its PRECISION.** WHYNOTDRAWN computed
the dither in full float and printed `dither=0.0000 → DRAWN` for an object the engine was
skipping with INF. It agreed with my reasoning rather than with the renderer, and it cost a whole
extra hypothesis cycle. It now does the half round-trip.

## What is still NOT fixed (deliberately)
The corrupt AABB itself. The clamp makes the symptom impossible for every object at any distance,
but pickups still carry a 2 km bounding box, which degrades culling quality (they can never be
frustum-culled) and would keep biting any other code that trusts object.center/radius.
★ That is a GAME-side defect in how a placed collectable's bounds absorb the parked master, and
it is the right next target. It is now cosmetic-to-performance rather than a visible bug.

## Sweep criteria, fixed BEFORE the 2.32 run (tag 0813)
Amending in writing beforehand, per the C2′ discipline — after the run it is rationalising.
* **C1 loads** — 19/19 reach editor and test game. Any crash or hang FAILS.
* **C2″ POLYS — the identity gate is SUSPENDED and replaced, deliberately.** 2.32 makes objects
  draw that the engine was wrongly skipping, so POLYS may legitimately **INCREASE**. An increase
  passes only if it is attributable (a demo with pickups/props whose centre distance exceeded
  65504). A **DECREASE fails** — nothing in 2.32 can remove geometry. Unchanged also passes: most
  demos have no object far enough out to have overflowed.
* **C3 VRAM** — both columns under 4096 MB, and no demo more than +100 MB against the 2.25
  baseline without attribution. Newly-drawn objects cost VRAM only if their materials were not
  already resident, so a large rise would be a surprise worth chasing.
* **C4 FPS** — no demo worse than −10% vs the 0806 baseline, judged against the known ±8 FPS
  launch variance and the lazy-PSO warm-up caveat (levels gain 10-12% by 180 s).
⚠ The sweepgate C2 reference is still the stale 0809 sweep and will keep flagging Aztec Teaser —
read its POLYS verdict manually against C2″ above, do not trust its pass/fail line.

## 2.32b — the fp16 CLASS audit the lesson demanded (read-only, no code changed)
Having been bitten twice, I swept every `XMConvertFloatToHalf` in the engine for distance-like
values rather than waiting for the third.

| site | what is packed | worst authored value | headroom to 65504 |
|---|---|---|---|
| `wiRenderer.cpp:483` `RenderBatch::Create` | queue sort/fade distance | **86,730 measured** | ★ **OVERFLOWED — fixed in 2.32** |
| `ShaderInterop_Renderer.h:1017` `SetRange` (light) | light range | authored, editor-bounded | not reached in shipped content |
| `ShaderInterop_Renderer.h:1017` `SetRange` (env probe) | probe range | **50,000** (`GGTerrain_part0.cpp:7471`, `:9285`, `:9710` `globalrange`) | **only 1.31× — 24% margin** |
| `:1021/:1025` `SetRadius` / `SetLength` | light radius, capsule length | small by construction | safe |
| `:1040-1053` direction / cone cos | normalised | ≤1 | safe by construction |
| `wiMath.h:555/564` pack_half2/3 | UVs, normals, colours | ≤1-ish | safe |

**Verdict: no second live overflow, but the env-probe range sits at 50,000 against a 65,504
ceiling.** One authoring change — a global probe range raised past ~1.3× — turns that into +INF
and, unlike the pistol, an INF light/probe range fails LOUD (infinite influence), not silent.
⚠ I did NOT pre-emptively clamp those packers: they are upstream code, no bug is observed, and
clamping a legitimately huge value silently changes lighting. Recording the margin is the right
output; the decision to clamp is the user's.
★ For the record, the general rule now has a number attached: **in an inch-scale world a half
tops out at 1.66 km, and 1.66 km is an ordinary distance in a GameGuru level.**

## 2.32c — ★ and the corrupt AABB itself is root-caused too (read-only; no code changed)
`wiScene.cpp:5231-5241`, the object AABB update:
```cpp
aabb = mesh.aabb.transform(W);
if (mesh.IsSkinned() || mesh.IsDynamic())
{
    const ArmatureComponent* armature = armatures.GetComponent(mesh.armatureID);
    if (armature != nullptr)
        aabb = AABB::Merge(aabb, armature->aabb);      // ← merges a WORLD-SPACE armature box
}
```
The armature is resolved from **`mesh.armatureID` — a property of the MESH, not of the object** —
and `armature.aabb` is built in WORLD space from bone positions (`wiScene.cpp:4667`).

The placed pickup and the parked library master **share mesh 195** (both are `W_MK19T`; FIND_OBJECT
reports the same `mesh=2821` for both). They therefore resolve the **same armature**, whose world
box sits wherever that armature's bones are — at the master's park position
(100000,100000,100000). So the placed pickup merges a box 100 k units away into its own bounds.
**That is precisely the measured union**, and it explains the class split exactly: pickups are
weapon/ammo models and are SKINNED; `candle_holder`, `Rectangle001`, `Cube.002`, `plane` are
static props and take only the `mesh.aabb.transform(W)` line, which is why their radii are 8-29.
(`Body` is skinned but reads a correct r=32.7 — its armature is its own and co-located, the
control that proves the mechanism needs a SHARED armature, not merely a skinned mesh.)

### Two candidate fixes, neither applied — this needs the user's call
* **Game side (preferred):** stop keeping the library master as a live skinned object parked at
  100000. Either drop its skinned/dynamic state once it is hidden, or park it somewhere near the
  play area — parking at, say, (0,-5000,0) caps the union at a few thousand units and can never
  approach the 65504 half ceiling again.
* **Engine side:** only merge the armature box when the armature belongs to THIS instance.
  Upstream assumes one armature per skinned instance, which GG's clone-from-a-parked-master idiom
  violates. Riskier: it is upstream code on the hot path and every skinned object pays the test.
⚠ Not urgent now the 2.32 clamp makes the symptom impossible. Remaining cost is culling quality:
pickups can never be frustum-culled, so they are shaded on every camera in every level.

## 2.32 SWEEP (tag 0813) — FINAL, 19/19, scored against the pre-registered criteria
**C2″ POLYS — the headline, and it is bigger than the pistol.** Against the 2.28 baseline:
| | demos |
|---|---|
| POLYS **increased** | **11** |
| unchanged | 2 (Aztec Game Kit, Switch Escape) |
| **decreased (the FAIL condition)** | **0** |

```
Aztec Game Kit Teaser     10311639 -> 10330135   +18,496  (+0.18%)
Bounty                      463210 ->   469906    +6,696  (+1.45%)
Horseshoe Bend             2105365 ->  2168281   +62,916  (+2.99%)
Island Showdown            4114598 ->  4125704   +11,106  (+0.27%)
Operation Amazon           5496922 ->  5504271    +7,349  (+0.13%)
★ River Raiders            1906072 ->  2362345  +456,273 (+23.94%)
Snowy Mountain Stroll        81081 ->    81369      +288  (+0.36%)
A Grand Canyon Adventure   2272361 ->  2279506    +7,145  (+0.31%)
Disruption                 4665184 ->  4677579   +12,395  (+0.27%)
Foggy Forest              10195894 -> 10220589   +24,695  (+0.24%)
Indian Strike Force        3184527 ->  3229699   +45,172  (+1.42%)
```
★★ **This bug was suppressing real geometry on ELEVEN OF THIRTEEN SHIPPED DEMOS, and had been
doing so through every previous sweep.** POLYS was bit-stable at those old values across 2.25,
2.27 and 2.28 — which is exactly why the identity gate never flagged it: the geometry was
*consistently* missing. **A stable wrong number looks identical to a stable right one.**
River Raiders at **+23.9%** is the extreme case — nearly a quarter of its geometry was being
skipped. Attribution is sound: the mechanism is proven live on spotshadowtest, the direction is
predicted, and nothing in 2.32 can add geometry that was not already in the scene.

**C3 VRAM — PASS so far.** Worst in-game 3752.8 MB (Aztec Game Kit) against the 4096 gate.
No demo rose more than ~100 MB against 2.28; several fell.

**C4 FPS — PASS on the only side that is trustworthy.** No demo regressed more than 3%
(worst: Aztec Teaser and Foggy Forest at −3%). ⚠ **Several demos read +10 to +19% and I am NOT
claiming that as a win** — this rig's documented launch-to-launch swing is ±8 FPS, these are
cross-launch numbers, and "we draw 24% more geometry and got faster" is not a credible causal
story. The gate is the downside, and the downside is clean.

### FINAL SCORE — all 19 demos, all four criteria PASS
```
C1 LOADS   19/19 OK, no crash, no hang
C2" POLYS  increased=16   unchanged=3   DECREASED=0   (0 is the fail condition)
C3 VRAM    worst in-game 3752.8 MB (Aztec Game Kit) against the 4096 gate -> PASS
C4 FPS     worst delta -8% (Z Island in-game) against the -10% gate      -> PASS
```
| demo | POLYS delta | % | demo | POLYS delta | % |
|---|---|---|---|---|---|
| River Raiders | **+456,273** | **+23.94%** | Canyon Offensive | +21,845 | +0.25% |
| Horseshoe Bend | +62,916 | +2.99% | Aztec Teaser | +18,496 | +0.18% |
| Indian Strike Force | +45,172 | +1.42% | Z Island | +18,155 | +2.58% |
| Foggy Forest | +24,695 | +0.24% | RPG Template | +12,624 | +0.39% |
| Disruption | +12,395 | +0.27% | Island Showdown | +11,106 | +0.27% |
| Operation Amazon | +7,349 | +0.13% | A Grand Canyon Adventure | +7,145 | +0.31% |
| Bounty | +6,696 | +1.45% | Jungle Fever | +2,033 | +2.74% |
| Trapped | +1,559 | **+13.91%** | Snowy Mountain Stroll | +288 | +0.36% |
| *unchanged:* Aztec Game Kit, Switch Escape, Zombie Cellar | | | | | |

★★★ **16 of 19 shipped demos were losing geometry to this bug, and had been through every prior
sweep.** POLYS sat bit-stable at the wrong value across 2.25, 2.27 and 2.28 — which is exactly why
the identity gate never fired. **A stable wrong number is indistinguishable from a stable right
one; identity gates prove NO CHANGE, never CORRECTNESS.**
The three unchanged demos are the control: nothing in 2.32 invents geometry, so a level with no
object beyond the half ceiling reads identical, and three did.

### ⚠ Honest reading of the FPS column
Sixteen demos read **+1 to +19%** and I am **not** claiming that. Cross-launch variance on this rig
is a documented ±8 FPS, these are cross-launch numbers, and "we draw 24% more geometry and got
faster" has no mechanism. The trustworthy half of C4 is the downside, and it is clean.
★ **The one number that deserves follow-up is Z Island at −8% in-game** — it also gained 18,155
polys (+2.58%), so unlike the positive swings that delta has a plausible mechanism behind it. It
passes the gate, but it is the only demo where the new geometry may be a real cost. Re-measure
within a single session before treating it as either real or noise.

## 2.32d — chasing the one FPS number that had a mechanism (Z Island −8%)
Sixteen demos read FPS *up* and I discarded all of them as cross-launch noise. Z Island read
**−8% in-game** and also gained 18,155 polys, so it was the only number with a possible cause.
Method: **repeat launches of the SAME build** — the correct test, since the pre-2.32 "baseline"
is itself a cross-launch number and cannot be trusted as a reference.

Z Island in-game FPS history (each row is one launch; the three samples inside a row are 4 s apart):
```
2.25   95.2 / 95.2 / 95.2      mean  95.2
2.27  119.6 / 119.8 / 119.8    mean 119.6   <- +25.6% from a DECAL-POOL change. No mechanism.
2.28   98.1 / 98.0 / 98.0      mean  98.1
2.32   90.6 / 90.4 / 90.8      mean  90.6
2.32   86.3 / 88.2 / 88.6      mean  87.7   (repeat, same binary)
```
★ Note the shape: **within a launch the three samples agree to 0.4%; between launches the same
build moves 3%, and across builds with no plausible cause it moved 25.6%.** Tight in-run samples
are precision, not accuracy — they invite exactly the false confidence that made "2.27 is 25%
faster" look like a result at the time.

### ⚠ A REAL COST IS PLAUSIBLE HERE, and if so the AABB defect is the cause — not the clamp
The 2.32 runs sit ~89 against 95-98 before it. If that survives more samples, the mechanism is
NOT the clamp itself but the **still-unfixed corrupt AABB** (§2.32c): those pickups carry a 2 km
bounding box, so they pass every frustum test and are now **drawn on every frame regardless of
where the camera points — including behind it.** Before 2.32 they were skipped entirely by the
overflowed dither, so the level paid nothing for them.
★★ **That reframes the AABB fix**: it is not culling hygiene, it is the change that would let
these objects be culled normally and hand the cost back. Recommended as the next work item.
⚠ Do NOT read this as "2.32 made Z Island slower and should be reverted" — before 2.32 the level
was faster because it was **not drawing objects the designer placed**. The correct comparison is
2.32 versus 2.32-with-the-AABB-fixed, which does not exist yet.

## Regression check — the USER-CONFIRMED fixes re-verified on the 2.32 engine
The ragdoll and particle fixes were confirmed by the user on the **2.29** build, and the engine has
changed three times since (2.31 tracer, 2.32 clamp + queue attribution). Re-checked rather than
assumed:
```
RAGDOLL_UPDATE: calls=253 bones=13 framesMoved=66 writebackCalls=16445
RAGDOLL_WB:     applied=10626 splitTarget=0 clobbered=0 survived=252
VERDICT: CHAIN COMPLETE — every stage fired and the bone HELD its ragdoll pose
```
Identical signature to the 2.29 verification. **D is not regressed by 2.30-2.32.**
⚠ **A (the particle re-use fix) has no automated repro and was NOT re-verified.** It needs N+1
impacts of one decal type with expiry in between — `DECAL_BURST` exercises allocation, not the
emitter state machine, and the harness cannot fire a weapon. It is unaffected by anything in
2.30-2.32 by inspection (all three are render-path only, no emitter code touched), but that is
reasoning, not measurement. Worth one shot-the-ground check next time MAX is in front of a human.

## 2.32e — in-game verification, and a CORRECTION to my own §2.32d reasoning
Verified the pistol in TEST GAME (the editor check was not enough for a collectable). Full record:
```
aabb=(-700,42,153)-(100001,100001,100001) center=(49650,50021,50077) r=86750
camEye=(-615,65,59)  frustum=REJECT  dither=0.0000
batchDistance(2.32-clamped)=65504.0  rawDist=86741  (would have been INF before the clamp)
VERDICT: FRUSTUM rejects its AABB
```
**The fix is confirmed live in-game:** `dither` is 0.0000 where it was INF, so the silent
fade-skip no longer fires. The object is not drawn at this instant only because the player spawns
facing away from it — an ORDINARY frustum cull, and the instrument names it as such.

### ⚠ CORRECTION — I overstated the Z Island mechanism in §2.32d
I wrote that the 2 km AABBs "pass every frustum test, so the pickups are drawn every frame
regardless of where the camera points". **This measurement refutes that**: a box spanning to
100001 on every axis was FRUSTUM-REJECTED at the player start. A huge box still fails the frustum
when it lies entirely to one side of the view cone — which this one does, since it extends from
z=153 to z=100001 and the camera sits at z=59 looking away.
**So the proposed cause of Z Island's −8% is NOT established.** What the corrupt AABB actually
does is make culling INACCURATE in both directions (wrong centre, wrong extent) — not
unconditionally permissive. The −8% remains an unexplained n=2 observation.
★ Downgrading it accordingly: the AABB is still worth fixing for correctness, but **"it costs FPS"
is now unproven, and the claim should not be repeated until someone measures 2.32 against
2.32-with-the-AABB-fixed.** I made the same mistake here that I made twice earlier tonight —
narrating a mechanism from a measurement that did not test it.

---

# 2.33 — the game-side AABB lever (`masterpark`), and why it is DEFAULT-OFF

## What the measurement changed about the recommendation
§2.32c called the game-side fix "preferred". **That was written before I measured the armature.**
`WHYNOTDRAWN` now prints it:
```
mesh: skinned=1 (armatureID=2810) dynamic=0   meshAABB=(-3,-0,-7)-(3,2,7)
armature: bones=1  worldAABB=(99999,99999,99999)-(100001,100001,100001)
```
A **ONE-BONE** armature, owned by the hidden master parked at 100000, and **shared by every clone**
(both objects report `meshID=2817`). The mesh's own local box is tiny and correct.
★ Consequence: **no game-side change can make these bounds correct.** While a master and its clones
share one mesh — hence one armature — any two instances at different positions get a union. The
correct fix is engine-side: do not merge an armature's world box into an instance that does not own
that armature's transform. The game-side lever can only BOUND the damage.

## What shipped
`setup.ini masterpark=<units>` (2000..100000, **default 100000 = unchanged**) replaces the two
hard-coded park sites in `M-Entity_part0.cpp`. Read in the EARLY setup.ini pass because masters are
parked during entity-profile load.

### Measured A/B, one level, same binary
| arm | object AABB | radius | armature box |
|---|---|---|---|
| default (no key) | (-700,42,153)-(100001,100001,100001) | **86750.0** | at 100000 |
| `masterpark=4000` | (-700,42,153)-(4001,4001,4001) | **3625.8** | at 4000 |

* **The OFF path is byte-identical** to the pre-2.33 reading — the rule that caught a bogus
  hiersplit A/B once already.
* **Executed-check passes**: the armature world box actually moved, so the knob reaches the thing.
* Bogus radius cut **24×**, to 5.5% of the fp16 ceiling instead of 132% of it.
* ⚠ **Still not correct**: centre reads (1650,2021,2077) against a true ~(-693,42,160). Culling
  remains inaccurate, just at level scale rather than 100 km scale.

## Why the default was NOT flipped
Masters are hidden but still present in the scene. Moving every entity master from 100 km out to
inside the play area has real blast radius — ray picks, physics and anything that ignores
visibility could now find template geometry. **That needs a 19-demo sweep before it becomes the
default, and it is a product decision, not a code cleanup.** The knob exists so that sweep can be
run on one binary.
★ Order of preference for actually closing this: **engine-side ownership test first**; `masterpark`
is the mitigation if that proves too invasive.

---

# §2.34 — Is the engine-side AABB fix NECESSARY? (measuring the blast radius)

The 2.33 work answered *how* the bounds go wrong. It never answered the question that decides
whether to touch upstream code on the hot path: **how much of a real level is affected, and can
this defect make anything a player would see go wrong?** Both halves are measurable, and neither
is answerable by reading code, so this section is built on two instruments rather than argument.

## The instrument: `DUMP_BIGAABB [inflationRatio]` (game-side, no engine rebuild)

Walks every object with a mesh, recomputes the **clean** box the engine would produce from the
mesh alone (`mesh.aabb.transform(world)` — exactly what `RunObjectUpdateSystem` does at
`wiScene.cpp:5231`), and compares it to the AABB actually in the scene.

★ **The gate is a RATIO, not a coordinate threshold.** A coordinate test ("is any corner near
100000?") would report *zero affected* the moment `masterpark` moved the park point closer — and
that zero would read as a fix when nothing had been fixed. The ratio survives any park distance.
This mattered immediately: it also surfaces **legitimate** armature merges at ~2×, which is how the
one dangerous claim below got discriminated instead of assumed.

## Result: the defect is real, and it is TINY

| demo | objects | skinned | inflated >2× | over fp16 | distinct meshes |
|---|---|---|---|---|---|
| RPG Template | 7249 | 95 | **2** (0.03%) | 2 | 1 |
| Escape from the Zombie Cellar | 1167 | 70 | **5** (0.4%) | 3 | 4 |
| Island Showdown | 8558 | 285 | **17** (0.2%) | 5 | 17 |
| Switch Escape | 1030 | 33 | **0** | 0 | 0 |

Two findings that reframe the problem:

1. **Characters are NOT affected.** 93 of 95 skinned objects in RPG Template are clean. Every
   truly-corrupt entry is a **1-bone** rig named like a weapon or ammo pickup (`W_MK19T`, `W_M29S`,
   `W_AR15`, `870_W`, `44ammo`). GameGuru clones a character's mesh+armature per instance, so
   characters own their armature and the merge is correct for them. Only *shared* prop rigs
   inherit the parked master's armature.
2. **The 2.1–2.5× entries are NOT the bug.** `adult_male_head_07`, `zombie_male_facialhair_02` —
   12–13 bone armatures whose genuine extent slightly exceeds the mesh bind box. That is upstream
   behaving correctly, and it is exactly what the armature merge exists to do.

So the population is **~2–5 objects per level, all small opaque pickups.**

## Can it make anything invisible? — the consumer audit

Every consumer of an object's AABB / `center` / `radius` was enumerated across six domains
(visibility, shadows, LOD+sort, fp16 packs, GI/accel-structures, picking+game-layer), then every
"invisible" and every reached "none" claim was handed to independent refuters.

**The structural answer: a superset box cannot hide anything.** Every frustum / overlap /
containment test in the reached path (`CheckBoxFast`, `AABB::intersects`, Sphere-vs-AABB) is
monotone under enlargement — if the true box passes, the union passes. Wrong-direction failures
can only come from sites that use the **centre as a position** or that **narrow the value**.

Settled by measurement:

| worry | verdict | evidence |
|---|---|---|
| Shadow cascade extents widened by a 100 km caster | **NO** | `CreateDirLightShadowCams` builds cascades only from the *camera's* unprojected frustum corners (`wiRenderer.cpp:3491-3501`); casters never contribute |
| Props stuck at low detail | **NO — opposite** | `lod = clamp(log2(1/maxdim),0,lod_max)`; a huge box projects huge → clamps to **LOD 0, highest detail**, always. Cost, not a visual defect |
| Whole-scene bounds blown out | **NO** | scene bounds measured **±2,500,000** from the terrain preset — four orders above the 100 km boxes, which are lost in the noise |
| Transparent objects sorting wrong | **NO** | distance is the primary sort key, so this was the one live risk. Measured: **TRANSPARENT_OVERFP16 = 0**. The only transparent inflated objects are the 2.5× facial-hair (radius 13.4, normal distance, sorts correctly); all three corrupt objects are `filter=0x1`, opaque |
| Animation throttled on-screen (`wiScene.cpp:2253` — a *centre* distance test, so unprotected) | **NO** | applies only to objects with a running `AnimationComponent`; the affected set is 1-bone pickups with no clip |
| CC anim-proxy picked wrong (`wickedcalls_part0.cpp:1086`, strict `>` on radius) | **NO** | requires an affected *character*; census shows none |

### Audit tally (adversarial)

73 findings across the six domains. Every "invisible" claim and every reached "none" claim — 31 in
all — was handed to an independent refuter instructed to default to *refuted* when unsure.

| outcome | count |
|---|---|
| overturned by refutation | **17** |
| confirmed invisible | **1** (and it is *not reached*) |
| confirmed none | 13 |
| reached perf costs | 25 |
| not reached | 28 |

★ **Every reached invisible-class claim was refuted**, including all three I had provisionally
believed: the transparent sort key (downgraded to perf), the animation throttle, and the CC proxy
picker. The measurement (`TRANSPARENT_OVERFP16 = 0`) and the code argument agreed.

### The single surviving invisible-class claim — planar reflections

`wiRenderer.cpp:4562/4567` builds the mirror plane with `XMPlaneFromPointNormal(object.center, N)`.
`XMPlaneFromPointNormal` stores `d = -dot(N,P)`, so displacing the centre shifts the plane
**one-for-one** — and being conservative buys nothing, because a plane derived from a centroid has
no superset property. The normal stays correct (it comes from the world matrix) but the plane point
lands ~50,000 units off along it, so the reflection camera ends up outside the scene and the
reflective surface samples garbage.

**Not reached, and gated by three separate things:** it needs a planar-reflection material *on a
skinned mesh* (only skinned/dynamic meshes get the armature merge); the GG per-mesh checkbox
defaults off (`M-Entity_part1.cpp:238`, and the UI itself is labelled `// planar reflections
(buggy?)`); and in any watered level the ocean **overwrites** the plane afterwards anyway
(`wiRenderer.cpp:4789-4791`, and GG enables the ocean at `M-GridEditB_part3.cpp:1703`).

⚠ This is the clean illustration of the rule below: it is the one site in `UpdateVisibility` that
uses the centre as a **position** rather than as a bound, and it is the one claim that survived.

## The one real cost found: picking

`AABB::intersects(const Ray&)` early-outs `true` when the ray origin is **inside** the box
(`wiPrimitive.cpp:152`). The corrupt box spans from the prop out to 100 km, so it **contains the
camera**, the early-out fires before the slab test, and the candidate is admitted with `tmin` forced
to 0. That defeats the shipped 2026-07-31 **TMax cap** — the optimisation that took a 120-unit LOS
ray from 26 ms — for these objects. Bounded by the same census: ~5 extra candidates per ray, each
then rejected by the exact per-triangle test, so picks stay *correct*, just not free.

## VERDICT

**The engine-side fix is NOT necessary for playability.** No reached mechanism can make geometry
that should be on screen fail to draw; the one symptom that ever did (the invisible pistol) was the
fp16 distance overflow, already fixed in 2.32. What remains is a small, bounded tax on **~2–5
opaque pickup props per level**: always LOD 0, never occlusion-culled, admitted to every shadow
cascade, and defeating the ray TMax cap.

⚠ **The durable rule this produced** — worth more than the verdict itself:

> **A conservative (superset) bound is safe everywhere it is used AS A BOUND, and unsafe the moment
> it is used AS A POSITION.** Enlargement is monotone through every frustum, overlap and
> containment test, so those can only over-include. But a centre fed to a distance comparison, a
> plane equation, a sort key or a screen projection has no such protection and can fail in the
> wrong direction.

That rule is what made this auditable: it splits ~73 candidate sites into "provably fine" and "must
be checked individually", and the one claim that survived refutation (the reflection plane) is
precisely a centre-as-position site. Any future work on this defect should triage the same way.

The corollary is that the affected population is what keeps the centre-as-position sites dark, and
population is a **content** property, not a code one. A pickup with a glass scope, a pickup with an
idle animation, or a character whose meshes ever share a parked master rig would each light one up.
**The last shape has already bitten this project once** — GGMAX 1.61, a full-screen character stuck
at 30 fps animation because its proxy pin resolved wrong. `DUMP_BIGAABB` exists so that question can
be re-answered in two minutes instead of re-derived.

**Recommendation: don't do it now; do it when next touching that code.** The correct form is an
ownership test at `wiScene.cpp:5233` — merge the armature box only into an instance that actually
owns that armature's transform. It must fail **toward merging**, because the dangerous direction is
skipping a merge for a legitimately animated character that has moved beyond its mesh bind box
(root motion), which *would* wrongly cull. `masterpark` remains the bounded mitigation.

---

# §2.35 — Deleted/collected object leaves its shadow behind (GAME-SIDE FIX)

User repro: collect the ammo in test-game, or simply **delete it in the level editor**, and its
shadow stays on the table. The editor half is the important one — it turns a "walk over and press
E" bug into a one-click one, and it rules out anything specific to the collect path.

## Root cause: a cached shadow atlas whose change-detector cannot see a removal

Local (point/spot) shadow maps are **cached** — re-rendered only when something is detected to have
changed. The detector is GGMAX 2.07d (`wiRenderer.cpp:5117`), and it finds a caster that **MOVED**,
by comparing each object's world matrix against last frame's:

```cpp
if (!object.IsRenderable() || !object.IsCastingShadow()) continue;
if (memcmp(&mNow, &mPrev, sizeof(XMFLOAT4X4)) == 0) continue; // did not move
```

Neither line can fire for an object that is **removed**:
* **Deleted** — it is gone from the objects array, so there is no "now" matrix left to differ from
  its "prev" one. The loop never even reaches it.
* **Hidden** — it is skipped outright by the very first guard.

So `changed` stays false, the atlas is not re-rendered, and it keeps the removed object's depth
**indefinitely**. 2.07d was written for "a box moved under the spot"; removal is the case it does
not cover.

★ The engine already exported the nudge — `wi::renderer::InvalidateLocalShadows()`
(`wiRenderer.h:1200`), and `changed = localShadowInvalidate || ...` consumes it. It simply had
**no caller anywhere in the game**. Nothing ever refreshed the cache on a content change. That is
the whole defect, and the fix needs no engine change at all.

## The fix (game side only, three call sites)

| site | why |
|---|---|
| `M-Entity_part4.cpp` `entity_deleteentityfrommap()` | the single choke point for editor deletes — all 7 delete routes reach it |
| `DarkLUA_part0.cpp` collect branch | collecting teleports the pickup 999999 units away rather than hiding it |
| `DarkLUA_part0.cpp` drop-back branch | the same problem in reverse: an item dropped back would return **shadowless** |

Each calls `GGInvalidateLocalShadows()`, a one-line wrapper added to `wickedcalls_part2.cpp`.

## Verified — the causal chain, not just the symptom

Three shots on TESTPRO2, one binary, one session, only the nudge differing:

| shot | ammo | shadow |
|---|---|---|
| `01_before` | present | present |
| `02_removed` | **gone** | **still there** ← the bug, reproduced |
| `03_nudged` | gone | **gone** |

The **pistol's** shadow is present in all three — the control that proves the nudge cleared a
*stale* shadow rather than merely blanking the atlas. Shot 3 calls the same engine entry the
shipped fix calls, so the experiment tests the shipped mechanism and not a lookalike.

⚠ **Two instrument failures on the way here, both worth remembering.** `MOVE_ENTITY` writes the
Wicked transform directly, and GameGuru re-asserts entity positions in the editor, so the ammo was
still on the table afterwards — the caster was never actually removed. And the first Trapped run
compared two screenshots taken from across the room where the pickup is ~4 px wide, which is
indistinguishable from "nothing happened". **A visual A/B needs the subject legible and an
executed-check that the arm was actually applied**; the census (`DUMP_BIGAABB`) supplied the
latter, the user's re-framed TESTPRO2 camera the former.

## Still open

`SetEntityCollectedEx` does not hide a collected entity, it teleports it 999999 units away. It is
therefore *still a live, renderable, shadow-casting scene member* sitting a million units out, and
(being a pickup) it carries the corrupt one-bone armature AABB from §2.34, so its bounds now span
from −999999 to +100000 and contain the whole level. Harmless today given §2.34's findings, but
"removal by teleport" is a smell worth revisiting if pickups ever misbehave again.

## §2.35b — the ammo took a DIFFERENT removal path (follow-up fix)

2.35 fixed the editor delete, but in test-game the ammo shadow survived — and the user supplied the
observation that named the gap immediately: **collecting the GUN made the ammo's shadow disappear.**

That asymmetry is the whole diagnosis. The nudge works; the ammo simply never reached it:

| pickup | script | removal call | hooked by 2.35? |
|---|---|---|---|
| Magnum Pistol | `weapon.lua` | `SetEntityCollected(e,…)` | ✔ yes — so collecting it cleared the stale ammo shadow as a side effect |
| .44 ammo | `ammo.lua:95` | **`Destroy(e)`** | ✘ no — a completely separate path |

`Destroy(e)` → `entity_lua_destroy()` sets a **deferred** `destroyme = 1`, consumed a frame later at
`G-Entity_part1.cpp:1059`. The fix hooks the **consumer**, not `entity_lua_destroy`, for two
reasons: it covers every script that calls `Destroy()` plus explosion triggers, and it cannot fire
on a request that `entity_lua_destroy` *declines* (`iscollectable == 2` with quantity remaining).

★ **The lesson worth keeping: "collecting a pickup" is not one code path.** I hooked the one the
weapon used, verified it in the editor, and reported it fixed — the in-game half was a different
function reached by a different script. When wiring a cross-cutting notification like a cache
invalidation, enumerate the *removal verbs* (`SetEntityCollected`, `Destroy`, editor delete) rather
than the user-facing action, or the fix covers whichever one you happened to read first.

---

# §2.37 — No screen-editor image has EVER drawn on DX12 (one-line fix)

User repro: add an image widget to the In-Game HUD, point it at
`imagebank\hud\ammo-health-panel.png`, and the yellow selection box stays empty.

## The measurement that reframed it

`HUD_DUMP` reported the widget as **`exist=YES 235x139`** — the texture loads fine, and the
selection box is even *sized* from those dimensions (`M-GridEditB_part22.cpp:938` only reads
`ImageWidth/Height` when `ImageExist` passes). So the fault was never the path or the load; it was
the blit. That split — loaded but not drawn — is what made this a five-minute read instead of a
hunt.

## Root cause: a DX11-only accessor used as the guard

```cpp
void* lpTexture = GetImagePointer(imgID);   // <-- always NULL on DX12
if (lpTexture) { ... ImGui::ImgBtn(imgID, widget_size, ...); }
```

`GetImagePointer` (`CImageC_part1.cpp:236`) returns `m_imgptr->lpTexture`, the **D3D11 texture
object**. Only its sibling `GetImagePointerView` (`:253-268`) carries the DX12 bridge that lazily
creates the texture through `ImGui_DX12_GetOrLoadTexture`.

So the guard could never pass. ★ And the irony: the line it guards, `ImgBtn(imgID, ...)`, takes the
**ID** and resolves it through that same working bridge — which is exactly why the toolbar icons on
the left of the screen editor have always rendered. **The blit was never broken; only the
null-check in front of it was.**

Fix — use the DX12-aware accessor, which both tests correctly and warms the texture the blit is
about to need:

```cpp
void* lpTexture = (void*)GetImagePointerView(imgID);
```

One line, one site (`GetImagePointer` had exactly one caller in the whole game layer).

## Verified, and much wider than the reported symptom

| | images drawn on the HUD canvas |
|---|---|
| before | **0 of 10** |
| after | **11 of 11** |

The re-run shows the panel inside the selection box, *and* the ammo/health panels bottom-left and
bottom-right that had only ever rendered their bare `999999` text, *and* the `about.png` backdrop.
Nobody had reported those as broken — they were simply never seen working.

★ **The lesson: a paired accessor where only ONE half got the DX12 bridge is a bug template, not a
one-off.** `GetImagePointer` / `GetImagePointerView` differ by one word at the call site and by
everything at runtime. Worth grepping the DX11 halves of any such pair when a "loaded but invisible"
symptom appears.

---

# §2.39 — Quality presets were stamping over the level's authored Shadows panel

Reported as "the editor shows the right point/spot counts, test game zeros them".

## What the instrument showed (DUMP_SHADOWQTY, Snowy Mountain Stroll)

| | visuals | gamevisuals | editorvisuals |
|---|---|---|---|
| editor | 8 / 16 | 8 / 16 | 8 / 16 |
| test game (before) | **1 / 2** | 8 / 16 | 8 / 16 |

**Nothing was ever zeroed.** `gamevisuals` held the authored 8/16 throughout; `t.visuals` was
overwritten with 1 spot / 2 point — the literal values in `SetGlobalGraphicsSettings` `case 0: //
low` (`M-GridEdit_part0.cpp:1768-1770`), which runs from `M-GridEdit_part2.cpp:1045` whenever
`pref.iTestGameGraphicsQuality != 2`.

The on-screen "0/0" was a second defect layered on top: the panel's combos match only
{0,4,8,12,16} with no else, so 1 and 2 selected nothing and it printed index 0.

## ⚠ My first fix was wrong, and the failure is instructive

2.38 seeded those combos from the **nearest listed value**. That fixed nothing, and could not have:
**the nearest entry to 1 IS 0.** I had confirmed the mechanism (unlisted value → displays 0) and
then shipped a remedy for it without re-running the instrument I had just built — the one command
that would have shown the display still reading 0. A fix aimed at a symptom deserves the same
verification as the diagnosis that found it.

## The fix (authorised: presets must not touch user-chosen settings)

All **20** `t.visuals.iShadow*` writes across the four preset cases are commented out, tagged
`GGMAX 2.39`. This follows the rule the same function already applied to tree shadow LOD distance
and cascade range: *level-authored settings are exempt from quality presets*. Shadow quantity and
resolution live in the same panel and now behave the same way.

Non-shadow preset writes (SSR, FXAA, light shafts, lens flare, reflections) are deliberately
untouched — different panel, not part of the report.

## Verified

| | visuals | gamevisuals |
|---|---|---|
| test game (after) | **8 / 16, res 2048/512/128** | 8 / 16, res 2048/512/128 |

The authored values now survive into test game intact, resolutions included.

⚠ **Not visually confirmed in the panel itself.** The in-game Visuals panel opens via
`TOGGLE_PROFILER` but its Shadows section is collapsed and I did not find a harness click target to
expand it. The dump is authoritative — the combos read the same `t.visuals` it prints, and 8 and 16
are both listed values — but the pixel-level confirmation is owed.

---

# §2.41–2.43 — Firing the weapon from the harness, and the explosion refraction artifact

## The trigger gate: it is `t.gunmode = 101`, NOT `firingmode`

Two attempts failed before an instrument was built, and both failures came from the same wrong
assumption:

1. `g.playeraction = 1` — the engine's "Automated actions (script control)" hook
   (`M-Physics_part1.cpp:218`). Ammo stayed 30/388 across 14 frames.
2. Holding `t.player[1].state.firingmode = 1` for 6 consecutive frames, applied from *inside*
   `physics_player_gatherkeycontrols` — the only writer of that field — to remove both the
   frame-ordering and the single-frame questions. Same result.

**`DUMP_FIRE` (GGMAX 2.42)** settled it in two runs. It samples, per frame inside `gun_manager`
and *after* every gunclick-zeroing gate: `firingmode`, `gunclick`, `gunmode`, `pressedtrigger`,
`mustrelease`, ammo, and each gate's own inputs — plus a counter for frames the gun update
early-outs before reading the trigger at all. It prints a verdict naming the stage, because from
outside every one of these looks identical.

| run | reading | what it eliminated |
|---|---|---|
| 1 | `frames=30 earlyOut=0 firingmode1=0` | the gun update DID run every frame; firingmode never arrived as 1 |
| 2 | `holdLeft=0` | the hold DID run and drained — so the value was lost *between* the two functions |

★ That pointed at `DarkLUA_part5.cpp:1599`, where the original developers hit this in **2015** and
left the note: *"seems when in game, this gets ignored so no gunshoot happens.."*. Their own Lua
`FirePlayerWeapon(1)` therefore does **not** set `firingmode` — it sets **`t.gunmode = 101`**. Only
`firingmode >= 2` (zoom) still writes the field.

**`firingmode` is an animation input; `gunmode` is the trigger.** `FIRE_WEAPON` now uses that path
— verified, ammo 29 → 28.

## Capturing the explosion: harness round-trip is too slow, use BURST_FRAMES

`SCREENSHOT` costs ~1s per round-trip and the plume is far shorter, so every frame landed *after*
the blast: barrels gone, camera thrown at the ceiling by the shockwave. `BURST_FRAMES <n>` writes
**consecutive rendered frames**, so at ~215 FPS a 120-frame burst covers the whole event with no
gaps. Order matters — fire FIRST, arm the burst on the very next command, so the window opens
inside the blast rather than before it. Driver: `tools/sceneupdate/explosionburst.sh`.

⚠ **`BURST_FRAMES` writes its path relative to the GAME'S CWD**, and that CWD is `Max/Files` — so
frames land in `Max/Files/Files/screenshots/`, not `Max/Files/screenshots/`. The memory rule
"runtime `fopen` files land in the game's `Files/` CWD" exists for exactly this and I still looked
in the wrong folder, reporting "0 frames captured" while 85 sat on disk.

## The artifact, on record

Frames kept in `tools/sceneupdate/burstshots/keep/` (`frame_000` pre-blast, `frame_006` and
`frame_012` mid-plume). What they show, matching the user's report:

* **Hard-edged RECTANGULAR blocks** of smeared and duplicated scene content exactly where the
  outward-travelling particles are — the table and candle appear stair-stepped and repeated across
  block boundaries, with large flat regions of stretched wallpaper. Nothing resembles smoke.
* **FPS collapses ~215 → 49.6** across the blast.

Both are the signature of particles refracting the scene across their **entire quad** with hard
edges, instead of through a soft textured mask — i.e. the user's suspicion that the refraction
particles have **no texture bound** in that pass. The FPS collapse is the overdraw cost of many
quads each sampling the scene colour buffer.

## ▶ NEXT STEP (not started)

Identify which system draws these — **gpup/.arx** (`project_gpup_particles.md`) or **WPE**
(`project_wpe_particles.md`) — and check whether its material has a null/unbound texture in the
refraction pass. This is now a targeted question, not a hunt: we know it is the outward particles,
and `explosionburst.sh` reproduces the capture in one command.

---

# 2026-08-14 — ★★ KNOWN-GOOD MILESTONE ★★

**Game `ffe4de6b` (main) + engine `07a192f2` (master), both pushed.**
Lee marked this pairing a GOOD MILESTONE before the overnight autonomous run.
If anything after this point regresses, THIS is the pairing to bisect against.

State at the milestone:
- Explosion refraction artifact CLOSED in four steps, all documented in
  WICKED_ENGINE_CHANGES.md and the burstshots/ README:
  2.44 distortion NORMALMAP slot (game) · 2.45 rotation units (game) ·
  2.46 velocity-aligned rotation (engine) · 2.47 positive-only distortion clamp (engine).
- Zoom-fire no-damage bug ROOT-CAUSED, fix not yet applied (next section).
- All 2.4x changes still await Lee's eye + a 19-demo sweep.

## §2.48 (in flight) — zoomed firing applies no damage

Root cause, verified in source by me plus a 21-agent adversarial workflow (5 confirmed,
12 rejected candidates): the gun-exclusion swap in `IntersectAllEx`
(`CObjectsC_part3.cpp:1780-1786`) sets the LIVE LayerComponent, but the DX12 engine's
`Scene::Intersects` object pass tests the CACHED `aabb.layerMask`
(engine `wiScene.cpp:7180`, cache written only during `Scene::Update`, `wiScene.cpp:5516`).
The swap-pick-restore happens entirely between updates, so the cache never sees it and the
weapon mesh stays pickable. Zoom pulls the gun onto the camera axis
(`G-Gun_part1.cpp:69`), the ray starts inside it, `Pick` returns the closest hit = the gun,
and the out-of-entity-range branch (`CObjectsC_part3.cpp:1806-1810`) reports that as a
TOTAL MISS. Ammo/flash/sound are upstream of the ray — exactly the reported symptom.
DX11's `Pick` tested the LIVE layer (`WickedRepo wiScene.cpp:4989-4994`), so the same
game code worked there. The whole shot path game-side is byte-identical to DX11 —
three diffs, zero output. `pIgnoreObject` is broken the same way, so AI line-of-sight
and every other ignore-object raycast is affected too.

## §2.48 SHIPPED — zoomed firing does no damage (game bfadc05f)

The 2.48 root cause above held. Fix is GAME-SIDE: `WickedCall_SetObjectRenderLayer`
writes the cached `aabb_objects[i].layerMask` through alongside the live LayerComponent
(`objects.GetIndex` + direct vector write, swap-time cost only). Repairs the gun
exclusion AND `pIgnoreObject` — AI line-of-sight included.

Proof, same probe pre/post (tools/sceneupdate/zoomfire.sh):
  pre : zoomed shot hit=0 SWALLOWED-BY=16104, gunobj=16104 (id-for-id the weapon)
  post: zoomed shot hit=70081, swallowed=0 — the ZOOMED shot detonated the barrels
Instruments: ZOOM_FIRE (shipped Lua input path — RMB bit 2 zoom hold, LMB bit 1 fire;
⚠ NOT the 2.41 firingmode hold, which does not fire), DUMP_SHOT (per-ray rows +
verdict; g_ggLastRayBlockedBy names the swallower from IntersectAllEx's discard branch).

## §2.49 SHIPPED — Terrain Generator preview + Generate crash (game 0dbd984f)

A: preview showed no terrain — the generator "preview" is the LIVE scene through a
   HOLE in the opaque fullscreen ImGui window; the 4-scissor-strip hole existed only
   in the DX11 backend. Ported to ImGui_DX12_RenderBridge, including draw callbacks
   10/11 (bForceRenderEverywhere) that let the yellow editable-area overlay punch
   through. Verified: generator screenshot shows terrain + overlay + intact panels.
B: Generate crashed — NULL GetBackBufferForGG stub + forced DIGAHOLE screenshot block
   = null vtable call at M-TerrainNew_part5.cpp:3639 (matches Lee's own Guru-Crash.log
   02:07:10 entry exactly), and the crash preceded the Save-As trigger so the level was
   LOST. Now routes through WickedCall_CaptureBackbufferRegionToJPG and continues.
   Verified: TERRAINGEN_GENERATE → process alive, saveAsOpen=1.
   Remainder: the lastnewlevel.jpg capture returns false on DX12 — same gap as the
   storyboard-thumbs debt; now logged visibly ("TerrainGen thumb capture FAILED").
Harness: CLICK_NODE now opens the generator for an EMPTY level node (storyboard
recipe), TERRAINGEN_GENERATE / TERRAINGEN_STATE. Driver: terraingen.sh.

## §2.50 SHIPPED — tutorial videos play on DX12 (game 86eb8dda)

Root cause: blanket `if (m_pD3D == NULL) return FALSE;` in CoreLoadAnimation killed
ALL video; the WMF decode is CPU-side and never needed the device. Bridged the three
D3D11 pieces: ImGui_DX12_UpdateVideoTexture (new-texture-per-frame via the proven
one-shot upload + deferred delete), the YUY2→RGBA conversion into a CPU buffer, and
GetAnimPointerView returning the bridge handle (parallel array, NEVER pTextureRef —
that one is SAFE_RELEASE'd as real COM). Verified live: VIDEO_STATUS view handle
non-null AND CHANGING between polls, percent 5.0→10.5→16.0, playing=1.

★ PROBE POST-MORTEMS (cost three build cycles, worth remembering):
- timestampactivity BUFFERS and loses its tail under taskkill //F — hang forensics
  need open-append-close per line (gg_videotrace → Files/videotrace.txt).
- ANIMATIONMAX is 33. An out-of-range slot fails SILENTLY in LoadAnimation and
  PlayAnimation on a dead slot raises a MODAL RunTimeError — a headless run reads
  that as a permanent hang. The modal class of hang has now bitten twice (MF relative
  path failure is also modal); assume ANY legacy error path may be a MessageBox.
- After taskkill, the current run's log is still Guru-Game.log — the -last rename
  only happens on the NEXT launch. Grepping -last reads the WRONG RUN.

## §2.51 SHIPPED (afternoon 08-14, game 56377809) — 2.50's own perf leak, found by Lee's sweep

Lee's requested 19-demo sweep came back with 17/19 demos down 30-50% FPS (editor ~+6 ms,
game ~+10 ms per frame). The chain that cracked it, in order:

1. probe_one.sh same-binary re-probes read AT/ABOVE baseline (Switch 154.1, Kit 108.2) →
   I published "transient machine load, binary exonerated". **WRONG** — the re-run sweep
   REPRODUCED the slow numbers per-demo (Kit 47.9 vs 47.8) while probes stayed fast.
   ★ Lesson: a passing probe in a DIFFERENT context exonerates nothing; only reproduction
   under the failing context does. Launch-BIMODAL ≠ time-varying.
2. ENABLE_PROFILER + GET_PERF_DATA on 6 hunter launches (6/6 slow): CPU 20.5 ms vs GPU 13.1,
   and ONE range held it all: `Logic - ConstantNonDisplay: 11.70 ms`.
3. videotrace.txt (the 2.50 unbuffered tracer, still armed): every slow launch had loaded a
   hub/storyboard tutorial video (`tutorialbank\games\*.mp4`); both fast probes sat exactly
   in the only gap in the load ledger. 37/37 lines matched the fast/slow record.

Root cause: 2.50 made tutorial videos actually LOAD on DX12 (correct); but the widget
pauses its video only from its own per-frame draw (iSmallVideoFindFirstFrame countdown),
so leaving the section within seconds orphans a live MF session — and `iVideoChanged`
(the new-sample flag) was only ever SET, never consumed, so UpdateAllAnimation re-ran the
scalar YUY2→RGBA convert + synchronous bridge-texture upload on the SAME stale frame at
render rate, forever, even after the clip ended.

Fix (both halves independent):
- CAnimation_part0.cpp: DX12 branch consumes iVideoChanged BEFORE converting → conversion
  at sample rate (~30 Hz), zero once the session stops. DX11 path untouched.
- M-GridEditB_part4.cpp + GameGuruMain.cpp: per-entry heartbeat from every drawn tutorial
  widget; SmallTutorialVideoWatchdog (GuruLoopLogic, every frame) frees any widget slot
  stale >120 frames (Stop first if playing; logs to videotrace) — guards audio leaks and
  32-slot exhaustion across storyboard visits.

Verified: pre-fix 6/6 launches slow (48-51 FPS / 20.5 ms CPU); post-fix 4/4 fast
(90-105 FPS / 7.5-8.1 ms CPU), same recipe, minutes apart.

Side-findings settled by the same evidence: Grand Canyon's "20 FPS in game" sweep row =
its intro CUTSCENE (videobank introtolevel1.mp4) now genuinely plays — 2.50 working as
intended, sampled mid-cutscene; Bounty's intro (bountyintrocs.mp4) likewise, ending before
its samples. The +30-50 MB driver-VRAM drift on slow launches = the live MF session.

## §2.52 SHIPPED (evening 08-14, game 0b8f1f8f) — streaming video ring, cutscene 47 → 61 FPS

Lee: "make the cutscene path cheaper too." The 2.50 upload was a one-shot loader on
per-frame streaming duty: every new sample paid 2x CreateCommittedResource + fresh
upload buffer + row-copy + full CPU fence stall + new SRV + deferred delete. Replaced
with ImGui_DX12_UpdateVideoTextureYUY2 (bridge): 4-texture ring + SRVs + persistently
mapped upload buffer created once per video; YUY2→RGBA converted IN PARALLEL
(wi::jobsystem via GGVideo_ParallelFor shim in wickedcalls_part3 — the bridge can't
include wicked headers) straight into the mapped upload; one small copy + short fence
per NEW sample. Per-pixel math + the ceil((w/2)/8)*8 source-pitch rule byte-identical
to the legacy loop (now deleted from CAnimation — the bridge is the single copy).
RemoveTexture tears rings down, so DB_FreeAnimation + the 2.51 watchdog cover cleanup.

Measured (permanent `vidperf` lines in videotrace.txt, one/sec while playing):
1920x1080 convert=2.7ms copy+fence=2.5ms per new sample at 30 Hz = ~2.6 ms/frame
amortised. GC intro cutscene 44-49 → 58.6-62.1 FPS; screenshot-verified clean
colors/pitch; hub tutorial probe passes (handle cycles ring descriptors); editor
probe normal.

Open observation, NOT chased (diminishing returns for a 30fps movie): during-cutscene
FPS hovers suspiciously near 60 (58.6-62.1) while the residual video tax is only
~2.6 ms/frame — if someone later wants cutscenes >60, look for a legacy ~60 Hz pacer
in the fullscreen video path, not in the upload (that half is now measured cheap).
