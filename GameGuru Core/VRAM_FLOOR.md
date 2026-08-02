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
| SVT tile pool | 576 | **`svtatlasheight`** | 288 | 6144 in low-VRAM mode |
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

Identified reductions total **~1287 MB → floor ≈ 2.65 GB.** That is a 43 % cut from the
original 4452 MB but still above the 1.5–2.0 GB target, so two structural items matter:

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
