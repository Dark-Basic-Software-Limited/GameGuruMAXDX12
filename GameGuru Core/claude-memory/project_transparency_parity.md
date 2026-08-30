---
name: project-transparency-parity
description: "DX11-vs-DX12 transparency render-state parity — the double-sided no-depth-write rule that fixes character hair, and what else the DX11 fork did differently"
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-09T03:23:45.910Z
---

# Transparency render-state parity (DX11 fork vs DX12/upstream Wicked)

**2026-08-07: CHARACTER HAIR FIXED** — engine `8a79503b` (2.08) + game `78a5d2e7`. Awaiting the user's eye.

## The rule that was dropped in the port

DX11 fork `D:\max\WickedRepo\WickedEngine\wiRenderer.cpp` ~3736 (READ ONLY) carries a
GGREDUCED-only block whose own comment names it:

> "if mesh is double sided, probably hair or leaves, so ensure NO DEPTH WRITE happens to mess up coverage!"

For **double-sided TRANSPARENT** geometry it did two things upstream does not:

1. forced `BLENDMODE_ALPHA` → `BLENDMODE_ALPHANOZ`, whose depth-stencil state is
   `DSSTYPE_DEPTHREAD` — depth test on, **depth write OFF**;
2. drew back faces (cull FRONT) then **front faces only** (cull BACK). Upstream draws back
   faces then BOTH faces, so every back face blends twice.

DX12/upstream puts every transparent in `DSSTYPE_TRANSPARENT`, which writes depth. That is
upstream commit `c3e8d2df` "transparent render pass allows equal depth test" — a design
choice, **not a fork regression**, so do not go looking for a GGMAX bug that broke it.

## Why depth write eats hair

Transparents are sorted back-to-front **per object, never per triangle**. Inside one hair
mesh the cards draw in index order, so a near card drawn early writes depth across its whole
footprint and every card behind it fails the test → card-shaped bites with the scalp showing
through, and a paler overall mass because fewer layers accumulate.

`alphatest=0` on the hair material makes it maximal: even alpha≈0 texels of the card QUAD
rasterise and occlude. Alpha test would at least have punched the empty texels out.

## The fix (engine 2.08)

- `DSSTYPE_TRANSPARENT_NODEPTHWRITE` = `DSSTYPE_TRANSPARENT` with `depth_write_mask=ZERO`.
  Test + stencil deliberately unchanged so outline/SSS stencil bits and sort order are
  untouched. Appended at the END of `DSSTYPES` (upstream grows that enum by insertion).
- `ObjectRenderingVariant.nodepthwrite` bit; permutation built ONLY for MAIN + transparent
  blend + cull BACK/FRONT.
- `RenderMeshes` uses the DX11 face pairing, with a one-frame fallback to the upstream pair
  if a lazy PSO has not compiled.
- ⚠ The `blendmode != BLENDMODE_OPAQUE` guard is load-bearing, not defensive:
  `GetObjectPSO` indexes an `unordered_map` with `operator[]`, so requesting a permutation
  LoadShaders never built would **INSERT from a render thread**.

Knobs: harness `SET_HAIRDEPTH 0|1` (fully live — no reload, no early-parse trap, unlike
`lazypso`), `setup.ini hairnodepthwrite=0` to revert persistently.

## Scope — what is and is not affected

Only genuinely alpha-BLENDED materials. `MaterialComponent::GetFilterMask()` returns
`FILTER_OPAQUE` whenever `userBlendMode == BLENDMODE_OPAQUE`, so **alpha-tested opaque
foliage (trees, GGTrees) is untouched** — verified by a wide-view diff of 0.386% of pixels,
all invisible sub-pixel conifer edges. GG glass and water carry `customShaderID >= 0` and
take the custom-shader branch in `RenderMeshes`, never reaching this code.

In `spotshadowtest.fpm`: 190 double-sided-transparent subsets — 36 hair, plus weapon
textures (m67, flamethrower, plasma, rpggrenade), barbed wire and fire planes. All of those
also got ALPHANOZ under DX11, so this is parity, not a new look.

## The instrument

`DUMP_TRANSPARENTS [name-substr]` → `Files/transparents_dump.txt`. It is what NAMED the
culprit instead of reasoning about it: `adult male hair 10_color.dds`,
`blend=1 dsided=1(mesh=1,mat=0) alphaRef=1.000 alphatest=0 filt=0x2(TRANSP)`. Reports the
EFFECTIVE `GetBlendMode()` (not `userBlendMode`) because a material can be pulled transparent
by its filter mask alone. See [[feedback-instrument-before-theory]].

## Verification recipe

`OPEN_PROJECT TESTPRO2` → `CLICK_ONLY_LEVEL` → the user's own saved camera
`SET_CAMERA -4213.04 248.20 -10131.70 36.95 -765.98` frames the kneeling soldier at
(-4241,165,-10106). A/B with `SET_HAIRDEPTH`. **POLYS must stay bit-identical** (178839
here across four arms) — the poly counter skips the doublesided backside re-draw, so a
changed number means a pipeline went missing. FPS flat.

## 2026-08-07 later: the SECOND port gap — `BLENDMODE_FORCEDEPTH` (first-person weapon)

**CLOSED, USER-CONFIRMED 2026-08-07** ("I have tested manually, it works great") — engine
`601af547` (2.09) + game `9f7549d2`.

Same family, found the same way. DX11 gave `DisableObjectZDepth` objects
`userBlendMode = BLENDMODE_FORCEDEPTH`: one field meaning BOTH "transparent pass" AND
"stamp my own depth with compare ALWAYS". DX12 mapped it to `BLENDMODE_OPAQUE`, keeping only
`SetDoubleSided(true)` — so a crate the player stands against cut the front half off the pistol.
Restored as `GG_FORCEDEPTH` (material flag, reserved bit 24) + `ggdepthmode:2` variant field.

★ **The harness CANNOT drive the player in a test game** — three approaches failed: `PRESS_KEY`
is a 2-frame `WM_KEYDOWN` the DBP input layer ignores; Lua `SetGamePlayerControlCx/Cy/Cz` writes
are overwritten by the controller the same frame; and the offending crate was static geometry, not
a scriptable `g_Entity`. The answer was to make the fix LIVE-TOGGLEABLE (`SET_WEAPONDEPTH`) and ask
the user to walk into the crate once — one walk then yields both A/B arms at an identical pose.
Do not burn time re-attempting player automation; see [[feedback-dont-thrash-on-automation]].

### ★ THE LESSON: DX11's carve depended on PASS-LEVEL ordering, not on the two draws

DX11 ran `iDoubleRender` — two FULL LOOPS over the batch list, so **every** FORCEDEPTH mesh
stamped before **any** of them drew. The gun and the arms are separate FORCEDEPTH meshes.
Compressing that into adjacent per-subset draws breaks mutual occlusion, and it failed TWICE
in the same shape before I saw the pattern:

1. stamp + front per subset → gun's ALWAYS **depth** write wiped the glove's depth → grip over glove;
2. stamp moved to prepass, back-face colour fill with ALWAYS → gun's **colour** painted over the
   glove. Identical fault, different buffer.

Final shape — three draws, two passes, and the pass boundary is what enforces the ordering:
- **Z-prepass**: back faces, `DSSTYPE_WRITEONLY` (write + ALWAYS). Also restores the weapon to
  occlusion queries, light shafts, velocity/TAA and SSAO. DX11 has this pass too, and its comment
  names it: *"write depth for transparent objects that are solid (opaque=100%) like guns"*
  (WickedRepo RenderPath3D.cpp:1056), admitted via the `alphaRef == 0.01f` sentinel the weapon
  materials still carry. **I first dismissed that branch as dead code** after grepping the DX11
  *game* source for `0.01f` and finding nothing — while the live `DUMP_TRANSPARENTS` had
  `alphaRef=0.010` on every weapon material the whole time.
- **Main**: back-face colour fill, `DSSTYPE_DEPTHREAD` (read, no write, GREATER_EQUAL — NOT
  ALWAYS), then front faces. The fill exists because the opaque pass runs `DSSTYPE_DEPTHREADEQUAL`:
  any world pixel the stamp overwrote fails EQUAL and is never shaded, so back-face coverage
  exceeding front-face coverage would leave the clear value. DX11 got the fill free (its stamp pass
  wrote colour).

### ★ MEASUREMENT RULE for anything first-person

The weapon idle animation moves ~0.9% of pixels over an 8-luma threshold between any two frames.
That is LARGER than the carve's effect at a pose where nothing occludes the gun. **Always take a
same-knob control** (two frames, knob unchanged) and compare the A/B against it. Reading an
OFF-vs-ON diff on its own gave a false positive AND a false negative in one session.
See [[feedback-instrument-before-theory]].

### Other landmines

- `WickedCall_SetMeshTransparent` CLEARS `GG_FORCEDEPTH` deliberately — DX11's single field made
  this last-writer-wins, which is why the gun carves (`DisableObjectZDepth` last,
  G-Gun_part3.cpp:735) and LUA 3D prompt planes do NOT (`SetObjectTransparency` last,
  M-LUA-General.cpp:390). A guard pinning the flag would make those planes — single quads whose
  front faces point away from camera — stamp depth and draw nothing.
- `GetObjectPSO` is now **find-only** (+ `SetObjectPSO` for the LoadShaders safe point).
  `wi::unordered_map` is `ska::flat_hash_map`, open addressing: `operator[]` inserts, a rehash
  frees the entries array, and RenderMeshes holds two `PipelineState*` at a time.
- `setup.ini weaponforcedepth=0` is a true revert; harness `SET_WEAPONDEPTH 0` is only an A/B
  lever (materials are set up once at gun load).

## 2026-08-09: the THIRD gap — `WEAPON_SHADOW` — ★ PORTED (engine 2.14 `14307cad` / game `35ec005c`)

The shadow half of the weapon look. DX11 pulled the shading position to ⅓ camera distance at the
top of DirectionalLight/PointLight/SpotLight so a gun clipping into a wall is not shadowed BY it.

★ **The estimate in the old note was wrong** — it assumed a new `SHADERTYPE_WEAPON` +
`SHADERTYPE_BIN_COUNT` 12→13 + a permutation. Not needed. Done instead as a material option bit +
a wave-uniform Surface bool, so **no new SHADERTYPE, no BIN_COUNT bump, no extra permutation**.
(The recompile happens anyway — `lightingHF`/`surfaceHF` are included nearly everywhere — so the
shader count was never the real discriminator; the enum growth was, because `SHADERTYPE` is
upstream-owned AND serialized into materials.)

★★ **THE SELECTOR WAS ALREADY BUILT.** DX11 set `BLENDMODE_FORCEDEPTH` and `SHADERTYPE_WEAPON` on
*adjacent lines of the same function* (DX11 `wickedcalls.cpp:3336-3337`), so the two features cover
exactly the same object set — including the 276 non-weapon FORCEDEPTH objects, which got it under
DX11 too. The `GG_FORCEDEPTH` flag from 2.09 is therefore the faithful selector; zero new tagging.

★★★ **TWO LANDMINES CAUGHT BEFORE THEY SHIPPED, both by reading rather than assuming:**
1. **Bit 24 would have corrupted the stencil ref.** The field is `options_stencilref` and
   `GetStencilRef() = options_stencilref >> 24u`, so the usual GGMAX "reserved range 24-31"
   convention is WRONG for this field. Used bit 23 (top of the free window 12-23; upstream appends
   from 0, now at 11). ⚠ Generalise: check what else shares a field before claiming a reserved bit.
2. **`Surface::init()` does NOT initialise `material`.** hairparticlePS, emittedparticlePS_soft,
   impostorPS, oceanSurfacePS, objectPS_voxelizer and the raytracers all `init()` a Surface then
   light it — reading material bits in the light loops would have been uninitialised memory,
   randomly yanking grass/particle/ocean shadow positions. Carried as a Surface bool instead,
   defaulted false in `init()`, set in `create()` (which also folds in the live knob, so the light
   loops cost one SGPR bool).

Parity deviations, deliberate: DX12 has no `clip()` to skip (uses `AlphaToCoverage`; weapon
materials carry `alphaRef=0.01` so it was near-no-op), and `light_rect` untouched (DX11 had no rect).

★ **PERF — the method matters more than the number.** Island Showdown read a stable 71.5, vs 88.3
in the 08-08 sweep, and cross-run comparison could NOT attribute that (0801 67.6 / 0806 73.2 /
0807 78.7 / 0808 88.3 — the field is wider than the effect). Settled by **stashing the change,
rebuilding both repos, and measuring the same level on the same rig**: baseline 71.3/71.7 vs
with-change 71.5/71.4 → free. The 88.3 was that sweep's ~1.2 ms/frame ambient drift, i.e. my own
documented caveat. ★ RULE: for a perf question on this rig, a stash-and-rebuild A/B is worth 30
minutes — sweep history cannot attribute anything smaller than ~1.5 ms/frame.

Knobs: `setup.ini weaponshadow=0`, harness `SET_WEAPONSHADOW 0|1` (a TRUE revert as well as an A/B,
unlike `SET_WEAPONDEPTH`). ⚠ **Awaiting the user's walk-into-a-wall confirm — the harness cannot
drive the player.**

### Still open
- Review watch-list not yet exercised: gun `_glass` scope lenses (they get the flag too), light
  shafts against the weapon silhouette, occlusion survival held against a crate >32 frames, and an
  editor pass over the 276 non-weapon FORCEDEPTH objects (AI dot-arc markers are open geometry).
