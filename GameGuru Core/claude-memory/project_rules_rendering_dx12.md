---
name: project-rules-rendering-dx12
description: "Durable hard-won rendering/DX12 rules — throttle units, pass-pair contracts, fp16, PSO lifetimes, caches, streaming. Read before touching a render path."
metadata: 
  node_type: memory
  type: project
  originSessionId: 509f3c47-3d86-4b9b-a337-23ada2c00769
  modified: 2026-08-30T23:43:05.586Z
---

# Durable rendering / DX12 rules (verbatim from the index; each paid for at least once)

- ★★★ **When you throttle work, the throttle's unit must be the unit the DATA is shared over — not whatever collection you happen to be iterating.** A GameGuru character is SEVERAL OBJECTS sharing ONE ARMATURE (head/body/legs are separate meshes). Keying an animation-skip phase and distance off the OBJECT index put the head and body on different frames AND in different periods, and a head posed from an older armature state than its body is literally **a second head** (Lee, 3.25n). Decide ONCE per armature and have every consumer read that one answer — not "compute it identically in two places", which drifts. Notes §3.25n.
- ★★★ **A depth-prepass + depth-test-only colour pass makes the two passes' VERTEX MATHS a correctness contract — textually identical HLSL compiled twice is NOT the same code.** Through `rsqrt`/`sin`/`cos` (approximate instructions) a different schedule moves the vertex in WORLD space, not one ULP; reverse-Z `GREATER_EQUAL` then rejects the colour fragment → unwritten gbuffer → BLACK that flickers with tiny camera moves. **Fix by SHARING the compiled VS** (a PS input signature only has to be a SUBSET of the VS output). ★ Diagnose with two PSO twins: depth-write OFF and depth-test ALWAYS — if either clears it, no shader edit ever will.
- ★★★ **A visibility test added to ONE pass and not its partner writes depth without colour → BLACK.** Any `discard`/`clip`/`SV_ClipDistance` belongs to a FAMILY (prepass, colour, shadow, envprobe, reflection) and must land in all of them in the same edit. Bitten twice. ★ **Diff the pass pair's vertex shaders** before theorising. ★ A risk written in a design doc is not a control — 3.00 shipped the exact failure `DESIGN_FAR_TREES.md` §6.2 predicted.
- ★★ **Resource creation belongs on the MAIN THREAD, never inside a render callback.** Textures + CopyTexture inside `customDraw_*` (a job thread), bound in the same command list, AV'd in `DescriptorBinder::flush` twice. Split into an INIT and a GATE, with the ready flag set at the END of the upload.
- ★★★ **Wicked DX12 compiles PSOs lazily at BIND time and `PipelineStateDesc` stores POINTERS** — a PSO built from stack locals compiles from destroyed memory. Snapshot every state into per-PSO storage.
- ★★★ **fp16 is THE recurring bug class in this inch-scale world** — half tops out at 65504 = 1.66 km. Hit three times (2.07g light range², 2.32 RenderBatch distance, 2.89 probe parallax). `min16float` IS real fp16; `GetRange()`/`GetRadius()` are fp16 too.
- **DX11→DX12 ports must audit EVERY `UpdateBuffer`** — one shared buffer updated between consumers = a silent, validation-clean race. **Custom draw hooks must restore the pass's implicit binding contracts** (camera CB, common resources); slot-POINTER corruption is invisible to content instruments, only shader tints convict it.
- ★★ **A PAIRED accessor where only ONE half got the DX12 bridge is a bug template** — `GetImagePointer` (DX11-only → NULL) vs `GetImagePointerView` disabled EVERY screen-editor image (2.37). **Symptom "loaded but invisible"** → prove LOADED separately from VISIBLE.
- ★★ **DX12 pick tests the CACHED `aabb.layerMask`, not the live LayerComponent** — transient swap-pick-restore is a silent no-op (2.48). For any transient wicked state ask: who reads the cache vs the component? [[project-layer-cache-pick]]
- ★★ **A superset bound is safe AS A BOUND, unsafe AS A POSITION** — enlargement is monotone through frustum/overlap tests; a centre fed to distance/plane/sort/projection is not. Measure the affected content (`DUMP_BIGAABB`), don't reason about it.
- ★★ **Cached local shadows only invalidate on a caster that MOVED; a REMOVED caster is invisible to that test** — call `wi::renderer::InvalidateLocalShadows()`, hooked on the deferred CONSUMER (`destroyme`), not the requester. [[project-shadow-system]]
- **A field added to a Wicked component struct MUST ride its `Serialize`** — `Entity_Duplicate` is a serialization round-trip. **GGMAX bits in upstream FLAGS enums live in reserved range 24-31** ⚠ NOT `options_stencilref` (bits 24-31 = stencil ref).
- ★★ **Texture streaming is ON by default** — gate = per-load opt-in && **editor only** (`gameisexe==0`) && plain `DDS ` magic. Residency is per-MATERIAL mip feedback → no static/dynamic distinction. ⚠ task #37's "DEFAULT OFF" title is WRONG. Repo `STREAMING_STATUS_2026-08-18.md`.
- **SVT atlas default is 12288** (engine `90375285`) — 8192 IS DEAD (starves). Judge from `VT: free=`, not screenshots.
