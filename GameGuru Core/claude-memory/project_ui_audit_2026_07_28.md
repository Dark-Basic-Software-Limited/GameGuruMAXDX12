---
name: project-ui-audit-2026-07-28
description: "Level-editor UI hookup audit COMPLETE 2026-07-28 (autonomous, 330 widgets, 8 regions, workflow + adversarial verify). Fixes shipped c5354a92 (gamma/desaturate wired, View Options grass/terrain real levers, storyboard-cancel bug, 11 dead controls hidden). Open judgement items list inside — boundary-edge checkboxes still dead, gamma is an approximation."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-07-28T18:45:00.688Z
---

# Level-editor UI hookup audit — 2026-07-28 (user gardening, autonomous 2h)

**Method**: 8-region workflow fan-out (330 widgets inventoried, every binding traced to a real
consumer) + adversarial verify agents + solo sweeps. Fixes shipped `c5354a92`, screenshot-verified
on TESTPRO1 island (incl. catching my own inverted DeSaturate mapping — scene went grayscale,
proving the wire-up live, then fixed: DX11 semantics are 1 = full color).

## Fixed / wired (all in c5354a92)
- **Gamma** slider → tonemap `setBrightness((fGamma-2.2)*0.15)` (SetGamma gone from engine; neutral at default; applied live + on load in Wicked_Update_Visuals).
- **De Saturate** → `setSaturation(fDeSaturate)` — DIRECT mapping, 1 = full color (misleading name, DX11 default 1.0).
- **View Options grass/terrain checkboxes** (Editor + Level columns) → new `GGTerrainWicked_SetGrassVisible/SetTerrainVisible` sweeps of the LIVE Wicked entities (G-key layerMask / O-key SetRenderable mechanisms; `wickedTerrainHidden` promoted to file scope). Legacy `draw_enabled` flags only gated the dead custom-draw path. Called from Wicked_Update_Visuals; Editor Veg checkbox now applies immediately.
- **"PP Size" relabeled "Wind Wave Size"** (drives weather->windWaveSize; was cryptic legacy name).
- **File > Back to Storyboard: Cancel now cancels** (unconditional bStoryboardWindow=true removed).
- **AI "Show Navigation Debug Visuals"** tooltip now says it acts in test game.

## Hidden (dead engine paths; fields + save/load KEPT for level-file compat; commented UI, dated)
Transparent shadows (unconditional in new Wicked), Front Shadows Priority (**global read by NOTHING — never worked on any build**), Bloom Strength (setBloomStrength removed from RenderPath3D), Global Probe Brightness (probe SetBrightness removed), PP Snow/Dust + Disable When Indoor + PP Alpha + PP Voxel Steps (DX11 voxel weather gone), Water Fog Start/Maximum/Minimum (superseded by Underwater Fog / delta 1.23).

## Round 2 (same evening, user directives) — game `d4f8a29a`, engine `e82fbcc3`
- **Boundary checkboxes NOW WORK**: wi debug lines (3D = tall wire box, 2D = ground-sampled ring, 64 segs/edge) drawn in GGTerrainWicked per-frame from the SHOW_MAP_SIZE flags. NOTE: SHOW_MAP_SIZE defaults ON → editor shows the orange ring by default (DX11 parity).
- **Probe Brightness RESTORED** via engine delta 1.55 (ShaderScene.padding6 → gg_envprobe_brightness, lightingHF sample-time multiply; live drag, no probe re-render).
- **PORT TRANSLATE clamps at level load** (M-Visuals parse): Gamma ∉[0.5,5]→2.2, DeSaturate ∉[0,1]→1, BloomStrength ∉[0.1,3]→1, EnvProbeBrightness ∉[0.01,10]→1. Field-proven: island.fpm carried **Gamma≈15** (dead-slider-era) → washed scene once gamma went live.
- **Delta 1.54 (true tonemap gamma + bloom strength) PARKED/REVERTED**: at NEUTRAL pushed values the viewport washed to 3.4× luma (60→205, uniform near+far); bisect convicted the two shader lines, yet a corner-probe visualizer of the pushed pair never rendered in the viewport — the live editor path reads the push differently than assumed. **Do NOT re-attempt without a PIX capture** (which tonemap dispatch feeds the viewport + actual root-constant contents). Gamma keeps the brightness-offset approximation; Bloom Strength re-hidden.
- Hygiene markers ("Ready for deletion for hygiene") added instead of deletions (user directive): GridPopup family, legacy imgui_Customize_Water, ADDCONTROLSTOSTAUSBAR block.
- PP Snow return = real Wicked weather feature (rain emitter repurpose), not a small delta — still report-only.

## Still DEAD — needs user judgement (report given 2026-07-28)
- **View Options 3D/2D boundary-edge checkboxes** (Editor + Level, 4 boxes): need a boundary overlay port (SVT tint or wi debug lines) — the old GGTerrain transparent draw is dead code.
- **Gamma is an approximation** (brightness offset, not a power curve) — true gamma = small engine tonemap delta if wanted.
- Bloom Strength / Probe Brightness / PP Snow could return via small engine deltas (tonemap bloom mix, probe brightness re-add, Wicked rain/snow emitter).
- Detailed-list single-click "add to cursor" branch unreachable (M-GridEdit_part1.cpp ~9519) — click-semantics decision.
- Invisible dead code (hygiene deletes on offer): GridPopup() family (M-GridEdit_part8.cpp ~711-915), legacy imgui_Customize_Water() (M-TerrainNew_part3.cpp ~1384), ADDCONTROLSTOSTAUSBAR status-bar block (M-GridEdit_part1.cpp ~1980-2049).

Everything else (300+ widgets incl. all menus, toolbars, library panel, sky/water/weather/camera/physics panels, Game Elements grid) verified HOOKED. Graphics & Performance panel was already audited 2026-07-25 ([[project-gfx-perf-panel-audit]]).

Related: [[project-shadow-system]], [[project-gfx-perf-panel-audit]].
