---
name: project-customdraw-hook-inventory
description: "The DX12 port replaced DX11's ~20 extern \"C\" GGREDUCED engine hooks with customDraw_* function pointers, and the port's scope was defined by grepping three subsystem names — so every other hook fell out of scope silently and invisibly."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-19T11:54:46.168Z
---

**DX11 and DX12 wire GG custom rendering in completely different ways, and the port's scope
was set by a grep that could not see what it was missing.**

DX11 (`D:\max\WickedRepo`) compiles GG draw calls *into the engine* under `#ifdef GGREDUCED`:
~20 `extern "C"` functions declared in `RenderPath3D.cpp` / `wiRenderer.cpp` and called inline
(e.g. `Tracers::tracer_draw` at `RenderPath3D.cpp:2025`). Some have a
`#pragma comment(linker, "/alternatename:...")` weak-symbol fallback; **most do not**
(`tracer_draw`, `gpup_draw_init`, `gpup_draw_bydistance` are hard link deps — hence the no-op
stubs in `offlineshadercompiler.cpp`).

DX12 (`D:\max\WickedEngineDX12`) has **no GGREDUCED layer at all** — zero `extern "C"`, zero
`RenderPath3D.cpp` (the file is `wiRenderPath3D.cpp`; Wicked 0.71.858). Instead:
- 6 members on `RenderPath3D` — `wiRenderPath3D.h:366-372`: `customDraw_Prepass`,
  `_Prepass_Reflections`, `_Opaque`, `_Transparent`, `_Compose`, `_AfterPrepass`
- 2 namespace-scope — `wiRenderer.h:1503-1504`: `customDraw_ShadowMap`, `customDraw_EnvProbe`
- all `= nullptr`, **assigned by the GAME** in `master_part1.cpp:575-646`.

**So restoring a missing DX11 render feature usually needs NO engine edit** — it is a line
added to the right lambda in `master_part1.cpp`. Two rules for that file:
1. The call must go **above** `if (ggterrain_use_wicked_terrain) return;` — that flag defaults
   to 1 (`GGTerrain_part0.cpp:4243`), so anything below it is dead on the shipping path.
2. Anything reading the engine camera CB must go **above** `gpup_draw`, which clobbers b0/b1
   and never restores them (see the 3.25f comment block at `master_part1.cpp:614-632`).

★★★ **THE LESSON — why this stayed invisible for seven months.** The port's work inventory
(`DX11_to_DX12_Shader_Porting_Plan.md` §13.3, "GG Draw Functions Catalog") is a 16-row table
built by grepping for **GGTerrain / GGTrees / GGGrass**. Every hook in the same `#ifdef GGREDUCED`
blocks that did *not* carry one of those three names fell out of scope with no TODO, no delta row
and no backlog entry — `tracer_draw`, the gpup hooks, the terrain-occlusion accessors, the
`DrawSoftParticles` distance interleave. **An inventory built by grepping the things you know
about cannot list the things you don't.** The correct inventory is *every* `#ifdef GGREDUCED`
block in the DX11 engine, enumerated once.

The same defect class has now been hit **four** times — selection outline (08-05), gpup particles
(08-07), bullet tracers (09-19), and `GGTrees_Draw_ShadowMap`/`_EnvProbe` (still unwired: far-tree
billboards cast no shadow and miss env-probe captures). Each was fixed one at a time; the block was
never swept. The three `GGGrass_Draw*` declarations at `master_part1.cpp:431-433` are **not** in
this category — grass was re-implemented as Wicked scene geometry and the legacy pass is
deliberately dead (`GGGrass.h:80` "the dead legacy blade atlas ... if the legacy draw path wakes up").

⚠ **The word "tracer" is poisoned for grep in this project** — 18 of 19 markdown hits mean a
diagnostic instrument (gap tracer, draw tracer, caller tracer), not a bullet. A grep-based audit
drowns.

**The bullet-tracer case (3.55, fixed + Lee-confirmed 2026-09-19)** — the worked example.
Five defects stacked, the first sufficient alone: (1) no caller for `tracer_draw`; (2)
`Tracer_LoadTextureDDS` an empty stub since `d3ae5996` — ⚠ `Utility/dds.h` **cannot** be
included from `TracerManager.cpp` because `gameguru.h` pulls in ddraw.h whose `DDSD_*` /
`DDPF_*` / `DDSCAPS_*` **macros** expand inside dds.h's enumerator lists (46 errors); delegate
to `GGTerrain_LoadTextureDDS` instead, whose TU has no gameguru.h; (3) an unconditional
re-bind clobbering the white fallback — in DX12 an invalid resource binds a NULL descriptor and
an additive blend of zero writes **nothing**, so the failure is silent; (4) `desc.il` pointing
at a stack local, freed before DX12's **lazy** PSO compile at first bind; (5) `Update()` called
only from the draw hook, so with no hook the clock froze and `AddTracer` grew unbounded.
★ Diagnose with the `DUMP_TRACERS` harness verb — it names which link is broken.

⚠ **Adding a harness verb: the ladder is at MSVC's C1061 limit.** Hoist to a helper and **OR it
into an existing arm — never add an arm.** A new `else if` fails ~2000 lines away and reads as
an unrelated problem.

Related: [[project-wicked-engine-changes]], [[project-porting-clusters]],
[[project-gpup-particles]], [[project-rules-rendering-dx12]]
