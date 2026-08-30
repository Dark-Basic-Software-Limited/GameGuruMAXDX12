---
name: project-env-probes
description: "Env probe architecture (base env map = globalEnvProbe, NOT the sky file), the 2.73 pool re-bake fix, DUMP_ENVPROBE instrument, DX11 reflection-energy parity verdict"
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-18T06:00:48.010Z
---

# Environment probes / base env cube map (task #155, 2.73, 2026-08-16)

## ★★★ #157 "THE CIRCLES" — SOLVED + BASELINE 2026-08-18 (engine 630a245e / game 017c2b43)
**fp16 overflow in the parallax correction, not the cube.** `EnvironmentReflection_Local`
(lightingHF.hlsli) computed the ray-exit `Distance` in `half` (min16float = real fp16, max
65504) **in WORLD units**. `Distance` = half-extent … half-extent×√3, so **any probe OBB over
65504/√3 = 37,820 units overflows to +INF** for every direction outside six ~40° caps around
±X/±Y/±Z. GG's `globalEnvProbe` box is **50,000** (`GGTerrain_part0` globalrange) → six discs of
correct reflection in a field of garbage = the circles. Local probe boxes are 1–few hundred
units, which is the entire reason local reflections always looked clean.
- **Fix**: parallax intermediates promoted to `float` (+ the clearcoat copy). Knob
  `gg_probeparallax` — 0 = stock/defect, 1 = float precision with parallax kept, 2 = magenta
  overflow map, **3 = float + NO parallax on level-sized boxes = the SHIPPING DEFAULT** (Lee's
  call 08-18: parallax against a box enclosing the whole level drags the reflected horizon by the
  surface's offset from the map origin, making the look position-dependent, and DX11 never did
  it). ⚠ the precision fix still applies at mode 3 for probes between room-size and the 37,820
  ceiling. Plus `gg_probeonlyglobal` (ignore local probes) and `SET_GLOBALPROBEBOX`.
  Full derivation + the milestone summary: repo notes §2.89.
- **The decisive observation was Lee's**: shot 1 vs shot 3 — same ball, same material, same
  camera, clean on its LOCAL probe and circled the moment a drag released the pool slot and it
  fell to the GLOBAL one. One property differs between those probes: box size.
- ⚠ **The instrument that misled us for a whole night**: the debug probe sphere (`cubeMapPS`) is
  a RAW MIRROR — no Fresnel, no roughness, **no parallax**, stock mip 0 only. It cannot show a
  read-path defect. Use it for cube CONTENT; never for judging a reflection.
- ⚠ **Measuring on a live scene**: whole-frame screenshot diffs have a 3–5% animation floor
  (water/foliage/clouds) that swamps this effect. What worked: mode 2 gives an exact mask of the
  overflowing pixels, then compare the A/B difference INSIDE vs OUTSIDE that mask — 4.38×.
- Still true and still unfixed: the 2.77 capture defect (a placed probe photographs its own
  'root' shell). Still uncherry-picked: b44c3f0e (FilterEnvMap quality) and d5ce3478 (plain mips).

## Architecture (grep-verified; the part that keeps getting guessed wrong)
- **Reflections NEVER sample the skybank `_cube.dds`.** The raw sky file (`shaderscene.globalenvmap`) is sampled at MIP 0 only — sky backdrop + probe-capture backdrop (engine `skyHF.hlsli:185/189`). Any "corrupt base env map" hunt that starts at the sky DDS is aimed wrong (the 08-04 BC6H sky conversion was proven faithful, mips included; the originals were box-filtered all along; sunny's down face always contained a dark ball on radial rays — authoring artifact).
- **The base env reflection = `GetScene().globalprobe` = probes[0] = GGTerrain's `globalEnvProbe`** (`GGTerrain_part0.cpp:7485`): 128px, 4 mips, BC6H, GGX-filtered capture at world corner (0, terrain-height, 0), refreshed only when `globalEnvProbePos.y` diff > 50 (ClearEnvProbeList zeroes it → re-render). Shader MIP = roughness × 4 (`lightingHF.hlsli:683-732`), scaled by `wi::scene::gg_envprobe_brightness` (1.55 knob).
- **8 `localEnvProbe` pool slots** are created at init and assigned to user probe markers by a tracking system that ONLY runs when `g_envProbeList` is non-empty or in test-game — in an editor level without markers the pool is inert.
- Engine probe textures are created **uninitialized** (`CreateTexture(&desc, nullptr)`) and published as globalprobe immediately — transient undefined-VRAM window at level load (left unfixed, cosmetic).

## The 2.73 fix (game-side only, commit `d271beae`)
The pool baked ONCE at GGTerrain init — **before any level's sun/sky exist** — a twilight cube with a moon-like blob, kept forever. When a marker later grabbed a slot there was a 1-2 frame re-track→re-capture window showing that alien cube = Lee's "circle image on each cube side" flash. Fix: `bUpdateProbes` consumer now SetDirty's all 8 slots per level load / sky change and parks unassigned slots HIGH (terrain height + 20000) so the stored bake — and thus any flash — is clean current sky. Fade-out park (-999999)³ → same high park (it re-bakes there; was a void-black cube). Verified by before/after DUMP_ENVPROBE decode.

## Instruments (WETEST.md has full docs)
- `DUMP_ENVPROBE [radius]` — saves every probe cube + sky cube as .dds into `Files/` + census txt. Decode offline with texconv (scratchpad decode_cube.py pattern: texconv → RGBA8 → PIL face/mip montage).
- `REFRESH_ENVPROBE` — g_bLightProbeScaleChanged + WickedCall_UpdateProbes (editor-identical full refresh).
- `SET_ENVPROBE_BRIGHTNESS <f>` — live `wi::scene::gg_envprobe_brightness`.

## Terrain roughness dry-look floor — EXECUTED per Lee's decision (2.74 `a2494731`, 08-16)
Lee chose the CONTENT lever: 11 of 32 terrain mats with roughness mean < mat4's 175/255 lifted to exactly 175 (additive G lift, `tools/terrainroughness.py`, format/mips preserved, verified ±3). mat2 beach sand 41→175, mat31 33→175 the big two. **Originals mirrored no-clobber at `D:\max\mipbackup\terraintextures_buildarea` = the "maximum energy" custom set** (revert = copy back). Pipeline proven healthy end-to-end after a false "surface map disconnected" lead (see below).

## ⚠ Terrain-gloss A/B verification landmines (cost 4 null experiments, one false conclusion)
- **Island Showdown is the WRONG level to eyeball sand roughness**: its visible beaches are mat8/mat18-class (authored roughness ~250, already dry); mat2 is 72% of the PAINT map but not the visible beach. Whole-frame diffs read as noise (0.1-0.6%) through THREE A/Bs including a chrome (roughness 0) mat2 swap.
- **A dielectric chrome patch is INVISIBLE top-down** — F≈f0≈0.5% at normal incidence; gloss A/Bs need grazing views.
- **Prove the camera faces the target** — two "grazing" cameras faced away from the painted patch (SET_CAMERA yaw convention unverified).
- The conclusive recipe (WETEST DUMP_TERRAINSURF row): PAINT_TEST a mat2 patch, camera INSIDE it (y≈75, pitch 12), all 4 yaws → chrome-vs-lifted = 3.4-5.7% pixels, visibly sheen-vs-matte.
- The SVT surface atlas BAKE is correct (marker-instrumented CS: 100% sampled path, correct 2048² textures; diag fully reverted, engine repo clean). `DUMP_TERRAINSURF` dumps live atlases + chunk material slots.

## DX11 sand/reflection parity verdict (read-only forensics, for Lee's decision)
DX11 carried **two deliberate GG-local energy cuts** (upstream code commented out in place): (a) `envColor *= 0.5*metalness+0.5` — dielectrics get HALF env reflection; (b) crushed grazing fresnel `f90 = max(1-roughness^x, f0.r)` — no wet sheen on rough surfaces. Plus f0 0.02 vs DX12's 0.005. DX12 = stock upstream (EnvBRDFApprox, f90=1, no damp) → beach sand ~1.6-1.7× brighter env term. **Stock sand mat2 is AUTHORED glossy (roughness G≈0.16 = "wet sand"; mat4 0.69, mat8 0.98)** — DX12 renders what the content says; DX11 masked it. Levers: content roughness fix / `SET_ENVPROBE_BRIGHTNESS 0.5` (= DX11 damping on dielectrics exactly) / full emulation (needs f90 crush, dims everything rough).

## ★★ 2.78-2.80 (08-18): the DEBUG RIGS + what they settled + one BLOCKING failure
Rigs (all default OFF, setup.ini keys persist): `SET_LOCALPROBES 0`/`globalprobeonly=1` (park the
pool → whole level reflects the base cube), `SET_ENVONLY <1-10> [mip]`/`envonly=1` (objects output
ONLY the cube; ladder of one contributor at a time), `SET_ENVSOLID <0|1|2>`/`envsolid=1` (flat
colour for every cube read; 2 = SPLIT, one colour per read site), `SET_GLOBALPROBE <x y z|rebake|off>`
(move the base capture — stock is the MAP CORNER, (0,8215,0) = 208 m up), `SET_PROBECAPTURETRACE`.
★★ **SETTLED**: on the ball's disc the global SPECULAR read (`EnvironmentReflection_Global`,
lightingHF:711) owns **100.0%**; ambient **0.0%** (it feeds diffuse and the ball's albedo is
near-black); local box-projected 0.1%. With the solid-colour test flipping the whitish gaps AND
the detailed circles together, **both pixel populations are that one texture read**.
Ruled out: normals (shading vs face normal 0.13% different, field smooth), occlusion (98.7%
survives). Fresnel leaves only 10% (2.7% centre / 5.0% rim) and ambient/sky are flat washes
(201 std 13 / 220 std 7) — all near-uniform, so none of them can carve discrete shapes.
⚠ **A THIRD cube read**: `EnvironmentReflection_Local` (lightingHF:773) also serves the GLOBAL
cube — probes[0]'s descriptor is in the probe ENTITY array (wiRenderer:5688) with range 50000, so
the box-projected path wins for most pixels. It never names `GetScene().globalprobe`, so grepping
that misses it; any "replace the cube" test that skips it is invalid.
★★ **BLOCKED**: the mip rungs (`SET_ENVSOLID 3` paint chosen mip / `4 <mip>` force mip) NEVER
REACHED THE GPU — two failed attempts (engine rebuild + refresh_shaders, then a full game rebuild),
`objectPS.cso` timestamp frozen, and the deploy `.cso` producer is UNIDENTIFIED (not either build;
`SHADERSOURCEPATH` = `../WickedEngine/shaders/` does not exist; the game never calls
SetShaderSourcePath). Notes §2.80c.

## ★★ 2.77 (08-17 late, engine `07592075` + game `4ed42984`): THE CIRCLES ROOT-CAUSED — a %probe marker is TWO objects
Lee's lead cracked it ("a sphere rendered inside a cube; only the part outside the cube is
rendered" — geometrically exact: all six cube faces clip at the same near distance, so the
volume hidden around a probe IS a cube). **Instrument first**: `SET_PROBECAPTURETRACE <0|1> [r]`
(engine 2.77, default OFF) appends one block per CAPTURE to `Files/probecapture.txt` — znear/zfar
+ every object within r with its exact keep/skip reason. First run: znear=3.0 (inherited from the
MAIN camera, `vis.camera->zNearP` — the cube-clip is real but only 3 units), and **the marker
balls were `norefl=0 RENDERED` inside their own probe** despite 2.75b.
`DUMP_OBJENT <start> <end>` said why:
```
obj=71184 f0 'sphere' norefl=1   <- element 1192's obj -> 2.75b excluded this
obj=71185 f0 'root'   norefl=0   <- RENDERED into the capture
obj=71186 f0 'root'   norefl=0   <- RENDERED into the capture
obj=71187 f0 'sphere' norefl=1   <- element 1195's obj -> 2.75b excluded this
```
**A %probe marker owns TWO GG objects**: the element table points at the inner ball (DBO frame
'sphere', r=21.7); a SECOND object per marker (frame 'root', r=28.6) sits 19.9 units away with the
probe INSIDE it. Every capture photographed that shell's interior = the "circle image on each cube
side". Fix: `WickedCall_ExcludeObjectsEnclosingPoint(x,y,z,maxRadius)` from the probe rebuild —
**by GEOMETRY, not object number** (an object ENCLOSING a probe origin can't be captured by it;
radius cap = marker extent ×3 so a room around an interior probe stays in). Verified: all four
marker objects norefl=1, zero root/sphere RENDERED (was 2/capture), preview ball changed 11.6%
(4420/37950 px) — the dark band is gone. ⚠ SETTLED captures proven; the CLICK-HOLD window still
needs Lee's eye (arm the tracer + hold the X widget if anything survives). ⚠ Exclusion is sticky
for the session. Notes §2.77.
★★ **RULE: "the element's object" ≠ "the entity's geometry"** — GG allocates MORE THAN ONE object
per marker and the element table names only one. DUMP_OBJENT the object RANGE before trusting any
per-object treatment on a marker. (⚠ `DUMP_ENTOBJ` returning -1 is NOT proof an entity is
engine-side — that reverse lookup only scans the object short-list.)

## ★ 2.76 FIXED (08-17 late, engine `92df06c7` + game `a42e6919`): picking a probe resized EVERY probe ball
Lee: "click and release on the left probe marker and BOTH spheres increase in size". Both
defects were mine, from 2.75: (1) **`AABB::getRadius()` is the half-DIAGONAL** (sqrt(3) x the
half-extent of a ball's box) and it was scaled a further 1.15 → the preview drew at ~2x the
ball (Lee's pixels: 2.05x). Now `WickedCall_GetObjectWorldExtent` = max half-extent, x1.02
(just covers the mesh, no z-fight); the 2.75 "fallback 40 units" is deleted in favour of the
last-good extent — *a fallback that differs from the real size IS the size-pop bug*.
(2) The engine's `debugEnvProbes` block draws a sphere for **every** probe in the scene —
invisible as 1-unit specks at the stock scale, a level-wide swell once 2.75 sized them.
Engine delta **2.76 `SetDebugEnvProbeFocus(x,y,z,radius)`** draws only the probe NEAREST the
focus (radius<=0 = stock all-probes, used by SET_DEBUGPROBES); the game focuses the picked
marker's x/y/z, which is exactly `probe->position` (AddEnvProbeList copies it, no offset).
⚠ **Nearest-wins, not a radius test** — round 1 used max(extent*3,100) and both balls still
previewed because Lee's two markers sit **63 units apart**.
★★ RULES: "make X the size of Y" wants the HALF-EXTENT, never the bounding-sphere radius
(sqrt(3) too big in 3D, sqrt(2) in 2D — both are called "radius"). And before scaling up
anything the engine draws for ALL of a class, read the loop: harmless at scale 1 ≠ harmless
at scale 60.
Verified on Lee's TESTPRO2/spotshadowtest (probe markers = elements **1192/1195**;
⚠ LIST_ENTITIES reports 0 on that level — every element has `active=0` — so enumerate with
`GET_ENTITY <i>`; `SELECT_ENTITY <i>` does reproduce a settled pick because it sets
`t.widget.pickedEntityIndex`, the field lighting_loop reads): picking either probe leaves
the other **bit-identical (0 changed px**, ambient-controlled by two deselect frames), and
the changed silhouette is 172x118 = the marker ball's own dome. Notes §2.76.

## ⛔⛔ 08-18 05:45: FULL REVERT (Lee's call) — everything below in this file's 08-17/08-18
sections describes CODE THAT NO LONGER EXISTS. Reset to engine 92df06c7 / game a42e6919
(2.76 pair); revert commits game 7c75e6cd / engine 2237a181. Lee: the 10-hour hunt was
"a bit of a wash" — a DIFFERENT DIRECTION comes another day; do not resume the old hunt or
rebuild the scrapped rigs unprompted. Repo notes §2.88 = the complete ledger (every delta
2.77-2.87 with verdicts + the 12 scrapped harness commands + keepers). Surviving instruments:
DUMP_ENVPROBE, REFRESH_ENVPROBE, SET_DEBUGPROBES (pre-2.77). KEEPERS for cherry-pick:
engine b44c3f0e (FilterEnvMap 3-part fix — genuine, verified, engine-wide) + d5ce3478
(plain box mips lever) + the knowledge set (capture geography, authored portholes,
dielectric fresnel, capture-position policy question, the unexplained gg_envsolid 3/4
value anomaly).

## ★★★ 08-18 ~04:00: #157 CIRCLES ROOT-CAUSED + FIXED — FilterEnvMap (engine 2.84 b44c3f0e)
THE ANSWER: the circles live in the BRDF-PREFILTERED MIPS of the probe cube (mip0 was always
clean — checking only mip0 hid them for days). Two compounding causes: (a) the filter SOURCE
had only the probe's 4 mips — computeLod requests source lods ~4-8, every wide ray clamped to
the 16px level → filtered mips collapsed toward one value per face (the "gap" = a sky average
whose colour tracked Lee's cloud slider — his "single rogue pixel" deduction was exactly right);
(b) the capture's 10-20x blown HDR horizon band flooded every wide GGX cone (DX11 captured LDR
so never had the band). Fixes: full-chain source buffers + roughness ladder aligned to the read
(filterRoughness = i/mipcount, was i/(mips-1)) + filterHDRClamp (default 2.0, SET_PROBEFILTER,
0=stock). VERIFIED: ball at mip0 = flawless mirror panorama; mip1 = clean blur; flood dead.
Chain of proof: cube writes clean (face dumps, 0 dead texels) → direction smooth (R-as-colour)
→ normals smooth (facenormal view) → Lee's CARDINAL LOCK (2.83 SET_ENVDIR 5) killed the discs
→ per-tile contrast stretch of mips 1-3 revealed a bullseye inscribed in every face.
★ RULES EARNED: (1) when auditing a cube map, audit EVERY MIP — filtered chains are rebuilt
per level and can carry structure mip0 never had; (2) a BRDF prefilter needs a full-chain
SOURCE even if the shipped texture keeps fewer mips (computeLod assumes one); (3) HDR captures
+ wide-cone prefilters amplify blown pixels into face-wide floods — clamp or capture LDR-parity.
OPEN: (1) %probe preview ball still matte (2.75 — its justifying theory is dead) → reads the
deepest 16px mips → soft discs remain; un-matte = drop WickedCall_MakeObjectEnvMatte, ball
becomes the mip0 mirror. (2) ⚠ SET_ENVSOLID modes 3/4 render as mode 2 — pixel-proven on BOTH
routes, fresh cso, no magenta emitter at w=3 in source ⇒ value/read corrupted in transit for
3/4 while 1/2/5 pass; readback rung needed; modes 3/4 are DEAD instruments until then.
(3) Lee's roughness-pattern question unanswered (mip-paint never ran).

## ★★ 08-18: 2.81 +X FACE WIPE VERIFIED + the "shader edits not live" verdict RETRACTED
Lee's step (his baseline benchmark = spotshadowtest re-saved 08-18, stripped scene, camera on the
ball; he proved via Cloud Coverage slider that the GAP colour tracks CLOUD texels in the cube):
"wipe the +X face black to prove shader access". Shipped `GGEnvWipeFacePX` in lightingHF.hlsli
(engine `7ebc2493`), SET_ENVSOLID mode 5 / setup.ini `envsolid=5` — mode ≥5 AND dir.x>0 AND |x|
dominant → sample returns BLACK, at ALL THREE read sites. VERIFIED COLD from ini alone
(screenshot 00-30-54: clean black quarter on the ball, rest untouched).
**RETRACTION (§2.80c was wrong):** the deploy .cso producer was never a mystery — wi runtime
recompile backstopped by refresh_shaders.ps1, working as documented (project-shader-build-pipeline).
Evidence: objectPS.cso 23:52:36 was NEWER than the last 08-17 edit (23:49:47); today 152 cso
flagged, recompiled at the 00:28 launch, wipe visible. The REAL 08-17 bug: `GGSetEnvSolidIni`
BOOL-IZED the mode → Lee's mid-test MAX restart demoted modes 3/4 to mode 1 flat magenta, which
on the F≈1 mirror ball is pixel-identical to split magenta (192/0/155) — exactly what was
measured. Fixed (game `59fef574`): ini passes the int through. Mip rungs 3/4 UNBLOCKED.
★ RULE refined: when a shader edit "isn't live", audit the C++ VALUE PLUMBING (esp. ini-vs-live
path equivalence of debug knobs) before indicting the shader pipeline.

## ⚠⚠ STATUS 08-17 EVENING: CLICK-HOLD CIRCLES **STILL OPEN** — 3 fixes shipped, all survived by the bug (Lee-confirmed)
2.75 matte (wrong theory, reverted) → 2.75b placed-marker `SetNotVisibleInReflections` (made SETTLED cubes clean — Lee's unpicked glossy ball reflects correctly — but hold-circles persist) → 2.75c drag-ghost exclusion + stable preview radius (`b0ca1e7b` — circles STILL show on click-hold). **Full failure ledger + established facts + next-attack plan = notes §2.75e.** Next attack MUST start with the missing datum: **DUMP_ENVPROBE into Lee's LIVE session while he holds the probe** (the harness watcher runs in his MAX; that captures the exact displayed cube and splits capture-corruption vs display-path in one shot). Suspects: ghost recreation racing the per-frame flag; hold-time re-capture including editor-only passes; displayed sphere = a different slot than assumed. PARKED per Lee — the env probe SIZE-CHANGE bug goes first (he built a NEW TEST LEVEL for it, task #158).

## The marker-ball self-capture finding (2.75b `8556ec33` — REAL but PARTIAL; supersedes the 2.74b story below)
**The marker ball was rendered INTO its own probe's capture**: the pool probe captures from the marker's centre = inside the double-sided (FPE cullmode=1), circular-featured probe.dbo ball → the cube records the ball's openings as "portholes". DX11 excluded the marker via probe `userdata` (the commented `probe->userdata` fossils in GGTerrain_part0 = the amputated mechanism). Fix: `WickedCall_MakeObjectEnvMatte` now sets `ObjectComponent::SetNotVisibleInReflections(true)` per marker frame-object (RefreshEnvProbes honours it natively, wiRenderer.cpp:10781); the 2.75 material matte is REVERTED (it made the ball solid black — probe.png is near-black in linear). Proof: SET_PROBE_TEST (harness, drives GGTerrain_AddEnvProbeList without a marker) captures CLEAN at every suspect parameter (height under/at/above, yaw 47, sizes/range) — the ball was the only differential. New-marker first capture races the ball's renderability (clean), re-captures porthole. ⚠ **Rule: anything that parks AT a probe's centre gets captured INTO that probe — check reflection-visibility flags for widget/helper entities.**

## The "circles on the probe ball" FINALE (2.74b forensics + 2.75 fix, Lee-directed — PARTIALLY WRONG, kept for the instrument trail)
- Lee's porthole ball = the **%probe MARKER ENTITY** (probe.dbo scale 50, has the selection outline), NOT the engine's data view. Cube data proven clean 3 ways: lat-long pano reprojection seamless, offline mirror-ball sim clean, engine debug sphere clean. Under DX12 PBR the legacy ball reflected its own box-projected capture through a cube-patch sphere mesh = per-face portholes.
- **2.75 shipped both Lee-approved fixes** (engine `5ee09abc` + game `f3242c8b`): `WickedCall_MakeObjectEnvMatte` (wickedcalls_part3, PICK_AT frame-walk pattern) mattes marker materials from the lighting_loop probe-list rebuild; on pick the engine debug sphere is scaled to marker AABB radius × 1.15 (engine `SetDebugEnvProbeSphereScale`, delta 2.75 on re-apply list). Preview verified live at scale 60. ⚠ Camera INSIDE the debug sphere radius sees nothing (backface cull).
- ⚠ **No harness-reachable level contains a probe marker**: only Lee's spotshadowtest.fpm (loose mapbank, no loader); projects live in `Files\projectbank\<name>\project203.dat` (custom "Stor" container, not zip — synthetic projects don't register); fpm `map.ent` (ZipCrypto pwd `mypassword`) is the cheap offline detector for marker usage.

## Known remaining (options presented, not shipped)
- Global probe captures from the map CORNER → island bakes asymmetrically; centre-high capture would be a cleaner "sky + distant terrain" base but is a global look change.
- BC6H probe format terraces the smooth sun halo into **concentric rings** (visible as a bullseye on the sun-facing face; DX11's probe array was uncompressed). Option: probe format → RGBA16F (~+1 MB/probe, drops a BlockCompress).

## ★ 2.90 (08-18) — probe MARKER properties: Brightness restored, Range removed
Lee: *"probe range and probe brightness in the properties do not seem to do anything; Size XYZ works
fine."* Both dead, for **different reasons** — repo `PROBE_PROPERTIES_2026-08-18.md` is THE authority.
- **Brightness = severed chain (real DX12 port regression).** Value plumbed panel → `fProbeBrightness`
  → `g_envProbeList[].brightness` and died at a commented-out `//probe->SetBrightness(...)`
  (GGTerrain_part0 ~9500) because the DX12 engine had **deleted** `filterBrightness`/`SetBrightness`.
  Restored DX11's design (engine 2.90): baked into the cube during BRDF mip filtering, riding the
  **free padding slot** in `FilterEnvmapPushConstants` (struct size unchanged). ⚠ `SetBrightness`
  self-`SetDirty()`s **only on change** — baked quantity + every-frame caller = infinite re-bake
  otherwise. ⚠ Covers mips 1..N-1 only; mip 0 is an unfiltered `CopyResource`, so roughness-0 mirrors
  ignore it (**DX11 had the same gap**). ⚠ The GLOBAL probe deliberately stays on the 1.55
  `gg_envprobe_brightness` shader knob — baking it too would apply the Visuals slider **squared**.
- **Range = clobbered value (never worked, in DX11 either).** `Scene::RunProbeUpdateSystem` recomputes
  `probe.range = max(scale.xyz)*2` from the transform every `Scene::Update` (DX11's engine has the
  identical line). `fLightHasProbe` is really the **"has probe" FLAG**, tested `>= 50` everywhere; the
  slider's own min was 50, so all reachable values behaved identically. Slider removed; value
  canonicalised to 50 on load (.ele + .fpe — **.fpe X/Y/Z keep the authored value**, they are the real
  volume). Probe volume has always come solely from `pTransform->Scale(sx,sy,sz)`.
- Harness `SET_PROBEMARKERBRIGHTNESS <f>` = the per-probe slider (re-bakes); `SET_ENVPROBE_BRIGHTNESS`
  = the global shader knob (live, no re-bake). Don't confuse them.
- ⚠ Stale note corrected: TESTPRO2 + `CLICK_ONLY_LEVEL` **does** reach Lee's spotshadowtest (probe
  markers present) — the older "no harness-reachable level has a probe marker" line above is obsolete.
