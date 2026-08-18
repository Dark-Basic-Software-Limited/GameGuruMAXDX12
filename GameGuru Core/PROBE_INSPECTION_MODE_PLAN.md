# PROBE INSPECTION MODE — the post-compact plan (drafted 2026-08-18 ~06:00, Lee-approved direction)

## Goal
ONE clean debug mode — `SET_PROBEVIEW <0|1>` + setup.ini `probeview` (int passthrough!) —
that turns any %probe marker into a TRUE MIRROR of a chosen, clean cube. It reproduces the
22:48 "perfect panorama ball" (the 2.79 env-only mode-1 view) as a per-marker feature instead
of a scene-wide shader override. Default OFF, zero cost when off. From there, each real fix
(capture position, filter quality, probe resolution) can be judged against a trustworthy
preview instead of through PBR fog.

## Why the 22:48 view worked (the three ingredients — all reverted, all re-buildable)
That panorama = raw `cube.Sample(surface.R, mip0)` + local probes parked + shell excluded
from captures. The reverted 2.76 build has none of the three. Mip 0 of the cube was ALWAYS
clean — every "corruption" lived in presentation, filtered mips, shell-contaminated captures,
or capture geography (full proof chain: NIGHT_INVESTIGATIONS §2.77-§2.88).

## Part 1 — PRESENTATION: the marker ball becomes a real mirror
The black-void ball was correct physics: a dielectric reflects ~4% head-on. A mirror needs
metalness 1 + roughness 0 + WHITE basecolor with maps stripped (a metal's reflection tint IS
its basecolor — the stock dark circuit texture makes a smoked mirror).
- RECOMMENDED: hide the 'root' shell (`SetRenderable(false)`) + mirror the inner 'sphere' →
  clean single-surface chrome ball. (Mirroring BOTH gives the porthole look — proven working
  05:21, works but busy; probe.dbo's circles are AUTHORED portholes, not corruption.)
- Mechanics proven 05:21 (rewrite from notes — 2.87 was never committed): in
  WickedCall_MakeObjectEnvMatte, per frame-object set the material (metal/gloss/white/strip
  BASECOLORMAP+SURFACEMAP+NORMALMAP, SetDirty) — then find the SEPARATE root-shell object by
  NAME ('root' NameComponent) + proximity (≤6× ball radius). ⚠ Containment walks from the
  marker base point or ball centre BOTH miss the fresh marker (shell box floats above them —
  §2.88); the name-hunt is the proven finder.
- RECEIPT: DUMP_OBJENT on the marker range — both objects must show the intended
  norefl/renderable state before trusting any picture (2.76-era DUMP_OBJENT is gone too —
  reintroduce it or verify visually with a bright test cube).
- Longer-term alternative (content, no code): give %probe a plain-sphere DBO with no shell.

## Part 2 — ISOLATION: choose which cube the mirror shows
- Park the 8 local pool probes while the mode is on → the ball reflects ONLY the global cube.
  Mechanism = 2.78 (game 9b937481): GGTerrain_SetLocalProbesDisabled. ⚠ Landmine: BOTH
  tracking paths must be gated or the pool re-assigns next frame (§2.78).
- Optional second lever (same commit): move the global CAPTURE POINT + rebake
  (SET_GLOBALPROBE mechanics). This is the door to the REAL question — capture-position
  policy. Stock captures at the MAP CORNER + 208 m: the world from there IS an island-disc in
  blank sky, and every curved surface faithfully mirrors that geography. The DX11 intent
  (ground at origin + 30 m) is still commented in GGTerrain_part0. Live sweep results 04:52:
  in-scene 2.5 m = photographs furniture point-blank; +30 m = clean but sky/water-dominated.
- RECEIPT: DUMP_ENVPROBE (survives the revert) — probe[0] pos + decode the DDS.

## Part 3 — CAPTURE HYGIENE: keep the marker out of every capture
Placed-probe captures photograph the enclosing 'root' shell interior from inside — "a circle
image on each cube side". This defect is LIVE again in the reverted build.
- Mechanism = 2.77 (game 4ed42984 / engine 07592075 for the trace) — BUT use the Part-1
  name-hunt as the finder, not the enclosing-point walk (§2.88: the walk missed the fresh
  marker). Exclusion = SetNotVisibleInReflections(true) on 'sphere' + 'root'.
- RECEIPT: SET_PROBECAPTURETRACE-style capture census (engine 07592075) or visual: a settled
  placed-probe cube decoded via DUMP_ENVPROBE must show no shell interior.

## After the mode works — the real-fix queue it unblocks (each judged in the mirror)
1. Capture position policy (Part 2's lever made permanent — per-scene, ground-anchored).
2. Reflection quality: cherry-pick engine b44c3f0e (FilterEnvMap 3-part fix: full-chain
   filter source / read-aligned roughness ladder i/mipcount / HDR band clamp) and decide
   plain-vs-BRDF mips with d5ce3478's one-gate lever (plain 2x2 = DX11 parity look).
3. Probe budget: 128px/4-mip chain is coarse — resolution/mip-count/format (BC6H ring) knobs.
4. Marker art: plain-sphere preview mesh; retire the porthole shell for the preview job.

## Discipline carried over (§2.88 lessons)
- Every rung ships with a VISIBLE receipt before any measurement is trusted.
- ini knobs are INT PASSTHROUGH (the bool-ize class bit twice).
- ⚠ If a CB mode value ever misroutes again (the unexplained gg_envsolid 3/4→2 anomaly),
  build the readback rung (paint the value as colour) FIRST.
- Surviving instruments: DUMP_ENVPROBE, REFRESH_ENVPROBE, SET_DEBUGPROBES.
