---
name: project-gpup-particles
description: "Legacy gpup/.arx GPU particles in DX12 — re-enable fixes (static PSO descs, in-pass dynamic CBs, customDraw hook) + THE dt-cap bug (hitch frames warped the sim 33x = size/movement/reset complaints); GPUP_DUMP forensics; VRAM sweep clean"
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-07T20:27:40.565Z
---

# gpup/.arx legacy particles — RE-ENABLED 2026-08-07 (game `5829d665`, engine 2.11 `98a1df21`)

THIRD particle system (do not confuse: WPE/.PE = done 08-04; ravey M-Particles = still out
of scope). gpup carries **every entity "Particles" marker** (`_markers\Particles.fpe`,
ismarker=10, per-instance `newparticle.emittername` like `particlesbank\smoke_thick` →
`.arx`). The spotshadowtest steam = 8 smoke_thick markers. The WPE corpus has NO
equivalents for the particlesbank .arx effects — nothing else can render these.
Dead since Feb-17 (`80c936f5`): gpup_init hard-returned -1 "temporarily disabled".

## The three bugs that killed the Feb port (and their fixes)

1. ★★ **Dangling PSO descs**: gpup_init built PipelineStateDesc from STACK LOCALS.
   `PipelineStateDesc` stores POINTERS; modern Wicked defers the driver PSO compile to
   FIRST DRAW (pso_validate), and the input layout's SemanticName is kept as a `c_str()`
   into the caller's InputLayout (wiGraphicsDevice_DX12.cpp:4865). The lazy compile read
   dangling "POSITION"/"UV" → d3d12.dll AV at 0x18c. Fix: `static` descs. DX11-era Wicked
   compiled PSOs EAGERLY so stack locals were correct then — ★ RULE: any game-side
   CreatePipelineState must keep its desc objects alive to first draw; the same dormant
   landmine was fixed in GGGrass.cpp's legacy path. CustomShaders.cpp is clean (no IL).
2. ★★ **UpdateBuffer inside the render pass**: gpup_draw updated its CBs with
   `UpdateBuffer` (= records a CopyBuffer) INSIDE the transparent render pass → copies in
   a D3D12 render pass are an INVALID CALL → **device REMOVED at submit**
   (DXGI_ERROR_INVALID_CALL on Present — two error dialogs, NOT a process crash; the
   crash logs stay empty, easy to misread). Fix: `BindDynamicConstantBuffer` — THE in-pass
   idiom for anything drawn from customDraw_* callbacks. Sim-side UpdateBuffers are
   outside passes and stayed.
3. **No render hook**: DX11 drove gpup from ENGINE code (fork RenderPath3D.cpp:2010/2027 +
   per-object interleave wiRenderer.cpp:3546) — zero gpup refs in the DX12 clone. Fix is
   GAME-side only (selection-outline pattern): `GPUParticles::gpup_draw(GetCamera(), cmd)`
   in the customDraw_Transparent lambda (master_part1), BEFORE the wicked-terrain
   early-out. Whole batch back-to-front via the restored SINGLE-DIM sort (the [256]
   per-cmd arrays died with integer CommandLists); the per-object interleave
   (gpup_draw_bydistance) stays unported — needs an engine callback in RenderMeshes,
   deliberately not taken. Symptom if ever wanted: gpup smoke draws over glass panes.

## Hardening that made it debuggable

- gpup_init now CHECKS every LoadShader/CreatePipelineState/CreateBuffer (the Feb commit
  had removed all checking) and self-disables with per-line-flushed `gpup_trace.txt` —
  wi::backlog LOSES ITS TAIL in a crash; only fopen/fflush evidence survives.
- Engine 2.11: pso_validate null-guard — driver-rejected lazy PSO writes hr + shaders +
  IL dump + renderpass facts to `gg_pso_fail.txt`, skips the bind, app survives. Converts
  every future rejected-PSO from an unexplained Release AV into a named report.

## Verified live (spotshadowtest editor)

Steam columns render ANIMATED at the pipes, FPS 110, gpup_trace all-SUCCESS, no pso-fail
file, lights intact. A benign mystery remains: exactly ONE
D3D12CreateVersionedRootSignatureDeserializer 0x80070057 error at boot — all 7 GPUP .cso
carry byte-identical valid RTS0 and all 7 PSOs got root signatures, so it's some other
shader and predates today (log.txt is per-run; never checked before). Not blocking.

## ★★ ROUND 2 (2026-08-07 late, game `b1cf18e1`): THE dt-CAP BUG — size/movement/reset all one cause

USER (after first eyeball): particles ~25% too large, "moved around a lot more than DX11",
periodic visible resets, near-washout looking up through the steam. The sim CODE is
byte-identical to DX11 (all 7 shaders, arm logic, timers, RGBA8 formats, .arx parse,
matrices — exhaustively diffed). THE DELTA WAS THE CALLER: **DX11 capped deltaTime at 1/30s
INSIDE the engine loop (fork MainComponent.cpp:138) before any consumer; DX12's engine
clamps at 0.5s and the game's P6 1/30 cap sat AFTER gpup_update** — every hitch frame
(lazy-PSO compile, SVT upload, alt-tab) fed the sim up to 0.5s = a 33-quantum warp in ONE
tick: particles leapt, mass-aged (bigger via size-over-life), wrapped lifetimes (the
visible "reset") and the column ballooned (the washout trigger). Fix: cap moved to the top
of MasterRenderer::Update (nothing else between consumed dt). Also restored DX11's
zero-init of the sim RTTs (Feb port passed nullptr → fresh emitters sampled garbage under
LoadOp::DONTCARE).

★ RULE: when porting a subsystem, diff the CALLER'S dt path too — identical sim code with
a different clamp regime produces "the physics feel wrong" bugs that no code diff finds.
★ RULE: luma-based A/Bs on particles need the same-knob control AND the knowledge that the
washout class here was EPISODIC (hitch-triggered) — a clean burst proves nothing about it;
the cadence counter (max_time) is the honest detector.

Forensics shipped (WETEST): `GPUP_DUMP` (parsed fields + GPU constants + cadence counters
— live-verified sim rate 1.05× real, max single-step warp 2.94 ≤ the 3.2 legal bound, was
33+), `GPUP_SHOW 0|1` attribution lever, `SET_GPUPARM` arm pin (stock threshold 7.143ms =
140 FPS sits inside the editor band; both arms rate-fair long-run). Attribution at the
user's saved washout pose: base 41 mean luma, steam pulses +16, the 2.10 light curve ~1 —
lights exonerated for the look-up washout.

## ★★★ ROUND 3 (2026-08-08 00:40, game `c2ea63c4`): THE WHITE-OUT — fp32 HASH-SEED DEGRADATION

USER caught the live run ~50 min in: whole screen white. `GPUP_SHOW 0` = normal scene
(lights exonerated again). **Level-reload experiment was the decisive discriminator:
fresh emitters inheriting the old global clock are INSTANTLY broken → global-clock class,
not per-emitter sim state.** Root cause: `posConstantData.rnd` / `speedConstantData.rnd` /
`mainVS rota.z` are fed `gpup_settings.sn` RAW (grows ~10.4/s; DX11 wraps only at 1e9).
The shader hash `frac(sin(dot(seed, ~285× primes))×75633)` degrades with seed magnitude:
at sn≈32k the dot reaches 8-9 MILLION where fp32 ULP = 0.5 → the mod-2π collapses to a
handful of distinct values → spawn velocities degenerate into coherent jets → the cloud
congeals opaque. **Mid-degradation (5-30 min) = the "bigger, moves more" residue; terminal
= white-out. DX11 carries the same latent code — sessions just never idled 50+ min at a
particle view.** Fix: accumulator untouched, hash sees `fmodf(sn, 256)` (~25s macro-repeat
in turbulence seeding, imperceptible vs ~4s lifespans).

★ RULE: a SLOW-divergence bug needs a FAST-FORWARD lever (`GPUP_SET_SN`) — verified look
INVARIANT at sn = 0 / 32k / 500k (500k ≈ 13 h of runtime; pre-fix 32k was the white-out).
★ RULE: shader hashes of the frac(sin(big·seed)) family DEGRADE with seed magnitude in
fp32 — any ever-growing clock fed to one is a time bomb; audit WPE/other shader hashes.
★ RULE: fopen-relative files land in the game's CWD = `Files/`, NOT the exe dir (the
gpup_dump.txt "missing file" trap; boot-time writes DO hit the exe dir — CWD changes later).
★ MY ERROR to not repeat: I declared the washout fixed after a 3-minute verification of a
counter — the defect needed 50 minutes to express. Short-window verification of a
time-dependent system proves only the short window.

## ⚠⚠ ROUND 4 (2026-08-08 02:00, game `de34e273`): WHITE-OUT STILL OPEN — clocks EXONERATED

The white-out RETURNED at ~55 min on the fmod build (user caught it live twice). Facts that
now constrain it hard:
- `GPUP_SHOW 0` = normal scene both times → it IS gpup quads.
- Level reload (fresh emitters/textures/CPU-fields) on the FIXED build = **instantly white**
  → process-lifetime state, NOT per-emitter.
- `GPUP_SET_CLOCKS` injection bisect on a fresh process: sn=36000 alone, rotsn=4000 alone,
  agk=4000 alone, AND all three together (45s settle at the steam pose) = **all normal** →
  the three global clocks are ALL EXONERATED.
- Every visible gpup_settings/emitter field audited bounded (spawnint assigned-per-tick,
  testpos wraps, pauser 0..3, noiseOff = Random()/65536 with 16-bit Random(), rnd2 ±32k
  by design from tick one). Constants in the broken-state dump were byte-sane.
- Cadence clean throughout (max_time 3.14 all hour) — not rate, not warps.

REMAINING HYPOTHESES: (a) camera-motion-correlated accumulation — BOTH user occurrences
followed turning/flying the camera around the level, and the static-soak luma column
silently failed so pure-time degradation was never actually confirmed; (b) per-process
GPU/engine-side state not in any dump. INSTRUMENT IN PLACE: GPUP_DUMP now prints the full
per-emitter spawn gate (emiton/active/spawnint/testpos/subpos/pauser/noiseOff/zaehler/
spawnpos/rnd/rnd2/moveit); an alternating static/motion tracker (whiteout_tracker.log,
4-min cycles with full dumps) runs to catch the next organic occurrence with field-level
evidence AND discriminate time-vs-motion.

STANDS from earlier rounds: dt-cap (b1cf18e1), RTT zero-init, hash-seed fmod (c2ea63c4 —
real fp32 degradation, proven by sn=500k invariance, but NOT the only ager).

Round-4b instrument (engine 2.12 `8f85919f` / game `81274365`): **descriptor-ring
forensics** — GPUP_DUMP now carries both shader-visible heaps' allocationOffset, fence
completed value, CPU-GPU gap, ring size, laps, third-step-wait count. Suspect class =
process-lifetime GPU binder state (fits: reload-persistent, restart-cleared,
activity-scaled; gpup is effectively the ONLY slot-bound/GG-rootsig draw path left live).
NOTE: the ring's fence protection IS present (SignalGPU per submit — greppable only as the
struct method, wasted 20 min on a false "never signaled" alarm). Tracker v2
(whiteout_tracker2.log, ring stats every 4-min cycle, auto-stops on catch with full dump)
runs unattended.

ROUND 4c RESULTS (05:00): tracker2 ran 106 min through a FULL ORGANIC WHITE-OUT with ring
stats every cycle: **rings EXONERATED WITH DATA** — res gap 7,098-9,555 of ring 499,906
(≈2 frames), sam gap 208-224 of 1,776, ZERO fence waits, gaps flat from first to last
cycle. DUMP_VRAM in the broken state also CLEAN (2.54 GB census, 6,819 records, nothing
ballooned). The elimination chain now leaves ONLY the shared once-created bindings:
★ **BINDLESS_SAMPLER_CAPACITY = 256 process-wide** — pool exhaustion corrupts every
slot-bound sampler table copy; a leak of a few samplers/min matches the 20-50 min onset
and the activity scaling. Engine 2.12b (`c7d950fd`) adds bindless used/capacity + pending-
destroy for both pools to the stats line; tracker3 (whiteout_tracker3.log, fixed luma via
scratchpad file — ⚠ Windows python cannot see MSYS /tmp, TWO trackers lost their luma
column to that) watches occupancy per cycle. ⚠ tracker3 WAS INVALID — CLICK_ONLY_LEVEL silently failed and the loop fell through on
timeout: 100 min of STORYBOARD, no level, no steam (constant luma 52.61 = the static
storyboard; ★ RULE: a soak/tracker must HARD-ASSERT its target state per cycle, never
fall through a wait loop). Its pool numbers only exonerate the storyboard workload.
tracker4 caught it **4 MINUTES after level load** (07:03) — because the PROCESS was 2 h
old (storyboard idle): ★★ **COMPRESSED REPRO: let MAX idle (storyboard is enough — gpup_update
ticks with 0 emitters) for ~2 h, then load spotshadowtest → white-out within minutes.** The
aging is PROCESS-WIDE and does NOT need emitters alive to accumulate.

FINAL EXONERATIONS AT THE CAUGHT MOMENT (cycle-1 dump): bindless pools FLAT (sam 41/256,
res 20,420/500k, pend 0) — sampler-pool theory DEAD; rings healthy as ever; spawn gate
NOMINAL (⚠ I briefly declared spawnint=87 a smoking gun — MISLABELED comparison, both
healthy quotes were emitter 2; emitter 0's 87/tick is its correct arm-B sustain rate for a
128² pool. Verify arm math before declaring spawn anomalies).

EVERYTHING CPU-VISIBLE IS NOW EXONERATED WITH DATA (verified again on the tracker4
specimen 08-08 morning): clocks cleared by injection in BOTH directions (aged→fresh
process = normal AND fresh→broken process = still white), spawn gate nominal (emitter 0
healthy band 74-93/tick across all tracker1 cycles, broken 87.38 inside it), gpu-consts
byte-identical, whole/split arm-invariant (forced split on broken = still white 205.9),
rings/bindless flat. No PIX/RenderDoc on the machine.

★ CURRENT THEORY + INSTRUMENT SHIPPED (game `8d60eaf3`): texNoiseOrig/texDist2 are the
ONLY gpup GPU state with PROCESS lifetime (gpup_init early-outs on re-entry; per-emitter
sim textures are recreated each level load) and every emitter consumes them EVERY sim tick
(noise pass re-renders per tick from noiseOrig) — poisoned noise ⇒ spawn offsets garbage ⇒
particles EVERYWHERE at nominal spawn rate/size = uniform white fog, exactly the visual.
GPUP_DUMP now reads back every gpup texture (CRC32 + channel means + 0/255 sat%);
noiseOrig/dist2 CRCs are PNG-derived RUN-INVARIANT constants → drift = scribbled, no
same-run baseline needed. GPUP_REGEN re-creates both from PNG live: on a caught white-out,
clearing = poisoning CONFIRMED + durable fix known (re-create at level load); not clearing
= theory dead, per-emitter pos/speed saturation stats in the same dump are the next lead.
★★ ROUND 3 CLOSED (08-08 ~16:00, game `039f244d`, provoke-rig verified): the user's
"provoke it every time by dollying closer/further" = **THE SPLIT UPDATE ARM HALVED SIM TIME
ABOVE 140 FPS** — a DBPro-era bug dormant for two decades: split==0 loop has NO break (ticks
ALL emitters, lastEmitter runs to 9), split==1 ticks NOTHING, both frames consume
half-quanta → delivered sim time = HALF of real. DX11 never sustained >140 FPS (verbatim
same code, never manifested); DX12 sits at 116-141 at the steam poses → camera distance
(overdraw) flipped full↔half-speed regimes = shape flips. FIX: whole-batch arm always
(SET_GPUPARM 2 keeps the heritage arm for archaeology). VERIFIED with tracker11 provoke rig
(dolly sweeps + steam-masked same-pose diffs + crops): pre-fix spike 67.5 vs noise 22 +
room-filling-fog crops + e8 pool α 50.8→69.1; post-fix 1.02× control, arm_split frozen 0,
crops stable. ★ NOTE: the pre-fix "thin wisp" look WAS the half-speed artifact; the correct
full-speed steady state is the billowy column (= DX11's look, since DX11 always ran
full-speed). ★ RULE: FPS-THRESHOLD-GATED code paths are parity landmines — a faster
renderer AWAKENS code the old engine never executed; grep ports for frame-time branches.
★ RULE: for user-provokable defects, build the PROVOKE RIG FIRST (their reproduction steps,
automated, with same-pose masked diffs + saved crops + bracketing dumps) — it caught in one
run what three theory rounds missed.

★ SEED SAGA ROUND 2 (08-08 ~15:00, game `8d0c75a7`, user-reported post-white-out-fix):
"particles fine for a while, then all element positions shift at once, sometimes a whole
new shape" — CAUSED BY the c2ea63c4 fmod itself: mainVSConstantData.rota.z is NOT a hash
seed — MainVS multiplies it into the per-quad rotation angle (mode 0 spins sprite corners;
mode 3 rotates a POSITION offset) → fmod made it ramp 24.6s then snap 256→0 = synchronized
re-orientation. FIX: rota.z = raw DX11-verbatim sn (continuous; 1e9 wrap ≈ 3 years); fmod
stays ONLY on the sim rnd seeds (pure frac(sin(dot())) hash inputs, verified by reading
PosPS/SpeedPS — every rnd use is inside random1()). ★ RULE: before bounding a "seed",
classify EVERY consumer as hash (wrap-invisible) vs continuous-phase (wrap-snaps);
grep the shaders, not the CPU code. ★ HONESTY NOTE: the full-viewport frame-diff detector
could NOT resolve the snap above steam animation noise EVEN ON THE BROKEN BUILD (wrap-frame
diffs 1.31/1.15 vs noise mean 1.69, tracker10/snap_detector.log) — visually salient
synchronized motion can be photometrically tiny; verification here = by-construction
(rota.z was the only time-discontinuous shader input) + user eye. ⚠ The round-3 "hash
degradation at sn≈32k caused the white cloud" narrative is now SUSPECT — those experiments
were confounded by the lens-flare veil (#120); the fmod on sim seeds is kept as cheap
defensive bounding, not as a proven fix.

★★★★★ CLOSED (08-08 14:04, engine 2.13 `118e19d8` / game `c769513d`): **THE WHITE-OUT WAS
NEVER A PARTICLE BUG — gpup's CB binds STOMPED THE CAMERA CB FOR THE LENS FLARE.** gpup's
draw binds constants on shared-binder slots b0/b1; b1 = CBSLOT_RENDERER_CAMERA. DrawLensFlares
(same pass, after the hook) reads GetCamera() WITHOUT rebinding → the sun flare's VS ran
against particle constants misread as the camera → canvas_size_rcp garbage → flare quad
FULLSCREEN from its legitimately off-screen anchor, sampling the flare texture's bright
center → uniform ~88%-opacity veil (out = 197 + 0.12×scene). The 'trigger'/'aging' = the
per-frame back-to-front emitter SORT deciding whose constants sat in b1 (camera pose changes
the sort; 6553.5-unit decihash wrap included). FIX: after EVERY customDraw_* hook,
wiRenderPath3D restores BindCameraCB(pass cameras) + BindCommonResources +
GG_InvalidateCommandListState (tracker hygiene). 3/3 deterministic-repro clean, steam intact,
lensFlare shaders byte-identical to upstream. FALSE FIXES DISPROVEN ON THE WAY (each 3/3
white): state-tracker invalidation alone; flare push→CB conversion alone. ★ RULE: a game
hook recording draws inside an engine pass must restore EVERY implicit binding contract the
pass relies on (camera CB above all); this class corrupts WHICH buffer a slot points at —
invisible to every resource-CONTENT instrument. ★ RULE: enable/disable toggles that change
frame structure (SET_LIGHTSHAFTS, SET_LENSFLARE) can 'cure' such bugs as CONFOUNDS — only
pixel-level shader tints (red/green) discriminate the actual painter. Proof-chain archive:
scratchpad whiteout_tracker5-9 logs + WICKED_ENGINE_CHANGES.md row 2.13.

★★★★ BREAKTHROUGH (08-08 ~12:40): **THE WHITE-OUT IS THE LIGHT-SHAFTS CONTRIBUTION,
NOT GPUP PIXELS.** Chain of proof: (1) rounds 2-3 instruments (buffer byte-compare vs
in-exe arrays, effect-texture CRCs incl gradient_1/imagex/t_field, FULL VS+PS constant
dump, sampler re-creation, currImage) — every gpup input byte-identical healthy-vs-white;
(2) GPUP_CANARY (padding1/filler spare constants → every rasterized gpup pixel PURE RED)
changed NOTHING in a live white-out → gpup shaders rasterize none of the white (and the
steam rasterizes ZERO visible pixels at the user's pose even healthy — occluded);
(3) SET_LIGHTSHAFTS 0 on the live white instance: 199.1→40.3 INSTANT. Live interplay:
white requires BOTH gpup_draw executing AND the shafts contribution (either off = clean,
toggleable in real time). ★ DETERMINISTIC 3-MIN REPRO: fresh boot → TESTPRO2/spotshadowtest
→ SET_CAMERA -4368 169 -9800 5 180 (camera AT the steam pipes) → back to user pose =
latched white. NO AGING EXISTS (tracker5b: 250 min idle clean; tracker6: fresh process
whited on motion cycle 1 — every earlier 'aging' was waiting for a camera to pass the
trigger region). Game `4377b0d2` = canary + all instruments. IN FLIGHT: engine
GG_DumpSunChain (rtSun[0] DrawSun mask / rtSun[2] downsample / rtSun[1] blur out stats)
+ DUMP_SUN harness cmd + tracker8d three-snapshot run (healthy/white/SHOW0) to name the
stage that goes white and how gpup draws feed it (suspect: DrawSun reads engine state the
transparent-pass gpup draws perturb cross-frame; the contribution draws BEFORE the hook).

★★★ ROUND-1 CATCH VERDICT (08-08 ~11:48, motion arm cycle 1): **THE SIM IS INNOCENT,
THE DRAW IS THE VICTIM, MOTION IS THE TRIGGER.** Inside a live white-out (luma 199 at the
user pose): pos/speed pool readback stats == healthy baseline (pos α-mean 60-62 vs 57-59,
sat identical), noiseOrig/dist2 CRCs UNCHANGED (texture-poisoning theory REFUTED),
REGEN(textures) did NOT clear, GPUP_SHOW 0 → 40.3 (quads confirmed). A HEALTHY particle
population renders +159 luma. Once triggered it stays white at a static camera. By
elimination the only process-lifetime draw inputs left = the FIVE STATIC BUFFERS from
compile-time arrays (grid VB/IB ×2 + sim quad VB, created once in gpup_init). ROUND-2
INSTRUMENT (game `fc067138`): GPUP_DUMP byte-compares all five against their in-exe source
(diff offsets + 32-byte hex fingerprint of the scribble); GPUP_REGEN re-creates them too;
draw-globals line checks mainIndexCountObj0/1 vs compile-time expectations. tracker6 =
fresh boot → level → continuous motion from minute zero (time-to-white measures the aging
requirement; round-1 aged instance whited on motion-cycle 1 after 250 min).

★★ TRACKER5 RESULTS (08-08 late morning): the IDLE ARM IS NEGATIVE — fresh process,
100 min storyboard idle (CRCs polled every 10 min: stable) + 150 min in-level with ZERO
camera movement (luma flat 40.3, all sim pools nominal, 4.17M sim steps, split arm
exercised 141k times) → NO white-out. This REFUTES the 'process age alone' reading of
tracker4's 4-min-post-load catch — that process carried ~2h of LIVE emitter + editing
history (tracker2's session) before its storyboard idle. Every real occurrence followed
camera flight (user 2×, tracker motion cycles). tracker5c (same aged instance) is the
MOTION arm: 8 teleport jumps/cycle around the scene (VT/chunk churn + FPS swings), return
to the user's saved pose, luma read, same catch/REGEN verdict block. Baseline healthy
texture stats for comparison: pos alpha-mean ≈57 sat0≈17%, speed alpha-mean ≈40
sat0≈20.5%, per-emitter noise mean ≈127.5 sat≈0%.

★ LOAD RECIPE: spotshadowtest is a LEVEL inside project TESTPRO2 — `OPEN_PROJECT TESTPRO2` +
`CLICK_ONLY_LEVEL` (OPEN_PROJECT takes the PROJECT name; passing the level name silently
leaves MAX at the hub — cost tracker5 run #1 its load phase).

tracker5 (scratchpad tracker5.py → whiteout_tracker5.log) runs the WHOLE experiment
unattended: boot CRCs → 100-min storyboard age with 10-min CRC polls (a flip during aging
catches the poisoning moment pre-visually) → load spotshadowtest (hard-asserts loaded>=1
via GPUP_DUMP) → 60s luma watch → on catch: dump + REGEN verdict + GPUP_SHOW re-confirm,
MAX left alive.


## ★★★ THE PING — CLOSED 2026-08-08 evening (game `82959a2b`, tracker22-verified 0/60s + 10-min soak)

**ROOT CAUSE: shared-CB copy RACE in the sim.** gpup_doit updated ONE shared constant
buffer per pass type (posConstants/speedConstants/noiseConstants) via `UpdateBuffer` for
all 9 emitters per tick — copy→pass→copy→pass, NO barriers. Wicked DX12 UpdateBuffer =
AllocateGPU+CopyBuffer, "appropriate synchronization is expected" (CALLER'S job); D3D12
implicit buffer-state promotion means no validation error anywhere — the GPU overlaps
emitter B's copy with emitter A's still-running pass, so passes read LATER emitters'
constants INCLUDING SPAWN WINDOWS. Every pool consumed up to 9 foreign windows/tick:
e2 churned 2275-6200 slots/s vs its 227/s cursor, degenerated into ONE age-synchronized
cohort; the cohort's simultaneous death = whole-cloud one-frame vanish = "the PING".
Same race through speedConstants = the measured cloud-wide velocity kicks. **DX11 was
immune: UpdateSubresource has implicit hazard tracking — the port lost the sync
SILENTLY.** Fix: `BindDynamicConstantBuffer` per pass (transient upload, no shared dest)
— the identical pattern the draw path got in the 08-07 device-removal fix. Same fix in
TracerManager's per-tracer loop (same class). Terrain has 2 more candidate sites
(pageGenVertexBuffer ×2, terrainConstantBuffer ×2) — spawned as a follow-up chip.

**★ RULE (the big one): D3D11→D3D12 ports must audit EVERY UpdateBuffer/UpdateSubresource
call site for the implicit-sync assumption. The pattern "one shared buffer, updated
between consumers inside a frame" is CORRECT on D3D11 and SILENTLY RACES on D3D12 —
no debug-layer error (buffers get implicit state promotion), load/driver-timing
dependent, activity-correlated.** Grep `UpdateBuffer` and demand: once-per-cmd-list, or
transient allocation, or explicit barriers.

★ RULE: when a per-window mechanism produces N× its commanded rate, the window is being
applied N times — count the CONSUMERS, not just the producer (CPU counters proved 1
window/tick while the pool ate ~10; the delta had to be foreign windows).

★ RULE: an age/lifetime POOL's health is judged by its AGE-STRUCTURE (staggered band =
healthy, synchronized cohort = something mass-respawns). GPUP_AGES dumps the full plane;
birth masks between snapshots name the spawn geometry. Position/age smoothness per-slot
is NOT enough — my journey tracker read individual trajectories as "smooth" for two
rounds while the POOL-WIDE structure screamed cohort.

★ The exoneration chain that finally landed it (all instruments committed):
canary bisect (C1 dots=0 vs full=38/60s → appearance path; ALSO exonerated CB delivery,
PSO/blend, pool descriptors, s1 in one stroke) → subtractive modes 3-8 (size-gradient
dominant, everything else exonerated; probe 8 = t2 descriptor healthy on-screen) →
GPUP_REBIND decoy A/B-null (binder exonerated) → GPUP_AGES birth masks (foreign
window-shaped blocks = the race). Two DEAD theories killed by honest tests: warp-spike
(chance-level correlation) and stale-descriptor-table (probe + rebind null).
⚠ A subagent's engine audit FABRICATED its centerpiece (sampler hashmap cache that
doesn't exist in this vintage) — its second pass with line citations was excellent.
VERIFY AGENT CLAIMS BY GREP before acting.

⚠ tail -f monitors on Windows emit PHANTOM partial lines (mid-flush reads with wrong
numbers) — always re-read the log FILE before acting on a monitor event's numbers.

Verification: tracker22 (age extras 0/60s worst-snapshot 0; video pings 0/60s vs 43-50
pre-fix) + tracker23 10-min soak. New harness rows: GPUP_AGES, GPUP_REBIND, GPUP_TRACK,
GPUP_SOLO, GPUP_DRAWLOG, GPUP_CANARY 0-8 — all in WETEST.md.

## Open

- **User's eye on parity** (density/spread vs DX11 — the DX12 column looks DENSER than
  the DX11 reference shot at first glance; could be pose/wind phase).
- ~~VRAM sweep re-run~~ **CLOSED same session (results_0807 vs 0806): 4 GB gate HOLDS on
  all 19 demos** — worst Aztec Game Kit 3965.8 MB (+160.8, ≈ the predicted 45-emitter cost;
  margin 130 MB), total +754 MB across the hub, deltas tracking emitter counts. Game FPS
  flat-to-up except −4-8% on the three emitter-heavy demos (Aztec/Snowy/ISF) = restored
  content cost, not a regression.
- 60-min post-fix soak curve: scratchpad soak_curve.log (running 01:00-02:00 08-08).
- Test-game arm (bParticle_Show_At_Start) + weapon-impact .arx effects — first pass done
  same session (see MEMORY banner for outcome).
- gpup_draw_bydistance interleave parity (engine callback) — only if a user reports
  smoke-over-glass ordering.

Related: [[project-wpe-particles]] (the OTHER particle system + the bWPE boundary),
[[project-light-falloff-parity]] (same session), [[feedback-instrument-before-theory]].
