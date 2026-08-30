---
name: project-light-falloff-parity
description: "DX11 light power parity (engine 2.10) — point/spot lights use the DX11 product curve energy 30 x (1-d2/r2)^2, closing the ~half-strength complaint and task #109; SET_LIGHTFALLOFF A/B, lightfalloff=0 revert"
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-07T20:27:05.465Z
---

# Light power parity (engine 2.10, game `0a869fe8`) — 2026-08-07, awaiting user eye

USER-REPORTED vs their own DX11/DX12 spotshadowtest editor shots: "light power is about
half of what it should be". This is task #109's prediction landing — the `range²×π/4`
intensity compensation in `WickedCall_UpdateLight` was tuned while the 2.07g fp16 overflow
had DELETED the attenuation window.

## The three facts that decided the design (all verified in source, not agent hearsay)

1. **DX11 ran EVERY GG map light at constant energy 30** — set once in `WickedCall_AddLight`
   → `Entity_CreateLight(...,30,500)` (DX11 wickedcalls.cpp:6655); `UpdateLight` never
   touched energy; CPU packed it raw fp16 (WickedRepo wiRenderer.cpp:5092). Range alone
   shaped the curve. Editor camera/thumb lights: energy 8.
2. **DX11's curve is `(1-d²/r²)²` with NO inverse-square term** (WickedRepo
   lightingHF.hlsli:621); spot adds a LINEAR-in-cosine ramp to dead center,
   `saturate(1-(1-SpotFactor)/(1-coneCos))`, never squared (:700). No single intensity
   scalar can turn upstream's windowed 1/d² into that SHAPE — the user's shots show it:
   hot pool at the source vs broad even flood.
3. **The old comment's premise was FALSE: DX11's BRDF DID divide by π** — its
   `BRDF_GetDiffuse` is a fork early-return of constant 1/π (WickedRepo brdf.hlsli:449;
   the stock Fresnel line below is dead code). Modern DX12 applies the same single 1/π at
   `ApplyLighting` (`direct.diffuse / PI`), and both spec D terms carry their own. So with
   the DX11 curve in the shader, **intensity is in DX11 energy units 1:1** — no π factor
   anywhere, diffuse AND specular parity from one constant. ★ I first "corrected" the
   attenuation with a folded 1/π and caught it only by reading ApplyLighting — check where
   BOTH engines put their normalization before adding compensation constants.

## Shape of the fix

- Engine: `OPTION_BIT_GG_DX11_LIGHT_FALLOFF` (bit 30, GG reserved range) from
  `wi::renderer::gg_dx11_light_falloff` (default ON); branch INSIDE
  `attenuation_pointlight`/`attenuation_spotlight` (lightingHF) so every consumer follows:
  surface, volumetricLight_*, raytraceCS, ddgi, surfels, renderlightmap. Spot branch skips
  ONLY the modern squaring — GG never sets innerConeAngle (default 0 → cos 1), so the packed
  angle_scale/offset already reduce to DX11's exact linear term.
- Game: `UpdateLight`/`AddLight` push intensity 30 under the knob; the old heuristic is kept
  verbatim as the `setup.ini lightfalloff=0` revert arm. Sun/directional untouched (int 15.5).
- The DX11 curve reaches exactly ZERO at range → the 2.07g tile-cull truncation class
  cannot return under this curve.
- `SET_LIGHTFALLOFF 0|1` is FULLY live in one knob: shader bit is per-frame FrameCB AND
  lighting_loop re-pushes every light's intensity through UpdateLight each frame reading
  the same bool. Verified: LIST_LIGHTS flips 30.00 ↔ range²-heuristic (7850 on a range-100
  point) within a frame.

## Verified live (spotshadowtest, TESTPRO2)

27/28 lights at int=30 (28th = sun). A/B at the pipes/crate pose, right-third crop (away
from the now-live steam column): effect 26.4% pixels / mean|dL| 7.7 vs same-knob control
floor 4.8-7.1% / 1.4-2.9 — 4-5× above animation noise, mean luma UP with the knob on, and
the OFF arm visibly reproduces the user's "hot pool at the bulb" complaint. FPS 110 flat.

⚠ Look change is GLOBAL (every level, every point/spot). The user's eye is the final gate;
if a level was hand-tuned against the old DX12 falloff it will now read brighter/longer.
Related: [[project-shadow-system]] (the 2.07g overflow saga), [[project-gpup-particles]]
(same session), [[feedback-instrument-before-theory]].
