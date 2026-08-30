---
name: project-vram-floor
description: "Why every GameGuruMAX DX12 level costs 4+ GB before it contains anything, and what can be traded away to keep the DX11 4 GB minimum spec. Investigated 2026-08-02. Root cause of the long-unexplained census-vs-driver gap: ~14,000 object PIPELINE STATE OBJECTS at ~96 KB of driver VRAM each, invisible to the resource census. Two permutation axes were unreachable by GetObjectPSO (mesh_shader gated on GPU capability not MESH_SHADER_ALLOWED; cullMode 3 unreachable) -> engine 1.77 trim, 13976->7496 PSOs, -450 to -624 MB on EVERY level, POLYS bit-identical. Repo GameGuru Core/VRAM_FLOOR.md is the authority: floor table, costed follow-ups, and the shape of a Low VRAM (4 GB) preset."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-02T18:40:56.861Z
---

**Repo `GameGuru Core/VRAM_FLOOR.md` is the authority.** This is the summary.
Companion to [[project-vram-census]] (where BIG levels spend) — this file is about what
EVERY level spends before it contains anything.

## The product goal (user, 2026-08-02)

Keep DX11's **4 GB minimum video card** for DX12. Wanted budget shape: **1.5–2.0 GB floor for
an empty level + ~2 GB for content**. Preference is strongly for **knobs and dials that let
users trim visual complexity**, not for raising min spec to 8 GB. 8 GB users keep the full
DX12 visuals; 4 GB users get a good experience at a smooth 60 FPS with some visual treats
sacrificed. Raising min spec to 8 GB is a LAST RESORT.

## The floor is real and it is ~4.45 GB

Switch Escape — 109 K polygons, the smallest demo — cost **4452 MB driver VRAM**. Zombie
Cellar (indoor, 28 K polys) cost 4479 MB. A 4 GB card cannot open either.

Censusing three structurally unrelated levels showed **eight categories that are
byte-identical across all three** (indoor cellar / tiny level / 2.1M-poly outdoor): SVT tile
pool 576, mesh suballocator 512, terrain source arrays 220, SVT bookkeeping 134, terrain chunk
maps 115, terrain global maps 66, transparent shadow atlas 160, shadow depth atlas 81, plus
~222 MB of small mesh buffers. **~2.09 GB that content cannot influence.** The cellar pays
1111 MB of terrain machinery for terrain it does not have.

**The hub alone (no level chosen, fresh launch) is already 4477 MB** — including 816 MB of
grass and the full terrain machinery.

## ROOT CAUSE of the census-vs-driver gap: pipeline state objects

Driver usage ran **1.16–1.61 GB above the resource census on every demo ever measured**,
near-constant regardless of content. Two theories (descriptor heaps, command allocators) were
both WRONG. Four counters added to the census header settled it in one run:

```
pso_creates=13976  pso_compiles=19  descheap_bytes=32032768  cmdalloc=90
```

Descriptor heaps 30.5 MB, command allocators 90 — both ruled out. **13,976 object PSOs**, each
created with a `renderpass_info` so the DX12 backend builds a real `ID3D12PipelineState`
immediately = **≈96 KB of driver video memory each**, structurally invisible to the census.

Two permutation axes produced pipelines `GetObjectPSO` can NEVER select:
- **mesh_shader** gated on the GPU *capability*, but selection uses `IsMeshShaderAllowed()`
  (`MESH_SHADER_ALLOWED`, which MAX never sets) — a whole dead second set on capable cards.
- **cullMode ran 0..3** but the variant field is only ever filled from CullMode NONE/FRONT/BACK;
  value 3 is unreachable and duplicates BACK. A flat 25 %.

Engine 1.77 (`gg_pso_trim`, revert switch only): **13,976 → 7,496 PSOs, −450 to −624 MB of
driver VRAM on EVERY level**, resource census unmoved (which is the proof), **POLYS
bit-identical** on cellar/switch/horseshoe = no dropped draws.

## Costed follow-ups (full table in the repo doc)

Biggest first: terrain machinery allocated on demand (**1111 MB** on terrain-free levels) ·
lazy object PSOs behind the low-VRAM preset (~700 MB, costs first-draw hitches; the deferred
`renderpass_info == nullptr` path already exists) · SVT atlas 6144 in low mode (288) ·
transparent shadow atlas (160 floor / up to 5.3 GB hub-wide — **product decision, appears
visually inert**) · terrain source arrays 512² (165) · suballocate terrain chunk mesh buffers
(125 — 64 KB minimum-allocation waste, incl. **72 MB of 4-byte empty vertex streams**) ·
mesh suballocator 128 MB granularity (128) · tessellation + voxelize PSO axes (~100 each, but
`SetTessellationEnabled(false)` runs AFTER `LoadShaders` so the flag must move earlier first).

Identified reductions total ~1287 MB → floor ≈ 2.54 GB. **The two structural items above are
what actually reach the 1.5–2.0 GB target.**

**Grass is the whole content spread** (17.3 GB hub-wide, 4297 MB on Z Island alone) — no 4 GB
preset works without a grass cap, and it is the lever users will see most.

## The 4 GB preset work — user-directed 2026-08-02, done in revertable batches

User instruction: *"Drop the transparent shadow atlas and remove any UI that enables it, and cap
grass in the preset, and also the lazy PSO, do the work in batches so we can revert any that
corrupt the level."* One commit per batch, each independently revertable.

**Batch 1 — transparent shadow atlas DROPPED. Engine `8acce73f` + game `81224917`. VERIFIED.**
`gg_transparent_shadows` defaults false, atlas never allocated, shadow pass depth-only, object
shadow PSOs built `rt_count = 0`. Safe by construction: with the atlas invalid the engine binds
`getWhite()` and every shader site multiplies by white = no-op. Only the OBJECT shadow PSO bakes
a renderpass_info — hair/emitters/impostors/shadowClear use the deferred path and follow
automatically. 4 demos: POLYS bit-identical, batches 0, no census record, screenshots unchanged.
Island isolated −160 MB; the atlas was **520 MB** on Bounty / Foggy Forest (16384×4096 packers).
The UI checkbox was ALREADY hidden by the July audit — do not un-hide it; a real re-enable needs
the engine default plus a restart.

**Batch 2 — preset scaffold + grass cap. Game `0e1d1c97`. VERIFIED but MODEST.** setup.ini
`lowvram=1` / `lowvramgrassdist=750`, harness `SET_LOWVRAM`, bridge `GGSetLowVRAM()`. Caps the
grass DRAW DISTANCE via `GGGrass_LodDistEffective()` — every lod_dist consumer must go through it
or the per-strand cull and the chunk-creation ring disagree and grass pops in whole chunks.
Z Island: grass 4296.9→3804.1 MB (−493), POLYS identical, screenshots unchanged. **Inert on
Island Showdown** (already under the cap). **Only −11 % and here is why: the ring is
`viewDist/chunkStride + 1.0` with chunkStride ≈ 5040 and viewDist = lod_dist + 2500, so the
+1 chunk and the +2500 dominate** — lod_dist 2000→750 moves the ring 1.89→1.64 chunks.
**So the draw-distance cap alone cannot deliver the content half of a 4 GB budget.** More grass
savings means density/tier levers = the territory reverted on 08-01; gate on
`tools/grassdensity.ps1` clumpCV, never screenshots.

**Batch 3 — lazy object PSOs. Engine `3666bbf7` + game `a66d889e`. THE BIG WIN, zero visual
risk.** `gg_pso_lazy_object` passes nullptr for renderpass_info so the backend's deferred path
builds each pipeline at first BIND and caches it by {pso, renderpass_hash}. Island Showdown:
**driver 4312.7→3679.8 MB (−632.9), eager driver pipelines 6337→1, lazy compiles 22→57** (≈1 % of
what was built eagerly), POLYS 4115636 bit-identical, FPS flat. Z Island full preset
8998.8→8302.1, FPS 70.4→80.4. Default OFF — costs first-use compile hitches.

**TWO TRAPS in batch 3, both caught only by measuring:**
1. **`pso_creates` counts CreatePipelineState CALLS, not driver objects** — it stays 7496 in lazy
   mode by design. The first A/B read it, saw no change, and would have concluded the feature was
   dead. Added **`pso_driver_eager`** to the census header; eager + `pso_compiles` = what the
   driver holds.
2. **The flag must be set before `LoadShaders`, and `FPSC_LoadSETUPINI` runs LATER.** Wiring the
   key only there was measurably inert. `GetSetupIniEarly()` (called from `main()`) reads it now.
   Consequence: **`SET_LOWVRAM` cannot enable lazy PSOs** — setup.ini only.

**Batch 4 — grass DENSITY lever. Game `a46f55c9`. GATED ON clumpCV, default 75 %.**
`lowvramgrassdensity` (percent) scales `strandCount` uniformly in `GrassTierDensityScale()`.
Grass memory is linear in strand count, so this is the content lever with real leverage.
**Touches strandCount ONLY** — emitter mesh, `vertex_lengths` mask and `randomSeed` untouched, so
the same painted region is sampled uniformly with fewer strands. That is the whole difference
from the reverted 08-01 attempt, which narrowed the MASK and made strands bunch.

**The gate is read differently on purpose here: coverage is MEANT to fall for a 4 GB knob, so its
±0.15 pp threshold does not apply — clumpCV and the band profile decide.** TESTPRO1 curve:

| density | grass MB | driver MB | coverage | clumpCV | verdict |
|---|---|---|---|---|---|
| 100 | 2187.0 | 5990.3 | 9.450% | 1.104 | reference |
| **75** | 1645.6 | 4765.1 | 9.355% | 1.116 | **near free** |
| 50 | 1104.5 | 4499.4 | 7.979% | 1.178 | honest trade |
| 30 | 665.7 | 3746.2 | 5.403% | **1.354** | **FAIL, clumps** |

75 % is near-free because the meadow is **over-saturated** — strands overlap so heavily that
removing a quarter barely changes covered pixels. 30 % fails the same way the reverted build did
(+0.25 vs its +0.274). **Do not ship below 50 without re-running the table.**
Full preset on that scene: driver 5989.9→4765.1 MB, FPS 56.2→69.4, **clumpCV flat (−0.002)**.

**RESOLVED 2026-08-02 (game `c784d891`) — the reference was RETAKEN on this build.** New GOOD:
driver 5926-5990 MB, census 4870-4886, grass 2187 MB, 58 systems, 4,816,000 strands, FPS 56.1,
POLYS 3,149,243. **New gate values: coverage ~9.45%, clumpCV 1.104, bands 13.6 13.1 18.7 9.9 1.3.**
Thresholds UNCHANGED (3-shot noise: coverage spread 0.033 pp, clumpCV 0.006 — still under +-0.01).
Old 08-01 values kept in a collapsed block. **Standing rule that came out of it: if HAIR_SYSTEMS /
strands / POLYS jump TOGETHER, suspect the SCENE (different chunk set in range), not the renderer.**

What drove it (historic): 58 systems vs
52, 4.816M strands vs 4.216M, POLYS 3.15M vs 2.71M, clumpCV **1.104 vs 0.871**. They moved
together = a different chunk set in range = the scene's saved camera or content drifted. The
gate's THRESHOLDS are still right; its REFERENCE VALUES are stale. **Use same-session baselines
until it is refreshed**, and work out why it drifted before blessing a new camera.

**A/B isolation trick:** `lowvram=1` with `lowvramgrassdist=999999` makes the grass cap inert and
exercises the lazy-PSO half alone, where **POLYS must stay bit-identical**. Same trick pins the
cap while sweeping density.

**TRAP (cost me a control reading):** never send a harness command while an automated run is in
flight — `auto_command.txt`/`auto_result.txt` is a single slot and the probe steals the reply.

## CORRECTION to an earlier claim in this file's first draft

`svtatlasheight=6144` does NOT save 288 MB safely — it starves. Tiles = 62 × floor(height/264):
16384→3844, 12288→2852, 8192→1922, 6144→1426, against a measured peak demand of **1864–2001**.
There is no free atlas reduction left below 12288; shrinking it further requires reducing tile
DEMAND first (raise `SVT_MIP_BIAS`), and that pairing needs its own fast-travel soak.

## Method

Same lesson as the streaming crash ([[feedback-instrument-before-theory]]): two confident
theories about the missing 1.4 GB were both wrong; four counters in the census header named it
in one run. Instruments: `DUMP_VRAM` header fields + harness `VRAM_STAGE <label>` driver-usage
marks. Judge PSO cost from `pso_creates` × ~96 KB, and confirm any trim with **POLYS
bit-identical** to the pre-trim sweep — a missing PSO makes RenderMeshes skip the draw.
