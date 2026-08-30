---
name: project-reload-corruption
description: "The in-place level-reload / travel-churn rendering corruption saga (2026-07-25/26) — what was proven, what was fixed, what is still open, and the automated repro rigs."
metadata: 
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
  modified: 2026-07-26T17:15:10.511Z
---

# Reload / travel-churn rendering corruption

> **ROOT CAUSE FOUND, FIXED & TESTBENCH-VALIDATED 2026-07-26 evening — delta 1.47 (engine
> `8570d144`, game docs `a70d8a2c`).** Wicked's MSVC port of sebbbi's OffsetAllocator
> (`Utility/offsetAllocator.cpp` `insertNodeIntoBin`) rewrote the upstream aggregate node
> reset as three field writes, leaving `used`/`binListPrev`/`neighborPrev/Next` STALE on
> every recycled free-list node → refused merges, stale-pointer unlinks, self-looping bin
> lists, the same page range granted to multiple live meshes → GPU byte stomps with correct
> CPU records (ALL the slab/wedge/missing-chunk corruption) + the E_INVALIDARG device
> removals. Proven deterministically: lossless 1.46c op capture (user's triple-level repro)
> corrupts the unpatched allocator at a legal free in a standalone testbench (in-repo:
> `GameGuru Core/alloc_testbench/`, per-op node-graph integrity validation); the one-line fix
> (`m_nodes[nodeIndex] = Node();`) replays the same 7629-op workload with ZERO failures.
> **UPSTREAM-WICKED-WORTHY report (port bug, like 1.19). Automated acceptance gauntlet on the
> fixed build PASSED same evening: 40× camera churn + 2 reloads + 30× churn + 2 unbudgeted
> burst-verify storms = 42,151 allocator ops, ZERO violations, only the known-benign 204
> vb_bon wobble class in verify.**
>
> **✅ USER-CONFIRMED RESOLVED 2026-07-26 ~19:00: the user's own triple-stack repro (fly-far →
> 2nd level → leave mid-generation → Island Showdown → zoom) ran CLEAN — "my triple test
> worked FINE :)". Final session evidence: 75,958 allocator ops, ZERO violations; 620/620
> chunks byte-perfect. TOPIC CLOSED. Residuals: (1) tripwire left ARMED by choice (cost ≈
> 200KB of log per load, one mutexed file append — delete `alloc_tripwire.txt` in the exe dir
> occasionally; mute someday via `wi::allocator::gg_alloc_tripwire=false` + rebuild); (2) the
> upstream WickedEngine bug report is UNSENT — offsetAllocator.cpp insertNodeIntoBin port bug,
> testbench + workload in `GameGuru Core/alloc_testbench/` make a complete repro case; (3) the
> old open-items list below predates the root cause — the within-level texture swap and
> churn-slab items are EXPLAINED by this bug; the +3 ghost-human material leak and the
> 4MB applytransform guard spam remain real minor follow-ups.**

Symptoms the user reported: giant grey-blue **slabs and wedges** across the sky, **stretched
ribbons**, **chunk-shaped holes** in mid-distance terrain, the **terrain chunk under the
player gone**, a **wood-plank-textured pyramid**, and **blue/purple palm trees**. Intermittent,
worsening with session length. One AMD `DXGI_ERROR_DEVICE_HUNG` / `Device Lost on Present`.

## What was PROVEN (don't re-litigate)

1. **NOT caused by the P.5 performance push.** A build from the exact pre-push commits
   (engine `de50638b` + game `b09b9edb`) reproduces the identical corruption under the same
   provocation. All P.5 knob deltas were individually exonerated by user-manual bisect rounds
   (1.36 hierarchy, 1.33 VT-incremental, 1.40 list-merges, 1.41 material cache, 1.30 apparent
   cull, 1.35 anim-visibility-pause). The push's ~50% faster streaming only made a
   pre-existing bug much easier to hit. **The 07-19 note about Island Showdown showing "blue
   stretched geometry when loaded in-place on top of another session; clean restart perfect"
   was this same bug all along.**
2. **The CPU-side data is innocent, end to end.** Verified with purpose-built harness dumps
   at load and after each reload: scene component census stable, ZERO dangling references
   (mesh/material/buffer), tree pool perfectly healthy (6000/6000 bound+renderable, 0
   orphans), material texture names + descriptors correct, per-instance `ShaderMeshInstance`
   records correct (uid/color/geometryOffset/transform), `ShaderGeometry` records correct
   (materialIndex == expected). Correct data, wrong pixels.
3. **The corruption is GPU-side CONTENT.** `REUPLOAD_TEXTURE` (SetOutdated + re-Load from
   disk) **healed the blue palms in place** — the texture objects held another file's bytes
   (visually: their own normal map showing as basecolor) while every name/descriptor was
   right.
4. **The texture-STREAMING system is a writer.** With streaming paused from app start
   (`SET_STREAMING 0`), 3 stacked reloads produced ZERO blues and a fully correct scene.
5. **The engine deferred-destroy recycling is NOT the mechanism.** A full GPU quiesce +
   backlog drain between teardown and creation released ~16,500 items per reload and changed
   nothing. (Worse — see the device-lost warning below.)

## What was FIXED (all pushed)

| Delta | Commit | What |
|---|---|---|
| 1.42 | engine `683fa37c` | `material->SetDirty()` at the end of the terrain VT (re)bind block — the 1.41 cache could serve a stale composition for up to 63 frames after a rebind (grey / foreign chunk textures) |
| 1.44 | engine `3af8655c` | `GGReloadGuardBegin/End` — pause streaming, join the streaming job, drop pending texture replacements computed against the dying session, resume after load. Kills the deterministic reload texture corruption |
| 1.45 | engine `12a64a39` | **Join `Wait(virtual_texture_ctx)` before every terrain VT teardown site** (chunk-removal branch, `Generation_Restart`, terrainEntity==INVALID). Fixes a real use-after-free: the async VT job holds raw VT pointers and mutates the shared tile atlas while chunks were being freed underneath it. Targets the user's travel-churn repro |
| — | game `17ed34b9`, `2ee384f9` | Game-side hooks (`WickedCall_ReloadQuiesceGPU/End` in `gridedit_load_map` / case 502) + the forensic harness toolkit |

### ⚠ DO NOT re-enable the 1.43 deferred-destroy flush
`GraphicsDevice::FlushDeferredDestroys()` exists in the engine but its **call site is gated
off** (`gg_enable_deferred_flush=false`, game `2ee384f9`). `gridedit_load_map` runs
**mid-frame** (it pumps messages and renders during the load), so releasing the whole backlog
there can free resources that recorded-but-unsubmitted command lists still reference. The
user's `DEVICE_HUNG` appeared only after that delta shipped, and the harness repro then
crashed 2/2 during level load. It never fixed any corruption either. Only reconsider it if it
can be called at a true frame boundary.

## BREAKTHROUGH 2026-07-26 (live forensics on the user's failed 1.45 soak)

User ran the travel-churn repro on the 1.45 build ("a grand canyon adventure" as the second
level): **corruption reproduced** — red murk + giant wedges, user identified them as TERRAIN
geometry. Live probes on the running instance: DUMP_BROKEN clean (9202 objects, 0 dangling),
then **`REUPLOAD_ENTITY chunk_` (841 chunk meshes, CreateRenderData re-upload) HEALED the
terrain in place** — murk lifted, wedges gone, correct textures/lighting returned. So the
travel-churn corruption = **terrain chunk mesh GPU vertex/index buffer CONTENT corruption
with correct CPU arrays** — the geometry twin of the 1.44 texture finding. 1.45 (VT job join)
addresses the texture side of terrain teardown, not this. Suspect: the mesh buffer
upload/copy-queue path racing chunk destroy/create churn (pending copies vs recycled
heap blocks), same family as the 1.43 mid-frame-load danger note.

## PRIME SUSPECT NAMED (2026-07-26 afternoon): the shared GPU mesh suballocator

Every mesh + hair buffer lives suballocated inside shared 256MB blocks
(`wi::renderer::SuballocateGPUBuffer` → `wi::allocator::PageAllocator` → vendored
OffsetAllocator; each mesh's `generalBuffer` is a placed ALIAS at a page offset). Delta 1.46
(engine `abcfbd0e`) instrumented it and one island load caught **596 OVERLAPPING grants +
172 OUT-OF-BOUNDS grants**: OOB = the `CreateAliasingResource` E_INVALIDARG →
`DXGI_ERROR_INVALID_CALL` device removal (hit twice today under mass-rebuild storms — likely
also the user's AMD DEVICE_HUNG); OVERLAP = two live meshes sharing bytes → whichever uploads
last stomps the other, all records correct. **INTERMITTENT**: 6+ subsequent loads and 3
churn+reload cycles (27K ops) replayed 100% clean (`replay_alloc.py` in session scratchpad
validates the A/D/F/R op log); the storm run was the shader-recompile-burdened first launch →
timing-sensitive race, trigger conditions not yet pinned. Vendored OffsetAllocator == upstream
main (no known-fix divergence; upstream issues #3/#6 unrelated/bogus).

**1.46 guards now shipped**: OOB grants rejected gracefully (fallback standalone buffer — no
more device removal from this path), overlap tripwire + full op log → `alloc_tripwire.txt`
(lands in `Files/` after load), freed ranges held +8 extra frames, `is_empty()` lock fix.

**ORGANIC CAPTURE 2026-07-26 15:35 — ALLOCATOR CONVICTED.** The user's own plain island load
(fresh MAX launch) fired the trap: **1369 OVERLAP + 411 OOB grants**, and the op history shows
the free-list bin SELF-LOOPING — offset 129 granted 20+ times consecutively with NO frees
between (aftermath of a double-free double-inserting a node into a bin; corruption began in an
IDLE instance ~40 min after a clean load — background grass/chunk churn suffices).
`VERIFY_MESH chunk_` on the corrupted session: **294/1012 terrain chunk buffers held FOREIGN
bytes** (chunk_5_14's index buffer contained float vertex positions; its own positions were
zeros) — healed in place, FPS 7.3→64.7 (buffer corruption also tanks framerate). Archived log:
scratchpad `tripwire_USER_ORGANIC.txt` (may be gone; findings recorded here + repo docs).
**Delta 1.46b (engine `1b7709ae`, game `1ccb18fc`)**: op log now carries `m=<nodeindex>
t=<threadid>` (next capture shows the node double-free + colliding threads DIRECTLY;
`GameGuru Core/replay_alloc.py` — now IN THE REPO — models nodes too), plus
`Allocation::Reset()` steal-guard (candidate mechanism: racing double-Reset double-decrements
the refcount). **NEXT: deploy needs a game re-LINK once MAX is closed (engine lib already
built); then idle-soak or normal use until the tripwire fires again — the m=/t= log should
name the exact double-free and its thread pair. If Reset-guard alone stops recurrence, that
was the mechanism.**

**ORGANIC CAPTURE #2 (same day, 16:17) — user's triple-stack repro (fly-far → 2nd level →
fly-far leaving mid-generation → Island Showdown) fired again on the 1.46b build: 1024
overlaps, node-level log shows node 770 granted 20+× consecutively. TWO lessons: (a) the
Reset() steal-guard did NOT prevent poisoning — that race is NOT the mechanism; (b) 982 log
lines were TORN (per-call fopen appends from 7+ threads — main ~24K ops + 6 job workers) and
the poisoning op was almost certainly among them. Island Showdown: 132/917 chunks foreign
bytes, healed live. → Delta 1.46c (engine `f88144fe` + `578076fb`, game `5172bbd9`): LOSSLESS
log — mutex + persistent handle (_fsopen _SH_DENYNO = readable while game runs) + per-line
`#seq`; whole session logs to ONE file in the EXE DIR now (no Files/ split; collection
scripts must read exe-dir). Deployed EXE 16:28+, clean 841/841 baseline. NEXT CAPTURE gives a
complete strictly-ordered op history → replay offline (replay_alloc.py, or drive the REAL
vendored offsetAllocator.cpp with it — planned standalone harness, not yet built) → the first
divergent grant + the ops before it = the mechanism, deterministically. The user's
triple-stack recipe reproduces ~reliably — one more run on 1.46c should end this.**

**Earlier same-day reproduction attempts (all CLEAN — ~60K ops replay-verified)**: 6 cold
loads, 3 churn+reload cycles (50× MOVE_CAMERA far-out), 4 back-to-back `VERIFY_MESH burst`
storms (9360 one-frame rebuilds, game `e8f00533`), and 1 forced 385-shader-recompile load
(recreating the storm run's environment). The storm is a genuine timing lottery — ONE
observation. **The trap is armed: every load appends ~6K ops to `Files/alloc_tripwire.txt`
(delete occasionally, it never truncates); any organic recurrence carries full op history for
`replay_alloc.py`. If the user reports corruption: collect that file BEFORE restarting.**
VERIFY verdict rules (also in WETEST): chunk_ = byte-stable, any hit real; ~204 skinned-mesh
`vb_bon badwords=1` one-nibble class = benign quantization wobble; 100%-diff-all-regions =
real stomp (only ever seen alongside storms).

**New harness oracles (game `b886bdcb`, doc'd in WETEST.md)**: `FIND_OBJECT x y z [r]`
(position→entity map; GG frame names are generic `$dummy_node`/`StaticMesh-N` — the canyon
rocks are `$dummy_node`, learned by string-dumping `Rock brown.dbo`); `VERIFY_MESH [substr]`
(snapshot→re-upload→snapshot byte-compare; chunk_ meshes byte-stable + trustworthy — 841/841
clean baseline; **CAVEAT: ~420 island meshes (skinned + statics incl. tree pool) report
100%-diff every run — unresolved: legitimately non-deterministic rebuild vs verify-wave
self-stomping — cross-check alloc_tripwire.txt before trusting those**); `VERIFY_WATCH`
(no-re-upload double snapshot = active-writer detector, no false positives).

**Canyon session forensics (user's failed 1.45 soak, same day)**: travel-churn corruption
reproduced on "a grand canyon adventure"; `REUPLOAD_ENTITY chunk_` healed the terrain wedges
in place (GPU content vs correct CPU data — same as the suballocator-stomp signature); rock
entities stayed corrupt through StaticMesh re-upload (they're `$dummy_node`); mass re-uploads
can RE-ROLL corruption onto other meshes (each is an allocator churn storm — consistent with
allocator-race root cause). ALSO: `applytransform_garbage.txt` grew 4MB during a plain island
load — CHARACTERIZED same day: 4482 entries all from `WickedCall_LoadNode` entity-bank
loading; the DBO node hierarchies carry SHEARED baked matrices (row dot-products ≠ 0) which
`XMMatrixDecompose` legitimately rejects → the delta-1.9 guard spams. Benign (models render
fine), but 4MB of stack traces per load = load-time I/O waste — cap/gate the guard someday.
NOT a corruption writer.

## What is STILL OPEN

1. **Travel-churn corruption** (the user's crisp repro): fly the editor camera at full speed
   far outside the playable area for ~1 minute (hundreds of chunk generate/destroy cycles),
   leave the level WITHOUT saving, load a DIFFERENT level → missing chunk underfoot +
   far-field geometry corruption. A clean relaunch of the same level always renders fine.
   **Delta 1.45 targets exactly this and is UNVERIFIED — needs a user soak** (the automated
   re-run was interrupted by the machine reset).
2. **Occasional texture swapping within a level** (wrong texture on the wrong object). Same
   family as the healed blue palms; 1.44 covers the reload path, but a within-session path
   may remain.
3. **Churn-class slabs under synthetic abuse.** With sculpt storms + camera teleports +
   reloads back-to-back, slab/vertex-soup corruption still appeared ~3/4 runs, and ALSO with
   streaming hard-off → **a second writer exists beyond streaming** (suspect the mesh/buffer
   upload or copy-queue path). Note this abuse level is far beyond normal use.
4. Minor: the **"start with ghost human" player-start marker leaks 3 materials per reload**
   (deterministic, harmless-but-real).
5. `wi::renderer::ClearWorld`-style full reset on leave-level was never tried as a
   belt-and-braces alternative.

## Repro rigs (in the session scratchpad, copy them somewhere durable if wanted)

- `corruption_hunt3.sh` / `corruption_hunt19.sh` — heavy gauntlet: cold launch → load →
  aerial teleport storm → SCULPT_TEST+UNDO → 3× leave-and-reload (`NAVIGATE storyboard` +
  `CLICK_ONLY_LEVEL`) → screenshots at each phase. ~6 min/run. **hunt14 baseline = 4/4
  corrupt** — any fix must beat that.
- `corruption_hunt21.sh` — the USER'S recipe automated: load → `SET_CAMERA` high →
  50× `MOVE_CAMERA 3000 0 1500` (sustained far-field generation + removal churn) → leave →
  reload → inspect; detects mid-run death (device-lost/crash).
- Forensic commands (all in WETEST.md): `DUMP_MATERIALS`, `DUMP_BROKEN`, `DUMP_TREEPOOL`,
  `DUMP_ENTITY`, `DUMP_INSTANCE`, `DUMP_GEOMETRY`, `SET_ENTITY_VIS`, `REUPLOAD_TEXTURE`,
  `REUPLOAD_ENTITY`, `SET_STREAMING`, `SET_TREES draw`.
- Engine `corrupt_geometry.txt` tripwire (v2, `ba403530`) is still armed in
  `RunObjectUpdateSystem` — logs NaN/absurd world AABBs with a sane mesh AABB (matrix
  garbage), one entry per entity. **It never fired during any corruption** — consistent with
  the GPU-content conclusion. Note: files land in `Files/`, not the exe dir.

## Method lessons (expensive to relearn)

- **Screenshot-interval metrics are blind to per-frame artifacts**; and for CUMULATIVE-trigger
  intermittents (needs 2-3 stacked reloads), single-observation bisect rounds produce false
  negatives. Build the automated multi-trigger oracle FIRST, then bisect.
- **Knob-off ≠ commit-absent**: a runtime flag only exonerates the gated behavior; the
  commit's unconditional bookkeeping still runs.
- Instrument output landed in `Files/` (the game changes cwd after load) — an early "tripwire
  silent" conclusion was wrong because the checks looked in the wrong directory, and the v1
  tripwire had burned its cap on one by-design giant helper mesh (`box`/`node_mesh`, radius
  3.5M). Always validate that an instrument CAN fire before trusting its silence.
