---
name: project-object-library-preview
description: "Object Library live hover preview restored in DX12 (3.83) via the engine's native CameraComponent::render_to_texture, and the four inherited-parameter defects found doing it"
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-22T16:59:55.326Z
---

Hovering a thumbnail in the Object Library replaces the static image with a live rotating render
of the object. DX11 had it; DX12 showed nothing. **Restored 2026-09-22 (3.83), Lee-confirmed.**
Lighting, framing and object release fixed in 3.84; the black-shirt character in 3.84c.

## The shape — the 7th instance

The game-side half survived the port **1:1** (`M-GridEditB_part9.cpp` vs DX11
`M-GridEditB.cpp:19400-20300`, only `Timer()`→`MAXTimer()`). It was running and asking to be drawn
every frame. What died was one function: `GrabBackBufferCopy()` opened with
`if (ImGui_DX12_IsInitialized()) return;` because it renders into legacy DBP **bitmap 99** and
reads pixels back with `GrabImage()`. ★ **In DX12 `m_pD3D` is NULL (`master_part0.cpp:94`), so the
whole DBP image-grab layer is dead** — `GrabImageCore()` returns false on its first line. Anything
that reaches for `GrabImage`, `MakeBitmap`, `GetBitmapRenderTarget` or `GetBackBufferForGG` can
never work in this build. Same shape as [[project-customdraw-hook-inventory]] and
[[project-tree-sway-and-paths]]: nothing broke, something MOVED, nothing told anyone.

## ★★★ The engine already had the feature: `CameraComponent::render_to_texture`

Wicked DX12 has what the DX11 fork did not. Give **any scene CameraComponent** a non-zero
`render_to_texture.resolution` and `RenderPath3D::RenderCameraComponents`
(`wiRenderPath3D.cpp:2919`, called unconditionally from `Render()`) allocates the targets, culls,
prepasses, tile-culls lights, draws opaque + transparent + sky and mips it — every frame, into its
own texture. `resolution = {0,0}` frees the lot on the next pass.

**Reach for this before hand-rolling any offscreen scene render.** It cost zero engine changes.
Things it does NOT do, which the caller must: set `camera.scissor` and `canvas`, tonemap the HDR
result, and bind the result to whatever wants to display it.

⚠ It renders on the frame AFTER you ask. Anything the setup code tears down in the same call —
object position, backdrop visibility, lights — is already gone by then.
⚠ It SWAPS `rendertarget_render`/`rendertarget_display` each pass; read the handle fresh.
⚠ Give the camera entity **no TransformComponent** or `Scene::RunCameraUpdateSystem`
(`wiScene.cpp:5843`) overwrites Eye/At/Up from it every frame.

## ★★★ The recurring mistake: a preview is its own scene — FIVE instances now

**Ask this FIRST of any preview defect: which level-scoped value is it inheriting?**
exposure · near/far planes · scissor · blend mode (3.83) · field of view (3.84) ·
**animation throttling (3.94)**.

3.94: the preview parks its object ~39,000 units above the EDITOR camera, and every animation
throttle measures from that camera - so Reduction Scale held the preview character to one pose
every 98 frames at the shipped default, and 4 seconds at scale 100. **The distance that matters
is to the PREVIEW camera (~324 units).** Fixed by gating both throttles on
`GGObjectPreview_IsActive()` at their single publish point in `MasterRenderer::Update`, which
runs before the animation jobs. ★★ **Chosen over a per-armature exemption list because it
stores NOTHING per object and therefore cannot leak - when a fix would need teardown on N paths,
look for the formulation with no teardown at all.** ⚠ Zero the reduction scale, not just the
30fps flag: the GPU skinning skip and the 3.92 streamout gate read the scale directly.
★ Not caused by 3.93, EXPOSED by it - before, the parts sat on different phases so it LOOKED
animated while being incoherent.

### The original four (3.83/3.84)


**Every parameter it inherits from the level is a parameter that can be wrong for it.** Four
defects, all this:

- **exposure** — inherited 0.145 from a night level and crushed the preview to black. A library
  thumbnail is a product shot on fixed lights: grade it on its own fixed exposure (1.0). The
  full-scene path survives dark levels only because it also runs eye adaption.
- **far plane** — the backdrop plane is parked 21200 units out; the preview camera now owns
  near/far (1 / 60000) instead of borrowing the level's.
- **scissor** — `CameraComponent::scissor` defaults to all zeros and `scissor_uv` derives from it
  (`wiRenderer.cpp:13324`). Any postprocess opening with `is_uv_inside_scissor` returns on every
  thread. Silent, total, and looks like a different bug.
- **blend mode** — `CreateBackdropObject` does `SetObjectTransparency(obj,1)`, so the material
  carries `FILTER_TRANSPARENT` and **`GetBlendMode()` returns `BLENDMODE_ALPHA` whatever
  `userBlendMode` says**; the backdrop images have no usable alpha so a fully resident texture
  blended to nothing. Clear `alphaRef`, don't just set `userBlendMode`.

## ★★★ `BindCommonResources` without `BindCameraCB` = DEVICE HANG

Any extra command list running a Wicked postprocess must bind **both**, as
`RenderPath3D::RenderPostprocessChain` does one line apart. `tonemapCS.hlsl`'s first statement is
`GetCamera().is_uv_inside_scissor(uv)`; an unset root CBV is undefined in DX12 and this driver
answered with `DXGI_ERROR_DEVICE_HUNG`. DRED could not name it (`lastCompletedOp = 0` on every
list, **no page fault** — a TDR, not a freed resource).

## 3.84 — the four follow-up defects, three of which were ONE bug

A character rendered a **pure black silhouette** against a correctly-lit backdrop, and exposure 64
and thumb-light intensity 5000 both changed nothing: it received exactly zero light. Cause: the
preview's lights sat ~39,000 units from the editor camera, so the MAIN cull dropped them and they
were never written into the frame's entity array — see the light-array rule in
[[project-rules-rendering-dx12]]. Fixed by `GGObjectPreview_PreVisibility()`, called from
`Master::Update` right after `WickedCall_UpdateCharacterShadows()`: same slot, same reason — after
`Scene::Update` rebuilds `scene.aabb_lights`, before `PreRender`'s `UpdateVisibility` reads them.
No engine change.

Also: the camera was **1.39× too close** because `GrabBackBufferCopy` passed the LEVEL's FOV where
DX11 rendered thumbnails at the editor's fixed 45, which every distance constant in that function
was fitted against — the fourth inherited-parameter defect. And a new hover adopted an object
without giving the previous one back, so two sat superimposed; only a **fast flick between adjacent
thumbnails** reproduces it, because a normal move crosses a gap that ends the preview cleanly.

★★ **Bracket the knee before reading a flat line as "no effect".** `intensity = k·d²`; the first
sweep ran 1752 → 525,000 candela, entirely above saturation, so a working knob read as broken.
★★ **Measure p99 over the SUBJECT, not the mean over the cell.** Lee's DX11-vs-DX12 shots were
mean 66.8 vs 67.6 — indistinguishable, the backdrop swamps it — and p99 145.6 vs 220.1.

⚠ **`visObj=7`** — the preview camera reports seven visible objects where subject + backdrop is
two, on the FIRST hover of a fresh launch, so it is not the parking leak. Unexplained, recorded.

## Method that paid

- ★ **Stage the suspect steps behind a RUNTIME switch** (`SET_OBJPREVIEW 0..3`) when each failure
  costs a relaunch. 1 clean → 2 hangs: one run, one culprit, one build instead of three.
- ★ **A probe that takes one screenshot cannot test a live preview.** The motion IS the feature;
  take several spaced shots and require that they DIFFER.
- ★ A 0% CPU "hang" with the process Responding is a **modal** — see [[project-testgame-freeze]].

## Files

`Guru-WickedMAX/master_part2.cpp` (the module) · `M-GridEditB_part7.cpp` (selective DX12 gate) ·
`imgui_gg_dx12_bridge.cpp` (`ImGui_DX12_BindPreviewTexture`) · `CImageC_part1.cpp`
(`GetImagePointerView` override — the single UI hook) · `tools/objpreview_probe.sh`.
Notes: `NIGHT_INVESTIGATIONS_2026-08-12.md` §3.83.

## Still open

Thumbnail **generation to disk** and snapshot mode remain disabled in DX12 (the particle HOVER
preview was wrongly grouped with them - restored 3.110, see below) —
they need a GPU→CPU readback that does not exist yet. Only bites imported/user content; stock
content ships its thumbnails. The preview render target is now exactly what a readback would read.

## 3.97 / 3.98 (2026-09-24) - the preview owns its environment; a DX12 lighting stage

- **3.97**: every render-to-texture camera drew with the MAIN view's FrameCB (ambient, sun, fog,
  global probe, local probes). Engine hook `wi::gg_rtt_frame_override` gives the preview camera a
  COPY; `GGObjectPreview_FrameOverride` replaces every level input and swaps in a generated 32x32
  studio sky cube. Also: the old `k*d^2` key assumed inverse-square falloff this fork does not have
  (2.10 DX11 window falloff) - large objects got 365 units and went white. Now a fixed energy.
- **The DX11 thumbnails are NOT one target**: at matched angles, buildings need 0.7x and props 1.3-1.5x.
  Lee decided: stop chasing them, build a DX12 lighting stage.
- **3.98 stage**: key/fill/rim/bounce placed around the SUBJECT in the camera frame
  (`gg_objpreview_stage`, `SET_OBJPREVIEW_STAGE`), plus softboxes in the studio cube. Defaults
  PROVISIONAL until Lee judges them. Harness: `SET_OBJPREVIEW_LIGHT k abs fov studio amb env mod freeze`.
- The Pistol Ammo black contents were NOT lighting - see [[project-rules-rendering-dx12]] (skinned
  normal in float). `visObj=7` answered: subject + backdrop + the editor's 5000 km floor box.
- **3.99**: the full-size preview REBUILDS the backdrop under the SAME object id, so 3.83's id-keyed
  fix skipped it; the transparent plane was overdrawn by the sky wherever no ground lay behind it.
  Now the material STATE is checked every call. ★ A guard keyed on an ID cannot see a rebuild under that ID.

## 3.110 (2026-10-12) - the Particle Library rides the same preview

- ★ **The engine's RTT pass draws NO emitters and NO customDraw hooks**, and a WPE emitter is simulated
  for the BOUND camera (simulateCS culls + billboards against it) - so a WPE effect needs its own
  `UpdateGPU` with the preview camera bound, exactly once per frame: hide it from the main view
  (`SetVisible(false)`) or it simulates twice. Done on the main thread after Render's wait, on the
  preview's own list, onto mip 0 before the tonemap (`GGObjectPreview_DrawParticles`). No engine change.
- ★★ **An early return that skips a per-call CLEAR turns a request into a latch.** GrabBackBufferCopy's
  DX12 return skipped `BackBufferParticlesMode = false`; opening the Particle Library latched it, froze
  the library's hover code (wait counter re-armed per item) and killed the Object Library preview.
- A preview keyed on an image ID needs a UNIQUE id per item (uncached items share the default icon),
  and ImgBtn derives the ImGui ID from the image id - swap one, keep the other stable.
- 'Black backdrop.dds' is a dark gradient with a glow, not black.

## 3.111 (2026-10-12) - thumbnail GENERATION via readback of the preview

- The live preview's tonemapped RGBA8 (`s_ldr`) IS the DX11 grab. Generation = arm a one-shot request through
  the live path, HOLD until `GGObjectPreview_RenderedSinceArm` >= N (+ streaming epoch stable for objects),
  `GGObjectPreview_CaptureLDR` (wi::helper::saveTextureToMemory - submit + WaitForGPU, so BETWEEN frames only:
  GrabBackBufferCopy), write .jpg + 288 icon .png with `WickedCall_SaveRGBA8Region`, library reloads the item.
- ★ A multi-frame hold changes every assumption a one-call DX11 grab made: the library must not entity_load
  into slot 0 meanwhile (bDoneOneThumbPerSync = g_bThumbGenInFlight), the full preview must cancel it, the
  reload must survive folder changes (queue), and Finish must clear only its own request.
- ★ A loop guard must not outlive its reason: block a written name only while the file exists (Update
  Thumbnail deletes it on purpose), and per write area.
- Particle library loads EVERY item (bReleaseIconsDynamic false): opening it generates all uncached effects.
- Bare 'Black backdrop' particle capture = mean 8.0, no channel > 50: weather/birds draw nothing at preview
  scale - keep the icon.


**3.121 (2026-10-17, notes §3.121, checklist 251, game 6fddd1d2 / engine b23777a4, LEE-CONFIRMED 2026-10-17 "Live preview isolation confirmed working") - the 3.97 studio had NEVER reached a draw.** DrawScene / DrawSky / tiled light culling / light visualisers call BindCommonResources on entry and re-bound the LEVEL frame CB over the one-shot copy; studio 0 vs 1 measured identical, s_studioFrames counted CALLS. Lee saw it as weather fog turning the Particle Library tile pale blue (backdrop 21200 out, 95% fogged). Fix: engine per-list override `GG_SetFrameCBOverride` honoured by BindCommonResources (+ Super Quick and distance fade skipped for such a list); game `GGObjectPreview_ApplyStudio` shared by the engine job and the game particle+tonemap list, also replacing sky (except the item's own fpe cube), wind (+ a zero 3D wind texture: the gust field carries level speed/gust), aerial perspective, forces and the lit-WPE light (gg_objpreview_particlelight); Particle Density exempts g_LibraryPreviewWPERoot children. Studio ambient 0 -> 0.6 (what Lee approved the 3.98 stage under; p90 best fit to the cell thumbnails). Probe: `PERTURB_SCENE` + `tools/harness/preview_isolation_probe.sh` - ALWAYS include the studio-OFF control, or a dead instrument reads as a pass.
- ★ An override bound ONCE is only as good as the first callee that re-binds the slot - bind it where the callee binds.
