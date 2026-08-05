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

**Ranked remaining hypotheses + discriminators (each ~15 min with the soak harness):**

1. **WPE particle simulate CS reading `texture_depth_history`.** The depth-chain targets
   (`depthBuffer_Copy1` 9×, `rtLinearDepth` 2×) are in the freed-match list of EVERY dump;
   `emittedparticle_simulateCS.hlsl:197` reads that binding for depth collisions on the
   compute queue; WPE particles are the newest re-enabled system (2026-08-04/05, deltas
   2.00-2.02) and Aztec torches emit continuously near spawn. A stale camera CB /
   texture_depth_index_prev after the game-mode RenderPath re-init would read the FREED
   old depth buffer. Discriminator: soak with all emitters suppressed (harness command to
   pause/kill emitters, or preload_wicked_particle_effect force-disable) — if clean,
   particle path convicted.
2. **Stale ShaderMaterial texture descriptor surviving the set swap** (terrain material
   whose texture resource died without the material buffer entry being rewritten).
   Discriminator: at swap time, walk scene materials and log any whose texture descriptor
   index matches a just-freed texture.
3. **Somebody else entirely on the graphics queue** — needs per-queue fault attribution;
   DRED breadcrumb contexts do not decode on this driver (ANSI and UNICODE embedded
   markers both tried), so shape-matching is the only naming tool.

Note: mat18 + the depth target matching the same fault VA = D3D12MA heap reuse — both
occupied overlapping VA ranges at different times; the most recent occupant is what the
faulter believed it was reading.

## Open items

- [ ] `wilog_messagebox` has no automation suppression — modal boxes block the game (not
      the soak's traps, which read files); consider a headless flag later
- [ ] DRED context strings: neither PIX3 blobs nor raw ANSI/UNICODE embedded markers are
      decoded on this driver (NVIDIA) — pass naming abandoned for now; op-shape matching
      documented in WETEST.md instead
