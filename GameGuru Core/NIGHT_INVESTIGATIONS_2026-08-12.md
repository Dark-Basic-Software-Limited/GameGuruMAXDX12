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

## §2.52a — the "60 Hz pacer" hunt: THERE IS NO PACER (closed 08-15, no code change)

Hunted per Lee's request. Three independent proofs it does not exist:
1. A clean cutscene sample read **62.1 FPS** — no 60 Hz cap or vsync ladder can ever exceed 60.
2. Code: the Lua fullscreen video player (`entity_lua_playvideonoskip`,
   M-LUA-Entity_part0.cpp:1456) runs a MODAL loop — UpdateAllAnimation → PasteImageRaw →
   postprocess_preterrain → game_sync() → StartForceRender — with NO Sleep, no timer wait
   anywhere in it, and `SyncRate(0)` (uncapped) since init. Present stall during the movie
   is ~1.4 ms (not vsync-blocked).
3. Arithmetic: profiler during the movie reads CPU 8.4 ms (scene anims 5.0 + video
   convert/copy ~2.6 amortised) and GPU 10.1 ms (the 3D world still renders under the
   opaque movie). The modal loop's per-iteration render handshake mostly SERIALISES the
   two: wall 16.7 ≈ CPU+GPU (18.5) minus ~2 ms overlap. 1000/16.7 = 60 — a numerical
   coincidence that mimicked a cap. ⚠ Lesson: "suspiciously near 60" needs one sample
   ABOVE 60 before naming vsync; the 62.1 was sitting in the data all along.

Remaining lever, NOT taken (named constraint): skip the world render beneath a fullscreen
opaque movie → GPU ~10 → ~2 ms → cutscene ~95 FPS. Zero visible benefit (the movie is the
whole screen at 30 fps, already rock-steady at ~60) against real regression risk in the
modal player's other modes (3D-surface videos, VR paths, skip handling). Do it only if a
future need (battery/thermals on low-end) justifies touching that loop.

## §2.53 SHIPPED (08-15, engine 0230689f + game ebe10a1f) — generator pins the chunk ring to the yellow box

Lee's feature: in the Terrain Generator the chunk ring should generate around the
editable-area marker, not the camera; every other mode stays camera-centred. Mechanics
found first: at rest the yellow box ALWAYS sits at GGORIGIN (dragging shifts the terrain
noise offset under it, then re-centres), so the camera roaming in 3D view was the real
problem — the ring followed it away from the playable area.

Engine 2.53: three wi::terrain namespace globals substitute for camera.Eye at the ONE
center_chunk computation; ring/removal/sort all key off center_chunk so everything
follows. NOT Terrain members (transient UI state must not ride Serialize). Game: the
generator's marker block sets the override every frame (marker pos = origin at rest,
follows drag); the bridge force-clears it whenever bProceduralLevel is false and counts
a state flip as idle-gate activity (the centre jumps). TERRAIN_RING gains ovr/ovrX/ovrZ.

Verified: generator ovr=1 @ (0,0), 625/625 chunks, preview screenshot correct, Generate
flow intact (alive, saveAsOpen=1; lastnewlevel.jpg still the known 2.49 capture debt);
editor probe ovr=0 at normal FPS (Switch 135.6). Ledger: WICKED_ENGINE_CHANGES.md 2.53
re-apply-on-pull row.

## §2.54 SHIPPED (08-15, game 85cf2b4c) — generator entry wipes all chunks (Lee's leftover repro)

Lee re-entered the generator and got torn stale geometry. Structural: box-drags change the
noise offset, but invalidation marks only reach chunks under the GG editable area — ring
chunks OUTSIDE it keep old-offset geometry forever. Fix: GGTerrainWicked_Update watches the
bProceduralLevel false→true transition (one site, every entry route) → Generation_Restart +
blendmap-tracking clears + idle-gate ping. New harness TERRAINGEN_BACK (back arrow minus its
MODAL confirm) enables the enter→exit→re-enter loop. Verified: re-entry chunks=406 rebuilding
at +6s → 625 settled, session-2 screenshot fully coherent, 2.53 ovr intact both sessions.

## §2.55 SHIPPED (08-15, game 1648916e) — generator-only wide chunk ring (gen 19, 5 km view)

Lee wanted the full 5 km geography visible while creating a terrain; editor/test game keep
gen 12 untouched. Cost was MEASURED first (2.54a SET_TERRAIN_GEN A/B, gen 12 vs 19 in the
generator): +143% chunks but only +258 MB driver VRAM / +387 MB census / ~−12% preview FPS
(+1.5 ms CPU in Scene::Update, +1.2 ms GPU) — the near-ring high-res VT residency doesn't
grow, the extra 896 chunks are cheap far-LOD. Ring fill ~20-30 s.

Implementation rides the 2.53/2.54 bProceduralLevel transition watcher: entry saves the
ring, sets 19 BEFORE the session wipe; exit restores AND wipes again (a ring can never
shrink on its own — removal only reclaims past generation+2+removal_margin=26). The 2.54
wipe is hoisted into GGResetTerrainChunks. ⚠ SET_TERRAIN_GEN remains diagnostic-only.

Verified: entry 1521/1521 + screenshot shows ranges/rivers to the horizon past the yellow
box; back → storyboard reads gen=12 at exactly 625/625; re-entry 446→1521; 2.53 ovr pin
held; editor probe gen=12 ovr=0, Switch 135.6 FPS.

## §2.56 SHIPPED (08-15, game addc17ef) — generator grab/hover gated to the terrain view

Lee's repro: hovering the properties panel showed the grab tooltip; clicking there moved
the marker. The hover pick and drag-start pick cast a 3D ray from the raw cursor — the ray
passes straight through the ImGui panel into the scene (same blindness class as any pick
that ignores UI occlusion). Fix: point-in-rect gate on rClipRect (the dig-a-hole preview
area) for hover + drag START; active drags stay ungated so mid-gesture panel crossings
don't drop the marker. Verified with real-cursor probes (PowerShell Cursor.Position +
screenshots): panel hover = no tooltip; marker hover in-view = tooltip alive.
⚠ Reusable trick: PowerShell can move the REAL cursor for headless hover/UI tests —
first time the harness suite verified a hover behaviour without a human hand.

## §2.57 SHIPPED (08-15, game 382edc08) — generator ring-fill turbo, stash-controlled A/B

Lee offered to trade down to 30 FPS for faster ring fill. Levers: budget 8→20 ms +
high-priority job pool for the WHOLE fill (old code dropped to Low past the ~600-chunk
cone). Stash-controlled A/B on the same commit, same probe: **33 s → 28 s (−15%), and FPS
DURING fill improved 55-62 → 66-83** — the user never pays the offered price because the
budget is not the limiter: early-phase rates were IDENTICAL at 8 vs 20 ms (160/560 chunks
at the same ticks) = the async generation pipeline is the ceiling. The whole win is the
tail, where Low-priority starvation was stretching the work. Zero cost once the ring is
full (budget only consumed while work exists; post-fill 83.5 both arms).

★ Named constraint, not taken: the next speedup is engine surgery — parallelise the
generator job's sequential outer chunk loop (multiple chunks in flight; VT allocation
contention + merge-ordering risk) for maybe 28 s → ~12 s. Only if Lee wants it.
★ Method note: the false-exoneration lesson applied — the first turbo run LOOKED like an
improvement but the control run is what made the claim (and revealed the counterintuitive
FPS improvement + the real ceiling).

## §2.58 SHIPPED (08-15, engine 34f4bf29 + game 4ed9d6ad) — chunk cost NAMED: 75% was the pick-BVH

Lee challenged the per-chunk cost ("my DX11 terrain generated near-infinite distance in
seconds"). Instrument, not theory: per-phase accumulators in the chunk bake + TERRAIN_GENPROF.
**Answer: 10.96 ms/chunk, of which 8.20 ms (75%) = SetBVHEnabled's SYNCHRONOUS CPU triangle
BVH (8712 tris)** — a pick-acceleration structure DX11 never built (its picks were analytic
against the heightmap). The real mesh upload (renderdata) is 1.56 ms; heights 0.51, blendcb
0.50, regiontex 0.82, grass 0.32, vertex 0.27, physics 0.00. Meshlets exonerated by the 1.77
ledger note (MESH_SHADER_ALLOWED never on). ⚠ First split hid the BVH inside the renderdata
bucket — SetBVHEnabled(true) BUILDS synchronously when invalid (wiScene_Components.h:823).

Fix: `gg_generation_skip_bvh` — generator-only (bridge mirrors bProceduralLevel); drag-pick
rays fall back to brute force (hovergate probe verified); the 2.55 exit wipe means the
editor always regenerates WITH BVHs. Pregenerate also early-outs when already in-generator.
Fill: 33 s (start of day) → 24 s; during-fill FPS ~23-28 = the 2.57 budget finally fully
consumed (Lee's accepted trade); post-fill 83+ recovery; editor probe normal (134.4).

Named, NOT taken (both need their own verified pass):
1. ~875 chunks/entry still bake WITH BVHs during the flat-level LOAD (before bProceduralLevel
   flips), then get wiped (~4 s waste + the bvh=2.91 residue in GENPROF). The early flag
   (bProceduralLevelFromStoryboard) has an UNVERIFIED lifecycle across Generate→editor —
   suppressing on it risks a BVH-less or terrain-less EDITOR. Trace its resets first.
2. Fill is now MAIN-THREAD-INTEGRATION bound: halving generator-thread work moved the wall
   only 28→24 s. Next ceiling = merge + blend passes + VT residency on the main thread.

## §2.59 SHIPPED + the S1 mega-frame lead (08-15 evening, engine 35decb40 + game c6053994)

Lee's target: generator fill 12-15 s. Shipped safely this round:
- 2.59 entry-pending flag (OWN lifecycle + ~30s auto-heal; never reuse
  bProceduralLevelFromStoryboard — resets unproven) suppresses load-phase generation,
  Pregenerate and the reveal hold on generator entry. Entry events 1757 → 1611.
- Forensic yield of the bvh EVENT counter (engine 2.58b): the residual "bvh leak" was
  **MAX's STARTUP building a full 625-chunk ring (761 bakes, WITH BVHs) behind the hub**
  — pure boot-time waste on every launch (~7 s), a SEPARATE finding, untouched. The
  generator's own chunks bake bvh=0: the 2.58 skip is clean. ⚠ Cumulative-counter trap
  bit again: per-entry attribution must subtract the startup 761.
- Regression set green: enter/back/re-enter (skipbvh flips 1→0→1 correctly — the safety
  property), editor probe 134.1 FPS. TERRAIN_RING now shows pend/skipbvh/procLvl.

State: fill ~20-23 s (33 s at start of day). Generation-thread work is only ~2.6 ms/chunk
(~4 s total). **THE REMAINING WALL IS QUANTIFIED: ~25-29 recurring ~490 ms Scene-S1
"Anim+Transform" mega-frames ≈ 12 s of the fill.** SET_SCENESERIAL shows NO SU-* range
carrying it → the time lives in S1's UNINSTRUMENTED remainder: prime suspects are
(a) S1's jobsystem Waits starved by the 2.57 HIGH-priority generation dispatches
(the §33 job-thread trap — waits read as work), and (b) hierarchy/ComponentManager
ordered-insert storms on merge frames (Component_Attach is O(N) memmove per chunk into
the main scene's arrays). Two frame species seen: S1-spikes (~490 ms) and merge frames
(~67 ms, in the TerrainW range). NEXT MOVE: decompose S1's remainder (extend the
SCENESERIAL coverage or time the Waits separately), then fix per findings.

## §2.60 SHIPPED (08-15 evening, engine ab195c73 + game 64b8e959) — grass out of the generator; merge exonerated; the wall cornered

Lee's grass request measured FIRST: zero live grass systems exist during a generator fill
(HAIR_SYSTEMS=0 throughout) — grass was never the S1 monster, only ~0.3 ms/chunk of seeding
(~0.5 s) + his known generator grass bug. gg_generation_skip_grass now zeroes both; editor/
test game keep grass by construction (exit wipe + per-frame mirror). Verified: grass 0.32→
0.10, editor 134.8.

S1 decomposition, this round's verdicts:
- ★ Scene-S1 calls terrain.Generation_Update DIRECTLY (wiScene.cpp S1 block) — the bridge is
  a SECOND caller. The ring scan runs twice/frame; the idle gate + 2.59 suppression only ever
  governed the bridge copy. (Single-sourcing = clean-up candidate, ~1 ms/frame everywhere.)
- Generation_Update early-returns when the workload is busy (IsBusy → return) — no blocking.
- MERGE EXONERATED by MERGE_PROF: 423 ms TOTAL per fill, max call 5.1 ms (materials 199 +
  meshes 120). The ~490 ms "S1" profiler readings included SET_SCENESERIAL inflation.
- Gap tracer (the RIGHT tool, rediscovered): 32 gaps >100 ms per fill, all in `update`, with
  **texCreates bursts (+4..+22 per gap frame)** — plus one observed 66.8 ms bridge
  Generation_Update frame. THE REMAINING ~12 s WALL IS NOW CORNERED TO THE VT/REGION-TEXTURE
  ALLOCATION PATH inside the terrain update. NEXT: time CreateChunkRegionTexture's device
  calls vs the VT allocation/residency init in the bridge-call remainder, then batch or pool.

Fill unchanged this round (~24 s). Day total: 33 s → ~24 s with editor/test game untouched
(regression-proven every round: skipbvh/skipgrass flip 0 outside generator, Switch ~134).

## §2.61 SHIPPED (08-15 evening) — fill 24 s → 13 s: the wall was a BVH HEAL, not VT allocation

§2.60's closing attribution ("VT/region-texture allocation path") was WRONG — published off
a circumstantial correlation (texCreates bursts on gap frames). This round built the microscope
first and the microscope said no:

**Instrumentation (all durable):**
- VT_PROF harness command + gg_vtprof_* atomics: UpdateVirtualTexturesCPU total/max, its
  entry Wait, VirtualTexture::init, Residency::init (pool misses), main-thread "last minute"
  CreateChunkRegionTexture creates.
- gg_dbg_copywait_us/events in CopyAllocator::submit — the null-event SetEventOnCompletion
  there BLOCKS the calling thread until the copy queue drains (every with-initdata
  CreateTexture/CreateBuffer pays one). Gap tracer now prints copyWaits/copyWaitMs per gap.
- gg_trace_mark decomposition of the `update` segment: logic/gpup/ggcore/ggbridge/
  trees-grass/loop-render/pre-wicked/wicked-upd (master_part1), scn-begin/terrain/s1..s5
  (wiScene), tg-begin/merge/chunkloop/vtcpu/kick (Generation_Update).

**The refutation:** during a fill, UpdateVirtualTexturesCPU totals 0.36 s (max 5.5 ms/frame),
residency pool misses = ZERO, main-thread region-texture creates = ZERO, and the recurring
400–550 ms gap frames carry ~5 ms of copy-wait and ~4 texCreates. Not the VT path. The
2.60 texCreates correlation came from the handful of early entry frames only.

**The culprit (named by tg-chunkloop marks):** wiTerrain.cpp's per-frame chunk loop carries a
HEAL — `if (!chunk_mesh->bvh.IsValid()) SetBVHEnabled(true)` — which synchronously builds the
CPU triangle BVH ON THE MAIN THREAD for any chunk born without one. It silently DEFEATED the
2.58 generator skip: the 8.2 ms/chunk build never disappeared, it moved from the generation
thread (parallel) to the main thread (serialized into frames) — ~6 s of 400–550 ms frames per
fill. That is also why 2.58 "only" bought 28→24 s.

**Fix (one line + comment):** the heal honors gg_generation_skip_bvh. Generator chunks stay
BVH-less by design (drag picks brute-force; the 2.55 exit wipe regenerates WITH BVHs); every
other mode keeps the heal — editor/test game byte-identical behavior.

**Measured (filltime3, same recipe):** ring 1521 complete at +13 s (was +24/+25 s), FPS
DURING fill 82–110 (was 19–24), gap count 25 → 5 (survivors = entry one-offs: logic ~800 ms +
ggcore ~580 ms + three 170–215 ms first-burst frames, ~2.3 s total, one-time). skipbvh=1
procLvl=1 live throughout the fill; bvh events N=130 = pre-entry load-ring chunks only.
Lee's 12–15 s target: MET at 13 s. Day total: 33 s → 13 s.

**Second bug fixed en route:** TERRAIN_GENPROF RESET never zeroed gg_genprof_bvh_events —
a post-reset read showed N=1159 (cumulative since launch) and nearly relaunched the
"skip is inert" hunt. The 2.58b cumulative-counter lesson, this time inside my own reset
handler. RESET now zeroes it.

**Entry one-offs left on the table (~2.3 s, separate class):** the two ~1.1 s entry frames
(editor logic init + GGTerrain core priming at generator entry) and the first-burst frames
(trees-grass 165–207 ms, ggbridge 170 ms, copy-wait stacks from the entry texture burst).
Also still parked: startup 625-ring bake (~7 s of every boot), Generation_Update
single-sourcing (S1 + bridge both call it; ~1 ms/frame product-wide).

## §2.62 (08-15 late) — biome buttons live again on DX12: the reaction was missing, not the click

Lee's repro after confirming 2.61: DESERT does nothing (works on DX11). Built the navigation
instrument FIRST per his ask: **TERRAINGEN_BIOME [1-7|name]** injects a click through the
SHIPPED button branch (M-TerrainNew_part5's iRandomThemeChoice lane — the same one the
startup rainforest auto-click uses), no-arg dumps biome state + the reaction-chain counters.

**Root cause (2.54's leftover class, one layer deeper):** the button handler is byte-identical
to DX11 and works — probe showed sel/ptype/seed/slopemat/treebits all changing. What's missing
on DX12 is the REACTION: GGTerrain's CheckParams watcher (GGTerrain_part0 ~3510) detects the
params change and calls ResetChunks() + noise.reshuffle(seed) — but ResetChunks rebuilds the
LEGACY chunk system, which is dead code under wicked terrain. On DX11 that reset WAS the
visible terrain regenerating. The wicked ring was never told.

**Fix:** CheckParams' reset branch now calls GGTerrainWicked_NotifyParamsChanged(); the bridge
consumes it GENERATOR-ONLY (bProceduralLevel) with a 20-frame debounce (slider drags coalesce)
and a 60-frames-after-entry-wipe swallow (the entry auto-rainforest click's params are what
that fill already reads — reacting would double-fill). Editor/test game/level load: notify
discarded, flows byte-identical. Chain counters (cpRuns/cpResets/notifies/wipes in the
TERRAINGEN_BIOME readout) prove each link and stay as the debugging instrument.

**Verified:** click desert → wipes=1 within 0.3s → full 5km regenerates as desert (screenshot:
new height profile, desert.dat values in the panel, no leftovers) — refill faster than the 3s
polls could catch (mid-session wipe = warm caches; the 13s number is entry-path overhead).
Enter/back/re-enter guardrail re-run clean.

⚠ Harness lesson that cost two ghost rounds: `./build.bat Release | tail -2 && probe` runs the
probe even when the BUILD FAILED (pipeline status = tail's). Two probes silently ran a stale
exe and "refuted" a fix that was never in the binary. Gate probes on a real error count from
the full build log (grep -cE "error C[0-9]+|fatal error|: error LNK").

## §2.63 (08-15 late) — biome TEXTURES follow the click; ★ 2.61 fill + 2.62 heights Lee-CONFIRMED

Lee confirmed 2.61/2.62 ("generates new biomes and the heights look good") and reported the
texture half: each biome selects its own MATERIAL SET but the textures never changed on DX12.

**Root cause (the 2.62 story's second half):** terrain textures are a global catalogue
(terraintextures/matN); a biome selects INDICES into it — baseLayerMaterial + layerMatIndex[0..3]
(from the .dat via LoadSettings) + slopeMatIndex (set by the button) — all living in
ggterrain_global_render_params, which is watched by CheckParams' SECOND IsEqual branch. That
branch only refreshed the legacy page cache; the wicked materials resolve the indices ONCE in
SetupTerrainMaterial, so 2.62's geometry wipe rebuilt desert HEIGHTS under rainforest TEXTURES.

**Fix:** branch 2 now calls GGTerrainWicked_NotifyMaterialsChanged() — shares the 2.62 debounce
+ generator-only consumption; on fire it drops wickedTerrainMaterialsSetup instead of a plain
wipe. That path re-resolves the indices, reloads the DDS set, clears BOTH blend-pass key sets
and ends in its own Generation_Restart (a separate reset would double-fill). Chain gained a
matNotifies counter.

**Verified:** desert click → matNotifies+1, one wipe → full 5 km regenerated with SAND base +
desert slope rock (before/after shots night-and-day); editor discard proven live on the same
binary (notify arrived in editor state, wipes stayed 0).

**Probe archaeology this round:** Lee's own interactive session mutated TESTPRO2 (saved 20:16 —
'Level 1' node consumed/renamed; storyboard now 'Loading Screen'(EMPTY)/'sss'/'spotshadowtest')
which broke CLICK_NODE 'level 1' and burned two probe rounds on a phantom. Fix that stays:
CLICK_NODE's not-found error now LISTS every level node with (EMPTY)/(has-level) tags, and
biome262.sh parses it (falls back to CLICK add_level — RAM-only, probes never save).
⚠ Rule sharpened: when Lee messages mid-evening he was JUST TESTING LIVE — check project-file
mtimes/MAX liveness before taskkill-and-probe cycles; his session state is not yesterday's.

## §2.63b (08-15 late) — Lee's snow-with-sand-centre: a one-frame ordering hole, closed

Lee's screenshot after SNOW: the FIRST-generated cone chunks (centre, around the marker) wore
the previous biome's textures — and his instinct ("reminds me of the forward chunks we burst
through when loading a test game") named the population exactly: the closest-first cone.

Mechanism: the 2.63 consumption only DROPPED wickedTerrainMaterialsSetup — but the setup
check (GGTerrainWicked_Update ~2986) runs EARLIER in the function than the consumption
(~3160), so the drop wasn't seen until the NEXT frame, and THIS frame's generation kick
(~3200) still ran on the OLD material snapshot. The first cone chunks baked their one-shot
VT tiles from stale material data and kept them. Level load never shows this because setup
always precedes the first kick there (and the reveal cover hides the warm-up).

Fix: the consumption calls SetupWickedTerrainMaterials() SYNCHRONOUSLY (WaitForGPU +
re-resolve indices + reload DDS + blend-key clears + its own Generation_Restart) before this
frame's kick. Verified with the two-swap probe (rainforest→desert→snow, Lee's repro shape):
snow shot fully clean — no stale centre, lakes/slope-rock correct; chain matNotifies=3
wipes=2 across both swaps. Lee restored TESTPRO2's 'Level 1' node — probe found it first try.

## §2.63c (08-15 night) — the REAL stale-centre mechanism: reused material entities kept
## stale GPU-side ShaderMaterial data; the tile bake sampled the OLD biome through them

Lee refuted 2.63b (snow still green in the centre). The detective chain that finally named it
(every step an instrument, no lucky guesses):
- TERRAINGEN_MATS (new): slot dump proved the re-setup works (s1..s3 = snow mat12/9/13) and
  the painted-material map is EMPTY — paint theory dead.
- Time-lapse probe: the green is STATIC at +180 s — not a slow sweep.
- chunk(0,0) blend forensics (new): born=1, L3=100% — the blendmap is PERFECT, all weight on
  slot 3 which the CPU says is mat13 snow. Yet it renders olive.
- Pillow BC7 decode of the material DDS set: the olive = mat20 (68,69,19) — rainforest's HIGH
  slot material, the PREVIOUS occupant of slot 3. The paint is right, the slot is right, the
  bake sampled the slot's PREVIOUS material.
- Fast ring poll: the swap's restart DOES wipe (115→1521 in 5 s) — the green chunks were
  POST-restart bakes.

**Mechanism:** SetupWickedTerrainMaterials REUSED the 4 auto-slot entities. A reused entity
keeps its ShaderMaterial array slot, and the SVT tile-render CS reads that GPU-side data on
its own refresh cadence — tiles baked in the first seconds after the swap sampled the
OUTGOING set through the stale entries. This is the 2026-08-05 DEVICE_HUNG post-mortem's
exact mechanism in its benign edition: the 08-05 retention fix keeps the outgoing textures
ALIVE (no page fault), so the failure became silent wrong rendering. Centre cone bakes first
(closest-first) → stale window → green; outer chunks bake later → refreshed → white. Lee's
"the first chunks created" instinct was right from his first screenshot.

**Fix:** fresh entities for the 4 auto slots on EVERY re-setup (same idiom as the 08-05
stale-tail truncation; outgoing textures still ride gg_prevMaterialSetRetention). Fresh
entity = fresh array slot resolved at bake time — no staleness window.

**Verified:** snow-from-rainforest at +20 s: white edge to edge including the centre cone;
blend forensics unchanged (L3=100% onto mat13); swap refill ~5 s.

★ CLASS RULE for the ledger: the 08-05 "stale GPU-side ShaderMaterial" class has TWO faces —
page fault when the old texture DIED, and SILENT WRONG RENDERING when retention keeps it
alive. Anywhere a material's CONTENT is swapped in place under a one-shot GPU consumer
(VT tile bakes!), prefer FRESH ENTITIES over in-place rewrites.

## §2.64 (08-15 night) — marker grab/tooltip confined to the HANDLE (Lee's spec); the marker
## was never scene-pickable on DX12

Lee's spec after confirming 2.63c: clicking terrain must do NOTHING (it teleported the marker
then snapped back), the tooltip must only show over the marker handle, and only a handle-drag
may move it.

Hunt (two dead ends, each killed by an instrument):
1. The CURSOROBJECT-layer pick "hit" anywhere — wicked entities without a LayerComponent
   default to ALL layer bits, so terrain chunks pass every mask. First fix required the hit
   ENTITY to be the marker (new WickedCall_IsEntityOfObject)…
2. …which never fired. New PICK_AT harness command (client-coord pick + entity + marker-map
   + the marker's own frame entities) proved the ray passes straight THROUGH the transparent,
   z-depth-disabled marker into the chunk behind it — the marker was NEVER pickable on DX12;
   drags previously "worked" off the terrain hit by accident. (PICK_AT also had to bypass
   GetPick2's bImGuiGotFocus early-out — harness-time picks are otherwise dead.)

Fix: SCREEN-SPACE footprint test. The distance-proportional ScaleObject keeps the marker's
screen size ~constant, so a 28 px radius around its Convert3DTo2D-projected centre IS the
handle. Hover tooltip + drag START both use it; the drag itself still rides the terrain-layer
pick, and the grab offset now comes from the marker's own position (more stable than the old
ray-hit point). Click-anywhere teleport: gone (drag can only start on the handle).

Verified: hover bare terrain = no tooltip; hover handle = tooltip beside the marker; click
bare terrain = marker pixel-identical before/after (+4 s). Instruments kept: PICK_AT,
WickedCall_IsEntityOfObject.

★ Ledger: the 2.48 layer-cache family gains a THIRD face — layer masks cannot EXCLUDE
default-layer entities (all-bits), so "pick with mask X hit something" NEVER means "hit an
X-layer object". And transparent/no-zdepth UI helper objects may not be scene-pickable at
all — hit-test such widgets in screen space.
Deferred (Lee): after a successful drag the marker shows at the old position for a second
or two before the terrain regenerates.

## §2.65 (08-15 night) — marker release leaves the terrain alone; recentre folds invisibly
## into the Generate button (Lee's spec — closes the deferred §2.64 "marker ghost" too)

Lee, after testing 2.64: dragging the marker regenerates the ring at the new spot
beautifully (the 2.53 gen-center override follows the marker), and holding before release
even completes the fill — but RELEASING flicked back to the old centre view for a moment
and then regenerated everything from scratch. "Those last two events I feel are not
necessary." He's right, and both came from one place: the DX11-era release machine
(M-TerrainNew_part5, `movecameratotarget` 28-step): pan camera to the marker (steps 27-14),
write the drag delta into ggterrain_global_params.offset_x/z (step 13 — the world-recentre
commit), freeze presentation ~400 ms, snap the camera back (step 0), re-pin the marker to
GGORIGIN. On DX11 the freeze hid a fast synchronous rebuild; on DX12 the offset write
tripped CheckParams → the 2.62 params notify → full ring wipe + ~5 s refill, and the
camera snap-back + 10-frame GGORIGIN re-pin were exactly the "flick to the old centre".
The deferred §2.64 ghost was this same machine seen mid-flight.

**Fix (all game-side):**
- Release = NOTHING. Machine deleted; `bDraggingActive = false` is the whole handler.
  `movecameratotarget` survives only as an always-0 guard in a few UI conditions.
- Marker re-pin is now height-only at its OWN x/z (it used to force GGORIGIN — that was
  the visible snap). Entry still pins to origin; handle-scale math measures from the
  marker, not origin.
- The recentre bookkeeping moved to the ONE place that needs it: the "Generate Terrain and
  Open the Level Editor" button, BEFORE the player-start placement samples heights at
  world origin. New `GGTerrain_CommitGeneratorOffset(x,z)` writes global AND local params
  together, so CheckParams sees no inequality — no notify, no wipe behind the Save-As
  dialog — while height queries (CalculateHeightWithHeightmap reads LOCAL) and the terrain
  JSON save (also serialises LOCAL) already evaluate the new centre. Fold:
  `offset += MetersToOffset(UnitsToMeters(GGORIGIN - marker))` — the exact expression the
  old step-13 used, fed the marker's resting position (composes across multiple drags).
- Save-As CANCEL restores the pre-fold offsets through the same quiet commit — the
  generator session continues exactly as it was (marker off-centre, ring intact).
- The orange SHOW_MAP_SIZE boundary (engine debug lines since the 07-28 UI audit) now
  centres on the gen-center override when it's enabled (= the marker, generator only);
  everywhere else the override is auto-cleared and the box stays on world origin.

**Verified headlessly** (marker265.sh + manual C/D, real cursor):
- Release: marker stays at (23872,-50926) at +0.5 s/+2 s/+14 s, offsets untouched,
  notifies/wipes counters frozen, ring chunk count never dipped (min 2167), boundary box
  riding the marker. Shots pixel-still.
- Generate: Save-As opens; offsets fold EXACTLY (two sessions, two marker positions:
  Δ = marker × 0.0254 × 0.0007874 to the print digit), wipes still 0 — the fold is quiet.
- Cancel: offsets restore to the baseline to the digit, wipes still 0, generator resumes
  with marker + ring + view intact.
- Save path (fold → saved level has the chosen terrain at origin + player start on it)
  rides the same LOCAL-params reads — needs Lee's eye on a real Generate → Save.

**Instrument kept:** IMGUI_PROBE (io.MousePos, hovered/nav window, viewport + Save-As modal
rects) — built when the modal's Cancel refused two real-cursor clicks. It named the miss in
one shot: ImGui's client area starts 23 px BELOW the screen top (title bar at 125% DPI;
disp=1536x801, not x864). ★ Real-cursor probe rule refined: screenshot (x,y) → cursor
(x, y+23) for CLICKS. X is 1:1. Every earlier "working" click just had a tall target —
the marker's 28 px hover radius and the 52 px Generate button absorbed the 23 px error;
the short Cancel button did not. TERRAINGEN_BIOME no-arg dump now also prints marker=(x,z).

## §2.66 (08-15 night) — the camera glide restored (Lee: "a convenient feature I thought")

Lee liked ONE piece of the deleted release machine: the camera slowly scrolling to the new
marker location on release. Restored as a pan-only glide: on release the drag delta
(marker-at-release minus marker-at-grab, `fHitOffset*`) feeds the old `movecameratotarget`
counter — 16 linear steps with an exact landing on the last (the original stepped delta/16
for ~14 frames then jumped the remainder; same speed, no jump). Nothing else came back:
no offset commit, no presentation freeze, no snap-back, no re-pin — the 2.65 rules hold.
Verified headlessly: post-release shot shows the view scrolled so the marker lands framed
where it was grabbed (boundary box fully in frame), terrain chunk-identical across the pan,
offsets/notify/wipe counters and chunk floor all frozen, marker world pos unchanged.

## §2.67 (08-16 night) — generator Vegetation checkbox live on DX12 + RMB look pins the pointer

Lee's pair after confirming the 2.66 glide: (1) unticking Vegetation left the grass visible;
(2) right-button camera drags let the pointer drift off the terrain view.

**(1) Vegetation:** the checkbox only wrote gggrass draw_enabled — which gates the DEAD
legacy draw path. The editor's View Options checkbox learned this exact lesson in the 07-28
UI audit (M-GridEditB_part3.cpp:2085 → GGTerrainWicked_SetGrassVisible sweeps the live hair
entities); the generator's copy of the checkbox never got the call. Fix: one line — the
generator's veg block now drives GGTerrainWicked_SetGrassVisible every frame (early-outs
when unchanged; the same gate also stops grass CREATION while off, so chunks born hidden
stay grassless and a re-tick rebuilds).

Verification was a hunt of its own — three false trails before the instrument:
- The rainforest "grass" band at the shore is TREES; POLYS doesn't count hair strands; and
  at camera heights above the short grass view distance there are simply NO strands (and
  none get CREATED — the build radius hugs the camera), so screenshots and POLYS both read
  "no change" regardless of the toggle. Static terrain shots are bit-identical frame to
  frame (no TAA in the generator) — a 0.000 diff means "nothing was ever there", not "broken".
- New instrument closed it: TERRAINGEN_BIOME now prints
  `grass drawEn/wOn/hairs/vis/strands` (GGTerrainWicked_GetGrassDebug). Ground-level cycle:
  OFF: hairs=0 → ON: hairs=26 vis=26 strands=912000 → OFF: vis=0 (systems retained) →
  ON: vis=26. Both directions, no one-way trap; drawEn and wOn flip in lockstep.
- Note for testers: biome buttons RESET the Trees/Vegetation ticks to the biome profile's
  defaults (Plains = both OFF by design, M-TerrainNew_part5 biome branches) — grass
  "disappearing" after a biome click is the profile, not the toggle.

**(2) RMB pointer pin:** classic mouselook — on look-drag start capture the OS cursor + the
ImGui mouse pos as anchors; each frame consume io.MouseDelta, SetCursorPos back to the
anchor, and write io.MousePosPrev = anchor so the NEXT frame's delta is measured from the
anchor (without that fixup every second frame's delta subtracts the previous frame's
movement). Also fixes the drag dying at the view edge (the branch requires bIsItemHovered).
Verified: 250 px of synthetic drag → cursor ends exactly at the anchor (ImGui's ~6 px drag
threshold means the anchor locks a step into the gesture), view rotated, pointer never left
the view.

## §2.68 (08-16 night, OPEN — evidence gathered, fix not attempted) — bottom-of-window
## black band during generator churn (Lee's report: entry + water-slider hold)

Lee's physical screenshots: bottom of the WINDOW (3D view AND panel UI alike) goes black
below a horizontal line that flickers at varying heights, with a smeared garbage seam at
the boundary. Triggers: generator entry, and click-holding the Water Height slider
(his FPS during the hold: 10-24; he also had Editable Area Size at 5.0 Km).

★ Instrument lesson first: the harness SCREENSHOT (backbuffer readback) is BLIND to this
artifact class — dozens of generator captures all session never showed it. Physical
CopyFromScreen captures were needed (new blackband.sh probe).

EVIDENCE (all headless, reproduced on demand):
- Slider hold, stock queues: 37/40 physical frames show the band, height varying 21-161
  rows, always terminating at the taskbar edge. Entry: 0/90 on my rig (Lee's entry trigger
  likely needs his 5 Km editable-area setting — heavier entry).
- SET_SINGLEQUEUE 1: 6/18 frames vs 13/18 stock — REDUCES ~2x but does NOT eliminate.
  So this is not purely the async-queue split; incidence scales with timing.
- A backbuffer SCREENSHOT taken mid-hold came back fully BLANK (cleared, nothing composed)
  — under the churn there exist capture points where the frame is clear-but-unwritten.

READ OF THE APPEARANCE: the presented buffer's lower rows never received this frame's
composition — black = clear colour, the seam = the write frontier, varying height = how
far the frame got. A present/scanout that samples a cleared-but-partially-composed buffer
under very long frames (vsync-off windowed swapchain). The band swallowing the ImGui panel
too says it is the FINAL swapchain buffer, not a scene pass.

CONTRIBUTING PATHOLOGY (separate, game-side, cheap to fix): holding ANY terrain slider
runs CheckParams branch 1 EVERY frame → the LEGACY ResetChunks() fires per frame (the 2.62
wicked notify is debounced; the legacy reset is NOT) — that is why a slider hold runs at
10 FPS at all. Shrinking hold-frames to normal would make the band rare in practice, but
it is masking; the present-path gap is the disease.

NEXT (needs engine instrumentation, not attempted at 1am): fence/buffer-index logging at
Present in wiGraphicsDevice_DX12 SubmitCommandLists (which buffer, which fence value, was
the compose batch submitted before Present) during a slider hold; then the vsync A/B.
Probe: blackband.sh (physical-capture loop + band detector). The A/B harness lever
SET_SINGLEQUEUE is already wired.

## §2.68a (08-16 night) — water-slider hold debounced: 4.3 -> 84.5 FPS, band 13/18 -> 0/18

⚠ CORRECTION to §2.68's guess: the profiler REFUTED "legacy ResetChunks every frame" —
a Max Height slider hold runs at 6 ms / 85 FPS (CheckParams' per-frame branch is cheap on
this content). The real villain was the WATER slider handler specifically: every change
during a hold fired `ggterrain_extra_params.iUpdateTrees = 1` (consumer = "Max - Tree
Update": 152 ms/frame, profiler-named) + `Wicked_Update_Visuals` (~23 ms, the CL-EntityProps
spike) + ~24 ms knock-on in common_loop = 228 ms frames, 4.3 FPS.

Fix (2.62 settle pattern): the handler's VALUE writes (gdefaultwaterheight / waterliney_f)
stay immediate; the reactions moved behind a 20-frame countdown re-armed per change, fired
once from the generator's always-running block (after the fLastY fog sync — bTriggerStableY
and fLastY are function-local statics, the consumer must sit past both declarations). Both
the slider AND its numeric InputFloat share the debounce. Time-of-Day combo untouched
(one-shot).

Measured on the same probes as §2.68:
- Mid-hold: 228 ms / 4.3 FPS -> 6.14 ms / 84.5 FPS; "Max - Tree Update" 152 -> 0.00 ms.
- Black band: 13/18 physical frames -> 0/18 (same wiggle geometry). The §2.68 present-path
  gap still exists underneath — it just no longer gets the >100 ms frames it needs; the
  engine-side fence/Present hunt stays open for a heavy-load day.
- Settle correctness: slider dragged 13.4 m -> 1500 m and released — one reaction burst
  ~0.25 s later, world correctly submerged, 79 FPS steady after.

## §2.68b (08-16 night) — view-toggle camera presets follow the MARKER (Lee's report)

Since 2.65 the marker legitimately rests away from origin, but the generator's camera
presets still aimed at fixed world-origin coordinates: toggling top-down/3D after moving
the marker left it off at the edge of the view. Three sites made marker-relative in
M-TerrainNew_part5: the 3D-view button (the fixed vantage becomes an OFFSET from the
marker — identical framing when it sits at origin), the top-down button (centres on the
marker), and the Editable-Area-Size slider's camera reset (same). Snapshot mode untouched
(it photographs the real map at origin, correctly). bTriggerStableY already samples at the
camera. Verified numerically via NEW `markerScr=(x,y)` in the TERRAINGEN_BIOME dump (the
marker's projected screen position, published by the 2.64 footprint block): after a drag
to (18819,-61225), top-down put markerScr at (768,400) = exact backbuffer centre, and the
3D toggle re-framed it at the same spot the release glide had left it.
★ Probe rule: NEVER hardcode marker grab coordinates — its screen Y depends on the ground
height at its position (seed-dependent: 384/410/414 across three sessions tonight). Read
markerScr from the dump. Also: powershell escaping DIES inside `bash -c '...'` one-liners
(every mouse_event silently failed for one whole probe run) — real-cursor probes must be
.sh FILES.

## §2.68c (08-16 night) — "Disable Level Aspects" re-gated to the EMPTY biome (DX11 parity)

Lee's spec: only the EMPTY biome should offer the Completely Empty Level machinery. DX11
confirms (M-TerrainNew.cpp:10159: the whole section sits in `if (iSelectedThemeChoice ==
8)`). The DX12 copy had that gate until 2026-08-09, when a session removed it by request —
Lee's instruction tonight supersedes that: gate restored, the 08-09 snapshot/restore
machinery kept (DX11's untick is a one-way door that only puts terrain back; ours restores
everything the tick took away). Both checkboxes verified live on the wicked side:
- Completely Empty Level: its Wicked_Update_Visuals lands in the visuals sync
  (M-GridEditB_part3) that drives GGTerrainWicked_SetGrassVisible + the terrain-visible
  lever + bEnableEmptyLevelMode gating — tick AND untick reach the live entities.
- Do Not Generate Navmesh: live consumer at M-Game_part0.cpp:392 + save/load via
  visuals.EnableZeroNavMeshMode.
Verified: Rainforest panel = section ABSENT; click Empty (sel=8, blank grey grid) =
section PRESENT with both checkboxes.

## §2.68d+e (08-16 night) — Empty-mode exit on biome click + the grey grid finally hides

Lee's pair: (d) Empty -> tick both boxes -> click MOUNTAIN = nothing generates; (e) a saved
Completely Empty level still SHOWS the grey grid in the editor and test game (collision
correctly gone — heights honor the flag, the visuals did not).

(e) is the 07-28 audit lesson's FOURTH occurrence: Wicked_Update_Visuals' empty-mode branch
(M-GridEditB_part3) only cleared ggterrain_draw_enabled/ggtrees_draw_enabled — both gate the
DEAD legacy draw path. One line: the branch now also calls GGTerrainWicked_SetTerrainVisible
(false) (the live chunk-object sweep; grass was already handled by the unconditional sync
above it). The normal path re-shows per the usual checkboxes. Same function serves the
generator, the editor AND test game — verified in the generator: tick -> grid vanishes.

(d): bEnableEmptyLevelMode is a master kill switch and NO biome button cleared it — DX11
has the same latent trap (only its Empty button resets the flags; DX12's Empty button had
that parity already). New transition block after the biome buttons: leaving theme 8 with
empty mode active exits it — clears both master flags (matching the Empty button's own
reset), restores the editable area the tick blew to 25 km (snapshot if taken this session,
else the 2.5 km default), re-shows the edit-area markings, consumes the checkbox snapshot
(its trees/water values would stomp the NEW biome's settings — discarded; the handler that
just ran re-specified all of那些). The snapshot statics hoisted to file scope for sharing.

Verified headlessly (empty268.sh): Empty -> tick Completely Empty = grid GONE (D1 shot,
marker floating in void) -> tick navmesh box -> click Mountain = full mountain terrain,
sel=6, ring 1521/1521 rebuilt, editsize back to 50000 (2.5 km), section hidden again.

## §2.68f (08-16 night) — hidden terrain STAYS hidden across chunk rebirth (Lee's fresh-load
## repro on ssss8.fpm)

Lee: the freshly SAVED empty level was correct, but a fresh LOAD showed the grid again.
Mechanism: SetTerrainVisible(false)'s hide sweep only reaches chunks that exist at that
moment. A level load applies visuals early (empty flag -> hide fires with few/no chunks
alive), then rebuilds the ring — and the engine-side Generation_Update (the Scene-S1
direct caller, independent of the bridge's wickedTerrainHidden early-out) births new
chunks RENDERABLE. Nothing re-asserted the hide.

Fix: the bridge's hidden early-out now sweeps any renderable chunk back to non-renderable
every frame while hidden (~1500 flag checks, empty-mode only).

Verified with a forced rebirth: Empty + tick (void) -> SET_TERRAIN_GEN 21 grew the ring
1521 -> 1849 WHILE HIDDEN (proving the engine-side generation runs under empty mode) ->
view stayed void; post-grow shots bit-identical across 5 s (mean diff 0.000).

Noted, not chased tonight:
- A ~1 px dithered hairline at the far-plane horizon survives in the empty void (contrast
  ~20/255, only visible contrast-stretched; present BEFORE the rebirth too) — a flat-plane
  silhouette seam of some far-plane surface, sub-visible at normal viewing.
- Empty mode still BURNS generation work + VRAM building an invisible ring (1849 chunks!).
  Real fix = stop generation while hidden; tied to the parked Scene-S1 dual-caller
  single-sourcing cleanup.

## §2.68g (08-16 night) — Completely Empty mode made SELF-ENFORCING (Lee's ssss9 re-repro)

2.68f was not enough: Lee's fresh load of ssss9.fpm still showed the grid. The flag DOES
serialize (visuals.EnableEmptyLevelMode save/load in M-Visuals_part0; his collision-gone
observation proves it arrives true) — but the entire hide wiring hung off
Wicked_Update_Visuals being called AFTER the visuals parse, and the fpm load path does not
guarantee that ordering, so wickedTerrainHidden was simply never set there. The 2.68f sweep
keyed off that same flag = also inert on this path.

Fix: the bridge polls the mode DIRECTLY every frame — new game-side accessor
GGGame_IsEmptyLevelMode() (M-TerrainNew_part4, returns t.visuals.bEnableEmptyLevelMode;
global-scope extern in the bridge per the 2.53 linkage rule) OR'd into the hidden early-out.
No call-order dependence left. Symmetry hole closed too: when empty mode ends while the UI
hide is off, SetTerrainVisible(true) early-outs (wickedTerrainHidden never flipped), so a
one-shot re-show sweep (s_ggEmptyModeSwept) restores what the empty poll hid — otherwise
unticking would have left the terrain invisible forever.

Verified (empty268g.sh): tick = void; ring grown 1521 -> 1849 WHILE ticked = still void;
UNTICK = grid returns immediately (re-show path), editable box + Terrain Size restored.
★ Ledger rule: a mode flag that must gate a live wicked state should be POLLED by the
consumer, not pushed through an update function whose call ordering the flows don't
guarantee — this is the second push-vs-poll failure this week (2.62 CheckParams was the
first).

## §2.68h+i (08-16 night) — THE ssss10 mystery solved: it was never wicked terrain
## ★ Lee CONFIRMED 03:30: "the empty level now loads invisible on the fresh load"

Lee's re-repro (ssss10 -> spotshadowtest -> ssss10 = grid back) survived 2.68g because the
visible surface was NEVER the wicked chunks. Instrument chain that cracked it:
- emptyV/G/E + hidden added to the TERRAIN_RING dump (2.68h): ALL flags true, hidden=1 —
  the poll/hide machinery was working, yet the surface rendered.
- Terrain-mask PICK_AT MISSED the surface; all-mask hit a flat plane at Y=0.0 exactly
  (the empty biome's flat height), entity id 1021 (created very early).
- POLYS ~138k = the real terrain genuinely hidden.

MECHANISM: master_part1's terrain block is wrapped in `if (bEnableEmptyLevelMode == false)`
— and INSIDE that block is the wicked branch that clears ggterrain_draw_enabled EVERY
frame. In empty mode the whole block is skipped, so the LEGACY GGTerrain draw callbacks run
with a stale flag (default 1 on a fresh launch) and render the old-path terrain — grey
checker on Lee's rig, green legacy set on mine, flat at Y=0. "Dead code under wicked" is
only dead because a live-path line keeps re-killing it every frame; skip that line and the
corpse gets up. The generator never showed this because its flows keep the block running.
Also: with GGTerrainWicked_Update skipped, the 2.68f/g newborn-chunk sweep never ran in
empty levels either (engine-side Generation_Update births renderable chunks regardless).

FIX (2.68i): before the gated block, `if (empty) { ggterrain_draw_enabled = 0;
GGTerrainWicked_EnforceHidden(); }` — the sweep extracted into a callable so the mode
enforces both kills even while the bridge update is skipped.

VERIFIED — Lee's exact chain, headless (switch268i.sh, storyboard hops via the editor back
arrow): ssss10 first load = VOID; spotshadowtest = normal terrain (flags 0/0/0/0, re-show
path clean); ssss10 again = VOID. Flags flip 1->0->1 in lockstep.

Also in 2.68h: the Completely Empty tick is LOCKED (disabled + tooltip) until the Empty
biome's grid ring finishes generating (Lee: ticking mid-fill left half-and-half), via new
GGTerrainWicked_IsRingComplete(); unticking is never locked.

★ Ledger rule (the real lesson): "the legacy path is dead" was TRUE only as an emergent
property of a per-frame suppression line inside the live path. Any gate that skips the
live path resurrects the legacy one. When auditing "dead" DX11 code, ask WHO keeps it dead
and whether every mode runs that keeper.

## §2.69 (08-16 night) — standalone export: no more debug screen at boot (Lee's TESTPRO2 report)

Lee saved a standalone from testpro2 and the exported game booted showing the raw Wicked
init BACKLOG — every wi:: init line plus a wall of red "shader compile FAILED:
shaders/ffx-fsr2/..." errors.

Two packaging holes in `mapfile_savestandalone_stage4` (M-MapFile_part2.cpp), both fixed:

1. **The shaders copy loop lists loose FILES only** — `ChecklistForFiles()` on `shaders/`
   returned the 834 loose .cso/.wishadermeta but never the `ffx-fsr2` SUBFOLDER (the only
   subfolder; 16 files). The standalone's FSR2 loads failed and fell back to recompiling
   from the shader SOURCE dir — an absolute dev path (`D:/max/WickedEngineDX12/...`)
   glued onto the game's CWD, hence the surreal
   `My Games/TESTPRO2/D:/max/...` paths in the errors. Fix: explicit second copy pass
   for `shaders\ffx-fsr2`.

2. **`splash_screen.png` never shipped.** wiApplication.cpp:155-206 is the mechanism: while
   `wi::initializer` runs, the engine draws `<exe dir>/splash_screen.png` if it exists,
   ELSE it renders the backlog as text. The editor dir has the png (neutral grey→black
   gradient, no branding) which is why the editor never shows the debug text; the export
   never copied it, so standalones got the fallback. Fix: copy it next to the standalone
   exe.

Both fixes are additive file copies — zero engine change, zero behavior change for the
editor. Lee's existing TESTPRO2 export was hand-patched with the same two payloads
(splash png + 16 fsr2 files) so re-running the already-exported exe shows the fix without
a re-export; the next export does it automatically.

Residue noticed while in there (NOT fixed tonight, chip spawned): standalones accumulate
instrument droppings — gpup_trace.txt is written UNCONDITIONALLY at init
(GPUParticles_part0.cpp ~2049), gap_trace.txt/alloc_tripwire.txt are engine-side. The
natural gate is the standalone's own setup.ini `producelogfiles=0`.

## §2.70 (08-16 night) — standalone SAVE GAME wrote to a slot no reader could find
## (Lua 5.4 float filenames)

Lee: standalone SAVE GAME "does not save the game position into the selected slot."

The save was NEVER lost — his slot file sat in Files\savegames complete with position,
angles and level name. It was named **gameslot1.0.dat**. Every reader — the C++ slot
lister (M-GridEditB_part22 `gameslot%d.dat`), fillgameslots.lua's integer loop — looks
for **gameslot1.dat**. Save vanishes into a filename nothing else can see; slots report
EMPTY PROGRESS SLOT forever.

Root cause is a Lua VERSION regression in the port, not save logic:
- DX11 DarkLUA embeds **Lua 5.2** — one number type; `1 .. ""` prints `"1"`.
- DX12 DarkLUA compiles against **WickedEngine's Lua 5.4** — integer/float subtypes;
  every C++ push used `lua_pushnumber` (float), and a float 1 concats as `"1.0"`.

Two leak paths, both C++ pushes:
1. Slot number: `DisplayScreen()` returned the clicked widget via `lua_pushnumber(L,
   iSpecialLuaReturn)` → savegame.lua `gamedata.save(1.0, ...)` → gameslot1.0.dat.
2. Level state: `LuaPushInt(g_Storyboard_Current_Level)` → global.lua composes
   `"0-"..storyboardnodeid` → gameslot0-14.0.dat (same disease; the new-game cleanup
   loop in M-LUA.cpp deletes integer names only, so these also dodge deletion).

Fix (game-only, no engine change):
- DarkLUA_part8.cpp: LuaSetInt ×2 + LuaPushInt ×2 now `lua_pushinteger` — restores
  exact DX11 string behavior for every int the game hands to scripts.
- DarkLUA_part4.cpp: DisplayScreen + DisplayCurrentScreen return `lua_pushinteger`.
- gamedata.lua (build area + BOTH repo copies GameGuru Core/GameGuru/titlesbank and
  Scripts/titlesbank): defensive `math.floor` on numeric slotnumber at save AND load
  entry, so pure-Lua float arithmetic can never mint a float filename either.

Lee's existing TESTPRO2 export: patched gamedata.lua in, renamed his 03:52 save
gameslot1.0.dat → gameslot1.dat — his save now shows in slot 1 on the OLD exe. A fresh
export from this build carries everything.

★★ LEDGER RULE (bug class, not one-off): **DX12 runs Lua 5.4, DX11 ran Lua 5.2 — any
C++ int reaching Lua as a NUMBER prints with ".0" the moment a script concats it into a
string.** Filenames are where it turns fatal (reader/writer split), but HUD text built
by concat has the same exposure. When a "file not found"/"slot empty" bug appears under
DX12 with files visibly present, CHECK THE FILENAME BYTES first — the content was never
the problem here. Registered C functions still push via lua_pushnumber in many places;
sweep pending (chip).

## §2.70a — ★ Lee CONFIRMED (08-16 ~04:1x): "save and load now work in the standalone"
Both standalone fixes of the night are user-confirmed closed: 2.69 (boot shows the quiet
gradient, no debug backlog, no FSR2 errors) and 2.70 (SAVE GAME slots persist and load).

## §2.70b (08-16 night) — the Lua 5.4 integer sweep: 268 more push sites converted

Follow-through on the 2.70 bug CLASS (chip): every remaining direct `lua_pushnumber` in
DarkLUA_part0-8 was classified by the STATIC C++ TYPE of its expression — int-typed
converted to `lua_pushinteger`, float-typed left alone, nothing decided on naming
convention (the sweep caught convention-breakers both ways: float fields with no `_f`
suffix like `gunsettingstype.jamchance`, and integral oddballs like `footfallcount`
(long long) and bool flashlight flags).

Numbers: 611 total sites → 268 converted / 342 left float / 1 comment hit. Verified by
diff symmetry (268 pushnumber lines removed ↔ 268 pushinteger code lines added; only
WrapAngle went the other way, see 2.70c) and a float-pattern scan over every added line
(single hit = ODERayTerrain, whose RETURN is int per BulletPhysics.H:119 — args are the
floats). Four parallel subagents did the classification; every count reconciled by grep
before trusting (part4's own tally was off by one — the diff is the truth).

Smoke: editor → testpro2 → spotshadowtest level load, 139 FPS, ZERO Lua errors.
Pre-existing pushinteger sites (55 in part0 — weapon slots, GetTerrainHeight's
`lua_pushinteger(L, fReturnHeight)` etc.) were left untouched: they predate the port
notes and match DX11 behavior.

## §2.70c — WrapAngle returns the smoothed FLOAT now (DELIBERATE DX11 deviation)

Lee's chip asked for `lua_pushnumber` at DarkLUA_part0.cpp:969 (WrapAngle truncated its
smoothed angle). Verified before changing: DX11's DarkLUA.cpp has the IDENTICAL
`lua_pushinteger` there, and Lua 5.2 truncated floats through that call the same way —
so whole-degree WrapAngle is INHERITED behavior on both renderers, not conversion
fallout. The change is therefore an intent-fix (a smoothing function that can now
converge below 1°), recorded in-code as a deliberate parity deviation with a one-line
revert. Watch item: any stock script tuned around the old ±1° stall.

## §2.71 (08-16 night) — producelogfiles=0 now gates the diagnostic trace FILES
## (standalone folders stay clean)

Lee's chip: standalones accumulate instrument droppings. Full two-repo census first
(3-agent workflow): 46 game + 21 engine writer sites classified by trigger. The policy
line drawn: EVERY-RUN/ROUTINE writers gated; on-crash (Guru-Crash.log, crashdump.dmp,
dred_report.txt), anomaly-only tripwires (gg_pso_fail, resource_hijack, stream_guard,
corrupt_geometry), engine log.txt (support value, rewritten not accumulated) and all
~25 harness on-command dumps KEPT everywhere.

Gated (all default TRUE, cleared by GGSetDiagTraceFiles when producelogfiles=0):
- gpup_trace.txt (GPUParticles_part0.cpp — fresh every launch)
- reload_quiesce.txt (wickedcalls_part2.cpp — appended EVERY level load, forever)
- videotrace.txt (CAnimation_part0.cpp — 15 call sites on every video op)
- alloc_tripwire.txt (engine wiAllocator.h — a PER-ALLOCATION ledger, not fire-only:
  549 MB accumulated on the dev machine; tracking + graceful reject stay LIVE, only
  the file is gated)
- gap_trace.txt (engine wiProfiler.cpp — counters + hitch histogram keep feeding
  GET_PERF_DATA, only the file is gated)
- anim_garbage.txt + applytransform_garbage.txt (engine tripwires that fire on STOCK
  content via parrot load-time shear — sanitizers keep healing, files gated)
- contents.txt: the export itinerary is no longer written at all (zero readers in
  either repo; restore lines preserved in the M-MapFile_part2.cpp comment)

Wiring: `producelogfiles` joins GetSetupIniEarly's exact-match key list (iKeyLen 15;
the '=' test keeps producelogfilesdir from cross-matching) because the allocator ledger
and gpup_trace fire BEFORE FPSC_LoadSETUPINI — the same ordering trap that bit lazypso
(1.79/1.82). The normal parse calls GGSetDiagTraceFiles again for consistency. Engine
delta 2.71 in WICKED_ENGINE_CHANGES.md (re-apply on upstream pull). One linker lesson
re-learned: GPUParticles_part0.cpp sits ENTIRELY inside `namespace GPUParticles` — the
bridge extern needed the namespace (2.53 rule, now bitten twice).

VERIFIED both ways on one binary: producelogfiles=1 editor run → traces written
(gpup_trace fresh); producelogfiles=0 run to hub → 8/8 trace paths ABSENT; setup.ini
restored. Standalone exports already write producelogfiles=0, so a fresh export goes
quiet with no further action. NOTE: the automation harness still runs in standalones
(auto_command.txt watcher) — left as-is deliberately (we drive standalone tests with
it); flag for Lee whether shipped games should disable it.

## §2.71a — ★ Lee CONFIRMED (08-16 ~05:0x): "the re-exported standalone folder is clean
## and save/load still works"
The full standalone arc 2.69/2.70/2.70b+c/2.71 is user-confirmed closed end-to-end on a
fresh export. Lee's parting instruction: run the ★ MILESTONE full DDS conversion of the
entire Files\ media tree (authorized NOW — internal tester build wanted), then if time
allows a full 19-demo hub sweep with fresh FPS + VRAM readings for his afternoon return.
6-hour autonomous window starts here.

## §2.72 (08-16 morning) — ★ MILESTONE EXECUTED: full stock DDS conversion (Lee-authorized
## for the internal tester build)

Scope per MILESTONE_DDS_CONVERSION.md: the whole Files\ tree, 17,063 DDS scanned.

**Converted: 1641 files, 4058 → 5489 MB on disk (+1431 MB, +35.3% — the documented mip
overhead), zero unconverted failures.** Protocol = the 08-04 entitybank pass extended:
format-preserving texconv, per-file mirror backup to D:\max\mipbackup (no-clobber, so
the 08-04 originals are intact), per-file post-verification (dims/format-family/chain),
auto-restore from mirror on any failure. Driver: tools/ddsconvert.py (kept in repo).

Three texconv landmines found and encoded into the driver, each worth remembering:
1. **-m 0 does NOT mean "full chain" for sources that already have partial mips** — it
   keeps the source's count (only single-mip sources get full chains). 124 first-run
   "failures" traced to this + the next item; fix = explicit -m <full>.
2. **-dx9 BC output stops the chain at the 4x4 block floor** (2 levels short of 1x1) —
   standard for legacy headers, irrelevant to streaming (<4x4 is far under the 64KB
   floor); the verifier accepts full-2 for BC.
3. **-dx9 BC on non-power-of-2 dims truncates where mips stop being multiples of 4**
   (e.g. 1440² dies at mip 5) — those 24 files got a -dx10 retry, which carries the
   full chain (same header family as our BC7s).

The only non-format-preserving conversions, forced (no lossless mapping exists):
13 legacy 24bpp Cybernoir gun textures → B8G8R8X8 (the engine already promoted them to
32bpp at load), 1 D3DFMT_A16B16G16R16 normal → R16G16B16A16_UNORM (identical layout).

**Acceptance gate: single-mip >64KB went 1617 files / 3977 MB → ZERO streamable files.**
The one listed remainder is dreamnebulamoon_cube.dds — a CUBE map, structurally outside
the streaming reduction (the engine's shed loop skips cubes), excluded by protocol along
with 10 other cube/volume assets. 94 pre-existing alignment-blocked files are a
dimension property (1024x936 etc.), not a chain defect — unchanged by design.

**Female head 15 surface fix folded in** per the doc: FIXED.png → BC3 12 mips, replaced
in charactercreatorplus (fixes heads 15 AND 15b — shared file), original mirrored. The
AO channel remains flat-255 (re-bake = authoring, per the README).

**The _surface.dds degenerate-channel rider (tools/surfacescan.py) reframes the doc's
suspicion**: 5005 of 5022 surface maps have at least one flat channel — flat occlusion
(1894), flat metalness/reflectance are the CONTENT-WIDE CONVENTION, not defects (a
wooden prop with constant metalness 0 is correct). The interesting subsets for future
content QA: 700 all-four-flat pure placeholders and 816 flat-roughness maps (uniform
gloss response — head 15's actual defect class was BAD roughness, not flat channels).
Full report: tools/surfacescan_report.txt. No content action taken — report only.

19-demo sweep (RUNTAG 0816) launched on the converted tree; results follow as §2.72a.

## §2.72a — the milestone acceptance sweep (0816): ALL GATES HOLD, VRAM DOWN ON ALL 19

Full table in DEMO_FPS_SWEEP.md (2026-08-16 section); raw results tools/sweep_0816_2.72.txt.
Headline: 19/19 loaded, POLYS bit-identical on all 19 (the texture-only proof), 4 GB gate
holds with headroom UP (Aztec Game Kit 3987 → 3852, headroom 109 → 244 MB), in-game VRAM
fell on every demo (−22 to −135 MB). FPS: hub-wide editor −4.6% / game −3.9% = ambient
drift band; Bounty +16-18% and Grand Canyon in-game +28% improved; Aztec Teaser −12%/−10%
and ISF game −11% flagged at the band edge for an eyeball, consistent with warm-up
sampling. The MILESTONE's acceptance criteria are met in full. Streaming remains DEFAULT
OFF — the tree is now uniformly mip-complete, which is the stated precondition for
revisiting task #37. **[CORRECTED 2026-08-18: this is WRONG — streaming is and was ON by default; it defaulted 0 for one day (08-01) and was restored the same day by delta 1.73. See STREAMING_STATUS_2026-08-18.md.]**

## §2.73 — "circle image on each cube side": the base env cube map named, the twilight pool bake fixed (08-16 afternoon, #155)

Lee's afternoon report, two symptoms on the beach test level: (1) dropping an env probe
and sliding its range briefly shows the BASE env cube map — "a circle image on each cube
side" — before the local capture takes over; (2) DX12 sand is shinier/wetter than DX11,
with a vertical reflection band matching that corrupt-looking base map.

FIRST SUSPECT CLEARED: the sky cube files. skybank was converted 08-04 (R11G11B10 →
BC6H_UF16, 128→32 MB each) — face-by-face and mip-by-mip decode against the D:\max\mipbackup
originals shows the conversion FAITHFUL, and the originals' mips were already plain box
filters (never GGX-prefiltered). The sunny down-face "dark ball on radial rays" is IN the
Feb-2022 source asset. Also irrelevant anyway, see next.

THE ARCHITECTURE (3-agent trace, grep-verified): reflections NEVER sample the skybank
_cube.dds. `shaderscene.globalenvmap` (the raw sky file) is sampled at MIP 0 only, by the
sky backdrop + probe-capture backdrop (skyHF.hlsli:185/189). What every shiny surface falls
back to is `GetScene().globalprobe` = probes[0] = GGTerrain's always-created
`globalEnvProbe` (GGTerrain_part0.cpp:7485) — a 128px, 4-mip, BC6H capture at world
(0, terrain-height, 0), GGX-filtered by RefreshEnvProbes. MIP = roughness × 4. "The base
env cube map" IS this capture. DX11 was the same shape (128px live probe, 8 mips,
GGX-filtered, slot 0 at the camera) — the raw cube was never a reflection source there
either.

THE RUNTIME PROOF (new instrument DUMP_ENVPROBE, WETEST.md): on Island Showdown the
global probe's capture is a plausible sky+island cube (with a faint BC6H banding bullseye
at the sun position — BC6H terraces a smooth radial halo into concentric rings; DX11's
probe array was uncompressed and could not band like this). The smoking gun was the 8
localEnvProbe POOL slots: parked at (0,0,0) range 2, they baked ONCE at GGTerrain init —
BEFORE any level's sun/sky exist — capturing a dark twilight cube with a moon-like blob
on +Y. In the editor with no probe markers the tracking system's main branch never runs,
so that alien bake persists FOREVER (verified: byte-identical after the full editor
refresh path). When a user adds/edits a probe marker, a pool slot is assigned and there
is a 1-2 frame window (re-track → re-capture) where reflections sample the OLD cube =
Lee's flash: dark navy sphere with light circles.

FIX (game-only, 2.73): in the `bUpdateProbes` consumer (GGTerrain_EnvProbeWork), every
pool slot now gets SetDirty at level load / sky change, and unassigned slots are parked
HIGH in the sky (terrain height + 20000) first — so the stored bake is always clean
CURRENT sky, which is also the least-wrong content for the inherent 1-2 frame flash. The
fade-out park position (-999999)³ got the same treatment (it re-bakes at the park spot; a
void-black cube was the flash content for reused slots). VERIFIED by re-dump: pool probes
now sit at (0, 28215, 0) with a current-daylight bake, twilight cube gone.

THE SAND VERDICT (DX11 read-only forensics): DX11 was NOT "correct" — it carried TWO
deliberate GG-local energy cuts, both with upstream code commented out in place:
(a) `envColor *= 0.5*metalness + 0.5` at the end of EnvironmentReflection_Global/_Local —
sand (metalness 0) got exactly HALF the env reflection; (b) crushed grazing fresnel
`f90 = max(1-roughness^x, f0.r)` replacing upstream's ~1.0 — no wet grazing sheen on rough
surfaces. DX12 restored stock upstream shading (EnvBRDFApprox, f90=1, no damping) and also
uses f0 0.005 vs DX11's 0.02. Worked numbers for beach-class sand (mat2, authored roughness
~0.16 — the content literally says "wet sand"; mat4 is 0.69, mat8 0.98): DX12 env term is
~1.6-1.7× DX11 at typical view angles. The vertical band Lee circled = the sun-halo feature
of the global probe capture, smeared vertically by a glossy flat surface — legitimate
physics made prominent by the restored energy. Levers, in order of fidelity: (1) content —
raise mat2-class sand roughness toward mat4; (2) `SET_ENVPROBE_BRIGHTNESS 0.5` = exactly
DX11's dielectric damping on terrain/sand (knob already shipped as engine 1.55; harness
command added this session); (3) exact DX11 emulation would need the f90 crush too and
dims ALL rough materials. Lee's call — no look change shipped.

OPTIONS PRESENTED, NOT SHIPPED: move the global capture point from the map corner to
centre-high (symmetric sky+distant-terrain dome — closer to what "base env map" should
mean, but a global look change); probe format BC6H → RGBA16F to kill the ring banding
(~+1 MB VRAM per probe, removes a BlockCompress step). Engine untouched this session —
2.73 is game-side only (GGTerrain_part0.cpp + AutomationHarness.cpp + WETEST.md).

## §2.74 — terrain roughness dry-look floor EXECUTED (Lee-directed), and a false "pipeline disconnect" caught before it shipped (08-16 afternoon, #155)

LEE'S DECISION on the §2.73 sand verdict: keep DX12's true energy, alter the CONTENT —
"raise the beach sand's roughness toward mat4's value and treat all other terrain
textures to the same fix"; the shiny originals become the future "maximum energy"
custom set.

EXECUTED (tools/terrainroughness.py, committed): audit of all 64 terrain Surface.dds
(32 mats + 32 extras/lowpoly). Rule: any mat with roughness (G) mean below mat4's 175/255
(0.686) gets an ADDITIVE lift to exactly that mean (additive preserves authored variation;
a x4-5 multiplicative scale would clamp-distort). 11 mats lifted: mat2 41→175 (the beach
sand), mat31 33→175, mat22 73→175, mat10 103→175, mat19/mat27 122→175, mat25 146→175,
mat20 155→175, mat13/mat17 156→175, mat1 172→175. All verified post-encode: G mean on
target ±3, AO drift ≤0.1, DXT1 + 12 mips preserved. 53 already at/above the floor
untouched (all lowpoly Surface maps are flat 255). Originals mirrored no-clobber to
D:\max\mipbackup\terraintextures_buildarea — THAT is the maximum-energy set.

THE VERIFICATION SAGA — a §22.7-class correction, caught in-session: three successive
whole-frame A/Bs on Island Showdown read as noise (lifted-vs-original, then
chrome-mat2-vs-original at grazing cameras, then chrome-vs-lifted on a PAINT_TESTed mat2
patch), and I wrongly concluded "the DX12 terrain pipeline does not consume Surface.dds
roughness". Instrumenting the actual data killed that claim: DUMP_TERRAINSURF (new
harness command) dumped the live SVT surface atlas — bake output initially read as the
constants-fallback signature, but a marker-instrumented CS (temporary engine-shader diag,
fully reverted, engine repo clean) proved 100% of texels take the SAMPLED path on the
correct 2048^2 textures. The G≈250 sea is CORRECT CONTENT: Island Showdown's visible
beaches are mat8/mat18-class sand (authored roughness ~250 — already dry), not mat2; a
chrome DIELECTRIC patch is invisible top-down (F≈f0≈0.5% at normal incidence); and both
"grazing" cameras provably faced away from the painted patch. The conclusive test —
camera INSIDE a painted mat2 patch, 4 yaws, chrome vs lifted — shows 3.4-5.7% of pixels
changed (noise band all day: 0.1-0.6%): chrome sand carries a visible sky-sheen wash,
lifted sand reads warm and matte. PIPELINE HEALTHY, LEVER EFFECTIVE.

WHAT THIS MEANS FOR LEE'S BEACH: his level's sand (classic mat2-class) now renders
dry at DX12's full energy. Levels using mat8/mat18-class sand never looked wet in the
first place. Revert/max-energy path: copy D:\max\mipbackup\terraintextures_buildarea\*
back over Files\terraintextures\* (or ship it as the custom set).

Lessons pinned in WETEST.md (DUMP_TERRAINSURF row): whole-frame diffs need a target-
covering frame; dielectric gloss A/Bs need grazing angles; SET_CAMERA yaw must be proven
to face the target; and Island Showdown is the WRONG level to eyeball sand-roughness
changes on.

## §2.74b — Lee's re-test: the "circles" are the %probe MARKER BALL's rendering, the cube data is proven clean (08-16 evening, #155)

Lee re-tested after 2.73/2.74 and still saw "circled images in the cube map" when
clicking-and-holding his env probe. Full forensics chain, all on live dumps:

1. DUMP_ENVPROBE cubes reprojected to LAT-LONG PANORAMAS — global AND local probes are
   SEAMLESS across every face boundary (sky gradient, horizon and terrain continuous).
   Capture geometry (per-face FOV/matrices), GGX filter and BC6H store are all healthy.
   The 2.73 re-bake is live: pool content is current sky.
2. The engine debug-sphere shader (cubeMapPS.hlsl mirror-ball math) simulated OFFLINE on
   the dumped data — clean continuous mirror ball, no portholes.
3. The engine's own debug env-probe sphere rendered IN-GAME (new harness command
   SET_DEBUGPROBES + gg_debugprobes_force override in lighting_loop, because the editor
   clears the flag every frame) — clean mirror ball, matches the simulation.
4. Box-projected (parallax-corrected) sampling simulated for a ball AT the probe centre —
   also smooth; parallax alone cannot make portholes from clean data.

IDENTIFICATION: the ball Lee inspects is NOT the engine's data visualizer — it is the
`%probe` MARKER ENTITY (probe.dbo at scale 50, probe.png circuit skin, note the selection
outline hugging the sphere in his screenshot; status bar "Object: %probe (dynamic)"). On
DX12 that legacy ball is shaded by the full PBR path — a glossy sphere parked at the exact
centre of its own probe volume, reflecting its own box-projected capture through a legacy
cube-patch sphere mesh — and the combination reads as "a circle image per cube face".
DX11 never showed this because its marker ball never went through modern PBR shading.
Everything that MATTERS samples the clean data through lightingHF (sand, water, objects).

STATUS: data exonerated with instruments; the remaining item is a LOOK/UX decision for
Lee — (a) make the %probe marker ball matte so it stops posing as a data viewer, and/or
(b) enlarge the engine's true debug mirror-ball on probe pick as the accurate DX11-style
preview. No look change shipped without his call.

## §2.75 — Lee-directed: matte %probe marker ball + LARGE accurate preview sphere (08-16 evening, #155/#156)

LEE'S DIRECTION on §2.74b: "Yes do both, matte ball plus larger accurate preview sphere."

SHIPPED (engine delta 2.75 + game 2.75):
1. ENGINE `wi::renderer::SetDebugEnvProbeSphereScale(float)` (`gg_debugprobe_sphere_scale`,
   default 1.0 = stock): the debug env-probe mirror spheres draw with a game-set scale.
   WICKED_ENGINE_CHANGES.md row added (re-apply on upstream pull).
2. GAME matte pass: `WickedCall_MakeObjectEnvMatte(iObj)` (wickedcalls_part3) walks the
   marker object's frames -> mesh subsets -> materials and forces roughness 1 / metal 0 /
   reflectance 0 (idempotent, SetDirty). Called from lighting_loop's probe-list rebuild
   (g_bLightProbeScaleChanged walk) so level load AND newly placed probes are covered.
3. GAME preview sizing: on probe-marker pick, `WickedCall_GetObjectWorldRadius(iObj)`
   (live wicked AABB radius over the marker's frames) x 1.15 -> SetDebugEnvProbeSphereScale
   (fallback 40 if AABB unavailable, clamp 400) then SetToDrawDebugEnvProbes(true).
   Harness: SET_DEBUGPROBES now takes optional scale.

VERIFIED LIVE (Island Showdown): forced scale-60 preview sphere renders as a big CLEAN
mirror ball — continuous sky + island reflection, zero portholes (ball/ball_far.png).
⚠ Camera INSIDE the sphere radius sees nothing (backface cull) — cost one confused shot.
The matte pass could not be photographed tonight: NO reachable level contains a probe
marker (scanned every mapbank fpm map.ent — only Lee's spotshadowtest.fpm has one, and
loose mapbank fpms have no harness loader; TESTPRO1's project203.dat is a custom "Stor"
container, not a zip; a synthetic My Games/projectbank project does NOT register — the
hub lists projects from Files\projectbank\<name>\project203.dat only). The matte code is
15 lines on the proven PICK_AT frame-walk pattern and no-ops without markers (TESTPRO1 +
Island Showdown load clean). Lee verifies with one glance at his spotshadowtest probe.

BONUS re-verify: TESTPRO1 census shows all 8 pool probes parked at (0, 28215, 0) — the
2.73 high-park confirmed on a second level.

## §2.75a — evening FPS scare BISECTED TO AMBIENT: 2.73-2.75 code AND 2.74 content both exonerated

Post-2.75 spot-check read Island Showdown 65.6 (morning 0816 sweep: 77.1) and Switch
Escape 134-137 (morning: 162.6) — repeatable across launches, so not single-run noise.
Full bisect, all fresh-launch probe_one at the start camera:
- HEAD code (2.75) + lifted content:      Island 65.6-65.8, Switch 134.4-137.6
- 2.72 code (checkout c0ec519b) + lifted: Island 65.4,      Switch 128.9-129.8
- 2.72 code + ORIGINAL textures:          Island 65.5
Identical within noise across all three => the delta vs the morning is MACHINE/AMBIENT
(evening state after 25+ MAX launch cycles today), not a regression. Aztec Teaser agreed
with morning (−3%) throughout. VERDICT: ship state clean; the morning 0816 table remains
the baseline of record; cross-time-of-day FPS comparisons join the known-drift class
(±8 band documented, tonight demonstrated ~15% on two demos — SAME-SESSION A/Bs only).
Final state restored: main f3242c8b built, engine 5ee09abc, 11 mats re-lifted (mat2 G
mean 175.1 verified), mirror originals intact.

## §2.75b — Lee-requested full hub sweep on 2.75: CLEAN (0816b)

19/19 loaded, POLYS bit-identical on all 19, 4 GB gate holds (worst Aztec Game Kit
3838 MB — down another 14 from the morning), VRAM within ±17 MB per demo. FPS hub-wide
+4.6% editor / +4.1% in-game vs the morning 0816 table. One band-edge flag (Aztec Game
Kit editor −14%, in-game −5%, VRAM/POLYS identical — the morning table carried the same
single-cell class). Switch Escape editor measured 163.3 in this run vs 134-137 in the
mid-evening probes — §2.75a's ambient-drift verdict confirmed in the same night: the
machine recovered. Full table: DEMO_FPS_SWEEP.md 0816b section; raw
tools/sweep_0816b_2.75.txt. The day's entire arc (2.73/2.74/2.74b/2.75) is sweep-clean.

## §2.75c — ★★ THE REAL ROOT CAUSE OF THE CIRCLES: the marker ball was captured INTO its own probe (08-17 evening, #155 round 3)

Lee's re-test on 2.75 broke my §2.74b story open: his OLD probe's cube — viewed through
the new ACCURATE debug sphere — still showed the portholes (so they are REAL DATA at that
probe), his NEW probe on the same level captured clean, and the matte marker ball read as
solid black (probe.png is near-black in linear; reflections were its only bright term).

DIFFERENTIAL that named it: SET_PROBE_TEST (new harness command driving the real
GGTerrain_AddEnvProbeList path without a marker) captured CLEAN on Island Showdown at
every suspect parameter — under/at/above terrain height, rotation yaw 47, sizes 100,
range 383. The ONLY difference between those clean synthetic probes and Lee's marker
probes: THE MARKER BALL ITSELF. The pool probe captures from the marker's centre — i.e.
from INSIDE the double-sided (FPE cullmode=1), circular-featured probe.dbo ball — so the
cube map records the ball's own openings as portholes onto the world with its interior
as the pale wash. DX11 excluded the marker from probe rendering via probe userdata (the
`probe->userdata = 255/0` fossils commented "DX12 - userdata removed" in
GGTerrain_part0.cpp are the amputated mechanism). Old-vs-new probe asymmetry: a NEW
marker's first capture fires before its ball object is renderable (clean), and Lee's
"click-hold makes the new probe's preview nearly black" = the RE-capture with the ball
present (mostly interior).

FIX (2.75b, game-only): WickedCall_MakeObjectEnvMatte now sets
ObjectComponent::SetNotVisibleInReflections(true) on every frame-object of the marker —
wiRenderer's RefreshEnvProbes culling honours that flag natively (wiRenderer.cpp:10781),
restoring the DX11 exclusion. The 2.75 material-matte treatment is REVERTED: with
self-capture gone, the ball's natural glossy look reflects a CLEAN capture again (fixes
"the probe sphere is now black"). Pick still shows the enlarged accurate debug sphere.

Lee's one-glance re-test: reload the level, look at the ball (glossy again, sane
reflections), click-hold old AND new probes (big preview = clean sky/beach, no circles,
no near-black).

## §2.75d — the click-hold portholes: the DRAG GHOST was the second self-capture path (08-17 evening, #155 round 4)

Lee's 2.75b re-test: unpicked ball GLOSSY with a CLEAN reflection (so the stored cube is
clean — the placed-marker exclusion works), but click-and-hold STILL showed the circles,
plus the preview sphere changed size mid-hold. The differential (settled=clean vs
hold=portholed) named the second path: click-hold picks the entity up into the editor's
DRAG-CURSOR GHOST (t.gridentityobj) — a SEPARATE object from the placed marker. Drag
edits re-capture the probe every frame, and those re-captures photographed the GHOST
ball from inside; the placed ball's NotVisibleInReflections flag never covered it.

FIX (2.75c, game-only, G-Lighting.cpp): (1) the gridentity block now excludes the ghost
object from reflections every frame it exists (any dragged entity — a pickup ghost
should never bake into env captures); (2) the preview-sphere radius takes
max(element.obj, gridentityobj) world AABB radius — during pickup the element's own
object collapses, which was tripping the 40-unit fallback and making the sphere jump in
size mid-hold.

Lee's re-test: click-hold + DRAG the probe — the big preview should stay a clean
sky/beach mirror through the whole drag, stable size, no circles.

## §2.75e — ★ STATUS: THE CLICK-HOLD CIRCLES SURVIVE ALL THREE FIXES — OPEN, PARKED FOR A FRESH ATTACK (08-17 evening, Lee's verdict)

Lee confirms the corrupt circle images STILL show on click-hold after 2.75c. Honest
ledger of the three failed attempts and what each PROVED:

1. 2.75 matte (f3242c8b): treated the marker ball's shading — WRONG THEORY (the circles
   are in displayed cube data, proven by the accurate debug sphere showing them).
   Reverted in 2.75b. Side-finding: ball went solid black (probe.png is near-black in
   linear; reflections were its only bright term).
2. 2.75b placed-marker NotVisibleInReflections (8556ec33): PARTIAL truth — the SETTLED
   capture became clean (Lee's unpicked glossy ball reflects a correct sky/island), but
   click-hold circles persisted. So marker self-capture WAS one real path, not the whole
   story.
3. 2.75c drag-ghost NotVisibleInReflections + stable preview radius (b0ca1e7b): circles
   STILL present on click-hold per Lee. Ghost-exclusion theory insufficient (or the flag
   does not stick — the ghost object may be RECREATED after lighting_loop each frame,
   leaving an unflagged window every re-capture; unverified).

ESTABLISHED FACTS (instrument-backed, do not re-litigate):
- Settled/stored cubes are CLEAN: panorama reprojection seamless, offline mirror-ball
  sim clean, engine debug sphere clean on Island Showdown, Lee's own unpicked ball
  reflection clean after 2.75b.
- Synthetic probes (SET_PROBE_TEST, no marker ball) capture clean at EVERY suspect
  parameter: height under/at/above terrain, yaw 47, sizes 100, range 383.
- The circles appear reproducibly ONLY during click-hold of a probe marker, on Lee's
  machine, shown by the ACCURATE debug sphere => they exist in whatever cube is
  displayed at hold time.

THE MISSING DATUM (next attack should START here): DUMP_ENVPROBE **while Lee holds the
probe** — the harness watcher runs in his live session, so with his MAX open on the
level, drop DUMP_ENVPROBE into auto_command.txt DURING the hold and decode the exact
displayed cube. That splits capture-corruption vs display-path in one shot. Candidate
suspects for the hold window: ghost recreation racing the once-per-frame flag set
(instrument: log flag state at capture time engine-side); hold-time re-capture including
editor-only passes; the displayed sphere belonging to a DIFFERENT slot than assumed
(dump ALL slots during hold and match).

NEXT PER LEE: park this; tackle the ENV PROBE SIZE CHANGE bug first — Lee has built a
NEW TEST LEVEL for it.

## §2.76 — ★ FIXED: picking a probe marker resized EVERY probe ball in the level (08-17 evening, #158)

LEE'S REPORT (his TESTPRO2 / spotshadowtest level, two %probe markers on barrels):
"When I click and release the left mouse button on the left env probe marker, BOTH
spheres increase in size, that is a bug. They should stay the same size!!"

MY OWN 2.75 CODE, TWO SEPARATE DEFECTS, ONE SYMPTOM:

1. **Size** — `WickedCall_GetObjectWorldRadius` returned `AABB::getRadius()`, which is the
   half-DIAGONAL of the box (sqrt(3) x the half-extent for a ball's tight box), and
   G-Lighting scaled it a FURTHER 1.15. 1.732 x 1.15 = 1.99, so the preview sphere drew
   at ~2x the marker ball's visible radius. Lee's screenshots measure the jump at ~2.05x —
   the arithmetic and the pixels agree.
   ★ RULE: "make X the size of Y" wants the HALF-EXTENT. A bounding-sphere radius is
   sqrt(3) too big for anything box-shaped and sqrt(2) too big in 2D. Both are "radius".
2. **Every probe** — the engine's `debugEnvProbes` block loops `scene.probes` and draws a
   mirror sphere for EVERY probe. At the stock unit scale those were 1-unit specks in an
   inch-scale world, invisible, so nobody ever noticed; the moment 2.75 sized them to the
   marker ball, picking one probe swelled every probe in the level.
   ★ RULE: before scaling up something the engine draws for ALL of a class, check the
   loop — a harmless stock behaviour at scale 1 is a bug at scale 60.

THE FIX (engine delta 2.76 + game 2.76):
- game `WickedCall_GetObjectWorldExtent` (renamed from ...WorldRadius) = max half-extent
  of the largest frame AABB; preview scale = extent x 1.02 (just enough to cover the mesh
  instead of z-fighting it). The 2.75 "fallback 40 units" magic number is GONE — an
  unavailable AABB now reuses the last good extent (a fallback that differs from the real
  size IS the size-pop bug in another costume).
- engine `SetDebugEnvProbeFocus(x,y,z,radius)`: exactly one sphere draws, the probe
  NEAREST the focus point and only within radius; radius <= 0 keeps the stock all-probes
  behaviour for the SET_DEBUGPROBES harness override. Game focuses on the picked marker's
  x/y/z, which is exactly where its probe sits (`GGTerrain_AddEnvProbeList` copies the
  position into `probe->position`, no offset).
- ⚠ NEAREST-WINS, not a radius test: round 1 used `radius = max(extent*3, 100)` and BOTH
  balls still previewed, because Lee's two markers sit **63 units apart** — closer to each
  other than any sane tolerance. Measured, not guessed: the first build's screenshots
  showed the bug half-fixed (right size, still both).

VERIFICATION (harness, Lee's own level, elements 1192/1195 = the two %probe markers):
`SELECT_ENTITY` reproduces the settled pick exactly (it sets `t.widget.pickedEntityIndex`,
which is what lighting_loop reads). Per-ball changed-pixel measurement vs the unselected
baseline, same session, ambient control included:
| case | left ball | right ball |
|------|-----------|------------|
| pick left (1192)  | 10806 px changed, silhouette 172x118 | **0 px** (bit-identical) [+61 px = the cyan proxy-box line] |
| pick right (1195) | **0 px** (bit-identical) | 12429 px changed |
| deselect x2 (control) | 0 px | 0 px |
The changed silhouette equals the marker ball's own dome (172 px wide, still occluded by
the barrel rim exactly as before) — the preview is a drop-in for the ball, not a balloon.
Untouched probes are now provably untouched.

⚠ Note for the still-open circles hunt (#157): the click-HOLD path is unchanged in spirit —
during a drag the size still comes from max(element, ghost) extent, and the focus follows
the element, so once a marker is dragged >100 units from its probe the preview hides until
release re-places the probe. That is deliberate (the probe has not moved yet), but it does
mean the hold-time preview can now vanish rather than lag — worth knowing when reading
Lee's next hold-time report.

## §2.77 — ★★ THE CIRCLES, ROOT-CAUSED AT LAST: a %probe marker is TWO objects and only one was excluded (08-17 late, #157 round 5)

LEE'S LEAD (the one that cracked it): "this could be rendered if a sphere was being rendered
inside a cube, and the sphere intersects the cube so that only the part of the sphere outside
of the cube would be rendered." That reading is geometrically exact for an env capture: the
six cube-face frustums all clip at the SAME near distance, so the volume hidden around a probe
IS a cube — and anything close enough to straddle it survives only where it pokes out.

★ INSTRUMENT FIRST (the thing four failed rounds lacked): `SET_PROBECAPTURETRACE <0|1> [r]`
(engine 2.77) appends one block per env-probe CAPTURE to Files/probecapture.txt — the near/far
the capture used and every object within r with its exact keep/skip reason. First run on Lee's
spotshadowtest answered two questions at once:
- znear=3.0 / zfar=500000 (inherited from the MAIN camera, `vis.camera->zNearP`). The cube-clip
  Lee described is REAL but only 3 units across — not the carve he was seeing.
- **The marker balls were coming back `norefl=0 RENDERED`** — inside their own probe's capture,
  which 2.75b was supposed to have stopped.

WHY 2.75b MISSED IT (`DUMP_OBJENT` / `DUMP_ENTOBJ`, added to settle it):
```
obj=71184 frames=1 | f0 'sphere' ent=21781 norefl=1   <- element 1192's obj -> excluded
obj=71185 frames=1 | f0 'root'   ent=21784 norefl=0   <- RENDERED into the capture
obj=71186 frames=1 | f0 'root'   ent=21786 norefl=0   <- RENDERED into the capture
obj=71187 frames=1 | f0 'sphere' ent=21789 norefl=1   <- element 1195's obj -> excluded
```
**A %probe marker owns TWO GG objects.** The element table points at the inner ball (DBO frame
'sphere', r=21.7); a SECOND object per marker (frame 'root', r=28.6) sits 19.9 units away with
the probe INSIDE its bounds, and nothing ever excluded it. Every capture photographed that
shell's interior. 2.75b was correct and landed correctly — on half the marker.

THE FIX (2.77, game-side): `WickedCall_ExcludeObjectsEnclosingPoint(x,y,z,maxRadius)` called
from the probe-list rebuild per marker. Exclusion is by GEOMETRY, not object number, because
the object number was never the invariant: an object that ENCLOSES a probe's origin cannot be
meaningfully captured by that probe — you are inside it, and it paints the cube with its own
interior. maxRadius = marker extent x3 (~65 units) keeps this to widget/marker-scale props; a
room or building that legitimately encloses an interior probe is far larger and stays in.

VERIFIED (same level, same camera, same session type):
- instrument: all four marker objects now `norefl=1`, and **zero** 'root'/'sphere' objects are
  RENDERED into any capture (was 2 per capture).
- visual: the picked probe's preview ball changed by **4420 of 37950 pixels (11.6%)** — the
  dark band the shell's interior was eating is gone; beach, water and sky are now complete.

⚠ HONEST SCOPE: this is proven for the SETTLED capture. Lee's report is about the CLICK-HOLD
window; the settled cube was visibly polluted by the same shell, so this is very likely the
same defect, but only his hold repro can close it. If any corruption survives: arm
`SET_PROBECAPTURETRACE 1 120` in his live session, hold the X-axis widget, and read
probecapture.txt — every capture during the hold is now itemised by name and skip reason.
⚠ The exclusion is sticky for the session (a prop dragged through by a probe stays out of
reflections until level reload). Bounded by the radius cap; revisit if it ever bites.
★★ RULE EARNED: "the element's object" is NOT "the entity's geometry". Before trusting any
per-object treatment on a marker, DUMP the object range around it — GG allocates more than one
object per marker and the element table names only one of them.

## §2.78-2.80 — the env-map DEBUG RIGS and what they measured (08-17 night → 08-18, #157, Lee-driven)

Lee ran this session; the rigs below exist to serve HIS step-by-step method (known → unknown,
one contributor at a time). Read §2.80c FIRST if you are about to edit a shader.

### The rigs (all default OFF; setup.ini keys persist across relaunch)
| rig | live command | setup.ini | what it does |
|-----|--------------|-----------|--------------|
| park local probes | `SET_LOCALPROBES 0` | `globalprobeonly=1` | whole level reflects ONLY the global/base cube — the permanent form of dragging Probe Range to glimpse it |
| move the base capture | `SET_GLOBALPROBE <x> <y> <z>\|rebake\|off` | — | stock captures at the MAP CORNER (0, 8215, 0) on spotshadowtest = 208 m above the corner |
| env-only object output | `SET_ENVONLY <mode> [mip]` | `envonly=1`, `envonlymip=` | objects output ONLY the cube; modes 1-10 (see WETEST) |
| solid-colour cube | `SET_ENVSOLID <0\|1\|2> [r g b]` | `envsolid=1` | 1 = every cube read returns a flat colour; 2 = SPLIT, one colour per read site |
| capture contents | `SET_PROBECAPTURETRACE 1 [r]` | — | names every object entering a probe capture (this is what cracked #157's root cause) |

### What the ladder MEASURED (all same-state A/Bs on Lee's spotshadowtest)
- **normals are innocent**: cube via shading normal vs FACE normal differs 5% (mean 4.3); the two
  normals drawn as colour differ 0.13% — the normal map does nothing on this ball, and the field
  is smooth with no lobes.
- **fresnel is the big suppressor**: cube x surface.F leaves **10%** overall — 2.7% at the centre,
  5.0% at the rim. Near-uniform, so it cannot carve discrete shapes.
- **occlusion is a no-op here**: 98.7% survives; the factor is 1.00 across almost the whole ball.
- **ambient is a flat wash**: alone it reads 201/255 with std 13 across the entire sphere. It is
  the SAME cube sampled at `mipcount` (clamps to the last mip = one average colour) plus the flat
  weather ambient — not a separate texture.
- **the sky term is flat too**: 220/255, std 7, and 99.7% of ball pixels differ from the cube.
- **mips behave normally**: detail collapses 11x from mip 0 to mip 1, then softens. ⚠ mip 3 came
  back pixel-identical to mip 2.
- ★★ **SPLIT MAP — ownership settled**: on the ball's disc, the global SPECULAR read
  (`EnvironmentReflection_Global`) owns **100.0%**; ambient **0.0%**; local box-projected 0.1%
  (the highlight dot). Ambient paints nothing here because it feeds diffuse and the marker ball's
  albedo is near-black.
- ★★ **Lee's question answered**: BOTH pixel populations — the whitish gaps AND the detailed
  circles — come from that one texture read. Split map proves ownership (nothing else paints the
  ball); the solid-colour test proves the value (both regions changed to the flat colour together;
  if the gaps came from the sky/ambient/clear colour they would have stayed pale).

### ⚠ A near-miss worth keeping: the THIRD cube read
An exhaustive census (not a grep) found `EnvironmentReflection_Local` (lightingHF.hlsli:773) also
serves the GLOBAL cube: probes[0]'s descriptor is written into the probe ENTITY array
(wiRenderer.cpp:5688) and GGTerrain gives it range 50000, so its OBB covers the level and the
box-projected path wins for most pixels. It never mentions `GetScene().globalprobe`, so grepping
that name misses it. Covering only the two obvious reads would have left the cube's content on
screen and made the whole solid-colour test meaningless.

## §2.80c — ★★ FAILURE TO RECORD: the mip rungs NEVER REACHED THE GPU (2 failed attempts)
`SET_ENVSOLID 3` (paint the chosen mip index) and `4 <mip>` (force the mip) were written, compiled
and shipped — and do NOTHING. The ball renders 192/0/155, pixel-identical to the PREVIOUS shader
version's split magenta, for both modes.
What was tried and did NOT fix it: (1) engine rebuild + refresh_shaders; (2) a full GAME rebuild
afterwards. Neither changed `objectPS.cso`'s timestamp (frozen at 23:52:36).
What is NOT known: **who actually produces the deploy `.cso`**. It is not either build (both left
the timestamp untouched) and it cannot be a runtime source recompile — `SHADERSOURCEPATH` defaults
to `../WickedEngine/shaders/` relative to the exe, **that folder does not exist**, and the game
only ever calls `SetShaderPath("shaders/")`, never `SetShaderSourcePath`.
★★ RULE (this is the repo's "two shader paths" warning biting again, harder): **a shader edit is
not live until a DELIBERATE, VISIBLE change proves it.** Add a rung that must obviously alter the
image, confirm it, and only then trust any measurement from that shader. Earlier results in this
session are safe because each one visibly changed on command; anything measured from an unverified
shader edit is worthless.
NEXT (do this before any further shader rung): find the real producer of the deploy `.cso`
(likely an offline shader-compiler step with its own output dir + copy rule) and gate on a
visible-change test.

## §2.81 — ★★ +X FACE WIPE VERIFIED LIVE + the §2.80c mystery SOLVED (08-18, Lee-directed)
Lee's new baseline (spotshadowtest re-saved 08-18 00:1x): stripped-down scene, camera on the ball;
he proved with the Cloud Coverage slider that the "gap" colour tracks CLOUD content in the env map
(coverage 80→171→144 shifts the solid gap tone — so the gap = cloud texels, likely a small/deep-mip
sample). His ask: wipe the cube's +X face to black to PROVE the shader rig touches the exact
texture on the ball.

### The wipe (SET_ENVSOLID mode 5 / setup.ini envsolid=5)
`GGEnvWipeFacePX(dir)` in lightingHF.hlsli: mode ≥5 AND dir.x>0 AND |x| dominant → the sample
returns BLACK; everything else renders normally. Applied at ALL THREE global-cube read sites
(specular EnvironmentReflection_Global, ambient GetAmbient, parallax-local
EnvironmentReflection_Local — the §2.80b census sites). VERIFIED: cold launch, no live command —
setup.ini `envsolid=5` alone — screenshot 00-30-54 shows a clean black quarter on the ball where
the +X face reflects; horizon circles + cloud speckle elsewhere untouched. **We have live shader
access to the cube.**

### ★★ §2.80c RETRACTION — the shader pipeline was NEVER broken
Three facts found this morning overturn the "mip rungs never reached the GPU / .cso producer
unidentified" verdict:
1. **The deploy `objectPS.cso` (23:52:36) is NEWER than the last source edit (lightingHF
   23:49:47).** The runtime recompile DID run and DID pick up the rungs; the "frozen timestamp"
   after the two rebuilds was correct behaviour (nothing was stale). The producer was never a
   mystery: wi::shadercompiler runtime recompile, backstopped by refresh_shaders.ps1 — the
   documented pipeline (memory: project-shader-build-pipeline) working as designed. Proof today:
   refresh_shaders flagged 152 cso after the 2.81 edit and the 00:28:29 launch recompiled them.
2. **The real bug was game-side: `GGSetEnvSolidIni` BOOLEAN-IZED the mode** —
   `(iOn != 0) ? 1 : 0` — so any MAX restart with `envsolid=<mode>` in setup.ini collapsed 2/3/4
   to mode 1 (flat magenta). Lee restarted MAX mid-test ("I restarted MAX", "That does not look
   like mode 1") — every observation after that restart was mode 1 wearing mode 3/4's label.
   On the mirror-ball F≈1, mode-1 magenta ≈ split-mode magenta (192/0/155) — pixel-identical,
   which is exactly what was measured. Fixed 2.81: ini passes the int through.
3. The SHADERSOURCEPATH-points-nowhere claim was a red herring — the engine bakes the source
   path from __FILE__ (SHADER_INTEROP_PATH), it does not need the exe-relative folder.
The DURABLE RULE from §2.80c stands unchanged — a shader edit is not live until a deliberate
visible change proves it — 2.81 is that rule executed properly (the wipe IS the visible gate).
The mip rungs (modes 3/4) are hereby UNBLOCKED: same-session `SET_ENVSOLID 3` / `4 <mip>` after
this build will run the real code. Engine e179b981's "DO NOT TRUST" body is superseded by this
section.

### State for Lee's next step
setup.ini: `globalprobeonly=1`, `envsolid=5` (wipe ON — set 0 to return to the plain baseline),
`envonly=0`. MAX running, TESTPRO2/spotshadowtest loaded via harness. Revert lever: SET_ENVSOLID 0
live, or envsolid=0 + relaunch.

## §2.82 — DIRECTION-PEEL rungs (Lee-directed): every bender of the env-read direction, isolated
Lee's method continues: having proven (§2.81 + the live A/B/C diff) that the cube WRITES are clean
and the black wipe is read-side, he asked for the direction chain itself — every contributor that
bends the sample direction, each isolatable, peeled one at a time down to NO direction (a fixed
vector for all reads → the whole env term becomes one texel).

### The verified chain (what bends the direction, in order)
1. MESH VERTEX NORMALS — authored smoothing, interpolated (objectHF:567).
2. BACKFACE FLIP — `if (!is_frontface) nor = -nor` (objectHF:563); the %probe ball is
   double-sided, so openings show backfaces with mirrored normals.
3. `facenormal` SNAPSHOT taken here (objectHF:568) — AFTER the flip, BEFORE the normal map:
   this is the peel point rungs 2/3 use.
4. NORMAL MAP — `N = lerp(N, mul(bump,TBN), strength)` (objectHF:843, the 1.63 parity site).
5. CAMERA — `R = -reflect(V, N)` (surfaceHF:280).
6. BOX PROJECTION — local path only: R rewritten by pixel-position-vs-probe-OBB (lightingHF).
(Clearcoat/aniso variants alter direction too but the ball's material has neither.)
MIP is selection not direction (roughness × mipcount) — freeze it with SET_ENVSOLID 4 <mip>.

### The rungs — SET_ENVDIR <0-4> [x y z] (engine 2.82: gg_envdir CB row + GGEnvPeelDirSpec)
0 stock · 1 box projection OFF (local path samples raw R) · 2 + normal map OFF (reflect off
facenormal) · 3 + camera OFF (sample facenormal, no reflect) · 4 FIXED xyz (default +Z) at ALL
sites incl. ambient — no direction left; expected image = flat single-colour env term everywhere.
Composability: SET_ENVSOLID's wipe/mip/split all operate on the PEELED direction (ggDir), so
e.g. wipe+rung2 shows "which pixels' GEOMETRIC reflection hits +X". Ini: envdir=<mode> (int
passthrough per the 2.81 rule; ini fixes dir at +Z, custom xyz is harness-only).
⚠ What CANNOT be peeled individually at read time: the mesh vertex normals and the backface
flip — they ARE facenormal. Rung 3→4 removes them together. If a flip-only rung is ever needed
it requires plumbing is_frontface into Surface (engine change, not taken).

## §2.83/§2.84 — ★★★ THE CIRCLES ROOT-CAUSED AND FIXED: FilterEnvMap starvation + HDR flood
The night's peeling converged: normals smooth (facenormal-as-colour view), direction field
smooth (R-as-colour view), mip0 faces clean (dump), cube writes clean — then Lee's CARDINAL
LOCK test (2.83, SET_ENVDIR 5: snap direction to dominant axis = face-centre reads only)
KILLED the discs, and the per-tile contrast-stretched blowup of mips 1-3 showed the smoking
gun: A BULLSEYE INSCRIBED IN EVERY FACE of the BRDF-filtered chain. The circles were in the
cube after all — one mip below the level I had checked.

### Why the ball painted them (the full mechanism)
The ball's material is rough (2.75 matte) → MIP = roughness×4 lands on the FILTERED levels.
Face-centre cones return real content (the "circles with detail"); face-edge/wide cones
returned a heavily-averaged near-single value (the "gaps") whose colour tracked Cloud
Coverage because it IS a sky average — Lee's "single rogue pixel" deduction, exactly.

### The three 2.84 fixes (engine b44c3f0e)
1. FULL mip chain on the filter SOURCE buffers (were probe's 4 = GetMipCount(...,16)):
   computeLod requests source lods ~4-8 for rough levels; with 4 mips EVERY wide ray clamped
   to the 16px level → filtered mips collapsed toward one value per face.
2. Roughness ladder aligned to the read: filterRoughness = i/mipcount matching lightingHF's
   MIP = roughness×mipcount (was i/(mips-1) → 0.33/0.67/1.0 = ~33% over-blur per level).
   Only the shipped probe mips are filtered (source/filtered buffers carry the full chain so
   the whole-resource CopyResource stays legal; BlockCompress loops the DST's mip count).
3. filterHDRClamp (push-constant pad slot reused; default 2.0; SetEnvProbeFilterHDRClamp;
   harness SET_PROBEFILTER <v>, 0 = stock): the capture carries a 10-20x blown horizon band
   (DX11 captured LDR — its band clamped at 1.0 by construction); the GGX prefilter spread
   that energy across every wide cone, flooding filtered mips flat.

### Verified (all same-session, spotshadowtest)
- Filtered mips keep real structure: side-face CV 0.40→0.65 (mip1), 0.25→0.37 (mip3);
  -Y foliage crisp at mip1. (+Y retains a ~4% radial ring — likely legit red Horizon/Fog
  colour bleeding into edge cones; watch, don't chase.)
- SET_ENVONLY 1 0: ball = FLAWLESS mirror panorama (mip0). SET_ENVONLY 1 1: clean gentle
  blur, no disc edges. Lee's prediction ("crisp reads + blur-style ring") realized.
- Normal-shaded ball: much improved (no white flood; gaps now real dark horizon content)
  but soft discs REMAIN because the 2.75 matte pushes its reads to mips ~3 (16px + BC6H).
### REMAINING (Lee's call): un-matte the %probe preview ball (revert/flag 2.75's
WickedCall_MakeObjectEnvMatte in the lighting_loop rebuild) so the preview reads mip0 =
the flawless mirror. The matte was added when the ball itself was the suspect; that theory
died tonight. ⚠ ALSO unexplained (parked): live SET_ENVSOLID modes ≥3 rendered as the
mode-2 split during the 02:39 test (mode 4 forced-mip printed OK but painted split
magenta 191/0/155) — the ini path works; the live-path w value needs one clean look.

## §2.84b — ⚠ OPEN INSTRUMENT BUG: SET_ENVSOLID modes 3/4 render as mode 2 (pixel-proven)
04:03 forensics, one fresh session (2.84 build, objectPS.cso 03:45 > lightingHF 03:05):
mode 5 → wipe fires (ball black in +X zone) ✓ · mode 2 → split magenta ✓ · mode 1 → flat×F ✓
· **mode 3 → BIT-IDENTICAL to mode 2** (ball 191/0/155 both) ✗; mode 4 did the same at 02:39.
Reproduces on BOTH routes (live harness AND setup.ini — the "ini path is reliable" claim from
the 2.81 post-mortem is hereby RETRACTED) and across two builds. Decisive contradiction: in the
current shader source NO read site emits magenta at w=3 (specular would paint mip colours,
local blue, ambient green) — so the VALUE or the READ of gg_envsolid.w is corrupted between
the C++ globals and the branch test, for the values 3 and 4 specifically while 1/2/5 pass.
NEXT (fresh head): a readback rung that paints gg_envsolid.w itself as a colour ramp — one
look names "value corrupted in transit" vs "branch test misbehaving". Until then modes 3/4
are DEAD; do not use them as instruments.
CONSEQUENCE: Lee's per-pixel roughness question (do the remaining soft discs on the matte
ball come from a varying gloss/surface pattern?) is STILL OPEN — the mip-paint view never
executed. Cheapest reliable probe: a rung painting surface.roughness as greyscale, bypassing
gg_envsolid entirely.
State handed back 04:07: envsolid=0 live+ini, envdir=0, envonly=0, globalprobeonly=1 (Lee's
baseline). The 2.84 filter fixes stand verified regardless (mip0 mirror / mip1 blur / flood dead).

## §2.85 — Lee's verdict on 2.84: NOT SOLD (fish still shows the gap class) → PLAIN MIPS mode
Lee re-tested with a glossy fish (Bass): the large solid-colour outer-circle gaps persist on
real content. His call: the 2.84 "FIXED" claim was premature — what stands is the WHAT (the
prefiltered mip chain). His directive: NEW mode (no reuse of old modes), skip the FilterEnvMap
BRDF prefilter ENTIRELY and ship the good old 2x2 box-reduction chain, irrespective of
roughness — the DX11 approach.
Implementation (engine 2.85): `gg_envprobe_plainmips` DEFAULT 1 gates the FilterEnvMap
dispatch loop; the CopyResource that precedes it already carries GenerateMipChain's LINEAR
box chain into the Filtered buffer, so skipping the dispatches ships exactly the DX11-style
mips (clouds included — the chain is built after the cloud composite). SET_PROBEMIPS <0|1>
(0 = BRDF filter with 2.84 fixes) + REFRESH_ENVPROBE to re-bake. wiRenderer.h
SetEnvProbePlainMips.

## §2.88 — ★★★ FULL REVERT (Lee's call, 05:30 08-18): back to engine 92df06c7 / game a42e6919
Lee's verdict on the 10-hour env-probe hunt: "a bit of a wash" — reset to the 2.76 pair and
take a different direction another day. This section is the complete ledger of what is being
reverted, what was learned, and what deserves RE-APPLICATION on its own merits.

### Reverted deltas (2.77 → 2.87), one line each
| Delta | What it was | Verdict |
|---|---|---|
| 2.77 capture trace + root-shell exclusion | SET_PROBECAPTURETRACE named the marker's 2nd object photographing its interior; geometric exclusion | Real root cause of CAPTURE circles; verified; reverted with the rest |
| 2.78 SET_LOCALPROBES / SET_GLOBALPROBE / SET_DEBUGPROBEGLOBAL / globalprobeonly | park pool probes, move global capture point, preview forced to global cube | Debug rigs; scrapped |
| 2.79 env-only ladder (SET_ENVONLY 1-10) | cube + exactly one contributor at a time | Debug rig; scrapped |
| 2.80/2.80b solid/split/mip modes (SET_ENVSOLID) | flat colour / site-split map / mip rungs | Debug rig; scrapped. Split map measured: ball disc = 100% global specular read |
| 2.81 +X face wipe + ini int passthrough | proved live shader access to the cube; fixed GGSetEnvSolidIni bool-ize | Debug rig; scrapped |
| 2.82 direction-peel rungs (SET_ENVDIR 1-4) | box projection / normal map / camera peeled cumulatively; fixed dir terminal | Debug rig; scrapped |
| 2.83 cardinal lock (SET_ENVDIR 5) | Lee's discriminator — face-centre-only reads | Debug rig; scrapped |
| 2.84 FilterEnvMap 3-part fix | full-chain filter source + read-aligned roughness ladder + HDR clamp | ★ GENUINE ENGINE FIX — see keepers |
| 2.85 plain 2x2 box mips (SET_PROBEMIPS) | skip the BRDF prefilter entirely, DX11-style chain | ★ Valid look lever — see keepers |
| 2.86 forced env mip (SET_ENVMIP) | force one mip at every read, fresh CB row | Debug rig; scrapped |
| 2.87 marker mirror (metal/gloss/stripped maps, name-hunt for the 'root' shell) | made the %probe ball a true mirror | Cosmetic experiment; scrapped; revealed the portholes are AUTHORED |

### ★ KEEPERS — genuine findings INDEPENDENT of the marker-circle question
1. **FilterEnvMap is misconfigured for GG's probes (2.84, engine commit b44c3f0e carries the
   full fix).** Three verified defects affecting EVERY reflective surface in EVERY scene:
   (a) the filter SOURCE buffer had only the probe's 4 mips while computeLod requests source
   lods up to ~8 → wide rays clamp to the 16px level → filtered mips collapse toward one
   value per face; (b) write/read roughness ladder mismatch — filter authored mip i at
   roughness i/(mips-1) while lightingHF reads MIP = roughness × mipcount → every level ~33%
   blurrier than the reader assumes; (c) the HDR capture carries a 10-20x blown horizon band
   (DX11 captured LDR) whose energy floods every wide GGX cone. Evidence preserved: before
   sheets (bullseye per face), after sheets (side-face CV 0.40→0.65 at mip1), the mip0
   flawless-mirror ball and mip1 clean-blur ball screenshots. RE-APPLY when reflections
   quality matters: cherry-pick b44c3f0e (+ d5ce3478 for the plain-mips lever).
2. **Plain 2x2 box mips (2.85, engine d5ce3478)** — a one-gate DX11-parity option: skip the
   BRDF prefilter, ship GenerateMipChain's box chain. Kept structure at every level
   (CV 0.77→0.76 across mips vs the filtered chain's collapse).
3. **The knowledge set** (no code needed):
   - The global probe captures at the MAP CORNER + 208 m up → the cube's content IS "island
     disc + sky wash"; every curved glossy surface faithfully reflects that geography. Any
     future direction must decide the CAPTURE POSITION POLICY (the DX11 intent — ground at
     origin + 30 m — is still commented in GGTerrain_part0).
   - Probe textures ship 4 mips at 128px (GetMipCount min-dimension 16) — remember this when
     reasoning about roughness→mip behaviour.
   - probe.dbo is AUTHORED with portholes: shell object 'root' (offset ~20 units) over inner
     'sphere' — the marker's on-ball circles are the model's own design, seen through PBR
     optics. A clean preview needs a plain sphere mesh or the shell hidden.
   - A dielectric shows ~4% of the env reflection head-on (fresnel) — a "mirror ball" preview
     REQUIRES metalness 1 + roughness 0 + white basecolor (maps stripped), or the env-only
     debug view. The black-void ball was correct physics, not a broken reflection.
   - The capture pipeline layers writers per capture: scene → sky → GG terrain customDraw →
     aerial-perspective CS → volumetric-clouds CS → mips → BRDF filter → BC6H.
   - Cube face selection is fixed-function inside SampleLevel; diagonals are valid; there is
     no NaN path for finite directions.
4. **Bug class lessons re-earned tonight**: ini bool-ize of a mode value silently demotes
   debug modes across restarts (2.81 class — check BOTH value routes of any knob);
   ⚠ UNRESOLVED ANOMALY: values 3/4 sent to gg_envsolid rendered as mode 2 on BOTH routes
   with a fresh cso while 1/2/5 passed — never explained; if similar CB-mode instrumentation
   returns, budget a readback rung (paint the value) FIRST.
5. **2.77's root cause is real even though reverted**: env captures photograph the marker's
   enclosing 'root' shell interior ("circle image on each cube side") — whatever the new
   direction is, placed-probe captures still need SOME exclusion answer.

### Scrapped harness commands (all added after a42e6919, gone with the revert)
SET_PROBECAPTURETRACE · DUMP_OBJENT · DUMP_ENTOBJ · SET_LOCALPROBES · SET_DEBUGPROBEGLOBAL ·
SET_GLOBALPROBE · SET_ENVONLY · SET_ENVSOLID · SET_ENVDIR · SET_PROBEFILTER · SET_PROBEMIPS ·
SET_ENVMIP. (DUMP_ENVPROBE, REFRESH_ENVPROBE, SET_DEBUGPROBES predate the revert point and
SURVIVE.) setup.ini debug keys removed: globalprobeonly / envonly / envonlymip / envsolid /
envdir.

### Revert mechanics
Tree-restore commits (no history rewrite): game `git checkout a42e6919 -- <code files>`
keeping the three living docs (this file, WETEST, WICKED_ENGINE_CHANGES) at tip; engine
`git checkout 92df06c7 -- .`. Both rebuilt and smoke-tested before handover.

---

## §2.89 — ★★★ THE CIRCLES, ROOT-CAUSED: fp16 overflow in the env-probe parallax (2026-08-18, overnight)

**One sentence:** `EnvironmentReflection_Local` computes the parallax ray-exit distance in
`half` (= `min16float` = real fp16 on this hardware, max 65504) in **world units**, so GG's
50,000-unit `globalEnvProbe` box overflows to `+INF` for every reflection direction outside
six ~40° caps around the ±X/±Y/±Z axes — and those six surviving caps ARE the circles.

### What Lee's three screenshots actually showed (the observation that cracked it)
He supplied the decisive A/B without knowing it:
- **Shot 1** (nothing picked): the %probe marker ball reflects its **local** pool probe →
  smooth, clean.
- **Shot 2** (probe picked): the engine's debug inspection sphere → clean panorama. This one
  is a red herring for the bug: `cubeMapPS` samples the cube as a **raw mirror at mip 0**, no
  Fresnel, no roughness, **no parallax**. It literally cannot show this defect.
- **Shot 3** (LMB held, probe update suspended): the same ball, same material, same camera,
  now falling through to the **global** probe → circles.

Same geometry, same shader, same mip regime, only the probe changed. That killed every
geometry/normal-map/capture-content theory from §2.77-§2.88 in one step and pointed at the
one thing that differs between a global and a local probe.

### The one load-bearing difference
Both probes are created identically (`Entity_CreateEnvironmentProbe`, res 128, 4 mips,
realtime=0, same capture code path, same read math). They differ in exactly one property:

| | global (`globalEnvProbe`) | local pool probe |
|---|---|---|
| OBB half-extent (transform scale) | **50,000** | 1 → a few hundred |
| `probe.range` | 100,000 | 2 → a few hundred |
| parallax exit distance | **50,000 … 86,600** | tiny |
| fp16 ceiling 65,504 | **exceeded** | never approached |

`lightingHF.hlsli` (stock):
```hlsl
half3 RayLS = mul((half3x3)probeProjection, surface.R);
half3 FirstPlaneIntersect  = (1 - clipSpacePos) / RayLS;
half3 SecondPlaneIntersect = (-1 - clipSpacePos) / RayLS;
half3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
half  Distance = min(FurthestPlane.x, min(FurthestPlane.y, FurthestPlane.z));   // WORLD units
half3 R_parallaxCorrected = surface.P - probe.position + surface.R * Distance;
```
`Distance` = half-extent / max|R.axis|, so it runs 50,000 (ray along an axis) to 86,600 (ray
into a corner). It stays finite only while `max|R.axis| > 50000/65504 = 0.763`, i.e. inside a
**40.2° cap around each of the six axes**. Everywhere else → `+INF` → the sampled direction is
garbage. Threshold for any probe: **half-extent > 65504/√3 = 37,820 units breaks.**

### Why this explains every symptom the hunt collected
- **Circles**: six finite caps (≈71% of the sphere, overlapping) separated by INF bands around
  the cube edges/corners = discs of correct reflection in a field of rubbish. A numpy mirror-
  ball simulation built only from the shader source reproduces Lee's shot 3 (5 visible discs,
  29.9–36.3% of the ball overflowing) with no engine involved.
- **Local probes always clean** — their boxes are nowhere near the ceiling.
- **Mip 0 forcing, plain mips (2.85), and the FilterEnvMap fixes (2.84) never helped** — the
  corruption is in the *direction*, upstream of every mip decision. Correct diagnosis of the
  filter defects, wrong defect.
- **Cube dumps always looked fine** — because they were fine. The write was never the problem.
- **The "gap" colour tracked the Cloud Coverage slider** — the INF direction lands on a
  degenerate texel whose colour is sky.
- **Wiping the +X face black left "yellow" inside the wiped region** — those pixels were never
  reading +X; they were reading the degenerate direction.
- **The debug sphere looked perfect all night** — `cubeMapPS` has no parallax. We were
  repeatedly comparing the one path that cannot show the bug against the one that can.

### The fix (engine 2.89, `lightingHF.hlsli`)
Promote the parallax intermediates to `float` (and the clearcoat copy of the same block).
Cost is a handful of scalar ops per probe per pixel; correctness is restored for any box size.
**This is the third instance of the fp16 range class** in this codebase, and the second in this
very file — 2.07g was `half range2` overflowing past light range 255.9. Precedent also settles
the "is `min16float` really 16-bit here?" question: `-enable-16bit-types` is commented out in
`wiShaderCompiler.cpp:141`, but 2.07g was reproduced, fixed and user-confirmed as a genuine
fp16 overflow, so min-precision *is* executed at 16 bits on this hardware.

⚠ **Landmine found alongside**: `ShaderEntity::GetRange()` is also fp16 (`SetRange` packs via
`XMConvertFloatToHalf`), so the global probe's authored range of 100,000 reads back in every
shader as **+INF**. Nothing depended on it here, but never trust a probe/light range comparison
in a shader without checking that ceiling first.

### Knobs shipped with it
| command | ini key | meaning |
|---|---|---|
| `SET_PROBEPARALLAX 0` | `probeparallax=0` | stock half math — reproduces the defect on demand |
| `SET_PROBEPARALLAX 1` | `probeparallax=1` | float math — **the fix, default** |
| `SET_PROBEPARALLAX 2` | `probeparallax=2` | float + **magenta** on every pixel the stock math would break |
| `SET_PROBEPARALLAX 3` | `probeparallax=3` | float + skip parallax on level-sized boxes (design alternative, see below) |
| `SET_GLOBALPROBEBOX <n>` | — | resize the global probe's OBB live (threshold experiment) |
| `SET_PROBEVIEW <mode> [mip] [scale]` | `probeview` / `probeviewmip` | the inspection sphere (below) |

### PROBE INSPECTION MODE (`SET_PROBEVIEW`) — the requested debug mode
The engine already had a trustworthy raw-mirror sphere; it was just hard-wired to the *picked*
probe at *mip 0*. 2.89 re-points it:
- `1` = mirror the **GLOBAL** cube (`probes[0]`, exactly what shaders read as
  `GetScene().globalprobe`), `2` = mirror the **LOCAL** cube at the same spot, `0` = stock.
- `mip` selects the level `cubeMapPS` samples (via `MiscCB.g_xColor.x`), so the filtered chain
  can be walked on a live cube.
- Draws exactly one sphere: at the picked marker if there is one, otherwise hovering in front
  of the camera, so it works from the harness on any scene with no marker picked.
- Default OFF, zero cost when off.
Note for future use: this sphere is deliberately **not** a PBR surface and does **not**
parallax-correct — it shows cube CONTENT. Judging a reflection *defect* needs a real surface.

### Open design question left for Lee (mode 3)
Should a level-sized "global" probe be parallax-corrected at all? Wicked has no concept of a
global probe — GG makes `probes[0]` global by convention, and its 50,000-unit box then rides
the local entity array and wins the blend for every pixel. Parallax against a box that size
skews the reflection by the surface's offset from the probe centre for no physical gain, and
DX11 did not do it. Mode 3 reads the raw reflection vector for boxes over 37,820 units (DX11
behaviour). Mode 1 is the conservative default; mode 3 is one command away for comparison.

### Swept the neighbourhood for the same class (while the fix built)
Grepped every `half`-typed world-space quantity in `lightingHF` / `shadingHF` / `surfaceHF` /
`objectHF`. Only one other theoretical exposure, and it is not reachable in practice:
- `lightingHF.hlsli:217` `half water_depth = water_height - surface.P.y;` — a world-space Y
  delta. Needs a surface more than 65,504 units below the water plane to overflow; GG levels
  are ~100,000 units across but nothing sits that deep. Left alone, recorded here.
- `shadingHF.hlsli:68/320` `half3 clipSpacePos` is normalised box space (|·| ≤ 1 inside the
  box) and a far-outside surface simply fails `is_saturated` — benign, including when it
  reaches INF against a parked 1-unit pool probe.
- `shadingHF.hlsli:601` capsule-shadow `half range` is character-scale.

### §2.89 VERIFICATION — live, on Lee's TESTPRO2/spotshadowtest (2026-08-18 07:0x)
**Reproduced and fixed on the marker ball itself.** `SET_PROBEONLYGLOBAL 1` makes every surface
read the global cube — the same thing Lee's mouse-drag does by releasing the pool slot — so shot 3
can be reproduced without touching the mouse.

| | ball region only, 465×465 px |
|---|---|
| noise floor (two shots, nothing changed) | **0.325** mean, 1.02% of pixels |
| repeatability (mode 0 twice) | 0.270 |
| repeatability (mode 1 twice) | 0.278 |
| **THE FIX (mode 0 → mode 1)** | **8.839** mean, **18.86%** of pixels |
| THE FIX, independent second pair | 8.843 mean, 18.87% |
| **signal / largest control** | **27.2×** |
| mode 1 (parallax) vs mode 3 (raw R) | 4.145, 9.59% — the design question, a real but smaller change |

Two independent A/B pairs agreeing to 0.005 against a 0.325 floor. Screenshots:
`G1_ball_global_stockhalf` (Lee's circles, reproduced) → `G2_ball_global_floatfix` (clean).

★ **The single best picture is `G4_ball_global_overflowmap`**: mode 2 on the same ball paints
magenta wherever the stock maths exceeds 65504, and what survives in dark are EXACTLY the circles.
The circles were never something drawn onto the ball — they are the last few directions still
being computed correctly.

**Scene-wide** (whole viewport, mode 2 mask): the overflow covers the terrain, beach, rocks and
vegetation. The mode0→mode1 difference is **4.38× stronger inside the mask than outside**
(7.010 vs 1.600). ⚠ **Method note worth keeping**: a naive whole-frame screenshot diff CANNOT
settle this — live water/foliage/clouds give a 3–5% floor, and the box-size threshold sweep
(30000/37000/39000/50000 at mode 0, straddling the 37,820 ceiling) came back *inside* that floor
and proved nothing on its own. The mask test is the valid instrument; the ball-region test is the
clean one because that region is nearly static.

**Cost: nil.** spotshadowtest 77.4 fps mode 0 vs 77.4 mode 1. Island Showdown 66.3 / 66.4 / 66.3
for modes 0 / 1 / 3. Island Showdown's whole frame changes by only 1.45 mean (4.7% of pixels) —
the fix is dramatic on shiny surfaces and subtle on ordinary diffuse content, which is exactly the
expected shape and means no jarring look change on typical demos.

**Instrument receipts** (the "prove it is live" rule): mode 2 paints magenta — a behaviour that
did not exist before this build; `SET_PROBEVIEW 1 3` renders the inspection sphere as a uniform
blur where `SET_PROBEVIEW 1 0` is sharp — proving the new `MiscCB.g_xColor.x` mip path is live.
Both confirm the edited shaders actually reached the GPU (⚠ `refresh_shaders.ps1` reported
"0 stale" on this build — do not take that as evidence either way).

### §2.89 cross-demo look + cost check (editor, same camera per demo, modes 0 / 1 / 3)
| demo | mode 0 (defect) | mode 1 (fix) | mode 3 (no parallax) |
|---|---|---|---|
| spotshadowtest | 77.4 / 77.0 / 77.7 | 77.5 / 77.2 / 77.7 | — |
| Island Showdown | 66.3 / 66.4 / 66.2 | 66.6 / 66.3 / 66.2 | 66.2 / 66.4 / 66.5 |
| Switch Escape | 132.3 / 130.6 / 131.8 | 136.9 / 135.6 / 136.2 | 137.6 / 135.7 / 136.2 |

⚠ **Do not read Switch Escape's +4 as a speed-up from the fix.** Mode 0 ran first in each demo,
straight after the settle, so it absorbs the tail of lazy-PSO warm-up; modes 1 and 3 land within
0.7 of each other afterwards. Island Showdown and spotshadowtest — where warm-up had finished —
are dead flat. The honest verdict is **no measurable cost**, not a gain. (Standing rule: editor
FPS moves ±8 between launches; only within-session, warm-order-matched A/Bs mean anything.)

Island Showdown's whole frame changes by only **1.45 mean / 4.7% of pixels** between mode 0 and
mode 1, against 8.84 / 18.9% on the shiny ball. The fix is dramatic on reflective surfaces and
subtle on ordinary diffuse content — no jarring look change on typical demos, which is the
reassuring shape for a change that touches every level.

("Aztec Teaser" failed SELECT_DEMO in this run — the hub name is "Aztec Game Kit Teaser". Harness
name mismatch only, not a demo fault.)

### Housekeeping
`setup.ini` had two orphaned comment lines left over from the reverted `envdir` key (referencing
the scrapped SET_ENVDIR command). Removed; backup at `setup.ini.bak_2.89`. No probe debug keys
are set in the ini — 2.89's defaults apply (`probeparallax` = 1 = fixed, `probeview` = 0 = off).

### §2.89 acceptance gate — PARTIAL, and the one loose end
An editor-phase gate over all 19 hub demos was started (load → settle → screenshot → 3 FPS
samples → DUMP_VRAM each). It got 3/3 clean and then **stalled on Horseshoe Bend**, so the full
19 is **still owed**:

| demo | editor FPS |
|---|---|
| Aztec Game Kit Teaser | 61.8 / 61.5 / 61.5 |
| Aztec Game Kit | 85.3 / 84.7 / 84.9 |
| Bounty | 134.2 / 134.2 / 134.4 |
| Horseshoe Bend | ⚠ stalled (see below) |

Counting the look A/B, **five distinct demos** (those three + Island Showdown + Switch Escape)
plus spotshadowtest loaded and rendered correctly on 2.89.

⚠ **Horseshoe Bend, honestly stated**: it reached the editor and its SCREENSHOT succeeded — so
rendering was alive and the level loaded — but the following `GET_PERF_DATA` never returned. MAX
sat at a 5.8 GB working set and stopped answering the harness. I could not attribute this to the
2.89 change from that evidence alone. Points against it being 2.89: rendering worked, the
screenshot came back, and the stall was in the profiler command, not the draw path; Horseshoe
Bend is also the historical problem child (§ the 07-31 "3.7 FPS" prep-loop saga and the navmesh
cache work). Points that keep it open: it was not observed before this build **in this session**,
and no pre-2.89 control was run on that demo. **A retest is the first thing to do** — and if it
reproduces, the discriminator is already scripted: run `GET_PERF_DATA` with the fix OFF
(`SET_PROBEPARALLAX 0`) at the same pose; the shader binary is identical between those two modes,
so a stall in both exonerates 2.89 and a stall in only mode 1 convicts it.

#### Horseshoe Bend: RESOLVED — not 2.89, and the real culprit named
Retested on the same build. It loaded, screenshotted, and `GET_PERF_DATA` answered three times
(**102.4 / 98.6 / 103.1 fps**), then twice more with the fix OFF (103.2 / 103.4) and stayed
responsive to `GET_STATE` afterwards. **No stall, and FPS flat between modes there too.**

The first stall was environmental, and I caused part of it. ⚠ **`TaskStop` kills the wrapper, not
the script's process tree**: after I stopped the sweep, its shell survived and relaunched MAX 90
seconds later. That second instance then held `GameGuruMAX.exe` and failed two consecutive builds
with `LNK1104`, and it also means the "stall" I was mid-way through diagnosing had two MAX
processes in the picture. Cleanup that actually works: `ps -W`, find the PGID, `kill -9` every
member of the group, confirm zero survivors, then build. (Same family as the 08-10 leaked-runner
rule — `pkill` is dead in Git Bash — but the TaskStop wrinkle is new and worth the memory entry.)

### §2.89 ACCEPTANCE GATE — ★ 19/19 CLEAN (2026-08-18 07:27→08:02)
Re-run end to end after the leaked-runner cleanup. Every hub demo loads, renders, screenshots and
reports perf normally on the 2.89 build. **0 failures.** Editor FPS, 3 samples each:

| demo | FPS | demo | FPS |
|---|---|---|---|
| Aztec Game Kit Teaser | 68.3 / 68.1 / 67.9 | Foggy Forest | 60.1 / 60.2 / 59.9 |
| Aztec Game Kit | 94.4 / 93.7 / 91.4 | Indian Strike Force | 89.4 / 89.6 / 88.9 |
| Bounty | 105.7 / 104.6 / 106.0 | Switch Escape | 141.1 / 140.6 / 141.6 |
| Horseshoe Bend | 103.5 / 105.5 / 105.1 | Canyon Offensive | 68.8 / 68.7 / 68.8 |
| Island Showdown | 69.3 / 68.9 / 69.0 | Escape from the Zombie Cellar | 137.7 / 137.8 / 137.1 |
| Operation Amazon | 77.1 / 76.6 / 76.9 | Jungle Fever | 113.2 / 112.7 / 112.9 |
| River Raiders | 109.5 / 108.8 / 108.6 | RPG Template | 96.7 / 96.0 / 95.9 |
| Snowy Mountain Stroll | 137.6 / 138.2 / 137.9 | The Mystery of Z Island | 98.4 / 98.3 / 98.5 |
| A Grand Canyon Adventure | 93.3 / 92.4 / 93.3 | Trapped | 151.3 / 150.4 / 150.5 |

Horseshoe Bend passing at 103.5 is the **second independent** confirmation that its earlier stall
was environmental, not 2.89. Per-demo screenshots in the run folder; VRAM unchanged by
construction (the fix allocates nothing — it changes the precision of a few scalar ops).

---

## ★★★ §2.89 BASELINE MILESTONE — env-probe reflections, settled (2026-08-18)
**This is the anchor point. Read this section alone to know where env probes stand.**

### The defect and the fix
`EnvironmentReflection_Local` (lightingHF.hlsli) computed the parallax ray-exit `Distance` in
`half` — `min16float`, real fp16 on this hardware, ceiling **65504** — and that distance is in
**WORLD units**. It runs half-extent … half-extent×√3, so **any probe OBB over 65504/√3 = 37,820
units overflows to +INF** outside six ~40° caps around ±X/±Y/±Z. GG's `globalEnvProbe` box is
**50,000**. Those six surviving caps ARE the circles — not something drawn onto surfaces, but the
last directions still being computed correctly. Local probe boxes are 1–few hundred units, which
is the whole reason local reflections always looked clean.

### What ships (Lee-decided default, 08-18)
**`gg_probeparallax` DEFAULT = 3**: float precision **and no parallax correction against
level-sized boxes** — the DX11 behaviour. Rationale Lee accepted after seeing the A/B: parallax
against a box enclosing the entire level models nothing physical; it drags the reflected horizon
by however far the surface sits from the probe centre (the map origin), making the look
**position-dependent across a level**. Mode 3 reads the raw reflection vector instead.

| mode | behaviour | status |
|---|---|---|
| 0 | stock `half` math | the DEFECT — kept only for A/B |
| 1 | float precision, parallax kept | precision-only fix, retained for comparison |
| 2 | float + magenta map of fp16 overflow | diagnostic |
| **3** | **float + no parallax on level-sized boxes** | **SHIPPING DEFAULT** |

Measured on the marker ball forced onto the global cube: mode 0→1 = 8.48 mean / 17.8% of pixels;
mode 1→3 = 4.04 / 9.3%; mode 0→3 = 9.80 / 21.0%. Noise floor 0.325.
⚠ The precision fix still matters at mode 3 for any probe between room-size and the 37,820
ceiling — mode 3 only bypasses the block for boxes ABOVE it.

### Verification standing behind this milestone
- Circles **reproduced on demand** (`SET_PROBEONLYGLOBAL 1` + `SET_PROBEPARALLAX 0`) and gone at
  the default. Signal/noise **27.2×**, two independent A/B pairs agreeing to 0.005.
- Mode-2 overflow map on the same ball is the clearest artefact: magenta everywhere the fp16
  math breaks, the surviving dark patches exactly the circles.
- Predicted from the shader source alone by a numpy mirror-ball sim before the engine was touched.
- **19/19 hub demos** load, render and report clean. No measurable FPS cost. VRAM cannot move.

### Knobs (all ini keys INT PASSTHROUGH)
`SET_PROBEPARALLAX 0|1|2|3` (ini `probeparallax`) · `SET_PROBEONLYGLOBAL 0|1` ·
`SET_GLOBALPROBEBOX <n>` · `SET_PROBEVIEW <mode> [mip] [scale]` (ini `probeview`/`probeviewmip`)

### Durable lessons from this one
1. **fp16 is THE recurring bug class here** — third instance, second in this same file (2.07g was
   `half range2` past light range 255.9). Treat any `half` world-space quantity as a bug until
   measured. ⚠ `ShaderEntity::GetRange()`/`GetRadius()` are fp16 too, so the global probe's
   authored range of 100,000 reads back to every shader as **+INF**.
2. **An instrument that bypasses the suspect path can only exonerate it.** The debug probe sphere
   (`cubeMapPS`) has no Fresnel, no roughness and no parallax — it could never show this defect,
   and a whole night went into comparing it against the marker ball and concluding "the cube is
   fine". It was. Ask which stages an instrument actually exercises before trusting a clean result.
3. **On a live scene, whole-frame screenshot diffs have a 3–5% animation floor.** The box-size
   threshold sweep drowned in it and proved nothing. What worked: use mode 2 to get an exact mask
   of the affected pixels, then compare the A/B difference INSIDE vs OUTSIDE that mask.
4. ⚠ **`TaskStop` kills a script's wrapper, not its process tree.** A stopped sweep relaunched MAX
   90s later, held the exe, failed two builds with LNK1104 and muddied a hang mid-diagnosis.
   `ps -W` → find the PGID → `kill -9` every member → confirm zero survivors.

### Still open in env-probe land (NOT part of this milestone)
- The **2.77 capture defect** is live and real: a placed probe photographs its own enclosing
  'root' shell ("a circle image on each cube side"). Separate from the read-path bug fixed here.
- Cherry-pick candidates untouched: engine `b44c3f0e` (FilterEnvMap 3-part quality fix — a genuine
  IBL defect, just not this bug) and `d5ce3478` (plain 2×2 mips lever).
- Capture-position policy: the global probe still bakes at the map origin. `SET_GLOBALPROBEBOX`
  and the DX11 intent (ground + 30 m, still commented in GGTerrain_part0) are the starting points.

---

## §2.90 — Env probe marker properties: Probe Brightness restored, Probe Range removed (2026-08-18)

Lee, after the 2.89 milestone: *"probe range and probe brightness in the properties of the env
probe do not seem to do anything. Size XYZ works fine as I can see half the fish using the local
rather than the global env map, but I suspect the other 2 properties do nothing (or nothing
useful). No code changes just a report thanks."* Then, on reading the report: *"Fix the brightness
one, restore the DX11 filterBrightness and remove the probe range from the UI. For the probe range
value internally, you are free to replace any level-set value with one that works better for our
new DX12 engine."*

Full audit: **`PROBE_PROPERTIES_2026-08-18.md`** (that file is THE authority). Summary:

### Probe Brightness — a DX12 port regression, one line from working
The value plumbs perfectly from the panel into `g_envProbeList[].brightness` and then dies at
`GGTerrain_part0.cpp:9500`, a commented-out `//probe->SetBrightness(...)`. Written in one place,
read in one place, and that one place was commented out. It was commented out because the DX12
Wicked engine **deleted the feature** — DX11 had `filterBrightness` + `SetBrightness`
(`WickedRepo` `wiScene.h:1031,1034`), the CB field (`wiRenderer.cpp:9092`) and the shader multiply
(`filterEnvMapCS.hlsl:43`); DX12 had none of them.

Restored as DX11 designed it — **baked into the cube** during BRDF mip filtering, so zero
per-pixel cost. `filterBrightness` took the **free padding slot** in `FilterEnvmapPushConstants`,
so the struct's size and layout are unchanged.

Two deliberate departures from a literal copy:
- `SetBrightness` **self-dirties, but only on change**. A baked quantity needs a re-capture, and
  the caller runs on every probe-tracking update — an unconditional `SetDirty()` would re-bake
  forever. One re-bake per slider move. (`SetDirty()`→`DeleteResource()` is a no-op for GG's
  runtime-baked probes — it only clears asset-sourced ones — so there is no black-cube flash.)
- `filterBrightness` is **not serialized**, sitting with `position`/`range`. The source of truth
  is the .ele's `fProbeBrightness`, and GGTerrain re-pushes it every tracking update.

⚠ **Known gap, inherited from DX11: mip 0.** The filter loop is `i > 0` — mip 0 is a straight
`CopyResource` of the unfiltered render, so mirror-sharp (roughness ≈ 0) surfaces ignore the
slider and a roughness-0.1 chrome prop gets a partial effect. Closing it needs a scale pass over
`envrenderingColorBuffer` mip 0 before `GenerateMipChain`; deliberately not done here.

⚠ **The GLOBAL probe deliberately stays on the 1.55 shader knob.** The Visuals panel's "Env Probe
Brightness" already drives `gg_envprobe_brightness` inside `EnvironmentReflection_Global`; baking
it here too would apply the same slider **twice (squared)**. One owner per knob.

### Probe Range — never did anything, in DX11 either
GG writes `probe->range`, but `Scene::RunProbeUpdateSystem` (`wiScene.cpp:5734`) recomputes
`probe.range = max(scale.x,y,z) * 2` from the transform on **every** `Scene::Update` — the write is
clobbered before anything reads it. **The DX11 engine has the identical line**
(`WickedRepo/wiScene.cpp:4190`), so this was never a port regression; the slider was inert there
too. The probe volume comes solely from Size X/Y/Z (`pTransform->Scale`), which is exactly why
Lee's fish responded to XYZ and not to Range. No shader reads a probe's range at all — the only
GPU consumer is the tile-cull sphere, already derived from the scale.

What the value really is: a **flag, not a magnitude**. `fLightHasProbe` is literally "has probe",
tested `>= 50` in a dozen places (admit-to-list, `bIsLightProbe`, probe-vs-light branching). The
slider's own minimum was 50, so every reachable value 50–500 behaved identically.

Shipped: the slider is **removed** from the panel, and the value is **canonicalised to 50 on
load** (Lee-authorised) at both entry points — `.ele` (`M-Entity_part3.cpp:461`) and `.fpe`
`lightprobescale` (`M-Entity_part1.cpp:1163`, where **X/Y/Z keep the authored value** because they
are the real volume). Below 50 means "not a probe" and is untouched.

⚠ If Range is ever revived by multiplying it into the OBB scale: 500 × 500 = half-extent 250,000,
well past 2.89's 37,820 fp16 parallax ceiling — `probeparallax=3` would silently drop parallax on
that probe — and it would change every existing level's probe volumes.

### Harness
`SET_PROBEMARKERBRIGHTNESS <f>` — drives the per-probe slider on every probe marker and raises
`g_bLightProbeScaleChanged` exactly as the panel does. Distinct from `SET_ENVPROBE_BRIGHTNESS`
(global, shader-side, no re-bake).

### Durable lesson
**A knob can be dead in two entirely different ways, and the difference decides the fix.**
Brightness was a *severed* chain — every stage worked, one line was commented out, so restoring
the missing engine feature made it work. Range was a *clobbered* value — the write happened, the
engine simply overwrote it every frame from another source, in BOTH renderers. Grepping for "is
the value used?" would have called both "used". The question that separated them was **"who writes
this last, and who derives it from what?"** Range was a derived attribute all along; treating it
as an input was the original mistake, and the UI had been promising a behaviour no code
implemented since the DX11 days.

### §2.90a — 0818b acceptance sweep + pre-alpha dead-control audit
Full 19-demo editor gate on 2.90: **19/19, POLYS bit-identical, 4 GB gate holds (worst 3586
MB)**. Table + caveats in `DEMO_FPS_SWEEP.md` §0818b; dead-control audit and alpha
recommendations in `ALPHA_READINESS_2026-08-18.md`.

⚠ **I nearly published a false regression.** The first comparison used the 0816 baseline and
showed −13.4% across ALL 19 demos. The tell was the uniformity — a real regression hits the
demos exercising the changed path, not every demo equally. The control was already on disk:
the 2.89 gate run at 07:27 the SAME morning, which was itself −11.2% below 0816. 2.90 vs
that = −2.5% with two demos up. Cross-day FPS baselines are not evidence; this is the second
time this exact trap has appeared (cf. §2.75a) and the rule is in MEMORY for a reason.

**Dead-control audit — the useful part is what it proves absent.** The 07-28 audit traced
330 widget bindings to consumers, which catches a *severed* chain but is structurally blind
to a *clobbered* value — the Probe Range failure mode. So this pass enumerated every field
`Scene::Update` recomputes each frame and checked GG's writes against that set:
`probe->range` was the only instance (fixed in 2.90), `light->range` is safe because
`RunLightUpdateSystem` does not recompute it, and the other nine recomputed fields get
**zero** writes from the game. The class is closed.
Empty-body widget scan: 55 raw hits, zero real defects — 14 are the 07-28 hidden set
(commented out), the rest assign their bound temp through on the next line, and the two
genuine no-consumer cases are a dev-only Template Window slider and a one-item EBE combo.
MenuItem scan: 0 unhandled clicks.
One real find: `WickedCall_Create/Move/DeleteReflectionProbe` are **empty stubs still called
from 8 sites** (the editor preview "editorProbe" + one per entity) — the DX12 editor preview
has no reflection probe. Quietly absent feature, not a visible dead button.

⚠ Instrument that FAILED, recorded so it is not repeated: a scan for `t.visuals.*` fields
with no consumer reported 178 "orphans". All false — the regex matched the ini KEY STRINGS
(`"visuals.Gamma"`) inside the parser, not C++ member accesses. Discarded, not reported.

## §2.91 — "the GPU items don't add up": GPU Frame is a SPAN, not a sum (2026-08-19)

Lee: *"the individual items of the GPU performance breakdown does not add up to the millisecond
total time being taken by the GPU... I would always like to know where my GPU costs are going."*

**They never could add up.** `GPU Frame` is an ordinary range whose begin query rides the frame's
FIRST command list (`wiProfiler.cpp:326`, cmd from `BeginCommandList` at :388) and whose end query
is written manually onto the LAST (`:336`, the in-code comment says exactly why). The only
arithmetic is `range.time = (end - begin) / gpu_frequency` — there is no summation of children
anywhere. So the header is a **wall-clock span** and structurally contains three things no child
can: unranged passes, driver work at RenderPassBegin/End (CLEAR / STORE / MSAA resolve / barrier
drains), and intervals where the GPU had nothing to run. On Lee's screenshot it matched the frame
period to 0.014 ms (11.150 vs 1000/89.8 = 11.136) — it was reporting frame duration, not workload.

### ⚠ Two things I got wrong first, both corrected by the parallel audit
1. **I summed the rows and got 5.65 ms. Wrong — `Occlusion Culling` CONTAINS
   `Occlusion Culling Render`** (`wiRenderPath3D.cpp:1050` scope wraps the call at :1074). One
   pass counted twice. Correct children ≈ 5.24, gap ≈ 5.91.
2. **I blamed cross-queue fence bubbles. Wrong** — `gg_single_queue = true`
   (`wiGraphicsDevice_DX12.cpp:479`) routes COMPUTE/COPY onto graphics and elides same-queue
   waits, so all 12 intra-frame wait sites collapse. That story is off by default here.

### Shipped (2.91)
- **`GPU Busy` / `GPU Idle + unranged`** lines. Busy is the **UNION** of child tick intervals,
  deliberately not a sum — nesting would double-count, and a union is also right if async work
  ever overlaps again. Idle = Frame − Busy, clamped at 0.
- Ranges added for **`Postprocess_Tonemap`** (runs unconditionally, full-res ClearUAV + dispatch,
  had a PIX marker but no range) and the **transparent tail** (everything between the Transparent
  Scene range closing at `:2469` and `RenderPassEnd` — customDraw_Transparent/gpup, DrawDebugWorld,
  DrawWireframeOverlay, DrawLightVisualizers, DrawSpritesAndFonts, DrawLensFlares; only
  DrawSoftParticles was ever measured). ⚠ Explicit Begin/End, not `ScopedGPUProfiling` — the
  enclosing scope runs well past the pass and a scoped object swallowed the postprocess chain.
- **Editor UI (ImGui)** range in `master_part1.cpp`. GG never calls `wi::gui::Render` (zero call
  sites) so the engine's own `GUI Render` range never fired; the whole editor UI was uninstrumented.
- ★ **The one-line snapshot bug.** `g_cachedProfilerText = GetTextData()` sat immediately BEFORE
  `__super::Compose(cmd)`, while the comment above it said the point was to read "when all GPU
  ranges are active". `GetTextData` skips `!in_use` and `BeginFrame` clears `in_use` each frame,
  so ranges OPENED DURING Compose were never in_use at read time and could never appear —
  permanently hiding `Outline`, `Terrain - Debug`, `Terrain - Overlay`, all correctly instrumented.
  Moved one line later.

### Live receipt (Island Showdown, editor, profiler on)
`GPU Frame 14.62 = Busy 8.47 + Idle 6.15`. Naive sum of the visible rows is 8.90; Busy is 0.43
lower, which is the nested Occlusion Culling Render (0.54) correctly excluded, offset by ~0.11 of
`Editor UI (ImGui)` — counted in Busy but with no displayed row, because the text snapshot must
exist before the UI that draws it. That residual is a consistency check, not a direct measurement.

### Durable lesson
**A "total" and a "sum of parts" are different claims, and profiler headers are usually the
former.** Before treating any total as the sum of its breakdown, find the arithmetic that
produces it. And when summing a breakdown by hand, check for NESTING first — I made exactly the
double-count error that the new union counter now makes impossible.

### §2.91a — 0819 regression gate: CLEAN, and the noise floor finally measured
Two identical passes back to back on 2.91 (engine `ace9088a` / game `1d766a63`).
**19/19 both passes, zero failures, POLYS bit-identical on all 19 across A, B and 2.90,
worst editor VRAM 3588 MB (4 GB gate holds).** Table in `DEMO_FPS_SWEEP.md` §0819.

★★ **The two-pass design produced a calibration worth keeping: |A−B| median 0.8%, mean 0.9%,
worst 4.1%.** The project has been carrying "editor FPS ±8 between launches" as the drift band
and using it to wave away differences. That number is the CROSS-DAY band. Back-to-back
same-session runs resolve to about **1%** — ten times finer. Practical consequence: an
in-session A/B can trust a ~2% effect, while a cross-day comparison cannot be trusted below
~10%. Both halves of that matter; the first was being thrown away, the second is what burned
me on 0818b.

2.91 vs 2.90 = **+2.4% hub-wide**, i.e. faster. That is NOT a 2.91 effect — 2.91 is zero-cost
in this gate by construction (`BeginRangeGPU` early-returns on `!ENABLED`, the profiler
defaults off, the sweep never enables it, and the game-side snapshot move is gated on
`IsEnabled()`). It is ambient recovery, and the fingerprint proves it: the three biggest
gainers (Aztec Game Kit +7.0%, Zombie Cellar +6.9%, Trapped +5.9%) are exactly the cells 0818b
flagged as band-edge DROPS. Same demos, opposite direction, one day apart.

### §2.92 — the last unranged passes, and the negative result that matters
Lee: *"add the remaining ranges for the 2D layer and Compose."* Done — `RenderPath2D::Render`,
`RenderPath2D::Compose`, `RenderPath3D::Compose`. `wiRenderPath2D.cpp` had **zero** profiler
instrumentation and needed the include adding. All three use scoped ranges rather than explicit
Begin/End, because these are whole-function wraps with early-return paths and a scoped object
closes on every exit.

⚠ They **NEST**: `RenderPath3D::Compose` contains `RenderPath2D::Compose` (called at
`wiRenderPath3D.cpp:2009`) and `Outline` (via customDraw_Compose). Those rows overlap and must
never be summed — the Busy union handles it, which is exactly why Busy was built as a union.

★ **The measurement refutes my own earlier hypothesis, and that is the useful part.**
Live on Island Showdown: `RenderPath3D::Compose 0.03`, `RenderPath2D::Compose 0.01`,
`RenderPath2D::Render 0.00` — **~0.04 ms combined, against a 6.5 ms Idle bucket.** Add the 2.91
additions (`Postprocess_Tonemap 0.04`, `Transparent Tail 0.01`) and every pass I suspected of
hiding real cost totals **under 0.1 ms**. I had told Lee my instinct was that unranged work was
the larger share of the gap. It is not. The gap is overwhelmingly **genuine GPU idle plus
driver work at RenderPassBegin/End** (CLEAR / STORE / MSAA resolve / barrier drains), neither of
which any profiler range can ever capture.

Practical consequence: further range-hunting is now a poor use of time. If the ~6.5 ms is ever
worth attacking it is a *scheduling/pacing* problem (backbuffer acquire with BUFFERCOUNT=2,
submit pacing), not a missing-instrumentation problem — and a PIX capture is the right next
instrument, not more ranges.

Panel now (Island Showdown, profiler on): `GPU Frame 15.15 = Busy 8.61 + Idle 6.54`.

### §2.92a — 2.92 gate: all hard gates PASS, one cell UNRESOLVED (Aztec Game Kit)
Single-pass 19-demo gate on 2.92 (engine `74c8f21b`): **19/19 loaded, zero failures, POLYS
bit-identical on all 19, worst editor VRAM 3586 MB — 4 GB gate holds.** Hub-wide −1.5% vs the
2.91 A/B mean, median |Δ| 1.0% — at the noise floor measured yesterday (median 0.8%).

⚠ **NOT waved away: Aztec Game Kit read −8.2%, above yesterday's 4.1% worst-case floor.**
Per the new rule (in-session, trust ~2%), that needed testing rather than an argument, so the
demo was re-run three times on 2.92:

| | median FPS |
|---|---|
| 2.91 A / B | 92.7 / 91.6 — spread **1.2%** |
| 2.92 gate + 3 repeats | 84.6 / 89.2 / 88.2 / 86.5 — spread **5.4%** |

Mean 92.2 → 87.1 = **−5.5%**. Status: **unresolved, but a real regression is mechanistically
implausible.** Against it: 2.92 is zero-cost with the profiler off (`ScopedGPUProfiling` →
`BeginRangeGPU` → `if (!ENABLED || !initialized) return 0;`), the only other change being an
`#include`; this demo's own within-build spread (5.4%) already exceeds the hub floor; and the
three repeats decline monotonically (89.2 → 88.2 → 86.5) over ~20 min of continuous running,
which is the thermal signature, not a code signature. Also note Aztec Game Kit is the
project's most volatile cell — it was the single biggest GAINER (+7.0%) in the 0819 gate.

★ **The decisive test was NOT run and should be if this ever matters: revert 2.92, rebuild,
re-measure Aztec Game Kit in the same session.** Nothing short of a same-build A/B settles a
5% claim on a demo whose own spread is 5.4%. Flagged rather than closed.

### §2.92b — the Aztec cell: CLOSED by a revert A/B. 2.92 costs exactly nothing.
Lee asked for the decisive test. Ran it properly: built the pre-2.92 binary (revert
`74c8f21b` on the two .cpp files — 2.92 touches **no shaders**, so the exe is the only
differing artifact and an exe-swap A/B is valid), then measured **INTERLEAVED**
291/292/291/292… four rounds, fresh launch each time, so both builds ride the same thermal
ramp. Interleaving is the whole point — the prior suspicion was drift, and a
run-all-A-then-all-B design would have re-created exactly the confound it was meant to test.

| round | 2.91 (reverted) | 2.92 (shipped) | paired Δ |
|---|---|---|---|
| 1 | 85.9 | 85.7 | −0.2 |
| 2 | 86.0 | 85.6 | −0.4 |
| 3 | 84.3 | 85.6 | +1.3 |
| 4 | 84.8 | 84.1 | −0.7 |
| **mean** | **85.25** | **85.25** | **+0.00%** |

**2.92 costs 0.00% — the means match to two decimal places, and the paired deltas alternate
sign.** Within-build spread was 2.0% (2.91) and 1.9% (2.92), so the ±1.3 excursions are noise.

★★ **The decisive detail is what the REVERTED build did.** 2.91 measured 92.7 / 91.6 at 18:0x
and 85.9 / 86.0 / 84.3 / 84.8 at 22:2x — the *same binary*, ~7% slower four hours later. The
−8.2% attributed to 2.92 in the 2.92a gate is present in the build that PREDATES the change.
It is the machine, conclusively, and no amount of reasoning about `!ENABLED` early-returns
would have proven that — only the revert did.

Machine left in the shipped state: 2.92 exe installed (byte-verified against the saved copy),
both repos clean, source never left reverted (restored immediately after the 2.91 build).

### The durable lesson
**When a single cell exceeds the noise floor, the only closing move is a same-session A/B of
the two BUILDS — and it must be interleaved.** Mechanistic arguments ("this code can't cost
time") are a reason to *run* the test, never a substitute for it. And keep a baseline reading
of the reverted build in the same session: here it was the reverted build's own 7% drop that
settled the question, not the A-vs-B delta.

## §2.93 — Build-folder test-script audit: what is mine, what is stale (2026-08-20, Lee asked)

Lee, reading `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max`, asked whether scripts like
`run_all_tests.sh` are needed for ongoing project testing. They are not, and the reason is
worth recording so the question does not have to be re-answered.

**Inventory of loose scripts in the build root**

| File | Date | Verdict |
|---|---|---|
| `run_test.sh` | 2026-02-15 | stale |
| `run_fps_test.sh` | 2026-02-16 | stale |
| `run_remaining_tests.sh` | 2026-02-17 | stale |
| `run_all_tests.sh` | 2026-02-17 | stale |
| `perf_test.sh` | 2026-03-03 | stale |
| `dxdiagsystemspecs.bat` | 2021-03-11, 33 bytes | ⚠ **PRODUCT FILE — do not delete** |

**Why none of them are load-bearing**

- All five predate this campaign by ~6 months. They drive the same `auto_command.txt` /
  `auto_result.txt` harness protocol and iterate the same 19-demo list, so they are very
  likely earlier-session artifacts — but nothing in the current workflow reads them.
- **None are git-tracked** (the build directory is not a repo), so deleting them loses no
  history and no revert path other than the file itself.
- ★ **Every script this campaign runs is generated fresh into the session scratchpad**
  (`%LOCALAPPDATA%\Temp\claude\...\scratchpad`) and never written to the build folder. That
  is deliberate: the build folder is a build **output** and is what gets packaged for
  testers — anything left there ships.

**They are not dead code, though.** All six harness verbs they call (`NAVIGATE`, `CLICK_NODE`,
`PRESS_ESCAPE`, `SELECT_DEMO`, `CLICK_ONLY_LEVEL`, `GET_PERF_DATA`) still exist, so
`run_all_tests.sh` would probably still run. It also covers a phase the current sweeps do not:
it enters **test-game mode** (`CLICK test_level`) and samples in-game FPS, where the 19-demo
gate is editor-phase only. Two reasons not to simply keep it where it is: it writes results to
`/tmp/fps_results.txt` (a Git Bash temp path that evaporates), and it has **no lockfile** —
exactly the shape that produced the leaked-runner corruption recorded earlier.

**Disposition:** delete the five `.sh`, keep the `.bat`. If the in-game FPS phase is wanted
back, the right home is `GameGuru Core/tools/` (versioned, lockfiled), not the packaged output
directory. Not deleted yet — awaiting Lee's go, same protocol as the 328 MB debris clear
(manifest first, non-recursive, verified after).

### The durable rule
★ **Never leave working files in the build output directory.** It is packaged, it is not
version-controlled, and six months later nobody can tell a live test rig from an artefact
without reading it. Scratchpad for throwaway, `tools/` for anything worth keeping.

## §2.94 — PHASE 2 PERF: the four brutal off-switches, and what they measured (2026-08-22, Lee-directed)

Lee's alpha ran the Aztec demo at 20 FPS on an old AMD card where DX11 managed 50. Brief: add
"serious off switches" to Graphics and Performance — Terrain / Trees / Grass / Water — that do
not merely stop drawing but remove the elements from the entity system, and drive the editor
GPU figure on `aztec game kit teaser` from ~16 ms to under 6 ms. Explicitly: implement first,
investigate later.

### What shipped
Four session-scoped effective flags (`gg_no_terrain/_trees/_grass/_water`, GGTerrainWicked.cpp
next to `gg_lowvram`), each `(machine || level)`: setup.ini `noterrain`/`notrees`/`nograss`/
`nowater` OR a tick box in the panel. Setters sit beside `GGSetLowVRAM` in master_part1.cpp.
Harness: `SET_TERRAINOFF`/`SET_TREESOFF`/`SET_GRASSOFF`/`SET_WATEROFF` plus a keyed `GET_GPUMS`.

| switch | mechanism | why not the obvious thing |
|---|---|---|
| Water | override `bWaterEnabled` after the editor/game if/else (M-GridEditB_part3.cpp) | NOT `visuals->bWaterEnable = false` — that field is re-derived on every level load and by `wicked_set_water_level`, so the switch would un-stick AND it would overwrite the level's authored setting |
| Trees | `ReleaseTreePool()` at the top of `GGTrees_WickedUpdate`, plus folding the flag into `draw_enabled` | NOT `GGTrees_HideAll()` — it writes bit 0 of all 400,000 instance data words, which is LEVEL DATA a later save would persist |
| Grass | `ForceGrassRebuild()` + gate `ProcessGrassChunks` (the only caller of `hairs.Create`) | NOT `GGGrass_RemoveAll()` — it zeroes all 16 MB of `pGrassMap` in place, destroying the level's PAINTED grass |
| Terrain | `GGTerrainWicked_Shutdown()` — removes the Terrain component, taking `terrains.GetCount()` to 0 | NOT `SetTerrainVisible(false)` — chunks still exist, still pay Scene::Update, still hold the ~576 MB atlas, and `Generation_Update` KEEPS MAKING NEW ONES (the 2.68f note) |

Two latent bugs fixed on the way in, both in code that had never executed:
`GGTerrainWicked_Shutdown` had ZERO callers, and it called `Generation_Cancel()` (which waits
only on `generator->workload`) without joining the async VT job — the exact use-after-free the
GGMAX 1.45 comment in `Generation_Restart` documents. Engine delta 2.94 exposes
`wi::terrain::gg_WaitVirtualTextureJob()` for it. It also left the three grass tracking maps
holding entity ids for chunks it had just removed.

### MEASURED, aztec game kit teaser, editor opening view, profiler on throughout
Baseline reproduces Lee's screenshot exactly (15.6 ms / 63.1 FPS / 10,330,135 polys).
Each row is a mean of two reads; the ladder returns to baseline between switches.

| stage | GPU frame | GPU busy | GPU idle | CPU | FPS | POLYS |
|---|---|---|---|---|---|---|
| baseline (all on) | 15.60 | 9.38 | 6.22 | 6.5 | 63.3 | 10,330,135 |
| water off | 15.62 | 9.44 | 6.18 | 6.5 | 63.2 | 10,330,135 |
| grass off | 14.34 | 8.06 | 6.32 | 6.4 | 69.2 | 10,330,135 |
| trees off | 12.71 | 6.55 | 6.38 | 4.7 | 77.8 | 6,370,501 |
| **terrain off** | **5.91** | **4.61** | **1.29** | 2.8 | **167.3** | 6,180,925 |
| all four off | 5.85 | 4.51 | 1.31 | 2.9 | 163.4 | 6,180,925 |

★★ **TARGET MET: 5.85 ms, under the 6 ms goal, at 163-167 FPS against a 190 FPS DX11 reference.**

### ★★★ The finding that matters more than the switches
**Terrain owns essentially the whole gap, and most of what it owns is NOT shading.**
Look at the GPU Idle column: 6.2 ms at baseline, **1.3 ms with terrain off**. Roughly 5 ms of
the "GPU Idle + unranged" bucket that 2.91 exposed — the bucket that shows up in no profiler
row, which is exactly why Lee's rows never added up — is terrain. Terrain removal is worth
9.7 ms of a 15.6 ms frame while removing only 4.15 M of 10.33 M polygons, so this is not a
triangle story.

The mechanism is named in the recon: with `terrains.GetCount() > 0`, wiRenderPath3D opens
THREE extra per-frame command lists (`CopyVirtualTexturePageStatusGPU` :946,
`AllocateVirtualTextureTileRequestsGPU` :1880, `WritebackTileRequestsGPU` :1889) and inserts
`device->WaitCommandList(cmd_allocation_tilerequest, cmd)` at :1876 — a cross-queue dependency
on the opaque scene, **every frame**. A stall is not work; it can never appear as a range.
That is the next real optimisation and it does not require giving anything up visually.
⚠ NOT YET PROVEN — the attribution above is a read of the code plus the Idle delta. Prove it
with PIX before acting.

### Two implementation traps, both caught by measurement not by reading
1. ★★ **An edge-triggered teardown latch must be cleared on BOTH edges.** The first grass
   implementation latched `s_ggNoGrassApplied` inside `if (gg_no_grass)` and never reset it, so
   the switch was ONE-WAY — and because the ladder alternates, every later reading in that run
   was silently taken on a grass-free scene. Visible only because the return-to-baseline rows
   did not return.
2. ★★ **Clearing the state is not the same as re-running the pass.** Even with the latch fixed
   grass did not regrow: `ProcessGrassChunks` is gated on
   `grassDirty || chunkSig changed || settleRetry || g_grassPassNudge || camera moved > 8 units`
   and a parked camera on a settled level fires none of them. The fix is one line —
   `g_grassPassNudge = true` on the OFF->ON edge. Confirmed both ways twice: 15.60 -> 14.34 ->
   15.64 -> 14.18 -> 15.70.

### The wasted first run — worth recording
The first ladder was driven with `OPEN_PROJECT "Aztec Game Kit Teaser"`, which returned
`ERROR: project not found (3 available): TESTPRO2 REMOTEY TESTPRO1` — **OPEN_PROJECT only sees
My Games projects; the 19 hub demos need `SELECT_DEMO` + `CLICK edit_game` + `CLICK_ONLY_LEVEL`
(the tools/probe_one.sh sequence).** The run completed and produced a full, plausible,
entirely worthless table taken on the hub's empty scene (POLYS 17,424 against the real level's
10,330,135). ★ It only failed loudly at the top of the log; every row after that looked fine.
Gate a measurement on the load having SUCCEEDED, not on the script exiting 0.

### Known limitations (not defects to chase yet)
- **Water off measured 0.00 on this level** — the aztec teaser has no ocean. The switch is
  therefore UNVERIFIED here. Its value is the planar-reflection pass (4.37 ms of a 10.06 ms
  frame on the TESTPRO1 island, PERFORMANCE.md:686-687); verify on a water level.
- **Terrain off leaves no ground.** It is a measurement instrument and a last-resort user
  switch, not a shipping default.
- State is SESSION-scoped by choice: a global, not an FPM field, so it survives level loads
  within a session and writes nothing into the user's levels. Per-level persistence is a known
  follow-up (Types.h field + 4 sites in M-Visuals_part0.cpp + the test-game carry-back in
  M-GridEdit_part2.cpp) and was deliberately deferred.

## §2.94b — TESTPRO2 terrain isolation: the ~4.85 ms is FIXED and content-independent (2026-08-22)

Lee saved TESTPRO2 with everything else stripped back and asked for a terrain-only A/B.
Loaded level: `spotshadowtest.fpm` (the first level node). Interleaved A/B/A/B/A/B on one
binary, three pairs, profiler on throughout.

⚠ Note the four 2.94 switches are SESSION globals and all read 0 at launch, so whatever
TESTPRO2 has "switched off" is in the level's own visuals, not these switches. Consistent with
the poly count: 503,491 with terrain, 26,813 without — i.e. terrain is ~477 K polys, nothing.

| stage | GPU frame | GPU busy | GPU idle | CPU | FPS | POLYS |
|---|---|---|---|---|---|---|
| A1 terrain ON  | 9.16 | 3.20 | 5.95 | 4.11 | 107.1 | 503,491 |
| B1 terrain OFF | 2.45 | 1.26 | 1.20 | 2.27 | 391.5 | 26,813 |
| A2 terrain ON  | 9.18 | 3.21 | 5.97 | 4.21 | 102.7 | 503,491 |
| B2 terrain OFF | 2.34 | 1.22 | 1.13 | 2.26 | 391.8 | 26,813 |
| A3 terrain ON  | 9.26 | 3.21 | 6.04 | 4.37 | 103.4 | 503,491 |
| B3 terrain OFF | 2.39 | 1.22 | 1.17 | 2.26 | 391.2 | 26,813 |
| A4 terrain ON  | 9.20 | 3.20 | 6.01 | 4.27 | 103.3 | 503,491 |
| **mean ON**  | **9.20** | **3.21** | **5.99** | 4.27 | **104.1** | |
| **mean OFF** | **2.40** | **1.23** | **1.16** | 2.26 | **391.5** | |
| **delta**    | **−6.80** | **−1.98** | **−4.83** | −2.01 | +287 | |

Within-state spread under 1%. Terrain costs **6.80 ms/frame on a level with 477 K terrain
polygons and nothing else** — 104 FPS becomes 391 FPS.

### ★★★ The decomposition, now measured on two very different levels
| | aztec teaser (4.15 M terrain polys) | TESTPRO2 (0.48 M terrain polys) |
|---|---|---|
| GPU **busy** delta | −4.77 | −1.98 |
| GPU **idle** delta | **−4.93** | **−4.83** |

**The busy half scales with terrain geometry. The idle half does not — it is ~4.85 ms on both,
a 2% spread across a 9× difference in terrain polygons.** So roughly 4.85 ms per frame is a
FIXED tax that any level with terrain pays regardless of content. At 60 FPS that is 29% of the
frame budget spent in no measured pass.

### ⚠ CORRECTION to the §2.94 mechanism guess
§2.94 named the cross-queue `WaitCommandList` at wiRenderPath3D.cpp:1876 as prime suspect.
That is WEAK: `gg_single_queue = true` (wiGraphicsDevice_DX12.cpp:479) already collapses the
queues, so there are no cross-queue bubbles to pay. The three extra command lists still exist
and still serialise, but the specific mechanism is now UNKNOWN. Per-row diffing does not reach
it either — with terrain on, GPU Busy is 3.17 of a 9.14 ms frame and no GPU row is over 0.93 ms.

A cost that is invisible to every range, fixed in size, and disappears with the Terrain
component is exactly what PIX exists for. That is the next instrument; do not guess again.

### The profiler row fix is confirmed working
105 rows parsed with real values in both states (the fix routing M-GridEdit_part1.cpp through
the cached snapshot). Largest CPU movers on terrain removal: `VT-job Total` 0.96 → 0.00,
`Update - Wicked (Total)` 2.18 → 0.95, `Update Buffers (GPU)` 0.93 → 0.23, `VT-job PageBuf`
0.45 → 0.00.

## §2.94c — ★★★ THE 4.85 ms ROOT-CAUSED: `WritebackTileRequestsGPU` = 3.96 ms/frame (2026-08-22)

Lee asked for a PIX capture in each state. **PIX is not installed on this machine** (only
`WinPixEventRuntime.dll`, which emits markers and cannot capture) and a `.wpix` needs the PIX
GUI to read. Rather than install anything, I built the instrument the data already supported —
and it answered the question more precisely than a capture would have.

### The gap report (engine 2.94c, harness `DUMP_GPUGAPS`)
"GPU Idle + unranged" says HOW MUCH dead time a frame holds, never WHERE. But the dead time is
literally the holes between the measured intervals, and those intervals are already collected
in raw ticks for the Busy union. So: sort, merge, and report the biggest holes **labelled with
the range that closed before each one and the range that opens after it**.

First run on TESTPRO2, and it was not subtle — one hole, four times over:

    4.394 ms  after [Opaque Scene]  before [Transparent Tail]      <- terrain ON
    4.152 ms  after [Opaque Scene]  before [Transparent Tail]
    5.365 ms  after [Opaque Scene]  before [Transparent Tail]
    0.078 ms  after [Opaque Scene]  before [Transparent Tail]      <- terrain OFF

The only thing the engine does between those two points that is conditional on terrain is
`if (scene->terrains.GetCount() > 0)` at wiRenderPath3D.cpp:1873, which opens
`AllocateVirtualTextureTileRequestsGPU` and `WritebackTileRequestsGPU`.

### ★ It was never idle. It was UNMEASURED WORK.
All three terrain VT passes carried `device->EventBegin(...)` — a **PIX marker**, which is not
a profiler range — and no `BeginRangeGPU`. So their GPU time could not appear in any row by
construction; it fell into the unranged bucket and read as dead space. Added ranges to all
three. The hole vanished and the accounting closed:

| | before ranges | after ranges |
|---|---|---|
| GPU Frame | 9.12 | 9.25 |
| GPU **Busy** | 3.05 | **7.71** |
| GPU **Idle + unranged** | **6.08** | **1.54** |

Same frame time; 4.66 ms moved from "idle" into named rows. Measured, TESTPRO2, terrain on:

    TerrainVT - WritebackTileRequests: 3.96 ms   <- THE CULPRIT
    TerrainVT - AllocateTileRequests:  1.09 ms
    TerrainVT - CopyPageStatus:        0.02 ms
    Opaque Scene:                      0.62 ms   <- drawing the entire scene
    Shadowmap Rendering:               0.00 ms

**`WritebackTileRequestsGPU` costs 3.96 ms/frame — SIX TIMES the cost of drawing the whole
scene — on a level whose visible content is 26,813 polygons.**

### Why it costs that
Per virtual texture in use, EVERY FRAME, it issues **four `CopyResource` calls**: the
allocation-buffer readback, then feedbackMap + requestBuffer + allocationBuffer clears (upstream
uses copies-from-a-clear-resource because ClearUAV "was having a very bad performance especially
with DX12" — that comment is still in the source). `TERRAIN_RING` reports **chunks=625**, so
this is on the order of **2,500 CopyResource calls plus barriers per frame**, purely for
residency bookkeeping. Fixed cost, independent of scene content — exactly the signature
measured in 2.94b (~4.85 ms on two levels 9x apart in terrain geometry).

WARNING This is UPSTREAM Wicked code, not a GGMAX port artifact. It bites here because GGMAX
runs a 625-chunk ring with per-chunk virtual textures.

### Corrections to my own two earlier guesses (both wrong; recorded so neither returns)
1. 2.94 blamed the cross-queue `WaitCommandList` at wiRenderPath3D.cpp:1876. Wrong.
2. 2.94b retracted that because `gg_single_queue` is true — right conclusion, and the mechanism
   is now confirmed: `BeginCommandList` rewrites QUEUE_COMPUTE/QUEUE_COPY to QUEUE_GRAPHICS when
   `gg_single_queue` (wiGraphicsDevice_DX12.cpp:5951), so the queue-equality test in
   `WaitCommandList` (:6768) passes and no semaphore is ever created. No fence, no bubble.
   But that retraction also said the mechanism was unknown and needed PIX. It needed neither —
   it needed a profiler range around code that had never had one.

### ★★★ The durable rule
**A PIX/debug marker is NOT a profiler range.** `EventBegin`/`EventEnd` is invisible to
`wi::profiler`, so any pass instrumented only with markers is guaranteed to land in the
unranged bucket and read as GPU idle. Before concluding "the GPU is stalling", check whether
the suspect region is merely UNMEASURED. Sibling: an idle bucket is not evidence of idling — it
is evidence of an accounting hole, and the gap report is how you find its edges.

### Where this leaves the optimisation (NOT done — Lee's call)
None of this needs an off-switch. Cheapest first:
1. **Stop running writeback every frame.** Residency feedback does not need per-frame latency.
   GGMAX already has `gg_vt_incremental` and a `gg_vt_frozen` hysteresis; this pass is gated by
   neither. Every-Nth-frame should cut it near-proportionally.
2. **Batch the three clear copies** into one copy from a single combined clear resource.
3. Fewer resident VTs (ring size / mip bias) reduces it linearly.

## §2.94d — VT tile-request round trip now runs every 4th frame (2026-08-22, Lee: "do option 1")

`wi::terrain::gg_vt_writeback_interval` (default **4**), gated at the wiRenderPath3D.cpp call
site so the command lists are not even opened on skipped frames. Harness `SET_VTWRITEBACK <n>`;
1 restores stock every-frame behaviour for A/B.

### ★ Allocate and Writeback MUST share the cadence — gating writeback alone is a BUG
Lee asked for writeback every 4th frame. Implemented as the PAIR, and that is not scope creep,
it is correctness: **`WritebackTileRequestsGPU` is what CLEARS feedbackMap, requestBuffer and
allocationBuffer.** Skip only writeback and `AllocateVirtualTextureTileRequestsGPU` still runs
its TILEREQUESTS CS every frame, re-deriving requests from an uncleared feedbackMap into an
uncleared requestBuffer — duplicate requests feeding TILEALLOCATE, with free-tile exhaustion the
plausible end state. It would also burn Allocate's 1.09 ms/frame producing an allocationBuffer
that nothing reads 3 frames in 4.

Gating the pair is coherent instead: feedback simply ACCUMULATES over the interval (the object
shader `InterlockedOr`s into feedbackMap and nothing else touches it), then one allocate pass
converts the accumulated set and one writeback exports and clears. Each decision sees MORE
samples, not fewer. `CopyVirtualTexturePageStatusGPU` is deliberately NOT gated — opposite
direction (CPU page table -> GPU), already incremental via `gg_vt_incremental`, and 0.02 ms.

### MEASURED — interleaved 1/4/1/4/1/4, one binary, profiler on
| | interval=1 (stock) | interval=4 | saving |
|---|---|---|---|
| **TESTPRO2** frame | 9.27 ms | 5.87 ms | **−3.41 ms (−36.7%)** |
| **TESTPRO2** FPS | 106.1 | 166.4 | **+60.3** |
| **Aztec teaser** frame | 15.58 ms | 12.30 ms | **−3.28 ms (−21.1%)** |
| **Aztec teaser** FPS | 63.2 | 80.9 | **+17.7 (+28%)** |

★ The rows corroborate the arithmetic exactly: at interval=1 Aztec reads
`WritebackTileRequests` 3.34 + `AllocateTileRequests` 1.09 = 4.43 ms, and 3/4 of that is
3.32 ms against a measured saving of 3.28 ms.

**Nothing is switched off for this. Terrain, trees and grass all still render.** POLYS identical
at 10,330,135 in both states.

### Visual check — done under the FAILING context, not a parked camera
A parked screenshot cannot show streaming lag, so the test forces a genuine cold re-stream:
`SET_CAMERA` 400000 away, settle 8 s, `SET_CAMERA` home, then shoot after a SHORT 2 s settle.
Two reps per interval.

| | edge energy (texture sharpness) |
|---|---|
| rep1 iv=1 | 29.332 |
| rep1 iv=4 | 29.351 |
| rep2 iv=1 | 29.310 |
| rep2 iv=4 | 29.348 |

Spread across all four is 0.041 (0.14%), and iv=4 reads marginally SHARPER than iv=1 in both
reps — i.e. the between-condition difference is smaller than the within-condition noise.
Whole-frame mean|diff| was 5.6/6.3 across conditions against same-condition CONTROLS of 4.2/3.0,
so that channel is dominated by the scene's animation floor and says nothing either way — which
is exactly why the edge-energy measure is the one quoted.

⚠ HONEST LIMIT: this tests the state 2 s (~160 frames) after a cold re-stream, not the
sub-200 ms window. The change adds at most 3 frames of feedback->decision latency (~37 ms at
80 FPS). Lee flying the level fast is still the better judge than any of this.

### Where Aztec now stands
| | GPU frame | FPS |
|---|---|---|
| before today | 15.6 ms | 63 |
| **VT cadence 4, nothing turned off** | **12.3 ms** | **81** |
| all four brutal off-switches | 5.85 ms | 167 |

⚠ Not yet gated by the 19-demo sweep. That should run before the alpha, since this changes
terrain streaming behaviour on every level.

## §2.94e — THE TERRAIN BAKE, TESTED WITHOUT WRITING A BAKER (2026-08-22, Lee's proposal)

Lee: bake the final chunk-generated terrain into dumb static meshes with a basic texture,
dropping dynamic terrain management and VT; he expected the residual ~2 ms CPU + ~2 ms GPU to
"mostly disappear", since it would be "a few state changes and a few extra draw calls".

★★★ **It turned out the proposal could be TESTED EXACTLY, today, with no baker and no assets.**
Virtual texturing on a terrain chunk is ONE INTEGER: `sparse_residencymap_descriptor` in the
material's texture slot (ShaderInterop_Renderer.h:220, set at wiTerrain.cpp:2102/2111). Set
`Terrain::gg_near_ring_dist = 0` and every chunk takes `min_resolution`, gets NO residency
object, is therefore skipped by all four VT GPU passes (each opens `if (vt->residency ==
nullptr) continue;`), and binds with the descriptor at -1 so the pixel shader falls through to
a plain `tex.Sample`. Same shader, same PSO, no permutation. That IS the proposal's runtime
state. Harness `SET_TERRAINBAKE 0|1` (0 = GGMAX default 4, 1 = bake-equivalent 0), with an
executed-check that re-reads `residencyVTs` — confirmed 49 -> **0** -> 49 -> **0**.

### 1. What the residual actually is (TESTPRO2, after 2.94d)
Terrain ON vs OFF: **GPU frame +3.31 ms, GPU busy +2.02, GPU idle +1.40, CPU +1.78.**

| row | ON | OFF | delta | class |
|---|---|---|---|---|
| Update - Wicked (Total) | 2.19 | 0.94 | +1.26 | 625 chunk ENTITIES in Scene::Update |
| VT-job Total + PageBuf | 0.75 | 0.00 | +0.75 | dynamic VT only |
| Opaque Scene | 0.73 | 0.12 | +0.61 | drawing terrain |
| Update Buffers (GPU) | 0.91 | 0.33 | +0.58 | entity/instance upload |
| Z-Prepass | 0.40 | 0.02 | +0.38 | drawing terrain |
| Update - Terrain + Wicked Bridge | 0.29 | 0.02 | +0.27 | Generation_Update |

### 2. Is the residual entity-count or pixels? — the chunk-ladder (`SET_TERRAINGEN`)
Chunk count is (2n+1)^2, so the ring radius scales ENTITY count while barely touching what is
drawn. This is a direct proxy for "what would merging 625 meshes buy?".

| gen | chunks | POLYS | GPU frame | CPU | cost above terrain-off (GPU / CPU) |
|---|---|---|---|---|---|
| 12 | 625 | 506,388 | 5.99 | 4.27 | +3.29 / **+1.72** |
| 6 | 169 | 475,180 | 5.53 | 3.55 | +2.83 / +0.99 |
| 3 | 49 | 445,860 | 5.43 | 3.37 | +2.73 / **+0.82** |

★ **Removing 92% of the entities halves the CPU residual (1.72 -> 0.82) and moves GPU busy by
12% (2.02 -> 1.78), with POLYS essentially unchanged.** So the CPU residual IS an entity-count
tax; the GPU residual is NOT.

### 3. The bake-equivalent A/B (nearRing 4 vs 0, interleaved, executed-checked)
| arm | GPU frame | GPU busy | GPU idle | CPU | FPS |
|---|---|---|---|---|---|
| stock nearRing=4 | 5.93 | 3.31 | 2.62 | 3.96 | 166.8 |
| **BAKE-EQUIV nearRing=0** | **4.66** | 3.19 | **1.47** | 4.15 | **207.2** |
| terrain OFF | 2.62 | 1.24 | 1.38 | 2.18 | 372.1 |

★★ **−1.27 ms GPU frame, +40.4 FPS, from ONE INTEGER.** But read the columns: GPU busy moved
only −0.11; the entire saving is the idle/unranged bucket (−1.15), i.e. still-unmeasured
residency work. ⚠ **CPU got 0.19 ms WORSE, not better.**

### ★★★ VERDICT ON THE PROPOSAL — half right, and the halves are separable
Lee's idea is really TWO independent changes, and they pay off in opposite places:

| | mechanism | measured | verdict |
|---|---|---|---|
| **Drop VT / plain texture** | one integer, no baker | **−1.27 ms GPU, +40 FPS**, CPU +0.19 | ★ real, and available TODAY |
| **Merge 625 chunks -> few meshes** | needs a real baker | **−0.90 ms CPU**, −0.24 GPU busy | ⚠ risky, see below |

- "A few state changes and a few draw calls" — right about the MECHANISM, but the draw calls
  were never the cost. Chunks do not instance (each has a unique meshID, so every visible chunk
  is one DrawIndexedInstanced) yet cutting them 12.8x moved GPU busy by 0.24 ms.
- "2 ms CPU and GPU will mostly disappear" — the GPU half loses 38% (1.27 of 3.31), not most;
  the remaining 2.05 ms is rasterising terrain, which a bake still has to do. The CPU half gets
  NOTHING from dropping VT and needs the merge instead.
- ⚠ **Mesh merging has a real downside**: LOD (7 levels) and frustum culling and occlusion
  queries are all PER OBJECT. One merged AABB is never occluded, never culled, and pins LOD —
  so it can draw MORE triangles than today. Merge to a modest grid (say 5x5), never to one mesh.

### Quality cost of the bake-equivalent
Ground detail (edge energy over the lower 45% of frame) stock 22.561/22.575 vs bake
22.227/22.186 — a consistent **−1.6%**, about 10x the within-condition spread, so real but
subtle. ⚠ **Measured on spotshadowtest, a nearly empty scene.** nearRing=0 puts every chunk at
256 px instead of 2048, so on a painted, textured level with the camera near the ground the
loss will be considerably more visible than 1.6%. **This needs Lee's eye before it ships.**

### Other findings worth more than the bake (from the parallel code analysis, NOT yet acted on)
1. ★★ **`GGTerrainWicked_Update` has NO editor/game discriminator.** The Test Game and the
   exported exe run the full sculpt/paint bridge, the chunkSig census and a SECOND
   Generation_Update every frame — none of which can do anything in a shipped game. Gating on
   `t.game.gameisexe` is free CPU in exactly the place Lee cares about, with no visual change.
2. ★★ **The engine's own `Generation_Update` (wiScene.cpp:180) is UNGATED.** The GGMAX idle gate
   (GGTerrainWicked.cpp:3344) skips only the BRIDGE call, 7 frames in 8. On a parked, settled
   scene the full 625-chunk walk still runs every frame via the engine caller.
3. `gg_instinit_parallel` (harness SET_INSTINIT) defaults OFF and its own comment calls the
   serial instance-slot init "the largest unnamed cost in Scene-S1".
4. The SVT atlas is ~480 MB fully committed at boot and held forever — on every level,
   including indoor ones. Relevant to the 4 GB floor, independent of frame time.

### RECOMMENDATION
Do NOT write a baker yet. In order:
1. Lee eyeballs `SET_TERRAINBAKE 1` on a painted level (Aztec). If the ground holds up, ship it
   as a **low-end preset knob**, not a bake — it is +40 FPS for one integer.
2. Do the `gameisexe` gate on the terrain bridge. Free, no visual change, targets the exported
   game directly.
3. Only then consider a real bake, and only for the mesh-merge half, at a coarse grid — and
   measure against the culling/LOD loss rather than assuming it is free.

## §2.94f — the terrain idle gate was only HALF applied (2026-08-22)

The GGMAX terrain idle gate (terrain calm >45 frames, then skip 7 frames in 8) gated the
BRIDGE's `Generation_Update` call. `Scene::Update` calls `Generation_Update` a SECOND time on
the same frame (wiScene.cpp:180) and that caller was never gated — so on a parked, settled
scene the full 625-chunk walk still ran every frame: per chunk a frustum test plus four
component hash lookups, then `UpdateVirtualTexturesCPU` over all of them again.

The bridge already computes the right predicate and runs before `__super::Update` in the same
frame, so it now publishes it as `wi::terrain::gg_skip_generation_update`.
★ **CONSUME-AND-CLEAR on the engine side, deliberately.** The flag is a one-shot the bridge
must re-arm every frame. A sticky `true` — a host without the bridge, a load path, an early
return added later — would gate generation FOREVER and terrain would never appear. Stock
every-frame behaviour is the only safe direction for this flag to rot in. The bridge also
clears it at the top of `GGTerrainWicked_Update`, ahead of every early return.

### MEASURED, TESTPRO2 (profiler rows — see the caveat below on why not frame time)
| row | stock | gate | delta |
|---|---|---|---|
| VT-job Total | 0.870 | **0.000** | −0.870 |
| VT-job PageBuf | 0.450 | **0.000** | −0.450 |
| Update - Wicked (Total) | 1.890 | 1.570 | −0.320 |
| Update | 2.870 | 2.570 | −0.300 |
| Update Buffers (GPU) | 0.920 | 0.750 | −0.170 |

Gating the engine caller also stops `UpdateVirtualTexturesCPU`, so the async VT job is never
kicked on gated frames — hence VT-job going to zero.

★★ **Be precise about WHERE the time is.** ~0.30 ms comes off the MAIN thread (matching the
−0.29 ms CPU frame delta measured for the gate alone). ~1.3 ms is WORKER time. On a dev box
with spare cores the worker saving barely moves frame time; on the low-core machines this
campaign is aimed at it is the more valuable half. **Do not quote the 1.3 as a frame-time win.**

⚠ **The CPU frame-time metric could not resolve this** — within-condition spread (A1 4.21 vs
A2 3.81) was as large as the between-condition delta. The profiler rows could. When an effect
is ~0.3 ms on a 4 ms frame, measure the ROW you changed, not the frame.

`gg_instinit_parallel` was A/B'd in the same run: −0.16 ms CPU alone, but one rep was faster
and one a hair slower. **NOT RESOLVED — default stays OFF.** Recording the null, not banking it.

### ⚠⚠ AN INSTRUMENT FAILURE, AND THE CORRECTION IT FORCED
The first safety test flew the camera to five waypoints and reported "chunks=625, pend=0,
POLYS identical — no missing chunks". **That test was worthless.** It computed offsets with
`$(echo "$HX + 120000" | bc)` and **`bc` does not exist in this shell**, so every `SET_CAMERA`
received a malformed argument and the camera never moved. The test could only ever have
passed. The same bug invalidated the flight half of the six-demo mini-sweep. The claim was
already in the 2.94f commit message before it was caught.

★ Sibling of the standing rule that an instrument bypassing the suspect path can only exonerate
it — here the instrument never reached the path at all. **Log the reply from the thing you
changed and assert on it.** The redo prints every `SET_CAMERA` reply AND a `GET_CAMERA`
read-back, which is how it was proven fixed.

### The redone test (awk arithmetic, positions read back and logged)
Aztec, five real waypoints out to ~7.6 km, gate ON then OFF. Camera Y auto-snapped to terrain
height (1989 → 7698 → 11328), which independently proves real ground existed out there.

| waypoint | chunks on/off | pend | POLYS on/off |
|---|---|---|---|
| home | 625 / 625 | 0 | 10,330,135 / 10,356,401 |
| +148k,+149k | 725 / 725 | 0 | 219,792 / 212,680 |
| −202k,+99k | 625 / 625 | 0 | 192,264 / 192,264 |
| +298k,−251k | 625 / 625 | 0 | 184,920 / 184,920 |
| home again | 625 / 625 | 0 | 10,356,401 / 10,356,401 |

Identical chunk counts and pend=0 in both states at every waypoint, converging on the same
POLYS at home. (725 > ringMax is the documented removal lag, not a leak.) **The gate is safe:
terrain generation behaves the same with it on and off, under real camera movement.**

### Regression mini-sweep (PARKED measurements only — see caveat)
| demo | FPS | POLYS | chunks | pend |
|---|---|---|---|---|
| Aztec Game Kit Teaser | 83.0 | 10,330,135 | 625 | 0 |
| A Grand Canyon Adventure | 139.3 | 2,279,506 | 625 | 0 |
| Switch Escape | 250.3 | 109,358 | 625 | 0 |
| Trapped | 285.0 | 12,768 | 625 | 0 |
| Jungle Fever | 179.7 | 76,157 | 625 | 0 |
| RPG Template | 138.3 | 3,247,629 | 625 | 0 |

All six load clean with 2.94d + 2.94f live; 12 screenshots all render real scenes. ⚠ Only the
PARKED half is valid (the flight half used the broken `bc` path). The full 19-demo gate still
owes a run before the alpha.

### Held deliberately
The `gameisexe` gate on the terrain bridge was NOT done. The work a subagent called
editor-only (the chunkSig census, the blendmap scans) still has to run in a shipped game
because the chunk ring follows the camera, and an exported build cannot be verified from the
harness. An unverifiable change to shipping behaviour is not a good trade — it wants Lee's
call and a real exported-build test.

## §2.95 — ★★★ THE MISSING MOUNTAIN TREES: one flag, and a judgement call to revisit (2026-08-23)

Lee supplied a DX11 and a DX12 shot of `spotshadowtest.fpm` and asked what is missing in DX12.
★ I initially had the two shots the wrong way round and Lee corrected me. The corrected reading:

| | DX11 | DX12 |
|---|---|---|
| FPS | 300.3 | 257.8 |
| Mem | 7.27 | 6.61 |
| VRam | **4.33** | **2.71** |
| distant mountains | densely covered in TREES | BARE rock/grass |
| sky | warm gradient, tan horizon | flat neutral grey |

**What is missing is the distant trees.** In DX11 the mountainsides are forested to the horizon.
In DX12 they are bare. That single difference is most of the visual gap.

### Root cause — and it is documented in our own source as a deliberate decision
Two comments in `Guru-WickedMAX/GGTerrain/GGTrees_part2.cpp` tell the whole story.

`:87` — how DX11 does it: *"DX11 renders LOD0 + billboards past lod_dist"*.

`:340-350` — why DX12 does not:
> *Stage 4 (deprecated 2026-07-13 evening): no ImpostorComponent. Wicked's impostor render path
> (wiRenderer.cpp:7298 RenderImpostors) draws via a separate DrawIndexedInstancedIndirect that
> does NOT respect ObjectComponent::IsNotVisibleInReflections. Result: water reflection pass drew
> impostor atlas quads as bright white splats... we drop the impostor path entirely.* **Far trees
> now just don't render; the pool covers what's visible.**

The reasoning recorded at the time was that far trees *"would have been pixelated dot-scale
billboards contributing minimal visual value for the ECS overhead"*.
★★ **Lee's screenshot is the counter-evidence to that judgement.** On a level with mountains at
distance they are not minimal value — they are the entire character of the landscape. The call
was reasonable given a water-reflection bug; it should now be revisited on the evidence.

### ★★★ The fix is likely ONE FLAG, because the geometry already ships
`GGTrees_BuildShadowProxyChunk` (`:648` onward) already builds, every frame, exactly what is
needed: per 16x16 tree chunk, ONE merged static mesh of two crossed vertical quads per tree,
textured with **the exact same billboard silhouette DDS DX11 uses**
(`Files/treebank/billboards/*_BB_SF_*_color.dds`), alpha-tested, max 256 objects. It is
renderable and it already draws into the far shadow cascades. It is hidden from the player by
one line:

    GGTrees_part2.cpp:887   obj.SetNotVisibleInMainCamera( true );

So the distant forest is being built and shadow-cast every frame and then withheld from the
main camera. Clearing that flag is the experiment.

⚠ Three things to handle before calling it a fix:
1. **Double-draw near the camera.** The proxy chunk contains a billboard for EVERY tree in the
   chunk, including ones the pool already draws as real meshes. Needs a distance gate — the
   proxies are chunk-granular, so gate a chunk's main-camera visibility on it being beyond the
   pool's coverage.
2. **Keep `SetNotVisibleInReflections(true)`** (`:888`). The white-splat bug that killed the
   impostor path was a reflection-pass problem; do not reintroduce it.
3. Cost is not zero but should be small: up to 256 alpha-tested merged objects, already built,
   already culled, already occlusion-query-disabled.

### Corrections to what I said from the swapped shots
- **VRAM: DX12 uses 1.6 GB LESS, not more** (2.71 vs 4.33). My "DX12 exceeds the 4 GB min spec
  on this level" was exactly backwards — it is DX11 that sits at 4.33 GB. Part of DX12's saving
  is simply the forest it is not drawing, so expect some of it back when the trees return.
- **DX12 is ~14% SLOWER while drawing substantially less** (257.8 vs 300.3 FPS). That is a
  wider gap than the raw FPS suggests and it is the honest headline for this level.
- The warm gradient sky is **DX11's**; DX12's is the flat grey one.

### Not yet investigated (the shots are NOT camera- or time-matched, so these are unjudged)
Water band brightness, foreground grass density, and overall contrast/colour cast. Several of
those may simply follow from the sky/time-of-day difference. Worth a camera-matched pair before
spending effort on them.

## §2.95b — far-tree billboards: the "one flag" fix is WRONG, and the instrument says why (08-23)

Attempted the 2.95 fix (show the merged billboard proxy chunks to the main camera beyond the
nearest-N pool radius). **It does not work, and the reason is more interesting than the fix.**

### What the diagnostic found (harness SET_FARTREES now reports it)
On spotshadowtest, as shipped:

    proxyChunks=256  validProxies=0  proxiesShown=0
    candidates=7775  poolBuilt=6000  poolSize=6000  cutoffDist=24812

★ **validProxies=0 — all 256 proxy slots are INVALID_ENTITY. The billboard geometry is not
built on this level at all.** There was never anything for a visibility flag to un-hide.

Why: `GGTrees_BuildShadowProxyChunk` is SHADOW infrastructure. It only builds when
`TreeShadowsEnabled()` is true (`draw_shadows != 0 && TreeShadowRangeClamped() > 0`,
GGTrees_part2.cpp:534). Sending `SET_TREES shadowrange 4` flips it:

    proxyChunks=256  validProxies=244  proxiesShown=0
    validProxyChunkDist: nearest=0 farthest=125516

★★ **So the distant-tree billboards exist ONLY when the user has tree shadows switched on.**
Tying the VISUAL restoration of distant forest to the tree-SHADOW setting is wrong on its face:
a player who turns tree shadows off would also lose every distant tree. Any real fix has to
build the merged billboard chunks for their own sake, independent of the shadow toggle.

### ⚠ The gate itself is also still broken, and I could not explain it
With 244 valid proxies and chunks out to 125,516 units against a cutoff of 24,812, the
show condition
`(dx*dx + dz*dz) >= g_treePoolCutoffDist2` should be true for many chunks. `proxiesShown`
stayed 0 in every reading. Three attempts, no explanation — so by the standing rule
(two failed attempts means the approach is wrong, not the parameters) I stopped rather than
keep tuning. **`gg_trees_far_billboards` therefore ships DEFAULT OFF** and changes nobody's
picture; `SET_FARTREES 1` enables it for whoever picks this up.

Prime suspects for the next person, in order: (a) the gate reads
`g_treePoolCutoffDist2` on a frame where it is still -1 because it sits ABOVE the selection
throttle's early return and the ordering is subtler than it looks; (b) something downstream
re-asserts `SetNotVisibleInMainCamera(true)` after the gate runs — the proxy rebuild at
:1055-1080 and the park-release path at :945 both touch these objects.

### Second-order problem to solve at the same time
Even once it fires, the gate is CHUNK-granular: chunks are treeArea/treeSplit = 12,500 units
(~317 m) and the measured pool radius is 24,812 (~630 m), so only whole chunks past 630 m
qualify and the straddling ring shows nothing. The proxy holds a billboard for EVERY tree in
its chunk, including ones the pool already draws as real meshes, so relaxing the rule
double-draws them. DX11 gets this right because its billboard swap is PER TREE, not per chunk.

### What is genuinely established
1. Distant trees ARE missing in DX12 and the cause is documented (2.95: the impostor path was
   dropped 2026-07-13 over a water-reflection bug).
2. The billboard assets and the merged-chunk builder ship and work — but only under the tree
   shadow toggle.
3. Restoring the look needs: build proxies unconditionally, AND a finer-than-chunk gate, AND
   the reflection exclusion kept. That is a real piece of work, not a flag.

## §2.96 — far-tree billboards, steps 1-2: route PROVEN, three dead-code layers found (08-23)

Lee approved steps 1-2 of DESIGN_FAR_TREES.md: prove the customDraw route, then get all chunks
drawing colour-only. **Step 1 is done. Step 2 is one layer short**, and the reason is a chain of
three independently-reasonable "this is dead, skip it" decisions that compounded.

### ★ Layer 1 — the PSO failure was a C++ LIFETIME BUG, not a graphics one (FIXED)
`GGTrees_Draw` opened with a hard `return` and the comment *"DISABLED: tree Draw causes GPU hang
due to PSO compilation failure in DX12"*.

`GGTrees_Init` built all 11 tree PSOs from **stack locals** — `rastState` (:1242),
`depthStateOpaque` (:1254), `blendStateOpaque` (:1261), `inputLayout` (:1274),
`inputLayoutHigh` (:1281). `PipelineStateDesc` stores POINTERS to those, and Wicked's DX12
backend does not compile at `CreatePipelineState` time — it defers to `pso_validate()` at BIND
time (wiGraphicsDevice_DX12.cpp:2578). By then the stack frame is long gone, so every PSO was
compiled from destroyed memory. The engine even ships a logger for this exact failure class
(`gg_pso_fail.txt`, added for the gpup particle restore).

⚠ A SECOND bug sits in the same block: those locals are **mutated between** the 11 create calls
(cull_mode, depth_write_mask, alpha_to_coverage flip back and forth). Fixing only the lifetime
would give all 11 PSOs whatever the LAST write happened to be. Each needs its own copy taken at
create time. Fixed with `GGTreeCreatePSO`, which snapshots rs/dss/bs/il into per-PSO file-scope
storage. **Result: pass enabled, no crash, no GPU hang, `gg_pso_fail.txt` never created.**

### ★★ THE ROUTE IS PROVEN
With the pass on, spotshadowtest:

    BILLBOARD PASS: entered=4457 draws=87 instances=98410
                    chunksTotal=256 withInstances=256 frustumKilled=169

**87 draw calls submitting 98,410 tree billboards per frame**, 87 + 169 = 256 chunks. Exactly
DX11's shape (1-256 draws, typically 40-90 for a normal FOV). Zero scene entities. The static
instance buffers the DX12 build has been maintaining all along are correct and bindable.

### Layer 2 — the constant buffer was skipped as dead work (FIXED)
`GGTrees_Update` returns early at :2432 under `if (ggterrain_use_wicked_terrain)` with the
comment *"tree constant buffers for the OLD DX11 draw path... skip the dead work"*. The CB fill
sits BELOW it, so `tree_type[i].scaleX/scaleY` were never written — **every billboard quad had
zero area.** That is why the first run submitted 98,410 instances for zero pixels and zero
measurable cost. Factored into `GGTrees_UpdateBillboardCB`, called on the Wicked path when the
pass is on.

### ⚠ Layer 3 — THE ATLAS IS EMPTY AND ITS LOADER IS A DISABLED STUB (open)
GGTrees_part0.cpp:819, GGMAX Tier A (2026-08-02), a VRAM-campaign delta:
> *the four legacy tree atlases (texTree 52.2 MB, texBranchesHigh 52.2, texTreeNormal 26.1,
> texTreeHigh 26.1 = 156.6 MB) are allocated on demand rather than at init... created EMPTY, and
> their only writer `GGTrees_LoadTextureDDSIntoSlice` is a disabled stub, [because] their only
> readers are GGTrees' own draw functions, and every customDraw_* callback early-returns while
> ggterrain_use_wicked_terrain is set*

So the billboards now draw, correctly scaled, sampling a **blank fully-transparent atlas**.
Measured horizon-band screenshot delta: 0.002 mean. Nothing.

**Next step: re-enable `GGTrees_LoadTextureDDSIntoSlice` for the two BILLBOARD atlases.**
⚠ That costs **78.3 MB VRAM** (texTree 52.2 + texTreeNormal 26.1) — a real 4 GB-floor decision,
not a free flip. The high-detail pair (78.3 MB more) should stay disabled, since the Wicked pool
draws near trees with its own per-tree materials.

### The moral
Each of the three decisions was correct at the time, and each cited the others as justification:
the draw was dead because the PSO failed, the CB was dead because the draw was dead, the atlas
was dead because the draw was dead. **Nobody was wrong; the reasoning went round in a circle and
the feature fell out of the bottom.** When a comment says "this is dead, skip it", check whether
the thing that killed it is itself a bug.

### State
`gg_far_tree_pass` DEFAULT OFF, harness `SET_FARTREES 0|1`, which also reports the draw/instance
counters. Nothing changes for anyone until the atlas decision is made.

★ Also explains the 2.95b mystery: `SET_FARTREES` reads its counters in the same harness tick it
sets the flag, one frame before the pass can run — hence `entered=0` on the first read and real
numbers on the second. The old `proxiesShown=0` was the same instrument artefact, not a bug.

## §2.96b — four layers down, pixels still absent; the binding model is the remaining suspect

Continuing 2.96 after Lee approved the 78.3 MB. Three more layers cleared, one to go.

### Layer 3 CLEARED — the atlas loader now works
`GGTrees_LoadTextureDDSIntoSlice` was `// stub - DDS loading disabled` because **tinyddsloader
was removed in the DX12 port** (the same TODO sits over the grass equivalent). Rather than
re-add a third-party header, it now delegates to `GGTerrain_LoadTextureDDSIntoSlice` — a working
DX12 implementation of exactly this (parses the header itself, builds a staging texture,
`CopyTexture`s each mip into the array slice).

Two traps on the way:
- The terrain loader dereferences `requirements->` unconditionally under `CUSTOMTEXTURES`
  (which IS defined), so nullptr would crash. It validates width/height, and the billboard
  atlases are 1024x1024 against terrain's 2048 default — the requirements are now sized from
  the destination texture.
- ★ **The 2.53 linkage rule bit again**: `extern bool GGTerrain_LoadTextureDDSIntoSlice(...)`
  written inside `namespace GGTrees` mangles to `GGTrees::GGTerrain_...` and fails to link.
  Fixed with a GLOBAL-scope shim `GG_LoadDDSIntoTextureSlice` at the foot of GGTerrain_part0.cpp.

Also: `GGTrees_EnsureLegacyTexArrays` is only reached from GGTerrain_Update's legacy tail, which
early-returns on the shipping path — so the atlases were never even created. Added
`GGTrees_EnsureBillboardAtlases`, a one-shot lazy init that creates and fills ONLY texTree +
texTreeNormal (78.3 MB). The HIGH pair stays absent by design.

**Measured: `atlasSlices=38`.** All 38 tree types upload their billboard DDS.

### Layer 4 CLEARED — the prepass is enabled and hooked
`GGTrees_Draw_Prepass` carried the same "GPU hang / PSO compilation failure" disable. It is
**structurally required, not optional**: the main billboard pass runs `depth_write_mask = ZERO`
precisely because DX11 has the prepass lay the depth down first. Both now run, billboards only.

### ⚠ STILL NO PIXELS — and this is where I stopped
Current state, every measurement clean:

    BILLBOARD PASS: entered=4396 draws=87 instances=98410 atlasSlices=38
                    chunksTotal=256 withInstances=256 frustumKilled=169
    horizon-band screenshot delta: 0.002 mean   (control 0.97)

So: PSOs compile (`gg_pso_fail.txt` is never created), 87 draws issue per frame, 98,410
instances are submitted, the constant buffer is filled, 38 atlas slices are resident, and the
prepass runs. Nothing appears.

★ **Four build-test cycles without pixels is past the two-attempt rule, so I stopped guessing.**

### The remaining suspect, and the diagnostic that would settle it
**The DX11 shaders assume a binding model current Wicked does not provide.** `GGTrees_Draw`
binds the tree CB at slot 3, resources at 50/51/53 and samplers at 0/1/2, then calls
`GGCustomFrame_Bind(cmd)` for the camera. Wicked's DX12 root signature is descriptor-heap based
and those register assignments date from the DX11-era engine. If the view-projection the VS
reads is garbage, every quad projects off-screen — which fits the evidence exactly: correct
draw counts, correct instance counts, zero pixels, and no PSO error.

The pso_validate path never logging a failure is itself informative: **the pipeline is fine, so
this is a DATA or BINDING problem, not a pipeline one.**

NEXT (a bisect, not a guess): make the billboard PS output a solid opaque colour and the VS
bypass the tree transform, emitting a fixed clip-space quad. If a colour appears, the atlas or
alpha path is at fault. If nothing appears, it is the transform/binding — and then the register
mapping in GGTreesVS.hlsl versus Wicked's current root signature is the thing to read.

### State
Everything is behind `gg_far_tree_pass`, DEFAULT OFF. `SET_FARTREES 0|1` reports draws,
instances, atlas slices, chunk counts and frustum kills. Nothing changes for anyone.
The PSO lifetime fix is a genuine repair and is worth keeping regardless of how the rest lands —
it silently corrupted all 11 tree PSOs.

## §2.96c — ★★★ BILLBOARD TREES RENDERING. The bisect, and the last two bugs (2026-08-23)

**Done.** The distant forest is back on the DX12 path, through the DX11 design: zero scene
entities, 87 draw calls, 98,410 billboards per frame.

### The bisect settled it in one run
Forced the billboard PS to solid magenta, keeping the real transform (the first attempt bypassed
the transform too and emitted a fixed 1.2x1.2 clip-space quad — 98,410 near-fullscreen quads per
frame, which took the app down; a lesson in sizing a debug primitive).

Result: **142,544 magenta pixels, 11.6% of frame, zero in both OFF shots.** That single reading
cleared geometry, transform, camera constants, depth, blend AND the root signature in one go, and
localised the fault to the texture/alpha path. Worth the build.

### Bug A — the atlas upload used one command list PER SLICE
`GGTrees_LoadTextureDDSIntoSlice` opened a fresh `BeginCommandList()` for each of 76 slices, from
inside a draw callback. Now one list for the whole upload, opened once by
`GGTrees_EnsureBillboardAtlases`.

### ★ Bug B — a DOUBLED PATH PREFIX, and the diagnostic that named it
`atlasSlices=38` looked like success. It was not: the counter counted CALLS, and the loader's
bool return was being discarded. Counting SUCCESSES gave `atlasOK=0`, and capturing the reason
gave it outright:

    fopen FAILED: ...\Max\Files\files\treebank\billboards\birch autumn3_BB_SF_0.8_normal.dds

**`Files\files\`** — `GG_GetRealPath` already prepends `Files/`, and I had passed
`"files/treebank/billboards/"`. Every one of the 76 uploads failed silently into a blank atlas,
so every billboard pixel discarded on alpha. Same for `treebank/noise.dds`.
★ **A count of CALLS is not a count of SUCCESSES, and a bool return you discard is a diagnostic
you threw away.** `atlasSlices=38` actively misled me for two build cycles.

After the fix: `atlasOK=38  firstFail=(none)`.

### MEASURED, spotshadowtest
| | OFF | ON |
|---|---|---|
| FPS | 216.3 | 201.1 |
| VRAM | 2.71 GB | 2.88 GB |
| horizon-band screenshot delta | 0.002 | **10.761 mean, 25.8% of pixels** |
| sky (upper third) delta | — | 0.025 (unchanged, correct) |

~7% frame cost to restore the entire distant forest, and +0.17 GB against the 78.3 MB of atlas
predicted. Zero scene entities added.

### Full chain of five layers, for the record
Every one was individually reasonable and each cited the others:
1. `GGTrees_Draw` hard-disabled over a "GPU hang / PSO compilation failure" — actually 11 PSOs
   built from **stack locals** that died when `GGTrees_Init` returned (2.96).
2. The tree constant buffer skipped as "dead work", leaving per-type scales at zero, so every
   quad had **zero area** (2.96).
3. The billboard atlas created empty with its DDS loader a **disabled stub** (tinyddsloader was
   removed in the port) (2.96b).
4. The atlas upload using a command list per slice (2.96c).
5. A doubled `Files/files/` path prefix (2.96c).

### State
`gg_far_tree_pass` now **DEFAULT ON**; `SET_FARTREES 0` disables. The debug define
`GGTREES_DEBUG_SOLID` is 0 in both shaders and should be deleted once this settles.

⚠ **NOT YET GATED.** This changes the picture and adds ~78 MB on every terrain level. Owed before
it can be called shipped: the 19-demo sweep, a VRAM re-check against the 4 GB floor (worst
in-game was already 3947.9 MB on Aztec Game Kit — that one could now exceed it), and Lee's eye on
the swap seam, which is currently a hard cut with no dither because the mesh side has no
far-discard. The pool radius cap from DESIGN_FAR_TREES §3 is the next step.

## §2.97 — pool radius cap: −40% triangles, +7% FPS, same picture (2026-08-23)

With billboards live, the real-mesh pool and the billboards were both drawing across a 550 m
band: the billboard PS discards inside `tree_lodDist` = 3000 units (~76 m, GGTreesPS.hlsl:81 —
the noise-dithered discard survived the port intact) while the pool reached a measured 24,812
units (~630 m). Capping the pool is what makes them complementary instead of overlapping.

Cap = `lod_dist + 2 * GGTREES_LOD_TRANSITION` = 4000 units (~102 m), matching DX11, where the
mesh dissolves out over lod_dist+500..lod_dist+1000. Applied in two places in the gather: a
per-candidate distance reject, and a `maxRing` bound derived from the cap so a sparse level
cannot make the ring-gather walk the whole 128x128 grid looking for trees it will never accept.
`gg_tree_pool_max_dist`: <0 derive (default), 0 uncapped, >0 explicit. Harness SET_TREEPOOLCAP.

### MEASURED, spotshadowtest, interleaved x2
| cap | FPS | POLYS |
|---|---|---|
| 0 (uncapped, pre-2.97) | 203.6 / 203.2 | 506,724 |
| **−1 (derived, 4000u)** | **218.1 / 218.5** | **304,731** |

**−201,993 triangles (−40%) and +7% FPS, for a visually identical frame** — the screenshots show
the same forested mountains with no gap or seam at the swap. Those triangles were near-field
mesh detail the billboards were already covering.

### ⚠ A crash on the way, and the rule it re-taught
Two AVs in `DescriptorBinder::flush` from `GGTrees_Draw_Prepass` before this landed. Cause:
`GGTrees_EnsureBillboardAtlases` was creating textures and issuing CopyTexture **inside a draw
callback on a job thread**, then binding those same textures in the same command list.
★ **Resource creation belongs on the main thread, never inside a render callback.** Split into
an INIT (main thread, from GGTrees_Update via the CB update) and a GATE
(`GGTrees_BillboardAtlasesReady()`, checked by both draw functions). The ready flag is now set
at the END of the upload, so a draw can never observe half-built atlases.

## §2.98 — drop the billboard normal map, and stop drawing trees past the terrain (2026-08-23)

Both from Lee, off a screenshot showing trees marching to the horizon with no ground under them.

### 1. texTreeNormal removed — 26.1 MB back
The billboard PS sampled a 1024x1024x38 BC1 normal array. Directly above that sample, still in
the file and commented out, was the analytic billboard normal that shipped before the normal map
existed. Restored it and deleted the texture: no creation, no 38 DDS slice loads, no bind, no
declaration. Cost is that distant foliage lights as a smooth card rather than with per-texel
relief — at billboard range, a fair trade for 26 MB.

### 2. Billboards no longer drawn beyond the terrain chunk ring
The tree grid spans the whole 200,000-unit (5 km) tree area, but terrain chunks only exist
within `generation * chunkU` of the camera (12 x 5280 = 63,360 units, ~1609 m) because the ring
is camera-centred. Past that, billboards were standing on nothing — exactly what Lee's shot
showed. New global-scope `GG_GetTerrainViewRadius()` reports the live reach; both billboard
loops reject chunks whose nearest point is beyond it. ★ The prepass carries the IDENTICAL test —
mismatch there would lay depth for trees the colour pass then skips.

### MEASURED, spotshadowtest
| | before 2.98 | after |
|---|---|---|
| draw calls | 87 | **41** |
| billboard instances | 98,410 | **42,370** |
| chunks culled: no terrain | — | **46** |
| VRAM | 2.88 GB | **2.78 GB** |

41 drawn + 169 frustum-killed + 46 no-terrain = 256 ✓. Instances down 57%, and every one of
those removed was a tree floating over nothing. Forest on the real terrain is unchanged.

⚠ The 4 GB question this was for: Aztec Game Kit in-game was 4026.6 MB against a 4096 gate.
26 MB comes off the atlas directly; the no-terrain cull does not change VRAM, only draws.
Needs a re-measure on that demo specifically before the headroom can be called safe.

## §2.98b — normal map RESTORED; the VRAM has to come from somewhere else (2026-08-23)

2.98 swapped the billboard normal map for the analytic normal to reclaim 26.1 MB. Lee's
side-by-side against DX11 settled it in one look: **the canopy went flat and uniformly bright
green** where DX11 has real depth and shading variation across the foliage. Reverted — sample,
declaration, creation, slice loads and all three binds are back. VRAM 2.78 → 2.83 GB on
spotshadowtest; Aztec in-game goes back to ~4026 MB, 69 MB under the gate.

The terrain-ring cull from 2.98 STAYS (draws 41, instances 42,370, 46 chunks culled) — that part
was pure win and unrelated.

★ Worth recording as a judgement, not just a revert: **26 MB was not worth that.** The analytic
normal is a single direction for the whole card, so every leaf on a billboard lights identically
— which is exactly the "flat cutout" failure mode the design research warned about, and it shows
at the ranges billboards actually occupy.

### Better VRAM routes, in order of value
1. ★★ **Load only the tree types the level USES.** The machinery already exists — the
   ONLYLOADWHENUSED path keys off `ggtrees_global_params.paint_tree_bitfield` and
   `bTreeTextureUploaded[]`. A level using 6 of 38 types needs 6 slices, not 38. That is up to
   **~66 MB across both atlases at ZERO quality cost**, far more than the normal map was worth.
   Note the atlas is still allocated at full array size, so this needs the array sized to the
   used count (or the slices simply left unwritten, which saves upload but not allocation) —
   check which before promising the number.
2. Halve both atlases to 512x512: 52.2 → 13.0 and 26.1 → 6.5, ~59 MB, keeps per-texel relief.
   A billboard at 600 m is tens of pixels tall, so 512 is still heavily oversampled.
3. Drop the HIGH-detail pair properly (already absent, but confirm nothing re-creates them).

## §2.99 — ★★★ PER-LEVEL ATLAS SLICES: the VRAM back, at zero quality cost (2026-08-23)

The right way to pay for the billboards. The atlases were allocated with one slice per tree type
in the BUILD (38) and all 38 uploaded, regardless of how many the level places. Levels use a
handful.

Now: scan the placed instances for distinct `GetType()`, allocate exactly that many slices, and
publish each type's slice index to the shader. The index lives in a **spare padding float** in
the shared `TreeType` CB struct (`GGTreesConstants.hlsli`, one definition for C++ and HLSL), so
there is no constant-buffer layout change — the three `texTree`/`texTreeNormal` samples just
index `tree_type[treeType].slice` instead of `treeType`.

**spotshadowtest uses 13 of 38 types.** Atlases 38 → 13 slices: texTree 52.2 → 17.9 MB,
texTreeNormal 26.1 → 8.9 MB. **51.5 MB saved, and not one texel of quality lost** — it is the
same images, just none of the unused ones.

### ★★★ The number that was the whole point
| Aztec Game Kit, IN GAME | MB |
|---|---|
| 0822, before any far trees | 3947.9 |
| 0823, full 38-slice atlas | 4026.6 |
| 2.98, normal map dropped (reverted) | 4000.1 |
| **2.99, per-level slices** | **3941.2** |

**Below the pre-billboard baseline, with the normal map restored and the whole distant forest
rendering.** Headroom against the 4096 gate: 69 MB → **155 MB**. Editor 3746.5 → 3629.1.

★ The lesson worth keeping: the first instinct (drop the normal map) traded 26 MB for a visibly
worse picture. The right lever was never a quality knob at all — it was that we were uploading
25 textures the level never asked for. **Before trading quality for memory, check what is being
loaded that nothing uses.**

⚠ Verify before the next gate: the remap is confirmed correct on spotshadowtest by eye (wrong
slices would show as wrong species), but a level whose types change at runtime (Lua spawning a
tree type absent at load) would index a slice that was never uploaded. The atlas init is
one-shot; if runtime type changes are possible this needs a dirty flag.

## §3.00/3.01 — two billboard fixes, and the baked-terrain question ANSWERED (2026-08-24)

### 3.00a — "Trees Off" did not switch the billboards off, and Water Off did
Lee: *"strangely when Water Off is selected later on, the billboard trees THEN disappear"*.
Exactly right, and the cause is a one-way dependency: `gg_no_trees` only reaches
`ggtrees_global_params.draw_enabled` inside `Wicked_Update_Visuals`
(M-GridEditB_part3.cpp:2095). The Trees Off tick box did not re-apply visuals; the **Water Off**
tick box calls `GGApplyVisualsNow()`, so ticking water was what finally recomputed the tree
flag. Two fixes: the billboard passes now test `gg_no_trees` directly (immediate, no dependency
on a visuals re-apply), and Trees Off calls `GGApplyVisualsNow()` like the water switch does.
★ A flag that only takes effect when some UNRELATED control happens to re-derive it is a
clobbered-value bug wearing a different hat.

### 3.00b — trees past the terrain edge, properly this time
2.98's cull was chunk-granular: it kept any 12,500-unit chunk that merely OVERLAPPED the
terrain, so trees near a straddling chunk's far edge still stood on nothing. Now exact and
per-tree, in the vertex shader: `tree_terrainReach` rides in a spare CB float and the VS folds
`reach - distXZ` into `SV_ClipDistance0` with `min()`, so the water clip plane still applies.
Zero CPU, per-tree accuracy the CPU cull cannot reach.

### ★★★ 3.01 — IS A BAKED TERRAIN MODE WORTH IT? Yes, about two thirds of it.
Lee's configuration (trees/grass/water off, terrain on), TESTPRO2, interleaved x2.
**Identical POLYS in A and B — same terrain, same extent, the only difference is the bake.**

| | GPU frame | GPU busy | CPU | FPS | POLYS |
|---|---|---|---|---|---|
| A stock VT terrain | 4.646 / 4.675 | 1.90 | 2.72 | 214.4 / 213.7 | 512,996 |
| **B bake-equivalent** | **3.502 / 3.492** | 1.82 | 2.65 | **285.2 / 286.2** | 512,996 |
| C terrain OFF entirely | 2.921 | 1.59 | 2.16 | 341.1 | 375,684 |

Terrain's total remaining cost is **1.74 ms** (4.66 - 2.92). The bake recovers **1.16 ms of
that, 67%**, for **+72 FPS**. The other third is rasterising the ground, which any floor must pay.

Scaled to Lee's own profiler-off numbers (280 FPS terrain-on, 570 off): a bake should land
around **425 FPS**, i.e. roughly half the distance to "no terrain at all".

⚠ **A first pass at this measured only 26%** because it also cut the ring from 625 to 49 chunks —
that draws 70K fewer triangles, so it was "less terrain", not "the same terrain, baked". Holding
POLYS identical is what makes the 67% trustworthy. Same trap as every content A/B on this project.

Three things that shape what the mode should be:
1. **GPU busy barely moves** (1.90 -> 1.82). The win is the idle/unranged bucket again - the VT
   machinery - the same class as 2.94d, not shading.
2. **CPU barely moves** (2.72 -> 2.65). A bake that also MERGED chunks would cut CPU too, but
   that is the half 2.94e warned kills per-object LOD, frustum culling and occlusion.
3. ★ **The performance is already available today** - `SET_TERRAINBAKE 1` is one integer. What a
   real baker adds is the QUALITY: nearRing=0 drops every chunk to 256 px, so the mode is really
   "bake a decent ground texture so the cheap path looks acceptable", not "make it fast".

Harness `SET_FASTTERRAIN <ring>` combines both levers (VT off + smaller ring) for further A/Bs.

## §3.02 — the bake cliff is TILE COUNT, not resolution (2026-08-24)

Lee asked to see the bake at 1024 per chunk. Added `SET_TERRAINBAKE <mode> <res>` and measured
on TESTPRO2 in his configuration (trees/grass/water off, terrain on), **profiler OFF** so the
FPS is comparable to the number on his screen, same camera throughout:

| | FPS | vs stock |
|---|---|---|
| stock terrain (nearRing 4) | 214.4 / 213.7 | — |
| **bake @256** | **289.6** | **+35%** |
| bake @512 | 252.1 | +18% |
| **bake @1024** | **255.3** | **+19%** |

★★★ **512 and 1024 perform the SAME.** That is the tell, and the code predicts it exactly:
`VirtualTexture::init` does `if (tile_count > 1) residency = atlas.allocate_residency(...)`
(wiTerrain.cpp:499). `SVT_TILE_SIZE` is 256, so 256 is a SINGLE tile and needs no residency
object — which is the entire reason the bake path skips all four VT GPU passes. At 512 (4 tiles)
residency returns; at 1024 (16 tiles) it is already back, so the extra resolution is nearly free.
Resource count corroborates: 5,983 at 256 → 11,617 at 1024.

So it is a BINARY choice through this path, not a slider:
* 256 — one tile, no residency, **+35%**
* >=512 — residency back, **+19%**, and since 1024 costs no more than 512, take 1024.

### ★ What this proves about a REAL bake
The +19% ceiling is an artefact of the experiment reusing the VT machinery — **not** a property
of baking. A genuine baked terrain binds an ORDINARY SRV per chunk, outside the virtual-texture
system entirely, so it should keep the 256-path's **+35% at any resolution**. That is the next
experiment and it is the one that settles whether the mode is worth building.

⚠ My stock reads 214 FPS where Lee's screen reads 280 on nominally the same config — the camera
is not matched (mine is the level default). **Trust the ratios, not the absolutes.** Applied to
his 280 baseline: bake@1024 ~330 FPS, bake@256 ~378 FPS.
⚠ The `residencyVTs` field in the SET_TERRAINBAKE reply lags a frame or two (it is read in the
same harness tick that sets the flag), so the resource count is the more reliable witness here.

## NEXT ACTION (2026-08-24) — plain-SRV baked terrain
Lee has approved coding it. The goal: keep the +35% AT 1024 by taking the terrain material off
the VT path entirely.

Shape of the work, from what 2.94e-3.02 established:
1. VT on a chunk is ONE INTEGER: `sparse_residencymap_descriptor` in the material's texture slot
   (ShaderInterop_Renderer.h:220; set at wiTerrain.cpp:2102/2111). -1 = plain `tex.Sample`.
2. So: create a normal Texture2D per chunk at the chosen resolution, write the blended terrain
   surface into it ONCE, bind it as the material's basecolor SRV with the residency descriptor
   at -1, and never touch it again.
3. Source for the bake: the same CPU blend that feeds the VT today
   (`ApplyDX11StyleAutoBlend` / the blendmap pipeline in GGTerrainWicked.cpp) — the honest
   source, versus reading back the SVT atlas.
4. Budget check first: 625 chunks x 1024^2 BC-compressed is roughly 0.5-1 MB each = 300-600 MB.
   That is NOT affordable against the 4 GB floor at the full ring. Either bake only inside a
   radius, or bake at 512, or bake to a shared atlas rather than per-chunk textures.
   ★ DO THIS ARITHMETIC BEFORE WRITING THE BAKER.

---

## 3.03 — the billboard/mesh handover POPPED. Lee spotted it by eye. (2026-08-24)

> *"I was just running the latest MAX and noticed a flicker as the tree transitions from a real
> tree to its billboard. On closer inspection it looks like the billboard renders at the same time
> as the real tree, and then the real tree disappears when you move a little further back. Should
> both real and billboard be rendering at the same time?"*

**Answer: yes, both SHOULD draw at once — for ONE 500-unit band. Ours did it for TWO, and then
hard-popped.** He described the defect precisely without seeing a line of the code.

### What the bands actually were

DX11 carries the swap in two matched per-pixel discards:

    billboard (GGTreesPS.hlsl:81)        limit = noise*500 + lod_dist
                                         discard where sqrDist <  limit^2
    mesh      (DX11 GGTreesHighPS:215)   limit = noise*500 + 500 + lod_dist
                                         discard where sqrDist >  limit^2

so with lod_dist = 3000:

| distance | DX11 | DX12 before 3.03 |
|---|---|---|
| 0 .. 3000 | mesh only | mesh only |
| 3000 .. 3500 | billboard dissolves IN, mesh solid — **deliberate double-draw** | same ✓ |
| 3500 .. 4000 | mesh dissolves OUT, billboard solid — **cross-dissolve** | **both solid** ✗ |
| 4000 | billboard only | mesh **POPS** off (pool cap `SetRenderable(false)`) ✗ |

We had implemented the first half of the handover and not the second. 2.97 gave the pool a hard
radius cap of `lod_dist + 2*500` = 4000 precisely so the two would stop overlapping, and that was
right as far as it went — but a cap is a *binary* removal. DX11 never removes anything binarily.

★ The extra 500-unit band where BOTH are fully opaque is also where the shimmer comes from: the
billboard quad and the mesh occupy the same space, so their fragments z-fight as the camera moves.

### The fix — Wicked already had the feature, our objects were opting out of it

We cannot port DX11's mesh-side discard: our near trees are stock Wicked `ObjectComponent`s on the
stock object shader, and a custom PSO for them is the large precedent-free job `DESIGN_FAR_TREES.md`
ruled out (§3, "The one genuinely hard problem"). **We do not need to.** The engine has a
per-object dithered distance fade already wired end to end:

    ObjectComponent::draw_distance -> object.fadeDistance          wiScene.cpp:5281
    dither = max(0, batch.GetDistance() - fadeDistance) / radius   wiRenderer.cpp:4237
    dither > 0.99  -> instance skipped entirely                    wiRenderer.cpp:4238
    dither > 0     -> forceAlphatestForDithering = 1               wiRenderer.cpp:4243
    packed 4 bits into ShaderMeshInstancePointer                   ShaderInterop_Renderer.h:846
    GetDither() -> AlphaToCoverage                                 objectHF.hlsli:1241/1248
    hard cull past fadeDistance + radius                           wiRenderer.cpp:8760

`draw_distance` defaults to **FLT_MAX**, so every pool tree we have ever created has opted OUT of
it. Setting it is the entire fix. **Zero engine changes — no WICKED_ENGINE_CHANGES.md row.**

### ★ The subtlety that made it more than a one-liner

Wicked's fade band is `[d, d + object.radius]` and **the width is not tunable** — it is the object's
own AABB radius, which is the HALF-DIAGONAL (the standing rule), so a big trunk at scale 1.5 runs
~900 units. A fixed DX11-style start of 3500 would therefore still be ~44% dissolved when the pool
cap yanks the slot at 4000: a smaller pop, but the same bug.

So `TreeMeshFadeDistance( radius )` works BACKWARDS from the cap instead:

    d = cap - radius*1.1        // 10% margin, our radius estimate ignores transform rotation
    if ( d > lod_dist + 500 ) d = lod_dist + 500    // small tree: no later than DX11 starts
    if ( d < lod_dist )       d = lod_dist          // never fade before the billboard has begun

Small trees get DX11's exact 3500 start; big ones start earlier so the dissolve always LANDS on the
cap. Only a tree with radius > 1000 clamps and keeps a small residual pop.

Applied at `BindTreeSlot` (GGTrees_part2.cpp:726) using `mesh.aabb.getRadius() * scale`, plus a
placeholder at pool creation. ⚠ **per-BIND, so the knob is not instantly live** — churn the pool
before reading any A/B.

### Verification

★ **A static screenshot pair cannot show this defect.** A pop is TEMPORAL; the two endpoints are
the same picture. The A/B frames came out 104 bytes apart and proved nothing — which is itself the
expected result, not a failure. Sibling of "an instrument that bypasses the suspect path can only
exonerate it".

The signal that DOES work is POLYS: instances with dither > 0.99 are skipped outright, so the fade
must show up as fewer triangles. TESTPRO2, interleaved, camera returned home and read back each time
(400,000 trees, pool built 184 / bound 180 — the 2.97 cap doing its job):

| pass | fade | FPS | frame ms | POLYS |
|---|---|---|---|---|
| 1 | OFF (0) | 154.6 | 6.47 | 798,800 |
| 1 | ON (-1) | 152.0 | 6.58 | **781,927** |
| 2 | OFF (0) | 154.2 | 6.49 | 798,800 |
| 2 | ON (-1) | 155.1 | 6.45 | **781,927** |

POLYS bit-identical within each condition across both passes, −16,873 (−2.1%) with the fade on.
FPS unchanged inside the noise floor (154.4 vs 153.6 mean, spread wider than the delta) — this is a
correctness fix that happens to be free, not a performance change. Do not quote it as one.

### Honest limits

1. **Not visually confirmed.** The mechanism is proven live; that the POP IS GONE needs Lee's eye
   or a temporal capture. He found it in seconds by flying, so that is the cheap test.
2. **16 dither steps, not DX11's continuous per-pixel noise.** `ShaderMeshInstancePointer` packs the
   dither into 4 bits, so a tree steps through 16 levels across the band (~30 units per step)
   instead of dissolving smoothly. Far better than binary; not identical to DX11. If the stepping
   shows, the answer is a wider band, not more bits.
3. **The 3000..3500 double-draw remains** — by design, DX11 does the same. If z-fight shimmer is
   still visible there, that is the next thing to look at, and it is a separate defect.

Knob: `gg_tree_mesh_fade_dist` (GGTrees.h). `<0` derive [default], `0` = pre-3.03 hard pop,
`>0` explicit. Harness `SET_TREEMESHFADE <units>`, hoisted into the SET_TREEPOOLCAP helper chain
(C1061 rule).

---

## 3.04 — the BLACK trees. My bug, from 3.00, and the design doc predicted it. (2026-08-24)

> *"a flicker as the tree transitions... I have saved the test scene at the moment the black is
> showing on the tree directly in front in the mid distance, and when I move the camera a touch
> forward, the black disappears to show the tree texture... It feels like acne in that the
> transition is so flickery/fleeting it feels like zdepth fighting. If these trees are indeed
> billboards facing the camera, the effect would be as though two quads both facing me, and one
> was darker and both fighting each other to be the nearest to render."*

Lee's read of the mechanism was almost exactly right, and it is NOT the 3.03 handover pop.

### The defect

3.00 added the per-tree terrain-reach clip (`tree_terrainReach` folded into `SV_ClipDistance0`)
so a tree standing past the edge of the terrain would not draw over nothing. **I added it to
`GGTreesVS.hlsl` and not to `GGTreesPrepassVS.hlsl`.**

    prepass VS : no reach test  -> quad drawn, DEPTH + velocity written
    colour VS  : reach test     -> whole quad CLIPPED, no fragments at all

Depth now says "opaque surface here" so the terrain and sky behind fail the depth test too, and
nothing ever writes that gbuffer texel. Deferred lighting shades an empty gbuffer **BLACK**.

Everything in Lee's description falls out of that: it is tree-shaped and black; it sits at a
distance boundary; and **moving forward clears it** because forward reduces `distXZ`, pulling the
tree back inside reach so the colour pass draws it again. His "two quads, one darker, fighting to
be nearest" is precisely one quad present in the depth pass and absent from the colour pass.

★★★ `DESIGN_FAR_TREES.md` §6 risk #2, written before a line of this was coded:

> *"The near-discard must be duplicated identically into the prepass. DX11 carries it in five
> shaders. Miss one and depth is written for fragments the colour pass discards."*

I wrote the risk down, then committed it three deltas later with a DIFFERENT test than the one I
was watching for. **A documented risk is not a control. It needs a mechanical check.**

### What was ruled out first, and how

Four candidates died before the real one, each on evidence rather than argument:

1. **Prepass/colour PS near-discard mismatch** — read both; byte-for-byte the same maths, and
   both match DX11. Dead.
2. **Two billboard systems drawing at once** (the merged proxies from 2.95 + the real pass from
   2.96) — `gg_trees_far_billboards = false`, proxies are shadow-only. Dead.
3. **Unfilled atlas mips** — the atlas is created with 9 mips and `maxMip = min(DDS mips, 9)`, so
   a short DDS chain would leave high mips uninitialised = black at distance, and 2.99 had just
   rewritten slice upload. Scanned all 76 billboard DDS headers: every one is 1024x1024 with 11
   mips. All 9 destination mips get written. Dead. (Good theory, wrong. Checking took 2 minutes.)
4. **Shared-CB race between prepass and opaque** — the 2.00-era bug class. Both `UpdateBuffer`
   sites are inside `GGTrees_Update`, once per frame, not per pass. Dead.

The fix was then found by **diffing the two vertex shaders** rather than reasoning about them.

### The fix, and the diagnostic that proves it

Prepass VS now carries the identical block. Also deleted `GGTREES_DEBUG_SOLID` from both tree
shaders — it forced `OUT.clip = 1.0` in the COLOUR VS only, i.e. the mirror image of this same
bug, armed and waiting for whoever set it to 1.

★ Because this failure mode has now bitten twice, it is kept reproducible on demand:
`tree_prepassReach` (the spare CB padding float, no layout change), harness
**`SET_TREEPREPASSREACH 0|1`**. 1 = correct. 0 = defect reproduced, live on the next frame with
no rebind and no pool churn — so it A/Bs cleanly INSIDE one process at one camera.

Instrument: count near-black pixels in the sky/tree band of the viewport. Same camera, same
frame, only the prepass clip differs:

| camera Z | defect (0) | fixed (1) |
|---|---|---|
| -4806 | 3.647% | 1.851% |
| -5306 | 3.729% | 1.821% |
| -5806 | 3.750% | 1.649% |
| -6306 | 3.876% | 1.612% |
| -6806 | 3.933% | 1.496% |
| -7306 | 4.020% | 1.419% |

Total −57.1%. ★ The confirming detail is not the size of the gap but the **opposite trends**: with
the defect the black GROWS as the camera pulls back (more trees fall beyond the reach), while
fixed it FALLS (simply less foliage on screen). A generic rendering difference would not do that.

Cost: **nothing**. Interleaved x3 at one camera — FPS 167.7/169.2/171.3 (defect) vs
169.2/171.2/169.4 (fixed), POLYS **452,328 identical both ways**, because a clip distance kills
fragments and not geometry. ⚠ A screenshot corner read 87.7 vs 184.1 FPS and I nearly quoted it;
it was a transient hitch right after the CB flip. Measure, do not read HUD corners.

### Still open

- `GGTreesHighShadowMapVS.hlsl` and `GGTreesHighEnvProbeVS.hlsl` have **no clip output at all**,
  so trees past the terrain edge still cast shadows and still appear in env probes. Their shadows
  land where there is no terrain, so it is probably invisible — but it is the same inconsistency
  and it wants one deliberate look before it is called fine.
- The 3.03 handover fade is unaffected by this and still needs Lee's eye.

---

## 3.05 — flat grey billboard quads, for eyeballing the handover (2026-08-24)

Lee, after 3.04 fixed the black but the flicker survived:

> *"Before we fix the issue of the billboards not matching the real tree visual, or the flicker,
> I want to see these billboard quads in the flesh, without any textures applied. Can you make all
> the billboards a flat grey solid color, I want to perform the transition manually and see the
> real trees transition out and the grey quads transition in."*

`tree_debugSolid` (the spare CB padding word, no layout change). 1 = every billboard draws as a
flat unlit mid-grey quad: no texture fetch, no alpha cutout, no lighting, no fog.

★ **The LOD dither discard is deliberately KEPT.** The point is to watch the handover, so the
quads must still dissolve in over `lod_dist .. +500` exactly as they normally do. Real meshes are
untouched and still dissolve out via 3.03.

★★ Applied **identically in `GGTreesPS` and `GGTreesPrepassPS`**. Skipping the alpha cutout changes
the pass's coverage, so putting it in one shader and not the other would have recreated 3.04's
black fringes *inside the very build meant to diagnose the flicker*. This is the 3.04 lesson being
applied the same day rather than re-learned.

Note this deliberately re-introduces, as a supported runtime knob, what `GGTREES_DEBUG_SOLID` was
as a compile-time define before 3.04 deleted it. The define was the right idea and the wrong
mechanism: it was invisible at runtime, it only touched the colour shaders, and it sat at 0 in the
tree for two deltas as a live landmine.

Three ways in, all equivalent:
- `setup.ini` **`treedebugsolid=1`** (documented with a DOCDOC line next to the off-switches)
- harness **`SET_TREEDEBUGSOLID 0|1`**, live on the next frame, one CB write
- `GGTrees::gg_tree_debug_solid`, **default false** — the repo ships it OFF

⚠ The INI and LIVE paths were proven equivalent rather than assumed (the standing rule). Same
camera, ini-only launch vs harness-enabled: 108,800 vs 108,756 grey pixels — 0.04% apart, with
0.90% of the frame differing, all of it water animation.

⚠ `setup.ini` in the build folder currently carries `treedebugsolid=1` so Lee's build shows grey
on launch. That is a LOCAL, un-versioned edit; `setup.ini.bak_pre305` sits beside it. Set it to 0
before any visual or performance testing.

### Standing questions this build should answer

The flicker survived 3.04, so it is a third defect. With textures gone the geometry is naked, and
the answer should be visible directly:

1. Do quads **appear/disappear** frame to frame? -> LOD dither / pool churn.
2. Do quads **z-fight the real mesh** in the overlap band? -> the 3000..3500 double-draw.
3. Do quads **change size or orientation** as the camera creeps? -> the CB rotation or per-type scale.

---

## 3.06 — THE FLICKER. Two compilations of the same vertex maths. (2026-08-24)

Lee, after the grey-quad build, cutting through three of my wrong turns:

> *"it is not a tree trunk, not the water line, not other magical unicorns, its the damn quad
> draw... moving the camera EVER so slightly changes the artifact from no black to black somewhere
> else, all inside the quad and all related to the billboard rendering draw. Do not speculate
> outside this framework. MY next test I want YOU to perform is to ALWAYS draw the pixel for the
> quad, discard nothing and do not write into the depth buffer. My guess will be that the black
> will disappear."*

**His guess was exactly right, and his framing was the thing that cracked it.** I had wandered off
into tree trunks, waterlines and mesh fades after the debug build had already proved the answer
was inside the quad draw.

### The measurements

Debug mode 2 = no discard in either pass. Mode 4 = mode 2 plus the billboard prepass bound to a
twin PSO with `depth_write_mask ZERO`. Mode 5 = depth writes ON but the colour PSO `depth_func`
forced to ALWAYS. Same camera throughout:

| mode | prepass depth write | colour depth test | black px | grey px |
|---|---|---|---|---|
| 2 | ALL | GREATER_EQUAL | 1779 | 127,575 |
| 4 (Lee's test) | **ZERO** | GREATER_EQUAL | **0** | 118,539 |
| 5 | ALL | **ALWAYS** | **0** | 134,141 |

Both ways of breaking the comparison clear it, and mode 5 draws **6,566 MORE grey pixels** than
mode 2 - those are the fragments that were being thrown away. Conclusive: the billboard colour
pass fragments were being **depth-rejected against depth written by its own prepass**.

### Root cause

`psoTrees` (colour) is created with `depth_write_mask = ZERO` and `depth_func = GREATER_EQUAL`.
It writes no depth; it only tests what `psoTreesPrepass` laid down. **That makes the two passes'
vertex maths a correctness contract.**

The two shaders' position code is *textually identical*. It is still not the same code:

    float invV = rsqrt( diff.x*diff.x + diff.y*diff.y ); // approximation

★★★ `rsqrt` is an APPROXIMATE instruction, and `invV` drives the billboard camera-facing rotation.
A different instruction schedule in a separate compilation does not shift the result by one ULP of
depth - it rotates the quad slightly differently, moving it in WORLD space. Under reverse-Z
GREATER_EQUAL every fragment landing behind the prepass depth is rejected, nothing writes the
gbuffer, and the quad reads BLACK. `diff` is camera-relative, so which side of the comparison you
are on changes as the camera moves: hypersensitive, view-dependent, flickering. Lee called it
z-fighting between two quads on day one. It was, near enough - one quad rasterised twice from two
slightly different transforms.

### The fix

**The prepass PSO now binds the COLOUR pass compiled vertex shader** (`shaderTreesVS`), so the two
passes cannot disagree - bit-identical by construction, not by discipline.

Legal because the prepass PS input signature (position/worldPos/clip/uv/data) is an exact SUBSET
of the colour VS output - it just ignores the extra `TEXCOORD4 dir` - and both PSOs already use
the same `inputLayout`. `GGTreesPrepassVS.hlsl` is now unused by this PSO.

★★ It also permanently retires the 3.04 bug class between these two passes: a clip or transform
added to the VS can no longer reach one pass and not the other, because there is only one VS.

### Verification

Exact-black pixels - the artifact wrote literally (0,0,0), an unwritten gbuffer texel, which lit
foliage never is - swept across camera nudges of 15/30/60/120/240 units, the scale at which Lee
saw it flip:

| | before | after |
|---|---|---|
| debug grey quads | 1218 | **0** at all 6 positions |
| normal textured billboards | - | **0** at all 6 positions |

⚠ A naive "near-black" threshold reads ~6,000 px on textured billboards and is MEANINGLESS - that
is real dark foliage. Only exact (0,0,0) separates an unwritten texel from a dark one.

### Method note, for next time

Four candidates died on evidence (terrain reach, mesh fade, discard mismatch, water), but I also
burned two rounds narrating shapes in a zoomed screenshot - "a trunk", "the waterline" - after the
mode 1/2/3 table had ALREADY proved the pixel shader was irrelevant (black pixel-identical across
all three, zero red in mode 3). ★ When an instrument says a whole stage is not involved, believe
it and move to the next stage; do not go back to reading the picture. And ⚠ one row of that
bisect was VACUOUS: `SET_TREEMESHFADE` only applies at pool BIND and I read it without churning
the pool. Re-run properly it was flat - but I had already quoted it.

Debug levers kept: `SET_TREEDEBUGSOLID 0..5` (0 off, 1 grey+dissolve, 2 grey no-discard,
3 red dissolve zone, 4 no depth write, 5 depth test ALWAYS) and `SET_TREEPREPASSREACH`.

---

## 3.07 — billboard shade wrap, and a correction to my own advice (2026-08-24)

> *"I think the billboard light effect is too aggressive as the dark patches are too dark. I like
> the technique, and we should keep it, but just lessen the severity of when the billboard is in
> shade. Is that the same knob you just described about the softening?"*

**No - and the knob I had named would have made it WORSE.** In §3.06's write-up I said the dial was
the `2` in `lerp( dir, normal, 2 )`. Measured on the actual billboard texture under Lee's sun
(87/75, nearly overhead), lowering it deepens the shade:

| option | mean lit | p10 (dark tail) | % very dark |
|---|---|---|---|
| t = 2 (current) | 0.982 | **0.136** | 10.3% |
| t = 1.5 | 0.681 | 0.042 | 18.0% |
| t = 1.0 (plain normal map) | 0.383 | 0.000 | 25.7% |
| normalise N | 0.465 | 0.099 | 13.8% |
| **wrap the diffuse** | 0.988 | **0.568** | **0.0%** |

★ Why: `normal.y = abs(normal.y)` and `dir` is purely horizontal, so `N.y = t*|n.y|`. The
extrapolation is the very thing lifting the canopy toward a high sun. Reducing `t` dims the tree
AND deepens the shadows. I had reasoned about that constant without measuring it.

### First, the technique itself is deliberate - Lee called it correctly

He asked whether the patchiness was intentional, to keep one billboard image plausible from any
angle. It is, and there are three tells in the shader:

1. `normal.y = abs(normal.y)` - every baked normal forced upward, so the canopy always catches an
   overhead sun. A tree bake has plenty of downward leaves; someone chose to flip them.
2. the normals are ROTATED by the billboard's facing angle, so the baked shading spins with the
   quad and the tree reads as the same lit object from anywhere.
3. `lerp( dir, normal, 2 )` - `t=2` is extrapolation, deliberately amplifying deviation from the
   flat quad normal. `t=1` would be the plain map.

Measured, sweeping the sun: the normal-mapped billboard stays 24-69% shaded at EVERY azimuth and
never goes fully lit or fully black, and halves the brightness swing (0.35 vs 0.71). A flat quad
flips 0% <-> 100% and goes completely black for half the compass. Exactly Lee's hypothesis.

⚠ Ruled out by measurement, not assumed: mip filtering dragging the normal map's out-of-silhouette
rainbow facets into the canopy. Averaging shortens normals (|n| 0.92 -> 0.36 by mip 7) without
biasing them (z mean -0.04 -> -0.07), and as |n| -> 0 the shader's `2n - dir` converges cleanly to
the flat quad normal. Distance makes it flatter, not patchier.

### The fix: `tree_shadeWrap`

Classic wrap / half-Lambert on the billboard diffuse. The exact term `(N.L + w)/(1 + w)` cannot be
injected where the light loop lives (`DirectionalLight()` is inside Wicked's lightingHF), but it
does not need to be: **Wicked never normalises `surface.N`** (`surfaceHF.hlsli update()` only
saturates roughness), so feeding it

    N' = ( N + w * |N| * L ) / ( 1 + w )

reproduces the wrap term exactly for the sun with |N| carried through unchanged. Entirely inside
GGTreesPS - no engine change, nothing else in the scene touched. `w = 0` skips the branch, so the
default is **bit-identical** to pre-3.07.

`GetSunDirection()` points TOWARD the sun (skyHF.hlsli:159 draws the sun disc where the view ray
matches it), so it is the `L` in `N.L` with no sign flip - checked, not assumed.

In-engine, same camera, forested band:

| wrap | darkest 5% | median | % below 40 |
|---|---|---|---|
| 0 | 19.2 | 75.7 | 16.4% |
| 0.30 | 25.5 | 77.5 | 12.9% |
| 0.50 | **28.3** | 78.2 | 11.3% |

Darks lift ~47%, median moves 3% - it lifts the shade and leaves the lit side alone, which is what
was asked for.

**DEFAULT 0 (off).** Picking the value is an art call and it is Lee's. `setup.ini treeshadewrap`
(PERCENT int, house rule), harness `SET_TREESHADEWRAP 0..1` (live next frame, no rebind).

### 3.07a — 0.5 shipped as the default (2026-08-24)

Lee A/B'd 0 / 0.35 / 0.5 live at the machine and chose **0.5**. Now the compiled default
(`gg_tree_shade_wrap = 0.5f`), with `setup.ini treeshadewrap` defaulting to 50 to match.

★ 0 remains bit-identical to pre-3.07, so the off position doubles as the A/B control - keep it
that way.

⚠ Two boundaries recorded with the value, because a bare number loses them:
1. **0.6+ starts eating the shading PATTERN** rather than lifting it, flattening the tree toward a
   card - the exact thing the normal map exists to prevent. 0.5 is the top of the useful range,
   not a midpoint.
2. **Tuned under a near-overhead morning sun (87/75).** Wrap does proportionally more work as the
   sun drops, so a dusk or overcast level may want less. If that bites, the knob wants to move to
   the PER-LEVEL visuals rather than a machine-wide ini key. Not done - no evidence it bites yet.

Verified the DEFAULT path, not just the harness path (the standing rule). Fresh launch, no command
sent, no ini key present, same camera:

| | p05 lum | median | % below 40 |
|---|---|---|---|
| harness w=0 (old look) | 19.2 | 75.7 | 16.36% |
| harness w=0.5 | 28.3 | 78.2 | 11.25% |
| **fresh launch, nothing sent** | **28.3** | **78.4** | **11.20%** |

---

## 3.08 — low-spec off-switches, round 2 (2026-08-24, overnight)

Brief: *"further improve performance by adding more brutal off switches... allowed to reduce
visual contents and reduce fidelity in the pursuit of ultra-low-spec support."*

The 2.94 four remove CONTENT (terrain/trees/grass/water). These remove per-frame RENDERING WORK,
so they still help on a level with nothing left to strip - an indoor level being the obvious case.

★ Every lever here **already existed and worked**. They were bound to debug KEYBOARD keys 1-9 in
`GGTerrainWicked_Update` and had never been exposed to a player. This makes them real switches:
panel tick + `setup.ini` + harness, the same shape as the 2.94 four.

⚠ NOT duplicated: render resolution. The **FSR combo in Graphics & Performance already exists**
and drives `resolutionScale` down to 1/2 native (None / Ultra Quality / Quality / Balanced /
Performance). That is the single biggest low-spec lever and it was already there.

| switch | setup.ini | harness |
|---|---|---|
| Post Effects Off | `nopostfx` | `SET_POSTFXOFF` |
| Ambient Occlusion Off | `noao` | `SET_AOOFF` |
| Simple Sky | `simplesky` | `SET_SIMPLESKY` |
| Shadows Off | `noshadows` | `SET_SHADOWSOFF` |

### Measured — Aztec Game Kit, editor, camera parked, profiler rows, 2 interleaved passes

| switch | GPU busy | FPS | what vanishes |
|---|---|---|---|
| **Shadows Off** | 4.12 → **2.95** (−1.18 ms) | 146.8 → **171.7** (+17%) | `Shadowmap Rendering` |
| Post Effects Off | 4.12 → 3.94 (−0.17 ms) | +2.4 | `Bloom`, `LightShafts` |
| Ambient Occlusion Off | −0.09 ms | ~0 | `MSAO` |
| Simple Sky | ~−0.08 ms | ~0 | nothing — see below |

★ **Shadows Off is the whole story on this level**; the other three are honest but small. Post FX
is small *here* because Aztec runs only bloom and light shafts — on a level using SSR, depth of
field or motion blur the same switch removes far more. Simple Sky measures ~0 because Aztec does
not use volumetric clouds or the realistic sky at all; it needs a level that does before anyone
claims a number for it.

### ⚠ Two process failures worth keeping

1. **I nearly declared four working switches dead.** Grepping the engine for the member name
   `shadowsEnabled` found "NOWHERE", and the same for lens flare, chromatic aberration, sharpen
   and occlusion culling. All five are read through their **ACCESSORS** (`getShadowsEnabled()`)
   at the call site. Grep the getter, not the field.
2. **The first restore was one-way.** It force-set things OFF each frame and leaned on
   `GGApplyVisualsNow()` to put them back on the OFF edge. Measured, that only restored *some*:
   bloom and light shafts returned, shadows and AO did not, so every later row of the first
   ladder was taken on a scene that still had shadows off. Now each switch **captures the
   caller's own value on the ON edge and writes it back on the OFF edge** — which also means we
   never invent a default: a level that ships with bloom already off keeps it off.
   ★ The ladder now prints a `\_restored` row after every switch precisely so a one-way latch
   cannot hide again.

⚠ The `Shadowmap Rendering` row itself swings 0.6–3.4 ms between samples (staggered cascade
refresh), so quote **GPU Busy**, not that row.

⚠ First test bed was wrong: TESTPRO2 has GPU Busy 2.66 of a 5.32 ms frame and no Shadowmap,
Bloom, AO or SSR row at all — nothing for these switches to remove. A null result there measured
the level, not the code.

## 3.09 — Occlusion Culling Off + Particle Density (2026-08-24)

| switch | setup.ini | harness | panel |
|---|---|---|---|
| Occlusion Culling Off | `noocclusion` | `SET_OCCLUSIONOFF` | tick |
| Particle Density % | `particlepct` | `SET_PARTICLEPCT 0..100` | slider |

### Occlusion Culling Off — measured a LOSS, and shipped anyway

Aztec, 3 interleaved passes:

| | GPU busy | FPS |
|---|---|---|
| occlusion ON (default) | 4.06 / 3.97 / 4.12 | 146.6 / 147.3 / 146.8 |
| occlusion OFF | 5.04 / 4.87 / 4.94 | 124.1 / 123.7 / 123.9 |

Turning it off **costs ~0.9 ms and 15% of the frame rate**. The `Occlusion Culling` +
`Occlusion Culling Render` rows are 0.47 ms, so on this level the queries earn roughly double
their price. Kept as a switch because the trade genuinely inverts on scenes with little
overdraw and a weak GPU - but the tooltip and the DOCDOC both say plainly that it is an
experiment, not a saving, and Aztec is the counter-example.

### Particle Density

Scales `count` (the emit rate) on every Wicked emitter. Writes `base * k`, never `count * k` -
scaling the live value each frame would decay it to zero within a second. Captures each
emitter's authored rate the first time it touches it and writes that back at 100%.

⚠ **Implemented, NOT measured.** Aztec's `EmittedParticles - Render` rows are 0.00 ms - the level
has no meaningful particle load - so there was nothing to measure it against. It needs a
particle-heavy demo before anyone quotes a number.
⚠ Wicked emitters ONLY. GG has three particle systems; the legacy gpup/.arx and WPE emitters do
not respond to this slider.

## 3.10 — two of Lee's ideas that turned out to be ALREADY COVERED

Worth recording so nobody builds them twice.

1. **"Reduce the entire texture resolution across the board... 1024 loaded as 256."**
   Partly covered *for the editor* by texture streaming, which already streams mips out toward a
   per-material target. ⚠ But streaming is **editor-only** (`gameisexe == 0`), so it does nothing
   for the players this brief is aimed at. A real version has to reduce at LOAD time. See the
   design below - it is worth doing and it is NOT built.

2. **"Simplify the water into a very simple flat plane."**
   The expensive half is already a shipped control. `getReflectionsEnabled()` gates the planar
   reflection pass - the one that redraws the whole scene from the mirrored camera - and that is
   the existing **Reflections** checkbox plus `SET_REFLECTIONS`. What a "simple water" switch
   would add beyond it is the flat-wave part, and `Ocean - Simulate` measures **0.18 ms**. Not
   worth a third water control next to Water Off and Reflections.

3. Likewise **render resolution**: the **FSR combo already in Graphics & Performance** drives
   `resolutionScale` to 1/1.3, 1/1.5, 1/1.7 and 1/2 native. That is the biggest low-spec lever in
   the product and it predates tonight.

### ★ DESIGNED, NOT BUILT: global texture-detail divide

The right insertion point is `wiResourceManager.cpp` `LoadResourceDirectly`, at `int mip_offset
= 0;` (line ~511), BEFORE the `Flags::STREAMING` branch. Shrink `desc` and advance `mip_offset`
by 1 or 2 steps; the upload already reads `header.mip_offset(mip_offset + m, 0)`, so both the
streaming and the plain path honour it. That makes it work in an EXPORTED GAME, which the
streaming route cannot.

⚠⚠ Two hazards, both already paid for once in this codebase:
- **GGMAX 1.73 block alignment.** A BC texture's TOP mip must be a multiple of the 4x4 block.
  Halving a legal size can produce an illegal one (500 -> 250); D3D12's `GetCopyableFootprints`
  then returns 0xFFFF.. sentinels and the upload memcpy's four billion rows off the end of the
  file buffer. That was the "Trapped" and "RPG Template" load crash. Copy the existing guard
  loop's `format_block_size` check exactly, and stop early rather than produce an illegal size.
- `initdata` is populated from the header BEFORE `mip_offset` exists, so the subresource array
  has to be rebuilt (or indexed with the offset) as well as `desc` shrunk.

It is an ENGINE change, so it needs `build_wicked.bat Release` first and a
`WICKED_ENGINE_CHANGES.md` delta row. Not attempted overnight because getting it wrong re-opens a
crash that takes out level loading, and default-off is not protection if the default path is
mis-wired. Deserves a session with the user awake.

## 3.11 — Object Detail Distance, and the U-shaped curve (2026-08-24)

`objectculldist` / `SET_OBJCULLDIST <units>` / panel slider. 0 = off. Caps every object's
`ObjectComponent::draw_distance`, so Wicked dissolves it out over its own radius and culls it
outright past `draw_distance + radius` - the 3.03 machinery, reused.

★ Objects that ALREADY hold a finite `draw_distance` are skipped, never overwritten. That is how
the 3.03 tree pool keeps ownership of its handover fade; capping it here would fight the billboard
swap and put the pop back. It also makes the restore exact: everything we touched came from
FLT_MAX, so FLT_MAX is what it returns to.

### Measured on Aztec (5,431 objects), 2 interleaved passes

| cull | FPS | GPU busy | POLYS | Shadowmap |
|---|---|---|---|---|
| off | 142.7 / 142.0 | 3.96 / 4.16 | 522,301 | 0.76 |
| 4000 | 144.4 / 148.2 | 3.96 / 3.92 | 522,301 | 0.80 |
| 2000 | 155.6 / 154.1 | 3.81 / 3.77 | 522,301 | 0.58 |
| **1000** | 154.6 / **157.1** | 3.72 / **3.67** | 522,301 | **0.40** |
| 500 | 140.3 / 139.8 | **4.19** | **312,049** | 0.42 |

★★★ **The curve is U-shaped and the bottom is not where you would guess.**

- The win peaks around **1000-2000 units (+9-10% FPS)** and it comes from **shadow casters**, not
  triangles: `Shadowmap Rendering` halves (0.76 -> 0.40) while POLYS does not move at all. Objects
  in that band are outside the shadow cascades' interest before they are outside the main view.
- At **500 units it becomes a NET LOSS** - FPS 142 -> 140 and GPU busy *up* to 4.19 - even though
  POLYS finally falls 40%. Because a quarter of the screen is then objects mid-dissolve, and
  Wicked's dither fade is an ALPHA-TESTED path: it costs more per pixel than the opaque draw it
  replaced. Cull hard enough and you pay more in dithering than you save in geometry.

⚠ **My first ladder measured nothing** (0 / 8000 / 20000 / 40000, all POLYS-identical) and I
briefly read that as a broken switch. It was a broken TEST: Aztec is compact and 8000 units is
200 m, well beyond anything in it. The switch proved itself instantly at 500 units (POLYS -40%,
24% of the screen changed). ★ When a lever does nothing, check its range against the CONTENT
before checking the code - the same mistake as measuring the 3.08 switches on a level that had no
shadows, bloom or AO to remove.

### Recommended defaults

Ship at 0 (off). The useful value is level-scale dependent: ~1000-2000 on a compact level like
Aztec, proportionally more on a sprawling one. Anyone tuning it should watch `Shadowmap Rendering`
rather than POLYS, and should stop lowering it the moment GPU busy starts rising again.

## 3.12 — global texture-detail divide (engine `b40fc4d2`)

Lee's flagship idea from the brief: *"reduce the entire texture resolution across the board so
everything gets divided by 2 or 4 (so a 1024x1024 texture is dynamically loaded as 256x256 for
faster bandwidth movement)."* Built.

`setup.ini texturedivide` (1/2/4) · harness `SET_TEXTUREDIVIDE` · panel **Texture Detail**
(Full / Half / Quarter). DEFAULT 1 = off, and the code path is skipped entirely.

### Where it lives, and why there

`wiResourceManager.cpp` `LoadResourceDirectly`, at `int mip_offset = 0;`. `CreateTexture` is
already called as **`initdata + mip_offset`**, so shrinking `desc` and advancing the offset is the
entire trick — the subresource array needs no rebuild. Engine change, so it carries a
`WICKED_ENGINE_CHANGES.md` row. ★ The engine builds in **38 seconds**, which is worth knowing —
I had been treating an engine change as expensive and deferring it on that basis.

★ **Deliberately a LOAD-TIME reduction, not a streaming target.** Wicked's streaming already walks
mips down toward a per-material goal — but it is gated **editor-only** (`gameisexe == 0`), so it
can do nothing for the low-spec players this whole brief is aimed at. A load-time cut runs in an
exported game.

⚠ Streaming is **disabled for a divided texture** on purpose: the streaming branch records
`streaming_data.data_offset = header.mip_offset(mip)` for ABSOLUTE mips, which would every one be
off by `mip_offset`.

⚠⚠ Copies the **GGMAX 1.73 block-alignment guard** verbatim and stops early rather than emit an
illegal top mip. Halving 500 to 250 makes an invalid BC resource; `GetCopyableFootprints` returns
0xFFFF.. sentinels and the upload memcpy's four billion rows off the end of the file buffer — the
"Trapped" and "RPG Template" load crash. An oddly sized texture simply keeps more detail.

### Measured — and the number is smaller than the idea suggests

Aztec Game Kit, editor, fresh launch per setting (which also exercises the ini path):

| | VRAM | FPS |
|---|---|---|
| Full | 3598.1 MB | 143.7 |
| Quarter | **3386.8 MB** (−211 MB) | 148.0 |

−211 MB and +3%. Real, and against Aztec's in-game 3947.9 MB / 155 MB headroom it more than
doubles the margin — but it is nowhere near the 16x a quarter-size texture implies. Three reasons,
all worth knowing before anyone tunes it:

1. ★ **The editor was ALREADY streaming those textures down**, and this switch opts divided
   textures OUT of streaming. The two partly cancel. The exported-game case — streaming off,
   full-size textures otherwise — should save considerably more. **UNMEASURED: the harness cannot
   drive an exported build.** That is the number Lee actually cares about and I could not get it.
2. GG's own loaders bypass `wi::resourcemanager` entirely — the terrain SVT atlas (~480 MB
   committed at boot on every level), the tree billboard atlases, grass. Untouched by this.
3. Much of the rest is render targets, shadow atlas and mesh data, which no texture setting moves.

⚠ On TESTPRO2 it measured **nothing** (2908 / 3149 / 2812 MB across 1 / 2 / 4 — non-monotonic
noise) while still changing 6.5% of the screen. Same lesson as 3.08 and 3.11: the level decides
whether a lever can show. TESTPRO2's VRAM is dominated by the SVT atlas and GG's own atlases,
which this cannot touch.

### 3.12 safety validation

The new code path re-enters the exact territory of the GGMAX 1.73 load crash, so the two levels
that crashed then were the ones to test — at Quarter, where the block-alignment guard has to hold:

| level | divide | result |
|---|---|---|
| Trapped | 1 (default) | loads, 297.3 FPS, VRAM 2312.7 |
| Trapped | **4** | **loads**, 295.4 FPS, VRAM 2248.9 |
| RPG Template | **4** | **loads**, 185.6 FPS, 540,778 polys, VRAM 2853.4 |

Guard holds. Aztec and TESTPRO2 also loaded repeatedly at 1/2/4 during the measurements above.
⚠ This is not a 19-demo sweep — that is still owed for the whole 3.08–3.12 batch. Everything
ships default-off, so the un-swept risk is confined to the ON positions.

### 3.08–3.12 regression coverage (partial, at DEFAULT settings)

Not the owed 19-demo sweep, but 8 levels loaded clean on the final build with every new switch at
its default:

| level | FPS | POLYS | VRAM |
|---|---|---|---|
| Aztec Game Kit | 143.7 | 522,301 | 3598.1 |
| Foggy Forest | 136.5 | 1,248,844 | 2915.8 |
| Operation Amazon | 143.8 | 486,602 | 3369.3 |
| Snowy Mountain Stroll | 203.5 | 81,369 | 2651.2 |
| Escape from the Zombie Cellar | 258.3 | 28,048 | 2246.1 |
| Trapped | 297.3 | 12,768 | 2312.7 |
| RPG Template (at Quarter) | 185.6 | 540,778 | 2853.4 |
| TESTPRO2 | 180.3 | — | 2908.3 |

★ Foggy Forest reads 1,248,844 polys against the 0823 sweep's 1.28M — consistent, so the tree work
underneath is undisturbed.

⚠ **STILL OWED: the full 19-demo sweep** for the 3.08–3.12 batch (and pass 2 of the 0822 gate,
outstanding since then). Everything in this batch ships default-off/neutral, so the unswept risk
is confined to the ON positions, but the sweep is the gate and it has not been run.

---

## 3.13 — the CPU panel was double AND triple counting (2026-08-24)

> *"I suspect common_logic is double counting those MS ticks, and I want to make sure the GameLoop
> is not also triple counting those MS ticks too."*

**Both suspicions correct.** Nothing was mistimed — the panel printed a call TREE as a FLAT,
alphabetically sorted list, so a parent and its child looked like two independent costs.

### The actual tree, traced through the source

    Update - Logic (Total)          GuruLoopLogic()               master_part1.cpp:604
    └── Logic - common_loop         common_loop_logic()           GameGuruMain.cpp:209
        └── CL-GameLoop             commonexecutable_loop_for_game()  M-GridEdit_part1.cpp:80
            └── game_main_loop()
                ├── Update - Logic - Physics / LUA / Objects / AI     M-Game_part3.cpp
        └── CL-PreBlock, CL-Head, CL-ObjLists, ... (siblings of CL-GameLoop)
    └── Logic - ConstantNonDisplay

Chain verified link by link: `common_loop_logic` calls `mapeditorexecutable_loop`
(Common_part0.cpp:780), which is where `CL-GameLoop` wraps `commonexecutable_loop_for_game`, which
reaches `game_main_loop` via `game_masterroot_gameloop_loopcode` (M-Game_part1.cpp:1642).

So on Lee's shots: **3.32 + 3.56 + 3.55 = 10.4 ms of a 7.99 ms frame.** One 3.5 ms cost, printed
three times at three depths, with nothing to say so.

### The fix — print the tree

`Range` now records `gg_depth` / `gg_parent` / `gg_main_thread` from a **thread_local** stack of
open CPU ranges, and `GetTextData` prints a DFS tree: indentation, a `[self]` column (parent minus
its direct children) and a `[worker]` tag. Engine change, delta row filed.

Reads like this now — Lee's preferred shape, kept:

    CPU Frame: 6.19 ms
      main-thread rows total: 6.82 ms   (+0.63 vs frame - 20-frame averaging skew)
      (indented rows are INSIDE their parent - only same-indent rows add up)
      Update: 4.01 ms   [self 0.13]
        Update - Wicked (Total): 2.06 ms   [self 0.01]
          Scene-S2 Hier+Mesh+Mat: 0.93 ms
          Scene-S1 Anim+Transform: 0.59 ms   [self 0.20]
            Animations: 0.39 ms
        Update - Logic (Total): 1.61 ms   [self 0.03]
          Logic - common_loop: 1.56 ms   [self 0.04]
            P2-mainfunc: 0.54 ms
            P2-entity_loopanim: 0.32 ms
            CL-ObjLists: 0.22 ms
      Render: ...
      Shadowmap Rendering: 1.97 ms   [worker]

★ The `[self]` column is the useful new number: `Update - Logic (Total)` has **self 0.03** — it is
pure pass-through, all its time is its children's. That is the honest answer to "is this double
counting": yes, and now you can see precisely how much of any row is its own work.

### Two traps hit while building it

1. ⚠ **"CPU Frame" is itself a CPU range and opens FIRST on the main thread**, so every
   main-thread range names it as parent. It is deliberately excluded from the row cache — so
   treating only empty-parent rows as roots ORPHANED THE ENTIRE MAIN THREAD and the first build
   printed nothing but worker rows, with `accounted 0.00 ms`. Its children have to be roots.
2. ⚠ **Every row, CPU Frame included, is a 20-frame rolling average on its own counter.**
   Independently-averaged children therefore do not sum to exactly the averaged parent; the
   residual wandered −0.30 → −0.90 → +0.63 between samples. It is reported as a **signed delta
   named as averaging skew**, not as "unattributed" — a negative "unattributed" would read as
   exactly the double-count bug this change exists to remove.

### What this does NOT claim

The tree is built from range NAMES, so a range called from two different parents collapses into a
single row under whichever parent was seen last, while its time includes both call sites. No such
case is visible in the current output, but it is the failure mode to watch if a row ever looks
mis-parented.

⚠ `CL-GameLoop` reads 0.00 in the editor because the game loop does nothing there — Lee's 3.55 ms
was in-game. The tree is the same either way; only where the weight sits changes.

---

## 3.14 — drilling Scene-S2: the pole was a rebuild of something already correct (2026-08-24)

Lee: *"drill into Scene-S2 Hier+Mesh+Mat and P2-mainfunc."*

### Naming the pole

`SET_SCENESERIAL 1` (the 2.15 diagnostic: serialises each `Scene::Update` system so each gets its
own range — totals inflate, shares are real) split Scene-S2 immediately:

| | ms | share |
|---|---|---|
| **SU-Hierarchy** | **0.85** | **71%** |
| SU-Mesh | 0.19 | 16% |
| SU-Material | 0.16 | 13% |

Shape of the hierarchy on this level: `roots=2694 maxSubtree=632 visited=8774 imbalance=0.072`.
★ So it is NOT one huge subtree starving the parallel dispatch — 8,774 nodes spread over 2,694
roots, well balanced. The cost is simply per-node work × 8,774, every frame.

Per node the walk does **three hash lookups** (`transforms.GetComponent`, `layers.GetComponent`,
`topdown_hierarchy.find`) — ~26,000 per frame — against trivial matrix maths. That is where 0.85 ms
of an 8-node-deep tree walk goes.

### But the actual fix was upstream of all that

`StartBuildTopDownHierarchy` runs at the head of every `Scene::Update` and **cleared and
repopulated the entire `topdown_hierarchy` map plus the roots list EVERY FRAME** — another ~8,774
hash lookups and vector pushes, to reproduce a structure that was already correct.

The consumer already decides whether the snapshot is usable from exactly two values (mutation
stamp + count). **If that pair is good enough to TRUST the snapshot, it is good enough to skip
REBUILDING it.** Now it returns early when they match.

⚠ This escalates what the mutation counter must be right about. Previously a missed increment
self-corrected — the snapshot was rebuilt from live data anyway. Now it would leave a stale
snapshot. So every mutation site was audited: `Clear`, `MergeFastInternal`, `Entity_Remove`,
`Component_Attach`, `Component_Detach` all bump it — and the **only** `hierarchy.Create` outside
them, `wiScene_Serializers.cpp:3025` (deserialize), did **not**. Fixed. (A pure add also moves
`GetCount()`, which the guard checks, so that one was belt as well as braces.)

### Measured — TESTPRO2 canyon, 3 interleaved passes

| | CPU Frame | FPS |
|---|---|---|
| rebuild every frame (pre-3.14) | 6.21 / 6.40 / 5.88 | 149.2 / 147.4 / 150.7 |
| **skip when unchanged** | 5.72 / 5.84 / 5.71 | **152.7 / 155.1 / 153.1** |

**+3.0% FPS, and every ON sample sits above every OFF sample** — no overlap, which matters because
the CPU-Frame spread within the OFF condition (0.52) is itself wider than the mean delta (0.40).
Quote the FPS separation, not the CPU-Frame delta.

Executed-check (`HIER: rebuilds=/skips=`): with the cache off, rebuilds climb ~2,250 per sampling
interval and skips barely move; with it on, rebuilds climb ~390 while skips climb ~1,670. ★ So
~80% of rebuilds go away — and the residual is real: ~37% of frames genuinely mutate the
hierarchy on this level. ⚠ Also note the counters prove `Scene::Update` runs ~3x per frame here
(≈2 skips per frame), which matches the 1.81 caller-tracer finding.

Harness `SET_HIERCACHE 0|1`, default ON. Engine `e280911e`, delta row filed.

### P2-mainfunc — scoped, NOT drilled

`editor_mainfunctionality()` (M-GridEdit_part4.cpp:1467-1961) is ~500 lines of editor input and UI
handling, and almost every call in it is conditional (undo, save-as, waypoint drag, panning), so
reading alone does not say where 0.54 ms/frame goes. It needs the same treatment Scene-S2 got: sub
-ranges at the top-level block boundaries, then read the panel. ★ NOT attempted in the time left,
because placing Begin/End pairs across 500 lines of branchy code without verifying every path is
how you leak a range and corrupt every row after it — and the 3.13 tree printer would show that
corruption as a mis-parented subtree. Next session's first job.

## 3.15 — P2-mainfunc: a per-frame ray-intersect over every waypoint node (2026-08-24)

Sub-ranges added to `editor_mainfunctionality` (safe to place by hand — the function has **zero
early returns**, checked first, so no path can skip an `EndRange` and leak a row into every
sibling after it). Balance verified 7 Begins / 7 Ends before building.

### The bisect, in two builds

Split 1 — and it refuted my assumption immediately:

| | ms |
|---|---|
| **P2M-Events+Tail** | **0.56** of 0.57 |
| P2M-Sel5-EditorMode (the 330-line rotation block) | 0.00 |

★ I had written the tail off as "cheap event-driven ifs" and expected the big block to dominate.
Exactly backwards. Split 2 narrowed it to `P2M-T2-SelModes+Keys` = **0.61 of 0.62 ms**, a 40-line
block whose only real call is `waypoint_mousemanage()`.

### What it was doing

`t.tokay` is set to 1 whenever you are in entity mode with nothing picked — i.e. essentially every
frame in the editor. So every frame, for EVERY waypoint node, `waypoint_mousemanage` did:

    CameraPositionX/Y/Z()                 <- a constant, read once per NODE
    ObjectExist(proxy) / MakeObjectCube   <- re-checked per NODE
    PositionObject(proxy, node)
    HideObject(proxy)
    IntersectObject(proxy, camera -> cursor)   <- a full engine ray test, per NODE

purely to find which node the mouse is hovering.

### The fix

1. camera position read once, not per node (it cannot change mid-loop);
2. proxy cube ensured once;
3. ★ a cheap analytic reject first — closest approach of the node to the pick SEGMENT; if that
   exceeds what the proxy could reach, the intersect cannot hit, so skip it.

(3) is a conservative **BOUND**, which is the safe direction (the standing rule: a superset bound
is safe as a bound, unsafe as a position). `MakeObjectCube(25)` has half-extent 12.5 and therefore
a half-diagonal of 21.65; the reject radius is 25, so it is generous and can only ever admit extra
candidates into the *same* intersect test that decided them before. It is never used as a position.

### Measured — TESTPRO2 canyon, 3 interleaved passes

| | P2M-T2 | P2-mainfunc | CPU Frame | FPS |
|---|---|---|---|---|
| intersect every node | 0.57 / 0.54 / 0.54 | 0.56 / 0.59 / 0.55 | 6.01 / 6.78 / 6.04 | 150.6 / 150.5 / 150.1 |
| **analytic reject** | **0.01 / 0.01 / 0.01** | 0.01 / 0.01 / 0.02 | 5.86 / 5.51 / 5.66 | **163.1 / 155.6 / 161.4** |

**+6.4% FPS**, every ON sample above every OFF sample. The row does not shrink — it disappears.

Harness `SET_WAYPOINTFAST 0|1`, default ON.

⚠ **NOT interactively verified.** The bound is conservative by construction and only skips nodes
that could not have intersected, but nobody has hovered and clicked a waypoint node on this build.
That is a two-minute check with a mouse and it should be done before this is trusted in anger.

### Running total for the CPU side on this level

3.14 (hierarchy snapshot) +3.0% and 3.15 (waypoint reject) +6.4%, both measured independently and
both with clean sample separation.

## 3.16 — P2-entity_loopanim and CL-ObjLists: attributed, not yet fixed (2026-08-24)

### entity_loopanim — the shape

Counters rather than profiler ranges (a Begin/End per entity would cost more than the thing being
measured). `ELANIM:` now rides `GET_PERF_DATA`:

    total=2241  skipNoEnt=37  skipStatic=1961  work=243

**87.5% of iterations exist only to discover they have nothing to do.** They read `bankindex`,
`staticflag`, `eleprof.phyalways`, `eleprof.animspeed` out of a very large struct and
`entityprofile[entid].animspeed` out of a second array at a random index, then `continue`.

### Splitting the 0.34 ms

`SET_ELANIMSKIPWORK 1` bails immediately after the static decision, so the loop measures only
"walk and decide". Interleaved:

| | P2-entity_loopanim |
|---|---|
| full | 0.32 / 0.34 ms |
| walk + decide only | 0.13 / 0.13 ms |

So **~0.13 ms is the scan over 2,241 elements (38%)** and **~0.20 ms is the dynamic path for just
243 of them (62%)** — 0.8 µs each, which is ~2,500 cycles per entity and far more than the visible
array reads justify.

⚠ `ObjectExist` and `GetNumberOfFrames` were the obvious suspects and are NOT the cause — both are
a bounds check plus an array index in `CObjectsC_part4.cpp`, not map lookups. Checked before
optimising, which is the only reason that dead end cost a grep instead of a build.

### Why nothing was changed yet

Two candidate fixes, both refused for now with reasons:

1. **Cache a compact "needs work" element list** (the 3.14 pattern) to kill the 0.13 ms scan. The
   skip decision depends on per-element `animspeed` matching the profile's, which a user can edit
   at runtime, so the invalidation surface is every entity-property path in the editor. ★ 3.14 was
   safe to cache because ONE counter already guarded it and I could audit all five mutation sites
   in minutes. Here I cannot, and the failure mode is animations silently stopping — a bug that
   would reach Lee as "some things don't animate any more" days later. Not worth it blind.
2. **Attack the 0.20 ms dynamic path** — needs to know what those 243 entities actually spend
   2,500 cycles on, which is another instrument-and-build cycle. That is the higher prize and the
   right next move.

Diagnostics left in, default off: `SET_ELANIMSKIPWORK 0|1` (⚠ changes behaviour — freezes
animations and decals, attribution only) and the `ELANIM:` counters.

### CL-ObjLists — assessed, deliberately not touched

`CL-ObjLists` spans M-GridEdit_part1.cpp:8516-11128 — **2,600 lines of ImGui** building the Level
Entities panel, for **0.22 ms**. No `ImGuiListClipper` anywhere in the file, so the standard win
would be clipping long lists to visible rows.

★ Not attempted: 0.22 ms against 2,600 lines of interactive UI is a poor ratio, and the failure
mode of getting list virtualisation wrong is broken selection and drag-drop in the editor's most
used panel. It should be done deliberately, with the panel open and a mouse, not overnight.
