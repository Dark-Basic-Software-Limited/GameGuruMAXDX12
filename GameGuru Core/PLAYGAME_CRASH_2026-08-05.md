# PLAY GAME crash investigation — Aztec Game Kit Teaser (2026-08-05 evening)

User report: fresh MAX → hub → Aztec Teaser → **PLAY GAME** → title menu → **START** →
(1) "Device Lost on Present 0x887a0006" + "D3D12: device removed, DXGI_ERROR_DEVICE_HUNG"
during LOADING LEVEL; (2) second launch crashed with NO dialogs; (3) third launch played a
short while, then LUA ERROR `.\titlesbank\gamedata.lua:288: bad argument #1 to 'close'`.

## What each of the three crashes actually was

| Time | Event | Root cause | Status |
|---|---|---|---|
| 20:13:19 | DEVICE_HUNG during LOADING LEVEL (dred dump 1) | GPU page fault: SVT compute list read freed terrain-material memory (below) | **Reproduced 6/6 by soak; hunt live** |
| 20:14:23 | Silent crash relaunching | `CreateCommandQueue` failed (driver still TDR-recovering) → `wi::platform::Exit()` is only PostQuitMessage and RETURNS → `SetName` on null queue (wiGraphicsDevice_DX12.cpp:2963) | **FIXED — engine 2.03(b)**: all 24 ctor failure sites now fatal-exit cleanly |
| 20:14:39 | DEVICE_HUNG again + AV in `DescriptorAllocator::allocate` (h:535) during `MasterRenderer::Load` | `CreateDescriptorHeap` fails post-removal → null-heap deref | **FIXED — engine 2.03(c)**: routes to `OnDeviceRemoved()` (DRED report) + clean exit |
| ~20:16 | LUA ERROR gamedata.lua:288 | **No `savegames` folder exists anywhere in the build area** → `io.open("savegames\\gameslotN.dat","w")` returns nil on every checkpoint save; `io.output(nil)` silently no-ops; `io.close(nil)` throws | **FIXED 3 ways** (game `00b60ee0`): `game_masterroot_initcode` creates the folder at game entry; `gamedata.save` nil-guards (both repo copies + build area); folder created |

## The primary bug: GPU use-after-free in the SVT terrain pipeline

**Reproduction: 6/6 soak cycles** (hub → PLAY → START → idle at spawn) hang the device
within 45 s of gameplay. Signature identical across all six dumps AND the user's 20:13 dump:

- Page fault VA ~9.03 GB region, every time in the same D3D12MA heap
- DRED "recent freed matches": `Files/terraintextures/mat18/{Color|Normal}.dds` +
  `depthBuffer_Copy1` (once `rtLinearDepth`) + the containing heap
- One QUEUE_COMPUTE list hung mid-execution at op ~100 (98–120) of ~510–580: shape =
  nested BeginEvents → ~12-dispatch group ("Update Residency Maps") → EndEvent →
  BeginEvent → long dispatch run = **`Terrain::UpdateVirtualTexturesGPU` "Render Tile
  Regions"** — the pass that samples terrain material textures bindlessly per tile.

Interpretation: the level-load terrain material swap (`SetupWickedTerrainMaterials`,
triggered by `GGTerrainWicked_OnTextureSetChanged` during load) frees the previous set's
DDS textures (weak resourcemanager cache — they die the moment the MaterialComponent
handle drops). Something in the SVT tile-render path still resolves to **material 18's
dead texture descriptor** when the player's VT feedback finally requests the tile that
needs it (~30–45 s in, deterministic). depthBuffer_Copy1 in the freed-match list dates to
the same load (RenderPath re-init) and shares the heap — the heap became unmapped when its
last occupant died, which is why the access faults instead of reading garbage.

Mitigation shipped this session (game `GGTerrainWicked.cpp SetupWickedTerrainMaterials`):
`WaitForGPU()` before the swap — closes the in-flight-work race window for the load-time
hang (the user's 20:13 shape). The mid-play fault indicates ADDITIONALLY a stale
descriptor held across frames; soak run 2 (guard + named breadcrumbs) discriminates.

**Why mat18 specifically**: consistent across user + all soak cycles — the painted-layer
blend data of the Aztec level references a material whose slot mapping/live entity no
longer carries live textures after the set swap. (Slot bookkeeping: `materialToSlot` /
`maxPaintedSlot` / `terrain->materialEntities` in GGTerrainWicked.cpp; on re-setup, extra
painted slots are overwritten with fresh entities and the old MaterialComponents are
orphaned — never Entity_Remove'd.)

## Instruments built (all committed)

- **Engine 2.03(a)** — DRED pass names: with `dred.txt` armed, EventBegin/SetMarker emit
  raw embedded markers so dred_report.txt names the pass per breadcrumb op (WinPixEvent
  PIX3 blobs are not decoded by DRED). ANSI form produced nothing on this driver; switched
  to UNICODE metadata. Also fixed the dump's context loop (started at firstOp; slots ≠ op
  indices).
- **Harness standalone-game emulation** (WETEST.md section): hub `CLICK play_game`
  (relaunches exe as `project=2` standalone — expect harness silence then a new process),
  `TITLE_CLICK start|exit|continue|back|resume|leave` (fires screen_editor widgets through
  the real click path), `GET_STATE` → `standalone_title|standalone_loading|
  standalone_playing` + raw STANDALONE flags line.
- **Soak driver** `playgame_soak.sh` — full user-flow cycles with three traps per poll:
  dred_report growth (written BEFORE the modal box), Guru-Crash.log growth, process death.
  Captures new dump bytes per cycle.
- Known limitation: `RUN_LUA` hangs the harness poll in standalone mode (main-thread hang,
  probe never executes). Editor/test-game use unaffected. Do not use in soak scripts.

## Soak run 2 (WaitForGPU guard) + the mechanism nailed

Run 2 (guard build): cycle 1 CLEAN (first ever), cycles 2–3 device-removed with the
IDENTICAL signature, cycle 4 process died. So the load-window race was real but small;
the dominant mechanism survives mid-play. Neither ANSI nor UNICODE raw embedded markers
produce DRED context strings on this driver — pass identification rests on the op-shape
match, which is unambiguous.

**Root cause (static, every piece fits the dumps):**
`Terrain::UpdateVirtualTexturesGPU` "Render Tile Regions" builds a per-layer
`material_index` buffer at record time from `scene->materials.GetIndex(materialEntities[i])`
(wiTerrain.cpp:2394-2405). Two unguarded holes:

1. `GetIndex(entity)` returns SIZE_MAX for an entity without a live MaterialComponent →
   `material_index = 0xFFFFFFFF` → the CS reads a ShaderMaterial miles out of bounds →
   garbage texture descriptor → page fault at a DETERMINISTIC garbage VA (matches the
   same-VA-every-run signature; "mat18" is simply the corpse that used to live there).
2. `baseMaterialCount` comes from the CHUNK's blendmap array size — if a chunk carries
   more layers than `materialEntities.size()`, `materialEntities[i]` was an OOB vector
   read before the entity check even ran.

**How dead entities get into `materialEntities`:** `SetupWickedTerrainMaterials` never
truncates the vector on a set swap. A previous set with MORE painted materials (title
scene / previous level) leaves stale tail slots; their entities/components/textures die
with the old level, while regenerated chunk blendmaps still span those tail layers. ~30-45 s
into play, VT residency finally requests a tile touching a tail layer → boom. Deterministic.

## Fixes shipped

- Game `GGTerrainWicked.cpp SetupWickedTerrainMaterials`: `WaitForGPU()` before the swap
  (load-window race) + **truncate materialEntities to the new set and Entity_Remove the
  stale tail** (structural fix).
- Engine 2.04 `wiTerrain.cpp`: bounds-clamp both holes (component-less entity → material 0;
  layer beyond registered entities → material 0), with a one-shot backlog line naming the
  slot/entity when it fires — the belt-and-braces guard that also protects stock content.

## Soak run 3 verdict (clamp + truncation): NOT the (whole) story

Run 3 (engine 2.04 clamp — string-verified present in the exe — + game truncation +
WaitForGPU): 4 cycles → 2 DEVICE_REMOVED (identical signature: fault VA same heap region,
freed matches mat18 + rtLinearDepth/depthBuffer_Copy1, compute list hung ~op 98 of ~526)
+ 2 silent process deaths during play (NO dred growth, NO crash-log entry — possibly the
game ending by design: an idle player killed → storyboard flow exit relaunches the editor;
needs a GET_STATE probe at death to classify).

Conclusion: the RECORDED material_index path is exonerated (clamped and the fault
persists). The hung SVT list may be a VICTIM — DRED's page fault is global, and the
faulting access can come from other in-flight work on any queue.

**Soak run 4 (KILL_EMITTERS discriminator): PARTICLES EXONERATED.** The new harness
command reported **"removed 0 emitters"** — standalone Aztec play has NO
EmittedParticleSystems at all — and the hang still fired with the identical signature.
The WPE-particle hypothesis is dead (and WPE re-enablement is NOT the regression source).

**Timing refinement**: hang dump timestamps vs cycle starts put the fault at **~5-10 s
after entering the level** (user's original: during LOADING LEVEL itself) — the danger
window is the load/residency storm, not steady-state play.

**Ranked remaining hypotheses + discriminators:**

1. **Temporal/history reader with a stale `texture_depth_index_prev`** after the
   game-mode RenderPath re-init (`MasterRenderer::Load` → ResizeBuffers frees the OLD
   `depthBuffer_Copy1`/`rtLinearDepth` — both appear in the freed-match list of EVERY
   dump, 11/11). If the first game frames consume a camera CB / bindless index captured
   pre-resize, any depth-history read (TAA temporal, disocclusion checks) hits the freed
   target. Discriminator: disable TAA/temporal effects for the standalone session and
   soak; or force a full temporal-history invalidation after MasterRenderer::Load.
2. **Stale ShaderMaterial texture descriptor surviving the terrain set swap** (texture
   died, material buffer entry not rewritten). Discriminator: at swap time, walk scene
   materials and log any whose descriptor index matches a just-freed texture.
3. **Graphics-queue faulter with the compute list as victim** — several graphics lists sit
   at lastCompletedOp=0 in each dump; DRED contexts don't decode on this driver, so
   shape-matching is the only naming tool.

Note: mat18 + the depth target matching the same fault VA = D3D12MA heap reuse — both
occupied overlapping VA ranges at different times; the most recent occupant is what the
faulter believed it was reading. No dump has an ACTIVE object at the fault VA (11/11
pure use-after-free on unmapped memory).

## Round A (engine 2.05 depth keep-alive): DEPTH CHAIN EXONERATED

With DRED armed, `RenderPath3D::DeleteGPUResources` retains every generation of
`depthBuffer_Main/Copy/Copy1/rtLinearDepth` for the process lifetime. Fault STILL fires —
and the freed-match list changed to **mat5/Color.dds + rtParticleDistortion + Heap** (the
depth targets can no longer appear since they are never freed). Conclusions:

- The depth-history chain is NOT the faulted resource (hypothesis 1 dead).
- The freed-match NAMES rotate with allocation layout (mat18 → mat5, depth →
  rtParticleDistortion) — they are corpses sharing the released D3D12MA heap, not a stable
  identification. The stable facts remain: an SVT-shaped compute list hung mid-dispatch +
  a released heap in the ~9.0 GB VA region containing mid-size level-load-freed textures.

## Round B (engine 2.05 leakall.txt): the decisive dichotomy

`leakall.txt` next to the exe makes `AllocationHandler::Update` retain EVERY deferred
destroy — no VA ever unmaps for the whole session. Two possible verdicts:
- Hang STOPS → the faulter dereferences a legitimately freed object → bisect classes
  (content textures / render targets / terrain chunk VT buffers) with targeted keep-alives.
- Hang PERSISTS → the faulter reads memory that was NEVER mapped → garbage or corrupt
  descriptor (OOB descriptor index / corrupted ShaderMaterial), a different bug class.

**VERDICT: CLEAN — 3/3 cycles, 60 s play each, zero device removals, zero crashes, and
even the previously-unexplained silent process deaths vanished** (same fault, different
presentation). The faulter dereferences a legitimately freed object. Note: intermediate
attempts were disrupted twice by driver-side AVs inside D3D12CreateDevice when booting
too soon after a TDR (the soak now waits 25 s after a trapped cycle) — same crash class
as the user's silent crash #2, but inside the driver where no app guard can reach.

## Round C (leakterraintex.txt): pin ONLY terraintextures/* resources

wiResourceManager pins every resource whose path contains "terraintextures" (the flag
file gates it). leakall.txt removed. If clean → the faulted resource is NAMED: a terrain
material DDS freed at the level-load set swap, sampled by the SVT "Render Tile Regions"
CS (its only consumer). If it hangs → next class (terrain chunk VT buffers).

**VERDICT: CLEAN — 3/3 cycles, 60 s play each, zero traps.**

## ★ THE FAULTED RESOURCE, NAMED ★

**The terrain material source textures — `Files/terraintextures/matNN/{Color,Normal,
Surface}.dds` — freed at the level-load material set swap.** Reader: their only GPU
consumer, the SVT **"Render Tile Regions"** compute pass in `Terrain::
UpdateVirtualTexturesGPU` (matches the hung-list op shape in all 13+ dumps).

Evidence chain (keep-alive bisection):
1. 13+ dumps: released-heap fault, matNN DDS in every freed-match list.
2. Round A (depth chain pinned): hang persists → depth exonerated; freed-match names
   shown to rotate with layout (they are heap corpses, only the DDS class is constant).
3. Round B (everything pinned): 0 hangs in 6 cycle-equivalents (previously ~85%/cycle).
4. Round C (ONLY terraintextures pinned, all else freeing): 0 hangs in 3 × 60 s.
   P(9 consecutive clean cycles | bug active) ≈ 10⁻⁷.
5. The 2.04 clamp NEVER fired → material indices always valid → the staleness lives in a
   GPU-side ShaderMaterial texture descriptor still pointing at the freed DDS.
6. WaitForGPU at the swap did NOT prevent it → the stale descriptor is persistent data
   surviving a full drain, not an in-flight-work race.

## The ship fix (game `GGTerrainWicked.cpp SetupWickedTerrainMaterials`)

**Double-buffered retention**: before the swap replaces textures, the outgoing set's
`wi::Resource` handles are moved into a static retention vector that is only released at
the NEXT set swap. The outgoing DDS can therefore never die mid-play, whatever stale
descriptor still references it. Cost: one extra material set (~a few MB) kept during
play; released at the next level load. Final validation: ship config (no diagnostic
flags), 5-cycle soak — verdict in the addendum below.

Residual engineering note (root-root, non-blocking): WHICH ShaderMaterial entry keeps the
dead descriptor is still unproven — candidates are the terrain generator's deep-copied
material snapshot (`Generation_Restart()` "deep-copies the materials internally") writing
stale entries for newly generated chunks, or an orphaned material entity's buffer slot.
With retention in place this is unreachable; if desired later, dump ShaderMaterial
texture descriptor indices vs the retained textures' indices at swap+N frames.

## Open items

- [ ] `wilog_messagebox` has no automation suppression — modal boxes block the game (not
      the soak's traps, which read files); consider a headless flag later
- [ ] DRED context strings: neither PIX3 blobs nor raw ANSI/UNICODE embedded markers are
      decoded on this driver (NVIDIA) — pass naming abandoned for now; op-shape matching
      documented in WETEST.md instead
