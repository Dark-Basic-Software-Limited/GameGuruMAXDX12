---
name: project-vram-retention
description: "VRAM accumulating across level loads: three fixed defects, the named VRAM census as the instrument, and the one still-open cause (a shadow-packer input that survives a level load)."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-19T23:30:47.040Z
---

**2026-09-19/20. Lee: "find where the VRAM is being retained across level loads. We need zero VRAM
leaks!"** Game `ec5e38a7`, engine `df7d78e3`. Notes §3.65-§3.66.

★★★ **THE INSTRUMENT IS `DUMP_VRAM`** (census, engine 1.70): every D3D12MA allocation recorded WITH
ITS DEBUG NAME at create, erased in `~Resource_DX12`. Load A -> B -> A in one session and diff the
two A dumps: content is constant, so anything extra is genuinely retained. Turned "2 GB
unaccounted for" into a named list in one run. Tools: `tools/vramleak_probe.sh`,
`tools/vram_union_sweep.sh` (19 demos + a repeat of demo 1 at load 20), `tools/union_analyse.py`,
`tools/compare_runs.py`.

**RESULT: per-load accumulation eliminated.** 19-demo single session:
slope **+90.1 MB/load (r2 0.71) -> +5.2 MB/load (r2 0.01)**; load1->load19 3107.7->5121.6 MB
became 3075.4->3051.8; peak 5743->4231; over the 4096 C3 gate 16 of 19 -> 1 of 19.

**Four defects, all the same shape - a per-level thing living in a process-global:**
1. **Stale terrain paint map, +142 MB.** `pMaterialMap` is persistent 4096x4096; the loader wrote it
   only `if (FileExist("<size>.ptd"))` with **no else**, so an unpainted level inherited the previous
   level's paint - and `SetupWickedTerrainMaterials` scans that map to decide which materials to
   instantiate. Not only memory: the same map feeds the blend/slot mapping.
2. **Orphaned material slot entities.** Auto slots called `Entity_Remove`; painted/layer-1 slots did
   not. The 08-05 tail truncation covered entities beyond the new SIZE, not those overwritten in place.
3. **`GPUP_DeleteTexture` was an EMPTY STUB, +28 MB.** Whole teardown chain correct and running into
   a no-op. ⚠ Inherited from DX11, not a port regression. ⚠ Half a fix: `RenderPassAttachment` holds
   a Texture BY VALUE, so the 5 render passes must be released too - clearing the members alone took
   `imageTex` to zero and left `renderTex` untouched.
4. **Depth-chain keep-alive, +67 MB.** GGMAX 2.05 diagnostic ("Remove once the hunt closes") pushing
   4 depth textures into a static vector every `DeleteGPUResources`. Hunt closed on a terrain DDS.
   ⚠ Gated on `gg_dred_armed` = `dred.txt` present, i.e. **every dev machine, including the one all
   VRAM baselines were measured on**.

⚠⚠ **STILL OPEN - the shadow atlas is NOT the leak.** Same level asks the packer for **5120x1024
cold and 12288x4096 after a heavy level**, same 5 rects, stable over 7000+ frames. The atlas
faithfully allocates what it is asked. An INPUT to the shadow packer survives a level load.
**Ruled out by measurement:** the atlas, the release timing, a load transient, and
`visuals.iShadowSpotCascadeResolution` (`visuals_load` assigns the 1024 default at
M-Visuals_part0.cpp:1339 BEFORE parsing at :1717). ★ **Next lead:** `DUMP_SHADOWRECTS` says the sun
rect is 512x512 in BOTH cases while the packer says 5120 vs 12288 - the harness accessor and the
packer disagree, so instrument the rects INSIDE the packer. Arm the trace with `gg_atlas_trace.txt`
beside the EXE.

★★ **Releasing the shadow atlas at a level load DOES NOT WORK** (tried, measured, reverted):
release and create land on the SAME FRAME, before the incoming level's visuals are applied, so the
new atlas captures the OUTGOING level's cascade resolution and the grow-only allocation keeps it.
Replaced by `wi::renderer::GG_ArmShadowAtlasShrink()` - a one-shot permission to shrink when the
packer wants <= half, which is order-independent.

★★★ **Method, all paid for tonight:**
- **A two-level A/B/A cannot separate two leaks that both predict "current union previous".** I
  reported a working fix as doing nothing because of this. Use **A -> B -> C -> A**.
- **Four plausible theories died to measurement** in one session. Reasoning generated all four.
- **A one-shot diagnostic reports the wrong moment** - the first post-arm sample looked correct;
  the number that mattered existed 30 s later. Make it periodic.
- ⚠ **`wi::backlog` writes log.txt only from its DESTRUCTOR** - a taskkill loses everything, and an
  empty log looks exactly like code that never ran. Append+flush for anything a kill must not lose.
- ⚠ **First launch after an ENGINE build needs a long warm budget** (300 failed twice, now 900); a
  cold launch fails identically to a code regression.
- ⚠ **Blank-frame detection by BYTE SIZE is wrong** - conflates "blank" with "dark". Use variance.
- ⚠ **`rm -rf` on an output dir deletes the LOCKFILE of a run still using it** - two probes then
  drove one `auto_command.txt`. Kill the runner first.
- ⚠ **An unmatched `sed` silently passes text through** - a derived script kept the original's
  output path and began overwriting the baseline. Assert the substitution matched.

Related: [[project-single-session-soak]], [[project-vram-floor]], [[project-measuring-rules]],
[[project-rules-rendering-dx12]], [[project-playgame-crash]]
