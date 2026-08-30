---
name: project-shadow-system
description: "Shadow distance parity DONE 2026-07-18 (0d8b185e) + tree shadow sliders WIRED (58e39dd5) + staggered cascade refresh PORTED AND ON (2021f76b, Wicked delta 1.11) + tree shadow LOD/range PERSIST with level & survive presets (c04ccd5c+cd7644fd, USER-CONFIRMED 2026-07-28). Key facts: new Wicked ignores fWickedCallShadowFarPlane; cascadeMask skips FARTHEST cascades; Front Shadows Priority is a raw global, NEVER saved."
metadata: 
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
  modified: 2026-08-06T20:15:51.068Z
---

# Shadow system (DX12) — distance parity landed 2026-07-18 (`0d8b185e`)

## ★★ 2026-08-06: SCREEN-TILE LIGHT DIVIDE — CLOSED, USER-CONFIRMED ("Confirmed, the tile divide is gone!") — 2.07g `a229fffe`; the sections below are the resolved hunt history
**ROOT CAUSE: fp16 range² overflow.** Attenuation math ran in half; fp16 max = 65504, so
any light with range > 255.9 had range² = +INF → `saturate(1-(d²/r²)²)` window collapsed
to 1 → raw 1/d² NEVER reaching zero → the tiled light culling truncated a still-visible
intensity/d² at its 16px-tile boundary = the user's screen-tile-aligned staircase divides.
Bonus bug from the same overflow: dist² → INF beyond ~256u hard-killed big lights at a
fixed 256u circle regardless of authored range. FIX: float promotion of dist²/range² in
lightingHF (light_point/spot/rect + attenuation_pointlight/spotlight). VERIFIED: 20-luma
one-pixel tile cliffs → continuous gradients at BOTH repro poses (new floor pose AND the
old warehouse razor-edge pose — one fix, both artifacts). Explains every stubborn fact:
screen-tile aligned (cull tiles), camera-following (cull-shape projection), shadow- and
shadow-res-independent, "point light clean" (their points had range < 256 = windowed),
"in the engine a long time" (since the fp16 Wicked upgrade). Decisive instrument: the
TILE_DEBUG heatmap showing the per-tile COUNT staircase exactly along the visible divide.
LESSONS: (1) the user's screen-alignment measurement was correct from hour one — trust
pixel-verified user data over instrument conclusions; (2) my two "uniform tile lists"
fingerprint probes were instrument bugs; (3) 2.07f also landed: spot feather = FIXED 8
taps (DX11 spot parity — distance-stepped taps banded dappled spot shadow).
⚠ FOLLOW-UP task #109: intensity compensation (range²-based, wickedcalls_part3) was tuned
against the broken window — big lights now fall off correctly = look change, needs eyes.

## (RESOLVED — hunt history) 2026-08-06 ~19:10: WAREHOUSE SPOT SHADOW — VERDICT REVISED: PARTIALLY OPEN (user rejected the all-clear; top-half region)
User annotation: the mesh-lattice zone (bottom half) is accepted; the TOP-half dark region
with the RAZOR-STRAIGHT edge is the bug. STATE AFTER round 2:
**HARD ANOMALY (unresolved): the shadow region + its razor edge are BIT-IDENTICAL at spot
shadow res 512 vs 2048 (rects CONFIRMED 512 via dump in-frame) — impossible for map-sampled
content of a distant occluder.** Also world-anchored (camera nudge), budget-gated (only the
overhead spot), survives FULL shader refresh (see below), AO/advanced-cull/tile-lists all
exonerated with data. Window-gap RED diag = negative (uv window covers cone); occluder-
distance classifier = no tint — but BOTH may be instrument bugs (see canary anomaly:
with canary+classifier live the ENTIRE spot contribution vanished, mechanism never
explained — revert restored it; canary magenta never appeared).
**★ STANDING DISCOVERY: `refresh_shaders.ps1` found 152 STALE ENGINE .cso** — the runtime
staleness check silently fails (meta dep paths unresolved → skipped). ANY shader-edit
verification MUST run `powershell -File D:/max/WickedEngineDX12/refresh_shaders.ps1` first
(it's inside build_wicked.bat; shader-only iterations between engine builds DON'T get it).
The artifact itself SURVIVED the full refresh = not the stale-.cso class.
NEXT LEADS (task #108): (1) does MasterRenderer run a second visibility pass that my
gg_dbg rect dump reads instead of the viewport pass? (2) re-add DUMP_SHADOWATLAS, compare
the light's atlas region content at 512 vs 2048 (if identical → the map isn't re-rendering
→ THE res-invariance names the broken stage); (3) FIND_OBJECT above the light to identify
the solid occluder candidate for the razor edge; (4) redo the occluder classifier copying
shadow_2D_feathered's descriptor use EXACTLY + a magenta canary FIRST to prove liveness.

## 2026-08-06 ~17:30 (SUPERSEDED by above): "cone-edge 20px shadow blocks" — first verdict was NOT A BUG (real catwalk-mesh shadow); game `655fe9dd`
User reported 20px screen-aligned dark tiles at the spot cone edge (warehouse TESTPRO2
scene), point light clean. TWO-HOUR autonomous funnel, everything DATA-backed: tiled
culling depth-mask OFF = pixel-identical; per-tile bucket lists UNIFORM (TILE_DEBUG
heatmap + a bucket-fingerprint shader probe); AO (MSAO) off = step persists; VRS/compute
shading not in play; camera nudge = edges move smoothly (WORLD-anchored, not screen
tiles); budget SET_SHADOW_MAX_SPOT 1 = the whole pattern belongs to the overhead spot;
look-UP screenshot = a diamond-mesh CATWALK + girder directly above the lamp. The
"tiles" = the mesh's real shadow fragmenting at the dim cone rim; projected cell ≈ 20px
at that camera (coincides with the 16px-render-tile size — the trap that launched the
hunt). **Point light looked clean because point CUBE faces pack TINY (29×29/10×10 px
rects measured!) — too coarse to resolve the mesh. THAT under-resolution is the real
engine gap (2.06 side-finding, now task #107): amount=range/dist*256-base crushes cube
rects at room distances.** Instruments kept: LIST_LIGHTS (live LightComponent ground
truth; dir is surface→light: down-spot reads (0,+1,0)), TILE_DEBUG, SET_AO.
RULE: before hunting a "grid artifact", CHECK WHAT'S PHYSICALLY ABOVE THE LIGHT — and
remember a repeating world-space shadow pattern can masquerade as a screen-space grid.

## ★ 2026-08-06 ~16:20: MOUSELOOK FLICKER **USER-CONFIRMED GONE** — root cause was spot shadow ACNE, fixed 2.07e (engine `0099b5a9`, shader-only)
("Confirmed, the mouselook flicker is gone!" — closes the multi-day hunt. Everything below
this line is the resolved history.)
USER'S DIAGNOSIS (correct): artifacts only on the DIRECTLY-LIT surface; camera pose flips
pixels IN/OUT (the "bistable patterns"); point light on the same surface = clean. Root:
**1.58 ported the DX11 feathered graded-tolerance compare for DIRECTIONAL ONLY** — spot kept
the hard SampleCmp with ZERO receiver tolerance (D32 raster bias is exponent-scaled ≈ 0);
point has its own 0.989 distance cushion in shadow_cube. DX11 shadowCascadeSpot ran the SAME
feather as the sun (65536, cascade 0). Fix: light_spot + light_rect → shadow_2D_feathered.
Explains ALL old evidence: map pixel-identical (map was never wrong), rects stable, uploads
bit-identical — the RECEIVER COMPARE was the knife edge; mouselook jitters transforms.
Verified: TESTPRO2 box lit-face mottling GONE, cast/contact shadows intact. Shader-only
(recompiles at launch). **Awaiting user mouselook confirm = the final flicker verdict.**

## 2026-08-06 ~15:50: FOURTH BUG — moving a CASTER left its shadow behind — FIXED (2.07d, engine `ab403fdd` / game docs `ab5ad545`)
User repro on the two-hovering-boxes scene: drag a box under the spot → shadow stays at
the old spot until the LIGHT is clicked. Cache key (even after 2.07c) covers only LIGHT
state — casters weren't in it. Fix: Phase 2 decision also invalidates when any renderable
shadow-casting object's world matrix changed since last frame (scene.matrix_objects vs
matrix_objects_prev memcmp) AND its AABB intersects a granted light sphere (radius +
translation delta → leaving the range also invalidates). Verified: static scene
SHADOW_LOCAL_RENDERED stays 0 (no churn, torch-bank win intact), FPS 119 flat.
⚠ KNOWN LIMITATION: bone-only animation (idling character) doesn't change the object
matrix → its cached local shadow can freeze mid-pose; masked by rect churn under camera
motion. If a user reports it: treat armature-driven objects as always-dirty near granted
lights (costs the cache win in NPC-adjacent torch scenes — decide then).
NOTE user is ALSO testing the two hovering boxes to isolate whether the FLICKER needs
floor geometry ("not conclusive at this step" — their words).

## 2026-08-06 ~15:30: THIRD BUG — stale cached spot shadow on Spotlight Radius edits — FIXED (2.07c, engine `2aad301a` / game `0199b539`)
User repro: radius 29→100 = "shadows where they should not be"; clicking the light healed
it. Phase 2 cache key was (ent, rect, pos, range) — cone/rotation edits change the shadow
projection WITHOUT changing any key field → cache served the old-cone map through the new
projection. Click heals because pos changes. Fix: LocalShadowSlot += dir + cone
(L.direction / L.outerConeAngle) in the change detector. No-churn verified live
(SHADOW_LOCAL_RENDERED stays 0 at steady state despite lighting_loop re-pushing identical
values per frame). Heal path = the same full-clear that pos changes already trip.
⚠ SET_LIGHT_RADIUS harness lever writes t.infinilight[].spotlightradius but the per-frame
profile sync can overwrite it for map-entity lights — the UI slider is the reliable driver;
user verifies visually. RULE: anything that affects rendered shadow CONTENT (pos, range,
dir, cone, rect) must be in the cache key.

## 2026-08-06 ~15:00: SECOND COUPLING — Sun=Off killed spot shadows — FIXED+VERIFIED (2.07b, engine `31795cc0` / game `9f0b43a0`)
User-proven minutes after 2.07: Sun shadow res Off → spot shadow vanished. DrawShadowmaps
SPOT/RECT case guarded `max_shadow_resolution_2D == 0` (correct upstream, but GG feeds _2D
the SUN dropdown) → spot rect PACKED but never RENDERED → sampled empty atlas = lit. Fix:
guard on max_shadow_resolution_spot. Isolation recipe that worked (minutes, not hours):
new `SET_SHADOW_RES sun|spot|point <res>` + DUMP_SHADOWRECTS (rect survived = budget fine)
+ SET_SHADOW_CACHE 0 (still broken = cache exonerated) → only the render path left.
DIRECTIONAL/_2D and POINT/_cube guards are correct for their own knobs.

## 2026-08-06 ~14:30: USER-PROVEN KNOB CROSSOVER — FIXED+VERIFIED (GGMAX 2.07, game `08a52131` / engine `a4e59ac2`)
DUMP_SHADOWRECTS gate on the user's TESTPRO2 box scene, all four cap combos exact:
spot0/pt0 = sun only; spot4/pt0 = sun + SPOT 2048² (was the broken "no shadow" case);
spot4/pt4 = sun+spot+3 points; spot0/pt4 = spot rect revoked, points stay (was the broken
"spot won't leave" case). Screenshots at this camera pose are too subtle to judge —
use the rect dump as the oracle. Awaiting user's visual confirm + flicker re-test.
User's trial on a fresh TESTPRO2 scene (box + 1 spot + 1 point): Shadow Quantity SPOT
dropdown did NOTHING; the POINT dropdown turned SPOT shadows on/off. Root cause: GG Phase 1
budget was ONE pool fed from `iShadowPointMax` only, but its candidate list mixes
POINT+SPOT+RECT casters (wiRenderer shadow packing); the game computed the spot cap and
threw it away (`SetShadowPropsSpot2D` removed in the port). ALSO dead: the spot RESOLUTION
dropdown (spot rects sized from `max_shadow_resolution_2D` = the SUN cascade res). Fix =
engine 2.07 `SetLocalShadowBudget(maxSpot, maxPoint)` (two pools, per-type nth_element +
hysteresis) + `SetShadowPropsSpot(res)`; game `Wicked_Update_Shadows` pushes both budgets
+ spot res every apply; caching enables when EITHER type on. Harness: SET_SHADOW_MAX now
forces BOTH caps; new SET_SHADOW_MAX_SPOT / SET_SHADOW_MAX_POINT.
**FLICKER RELEVANCE:** budget candidates come from frustum-culled `vis.visibleLights` —
mouselook rotation changes WHO COMPETES for grants each frame; with spots squeezed into a
pool sized by the point knob, grant flap under rotation was structurally guaranteed on
mixed-light levels (Snowy has many). Hysteresis (1.15× incumbent) makes outcomes
HISTORY-DEPENDENT = matches the observed bistability. NOTE the earlier "budget grant
exonerated" evidence was measured on spotshadowtest (2 visible/1 granted, stable) — that
exoneration does NOT cover mixed-light levels, and the granted-count instrument predates
the split.

## (RESOLVED by 2.07e above — historical hunt state) 2026-08-06: spot flicker not fixed by the cone fix; the evidence funnel below led to the acne diagnosis
User's `spotshadowtest.fpm` (in TESTPRO2 project — load via OPEN_PROJECT TESTPRO2 +
CLICK_ONLY_LEVEL; camera pose `SET_CAMERA 1894.57 197.67 -12818.32 80.29 -1071.32`; the
kept spot = e=1191, r503 at (1857.8,308.4,-12805.5), authored cone → 27° half post-fix):
right-mouse mouselook flips the grate lattice between crisp / all-dark / other-pattern.
**EXONERATED WITH EVIDENCE (do NOT re-chase):** shadow map content (atlas rect dumped at
good+bad poses = pixel-identical), rect/pack scale (2048², 1.0 both), light frustum cull +
budget (2 visible/1 granted both), cascade stagger (off = no change), per-cascade LOD (off
= no change), Phase-2 cache (off = no change), screen-space shadows (never enabled), and
the ENTIRE per-frame uploaded light state — CAPTURE_LIGHT dumped pos/rot/dir/cone/mulAdd/
matrixIndex(6)/full matrix BIT-IDENTICAL between good and bad poses.
**KEY DISCRIMINATORS FOUND:** (a) `SET_SHADOW_MAX 0` at a bad state → floor fully LIT by
the spot = the light reaches the surface; the SHADOW SAMPLE (or its branch) is what kills
it. (b) Color-diagnostic shader (lightingHF light_spot instrumented) showed the sampling
branch NOT REACHED in one bad capture but REACHED+LIT in another run at the SAME pose →
**the states are BISTABLE / HISTORY-DEPENDENT, not pose-determined** — matches the user
("I do it again and get another pattern"). (c) **SET_CAMERA pose teleports CANNOT trigger
the latch** (8/8 rapid-toggle attempts clean); only real right-mouse mouselook (continuous
per-frame rotation through the editor mouselook path) flips it. The earlier same-pose
pixel-identical determinism checks were WITHIN one latched state.
**Instruments:** committed = DUMP_SHADOWRECTS / SET_LIGHT_LIT / GET_CAMERA (game
746b73e9, engine 2.06 381f1fb9). REVERTED at user request (uncommitted, easy to re-add
from this description): TILE_DEBUG (SetDebugLightCulling), DUMP_SHADOWATLAS (engine
GG_GetShadowAtlas bridge), CAPTURE_LIGHT/DUMP_LIGHTCAP (engine capture in the spot
entity-fill), and the lightingHF color-table diagnostic (each early-out + sample verdict
a distinct color — ONE screenshot of a broken state names the failing stage; the user was
about to drive it when they chose to reset and take a different approach post-compact).
**Session-history suspects still standing (untested):** per-frame GPU buffers latching
across mouselook-mode frame timing (entity/matrix upload vs tile-culling order), the
editor mouselook code path itself (mode flips, per-frame angle writes), visibility-shading
tile list staleness. The 20-min repro loop: load TESTPRO2, mouselook until broken, then
color-diagnostic screenshot.

## 2026-08-05 late (superseded — real but NOT the user's flicker): SPOT CONE PORT BUG — fixed (game `746b73e9`)
Snowy Mountain Stroll start room: rotating the editor camera flipped the spot's caged
light/shadow pattern between states. NOT a shadow bug at all: the DX11→DX12 port renamed
old-Wicked `LightComponent::fov` (FULL cone angle; cone cos = cos(fov/2), projection =
fov) to `outerConeAngle` (HALF angle; cone cos = cos(outer), projection = outer*2)
**without halving** in `WickedCall_UpdateLight` — every spot ran at DOUBLE its authored
cone, and cones authored ≥90° had zero/NEGATIVE cone cos → the Forward+ tile-culling
bounding sphere (r = range·0.5/cos²) went infinite/mirrored → light presence flipped PER
SCREEN TILE with camera pose. Fix: halve + clamp 1..85° half-angle. **All spots
everywhere are now DX11-authored width (narrower)** — expect lighting look changes.
EXONERATED with evidence (do not re-chase): atlas pack scale + rects (stable, via new
`DUMP_SHADOWRECTS`), Phase-2 shadow cache, stagger, per-cascade LOD, screen-space shadows
(never enabled), grant churn. Pattern OWNER found by `SET_LIGHT_LIT` toggle bisection —
it was LIGHT[24], NOT the visually-obvious overhead spot (user's guess + mine).
SIDE FINDING: POINT light cube shadow rects run TINY (33-90 px/face at room distance;
amount = range/dist × 256 base) — quality lever for later.
NEW HARNESS: DUMP_SHADOWRECTS, SET_LIGHT_LIT <idx> <0|1>, GET_CAMERA (see WETEST.md).

## 2026-07-30 03:45: per-character DEDICATED SHADOWS — BUILT & side-by-side verified (game `4aff35da`+`ec83bb03`); ~28 texels/inch on characters, `SET_CHARSHADOW <0-3>` default 3, user verdict pending. **1.59c (`ac0b282d`) = the REAL rectangles fix: cascade Z-edge BLEND BAND was sampling the NEXT matrix slot (= another character's dedicated map) at out-of-range UVs — border clamp smeared foreign edge texels across the band. Fixed with an is_saturated containment guard on the blend fallback (sun cascades unchanged — concentric). 1.59b's depth-clamp theory was WRONG and its Z extensions are REVERTED (they only relocated/stretched the bands; clamped ortho casters are geometrically correct blockers — remember this). ON/OFF diff = clean.** Old 1.59b note (superseded): FALSE OCCLUSION RECTANGLES near characters claimed fixed — dedicated slots' tiny ±12ft projection-Z let up-sun geometry inside the ~83ft culling range depth-clamp to the near plane = solid false shadow strips on the slot's ground footprint; projection-Z now matches culling-Z (D32 precision ample). Convicted by elimination (bias 4/12 ✗, grass removal ✗) + before/after diff. NOTE: thin slab props on the TESTPRO1 beach cast small legit rectangular shadows — don't mistake them for this artifact. Cost RECLAIMED by engine 1.59 (`b4fc81ed`): stagger+farcull now offset past dedicated slots instead of bailing — hero slot costs ~0.3 FPS (69.3-69.6 vs 69.7 OFF). Original trace below:
User asked about high-res per-character sun shadows. **New Wicked already ships the whole renderer
side** (dormant): `Scene::character_dedicated_shadows` (plain public `vector<Sphere>`, wiScene.h:93)
→ each sphere becomes ONE EXTRA FULL-RES cascade slice PREPENDED before the 5 sun cascades (packer
wiRenderer.cpp:4208; cams :3072-3090; render :7294+; shader = plain cascade loop, our 1.58 feathered
sampler covers it with ZERO edits). **Bridge = ~60 lines GAME-SIDE ONLY**: do NOT create
CharacterComponents (upstream controller stomps transforms even when inactive, wiScene.cpp:6499);
instead fill the vector directly each frame in Master::Update AFTER __super::Update(dt)
(master_part0.cpp:633 — Scene::Update clears it at wiScene.cpp:6541) and before PreRender.
Sphere = feet(x,y,z from t.entityelement[e]) + (0,h/2,0), radius=h/2 (~36in). Filter: bankindex>0,
profile ischaracter==1, eleprof.disableascharacter==0, obj>0+ObjectExist, active, health>0,
!ragdollified, !ishidden; nearest-N by plrdist. **CAP N=3: 5×2048 sun + 3×2048 dedicated = exactly
the 16384 atlas cap; a 4th silently shrinks ALL slices.** Third-person char (t.playercontrol.
thirdperson.charactere) = prime candidate; FPS player has no element (weapon already shadow-off).
COSTS: +1 full-res slice/frame per slot; ANY live slot disables delayed-cascade stagger
(wiRenderer.cpp:4985) AND far-cascade cull (:7310) — engine bails on index shift; fixable later by
offsetting their cascade indices by dedicated_count. Full trace in session 2026-07-30 workflow
wf_a23a60ca-90f output.

## 2026-07-30 later: Wicked 1.58 DX11-FIDELITY SHADOWS shipped (engine 983dc4f8, game 3bc52f36)
User asked the cost of DX11's shadow luxuries: **D32_FLOAT atlas restored** (upstream D16 = ~128×
coarser bias units, quantized soft-compares) + **feathered gather PCF** verbatim-ported from the
DX11 fork (shadow_2D_feathered: point-clamp GatherRed via FLOAT bindless view — half4 view would
quantize D32; feather 65536/(cascade+1); manual bilinear; 8..1 distance taps; sun only, spots
stock) + wiHairParticle latent bug fixed (non-unorm branch biased rs not rs_shadow). Float-branch
bias = DX11 parity -1/-4/clamp0. **COST ≈ FREE: fresh-launch matched at the grass-heavy neck view:
D16+stock 71.7 / D32+feather 71.5 / D16+feather 70.5 FPS — all within noise; VRAM +10-32MB.**
MEASUREMENT TRAP LOGGED: an 89.4 FPS 'baseline' came from a 45-min-idle session — HairSim decays
when parked (3.8ms fresh vs ~1ms settled); NEVER compare fresh-launch vs long-idle numbers.
KNOWN GAP (user decision pending): atlas packer caps 8192 — five '2048' sun cascades pack at
1024 each; DX11's texture array ran true 2048. Raising the cap = real resolution win, more VRAM.
1.57 SET_SHADOWBIAS ULP units now D32-miscalibrated (default 0, harmless). User visual verdict on
1.58 PENDING (MAX left at the neck view).

## 2026-07-30: animated self-shadow flicker (neck/back) — RESOLVED & USER-CONFIRMED (1.57b slope-bias clamp); elimination table below still valid re what it ISN'T
User repro: TESTPRO1 close-up (SET_CAMERA 3305.6 187.2 -8483.0 16.9 -93.4), idle character,
sun self/cast shadow on the back toggles in sync with animation; no flicker when still.
**ELIMINATED (identical 1Hz burst signatures at the repro):** receiver depth bias 0/2/6/12/64
ULPs (delta 1.57 knob, `SET_SHADOWBIAS`); delayed-cascade stagger ON/OFF; per-cascade shadow
LOD override ON/OFF; far-cascade cull ON/OFF; slope-bias clamp (1.57b) on/off.
**INSTRUMENT LIMIT (the real lesson):** harness screenshots are ~1Hz — a 60fps temporal artifact
is unmeasurable; all burst metrics were dominated by LEGIT pose-driven shadow movement (grazing
incidence amplifies pose mm into shadow inches on shoulder/neck). Do NOT re-run 1Hz burst A/Bs.
**THE FIX (user-confirmed at 60fps 2026-07-30 — "shadow moves rather than flickers"):** 1.57b RSTYPE_SHADOW depth_bias_clamp=-16/65536 (was unbounded; D16 ULP is ~128× coarser
than DX11's D32 at the same -1/-4 numbers — principled cap). 1.57 receiver-bias knob DEFAULT 0.
**TOP REMAINING SUSPECT:** DX11 fork's receiver depth feather + 3×3 bilinear Gather PCF
(WickedRepo lightingHF.hlsli:48-60, `shadows *= saturate((depthSample-dist)*65536/(cascade+1))`)
— the new hard-SampleCmp 16-tap Vogel path lost it. Port = 1.58 candidate (shader-only), verify
with the USER'S EYES at 60fps, not screenshots. Flicker scripts: scratchpad flicker_burst.sh +
maps under scratchpad/flicker/.


## Root cause of the short-shadow bug

The port kept `WickedCall_SetShadowRange` → `fWickedCallShadowFarPlane`, but the NEW engine never reads that global. New Wicked drives directional cascades from `LightComponent::cascade_distances`; the sun was left on the stock default `{8,80,800}` = **~20 metres** in GG's inch world. Production DX11 (old engine, `D:\max\WickedRepo` wiRenderer.cpp GGREDUCED block) uses **5 cascades: 0/380/950/7500/30000/500000** (far = `visuals.fShadowFarPlane`, default `DEFAULT_FAR_PLANE` 500000). Production also STAGGERS cascade updates (comments "60/30/20/15/12 fps" per cascade) — not ported; see "future" below.

## What DX12 now does

1. **Sun cascades** = production splits, set at sun creation (master_part1.cpp) and re-applied by `WickedCall_SetShadowRange` (wickedcalls_part3.cpp — called from M-GridEditB_part3 visuals apply).
2. **Tree shadows = exact DX11 recipe**: real meshes cast ONLY in band 0 (<2500, DX11's `lod_dist_shadow` default) + `cascadeMask=2` cap (cascades 0-2); every tree beyond gets its shadow from **merged billboard proxy meshes** — per tree chunk (16×16 grid), one static mesh, two crossed alpha-tested quads per valid tree, textured with the authored `Files/treebank/billboards/*_BB_SF_*_color.dds` silhouettes (the same DDS DX11 billboards use), per-type subsets. Proxy objects are shadow-only: `SetNotVisibleInMainCamera` + `SetNotVisibleInReflections` + occlusion opt-out.
3. Proxies rebuild as one deferred batch ~0.5s (30 frames) after the last `g_treeInstanceStamp` change; park/unpark with the pool's `draw_enabled` gate; honour `ggtrees_global_params.draw_shadows`.

## Measured (TESTPRO1 A/B camera)

| Config | FPS | Shadow CPU/GPU |
|---|---|---|
| Stock 800-unit cascades (pre-fix) | 60.5 | 0.6 / 0.2 |
| Production cascades, all 20K meshes cast | 27 | 23.3 / 12.9 |
| + trees cascadeMask=1 | 35 | 16.9 / 8.6 |
| + trees cascadeMask=2 (no far tree shadows) | 51 | 5.5 / 1.4 |
| **Final: band-0 mesh + billboard proxies** | **58-59** | **1.9 / 0.6** |

## Key API facts

- `ObjectComponent::cascadeMask = N` skips the N **FARTHEST** cascades (`cascade < cascade_count - cascadeMask` in wiRenderer). There is no skip-NEAR mask.
- The shadow pass iterates ALL scene objects (not the main visibility list) — `NotVisibleInMainCamera` objects still cast. They DO render into env probes (accepted; they look like trees).
- Shadow depth quality relies on the material's alphaRef; billboard proxies use 0.85 (Wicked inverted semantics — clip below 0.15).

## Sliders + stagger (landed 2026-07-18, after distance parity)

- **Tree shadow sliders SURFACED into the live UI (`cd666bca`, 2026-07-19)**: the "Tree Shadow LOD Distance" + "Tree Shadow Range" pair had been stranded in `imgui_terrain_loop_v2()` (DEAD CODE, zero call sites) — the user "couldn't find the slider they designed". Moved into the LIVE terrain panel `imgui_terrain_loop_v3()` (`M-TerrainNew_part1.cpp`, called from `M-GridEdit_part1.cpp:6887`), in the Paint Tree tool next to density/water/wind/SSS. (There is NO general shadow-far-distance slider; `visuals.fShadowFarPlane` is pinned at DEFAULT_FAR_PLANE 500000, config-load commented out.)
- **Tree shadow sliders WIRED (`58e39dd5`)**: the Terrain Tools debug pair now drives the live Wicked path. `lod_dist_shadow` (750-7000, default 2500) = per-tree mesh-shadow radius via `g_treeShadow` flag computed in the nearest-N mark loop; `tree_shadow_range` (0-5) = cascades receiving tree shadows — proxy `cascadeMask = 5-range`, mesh mask `max(2, 5-range)`, live-updated on change; range 0 = no tree shadows (DX11 semantics). **DX12 default range bumped 3→5** (and ULTRA preset 4→5) — island-wide proxies ARE the DX12 look; defaults reproduce pre-slider behaviour exactly. Harness: `SET_TREES shadowdist|shadowrange|drawshadows`.
- **Staggered cascade refresh PORTED AND ON (`2021f76b` + Wicked delta 1.11 `38a9e82a`)**: c0 every frame, then %2 %3 %4 %9 + DX11 load leveler + 64-inch camera-move override. Engine mechanism: frozen matrixArray VPs for skipped cascades, atlas LoadOp::LOAD with per-rect scissored clear-draws (new shadowClearPS + screenVS z=0 + DSSTYPE_WRITEONLY), no instances/hair/customDraw for skipped cascades. Full refresh forced on atlas repack/rect move/sun rotation/split change (change-latched hooks in WickedCall_SetSunDirection/SetShadowRange). Measured static camera: Shadowmap CPU 2.32→1.04ms, GPU 0.62→0.17ms. `DELAYED_SHADOWS 0|1` harness A/B. Single-sun assumption; character dedicated shadows disable it per frame.

## Known gaps / future

- ~~Proxy rebuild is a full 256-chunk batch~~ **FIXED (`173c88e3`)**: user hit the predicted ~2s post-edit freeze; proxies are now per-chunk with setter-driven dirty marks (GGTrees_MarkProxyChunkDirtyAt) + 30-frame batching + 3ms/frame time-budgeted consumption. A brush swath = 1-4 chunk rebuilds; the all-dirty worst case (shadows toggle/terrain regen) = ~5s of 45-55 FPS responsive instead of a 2s freeze.
- Near trees get proxy-quad shadows ON TOP of mesh shadows (union in depth map — reads as slightly fuller near shadow; not visible in A/B so accepted).
- ~~Slider values are runtime-only~~ **FIXED & USER-CONFIRMED 2026-07-28 (`c04ccd5c` + `cd7644fd`)**: `fTreeShadowLODDist`/`iTreeShadowRange` now live in `visualstype` (master copy, defaults 4000/5), saved/loaded in the FPM visuals block, sliders (moved to Visuals > Shadows panel `b1d93341`) bind t.visuals + mirror t.gamevisuals + live-apply to GGTrees; `Wicked_Update_Shadows` syncs visuals→GGTrees on every apply with a zero-struct guard; test-game exit restores like other shadow fields. **Preset trap resolved for real**: first attempt mirrored tree values into `SetGlobalGraphicsSettings` tiers — user field-caught it stamping 750/2 over authored 7000/5 because their "Graphics Quality (Test Game)" preference is LOW (default 2=High; combo in M-GridEditB_part5). Now NO preset touches the tree fields — `GGTrees_SetPerformanceMode` only scales visual lod_dist, and the trailing `Wicked_Update_Visuals` re-asserts authored values. **Side-finding**: "Front Shadows Priority" (`bShadowsInFrontTakesPriority`, wickedcalls_part0) is a never-saved raw global — resets to false every restart; task chip spawned to wire it into visuals.
- PaintTree-undo landmine (pre-existing): the undo memcpy restores pAllTrees but pTreeChunks membership stays stale — proxies rebuilt from chunk lists can mis-bucket trees that were MOVED between snapshot and undo. Rare (move+paint+undo combo); fix = rebuild chunk membership on undo if ever reported.

Related: [[project-trees-phase5]], [[project-performance]], [[project-dx11-parity-baseline]].
