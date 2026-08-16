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
revisiting task #37.

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
