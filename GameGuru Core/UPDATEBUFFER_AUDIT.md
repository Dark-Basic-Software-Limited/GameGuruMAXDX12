# UpdateBuffer race-class audit — 2026-08-08

THE AUTHORITY on which `GraphicsDevice::UpdateBuffer` call sites in the game layer carry the
shared-buffer race that produced the particle "ping" (game `82959a2b`, task #122).

## The defect class

`UpdateBuffer(buffer, data, cmd, size)` is, in DX12, `AllocateGPU` + `CopyBuffer`
([wiGraphicsDevice.h:361](../../WickedEngineDX12/WickedEngine/wiGraphicsDevice.h)), and its own
header says *"Since it uses a GPU Copy operation, appropriate synchronization is expected"* — the
barriers are the **caller's** job. `GraphicsDevice_DX12::CopyBuffer` is a bare `CopyBufferRegion`
with no `ResourceBarrier`.

Under D3D11 the identical code was safe: `UpdateSubresource` carries implicit hazard tracking, so
the driver guarantees write #2 cannot land before reader #1 has finished. **This codebase is a
DX11→DX12 port, so that assumption is baked in wherever the original DX11 code did it.**

**The bug shape:** ONE destination buffer, written 2+ times inside the SAME command list with
DIFFERENT content, with GPU work that READS it in between. The GPU may overlap write #2's copy with
the pass still reading write #1, so a pass silently consumes the wrong data.

**It is invisible.** D3D12 gives buffers implicit state promotion/decay, so there is no debug-layer
error and no validation warning. Only content instruments catch it — in the particle case, dumping
the whole 4096-slot age plane at 2 Hz and diffing consecutive snapshots.

## Findings — all 9 call sites, 7 distinct destination buffers

| Buffer | Site(s) | Writes/frame | Verdict |
|---|---|---|---|
| `pageGenVertexBuffer` | GGTerrain_part0.cpp:7698, 7854 | 2, differing | **BENIGN — dead path** (race shape confirmed) |
| `terrainConstantBuffer` | GGTerrain_part0.cpp:10270, 10455 | ≤1 (exclusive branches) | BENIGN |
| `instanceBuffer` | GGTerrain_part0.cpp:11138 | 1, constant content | BENIGN — dead path |
| `ggCustomFrameBuffer` | GGTerrain_part0.cpp:810 | 1 | BENIGN |
| `grassConstantBuffer` | GGGrass.cpp:2131 | 1 | BENIGN |
| `treeConstantBuffer` | GGTrees_part0.cpp:2512 | 1 | BENIGN |
| `constantBuffers[CBTYPE_FORWARDENTITYMASK]` | GGTrees_part0.cpp:3004 | 0 — inside `/* */` | BENIGN |

**No fixes were required.** Two premises in the original chip turned out to be wrong, and both
matter:

- **`terrainConstantBuffer` is NOT written twice per frame.** :10270 lives in `GGTerrain_Update`
  (:9718) and :10455 in `GGTerrain_Update_EmptyLevel` (:10437) — *different functions on mutually
  exclusive branches* (`master_part1.cpp:450` vs `:478`). At most one runs per frame.
- **`pageGenVertexBuffer` has the race shape but cannot execute.** See below.

## `pageGenVertexBuffer` — a real race, dormant behind a dead path

`GGTerrain_DrawPages` (:7535) writes the buffer for mip 0 (:7698), begins a render pass, binds it as
the vertex stream (:7715), draws (:7738), ends the pass, rewrites the same CPU array with **mip-1**
values (:7743-7852), writes the same buffer again (:7854) and draws again (:7864) — same command
list, zero barriers between :7698 and :7864. If it ran, the mip-1 copy could land while the mip-0
draw was still fetching vertices, baking mip-1 UVs into mip-0 pages.

**Why it cannot bite (double lock, both verified against source):**

1. `GGTerrain_Update` returns inside the `if (ggterrain_use_wicked_terrain)` block (:9846 → the
   `GGCustomFrame_Update(cmd); return;` at :10003-10005) **before** its sole call to
   `GGTerrain_DrawPages` at :10418. The flag defaults to `1` (:4208) and is only flipped by the
   Y-key debug toggle (:9782), which cannot fire in the editor because ImGui holds keyboard focus.
2. `GGTerrain_DrawPages` early-outs on `!texPagesColorAndMetal.IsValid()` (:7539), and that 883.6 MB
   page atlas is allocated by `GGTerrain_EnsurePageAtlas()` (:10011) which sits **below** the early
   return — so on the shipping path the destination textures do not exist at all.

**Empirically corroborated, not just read:** the VRAM campaign already measured this. `VRAM_CENSUS.md`
records the pair as *"883.6 MB of dead page cache"* and making it lazy saved that per level — which
is only true if `GGTerrain_DrawPages` never runs. See `VRAM_CENSUS.md:63-79`.

★ The two sites now carry a landmine comment (GGTerrain_part0.cpp:7698, :7854). **If the legacy VT
path is ever revived, fix this before shipping it.** The tidy fix is not a barrier: widen
`g_VerticesPageGen` to `[.][12]`, write mip 0 into slots 0-5 and mip 1 into 6-11, issue ONE
`UpdateBuffer` before both passes, and give the second draw a vertex offset of `6*numPages`.
`BindDynamicConstantBuffer` (the gpup fix) is unavailable here — this is a vertex buffer.

## Incidental, not bugs

- `GGTerrain_part0.cpp:7712` and `:7713` bind the same CB to the same slot twice (copy-paste, dead path).
- `GGTrees_part0.cpp:2975-3006` and `GGTerrain_part0.cpp:11158-11184` are dead `/* */` blocks
  referencing engine symbols that no longer exist. Hygiene deletes, not fixes.

## The standing rule

★ **Any shared buffer updated between consumers inside one command list is a silent DX12 race.**
Audit every new `UpdateBuffer` call site against the shape above. Fixed instances so far: the gpup
sim's three per-pass CBs and `TracerManager`'s per-tracer CB (both `82959a2b`), each replaced with
`BindDynamicConstantBuffer` — a per-call transient allocation with no shared destination.
