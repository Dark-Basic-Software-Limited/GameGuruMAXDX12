# The VRAM floor — why an empty level costs 4 GB, and what can be traded away

Investigation 2026-08-02. Companion to `VRAM_CENSUS.md` (the per-resource instrument) and
`VRAM_HUB_SWEEP.md` (the 19-demo totals). This file answers a different question: **not what
the big levels spend, but what EVERY level spends before it contains anything.**

Target set by the product: keep the DX11 minimum spec of a **4 GB card** for DX12. Budget
shape wanted — **1.5–2.0 GB floor for an empty level, ~2 GB left for content.**

## The measurement

Three levels with nothing in common were censused on the same build (12288 SVT, streaming ON),
editor, 30 s settle:

| Level | POLYS | census | driver usage |
|---|---|---|---|
| Escape from the Zombie Cellar (indoor) | 28,048 | 2752.6 MB | 4479.0 MB |
| Switch Escape (smallest demo) | 109,358 | 2592.1 MB | 4452.0 MB |
| Horseshoe Bend (outdoor, no grass) | 2,133,269 | 3187.1 MB | 5156.7 MB |

**Switch Escape is the floor probe: 109 K polygons and it still costs 4.45 GB.** A 4 GB card
cannot open it.

Categorising the censuses shows why. These eight categories are **byte-identical across all
three levels** — indoor cellar, tiny level, and a 2.1-million-polygon outdoor scene pay exactly
the same:

| Category | Switch | Cellar | Horseshoe |
|---|---|---|---|
| SVT tile pool | 576.0 | 576.0 | 576.0 |
| Mesh suballocator blocks | 512.0 | 512.0 | 768.0 |
| Terrain source texture arrays | 220.0 | 220.0 | 220.0 |
| SVT bookkeeping buffers | 134.4 | 134.4 | 134.4 |
| Terrain chunk maps (blend/wet/height) | 115.0 | 115.0 | 115.0 |
| Terrain global maps | 66.0 | 66.0 | 66.0 |
| Shadow atlas TRANSPARENT | 160.0 | 160.0 | 160.0 |
| Shadow atlas depth | 81.2 | 81.2 | 81.2 |
| Unnamed small mesh buffers | 221.6 | 222.2 | 225.8 |

That is **~2.09 GB of allocation that content cannot influence.** Note the cellar: an indoor
level pays 1111 MB of terrain machinery for terrain it does not have.

### The hub already costs 4.3 GB

Fresh launch, sitting on the demo browser with no level chosen: census 2899 MB, **driver
4477 MB** — including 816 MB of grass strand buffers and the full 1.6 GB of terrain machinery.
The floor is paid before the user picks anything.

## The invisible 1.4 GB: it was pipeline state objects

Driver-reported usage ran **1.16–1.61 GB above** the resource census on every demo ever
measured, near-constant regardless of content. Nothing in the census explained it, so
engine 1.77 added the instrument rather than another theory: `DUMP_VRAM` now reports
`pso_creates`, `pso_compiles`, `descheap_bytes`, `cmdalloc`, plus `VRAM_STAGE <label>` marks
of driver usage at named milestones.

First reading settled it:

```
pso_creates=13976  pso_compiles=19  descheap_bytes=32032768  cmdalloc=90
```

- descriptor heaps **30.5 MB** — the 1,000,000-descriptor tier-1 heap is not the problem
- command allocators **90** — not the problem
- **13,976 pipeline state objects** — object PSOs are created with a `renderpass_info`, which
  makes the DX12 backend build a real `ID3D12PipelineState` immediately. That is driver-side
  video memory the resource census structurally cannot see.

### Two permutation axes were unreachable

`LoadShaders` builds object PSOs over renderpass × shadertype × mesh_shader × blendmode ×
cullmode × tessellation × alphatest. Two axes produced pipelines `GetObjectPSO` can never
select:

1. **mesh_shader** was gated on the GPU *capability*, but selection uses `IsMeshShaderAllowed()`
   — backed by `MESH_SHADER_ALLOWED`, which MAX never turns on. On any mesh-shader-capable card
   the engine built a complete second set of object pipelines that nothing could ever bind.
2. **cullMode ran 0..3**, but `ObjectRenderingVariant::cullmode` is only ever filled from
   `CullMode` NONE / FRONT / BACK (0..2) — the fourth value is unreachable by lookup, and its
   rasterizer state is a duplicate of BACK anyway. A flat 25 % of the object set.

Gating both on what can actually be selected (engine 1.77, `gg_pso_trim`, revert switch only):

PSOs created: **13,976 → 7,496 (−6,480, −46 %)**. Driver VRAM, editor, 30 s settle:

| Level | before | after | saved | POLYS before → after |
|---|---|---|---|---|
| Escape from the Zombie Cellar | 4479.0 MB | 4031.1 MB | **−447.9** | 28,048 → 28,048 |
| Switch Escape | 4452.0 MB | 3939.8 MB | **−512.2** | 109,358 → 109,358 |
| Horseshoe Bend | 5156.7 MB | 4644.1 MB | **−512.6** | 2,133,269 → 2,133,269 |
| Hub (no level) | 4270.0 MB | 3774.2 MB | −495.8 | — |

The resource census does not move (Switch Escape 2592.1 → 2587.9 MB), so the whole saving is
non-resource memory — which is the proof that the pipelines were the gap. **≈77–96 KB of
driver video memory per object PSO.** This lands on every level, editor and game.

**POLYS is bit-identical on all three levels**, which is the acceptance test that matters: a
missing PSO makes `RenderMeshes` skip the draw, so any pipeline that was actually needed would
show up immediately as fewer polygons. Nothing was needed — the removed pipelines were
unreachable by construction.

## Floor composition after the PSO trim (Switch Escape, 3940 MB)

| Item | MB | Knob today | Reducible by | How |
|---|---|---|---|---|
| PSO driver memory (7,496 left) | ~720 | none | ~200 | trim the tessellation + voxelize axes (below) |
| SVT tile pool | 576 | **`svtatlasheight`** | 0 today | see the correction below |
| Mesh suballocator blocks | 512 | none | 128 | 128 MB block granularity (only ~311 used) |
| D3D12MA block padding | 349 | none | — | allocator behaviour |
| Driver / descriptor heaps / cmd allocators | ~243 | none | — | not ours |
| Unnamed small mesh buffers | 222 | none | 125 | 64 KB minimum allocation waste, see below |
| Terrain source texture arrays | 220 | none | 165 | 512² instead of 1024² in low mode |
| Shadow atlas TRANSPARENT | 160 | none | 160 | appears visually inert — product decision |
| SVT bookkeeping | 134 | scales with atlas | 40 | follows the atlas |
| Terrain chunk maps | 115 | none | 52 | wetmap is half of it |
| Shadow atlas depth | 81 | shadow resolution | 60 | 1024 cascades in low mode |
| Terrain global maps | 66 | none | 49 | 2048² instead of 4096² |
| Render targets / postFX | 61 | resolution | 20 | follows internal resolution |
| Content (even the emptiest demo) | ~310 | streaming | — | already adaptive |

Identified reductions total **~999 MB → floor ≈ 2.94 GB** (the SVT 288 is struck, see below). That is a 43 % cut from the
original 4452 MB but still above the 1.5–2.0 GB target, so two structural items matter:

### CORRECTION — the SVT atlas cannot simply be halved again

An earlier draft of this table claimed 288 MB from `svtatlasheight=6144`. That is **wrong** and
would starve the atlas exactly as 8192 does. Tiles = 62 x floor(height/264):

| height | tiles | vs measured peak demand 1864-2001 |
|---|---|---|
| 16384 | 3844 | huge headroom (the old default) |
| 12288 | 2852 | 30-35 % headroom — the current default |
| 8192 | 1922 | BELOW Aztec's 2001 peak — starves |
| 6144 | 1426 | far below demand — starves badly |

So there is no free atlas reduction left. The only way to shrink it further is to reduce the
tile DEMAND first — raising `SVT_MIP_BIAS` lowers terrain sampling resolution and therefore how
many tiles a view needs, which is precisely the kind of visual trade a 4 GB preset is allowed to
make. That pairing (higher mip bias + smaller atlas) needs its own fast-travel soak before it can
be trusted; do not ship one without the other.

### The two structural wins that close the gap

1. **Do not allocate terrain machinery for levels without terrain.** SVT pool + source arrays +
   chunk maps + bookkeeping + global maps = **1111 MB**, allocated unconditionally. Zombie
   Cellar pays all of it for an indoor level. Allocating on first terrain use would take the
   indoor floor under 2 GB on its own.
2. **Lazy object PSOs.** `CreatePipelineState` already has a deferred path (`renderpass_info ==
   nullptr`) that builds the pipeline at bind time and caches it by hash in `pipelines_global`.
   Only the pipelines actually used would exist — the remaining ~720 MB largely disappears. The
   cost is first-draw hitching, which is why it belongs behind the low-VRAM preset rather than
   in the default path.

### Small-buffer allocation waste (~125 MB)

576 terrain chunk meshes allocate four buffers each. D3D12's minimum allocation is 64 KB:

| payload | count | needed | allocated | waste |
|---|---|---|---|---|
| 84,500 B vertex buffer | 576 | 46.4 MB | 72.0 MB | 25.6 MB |
| 16,766 B index buffer | 576 | 9.2 MB | 36.0 MB | 26.8 MB |
| 4 B vertex buffer (empty stream) | 576 | ~0 | 36.0 MB | **36.0 MB** |
| 4 B index buffer (empty stream) | 576 | ~0 | 36.0 MB | **36.0 MB** |

The two 4-byte streams are empty optional vertex streams paying 64 KB each — 72 MB of nothing.
Routing these through the existing `GPUSubAllocator` (which already serves mesh data) removes
almost all of it.

## Follow-ups, measured and costed

| # | Item | Saving | Risk | Notes |
|---|---|---|---|---|
| 1 | Terrain machinery allocated on demand | 1111 MB on terrain-free levels | medium | biggest single structural win |
| 2 | Lazy object PSOs behind the low-VRAM preset | ~700 MB | medium | first-draw hitches; mechanism already exists |
| 3 | Trim tessellation PSO axis | ~100 MB | low | MAX calls `SetTessellationEnabled(false)` (`master_part1.cpp:118`) — but AFTER `LoadShaders`, so the flag must move earlier before this can be gated on it |
| 4 | Trim RENDERPASS_VOXELIZE PSO axis | ~100 MB | low | `VXGI_ENABLED` is false and MAX never sets it; same ordering caveat |
| 5 | Suballocate terrain chunk mesh buffers | ~125 MB | low | 64 KB granularity waste, table above |
| 6 | Mesh suballocator block granularity 256→128 MB | 128 MB | low | only ~311 MB of 512 used on small levels |
| 7 | Transparent shadow atlas | 160 MB floor, up to 5.3 GB hub-wide | — | **product decision**: the feature appears visually inert |
| 8 | Terrain source arrays 1024²→512² in low mode | 165 MB | low | visible terrain detail trade |
| 9 | Terrain global maps 4096²→2048² in low mode | 49 MB | low | material/grass/tree paint resolution |
| 10 | Wetmap opt-out when no rain | 52 MB | low | 841 chunks × wetmap |

## What shipped — the 4 GB preset, built in revertable batches

User direction 2026-08-02: *drop the transparent shadow atlas and remove any UI that enables it,
cap grass in the preset, and also the lazy PSO — in batches so any one can be reverted.*

### Batch 1 — transparent shadow atlas dropped (engine `8acce73f`, game `81224917`)

Detailed in the WICKED_ENGINE_CHANGES 1.78 row. `gg_transparent_shadows` defaults false, the
atlas is never allocated, the shadow pass is depth-only, and the object shadow PSOs are built
with `rt_count = 0` to match. Verified on four demos: POLYS bit-identical, no census record,
screenshots unchanged. **Island isolated −160 MB; the atlas was 520 MB on Bounty and Foggy
Forest** (16384x4096 shadow packers). The UI checkbox was already hidden by the July audit.

### Batch 2 — preset scaffold + grass cap

`setup.ini lowvram=1` / `lowvramgrassdist=750`, harness `SET_LOWVRAM`, bridged through
`GGSetLowVRAM()` in master_part1.cpp so the preset's members stay in one place.

The grass lever is the **draw distance**, applied through a single accessor
`GGGrass_LodDistEffective()`. Every consumer of `lod_dist` must go through it: the per-strand
visibility cull and the chunk-creation ring are deliberately decoupled (the ring sits one chunk
further out so strands fade in individually), and if only one of them saw the cap, grass would
pop in whole chunks again. 750 is the editor slider's own minimum for that same reason. It is a
CAP — a level already asking for less keeps its value.

This deliberately does **not** touch placement or density. The 2026-08-01 attempt that did had to
be reverted for clumping; the post-mortem and its "what not to do next time" list are in
`VRAM_CENSUS.md` and still apply.

### Batch 3 — lazy object PSOs

`wi::renderer::gg_pso_lazy_object` passes nullptr for `renderpass_info`, putting object pipelines
on the backend's deferred path: `pso_validate` builds the real `ID3D12PipelineState` at first
BIND, filling in the live pass's formats and sample count and caching it in `pipelines_global`
keyed by `{pso, renderpass_hash}`. Only pipelines actually used ever reach the driver. Primitive
topology is set on that path too, so nothing is lost. The cost is a first-use compile hitch,
measurable in `gg_dbg_pso_compiles` / `gg_dbg_pso_compile_us`.

**Ordering trap, found by measurement not by reading.** The first attempt wired the flag only
into `FPSC_LoadSETUPINI`, and it did nothing at all: `pso_creates` stayed at 7496, identical to
the control, because setup.ini is parsed **after** the engine has already built its pipelines in
`LoadShaders`. The flag is now also read in `GetSetupIniEarly()`, which `main()` calls before the
engine starts. This is exactly why the A/B was designed to isolate the two halves — a combined
test would have shown the grass saving and hidden the fact that batch 3 was inert.

Because of that ordering, **`SET_LOWVRAM` cannot enable the lazy-PSO half** — object pipelines
are long since built by the time any harness command lands. Use `setup.ini lowvram=1`.

### Measured result of batches 2 + 3

Island Showdown, one binary, three configs, editor, 30 s settle:

| | control | lazy PSOs only | full preset |
|---|---|---|---|
| driver VRAM | 4312.7 MB | **3679.8 MB** | 3680.1 MB |
| eager driver pipelines | 6337 | **1** | 1 |
| lazy compiles | 22 | 57 | 57 |
| grass strand buffers | 602.6 MB | 602.6 MB | 602.6 MB |
| POLYS | 4115636 | 4115636 | 4115636 |
| FPS | 63.6 | 63.2 | 70.2 |

The Mystery of Z Island (the grassiest demo, where the cap actually bites):

| | control | full preset | delta |
|---|---|---|---|
| driver VRAM | 8998.8 MB | 8302.1 MB | **−696.7** |
| grass strand buffers | 4296.9 MB | 3804.1 MB | **−492.8** |
| eager driver pipelines | 6337 | 1 | |
| POLYS | 704717 | 704717 | identical |
| FPS | 70.4 | 80.4 | **+10.0** |

**Lazy PSOs are the big win and they are free of visual risk**: 57 pipelines is all Island
Showdown actually binds, against 6337 built eagerly — about 1 %. POLYS bit-identical on both
levels, FPS flat or better.

**The grass cap is real but modest at 750, and it is worth understanding why.** On Island
Showdown it does nothing at all (that level already asks for less than the cap). On Z Island it
takes 493 MB off the strand buffers — 11 %, not the ~1.14 GB the older Grass Draw Distance
measurement might suggest. The reason is arithmetic: the chunk-creation ring is
`viewDistInches / chunkStride + 1.0` with `chunkStride ≈ 5040` and `viewDistInches = lod_dist +
2500`, so the `+1.0` chunk and the `+2500` offset dominate at these distances. Dropping
lod_dist from ~2000 to 750 moves the ring from ~1.89 chunks to ~1.64 — a small change in area.

So: **the draw-distance cap alone will not deliver the content half of a 4 GB budget.** Getting
substantially more out of grass means the density/tier levers, which is exactly the territory
that had to be reverted on 2026-08-01 for clumping — it must go through
`tools/grassdensity.ps1` and the clumpCV gate, not screenshots.

### Batch 3 became the DEFAULT on 2026-08-03 (engine 1.82) — the hitch was measured and isn't there

Lazy PSOs behind `lowvram=1` only reached users who hand-edit setup.ini, so everyone else paid
~390 MB for pipelines they never bind. The blocker was never the memory case, it was the unmeasured
first-use compile hitch, and nothing in the build could see it: the wall-gap tracer starts at
100 ms and an FPS mean smears 26 compiles into nothing. Engine 1.82 added the two things that can:
`pso_compile_max_ms` (the **worst single** compile — the sum is not what a user feels) and a
per-frame histogram on the always-on tracer path, read as the `HITCH:` line of `GET_PERF_DATA`
with `HITCH_RESET` to scope a window.

Five cold runs on TESTPRO1, three windows each:

| | eager (`lazypso=0`) | lazy (default) |
|---|---|---|
| driver VRAM, settled | 5947 / 6011 MB | **5556 / 5556 MB** |
| driver VRAM, travelled | 8535 / 8535 MB | **8143 / 8144 MB** |
| eager driver pipelines | 4033 | **1** |
| PSO compiles, whole session | 22 | 48 |
| PSO compile time, whole session | 5.6-6.3 ms | 14.0-17.0 ms |
| **worst single compile** | 0.6-0.8 ms | **0.9-1.2 ms** |
| compiles in the first 25 s of play | 0 | **0** |
| compiles during 8-waypoint travel | 0 | 5 (1.6 ms) |
| POLYS settled / travelled | 3165848 / 2669257 | **identical** |
| launch→hub, launch→editor | 11 s, 39-40 s | 11 s, 38-40 s |

**−392 MB, and the hitch does not exist.** 26 extra compiles totalling ~10 ms across an entire
session, worst single 1.1 ms — under a tenth of one 60 FPS frame — and all of them land inside a
level load already dominated by a 13-second stall. The shader bytecode is compiled at startup;
`pso_validate` only does driver-side assembly with the live pass formats, which is why it is cheap.
4032 pipelines × 96 KB ≈ 387 MB confirms the arithmetic.

Note the saving is −392 here, not the −633 measured in the 1.79 row: the 1.80 A5 trim had since
taken eager pipelines from 6337 to 4033, so there was less left to defer. The two do not stack.

**Measurement trap, caught only by repeating the run.** The first A/B showed lazy with 98 frames
over 25 ms against the control's **1** — which reads as a smoking gun. But the causal counter said
`psoC=0` for that window in *both* configs, so the compiles could not be the cause. On repeat the
**control** swung 1 → 87 while lazy went 98 → 101 → 2. Overlapping distributions, pure run variance
on a scene whose frame time sits near the 25 ms bucket edge — the same ±8 FPS launch-to-launch
swing already on record for the editor. **One counter reading zero outweighed a plausible-looking
98× difference.** Do not accept a single A/B pair on this scene without a repeat.

## The D3D12MA padding, measured at last (2026-08-03, engine 1.83)

The `pad` column of `VRAM_AUDIT.md` — 363 MB mean, 121-601 range — had never been broken down.
`DUMP_MAPAD` now dumps per-heap-type `CalculateStatistics` plus D3D12MA's own detailed JSON block
map. Two separate findings came out.

### 1. The column was measuring the wrong memory

`d3d12ma_blocks` and `census_bytes` sum **both memory segments** — L1 video memory and L0 system
memory, where UPLOAD/READBACK staging lives — while `driver_usage` is **video only**. So:

| | old arithmetic | what it actually was |
|---|---|---|
| `padding = blocks − census` | 257 MB | **152 MB video** + 105 MB system RAM |
| `nonres = driver − blocks` | 259 MB | **483 MB video** (it subtracted system-memory blocks) |

The two errors cancel in the SUM, because the `blocks` term drops out of `pad + nonres`, so the
audit's "~3.3 GB base cost" conclusion is unaffected — but **neither column on its own meant
anything**. The census header now carries `d3d12ma_allocated_video` / `d3d12ma_blocks_video`, and
`tools/vram_audit.sh` uses them (and labels pre-1.83 censuses as mixed).

### 2. Most of the real padding was avoidable — block size 64 → 16 MB

The JSON showed the DEFAULT heap holding **24 pooled 64 MB blocks and 61 committed allocations**.
Committed allocations pad **exactly zero** — 3688 MB of them including the 576 MB SVT tile pool and
the 128 MB mesh suballocator blocks. Essentially all the waste sat in **4 sparse blocks**, one of
them holding just *two* allocations (0.12 MB + 25.25 MB) inside a 64 MB heap.

`PreferredBlockSize` had been left commented out at `CreateAllocator`, so D3D12MA's own 64 MB
default applied. Measured on TESTPRO1, settled level, repeated:

| block size | video padding | driver total | non-resource | FPS | POLYS |
|---|---|---|---|---|---|
| 64 (stock) | 103.8 / 151.7 / 151.8 | 5619.6 | 483.4 | 65.8 | 3165848 |
| 32 | 129.8 | 5597.8 | 483.4 | 66.0 | 3165848 |
| **16 (new default)** | **11.5 / 11.6 / 11.6** | **5479.8** | 483.7 | 66.0 | 3165848 |
| 8 | 1.7 | 5475.3 | 489.1 | 65.6 | 3165848 |

**−92 to −140 MB on every level, and the 41 extra heaps cost nothing** — non-resource driver
memory moved 483.4 → 483.7. Across hub / level / travel: padding 33→4.9, 152→11.6, 187→81.3.
16 MB is also *more stable* than the default it replaces (11.5/11.6/11.6 against a 104-168 swing).

- **8 MB is not chosen**: 4.5 MB more total while non-resource starts climbing — the heap-count
  cost appearing, and the same trade that made 256 MB blocks wrong for the mesh suballocator.
- **32 being worse than 64 is not a typo.** D3D12MA sends any allocation over *half* a block to
  its own committed heap, so changing the size reshuffles which resources pool and which do not.
  The curve is non-monotonic; do not interpolate it, measure the size you want to ship.
- Revert with setup.ini **`mablockmb=64`**. `mablockmb=0` also means "library default".

### Ordering trap, fourth occurrence — and how it was caught

The knob was first wired through the game's `GetSetupIniEarly()` and was **completely inert**: the
allocator is built with the device, before `main()` ever reaches that parse. The engine now reads
the key itself inside `CreateAllocator`, so there is no ordering dependency left to get wrong.

This was caught only because the JSON echoes `PreferredBlockSize` back. The padding numbers had
*moved* across those runs (104 vs 152 MB) purely from run variance, and without the echo it would
have read as a modest win from a change that was doing nothing at all. **Have the instrument
report the setting back to you, not just the effect.**

### What is left, and whether to chase it

At 16 MB the remaining video padding is ~12 MB settled and ~81 MB after heavy travel, the latter
being genuine fragmentation from load and streaming churn. Recovering that needs
`Allocator::BeginDefragmentation` and real resource relocation — every bindless descriptor would
have to be re-pointed. **Not worth it**: it is now a travel-only 80 MB against much larger, safer
items still open.

### How to A/B the two halves apart

    lowvram=1
    lowvramgrassdist=999999

makes the grass cap inert (no level asks for a million inches) and exercises the lazy-PSO half
alone. **POLYS must stay bit-identical there** — a drop means a pipeline that was actually needed
never appeared and `RenderMeshes` skipped the draw. With the cap at 750, POLYS is *expected* to
fall; that is the grass cap working.

## Proposed "Low VRAM (4 GB)" preset

Not yet implemented — this is the shape the numbers support. Floor items first (they apply to
every level), then content items (they scale with what the user built).

**Floor:** `svtatlasheight=6144`, shadow cascades 1024, terrain source arrays 512²,
terrain global maps 2048², lazy object PSOs, transparent shadow atlas off, wetmap off.
→ floor ≈ **1.6–1.9 GB**.

**Content:** grass draw distance 750 (measured −1.14 GB on the heavy demos), grass density
capped, tree draw distance reduced, texture streaming budget capped by card size.
→ content ≈ **1.5–2.0 GB** on the current demos.

Grass is the whole spread: 17.3 GB hub-wide, 4297 MB on Z Island alone. No 4 GB preset works
without a grass cap, and it is the one lever users will see most.

## Method note

Two theories about the missing 1.4 GB (descriptor heaps; command allocators) were both wrong
and would have cost hours to chase. Four counters in the census header settled it in one run.
Same lesson as the streaming crash: **reach for the instrument before the theory.**
