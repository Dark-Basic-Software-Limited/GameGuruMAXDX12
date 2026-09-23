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

- ★★★ **An extra command list running a Wicked postprocess must bind `BindCommonResources` AND
  `BindCameraCB`** — `RenderPostprocessChain` does both, one line apart. `tonemapCS.hlsl` opens with
  `GetCamera().is_uv_inside_scissor(uv)`; an unset root CBV is undefined in DX12 and answered with
  `DXGI_ERROR_DEVICE_HUNG`. DRED cannot name it (`lastCompletedOp = 0` everywhere, no page fault =
  TDR, not a freed resource) — stage the suspect steps behind a runtime switch instead. (3.83)
- ★★ **`CameraComponent::render_to_texture` renders any scene camera to its own texture**, engine-side,
  for free — reach for it before hand-rolling an offscreen pass. It renders on the NEXT frame, swaps
  its two targets each pass, and needs `camera.scissor`/`canvas` set by the caller (they default to
  zero, which makes every scissor-gated postprocess a no-op). [[project-object-library-preview]]
- ⚠ **`MaterialComponent::GetBlendMode()` returns `BLENDMODE_ALPHA` whenever `FILTER_TRANSPARENT` is
  set, whatever `userBlendMode` says** — clearing `alphaRef` is what actually makes a material opaque.
- ⚠ **The legacy DBP image/bitmap layer is DEAD in DX12** (`m_pD3D == NULL`, `master_part0.cpp:94`):
  `GrabImage`, `MakeBitmap`, `GetBitmapRenderTarget`, `GetBackBufferForGG` can never work here.

- ★★★ **A present-but-EMPTY vertex stream is worse than an absent one, because PRESENT is exactly what
  the generator tests.** The DBO loader pushes a tangent whenever the slot exists
  (`wickedcalls_part0.cpp:766` tests `offsetMap.dwTU[2] > 0` — existence, never content), and
  `wiScene_Components.cpp:727` generates tangents only `if (vertex_tangents.empty())`. A model file
  whose tangent slot is filled with zeros therefore gets 4298 zero-length tangents that the
  generator then trusts. **Degenerate tangent frame → uniform, gradient-free, light-immune BLACK**:
  TBN collapses, the normal-mapped normal dies, N·L = 0 for every light from every direction, and
  texture / UVs / vertex normals / base colour / material all measure perfect. A lit level hides it
  behind ambient and IBL; only a (0,0,0)-ambient view makes it obvious. **Validate the CONTENT of a
  stream, not the slot** — and 3.84c's fix is in the loader, so it applies to every mesh the engine
  loads. ★ The probe that splits this space in ONE run is **force the material UNLIT** (unlit
  bypasses the TBN entirely): torso 26.4 → 59.3 proved the whole bug was the lighting term. Run it
  FIRST, not ninth. ★ And **a COUNT is not a MEASUREMENT** — printing `tangents=4298` beside the
  normal lengths and reading it as health cost another build.
- ★★★ **The shader's light array is packed ONCE per frame from the MAIN camera's cull**
  (`wiRenderPath3D.cpp:473` → `wiRenderer.cpp:6349` `for (lightIndex : vis.visibleLights)`).
  `ComputeTiledLightCulling` touches a per-camera visibility only for an empty early-out
  (`wiRenderer.cpp:12228`). **So a per-camera light count is NOT a count of lights that camera's
  shader can see** — a `render_to_texture` camera reported `visLight=2` for the entire time its
  subject rendered pure black. A light that must reach a secondary camera needs to survive the MAIN
  cull: force its `scene.aabb_lights[idx]` infinite before `UpdateVisibility` runs.
  [[project-object-library-preview]]
