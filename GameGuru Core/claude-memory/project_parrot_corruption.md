---
name: project-parrot-corruption
description: "RESOLVED 2026-07-18: intermittent 'exploded parrot' skinned-model corruption on Island Showdown loads — diagnosis, layered fix (game 459397a6 + Wicked delta 1.9 a4539a76), tripwire files, and what remains unknown."
metadata: 
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
---

# Parrot (skinned-model) corruption — RESOLVED 2026-07-18

**Symptom:** ~half of Island Showdown loads, the animated galah (parrot) models exploded into giant coloured sheets across the sky. Pre-existing, user-confirmed, unrelated to lights/fog.

**Diagnosis (via new harness commands `DUMP_SKIN` + `SKIN_WATCH`):**
- The Wicked object name for the parrots is **`galah`** (17 skinned instances: 15 live + 2 parked masters). GG entity names do NOT propagate to Wicked entities.
- Corruption = garbage `rotation_local` on exactly **Bone006/Bone007** (the two matrix-key feather bones) of every galah instance: the CORRECT quaternion axis scaled by thousands (-8833… or -2.4e15) — an **unnormalized accumulation**, not random bytes. Scale (0.99,0.71,0.69) and translation always stayed legit (they're real feather-fold animation values).
- **Keyframe data was always clean** (BAD_ANIMDATA=0 even on corrupt loads). Garbage was written into bone transform LOCALS during the level-load burst, in one shot, on all instances.
- Any later animation evaluation heals the pose (slerp with amount=1 returns the keyframe exactly). Visible corruption = the loads where those animations never re-ticked (stopped + `last_update_time==timer` skip + culling). "Healed" loads could still carry sub-threshold garbage (load 7 ended at LR=-8818 — my `inf` grep missed it).
- AABBs are useless for detection: skinned-object AABB comes from bone positions (armature AABB), which stay sane — only the skin matrices (`|skinT|` in DUMP_SKIN) explode.

**Root-cause status: writer never caught in the act.** The transient stopped reproducing after the first engine relink (heap/timing sensitive — 12+ clean loads with instrumented builds vs 4-of-7 affected before). Slerp-propagation (`XMQuaternionSlerp` propagates/amplifies garbage inputs, engine stores UNNORMALIZED results) and `XMMatrixDecompose` on sheared/torn matrices are the proven amplification paths.

**The layered fix (all committed+pushed):**
1. **Wicked delta 1.9** (`a4539a76`): unit-quaternion guards at BOTH rotation write-back sites in `RunAnimationUpdateSystem` (renormalize / identity for zero-NaN-inf), `ApplyTransform` validates the decompose (keeps previous rotation/scale on garbage), tripwire logging.
2. **Game (`459397a6`)**: `WickedCall_SanitizeSkeletons()` at end of `gridedit_load_map` — repairs non-unit bone rotations then forces one evaluation of EVERY animation under the reveal cover. **Gotcha: `updateonce=true` alone is a NO-OP for idle anims — must also set `last_update_time = timer - 1.0f`** (engine skips when `last_update_time==timer` regardless of updateonce).
3. `WickedCall_LoadNode` local rotation init fixed to identity quaternion (0,0,0,1) — was the invalid (0,0,0,0), which poisons any slerp from it.

**Tripwires (leave in place):** if `anim_garbage.txt` or `applytransform_garbage.txt` ever appears next to GameGuruMAX.exe, a guard fired and the file names the original writer (ApplyTransform logs a symbolized call stack via dbghelp). Zero cost unless firing.

**Verified:** 4/4 clean Island Showdown loads, feather bones land on the true animated pose (unit LR, LS=(1,0.69,0.69)); TESTPRO1 baseline 59.9 FPS unchanged.

**Unrelated look-alike:** the small green object with sparkles on the beach near the A/B camera is a collectable pickup, not a parrot, present in every load — ignore.

Related: [[project-wicked-engine-changes]] (delta 1.9), [[project-harness-open-my-games]] (Island Showdown repro flow).
