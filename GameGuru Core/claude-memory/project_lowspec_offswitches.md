---
name: project-lowspec-offswitches
description: "The ultra-low-spec brutal off-switch set in Graphics & Performance - what ships, what each is worth measured, and which ideas turned out to be already covered."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-24T06:09:18.353Z
---

Brief (Lee, 2026-08-24, overnight autonomous run): *"further improve performance by adding more
brutal off switches to the Graphics and Performance component panel... allowed to reduce visual
contents and reduce fidelity in the pursuit of ultra-low-spec support."*

Round 1 (2.94): Terrain / Trees / Grass / Water Off — see [[project-terrain-vt-perf]].
Detail + all measurements: `GameGuru Core/NIGHT_INVESTIGATIONS_2026-08-12.md` §3.08–§3.11.

## ⚠⚠ 3.25 CHANGED THIS SET (2026-08-26) — READ BEFORE TOUCHING ANY SWITCH

After Lee tested on a **6-year-old AMD card**:
- **Terrain Off → Terrain Bake**, **Water Off → Water Bake**. Both still do the same removal
  underneath; each now leaves a cheap stand-in behind instead of a hole. Harness
  `SET_BAKETERRAIN` / `SET_BAKEWATER` / `SET_BAKERES` / `DUMP_BAKE`. The OLD `SET_TERRAINOFF` /
  `SET_WATEROFF` names still exist and still drive the raw removal — sweeps use them.
- **Post Effects Off and Simple Sky are GONE from the panel** (neither did anything visible on his
  card). Globals + `nopostfx`/`simplesky` ini keys deliberately kept.
- **New: Reduction Scale**, a sub-control under "Lower Animation & LUA Speed".
- ★★★ **EVERY control in this section is now PER-LEVEL**, saved in `visuals` like Post Processing.
  Defaults all-neutral; "Reset Visuals" unticks them. So a switch is no longer session-scoped and
  DOES write into the user's level — the opposite of the 2.94 design, on Lee's instruction.

★ Terrain Bake measured on a FAST card (not the target): FPS 174→228, VRAM 3.17→2.35 GB.
Detail: `NIGHT_INVESTIGATIONS_2026-08-12.md` §3.25.

## Shipped this round (all DEFAULT OFF / neutral)

| switch | setup.ini | harness | measured on Aztec |
|---|---|---|---|
| Shadows Off | `noshadows` | `SET_SHADOWSOFF` | ★ **−1.18 ms GPU busy, +17% FPS** (146.8→171.7) |
| Post Effects Off | `nopostfx` | `SET_POSTFXOFF` | −0.17 ms (only bloom+shafts run here) |
| Ambient Occlusion Off | `noao` | `SET_AOOFF` | −0.09 ms |
| Simple Sky | `simplesky` | `SET_SIMPLESKY` | ~0 — Aztec uses neither vol. clouds nor realistic sky |
| Occlusion Culling Off | `noocclusion` | `SET_OCCLUSIONOFF` | ⚠ **a LOSS: −15% FPS.** Shipped as an honest A/B |
| Particle Density % | `particlepct` | `SET_PARTICLEPCT` | ⚠ implemented, NOT measured (no particles on Aztec) |
| Object Detail Distance | `objectculldist` | `SET_OBJCULLDIST` | ★ **+9–10% at 1000–2000**, U-shaped (see below) |
| Texture Detail (Full/Half/Quarter) | `texturedivide` | `SET_TEXTUREDIVIDE` | ★ Aztec **−211 MB VRAM**, +3% — but see the caveat |

★★★ **Object Detail Distance has a U-shaped curve.** The win peaks at 1000–2000 units and comes
from **shadow casters**, not triangles — `Shadowmap Rendering` halves while POLYS does not move.
At 500 it turns into a NET LOSS despite POLYS −40%, because a quarter of the screen is then
mid-dissolve and Wicked's dither fade is ALPHA-TESTED: dearer per pixel than the opaque draw it
replaced. Tune it watching `Shadowmap Rendering`, and stop the moment GPU busy rises again.

★★ **3.19 (2026-08-25): Texture Detail now applies LIVE** — objects AND terrain, no reload.
Lee reported it as not working; it was working exactly as built (load-time) and that is
indistinguishable from broken. `wi::resourcemanager::gg_ApplyTextureDivideLive()` re-creates every
`.dds` IN PLACE (transplants Load()'s result into the ResourceInternal materials already hold, one
descriptor-epoch bump), and the terrain gets the 1.13 fast-repaint latch so its baked VT tiles
re-bake. Panel/harness arm it; setup.ini keeps the value-only path.
★★ **Judge it by `gg_texdivide_bytes_before/after`, NEVER driver VRAM** — VRAM read
2709→2773→3061 MB going 1→4→2 (monotonically UP) because the allocator keeps freed heaps and
editor streaming had already gone below the divided size. Real numbers, 242 textures on Grand
Canyon: **Full→Quarter 218.8→59.6 MB (−73%)**. ⚠ **Full→Half is a small LOSS in the EDITOR**
(218.8→238.3) — genuine: the divide opts out of streaming, which had already gone lower. An
exported game never streams, so Half saves there (UNMEASURED). Making the two compose is the real
fix and is open.

★ **Object Detail Distance only worked DOWNWARDS until 3.19** — the re-tighten was
`if (dist < s_prevCull)`, and the scan loop deliberately skips already-capped objects, so raising
the slider changed the global and nothing else. `<` → `!=`.

★ **Texture Detail is an ENGINE change** (`wi::resourcemanager::gg_texture_divide`, engine
`b40fc4d2`, delta row filed). It creates every DDS from its 2nd/3rd mip at LOAD time — not a
streaming target, because streaming is **editor-only** and so useless to the players it is for.
⚠ Its measured −211 MB UNDERSTATES it: the editor was already streaming those textures down and
this opts them out of streaming, so the two partly cancel. The exported-game saving should be
larger and is **UNMEASURED** — the harness cannot drive an exported build.
⚠ GG's own loaders (terrain SVT ~480 MB, tree billboard atlases, grass) bypass the resource
manager entirely and are untouched.
✅ Safety: Trapped and RPG Template — the two levels that crashed on the GGMAX 1.73
block-alignment bug this path re-enters — both load clean at Quarter.

## Already covered — do not build these

- **Render resolution divide** → the **FSR combo already in the panel** drives `resolutionScale`
  to 1/1.3 … 1/2 native. Biggest low-spec lever in the product, predates all this.
- **"Simple flat water"** → the expensive half is the existing **Reflections** checkbox
  (`getReflectionsEnabled()` gates the planar reflection pass). What a new switch would add is
  flat waves, and `Ocean - Simulate` is 0.18 ms. Not worth a third water control.

## House rules for adding one

- Session global + `setup.ini` (INT passthrough) + harness `SET_*` + panel tick, matching 2.94.
- Default OFF, and the off position **bit-identical** — that makes it its own A/B control.
- ★ **Capture the caller's value on the ON edge, write it back on the OFF edge.** Forcing "off"
  each frame and leaning on `GGApplyVisualsNow()` to restore is ONE-WAY: measured, it restored
  bloom and light shafts but not shadows or AO. Print a `restored` row in every ladder.
- ★ Grep the **getter** (`getShadowsEnabled()`), not the member — I nearly declared five working
  levers dead because the call sites use accessors.
- ★ Before believing a null result, check the TEST: is the effect even running on this level, and
  is the knob's range inside the level's scale? Both bit me this round.
