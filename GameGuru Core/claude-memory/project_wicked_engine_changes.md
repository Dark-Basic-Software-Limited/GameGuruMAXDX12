---
name: project-wicked-engine-changes
description: "The WickedEngineDX12 clone is NOT pristine — delta families 1.1–1.74 as of 2026-08-02 (1.74 = merged grass, BUILT but DEFAULT OFF pending the density gate) (1.73 = THE texture-streaming load-crash FIX, streaming back ON: the mip reduction halved block-compressed dimensions ignoring block alignment (500 is /4, 250 is NOT, and a BC resource's top mip must be aligned), GetCopyableFootprints then wrote 0xFFFF sentinels instead of failing and upstream's guard `rowSizesInBytes[i] > (SIZE_T)-1` can never be true on 64-bit, so memcpy ran 4-billion rows off the buffer; cracked by a symbolized StackWalk64 added to CrashLogger.cpp — reach for the instrument before the theory; 1.70 = per-resource VRAM census + dead-GGTerrain-VT lazy alloc + hair raytracing-copy gate, Island Showdown driver VRAM 8853→7064 MB; 1.69 texture streaming enablement; 1.68 Scene::Intersects front-to-back early-out + hidden-object skip = THE test-game FPS-plummet fix, 13.5→62.6, pairs with game-side ray TMax cap + static-mesh BVHs; 1.67 profiler QoL = stable perf-panel rows (persistent sorted GetTextData) + main-camera POLYS counter; 1.66 cloud-upsample inch-unit fix = mountain-silhouette-line kill; 1.65 DX11-parity fog color model + Fog Opacity = skybox-green-fog fix, pairs with game-side cloud UNIT correction = simulated-sky mountain-swallow fix; 1.64 skinningCS wave-uniform tan.w fix = THE character flicker, USER-CONFIRMED; 1.63 DX11 strength semantics; instruments SET_TANGENTVIS/DUMP_SOTAN/SET_SKYMODE) (GameGuru Core/WICKED_ENGINE_CHANGES.md status table is the authority; re-apply on upstream pull). Newest: 1.56 light-shafts sun-mask taming (skyHF.hlsli both darkMode sites: GetSunColor has intensity PRE-applied ~9× hotter than DX11's raw color — unit-peak normalize; + non-realistic gradient path ignored dark_enabled — full sky gradient entered the sun mask = whole-screen white wash; pairs with game-side setLightShaftsStrength(0.18) DX11 exposure parity — upstream default 0.5×64 taps vs DX11 0.2×32; harness SET_LIGHTSHAFTS), 1.55 global env-probe brightness at sample time (ShaderScene.padding6 → gg_envprobe_brightness, lightingHF multiply — restores the DX11 slider, live drag), **1.54 tonemap true-gamma+bloom-strength PARKED/REVERTED (washed viewport 3.4× at NEUTRAL pushed values, corner-probe never rendered — PIX capture REQUIRED before retry; full autopsy in the 1.54 doc row)**, 1.53(+b/c) terrain VT tiling cap+hold (near scale-swim fix, USER-CONFIRMED defaults cap 32 hold 4, SET_TERRAINTILE <cap> [hold]), 1.52 pooled weak_ptr underflow fix (upstream port be8c766e — entity wrong-texture root cause, resource_hijack.txt tripwire must NEVER appear), 1.51 DRED crash capture in Release (dred.txt flag → dred_report.txt), 1.50 GG grass wetmap opt-out (dark-on-reveal fix, SET_GRASSWET), 1.49b grass strand LOD (DEFAULT ON), 1.48b/c queue probes (both NEGATIVE — async overlap real, don't re-chase), 1.47 OffsetAllocator root-cause fix, 1.46 tripwire, 1.44/1.45 reload guards. Older highlights: 1.27 terrain SVT anisotropy x4->x8 in wiTerrain.cpp (`3a6fc38c` — fixes grazing 'pixel swim': the SHIPPING terrain is Wicked's native SVT path, NOT GGTerrain's custom draw which is gated off by ggterrain_use_wicked_terrain=1/ggterrain_draw_enabled=0, harness TERRAIN_DRAW_EN 0 — so the GGTERRAIN_TEXTURE_FILTERING macro is dead; stock Wicked terrain sampled x4 while every other surface is x16, causing minor-axis undersampling at grazing angles; x8 stays within the 4px SVT_TILE_BORDER, x16 risks seam spill; see [[project-terrain-render-path]]), 1.26 ocean env-map reflection fallback when planar reflections are OFF (`bf09e448` — fixes garbage/uninitialised-texture corruption on water when the user unticks Reflections: RenderPath3D::PreRender wrote camera->texture_reflection_index UNCONDITIONALLY from rtReflection_resolved, but setReflectionsEnabled(false) frees rtReflection NOT the _resolved textures, so the index stayed valid while the planar render was gated off → oceanSurfacePS sampled a stale texture; guard the index write with the same getReflectionsEnabled()&&IsRequestedPlanarReflections() gate and force -1 otherwise → ocean falls into its EnvironmentReflection_Global sky/probe fallback; ON byte-identical, OFF ~free + cheaper; harness SET_REFLECTIONS 0|1), 1.25 hair-particle sim skips billboard-write + physics for non-drawn culled strands (`3273b651` — safe perf, visuals identical; TESTPRO1 editor is GPU-bound on the grass hair sim, ~48ms/55ms GPU, ~905K strands), 1.24 ocean 'Water Base Color' tints transparent water (`77869a44` — new OceanCB.xOceanWaterColorDepth; oceanSurfacePS lerps refraction toward the base colour by depth so the colour shows on WaterAlpha-0 water, GG sets 0.005; default 0 = stock; fixes 'set red, sea unchanged from above'), 1.23 ocean underwater fog colour+density decoupled from the water surface (`f9b51982` — new ShaderOcean.underwater_color + underwater_fog_density; underwaterCS uses them with a stock fallback so the surface can stay transparent while the submerged view fogs; Water-panel 'Underwater Color' + 'Underwater Fog' controls; density 0 = stock), 1.22 ocean caustic size decoupled from wave patch size (`fc8e310a` — ShaderOcean.caustic_scale scales ONLY the seabed caustic lookup; Water-panel 'Caustic Size' slider; default 1.0 = stock), 1.21 VT expanded working set (`8ea7d29e`), 1.20 VT allocator freeze while camera moves (9f837cd1), 1.19 VT stale tile-identity release (b141786d, STOCK BUG upstream-worthy: dangling last_used), 1.18 VT upgrade hysteresis (37014667), 1.17 gg_generate_blendmap born-correct chunks (4c789814), 1.16 VT repaint main-thread latch (b26a962b), 1.15 preserve blendmap+VT residency (784ad539). Re-apply on upstream pull. Engine rebuild: build_wicked.bat Release, then rebuild the game."
metadata: 
  node_type: memory
  type: project
  originSessionId: 89bab6a9-6782-4447-8063-b03e17d3981b
  modified: 2026-08-03T17:15:08.656Z
---

The WickedEngineDX12 clone at `D:\max\WickedEngineDX12` is no longer
pristine — as of 2026-08-04 it carries the delta families **1.1–1.96** (1.85–1.95b flicker probe suite; 1.96 THE merged-grass flicker fix — dxc silently drops NonUniformResourceIndex when a resource handle crosses a function boundary, annotation must sit AT the sample subscript and be dumpbin-verified) (see
`GameGuru Core\WICKED_ENGINE_CHANGES.md` status table — ALWAYS the authority;
1.27 reverted, 1.37 default-disabled, 1.43 call-site disabled, 1.54 parked).

**1.76–1.82 = the VRAM campaign** (full rows in the repo doc, plan in [[project-vram-audit]],
floor analysis in [[project-vram-floor]]): 1.76 SVT atlas 16384→12288; 1.77 object-PSO trim of
two axes `GetObjectPSO` can never select (13976→7496); 1.78 transparent shadow atlas dropped;
1.79 lazy object PSOs behind `lowvram=1`; 1.80 suballocator 256→128 MB + tessellation/voxelize
PSO axes (6337→4033); 1.81 Scene::Update caller tracer.

**1.82 `f9723e35` (2026-08-03) — LAZY OBJECT PSOs BECOME THE DEFAULT, plus the instrument that
justified it.** Revert = setup.ini **`lazypso=0`**. Behind `lowvram=1` it only reached users who
hand-edit setup.ini, so everyone else paid ~390 MB for pipelines they never bind. The blocker was
the unmeasured first-use compile hitch, and nothing in the build could see it — the wall-gap
tracer starts at 100 ms and an FPS mean smears dozens of small stalls into nothing. Added
`gg_dbg_pso_compile_max_us` (the **worst single** compile; the sum is not what a user feels) and a
per-frame histogram on the tracer's always-on path (buckets >16.7/25/33/50/100 ms + reset), read
as `HITCH:` in `GET_PERF_DATA` with `HITCH_RESET`.
**Verdict, 5 cold runs on TESTPRO1: the hitch does not exist.** 26 extra compiles totalling ~10 ms
across a whole session, **worst single 1.1 ms**, all inside a level load already dominated by a
13-second stall; `psoC=0` in the first 25 s of play in every run. `pso_driver_eager` 4033→**1**,
driver VRAM **−392 MB**, POLYS bit-identical, screenshots identical, startup unchanged.
Saving is −392 not the −633 of the 1.79 row because A5 had already taken 6337→4033 — **they do
not stack**.
**Trap worth keeping:** the first A/B showed lazy with 98 frames over 25 ms vs the control's 1,
but `psoC=0` for that window in both configs, and on repeat the *control* swung 1→87 while lazy
went 98→101→2. Pure variance. **One counter reading zero beat a plausible 98× difference.**

**1.74 `a6cb310e` + game `8fd09cb7` (2026-08-02) — MERGED GRASS: one hair system per terrain
CHUNK instead of one per (chunk x painted type). BUILT, WORKS, **DEFAULT OFF — fails the
density gate**. `SET_GRASSMERGE <0|1>` then reload the level.**
Premise measured FIRST via a new per-chunk type histogram (`GRASS_CHUNKS` in GET_PERF_DATA):
the benchmark's 5 chunks carried all 52 systems at 7-13 types each — a **10.4x** reduction
available, not the 4x the plan assumed. Z Island: 12 chunks carry all 185 systems.
**Measured: 52 -> 5 systems, 4,216,000 -> 418,000 strands, VRAM 6536 -> 4969 MB (−1567 MB,
−24%), editor FPS 60 -> 75, POLYS identical.**
**BLOCKED ON:** coverage 10.96% vs 9.40% (+1.56 pp against a ±0.15 limit). clumpCV CLEAN
(0.863 vs 0.873) so it is uniform over-density, NOT the clumping failure of the reverted
08-01 attempt. Eliminated: texture aspect (measured — all 52 stock grass DDS are square);
absent-type rendering (`present` flag, no effect). Fixed but insufficient: strand-LOD
viewDistance coupling (host derives LOD steps from the SYSTEM viewDistance = max across types,
while GG halves it for FLOWER; now rescaled per strand — worth 11.19 -> 10.85).
**NEXT STEP IS AN INSTRUMENT, NOT A THEORY: read back the hair indirect-draw args for the
actual drawn index count per system in each mode — that one number decides count-vs-size.**
Implementation: `GGHairGrassType` table in HairParticleCB (88 entries, 2816 B); simulate CS
adopts the cell's type instead of killing the strand, per-type length/width/stiffness/drag/
viewDistance, surplus billboards collapse to zero area (stride is STRUCTURAL — shortening it
corrupts later strands); type encoded in **vb_nor.w** as (type+1)/127 (exact SNORM step; the
VS reads only .xyz so it was genuinely free); all FOUR hair pixel shaders take a per-strand
bindless blade texture (lit + prepass + prepass_depthonly + shadow — alpha cutout must match
the lit pass or silhouettes come from the wrong sprite).
**Safety verified: default-off path is untouched** — 52 systems, 4,216,000 strands, coverage
9.376 / clumpCV 0.873 / bands within 0.1 pp of the pre-change baseline.

**1.73 `53481336` + game `a0fd128a`/`0627f986` (2026-08-01) — THE texture-streaming load-crash FIX (19/19 demos verified).
Streaming is back ON by default.**

**Root cause: block-compression alignment in the mip reduction.** The reduction loop
halves width/height blindly. **500 is a multiple of 4; 250 is not** — and a BC
resource's TOP mip must be block-aligned (sub-mips are exempt, which is why the
unreduced load always worked). D3D12's `GetCopyableFootprints` refuses such a desc and,
rather than failing, writes `0xFFFF..` sentinels into every output. Upstream's only
guard against that is `rowSizesInBytes[i] > (SIZE_T)-1`, which on a 64-bit build
compares UINT64 against UINT64_MAX and **can never be true** — so `MemcpySubresource`
looped `numRows = 4,294,967,295` off the end of the file buffer. Victim:
`DOOR1_surface.dds`, 500×500 DXT1, in Trapped. Content-dependent because only a BC
texture whose dimension is 4×odd trips it; the rest of the hub is power-of-two.

Fix = `GetFormatBlockSize` guard on BOTH halving sites (load-time reduction in
`LoadResourceDirectly` AND the stream-out decay), a **no-progress bail** in the latter,
and a working sentinel check in `wiGraphicsDevice_DX12.cpp` as the backstop. Such
textures now keep a larger base mip and save nothing — the right trade. The no-progress
bail matters on its own: without it a texture that can shed no mips rebuilt a
byte-identical replacement every streaming pass (**Trapped `replaced=260468` → `12`**).

**⚑ METHOD LESSON — the most transferable thing here.** BOTH of my leading theories
(the background streaming thread; the decrypt→read→re-encrypt cycle) were **wrong**, and
I spent a long stretch reasoning about which path *could* overrun while every candidate
checked out clean. What actually cracked it in two builds was **instrumentation**:
a symbolized `StackWalk64` added to `CrashLogger.cpp` — which immediately showed the
fault was on the **MAIN thread in the INITIAL upload**, demolishing the streaming-thread
theory — plus a per-load breadcrumb (`SET_TEXSTREAMTRACE 1` → `stream_load.txt`,
`last_upload.txt`) that named the exact texture. **Reach for the instrument before the
theory.** The crash stack is permanent and now pays off for every future crash: call
chain, faulting thread tagged main-vs-worker, and for an AV the read/write flag, target
address and page state.

Permanent tripwires from this work: `stream_guard.txt` (should never appear) and
`guard_rejects=` on the harness `STREAM:` line (should always be 0).

**1.69 `22c56214` + game `3a97ed4b` (2026-08-01) — texture streaming enablement.**
(Historical note: this shipped ON, was defaulted OFF the same day by `e900f186` for the
crash above, and is ON again as of 1.73.) Workflow audit found Wicked's GPU-feedback mip
streaming ran every frame but the enrolled set was EMPTY: enrollment is
per-Load opt-in (`Flags::STREAMING`), only MaterialComponent::CreateRenderData
adds it, and MAX uses that path solely for terrain/tree materials which
explicitly `SetTextureStreamingDisabled(true)` (sync-load requirement — keep).
ALL entity textures route through the single choke point `WickedCall_LoadImage`
(wickedcalls_part0.cpp), which passed only IMPORT_NORMALMAP. Enable design:
STREAMING + container=real-disk-path (VirtualFilename — workshop/dds-renamed
cache names differ from disk) when (a) plain-DDS magic sniffed BEFORE
`g_pGlob->Decrypt` (the streaming thread re-reads the file at mip offsets LONG
after load; standalone re-encrypts → would read garbage), (b) not standalone
(`t.game.gameisexe==0`; follow-up option = IMPORT_RETAIN_FILEDATA to stream
from RAM), (c) caller allows it — 4 call sites opt OUT because their shaders
never write mip feedback and the engine would decay them to the 64KB floor
(permanent blur): sky map (M-Sky.cpp + M-GridEditB_part6.cpp), lens flares
(Common_part1.cpp), HUD blood splats (M-HUD.cpp). Engine auto-rejects
single-mip/array/3D/<64KB. **Island A/B (same spawn): VRam 10.11→8.81 GB
(−1.30 GB), sys Mem −1.37 GB, FPS 60.6 vs 59.7 = noise, visuals
pixel-comparable.** Demand-adaptive proof: village cage_wood streamed to FULL
1024/11 minlod=0, door variant converged at 512, off-screen ruins/cellar
parked at floor; entering test game replaced 77→146, resident 16→103→79 MB
(enrolled full set = 1336 MB). **INSTRUMENT TRAPS: (1) static camera →
frozen STREAM counters = correct ~2s demand equilibrium, NOT a stall (a
whole false-alarm hunt was spent here — chain probes copies/fb/req, STREAM2
job liveness, STREAM3 decision census all kept as permanent instruments);
(2) a floor-reduced 1024 reads `128x128 mips=8` in TextureDesc — byte-identical
to a native 128 full chain; only DUMP_STREAM2 (resource-manager map with
full mip_count) disambiguates; (3) resource-manager flag pinning: first load
wins — SET_TEXSTREAM changes need a level reload.** Content chip: 39
entitybank DDS are single-mip (Aztec Witch 2048s etc.) — can never stream and
stay full-size; authoring mip chains would enroll them.

**1.68 `388628e5` + game `26f4268f` (2026-07-31 evening) — Scene::Intersects
front-to-back early-out + hidden-object skip = THE test-game FPS-plummet fix
(user spawn 13.5 → 62.6 FPS).** Cause chain (byte-measured with the new
RAYS/RAYS2 counters + RUN_LUA probes): global.lua GetPlrLookingAt fires ~3
IntersectAll/frame; each wiScene::Pick cost ~26ms because SentRay4 rays had
NO TMax (infinite — traversed the whole island), Scene::Intersects brute-
forces every triangle of every AABB-hit object (50 villagers ≈ 2.47M CPU-
skinned tris at SkinVertex×3/tri), and a HIDDEN weapon (W_MK19T) with a
world-sized ±100K AABB taxed every ray from any direction. Engine 1.68:
candidates sorted by conservative world-unit AABB entry distance, break when
best hit < next entry (underestimates safe — never a missed hit);
`!IsRenderable()` skip = DX11 fork parity ("Do not Pick from hidden
objects"), nav-mesh filter exempt. Game: SentRay4/ThreadSafe normalize +
TMin/TMax; `WickedCall_BuildStaticMeshBVHs` at load-end (static meshes only;
skinned skipped = bind-pose staleness, terrain chunks skipped = engine
maintains their BVHs via SetBVHEnabled in wiTerrain). Probes: up-ray
42.5→0.3ms, in-game LOS ~1ms avg. Characters verified hittable (rays through
real villager positions return their obj) — the earlier "unhittable" scare
was stale g_Entity coords in my probe (POOL values; always aim with wicked
AABB coords from LIST_SKINNED). RESIDUAL: ray into the villager crowd with
no near blocker still ~15ms (skinned = no BVH); follow-up candidates:
lowest-LOD skinned raycast (DX11's bRaycastLowestLOD did this; our DBO
loader builds the LOD subsets but check subsets_per_lod semantics first —
it's set to the LOD COUNT, new engine reads it as subsets PER lod) or a
capsule pre-gate. Crosshair/button detection mechanism proven working
post-fix; user gameplay verdict pending.

**1.67 `259029f1` (2026-07-31 afternoon) — profiler QoL, user-requested.**
(a) GetTextData rows are now PERSISTENT (a name seen once prints forever,
0.00 ms on idle frames) and SORTED — the test-game Performance panel list no
longer jumps as conditional ranges (TerrainW - *, RP3D-rec UpdateTex...)
come and go; caches clear on profiler toggle. (b) GG_GetMainCameraPolyCount:
triangles submitted to the main camera color pass (RENDERPASS_MAIN +
DRAWSCENE_MAINCAMERA in RenderMeshes, once per subset; shadows/prepass/
reflections/impostors/hair excluded), latched per frame in UpdatePerFrameData.
Game `662df036`: panel header "FPS: x  POLYS: xK/M (DirectX 12)" +
GET_PERF_DATA `POLYS:` line (always on, no profiler needed). Verified:
triple-dump row diffs identical in editor (101 rows) and game (87);
island 4.21M editor / 5.33M test game.

**1.66 `fb742ed4` (2026-07-31 afternoon) — cloud upsample bilateral inch-unit
fix = the "mountain silhouette line" kill.** With the user's beloved LOW cloud
deck (height 19 m draping the mountains) a bright pixel-stepped line traced
every ridge: volumetricCloud_upsamplePS rejected ALL taps at terrain/sky depth
edges and emitted hard zeros (un-clouded holes). Three compounding faults:
cloud depth read as `half` (+inf beyond 65504 units = 1.66 real km → zero
weights at distance), GAUSSIAN_SIGMA_RANGE 100 "meters" (2.5 real m tolerance
→ now 3937), Gaussian() in half (2σ² overflows with the corrected sigma).
Plus weightSum==0 → bilinear fallback (no holes ever). Verified by before/
after ridge crops on the user's saved scene. NOTE: this disproves the earlier
"ridge blockiness = stock artifact" caveat — same meters-vs-inches bug class
as the 07-31 cloud unit correction.

**2026-07-31 morning (engine `01904c63`, game `9be0dff9`) — THE SKY SESSION
(user overnight brief with DX11 reference shots; both defects fixed, user
confirmation pending):**
1. **Simulated Sky "eats the mountains" = game-side cloud UNIT bug.** The
   new engine's volumetric clouds work in WORLD units (planet radius, march
   distances, extinction×distance) but the port fed
   `GGTerrain_UnitsToMeters(SkyCloudHeight)` → deck at 1524 units = 39 REAL
   meters — a ground fog bank legitimately in front of every distant
   mountain. A/B convicted (SET_FOGDENS 0 → still gone; SET_VOLCLOUDS 0 →
   back). Fix: raw-unit heights + rescale ALL meter-tuned defaults
   (horizonBlendAmount ÷39.37 — else every cloud beyond 2 real km fades out;
   extinctionCoefficient ÷39.37 — else a near-black 39×-thick deck; march/
   render/LOD distances, noise scales, skews, shadowStepLength, wind m/s);
   inverseDistanceStepCount = 3× shell thickness (stock ratio, else ~2 march
   samples per vertical ray = invisible); **coverageMinimum =
   saturate(v−1) — DX11-era shader is 1-BASED** (WickedRepo
   volumetricCloud_renderCS.hlsl:76), the level's 1.38 fed raw = permanent
   overcast. Iteration chain of symptoms (each screenshot-verified): low
   deck eats mountains → empty sky (horizon fade) → black speckled sky
   (extinction) → uniform overcast (coverage) → DX11-parity cumulus.
2. **1.65 = Sky Box/None stale green fog.** fogHF GetFog was horizon-color
   at FULL strength for non-realistic skies (and flat skyluminance average
   under realistic). Now DX11-parity: skybox/gradient fog = opt-in recolor
   toward FogRGB scaled by Fog Opacity (alpha×=opacity — NO-OP at FogA 0,
   which the island has); realistic-sky fog = per-view-direction skyview-LUT
   horizon sample (stationary, y=0.1); inscatter add dropped; both sky PS
   get the DX11 opacity tint. `WeatherComponent.gg_fog_opacity` ←
   FogA_f (legacy >2→0 clamp). The "Fog Opacity" UI slider is LIVE now
   (was ignored). None mode at FogA 0 correctly shows NO fog (the old green
   mountains in None were never DX11 behavior).
   Harness: `SET_SKYMODE <0-2>` (drives the full combo path via extracted
   `gridedit_set_sky_type`), `SET_VOLCLOUDS <0|1>`, `SET_FOGDENS <d|-1>`.
   Residual: quarter-res cloud-upsample blockiness at ridge silhouettes
   (stock artifact); cloud drift now real m/s (was 39× slower).

**2026-07-30 night (engine pushed through `92b7654d`, game `c950d361`) — THE
CHARACTER FLICKER SAGA, solved in three layers, all USER-CONFIRMED ("first
time no flicker", re-confirmed across exit/relaunch cycles):**
1. **1.64 skinningCS tangent-w float carrier = THE intermittent "massive
   texture flicker" root-cause fix.** Byte-proven via harness `DUMP_SOTAN`
   (two-frame GPU readbacks): the CS's streamed tan.w was NEVER the source
   handedness — each 64-thread wave stored ONE wave-uniform garbage value
   (RLE fingerprint: all corrupt runs 64-vert wave-aligned; source vb_tan
   100% canonical/static; so_nor/so_pos w-lanes intact). Frame-UNSTABLE
   garbage = the per-load-lottery flicker (~1 in 3 loads, 16-46% interior
   red/green flips in the mode-4 handedness view); frame-STABLE garbage =
   ever-present wrong-handedness patchwork (NO load ever had canonical w).
   Fix: read tan via the float4 bindless table, carry w in a dedicated
   float to an explicit store (packed-half register hazard around
   `tan.xyz = normalize(t)` — attribution inferred, facts airtight).
   Verified 10/10 loads at 0.00-0.07% + first-ever canonical ±1 histogram.
   **UPSTREAM Wicked report candidate (same half4 code upstream).**
2. **1.63 DX11-parity normal strength semantics** (N=lerp blend on the
   unscaled sample) + the finding that the saved TESTPRO1 level carries
   Normal Strength 4.0 = the separate DETERMINISTIC boil (SET_NORMSTR 1
   calms it; churn/sway 3.07→0.78). Advise strength ≤1.5 on characters.
3. **eb7da2c8 DBO tangent handedness w=-1** (DX11 parity for DBO-loaded
   tangents; props-only — skin meshes are MikkTSpace, mode-4 map proves).
Exonerated en route (all by live A/B on afflicted loads): dedicated
shadows, texture streaming, material cache 1.41, wetmaps (1.62e
`SET_WETMAPS` kill-switch kept), SRV+UAV descriptor collisions,
suballocator (tripwire 0 violations / 1.1M ops). Instrument suite born:
`SET_TANGENTVIS <0-22|CYCLE>` (render every pipeline contributor raw),
`DUMP_SOTAN`, `DUMP_SKINGEO`, `SET_BINDPOSE_TAN`, `SET_NORMSTR`,
`BURST_FRAMES` — all in WETEST.md. Instrument trap logged: mode-5
strength-scaled display binarizes at strength 4 (use RAW modes 21/22). Newest
(2026-07-27, engine `d5706805`): **1.50 GG grass wetmap opt-out** — root cause
of the "grass flat-dark on reveal, slowly brightens over 15-30s" artifact and
the shade half of the historic two-shade flicker: the GG sim early-out leaves
culled/LOD-dropped strands' ping-pong positions at raw zero = world (0,0,0) =
below the island waterline; the every-frame ocean wetmap ratcheted them to
wet≈0.8 (drying disabled) and hairparticlePS lerps wet albedo toward black.
GG grass (grass_type≠0) wetmap dispatches now carry gg_force_dry → wet=0 (DX11
parity, character hair stock); `SET_GRASSWET 1` reproduces the bug on demand.
If wet-look grass is ever wanted (rain), seed BOTH vb_pos ping-pong buffers at
regenerate FIRST (removes the zero-position hazard class). Also **1.49b grass
strand LOD DEFAULT ON since 2026-07-27**. Before that (2026-07-26
P.6 perf push, engine `8209a359`+`a16548d0`): **1.48a submit-tail phase timers**
(`SUBMIT_PHASES_MS` — proved the frame is GPU-WALL-bound, the 'submit tail' is the
end stall); **1.48b single-queue** + **1.48c lean-async** probes (BOTH measured
NEGATIVE on the RX 9060 XT — async-queue overlap is real; default OFF, do not
re-chase submission overhead); **1.49 grass strand LOD** (simulate-CS far-strand
2×/4× decimation + width compensation, pop-free by construction, +5 FPS opt-in,
default OFF pending user's visual call; `SET_GRASSLOD`). Before that: 1.47
OffsetAllocator port-bug root-cause fix, 1.46 tripwire, 1.44/1.45 reload guards.
Older detail below. Newest before this session: **1.30 apparent-size object cull**
(`8ad10e54`) — `wiRenderer::UpdateVisibility` drops objects whose world
`aabb.getRadius()/dist < tangent` from `visibleObjects` (distant specks), atomic
`gg_apparent_cull_bits` (0=off) driven per-frame by the game "Apparent Size" slider;
then **1.29 30fps-anim throttle wiring** (`93b7c189`); then **1.28 terrain SVT mip
bias** (`fd4a0399`); then **1.26 ocean env-map reflection fallback when
planar reflections are OFF** (`bf09e448`, game doc+lever `5cfcff0d`) — the DX12
ocean PS (`oceanSurfacePS.hlsl:63`) branches on `texture_reflection_index >= 0`
(planar) else the stable `EnvironmentReflection_Global` sky/probe fallback, but
`RenderPath3D::PreRender` wrote that index UNCONDITIONALLY from
`rtReflection_resolved` with no reflections-enabled guard; unticking Reflections
freed `rtReflection` but NOT `rtReflection_resolved`, so the index stayed valid
while the planar render was gated off → the ocean sampled a stale/uninitialised
texture = bright blocky garbage. Fix: guard the index write with the SAME
`getReflectionsEnabled() && visibility_main.IsRequestedPlanarReflections()`
condition as the planar render, force both reflection indices to -1 otherwise →
ocean uses its env-map fallback (reflect sky, matches DX11 look). Reflections-ON is
byte-identical; OFF is stable AND cheaper (skips the planar scene re-render:
TESTPRO1 island OFF 57.9 vs ON 51.3 FPS). C++-only, no shader recompile. DX11 never
corrupted because it bound `wiTextureHelper::getTransparent()` when off — a safety
net the bindless rewrite dropped. Root-caused by a 7-agent workflow (CONFIRMED),
verified live via harness `SET_REFLECTIONS 0|1`. Then **1.24 ocean "Water Base Color" tints
transparent water** (`77869a44` — the SURFACE base colour only reached the eye
through opacity/`WaterAlpha`, but GG water is `WaterAlpha 0` (transparent so the
seabed shows), so setting the water colour did nothing from above; new
`OceanCB::xOceanWaterColorDepth` makes `oceanSurfacePS` lerp the see-through
refraction toward the base colour by depth, alpha-independent — clear shallows,
coloured depths; GG sets 0.005; default 0 = stock; scales with colour saturation).
Then **1.23 ocean underwater fog colour+density
decoupled from the water surface** (`f9b51982` — `underwaterCS` took its fog
colour from `water_color.rgb` and density from `water_color.a`, the SAME values
driving the surface, so the submerged view couldn't be tuned separately AND the
GG "Water Base Color" picker forcing `WaterAlpha=255` turned the surface opaque;
new `ShaderOcean::underwater_color` + `underwater_fog_density` (density 0 = stock
fallback) let GG's "Underwater Color" + "Underwater Fog" Water-panel controls
drive the submerged look independently; the picker was also fixed to preserve
alpha). Then **1.22 ocean caustic size decoupled from
the wave patch size** (`fc8e310a` — the seabed `texture_caustics` lookup in
`light_directional` was sampled at `surface.P.xz * ocean.patch_size_rcp`, so
caustic size was welded to `patch_length` = wave size; new
`ShaderOcean::caustic_scale` scales ONLY the caustic lookup so a Water-panel
"Caustic Size" slider tunes ripple size without touching the waves; default
1.0 = stock. **Note:** the terrain's live caustics are in the ENGINE
`lightingHF.hlsli`, NOT the game copy — that copy is dead code for terrain,
see [[project-shader-build-pipeline]]), **1.21 VT expanded working set**
(`8ea7d29e`), **1.20 VT tile allocator freeze while
the camera is in motion** (`9f837cd1` — the GPU feedback keep-alive is a
2-3 frame delayed readback; fast camera moves open gaps where STILL-VISIBLE
tiles age into the free list and the miss flood steals them (foreign
pixels rendered into on-screen squares). While `gg_center_stable_frames <
10`: aging suspended + only tiles unused 60+ frames may be recycled, in
both the job rebuild and the main-thread chunk-init list),
**1.19 VT stale tile-identity release +
full freeze-while-moving** (`b141786d` — part 1 is a STOCK BUG worth an
upstream PR: `PhysicalTile::last_used` dangles into the freed
`VirtualTexture::tiles` vector, heap reuse makes `check_tile_resident`
spuriously true → page table maps physical tiles recycled to OTHER chunks
AND skips the re-render = random foreign-content squares during fast zoom
in/out; `free()` now nulls matching back-pointers before `tiles.clear()`.
Part 2 extends 1.18 to freeze downgrades too — NO VT reference changes at
all while the camera crosses chunk boundaries), **1.18 VT residency upgrade hysteresis**
(`37014667` — `gg_vt_upgrade_hysteresis`: fast zooms sweep the dist<2
high-res ring across the island; crossing chunks reset VT residency
mid-motion = square mixed-sharpness tile flicker. Upgrades now wait for
10 camera-stable frames, then 4/frame; downgrades/fresh/unbound chunks
never deferred), **1.17 gg_generate_blendmap — chunks
born with game-correct blendmaps** (`4c789814` — the generator thread
calls the game's `FillChunkBlendmapGG` [same math as the two blend
passes] before each new chunk's region texture is built. Streamed-out
chunks re-created by fast camera zooms no longer flash the engine-default
green region blend; passes latch `ChunkData::gg_blendmap_generated`
chunks and the edit bridge clears the flag. Side benefit: level load no
longer runs the ~1889-chunk main-thread rewrite — load AUTOBLEND/
PAINTBLEND counters read 0), **1.16 VT repaint flag main-thread
latch** (`b26a962b` — the async VT job consumes
`gg_repaint_blendmap_latched`, transferred from the live
`pending_repaint_blendmap` by the main-thread pickup in
UpdateVirtualTexturesCPU; the job blind-clearing the live flag could drop
a stroke's final repaint against a stale vt.blendmap binding. Found by
the adversarial multi-agent review of 1.15 — three independent reviewers
converged on it, 8/9 verifiers confirmed), **1.15 preserve blendmap + VT residency
on in-place chunk regen** (`784ad539` — `gg_preserve_blendmap_on_regen`:
sculpt-drag regens keep GG's blendmap layers, GPU blendmap texture AND
sparse-VT residency; the merge epilogue skips `vt->invalidate()` and the
freshly-merged bindingless MaterialComponent is re-bound via
`gg_material_rebind` in UpdateVirtualTexturesCPU WITHOUT `vt.init()`.
Fixed the chunk-shaped blur/checker + wrong-texture flash for the whole
duration of a sculpt drag), **1.14 generation_restart_on_dirty_materials**
(`cbce724d`), **1.13 VT pending_repaint_blendmap** (`b6f6c69a`),
**1.12 merge_pending + hotfix** (`b0518c6f`/`df3c10e0`),
**1.11 delayed shadow cascades**
(`38a9e82a` — staggered per-cascade refresh with frozen matrices + atlas
LOAD + per-rect clear draws; ON by default in GG, `DELAYED_SHADOWS 0|1`
harness A/B), **1.10 ocean foam world-unit scale**
(`da60bfad` — foam_unit_scale/foam_amount on OceanParameters; GG passes
0.08/1.3 because the stock foam math assumes meters and GG is inch-scaled;
live-tunable via the SET_OCEAN harness command), **1.9 animation/transform
hardening**
(`a4539a76` — unit-quaternion guards at both RunAnimationUpdateSystem
rotation write-backs + ApplyTransform decompose validation + tripwire
logging; the parrot-corruption fix, see [[project-parrot-corruption]]),
**1.8 high-priority generation jobs** (`48f0237e` — the Low job pool is
THREAD_PRIORITY_LOWEST and starves during level load; GG enables High
only while the view cone builds), **1.7 terrain view-cone generation
priority** (`9cbb6bdd` — chunks the camera faces build first), and
**1.6 `OCCLUSION_QUERY_DISABLED`** ObjectComponent flag (`b96e617a`) —
per-object opt-out from GPU occlusion queries, used by the 20K tree pool
(perf Stage P.1).
**Engine rebuild recipe:** `cd D:/max/WickedEngineDX12 && ./build_wicked.bat Release`
(build.bat for the game does NOT rebuild the engine lib — a wiRenderer/.cpp
change silently does nothing until the engine lib is rebuilt), then rebuild
the game. The two below are the originals this note was written for — both
exposed by `chunk_scale = 80`.

**Restore warning (2026-07-17 review):** WICKED_ENGINE_CHANGES.md does
NOT list the earlier shader-port-era engine deltas (RenderPath3D
customDraw hooks, SRV 16→64, GetDX12Device accessors, /MTd) — those live
only in `MIGRATION_PLAN.md` and `DX11_to_DX12_Shader_Porting_Plan.md`
§13.9. A re-cloned engine restored from WICKED_ENGINE_CHANGES.md alone
will not render.

**1.1** — `wiTerrain.cpp` per-vertex normal computation used a bare
`+ 1` for the horizontal step in the cross product instead of
`+ chunk_scale`. Stored vertex normals lay near-horizontal whenever
`chunk_scale > 1`. Affects terrain shading, slope-based material
weighting, and grass orientation. Fix is a one-line change at
[wiTerrain.cpp:1100-1107].

**1.2** — `hairparticle_simulateCS.hlsl` reads the stored per-vertex
normals from the chunk mesh. Wicked stores them as the face normal of
a fixed `(V, V+x, V+z)` reference triangle that doesn't match the
actual mesh triangulation at most vertices. Invisible at chunk_scale=1,
catastrophic at chunk_scale=80 — produces chaotic grass orientation
even after 1.1 lands. GameGuru workaround: in-shader recompute the
face normal from the three triangle vertex positions. The deeper
Wicked fix would be to compute proper averaged vertex normals in
`wiTerrain.cpp`.

**Why:** GameGuru MAX uses world units in inches and bumped
`Terrain.chunk_scale` to 80 to fix terrain chunk popping (see
[[project-chunk-scale]]). Both bugs were dormant at the
default chunk_scale=1 and only surfaced once we tuned that value.

**How to apply:**
- When reviewing terrain or grass behavior in MAX, assume these fixes
  are live in the rebuilt `WickedEngine_Windows.lib`. Don't suggest
  "the lib doesn't have your fix" without checking the .lib timestamp
  against the source.
- When briefing the upstream author (turanszkij), point them at the
  WICKED_ENGINE_CHANGES.md file — it has root cause, repro, and fix
  for both.
- When pulling a new upstream version into the clone, expect ALL local
  patches to need re-applying (they're not yet upstream) — check the
  WICKED_ENGINE_CHANGES.md status table for the current count.
- Wicked clone has TWO remotes: `origin` (Dark Basic fork) and
  `upstream` (turanszkij). We push our fixes to origin/master.

Related: [[feedback-check-main-repo-status]] for the general
"check before overwriting" rule.
