---
name: project-ssao-msao
description: "Visuals 'SSAO' is Wicked MSAO (MiniEngine) in DX11 and DX12 alike; its tolerances assume a ~100 m far plane (GG: 500000) and its depth pyramid is point-decimated - dark swimming patches / popping bands on thin railings. 3.126/3.126b engine fixes + how to measure AO"
metadata:
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-10-22T22:26:49.894Z
---

**The Visuals "SSAO" combo is {Disabled, Enabled} and Enabled = `AO_MSAO`** in BOTH builds (same code, same constants
at Power 1.0; range/sample count unused; radius is screen-space, so GG's 39.37 units/m does NOT apply). A DX11-vs-DX12
AO difference report is usually LIGHTING: AO multiplies indirect light only, so an ambient-only surface (check
SET_TANGENTVIS 17 = direct diffuse) shows the full AO. Lee's DX11 shot had the same blotch (2026-10-22).

**Two MSAO weaknesses, both made worse by GG's huge far plane:** (1) blur/upsample tolerances are absolute in
depth/zFar, tuned for ~100 m - 3.126 `gg_msao_reference_far` rescales them exactly (as if zFar = 10000); (2) the depth
pyramid keeps the top-left texel of every 2x2 - thin posts/gaps pop in and out of coarse levels with a sub-pixel turn -
3.126b `gg_pick_depth` takes the farthest depth at depth edges. The pick ALONE made the railing worse (97% dark vs 90%);
only the pair helps. Trade-off: alternate-row lightening of distant grazing-ground AO (only even rows have an exact
half-res twin); 10000 halves it vs 4000. Reverts: setup.ini `msaoreffar=0`, `msaodownsample=0`.

**Why:** Lee's railing report 2026-10-22 (Lee-confirmed 10-24, "better than DX11"); "it was better in DX11" was false for the AO texture - the far-plane lever
(RUN_LUA SetCameraPanelDistance 4000) proved the mechanism in one A/B.

**How to apply / measure:** same-session toggles (SET_MSAOREFFAR, SET_MSAODOWNSAMPLE), SET_TANGENTVIS 13 = the AO
buffer, SET_CAMERA yaw steps of 0.025-0.05 deg; score pop-in as 16x16 block-mean jumps between consecutive frames and
swim as per-pixel temporal std on an edge-free mask - raw pixel diffs reward blur (crisp edges alias). AO factor =
AO-on / AO-off final at the same pose isolates AO from texture. Grass (hair) is not drawn by mode 13 and moves with
wind - it contaminates AO-view stats. [[project-measuring-rules]] [[project-rules-rendering-dx12]] [[project-wicked-engine-changes]]
