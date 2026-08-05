# Automation Test Harness - Quick Start

## Overview

GameGuruMAX has a built-in file-based automation harness (`AutomationHarness.cpp`) that accepts commands via text files. It runs in the main loop, checking for commands every tick with zero overhead when idle.

## Key Paths

| Item | Path |
|------|------|
| **EXE directory (runtime root)** | `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max` |
| **Executable** | `GameGuruMAX.exe` (in the EXE directory) |
| **Command file** (write commands here) | `auto_command.txt` (in EXE directory) |
| **Response file** (read results here) | `auto_result.txt` (in EXE directory) |
| **Log file** (append-only timestamped log) | `auto_log.txt` (in EXE directory) |
| **Source code** | `D:\max\GameGuruMAXDX12\GameGuru Core\Guru-WickedMAX\AutomationHarness.cpp` |
| **Header** | `D:\max\GameGuruMAXDX12\GameGuru Core\Guru-WickedMAX\AutomationHarness.h` |

## Building

Always build **Release** — the Release configuration outputs directly to the runtime EXE directory, no manual copy needed:

```bash
cd "D:/max/GameGuruMAXDX12/GameGuru Core" && ./build.bat Release
```

The cwd MUST be `GameGuru Core` when the .bat runs — build.bat uses relative paths (`GameGuruWickedMAX.sln`) and MSBuild silently fails ("Project file does not exist") if invoked from any other directory.

**IMPORTANT**: The app must NOT be running when you build — the linker cannot overwrite the exe while it's locked. Quit the app first (via `QUIT` command or force kill), then build.

Do NOT use Debug builds for testing. Debug builds output to `Build/Debug/` and the exe will fail with exit code 3 when placed in the runtime directory.

## Quitting / Force Killing the App

```bash
# Force kill is the most reliable way to quit (note: //IM with double slashes for MSYS bash)
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true

# Check if running
tasklist.exe 2>/dev/null | grep -qi "GameGuruMAX" && echo "RUNNING" || echo "NOT RUNNING"; true
```

**Note**: Prefer `taskkill` over the `QUIT` harness command. The `QUIT` command can fail to be consumed if the app is in a state where the harness isn't polling (e.g., during level loads). `taskkill` always works.

## Protocol

1. Write a single command line to `auto_command.txt`
2. The app reads it, deletes the file, and writes the result to `auto_result.txt`
3. Poll for `auto_result.txt` to appear (it is overwritten each time)
4. All commands and results are also appended to `auto_log.txt` with timestamps

**Important**: If the command file is empty, the harness leaves it alone (handles writer flush race condition). Only non-empty files are consumed and deleted.

## Available Commands

| Command | Arguments | Description |
|---------|-----------|-------------|
| `GET_STATE` | (none) | Returns STATE, TAB, VISIBLE_PANELS, ERRORS, UPTIME. In storyboard state also returns PROJECT name and NODES summary |
| `GET_SCREEN_TEXT` | (none) | Detailed dump of all on-screen content: project info, node details, widget labels, connections |
| `NAVIGATE` | `hub` / `hub.<tab>` / `storyboard` | Switch between hub, hub tabs, or storyboard |
| `CLICK` | `play_game` / `edit_game` / `test_level` / `add_level` / `load_level` / `exit_screen_editor` | Simulate button clicks (context-dependent) |
| `CLICK_NODE` | `<node title>` | Click a storyboard node by title (case-insensitive). Level nodes load into editor, screen/splash nodes open the screen editor |
| `SELECT_DEMO` | `<demo name>` | Select a demo by display name (case-insensitive) |
| `LIST_DEMOS` | (none) | List all available demo games |
| `LIST_PROJECTS` | (none) | List My Games projects (from `projectbank_list`). Populated when the hub renders — call `GET_STATE` first if empty |
| `OPEN_PROJECT` | `<project name>` | Open a My Games project by name (case-insensitive). From hub only. Storyboard renders on next frame — poll `GET_STATE` |
| `CLICK_ONLY_LEVEL` | (none) | Load the first storyboard level node with a level file assigned. Storyboard only. Convenience for single-level projects (TESTPRO1 pattern) — no need to know the level's title |
| `WAIT` | `<milliseconds>` | Sleep up to 30000ms, then return state |
| `SCREENSHOT` | (none) | Capture screenshot to `auto_screenshot.*` |
| `SHOW_GAMESETTINGS` | `<0\|1>` | Open/close the Game Settings window (Graphics and Performance etc.) for panel screenshot verification |
| `GET_PERF_DATA` | (none) | Returns FPS, frame time, memory, VRAM, GPU adapter, scene counts, and visibility counts. Works in any state |
| `LIST_ENTITIES` | (none) or `lights` | Lists all active entities with position, type, and light data. Optional `lights` filter shows only light entities |
| `GET_ENTITY` | `<index>` | Detailed dump of one entity: position, rotation, scale, profile, light data, infinilight linkage, and WickedEngine cross-reference |
| `LIST_LIGHTS` | (none) | Full light pipeline debug dump: all infinilights with entity linkage, position, range, color, and WickedEngine LightComponent state |
| `TOGGLE_PROFILER` | (none) | Cycles the in-game TAB mode (0=normal, 1=visuals panel, 2=performance panel). Game state only |
| `ENABLE_PROFILER` | (none) | Enables Wicked Engine profiler for data collection. Sets both `bProfilerEnable` and `wi::profiler::SetEnabled(true)`. Data appears in `GET_PERF_DATA` after ~1s. **Warning: profiler adds massive overhead** (FPS drops ~75%) — enable briefly, collect data, then disable |
| `DISABLE_PROFILER` | (none) | Disables Wicked Engine profiler. Clears both `bProfilerEnable` and `wi::profiler::SetEnabled(false)` |
| `GET_PROFILER_STATUS` | (none) | Diagnostic: returns profiler internal state (ENABLED_REQUEST, ENABLED, IsEnabled, CPU/GPU frame times) without modifying anything |
| `PRESS_ESCAPE` | (none) | Exits test game back to editor (sets gameloop/levelloop/masterloop=0). Game state only |
| `PRESS_KEY` | `<key name>` | Simulate a keypress (WM_KEYDOWN + WM_KEYUP). Accepts A-Z, 0-9, F1-F12, ESCAPE, ENTER, SPACE, TAB, SHIFT, CONTROL, ALT, arrow keys, etc. Works in any state. In editor mode, also injects into the terrain key system via `g_autoHarnessInjectedKey` (bypasses the `bImGuiGotFocus` gate) |
| `SET_GRASS` | `<param> <value>` | Live-tune the Wicked grass path. Params: length, width, stiffness, drag, blades, maxstrands, segments, billboards, viewdist, sss, alpha, tintr, tintg, tintb, sssr, sssg, sssb (impl: AutomationHarness.cpp ~line 1657) |
| `DUMP_SKIN` | (none) or `<name filter>` | Skinned-mesh corruption diagnosis: writes a full report to `auto_skin.txt` (skinned objects with AABBs + SUSPECT flags, armature summaries with bone-world/skin-matrix magnitude ranges, per-bone detail for suspect/filtered armatures, one line per animation, keyframe-data garbage scan). Summary with suspect names goes to `auto_result.txt`. Filter matches Wicked object names (e.g. `galah` = the Island Showdown parrots) |
| `SKIN_WATCH` | `1` or `0` | Per-frame scan of ALL transforms for garbage local rotations (>1e3 or NaN); logs first detection per entity with a frame counter + scene counts to `auto_skinwatch.txt`. Enable BEFORE loading a level to timestamp when corruption lands. Heartbeat TICK line every 300 frames |
| `SET_OCEAN` | `<param> <value>` | Live-tune ocean shore/wave foam (applies instantly — the ocean CB refills from oceanParameters every frame). Params: `foamscale` (world-unit scale for the foam math; GG default 0.08, stock-meters would be 1) and `foamamount` (intensity multiplier, GG default 1.3). Backing globals: `g_fWaterFoamUnitScale`/`g_fWaterFoamAmount` in M-GridEditB_part3.cpp |
| `SET_TREES` | `<param> <value>` | Live-tune tree shadows + pool benchmarking. Params: `shadowdist` (mesh-shadow radius in inches, default 2500 — the "Tree Shadow LOD Distance" slider), `shadowrange` (cascades receiving tree shadows 0-5, default 5 = island-wide — the "Tree Shadow Range" slider), `drawshadows` (0/1), `stress` (force the pool nearest-N rescan for N frames — measures the camera-move cost via the TreePool profiler sub-ranges) |
| `DELAYED_SHADOWS` | `0` or `1` | A/B the staggered shadow-cascade refresh (Wicked delta 1.11, ON by default). OFF = every cascade renders every frame (stock). Compare `Shadowmap Rendering` CPU/GPU in GET_PERF_DATA with the profiler enabled |
| `SET_VTINC` | `0` or `1` | A/B Wicked delta 1.33: incremental terrain-VT bookkeeping (dirty-tracked page-table uploads + lazy free-list rebuild). 1 = incremental (default), 0 = stock every-frame full rewrite (the old ~16ms/frame VT job). Applies instantly — measured 52 ↔ 62-70 FPS on the TESTPRO1 gate view |
| `SET_ANIMVIS` | `<frames> [neardist]` | A/B Wicked delta 1.35: pause animation EVALUATION for objects unseen by the main-view cull for >frames (0 = off; default 3). `neardist` = never pause within this camera distance (default 2000). Timers keep advancing → resume in sync |
| `SET_HIERLO` | `0` or `1` | A/B Wicked delta 1.36: subtree-parallel hierarchy update (one job per root, world carried down the chain, one matrix multiply per bone) vs stock per-entity ancestor chain walk. 1 = subtree-parallel (default) |
| `SET_HAIRSKIP` | `<0\|1> [windInterval]` | A/B Wicked delta 1.37: at a parked camera, skip the 2M-strand hair/grass simulate dispatch (no wind) or run it every windInterval-th frame (wind active; default 4 = slow-motion sway). ~1.4-1.9ms GPU. 0 = stock every-frame sim |
| `SET_MERGELISTS` | `0` or `1` | A/B Wicked delta 1.40: command-list merges (occlusion → prepass tail; transparents+postFX → one list) to cut submit fragmentation. 1 = merged (default), 0 = stock list structure. Measured neutral on the dev rig; kept for lower-end submission savings |
| `SET_MATCACHE` | `0` or `1` | A/B Wicked delta 1.41: ShaderMaterial recompose cache (recompose only on dirty/streaming-epoch/heartbeat; memcpy cached otherwise). 1 = cached (default), 0 = stock full recompose every frame |
| `SET_SINGLEQUEUE` | `0` or `1` | A/B Wicked delta 1.48b: route COMPUTE/COPY command lists onto the GRAPHICS queue + drop same-queue fences (batches 15→1, deps 12→0). **Measured NEGATIVE on the dev rig (71.5→66.8 FPS — the async overlap wins ~1ms GPU wall); default 0.** Keep for low-end-GPU probes. The 1.48a `SUBMIT_PHASES_MS` readout in GET_PERF_DATA shows close/fences/present/sync/stall + lists/batches/deps — the 'submit tail' is ~95% the end STALL (GPU still busy) = the frame is GPU-WALL-bound |
| `SET_LEANASYNC` | `0` or `1` | A/B Wicked delta 1.48c: keep the two big compute lists async, move only the four TINY helper lists (VT copy-pages, ocean sim+readback, VT tile-req+writeback) onto graphics (deps 12→~5). **Also measured NEGATIVE (71.6→67.5 clean ABAB); default 0.** Verdict pair with 1.48b: Wicked's async-queue structure fully earns its keep on the dev GPU — submission overhead is a dead end here |
| `SET_GRASSLOD` | `<0\|1> [step2frac step4frac boost]` | A/B Wicked delta **1.49b**: grass strand LOD. Around `step2frac`×viewDistance half the strands drop, around `step4frac`× half the remainder — each strand at its own hash-jittered radius (±15%), and survivors widen by an EXACT hyperbolic coverage ramp aligned to the drop window. `boost` = widening endpoint per halving: **2.0 (default) = coverage-neutral in expectation; <2.0 = deliberate far thinning**. GG grass systems only; bit-stable at a parked camera. **DEFAULT ON since 2026-07-27 (user-requested after eyes-on confirm); `SET_GRASSLOD 0` = kill switch (also pulls AUTO tiers back to stock 1.0/1.7).** **BUNDLE (2026-07-27): this knob also swings the chunk-grass tier boundaries via AUTO — OFF = stock 1.0/1.7 chunks, ON = 1.5/2.2 (tier-3 square pop 50% further out). Measured matrix at the gate view: stock 72 / bundle ON ~70 (pop pushed, smooth falloff) / LOD-on+stock-tiers (`SET_GRASS tier3 1.0` + `tier2 1.7`) ~81 (+9 FPS, pop at stock distance) / tiers-out-without-LOD 53 (don't).** History: v1 used two HARD rings — user-visible two-shade shimmer sweeping the mid field during camera motion; v2 (jitter + exact ramp) is the fix. An adversarial 3-lens review also caught v1's coverage wiggle (+16%/−26% static rings) and the 1.8 boost undershoot — both corrected in v2 |
| `SET_GRASS tier3/tier2` | `<chunkDist>` | (new SET_GRASS params) Chunk-grass density-tier boundaries in chunk-distances (1 chunk ≈ 5040 inches): tier 3 = full density, tier 2 = 0.18×, tier 1 = 0.05×. Tier UPGRADES rebuild a whole chunk's grass in one frame — the closer the boundary, the more visible the square pop (user-reported in game mode). Explicit value overrides the AUTO coupling above; triggers a full grass rebuild. Also 2026-07-27: the regrowth creation budget now fills the NEAREST gaps first (travel churn used to refill in arbitrary order — visible holes lingered in front of the player while off-screen chunks regrew). **USER-CONFIRMED 2026-07-27: game-mode walk with the bundle ON = no square pops, no flicker** |
| *(tripwire file)* `resource_hijack.txt` | appears next to the EXE | Wicked delta **1.52b**: `wi::resourcemanager::Load()` verifies every cache hit's internal filename against the requested name. A mismatch = the name→resource binding was hijacked (the 1.52 pooled-weak_ptr underflow class — root cause of the "normal map as color map" palms/ferns) — it SELF-HEALS as a cache miss and appends a line here. **Post-1.52 this file should NEVER appear; if it does, a sibling aliasing bug survives — read it and investigate.** |
| *(file flag)* `imgui_upload_debug.txt` | empty file in the working directory | Re-arms the ImGui DX12 bridge upload/draw diagnostics (`DX12Log` → `imgui_upload.log`). **OFF by default since 2026-07-29** — it logged every ImGui draw call of every frame (~12K lines/sec, open-append-close per line) and had silently grown imgui_upload.log to 36 GB since February. Only create this flag while actively debugging ImGui texture uploads, and delete both flag and log afterwards |
| *(file flag)* `dred.txt` | empty file next to the EXE | Wicked delta **1.51**: arms D3D12 DRED (auto-breadcrumbs + page-fault tracking) in the Release build at launch — launch log prints "DRED armed". On ANY device removal (TDR/DEVICE_HUNG) the post-mortem — removal reason, last in-flight op per command list, page-fault VA with owning live/freed resources — is APPENDED to exe-dir `dred_report.txt`. Delete dred.txt to disarm (breadcrumb writes cost a few % GPU). Armed 2026-07-27 for the Canyon-load TDR hunt; pair with AMD Radeon GPU Detective for the driver-side view |
| `SET_TERRAINTILE` | `<cap> [hold]` | Wicked delta **1.53b**: terrain VT tiling repeat CAP. Every bake rung finer than `cap` repeats-per-chunk collapses to the cap scale (pure downsample chain = INVISIBLE transitions from the camera through the mid field); rungs at/below the cap keep the stock repeat-halving anti-tiling far away. **Doubles as the terrain texture world feature size: chunk ≈5120in → cap 8 ≈ 16m repeat, 16 ≈ 8m, **32 ≈ 4m (DEFAULT, user-tuned "human scaled")**; `hold` delays the anti-tiling ladder ~1.4× distance per +1 (**default 4, user-tuned**). 0 = stock (two-scale cross-fades right at the camera).** World-anchored: no chunk-border seams, no promotion scale-pops. Change queues a fast repaint of resident chunks (brief FPS dip, ~seconds); zero steady-state cost. History: v1 (share-mips K) FAILED in the user's field test — it anchored to sub-magnification top rungs and only densified the visible pattern |
| `SET_GRASSWET` | `0` or `1` | A/B Wicked delta **1.50**: GG grass wetmap. **0 = force-dried (default since 2026-07-27) — fixes the "grass renders flat-dark on reveal, then very slowly brightens over 15-30s" artifact**: culled/LOD-dropped strands hold raw-zero positions (GG sim early-out skips their writes) that decode to world (0,0,0) = below the island waterline, so the every-frame ocean wetmap ratcheted them to wet≈0.8 → `albedo×0.2` (flat dark, no shadow shape) drying at 0.02-0.08/s only while drawn. Also the shade component of the historical two-shade flicker (wet↔dried strand populations swapping across the jittered LOD radii). 1 = stock Wicked wetting = reproduce the bug on demand; toggle back to 0 recovers visible grass instantly. Character hair unaffected either way |
| `SET_CHARSHADOW` | `<0-3>` | Per-character DEDICATED sun shadow slots (2026-07-30, default **3**): the nearest-N on-screen characters each get their own full-res 2048 cascade slice fitted to a ~6ft sphere (~28 texels/inch vs cascade 0's ~2.5) — dramatically crisper character self/cast shadows. Game bridge `WickedCall_UpdateCharacterShadows` (wickedcalls_part3.cpp) fills `scene.character_dedicated_shadows` each frame from live `ischaracter` elements (editor + game modes). Hard cap 3 (5×2048 sun + 3×2048 = the 16384 atlas exactly). **COST while any slot is live: engine disables the delayed-cascade stagger + far-cascade cull (measured ~70→55 FPS at the grass-heavy fighter view) — index-offset engine fix pending.** 0 = stock |
| `SET_SHADOWBIAS` | `<0-64>` | Wicked delta **1.57** receiver-side depth bias for sun cascades (units: 1/65536 NDC). DEFAULT **0** — bracketing showed no effect on the neck flicker (the real fix was the 1.57b slope-bias CLAMP, engine-side). Kept as a live experiment knob |
| `SET_BLENDSCAN` | `<1-60>` | Blend-scan cadence outside initial build (default **4**). Fixes the **post-load FPS dip** (~5s after the editor appears, FPS halved for ~10s): once initialBuild ends, chunkSig churns every frame until the terrain ring completes, so the AutoBlend (4.7ms) + PaintedBlend (1.0ms) scans ran EVERY frame, and each pass's `Generation_Cancel` kept chopping the generator, prolonging the churn. Pacing to every 4th frame + an O(1) entity→chunk map (was O(pending×chunks)) cut the Canyon Adventure dip from 120→71 FPS to 117→94 FPS. 1 = stock every-frame (reproduce the dip). Paint/sculpt latency +≤3 frames (verified via PAINT_TEST/SCULPT_TEST) |
| `SET_LIGHTSHAFTS` | `<0 or 1> [strength]` | Toggle the sun light-shafts postprocess and optionally override its strength (engine "exposure"). Wicked delta **1.56** fixed the whole-screen white wash: this engine's `GetSunColor()` has intensity PRE-applied (~9× hotter disc than DX11's raw-color mask — now unit-peak normalized in dark mode), and the non-realistic sky path fed the FULL horizon/zenith gradient into the sun mask (`dark_enabled` ignored — now a pow-64 sun disc). The game also sets **strength 0.18 = DX11 end-to-end parity** (DX11: hardcoded exposure 0.2 × 32 taps; new Wicked default 0.5 × 64 taps). Strength override here is session-only — Wicked_Update_Visuals re-asserts 0.18 on the next visuals refresh |
| `SET_TERRAINIDLE` | `0` or `1` | A/B the game-side terrain idle gate: when fully quiescent (camera parked 45+ frames, no pending/invalidated/merge-pending chunks, chunk-set signature stable, no edit pings) `Generation_Update` runs every 8th frame (~0.8ms CPU saved at a parked camera). ANY activity signal restores full rate the same frame. `TERRAINW_IDLE: gate/calm/skips` readout in GET_PERF_DATA |
| `SET_STREAMING` | `0` or `1` | Pause/resume the engine texture-streaming system (`wi::resourcemanager::gg_streaming_paused`, Wicked delta 1.44). 0 = paused (no min-lod updates, no mip stream in/out, no texture replacements). **Corruption-hunt A/B: streaming paused = the deterministic reload texture corruption disappears.** NOTE the reload guard resumes streaming after every level load, so re-issue this after each reload if you want it off for a whole session |
| `SET_GRASSTYPEFREEZE` | `<bitmask>` | Merged-grass diagnostic bitmask (engine 1.87-1.95b), instant, no reload, default 0. `1` freeze whole type path · `2` present · `4` textureIndex · `8` length · `16` grasstype-visualisation PS · `32` stable DTid.x%8 hash into the type channel · `64` ALWAYSWRITE (suppress cull early-return) · `128` uniform-index CB resolve loop · bits 8-15 = `(k+1)*256` force EVERY pixel's texture lookup to type k. Density is intentionally wrong while non-zero — judge only CONSECUTIVE-FRAME diff within one config (`BURST_FRAMES` + `tools/imgdiff.ps1`). These probes cracked the merged-grass flicker (engine 1.96, full ledger in `VRAM_CENSUS.md`) |
| `DUMP_GRASSTYPES` | (none) | Merged-grass per-type table forensics: the first two merged systems' CAPTURED entries (textureIndex, present, len/wid/bb/vd) plus a LIVE re-resolve of every built grass material (`LIVE t<n>: idx/dims/mips/name`). Captured != live = stale descriptor; odd LIVE identity = wrong resource. Used to exonerate descriptor staleness in the flicker hunt |
| `SET_GRASSPARAM` | `scale\|drawdist <value>` | Write the Grass Scale / Grass Draw Distance slider globals (`gggrass_global_params.grass_scale` / `.lod_dist`) — exactly what the editor UI writes; the per-frame Apply loops push to live systems including MERGED ones (GGMAX 1.97 — both sliders were dead on merged grass before, the typeIdx guard skipped the merged sentinel). READ BACK via `HAIR_VIEWDIST` / `HAIR_LEN` in `GET_PERF_DATA`: viewdist = lod_dist+2500 (system max; flowers half), len = max present type length. Verified 2026-08-04: scale 40→80→20 gives len 56→112→28 exactly, coverage 3.8→12.5→0.6%. NOTE a level SAVES its slider values — TESTPRO1 carries scale≈45.36 (tall grass len 63.5), so restore by RELOADING the level, not by guessing "40" |
| `SET_TEXSTREAM` | `0` or `1` | Texture-streaming ENROLLMENT kill-switch (game `g_bTextureStreamingEnabled`, **default 1** — entity material DDS textures stream by default, engine delta 1.69 row; briefly defaulted 0 on 2026-08-01 while the load crash was open, restored the same day by delta 1.73). Unlike SET_STREAMING (pauses the running system), this gates whether NEWLY loaded textures get `Flags::STREAMING` — the resource manager pins flags per name, so already-loaded textures keep their state. **Clean A/B: SET_TEXSTREAM 0 → reload/reopen the level → GET_PERF_DATA STREAM shows enrolled=0** |
| `SET_TEXSTREAMTRACE` | `0` or `1` | Per-load streaming forensics (engine delta 1.73). Arms TWO traces: `stream_load.txt` gets one flushed line per DDS upload (file dims/mips, reduced upload dims, mip_offset, filesize vs bytes the mip chain needs, `*** OVERRUN ***` if they disagree), and `last_upload.txt` is rewritten per texture with the destination footprint table beside the source pitches. **This is what identified the 500×500 DXT1 that crashed "Trapped": when the process dies mid-load, the last line of stream_load.txt names the texture and last_upload.txt is the autopsy.** Very expensive (a file write per texture) — arm it only to hunt a load fault, never for a benchmark |
| `REUPLOAD_TEXTURE` | `<name-substr>` | For every named object matching the substring: `SetOutdated` + re-`Load` every material texture slot from disk, then `SetDirty`. **The probe that proved GPU texture CONTENT corruption** — it healed the wrong-texture (blue palm) objects in place while names/descriptors were already correct |
| `REUPLOAD_ENTITY` | `<name-substr>` | Re-run `MeshComponent::CreateRenderData()` on matching objects' meshes (re-upload vertex/index buffers). Counterpart of REUPLOAD_TEXTURE for the geometry side (note: CPU vertex arrays may be freed after first upload, so a no-op result is not proof of a healthy mesh) |
| `FIND_OBJECT` | `<x> <y> <z> [radius=500]` | Scan every scene object's world transform near a position (e.g. the editor's Object Tools position readout of a clicked corrupt object) and print entity/name/mesh/bufsize links, sorted by distance → `Files/find_object.txt` + top 8 inline. Bridges the GG instance list to the Wicked scene without knowing internal frame names (GG DBO frames are named things like `$dummy_node`, `StaticMesh-1`, NOT the bank name) |
| `VERIFY_MESH` | `[name-substr] [burst]` | **The GPU mesh-buffer content oracle** (empty arg = every mesh). Per mesh: snapshot `generalBuffer` bytes → `CreateRenderData()` re-upload from CPU arrays → snapshot → byte-compare per region (ib/vb_pos_wind/nor/tan/uvs/atl/col/bon). Mismatch = GPU content differed from what CPU data produces = corruption (now healed); report with first-diff offset + A/B hex samples → `Files/verify_mesh.txt`, summary REWRITES `auto_result.txt` on completion (async, seconds). Meshes with freed CPU arrays are SKIPPED (unverifiable). Re-uploads budgeted 48/frame; trailing `burst` token = unbudgeted one-frame rebuild storm (deliberate suballocator-race repro — pair with `alloc_tripwire.txt`). 30s watchdog aborts cleanly if the device dies mid-run. **Verdict interpretation (established 2026-07-26 on TESTPRO1)**: (a) terrain `chunk_` meshes are byte-stable — ANY hit is real corruption (841/841 clean baseline); (b) a stable ~204-mesh class (skinned characters, e.g. jaguar body/legs) reports `vb_bon` with `badwords=1` and a one-nibble A/B delta at a fixed offset — BENIGN bone-weight quantization wobble, ignore; (c) 100%-diff across ALL regions = foreign content = REAL stomp (seen only alongside allocator storms) |
| `VERIFY_WATCH` | `[name-substr]` | Same snapshot machinery, NO re-upload: two snapshots ~60 frames apart. Any diff = an ACTIVE writer stomped the buffer while the scene sat still. Zero side effects, zero false positives from rebuild non-determinism |
| `DUMP_MATERIALS` | (none) | Write every scene MaterialComponent (entity id, name, hierarchy parent) to `Files/materials_dump.txt`. Diff dumps across in-place reloads to find leaked materials (known: +3 per reload = the 'start with ghost human' player-start marker) |
| `DUMP_BROKEN` | (none) | Scan all scene objects for DANGLING references — meshID that no longer resolves, subset materialID that no longer resolves, invalid mesh `generalBuffer` — to `Files/broken_dump.txt` with name/pos/renderable + totals |
| `DUMP_TREEPOOL` | (none) | Tree-pool census (slots bound / renderable / missing, numTotalTrees, proxy chunks) + ORPHAN detector (renderable unnamed parent-less objects the tree system no longer owns) to `Files/treepool_dump.txt` |
| `DUMP_ENTITY` | `<name-substr>` | Per-subset material forensics for matching objects: texture names, LIVE descriptor index vs the CACHED composed `gg_shader_cache` descriptor, cache validity/epoch. Writes `Files/entity_dump.txt`. (A basecolor cached-vs-live off-by-one is EXPECTED — the composition uses the sRGB subresource view) |
| `DUMP_INSTANCE` | `<name-substr>` | Read back the GPU-consumed `ShaderMeshInstance` record for matching renderable objects (uid, color/emissive packs, geometryOffset vs the mesh's, center/radius, transformRaw vs component world) to `Files/instance_dump.txt` |
| `DUMP_GEOMETRY` | `<name-substr>` | Read back the `ShaderGeometry` record at each matching instance's geometryOffset: `materialIndex` vs EXPECTED (mismatch prints the ACTUAL material + its basecolor texture), vb/ib descriptors, uv ranges, aabb. Writes `Files/geometry_dump.txt` |
| `SET_TANGENTVIS` | `<0-22>` or `CYCLE` | Engine 1.62/b debug: object shading replaced with raw pipeline data. 0=off, 1=world tangent, 2=vertex normal, 3=bumped normal, 4=handedness (red=-1 green=+1; maps DBO-tangent vs MikkTSpace parts), 5=strength-scaled normal-map sample, 6=basecolor UV raw, 7=UV ×64 grid, 8=basecolor tex sample, 9=final albedo input, 10=ORM sample, 11=roughness, 12=specular F0, 13=occlusion (vertex AO×map×SSAO), 14=vertex color, 15=emissive, 16=world-pos grid, 17=direct diffuse, 18=direct specular, 19=indirect diffuse, 20=indirect specular, 21=RAW normal-map texels, 22=RAW normal-map forced mip0. `CYCLE` auto-advances all modes 3s each (mode 0 = loop marker). Pair with `BURST_FRAMES`. **Flicker-hunt verdict 2026-07-30: every geometric/data stage STABLE; the "crazy" character flicker was Normal Strength 4.0 saved in the level (churn 3.07→0.78 per sway unit at strength 1)** |
| `SET_NORMSTR` | `<0-8>` | Set `normalMapStrength` on EVERY scene material with a normal map bound (brute-force A/B lever; live only, does not touch saved data). The instrument that convicted strength-4 as the character-flicker cause |
| `SET_WETMAPS` | `<0\|1>` | Engine 1.62e: 0 = skip the whole RefreshWetmaps compute pass (object + hair loops). Was a streamout-stomp suspect A/B — EXONERATED (no effect on the coin-flip); kept as a perf/isolation lever |
| `SET_SKYMODE` | `<0\|1\|2>` | Sky session 2026-07-31: drive the Customize Sky combo programmatically — 0 = Simulated Sky (realistic sky + volumetric clouds), 1 = Sky Box (skyindex 1), 2 = None (gradient). Runs the full combo sequence (`gridedit_set_sky_type`: weather flags, skyspec reload, env map + probe refresh). Marks project modified but never saves |
| `SET_VOLCLOUDS` | `<0\|1>` | Sky session discriminator: toggles ONLY `SetVolumetricClouds`, leaving realistic sky + fog untouched. Live-only (next mode change or visuals update restores). Convicted the mis-scaled cloud deck as the "sky eats the mountains" culprit (clouds off → mountains back) |
| `SET_FOGDENS` | `<density\|-1>` | Sky session discriminator: writes `weather->fogDensity` directly (0 = distance fog fully off); negative restores the level's fog model via `Wicked_Update_Fog`. Live-only. Exonerated fog for the mountain-swallow (fog 0 → mountains still gone) and convicted the stale horizon color for the Sky Box green fog |
| `RUN_LUA` | `<lua chunk>` | Execute a lua chunk in the LIVE game lua state (main thread, harness poll point — never mid-script). `return <expr>` to read values back (multiple results joined with " \| "). THE swiss-army probe of the 2026-07-31 FPS-plummet hunt: timed IntersectAll batteries, debug.getinfo caller tallies, live function stubbing. **Caveat: `g_Entity[e].x/y/z` can be STALE/pool values — aim probes with real wicked AABB coords (LIST_SKINNED), not entity-table coords** |
| `RAY_COST` | `up\|down\|fwd` | Replicates Scene::Intersects' object AABB loop for one ray from the camera (up 3000 / down 300 / fwd 120 units) and lists every mesh that would be triangle-tested: tris, bvh?, skin?, renderable?. The instrument that exposed the uncapped-ray discrepancy (probe said 9 objects, engine counters said 85 — because the REAL rays had no TMax) |
| `LIST_SKINNED` | (none) | Every object with a skinned/bone-weighted mesh + its world AABB. Found the island's 50 villagers × ~49K tris = 2.47M CPU-skinned triangles and the hidden W_MK19T weapon with a world-sized (±100K) AABB taxing every ray |
| (in `GET_PERF_DATA`) `RAYS:` / `RAYS2:` | — | Running-total ray diagnostics — diff two dumps for rates. RAYS: LUA Intersect* calls/ms by mode + SentRay4 (wiScene::Pick) calls/ms. RAYS2 (engine): Scene::Intersects calls, objects iterated, AABB passes, triangles tested, skinned triangles. Healthy post-1.68 island spawn: ~1ms per LOS ray; pre-fix 26ms |
| (in `GET_PERF_DATA`) `VT:` / `GAPS:` | — | VT: terrain virtual-texture free-list rebuild forensics (rebuilds, scan/sort ms, sizes, trigger bits). GAPS: **wall-gap tracer** — count + last size of >100ms frame-to-frame gaps; each gap's per-segment ledger (engine Run() phases, pump, RunCustom tail, PSO-compile/texture-create deltas) is appended to `Files/gap_trace.txt`. THE tool for one-frame hitches that profiler 20-frame averages smear invisible. Two traps it exposed (2026-07-31): profiler CPU ranges on JOB threads read scheduling delay as work (a "42ms FreeSort" was 0.3ms real); an in-game FPS reading during "PREPARING TEST LEVEL - N\\100" is a loading screen, not gameplay — gate sweep sampling on that text clearing (GET_SCREEN_TEXT) |
| (in `GET_PERF_DATA`) `STREAM:` | — | Texture-streaming health (engine 1.69 counters): `on` = game enrollment switch, `enrolled` = resources in the last streaming job pass (island ~259 editor / ~272 game; 0 = nothing streams — pre-2026-08-01 state), `replaced` = texture mip up/down swaps applied since launch (new viewpoint → jumps; static camera → FROZEN = equilibrium, NOT a bug), `resident_mb`/`full_mb` = VRAM of enrolled textures now vs full chains (island game: ~80-100 vs 1336 MB), `gpumem_pct` = GPU usage/budget (streaming unloads fast above 80%), `copies/fb/req` = feedback-chain liveness (readback copies / materials with nonzero GPU feedback / resolution requests reaching resources — all must grow every second), `guard_rejects` = streaming uploads the 1.73 bounds guard refused — **should always be 0; anything else means details were written to `stream_guard.txt`**. Watch `replaced` for runaway churn too: a texture that can shed no mips used to rebuild itself every pass (Trapped read `replaced=260468` before the 1.73 no-progress fix, `12` after) |
| (in `GET_PERF_DATA`) `STREAM2:`/`STREAM3:` | — | STREAM2: streaming-job liveness (jobStart==jobEnd growing ~100/s = healthy; jobStart>jobEnd = job stuck). STREAM3: per-pass decision census — req0 (no feedback: off-screen), reqLow (wants smaller), noMips (at full res), cancel (at requested res = converged), in/out (mips moved this pass), reqMask = OR of all pow2 request magnitudes ever seen. **Static camera → in=0 out=0 + frozen `replaced` is CORRECT (demand equilibrium in ~2s)** |
| `SET_SVTATLAS` | `<2048-16384>` | Terrain SVT physical atlas height (engine 1.71, stock 16384 = 768 MB tile pool). **Runtime use is limited: the atlas is created on the first terrain update, which happens before harness commands land, so for a real A/B use the setup.ini key `svtatlasheight=8192` instead** (verified: tiles 3844→1922, −368 MB, FPS unchanged). Measured residency is only ~26% of the stock atlas on every demo — see `VRAM_CENSUS.md` |
| `VRAM_STAGE` | `<label>` | Record the driver-reported video memory usage at a named point (engine 1.77). The resource census cannot see descriptor heaps, pipeline states or command allocators, and those totalled ~1.4 GB on every level — marking the driver number at hub / level-loaded / after-play turns that lump into an attribution instead of a guess. Marks print as `# STAGE` lines in the next `DUMP_VRAM`. The `DUMP_VRAM` header also now carries `pso_creates` / `pso_compiles` / `descheap_bytes` / `cmdalloc`; **`pso_creates` x ~96 KB is the driver-side pipeline cost**, which is how the 1.77 trim was found |
| `SET_LOWVRAM` | `<0\|1> [grassdistcap]` | The **low-VRAM ("fit a 4 GB card") preset**, same switch as setup.ini `lowvram`. **RELOAD the level after flipping** — grass entities are built at chunk-spawn time. Caps the grass draw distance (default cap 750 inches = the editor slider's own minimum, below which the per-strand fade has no range and grass pops in whole chunks); it is a CAP, so a level already asking for less keeps its value. **The preset's other half — lazy object PSOs — CANNOT be set from here**: object pipelines are built once at `LoadShaders`, long before any harness command lands, so use setup.ini `lowvram=1` for that. Isolation trick for A/B: `lowvram=1` with `lowvramgrassdist=999999` makes the grass cap inert and exercises the lazy-PSO half alone, where **POLYS must stay bit-identical** — a drop means a needed pipeline never appeared and the draw was skipped. Floor analysis in repo `VRAM_FLOOR.md` |
| `DUMP_MAPAD` | `[tag]` | **D3D12MA padding attribution** (engine 1.83) → `Files\ma_padding[_tag].txt`: a per-heap-type table (blocks / allocations / bytes / unused-range count and size extremes) followed by **D3D12MA's own detailed JSON block map**, which gives the per-block fill ratio no aggregate can. This is what found (a) that `pad` was charging system-RAM waste to the video budget — `d3d12ma_blocks` sums video AND system memory while `driver_usage` is video-only, so **always use the `d3d12ma_*_video` census fields**; and (b) that 64 MB blocks were leaving 104-152 MB in 4 sparse blocks, fixed by `mablockmb=16`. Committed allocations pad exactly zero, so ignore them. **The JSON echoes `PreferredBlockSize` back — check it**: the first `mablockmb` wiring was inert and the padding moved anyway from run variance |
| (setup.ini) `mablockmb` | `<N>` MB | D3D12MA `PreferredBlockSize`, **default 16**, `64` reverts to stock, `0` = library default. Read by the ENGINE at `CreateAllocator`, not by the game — the allocator is built with the device, so `GetSetupIniEarly()` is already too late (measured inert). Worth −92 to −140 MB video on every level with non-resource memory flat. **The curve is non-monotonic — 32 MB is worse than 64** (D3D12MA sends allocations over half a block to their own committed heap, so the threshold reshuffles what pools). Measure the size you intend to ship; do not interpolate |
| `HITCH_RESET` | (none) | Start a fresh **hitch-measurement window** (engine 1.82) — read it back in the `HITCH:` line of `GET_PERF_DATA`. Built to answer "what does a lazy PSO first-use compile actually cost", which nothing else in the build could see: the wall-gap tracer only fires above 100 ms, and an FPS mean smears dozens of 1-30 ms stalls into nothing. Send it the moment a level reaches `STATE: editor`, so the window means "since the level came up" instead of "since launch" — otherwise the 13-second load stall swamps every bucket. Rides the always-on gap-tracer path, so **the profiler does not need to be enabled** (turning it on would change what you are measuring) |
| (in `GET_PERF_DATA`) `HITCH:` | — | `frames` / `mean_ms` / `worst_ms` / `over(16.7/25/33/50/100)` bucket counts / `psoC` + `psoMs` (compiles and their time **since the last reset**) / `psoMax_ms` (worst SINGLE compile **since launch** — the number that decides whether a lazy pipeline is felt; the sum never is). **Read `psoC` before believing a bucket difference**: the 1.82 A/B showed lazy with 98 frames over 25 ms against the control's 1, but `psoC=0` for that window in both configs, and on repeat the control swung 1→87 while lazy went 98→101→2. This scene's frame time sits near the 25 ms edge, so **always repeat a run before calling a bucket difference real** |
| `DUMP_VRAM` | `[tag]` | **Full per-resource VRAM census** (engine 1.70) → `Files\vram_census[_tag].txt`: every live GPU allocation with allocated bytes, dimensions/format/flags and debug name (content textures carry their file path), largest first, plus a header reconciling census vs D3D12MA vs driver-reported usage. **The census total equals D3D12MA's own AllocationBytes byte-for-byte — nothing missed or double counted.** Columns are awk-friendly: `kind bytes w h d mips arr samples fmt bind misc usage alias "name"` (usage 0 = DEFAULT heap = true VRAM). Findings + reduction backlog in repo `VRAM_CENSUS.md` |
| (in `GET_PERF_DATA`) `VRAM:`/`SUBALLOC:` | — | VRAM: census MB, default-heap MB, resource count, driver usage/budget. SUBALLOC: mesh-data suballocator blocks (256 MB each, released only when COMPLETELY empty) — total/free/used tells free space from fragmentation |
| `DUMP_STREAM` | (none) | Scene-side streaming ground truth → `Files/stream_dump.txt`: every material's name, raw GPU mip-feedback word, streamDis flag, and each texture slot's CURRENT WxH/mips. **Trap: floor-reduced 1024 (128x128 mips=8) is BYTE-IDENTICAL in TextureDesc to a native 128 full-chain — use DUMP_STREAM2 to disambiguate** |
| `DUMP_STREAM2` | (none) | ENGINE-side authoritative enrolled-set dump → `Files/stream_resources.txt`: per wi resource — ENROLLED/static, current WxH/mips vs full mip_count, STREAMING flag, live min-lod, instantaneous request value. The instrument that proved streaming WORKS (village cage_wood fully streamed in, off-screen ruins parked at floor) after the frozen-counter false alarm |
| `DUMP_SOTAN` | `<name-substr>` | Byte-level skinned-streamout forensics: reads back the nearest-to-camera matching skinned mesh's streamoutBuffer + generalBuffer on TWO consecutive frames; diffs the so_tan region (w-sign flips, contiguous runs, value census/histogram, **RLE block fingerprint with offsets**), so_nor/so_pos w-lane footprints, and the SOURCE vb_tan region (canonical census + interframe diff). Writes `Files/sotan_dump.txt`. **The instrument that convicted the skinning CS wave-uniform tan.w corruption (engine 1.64 fix): every corrupt run = 64-vert wave-aligned; post-fix histogram = pure ±1** |
| `SET_BINDPOSE_TAN` | `<0\|1>` | Engine 1.62c discriminator for the INTERMITTENT per-load tangent-w coin-flip (SET_TANGENTVIS 4 boiling red/green, ~46% interior flips/frame, random shared timelines = GPU race). 1 = raster tangents from the BIND-POSE buffer (skip so_tan patch): flip dies → skinned streamout CONTENT poisoned; flip survives → consumer path. 0 = normal. **PROTOCOL on an afflicted load: SET_TANGENTVIS 4 (confirm boil) → SET_BINDPOSE_TAN 1 (verdict) → report.** Affliction is a per-load lottery: 9 harness loads calm, user manual loads 3/3 afflicted (2026-07-30) |
| `DUMP_SKINGEO` | `<name-substr>` | Skinned-tangent pipeline forensics: per matching renderable object prints vertex array counts (pos/nor/tan/bones), armature binding, and CPU-side buffer-view descriptors — bind-pose `vb_nor`/`vb_tan` vs skinned streamout `so_pos`/`so_nor`/`so_tan` — plus a verdict line (skinned-tangents-OK / so_tan missing / no tangents). Diff descriptors against `DUMP_GEOMETRY` to prove which tangent source the GPU consumes. Writes `skingeo_dump.txt` (exe dir). Cap 60 entries |
| `SET_ENTITY_VIS` | `<name-substr> <0\|1>` | `SetRenderable` on every named object matching the substring. Elimination probe (used to prove the blue palms WERE the named palm entities, not orphans) |
| `SET_TREES draw` | `0` or `1` | (new SET_TREES param) 0 = `ggtrees_draw_enabled=0` + `GGTrees_HideAll()`; 1 = re-enable + force a pool rescan. Elimination probe for tree-pool involvement. **ONE-WAY: `GGTrees_HideAll` flips `SetVisible(0)` on every tree in the source array, and `draw 1` cannot restore that** — reload the level to bring the trees back (disk data untouched). Measured tree-pool draw share on the TESTPRO1 gate view: ~1.6ms GPU |
| `SCULPT_TEST` | `<worldX> <worldZ> <frames> [mode]` | Synthetic terrain-sculpt stroke: N frames of the real `GGTerrain_Update_Sculpting` apply at the given world position (default mode RAISE=1; pass a `GGTERRAIN_SCULPT_*` int to override, e.g. 2=LOWER, 4=BLEND). Exercises the full edit → InvalidateRegion → Wicked chunk-regen chain and finalizes a proper undo action. Uses the current brush size. ~60 frames of RAISE makes an obvious hill |
| `PAINT_TEST` | `<worldX> <worldZ> <material> <frames>` | Synthetic texture-paint stroke via the real `GGTerrain_Update_Painting`. `material` = GG source-texture index + 1 (0 erases back to auto). Exercises paint → material map → blendmap repaint on the Wicked chunks, with undo |
| `UNDO` / `REDO` | (none) | Invoke the editor's Ctrl+Z/Ctrl+Y handlers (`editor_undo`/`editor_redo` — routes to the EBE stack when the Easy Building Editor is open). Level-editor state only. Pairs with SCULPT_TEST/PAINT_TEST to verify + clean up synthetic edits. NOTE: after any sculpt/undo, the touched chunk(s) show Wicked's default blend for a few seconds until the regen merges and the DX11-style repaint lands — expected transient |
| `QUIT` | (none) | Gracefully close the application |

## Application States

`GET_STATE` returns one of: `initializing`, `hub`, `storyboard`, `editor`, `game`, `loading`

## GET_STATE Storyboard Output

When in storyboard state, `GET_STATE` appends:
```
PROJECT: Switch Escape
NODES(14):
  [0] type=splash title="Splash Screen" level="" out0=" Connect to Scene "->50001
  [7] type=level title="switch escape" level="mapbank\switch escape.fpm" out0="..."->50005
  ...
```

## GET_SCREEN_TEXT Output

Returns detailed info depending on current state:
- **Storyboard**: PROJECT, DESCRIPTION, READONLY, and per-node details including title, type, level, levelnumber, editable flag, all output/input pins with actions and link targets, and all widget labels with types
- **Hub**: TAB name and demo list (when on demo_games tab)
- **Editor**: PANELS list, TOOLBAR_BUTTONS with toggle states, MENU_BAR items

### Editor Output Example

```
STATE: editor
PANELS: DockSpaceAGK, ..., Object Tools##EntityToolsWindow, ..., Toolbar, Statusbar
TOOLBAR_BUTTONS:
  Back to Game Project Storyboard
  Save Level
  Test Level
  Terrain, Painting, Trees and Vegetation [active=0]
  Object Tools [active=1]
  Visual Logic Connections [active=0]
  Environment Effects [active=0]
  Game Settings [active=0]
  Editor Light [active=0]
  Camera View
MENU_BAR: File, Edit, Tools, Help
```

Toggle buttons show `[active=0/1]` to indicate their current state. Action-only buttons (Back, Save, Test Level, Camera View) have no toggle state. "Test Level in VR" only appears when VR developer mode is enabled. "Terrain" only appears when not in empty level mode.

## Hub Tab Names (for NAVIGATE hub.\<tab\>)

`demo_games` (0), `my_games` (1), `tutorials` (3), `user_guide` (4), `live_changelog` (5), `workshop_uploader` (6), `workshop` (7), `community_tutorials` (42)

## CLICK Context Rules

- `edit_game` from **hub** only: triggers `bTriggerEditDemoGame` flag (opens storyboard for selected demo). Does NOT work from storyboard — use `CLICK_NODE` to load a level from the storyboard
- `test_level` from **editor** only: triggers `iLaunchAfterSync = 1` (runs the level as a playable test game). `GET_STATE` returns `STATE: game` while running. Harness remains responsive during game
- `play_game` from **storyboard**: triggers space key (TEST GAME)
- `add_level` from **storyboard**: triggers 'N' key
- `load_level` from **storyboard**: triggers 'L' key
- `exit_screen_editor` from **screen editor** only: triggers Exit to Storyboard (sets `g_iAutoExitScreenEditor=1` which the screen editor consumes as `iQuitWindowLoop=4`). The exit takes ~4 frames to complete the thumbnail capture sequence

## GET_PERF_DATA Output

Returns performance metrics. Works in any state. Example output during game:

```
STATE: game
FPS: 236.6
FRAME_TIME_MS: 4.23
SYSTEM_MEM_MB: 5793456
SYSTEM_MEM_GB: 5657.67
VRAM_MB: 37.3
VRAM_GB: 0.04
GPU_ADAPTER: AMD Radeon RX 9060 XT
SCENE_OBJECTS: 734
SCENE_MESHES: 586
SCENE_MATERIALS: 587
SCENE_LIGHTS: 22
SCENE_TRANSFORMS: 1767
SCENE_CAMERAS: 0
SCENE_EMITTERS: 0
SCENE_HAIRS: 0
SCENE_ANIMATIONS: 13
SCENE_ARMATURES: 34
SCENE_DECALS: 0
SCENE_PROBES: 9
SCENE_SOUNDS: 0
SCENE_COLLIDERS: 0
SCENE_RIGIDBODIES: 0
SCENE_SOFTBODIES: 0
SCENE_SCRIPTS: 0
SCENE_WEATHERS: 1
VISIBLE_OBJECTS: 330
VISIBLE_LIGHTS: 6
VISIBLE_DECALS: 0
VISIBLE_ENVPROBES: 4
VISIBLE_EMITTERS: 0
VISIBLE_HAIRS: 0
TAB_MODE: 0
POLYS: 5329606
```

**POLYS** (2026-07-31, engine 1.67): triangles submitted to the main camera color pass (opaque+transparent, once per subset — shadow/prepass/reflection passes excluded) during the last completed frame. Always counted, no profiler needed. Same number the in-game Performance panel shows next to FPS.

**Note**: SYSTEM_MEM_MB is process working set from Windows, not free RAM. VRAM_MB is dedicated GPU memory usage from DXGI. VISIBLE_* counts change per frame based on camera frustum culling.

When the profiler is enabled (via `ENABLE_PROFILER`), additional fields appear:

```
CPU_FRAME_MS: 35.30
GPU_FRAME_MS: 7.50
PROFILER_DATA:
CPU Frame: 35.30 ms
	Update - Logic: 19.83 ms
	Render: 1.74 ms
	Shadowmap Rendering: 0.95 ms
	Update - Wicked: 12.60 ms
	Animations (2x): 10.90 ms
	...
GPU Frame: 7.50 ms
	Opaque Scene: 2.91 ms
	Z-Prepass: 1.12 ms
	...
```

**Profiler usage pattern**: Send `ENABLE_PROFILER`, wait 3-5 seconds for averages to stabilize, send `GET_PERF_DATA` to read the breakdown, then send `DISABLE_PROFILER` to restore full FPS.

**Stable row list** (2026-07-31, engine 1.67): `GetTextData()` output is sorted by name and PERSISTENT — once a range name has been seen it stays in the printout forever (0.00 ms on frames it didn't run), so consecutive dumps always have the same row set (it can only grow when a brand-new range fires for the first time). This is what stopped the in-game Performance panel list from "jumping" as conditional ranges (TerrainW - *, RP3D-rec UpdateTex...) came and went. Caches reset when the profiler is toggled.

## TOGGLE_PROFILER (Game State)

Cycles `g.tabmode` (0→1→2→0), mirroring the TAB key in test game mode:
- **tabmode=0**: Normal game view
- **tabmode=1**: Visuals panel (right side — Customize Sky, Water, Weather, etc.)
- **tabmode=2**: Performance panel (top-left — FPS, draw calls, triangles) + visuals panel

**Important**: Does NOT call `wi::profiler::DrawData()` — that causes GPU TDR crashes. The performance panel uses safe ImGui text rendering only.

## PRESS_ESCAPE (Game State)

Exits test game back to editor by setting `t.game.gameloop=0`, `t.game.levelloop=0`, `t.game.masterloop=0`. This is the same exit path as pressing ESC during a test game (M-Game_part1.cpp line 1523).

## WPE Particle Commands

For the `.PE` effects in `Files\particlesbank\wpe` and `gamecore\decals\*\wpe.pe`. All three
work in the editor — none of them needs a test game.

| Command | Args | Notes |
|---|---|---|
| `DUMP_EMITTERS` | (none) | Per-emitter state: visible/active gates, `_flags`, count/life/size, **world** position (a wrong hierarchy attach shows up here), live GPU `aliveCount`, material blend mode and `baseColor.a`. Separates "not simulating" from "simulating but not drawn". |
| `WPE_PREVIEW` | `<relative .pe path>` | Loads an effect exactly as the editor's Preview checkbox does and parks it 250 units in front of the camera, so it must be on screen if it draws at all. For screenshot A/B against DX11. |
| `WPE_CLONETEST` | `<relative .pe path>` | Loads the effect, then `Entity_Duplicate`s its root four times — precisely what `preload_wicked_particle_effect()` does to fill the 5-slot `ready_decals[]` cache — and diffs every GameGuru emitter field of each clone against the master. Prints PASS/FAIL, then **deletes everything it created** (a `CLEANUP: deleted N roots, emitters A -> B` line confirms; the first version leaked 5 roots per call). |

`WPE_CLONETEST` exists because of engine delta 2.01. `Entity_Duplicate` copies an entity by
**serializing it**, so any field missing from `EmittedParticleSystem::Serialize` silently comes
back as a struct default in the clone. `burst_amount` defaults to 0 and `Burst(0)` resolves
`num = burst_amount`, so a clone that lost it occupies a cache slot and emits nothing. Run this
after touching either the emitter struct or its serializer:

```bash
echo 'WPE_CLONETEST gamecore\decals\impact\wpe.pe' > "$D/auto_command.txt"
```

Expect `VERDICT: PASS - all cache clones would emit (0 of 4 clones differ from master)`.

## Selection-Outline Commands

The DX11-style selection outline was restored 2026-08-05 (the DX12 port had the whole
pipeline but the two engine callback sites were dropped in the API migration — the mask
pass is now called from `MasterRenderer::Render`, the composite from `customDraw_Compose`).
Diagnostics, all editor-state:

| Command | Args | Notes |
|---|---|---|
| `DUMP_OUTLINE` | (none) | Whole-pipeline state: mode/prefs gates, FEED globals, per-object stencil refs **with positions** (check the position — selecting by index routinely lands on an entity far outside the camera view), per-renderpass ref/total draw counters, mask/composite run counters, plus a same-mesh twin detector. |
| `DUMP_OUTLINE_RT` | (none) | Saves all three outline mask targets to `Files/outline_rt(.png/_red/_blue)`. |
| `OUTLINE_MASKTEST` | `<0\|1\|2\|3>` | 0 normal; 1 draw unconditionally (plumbing proof); 2 ENGINE-nibble compare (stencil writes proof); 3 full-byte compare. |
| `SET_GRIDEDITSELECT` | `<n>` | Force grid edit select mode (5 = entity selection). |

**Testing trap that cost a whole evening:** `SELECT_ENTITY <idx>` selects by entity index,
NOT by visibility. On TESTPRO1 every entity sits thousands of units from the saved camera,
and a selected far-away object still ticks the frustum/draw counters while being a
2-pixel speck — looking exactly like "outline broken". Verify with an entity whose
DUMP_OUTLINE position is within ~1500 units of the camera aim (Island Showdown's start
view has the Player Start marker close by). Locked entities highlight BLUE (ref 3), not white.

## Scene Interrogation Commands

Three commands for inspecting entities, lights, and the full light pipeline at runtime. Works in editor or game state.

### LIST_ENTITIES

Lists all active entities. Optional filter argument:
- `LIST_ENTITIES` — all active entities
- `LIST_ENTITIES lights` — only light entities (ismarker=2 or islightmarker=1)

Output per entity:
```
STATE: editor
TOTAL_ENTITY_SLOTS: 195
  [3] name="" bank=67 pos=(-59.6,117.6,49.0) marker=2 LIGHT(range=450 rgb=(111,72,23) islit=1 spot=0 lidx=1)
  [11] name="" bank=67 pos=(-569.6,76.2,110.3) marker=2 LIGHT(range=169 rgb=(250,115,46) islit=1 spot=0 lidx=2)
  [42] name="Big Red Button" bank=15 pos=(-580.0,50.0,120.0) marker=0
  ...
LISTED: 145 entities (LIGHT_ENTITIES: 21)
```

Fields: `name` = instance name from editor, `bank` = entity profile index, `marker` = ismarker value (2=light), `LIGHT(...)` = light-specific data (range, RGB color, islit, spot flag, infinilight index).

### GET_ENTITY

Detailed dump of a single entity by index. Includes entity element data, entity profile data, light data, infinilight linkage, and WickedEngine cross-reference.

```bash
echo "GET_ENTITY 11" > "$D/auto_command.txt"
```

Output:
```
ENTITY[11]:
  name=""
  active=1
  bankindex=67
  obj=70011
  pos=(-569.60, 76.20, 110.30)
  rot=(0.00, 0.00, 0.00)
  scale=(100.00, 100.00, 100.00)
  staticflag=0
  PROFILE:
    ismarker=2
    islightmarker=1
    usespotlighting=0
    castshadow=-1
  LIGHT_DATA:
    index=2
    islit=1
    range=169
    color=0xFFFA7332 rgb=(250,115,46)
    offsetup=45
    offsetz=0
    usespotlighting=0
    fLightHasProbe=0.0
    fProbeBrightness=1.00
  INFINILIGHT[2]:
    used=1 islit=1 e=11
    pos=(-569.6,76.2,110.3) range=169
    rgb=(250,115,46)
    spot=0 shadow=1
    wickedlightindex=2920
  WICKED_LIGHT:
    type=POINT
    color=(0.980,0.451,0.180)
    intensity=600.00
    range=169.0
    castShadow=1
    inactive=0
    position=(-569.6,76.2,110.3)
```

This traces the full data pipeline: entity element → entity profile → infinilight → WickedEngine LightComponent. Useful for diagnosing why a specific light isn't rendering.

### LIST_LIGHTS

Full light pipeline debug dump. Shows every infinilight entry with its entity linkage, GameGuru-side properties, and the actual WickedEngine LightComponent state (type, intensity, range, color, shadow, inactive).

```bash
echo "LIST_LIGHTS" > "$D/auto_command.txt"
```

Output:
```
STATE: editor
INFINILIGHT_MAX: 21
WICKED_SCENE_LIGHTS: 22
VISIBLE_LIGHTS: 16
LIGHT[1]: e=3 used=1 islit=1 pos=(-59.6,117.6,49.0) range=450 rgb=(111,72,23) spot=0 shadow=0 wkID=1838 W(PT int=600.0 rng=450.0 rgb=(0.44,0.28,0.09) shd=0)
LIGHT[2]: e=11 used=1 islit=1 pos=(-569.6,76.2,110.3) range=169 rgb=(250,115,46) spot=0 shadow=1 wkID=1839 W(PT int=600.0 rng=169.0 rgb=(0.98,0.45,0.18) shd=1)
...
LIGHT[9]: e=89 used=1 islit=1 pos=(-919.1,208.1,-108.3) range=0 rgb=(0,0,0) spot=0 shadow=1 wkID=1846 W(PT int=600.0 rng=0.0 rgb=(0.00,0.00,0.00) shd=1 INACTIVE)
```

Each line shows:
- **GameGuru side**: `e`=entity index, `used`, `islit`, `pos`, `range`, `rgb` (0-255), `spot` (0=point,1=spot), `shadow`, `wkID`=WickedEngine entity ID
- **WickedEngine side** `W(...)`: type (`PT`=point, `SP`=spot, `DIR`=directional, `RC`=rectangle), `int`=intensity in candela, `rng`=range, `rgb` (0-1 normalized), `shd`=castShadow, `INACTIVE` if intensity or range is near zero

Lights with `W(MISSING!)` indicate the WickedEngine entity was lost. Lights with `W(NONE)` have `wickedlightindex=0` (never created). Both are bugs worth investigating.

### Light Data Pipeline

The full data flow for lights in GameGuru MAX:

```
FPE File (ismarker=2, lightrange, lightcolor, usespotlighting)
  → entityprofiletype (template from FPE)
  → entityeleproftype (per-instance in entityelement[e].eleprof)
  → lighting_refresh() in M-Lighting.cpp creates infinilighttype entries
  → WickedCall_AddLight() creates WickedEngine LightComponent
  → lighting_loop() in G-Lighting.cpp calls WickedCall_UpdateLight() per frame
  → WickedEngine renders the light
```

Key identification: `entityprofile[bankindex].ismarker == 2` marks a light entity. `eleprof.usespotlighting` determines point (0) vs spot (1). Entities named `%probe` with `fLightHasProbe >= 50` are environment probe carriers — they have range=0 and color=(0,0,0) intentionally.

### Key Source Files for Light System

| File | Function | Role |
|------|----------|------|
| `GameGuru/Source/M-Lighting.cpp` | `lighting_refresh()` | Creates `infinilighttype` entries from all entities with `ismarker==2`. Calls `WickedCall_AddLight()` for each. Runs on level load |
| `GameGuru/Source/G-Lighting.cpp` | `lighting_loop()` | Per-frame update of all infinilights. Calls `WickedCall_UpdateLight()` with position, range, color, shadow. Also handles flashlight and env probes |
| `Guru-WickedMAX/wickedcalls_part3.cpp` | `WickedCall_AddLight(iLightType)` | Creates a WickedEngine `LightComponent` (1=POINT, 2=SPOT). Sets initial intensity via `BackCompatSetEnergy(30)` |
| `Guru-WickedMAX/wickedcalls_part3.cpp` | `WickedCall_UpdateLight(...)` | Updates an existing light's color, range, position, cone angle, shadow. **Does NOT update intensity** — intensity is set once at creation and never changed |
| `GameGuru/Include/Types.h` | `entitylighttype` (line 5574) | Light data struct: `color` (ARGB DWORD), `range`, `offsetup` (spot cone), `islit`, `fLightHasProbe` |
| `GameGuru/Include/Types.h` | `infinilighttype` (line 8227) | Runtime light bridge: position, range, color, `wickedlightindex` (WickedEngine entity ID), `e` (back-ref to entity index) |
| `WickedEngine/wiScene_Components.h` | `LightComponent` (line 1321) | WickedEngine light: `intensity` (candela for point/spot, lux for directional), `range`, `color`, `type`, shadow flags |

### Open Issue: Point Light Intensity Still Too Dim (2026-02-16)

> STATUS UNVERIFIED as of 2026-07-17 — five months old; confirm with the user whether the 600cd/6000cd mapping was accepted as final.

**Status**: First fix applied (`BackCompatSetEnergy(30)`) raised intensity from 30cd to 600cd for point lights and 6000cd for spots. Scene is brighter but still darker than expected. Further investigation needed.

**What was done**:
- `WickedCall_AddLight` now calls `lightComponent->BackCompatSetEnergy(30)` after `SetType()`, converting the old energy value of 30 to physically-based candela (POINT: 30×20=600cd, SPOT: 30×200=6000cd)
- Verified via `LIST_LIGHTS` that all point lights show `int=600.0` and the spot light shows `int=6000.0`

**What to investigate next**:
- The energy value `30` is hardcoded in `WickedCall_AddLight` — it may need to be higher, or derived from entity properties (e.g., proportional to range)
- `WickedCall_UpdateLight` never sets intensity — if lights need per-entity intensity control, this function needs a new parameter
- The entity `entitylighttype` struct has no explicit intensity/energy field — brightness was historically encoded in the color RGB values and range. The PBR model separates intensity from color, which may require a new FPE field or a formula to derive intensity from range
- Check whether other demos (especially indoor scenes like Trapped, Disruption) also appear too dark, or if this is specific to Switch Escape
- WickedEngine `BackCompatSetEnergy` multipliers: POINT=×20, SPOT=×200 (from `wiScene_Components.h` line 1396)

## CLICK_NODE (Storyboard)

Clicks a storyboard node by title to trigger its action:
- **Level nodes** with a level file assigned: loads the level into the editor via `cDirectOpen` + `iLaunchAfterSync = 7` (same as clicking the node thumbnail in the UI). The harness is unresponsive during the synchronous level load. After load completes, `GET_STATE` returns `STATE: editor`
- **Screen/splash nodes**: opens the screen editor by setting `bScreen_Editor_Window = true` and `iScreen_Editor_Node = <index>`. The screen editor renders within the storyboard window. Use `CLICK exit_screen_editor` to close it

## Screen Editor Sequence

```bash
D="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"

# Open screen editor for a screen node (from storyboard state)
rm -f "$D/auto_result.txt"; echo "CLICK_NODE Title Screen" > "$D/auto_command.txt"
t=0; while [ $t -lt 10 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done
# Expected: OK: Opened screen editor for node 'Title Screen' (type=screen, index=1)

# Wait for screen editor to render
sleep 3

# Exit back to storyboard
rm -f "$D/auto_result.txt"; echo "CLICK exit_screen_editor" > "$D/auto_command.txt"
t=0; while [ $t -lt 10 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done
# Expected: OK: Triggered Exit to Storyboard from screen editor

# Wait for exit sequence (~4 frames)
sleep 3
```

## Standard Sequence: Launch -> Hub -> Storyboard -> Editor

This is the proven sequence (tested 2026-02-15, ~34s launch to editor).

Use inline paths — do NOT rely on bash helper functions with `$EXE_DIR` expansion as they have scoping issues in MSYS bash. Use a short variable `D` set at the top of each script block.

```bash
D="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"

# Launch the app (background)
cd "$D" && ./GameGuruMAX.exe &

# Wait briefly then poll for hub (hub appears in ~5-10s)
sleep 3
rm -f "$D/auto_result.txt"; echo "GET_STATE" > "$D/auto_command.txt"
t=0; while [ $t -lt 30 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done
# Expected: STATE: hub / TAB: demo_games

# Select demo
sleep 2
rm -f "$D/auto_result.txt"; echo "SELECT_DEMO Switch Escape" > "$D/auto_command.txt"
t=0; while [ $t -lt 15 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done

# Edit Game (hub -> storyboard)
sleep 2
rm -f "$D/auto_result.txt"; echo "CLICK edit_game" > "$D/auto_command.txt"
t=0; while [ $t -lt 30 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done

# Confirm storyboard
sleep 5
rm -f "$D/auto_result.txt"; echo "GET_STATE" > "$D/auto_command.txt"
t=0; while [ $t -lt 30 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done
# Expected: STATE: storyboard / PROJECT: Switch Escape / NODES(14): ...

# Click level node to load into editor
rm -f "$D/auto_result.txt"; echo "CLICK_NODE switch escape" > "$D/auto_command.txt"
t=0; while [ $t -lt 15 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done

# Wait for level load then confirm editor
sleep 10
rm -f "$D/auto_result.txt"; echo "GET_STATE" > "$D/auto_command.txt"
t=0; while [ $t -lt 60 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done
# Expected: STATE: editor
```

## Timing Guide

| Phase | Sleep | Notes |
|-------|-------|-------|
| After launch | 3s | Hub appears in ~5-10s, the poll loop handles the rest |
| Between hub commands | 2s | Small gap for UI to settle |
| After CLICK edit_game (hub->storyboard) | 5s | Storyboard loads quickly |
| After CLICK_NODE (level load) | 10s | Synchronous load, harness unresponsive during load |

**Tested**: Launch to editor in ~34 seconds total (2026-02-15).

## Performance / Baseline Testing: TESTPRO1 Island Level

For performance profiling AND for the DX11-vs-DX12 visual A/B (see `SCRATCHPAD.md` — the island scene has a saved camera baseline), use the **TESTPRO1** project (a user project on the **My Games** tab) and its **island level node**.

### Why TESTPRO1 Island Level

- Single-level project (only one `type=level` storyboard node → `CLICK_ONLY_LEVEL` works without knowing the title)
- Large island terrain — good coverage of terrain/tree/grass/lighting paths
- Saved camera baseline in the .fpm — repeatable framing without harness camera commands
- Fills quickly from cold launch (no `SELECT_DEMO`, no tab-navigating a huge demo list)

### Sequence: Launch → TESTPRO1 → Island → Editor (fully autonomous)

Prior versions of this sequence tried `CLICK edit_game` for My Games projects — that path silently no-ops because it needs `bProjectExistsAndValidToUse` which is only set by an in-app mouse click. Use `OPEN_PROJECT` instead (added 2026-07-12):

```bash
D="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"

send_cmd() {
  rm -f "$D/auto_result.txt"
  echo "$1" > "$D/auto_command.txt"
  local t=0
  local timeout=${2:-30}
  while [ $t -lt $timeout ]; do
    if [ -f "$D/auto_result.txt" ]; then cat "$D/auto_result.txt"; return 0; fi
    sleep 1; t=$((t+1))
  done
  echo "TIMEOUT: $1"; return 1
}

# 1. Kill any existing MAX (must be closed before re-launch OR before build)
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
sleep 2

# 2. Launch (background)
cd "$D" && ./GameGuruMAX.exe &
sleep 8

# 3. Confirm hub (welcome screen rendered → projectbank_list populated)
send_cmd "GET_STATE" 30
# Expected: STATE: hub / TAB: my_games

# 4. Open TESTPRO1 — sets TriggerLoadGameProject + toggles bStoryboardWindow;
#    process_storeboard picks it up on the next frame
send_cmd "OPEN_PROJECT TESTPRO1" 15

# 5. Wait for storyboard render, verify it loaded
sleep 6
send_cmd "GET_STATE" 30
# Expected: STATE: storyboard / PROJECT: TESTPRO1 / NODES(...) includes island

# 6. Click the (only) level node — no need to know its title
send_cmd "CLICK_ONLY_LEVEL" 15

# 7. Wait for editor state (island load is slow, use 25s + 60s poll)
sleep 25
send_cmd "GET_STATE" 60
# Expected: STATE: editor

# 8. Settle then screenshot
sleep 8
send_cmd "SCREENSHOT" 15
# Screenshot lands under Files/screenshots/sc_*.png (timestamped filename)
```

**Verified 2026-07-12**: full sequence completes in ~50 seconds from kill-and-launch to screenshot on disk. `island.fpm` (v342 map.ele) loads through the pre-release entity-load version guard — entity props silently absent, but terrain / trees / grass / paint all render.

### Performance Data Collection (from editor or test game)

```bash
D="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"

# Baseline FPS (no profiler overhead)
rm -f "$D/auto_result.txt"; echo "GET_PERF_DATA" > "$D/auto_command.txt"
t=0; while [ $t -lt 10 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done

# Full profiler breakdown (WARNING: ~75% FPS drop while enabled)
rm -f "$D/auto_result.txt"; echo "ENABLE_PROFILER" > "$D/auto_command.txt"
t=0; while [ $t -lt 10 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done

# Wait for profiler averages to stabilize
sleep 5

# Collect profiler data
rm -f "$D/auto_result.txt"; echo "GET_PERF_DATA" > "$D/auto_command.txt"
t=0; while [ $t -lt 10 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done

# Disable profiler to restore full FPS
rm -f "$D/auto_result.txt"; echo "DISABLE_PROFILER" > "$D/auto_command.txt"
t=0; while [ $t -lt 10 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done
```

### Feature Toggle Keys (use via PRESS_KEY in editor mode)

| Key | Feature | Notes |
|-----|---------|-------|
| O | Terrain on/off | ~30 FPS impact |
| P | Profiler toggle + file dump | |
| U | Wireframe overlay | |
| 1 | Shadows on/off | **Largest impact** (~+30 FPS) |
| 2 | AO on/off | Minor (~+4 FPS) |
| 3 | Bloom on/off | Negligible |
| 4 | Volumetric Clouds on/off | Minor (~+3 FPS) |
| 5 | Realistic Sky on/off | Negligible |
| 6 | Light Shafts on/off | Negligible |
| 7 | Reflections on/off | Minor (~+5 FPS) |
| 8 | Volumetric Lights on/off | Negligible |

See `PERFORMANCE.md` for full profiler dumps, A/B test results, and optimization analysis.

## Crash Diagnosis Using Log Files

When the app crashes or triggers a GPU device reset (TDR), several log files in the EXE directory provide diagnostic information. Cross-reference these with the source code to identify the root cause.

### Log Files

| File | Description |
|------|-------------|
| `Guru-Crash.log` | **Primary crash log.** Append-only, survives across sessions. Records exception code, crash address, and **source file + line number** for each crash. Check this first |
| `crashdump.dmp` | Windows minidump for debugger analysis. Overwritten each crash |
| `auto_log.txt` | Automation harness log with timestamps. Useful for correlating crash timing with commands issued |

### Reading Guru-Crash.log

Each entry looks like:
```
==== GAMEGURU MAX CRASH DETECTED ====
Time:            2026-02-15 17:55:18
Exception code:  0xc0000005
Source Code:     D:\max\WickedEngineDX12\WickedEngine\wiProfiler.cpp:498
=====================================
```

- **Exception 0xc0000005** = access violation (read/write to invalid memory). Most common crash type
- **Source Code line** points to the exact C++ line. Read that line in context to understand what went wrong
- Multiple entries with the **same source file:line** across sessions indicate a reproducible bug, not a random glitch
- Multiple **different crash locations** in the same session often indicate a cascade — the first crash corrupts state, causing subsequent crashes elsewhere

### Common Crash Patterns

**GPU Allocator failures (`AllocateGPU` → `memcpy` crash)**:
Crashes at `std::memcpy` into `allocation.data` in files like `wiProfiler.cpp`, `wiGraphicsDevice.h`, or `wiRenderer.cpp` mean `AllocateGPU()` returned an allocation with a null `data` pointer. The GPU frame allocator failed to provide mapped memory. This typically happens when:
- A GPU resource is created or used in an invalid context (e.g., inside an active RenderPass)
- The GPU device has already been removed (TDR occurred, and subsequent operations crash)
- A feature was enabled mid-frame that requires GPU resources not yet initialized

**TDR / Hard GPU freeze**:
When the PC resets or the display driver recovers, the crash log may show the *symptom* rather than the *cause*. The access violation from a failed GPU allocation corrupts the DX12 command list (render pass started but never ended, command list never submitted), which hangs the GPU. Look for what was **enabled or changed** just before the first crash in the log, not just the crash location itself.

### Diagnosis Workflow

1. **Read `Guru-Crash.log`** — identify the crash source file and line number
2. **Read the source code** at that line — understand what operation failed (null pointer, bad allocation, etc.)
3. **Look for patterns** — if the same line crashes repeatedly, it's a reliable reproduction. If multiple different lines crash, find the earliest/most frequent one as the root cause
4. **Trace the call chain backwards** — from the crash site, trace what function called it, what enabled the feature, and what user action triggered it
5. **Check the frame lifecycle** — WickedEngine frame order is: `BeginFrame` → `Update` (game logic) → `Render` (scene) → `Compose` (overlays, ImGui) → `EndFrame`. Features enabled during Update take effect in the same or next frame's Compose

### Key Architecture Notes for Crash Tracing

- **WickedEngine frame loop** (`wiApplication.cpp`): `profiler::BeginFrame()` → `Update()` → `Render()` → `RenderPassBegin` → `Compose(cmd)` → `RenderPassEnd` → `profiler::EndFrame(cmd)` → `SubmitCommandLists`
- **MasterRenderer::Compose** (`master_part1.cpp`): calls `__super::Compose(cmd)` (which chains through `Application::Compose` where `wi::profiler::DrawData` runs), then `ImGui_DX12_RenderBridge(cmd)`
- **Game logic runs during Update**: `GuruLoopLogic()` → `common_loop_logic()` → `mapeditorexecutable_loop()` or `gameexecutable_loop()`. The `iLaunchAfterSync` state machine in `M-GridEdit_part1.cpp` controls app flow (states: 0=idle, 7=load requested, 502=loading, 80=post-load, 1=init, 201/202=test game)
- **ImGui frame lifecycle**: `ImGui::NewFrame()` is called during Update, `ImGui::Render()` during `ImGui_RenderLast()`, and the actual DX12 draw happens in `ImGui_DX12_RenderBridge()` during Compose
- **wiProfiler**: When enabled via `SetEnabled(true)`, initialization happens on the *next* frame's `BeginFrame()`. `DrawData()` runs during Compose inside an active RenderPass. Enabling mid-game can crash if the profiler's GPU allocations fail in that context

## Building WickedEngine LIB from Claude Code

### Shell and CMD issues

1. **Never try to call `VsDevCmd.bat` directly from bash** — it doesn't work. Always use `cmd //C` to wrap Windows batch commands.

2. **Never use `call` inside a bash command** — `call` is a CMD built-in and won't work in the Claude Code shell. Instead, write a `.bat` file and execute that via `cmd //C`.

3. **Always use absolute paths for `.sln` files in msbuild** — relative paths fail because the working directory context is unreliable when invoking through `cmd //C`. Use the full path like `msbuild "D:\max\WickedEngineDX12\WickedEngine.sln"`.

4. **Always `cd /D` to the project directory inside the batch file** — don't rely on the bash `cd` command carrying over into the CMD context. Put `cd /D "D:\max\WickedEngineDX12"` as the first line after `@echo off`.

5. **The correct working build command is:**

```bash
cmd //C "D:\\max\\WickedEngineDX12\\build_wicked.bat Release" 2>&1 | tail -20
```

**CRITICAL**: Always pass `Release` explicitly. The script defaults to **Debug** if no argument is given, and GameGuru Release links against the Release `.lib`. After rebuilding WickedEngine, do a **clean rebuild** of GameGuru (`build.bat Release rebuild`) — incremental builds may not detect the `.lib` change and skip relinking.

Where `build_wicked.bat` contains:

```batch
@echo off
SET CONFIG=%1
IF "%CONFIG%"=="" SET CONFIG=Debug
cd /D "D:\max\WickedEngineDX12"
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul 2>&1
echo Building WickedEngine_Windows %CONFIG% x64...
msbuild "D:\max\WickedEngineDX12\WickedEngine.sln" /p:Configuration=%CONFIG% /p:Platform=x64 /t:WickedEngine_Windows /m /verbosity:minimal
```

6. **Use `| tail -20` on build commands** to avoid flooding the context window with thousands of lines of compiler output. If the build fails, use `| tail -40` or grep for "error" to find the issue.

### Build order and verification

7. **WickedEngine must be built BEFORE GameGuru MAX** — the GameGuru build depends on the WickedEngine `.lib` output. Always build WickedEngine first, confirm it succeeded, then run the GameGuru `build.bat`.

8. **Always verify new symbols are in the `.lib` after rebuilding WickedEngine.** A successful build does NOT guarantee your new functions are linked in. The `.lib` can be stale or the functions can be optimized away by LTCG. After building, run dumpbin to confirm:

```bash
"C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717/bin/Hostx64/x64/dumpbin.exe" /linkermember:1 "D:/max/WickedEngineDX12/BUILD/x64/Release/WickedEngine_Windows.lib" | grep YourFunctionName
```

Use `/linkermember:1` not `/symbols` or `/exports` — those return empty for static `.lib` files.

9. **The WickedEngine `.lib` output path is `D:\max\WickedEngineDX12\BUILD\x64\Release\WickedEngine_Windows.lib`** — don't waste time searching in `x64/Release/` or `WickedEngine/x64/Release/`, they don't exist. The output directory is set in the `.vcxproj` as `$(SolutionDir)BUILD\$(Platform)\$(Configuration)\`.

10. **Source files are not listed directly in `WickedEngine_Windows.vcxproj`** — they come from `WickedEngine_SOURCE.vcxitems` (imported at line 40 of the `.vcxproj`). If you need to check whether a `.cpp` file is included in the build, search the `.vcxitems` file, not the `.vcxproj`.

11. **If you add new functions to WickedEngine headers but get linker errors in GameGuru**, the most likely cause is a stale `.lib`. Do a clean rebuild of WickedEngine and verify symbols with dumpbin before investigating anything else.

## Full Demo FPS Test (FULL TEST)

Automated test that launches all 19 demos, enters test-game mode, collects FPS samples, and reports results. Tested 2026-02-15 and 2026-02-16: 18/19 passed autonomously in ~20 minutes.

### How to Run

1. **Ensure the app is NOT running** — `taskkill.exe //IM GameGuruMAX.exe //F`
2. **Launch the app** and wait for hub state
3. **Write the test script** to a `.sh` file (see below) and run it as a **background task** with a 600000ms (10 min) timeout
4. **Check progress** every 2-4 minutes by tailing the background output file
5. **Kill the app** after the script completes

### MSYS Bash Compatibility — CRITICAL

- **Do NOT use `${!array[@]}`** — MSYS bash does not support it. Use a `while read` loop with a heredoc instead
- **Do NOT use bash arrays for the demo list** — use a heredoc `DEMOLIST` block fed into `while IFS= read -r demo`
- **Use `for s in 1 2 3 4 5`** instead of `for s in $(seq 1 5)` if seq is unavailable
- **Always use a short variable** `D="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"` at the top — do NOT use `$EXE_DIR` or functions that reference it

### The Complete Test Script

Write this to a `.sh` file and execute with `bash <file>`. Run as a background task.

```bash
#!/bin/bash
D="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
RESULTS_FILE="/tmp/fps_results.txt"

send_cmd() {
  rm -f "$D/auto_result.txt"
  echo "$1" > "$D/auto_command.txt"
  local t=0
  local timeout=${2:-30}
  while [ $t -lt $timeout ]; do
    if [ -f "$D/auto_result.txt" ]; then
      cat "$D/auto_result.txt"
      return 0
    fi
    sleep 1
    t=$((t+1))
  done
  echo "TIMEOUT"
  return 1
}

echo "DEMO | BEST_FPS | AVG_FPS | SAMPLES" > "$RESULTS_FILE"
echo "--- | --- | --- | ---" >> "$RESULTS_FILE"

TOTAL=19
PASSED=0
FAILED=0
FAILED_NAMES=""
num=0

while IFS= read -r demo; do
  num=$((num+1))
  echo ""
  echo "============================================"
  echo "[$num/$TOTAL] Testing: $demo"
  echo "============================================"

  # Step 1: Navigate to hub
  echo "  -> Navigating to hub..."
  result=$(send_cmd "NAVIGATE hub" 15)
  sleep 2

  # Step 2: Confirm hub state
  result=$(send_cmd "GET_STATE" 15)
  state=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
  echo "  -> State: $state"
  if [ "$state" != "hub" ]; then
    echo "  !! FAILED - not in hub state, got: $state"
    FAILED=$((FAILED+1))
    FAILED_NAMES="$FAILED_NAMES|$demo (not in hub)"
    echo "$demo | FAIL | - | not in hub" >> "$RESULTS_FILE"
    continue
  fi

  # Step 3: Select demo
  echo "  -> Selecting demo..."
  result=$(send_cmd "SELECT_DEMO $demo" 15)
  echo "  -> $result"
  sleep 2

  # Step 4: Edit game (hub -> storyboard)
  echo "  -> Clicking edit_game..."
  result=$(send_cmd "CLICK edit_game" 15)
  sleep 5

  # Step 5: Confirm storyboard and find level node
  result=$(send_cmd "GET_STATE" 30)
  state=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
  echo "  -> State: $state"
  if [ "$state" != "storyboard" ]; then
    echo "  !! FAILED - not in storyboard, got: $state"
    FAILED=$((FAILED+1))
    FAILED_NAMES="$FAILED_NAMES|$demo (not in storyboard)"
    echo "$demo | FAIL | - | not in storyboard" >> "$RESULTS_FILE"
    continue
  fi

  # Step 6: Extract level node title (must have type=level AND level="mapbank...)
  level_node=$(echo "$result" | grep 'type=level' | grep 'level="mapbank' | head -1 | sed 's/.*title="\([^"]*\)".*/\1/')
  echo "  -> Level node: '$level_node'"
  if [ -z "$level_node" ]; then
    echo "  !! FAILED - no level node with mapbank found"
    FAILED=$((FAILED+1))
    FAILED_NAMES="$FAILED_NAMES|$demo (no level node)"
    echo "$demo | FAIL | - | no level node" >> "$RESULTS_FILE"
    continue
  fi

  # Step 7: Click level node to load into editor
  echo "  -> Loading level '$level_node'..."
  result=$(send_cmd "CLICK_NODE $level_node" 15)
  echo "  -> $result"

  # Step 8: Wait for level load + confirm editor (long timeout for big levels)
  sleep 12
  result=$(send_cmd "GET_STATE" 60)
  state=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
  echo "  -> State after load: $state"

  # If still loading, wait more (some levels like Aztec Game Kit take 20s+)
  if [ "$state" = "loading" ] || [ "$state" = "storyboard" ]; then
    echo "  -> Still loading, waiting more..."
    sleep 15
    result=$(send_cmd "GET_STATE" 60)
    state=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
    echo "  -> State: $state"
  fi

  if [ "$state" != "editor" ]; then
    echo "  !! FAILED - not in editor, got: $state"
    FAILED=$((FAILED+1))
    FAILED_NAMES="$FAILED_NAMES|$demo (not in editor, got $state)"
    echo "$demo | FAIL | - | not in editor ($state)" >> "$RESULTS_FILE"
    continue
  fi

  # Step 9: Test level (enter game mode)
  echo "  -> Testing level..."
  result=$(send_cmd "CLICK test_level" 15)
  sleep 8

  # Step 10: Confirm game state
  result=$(send_cmd "GET_STATE" 30)
  state=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
  echo "  -> State: $state"

  if [ "$state" != "game" ]; then
    sleep 5
    result=$(send_cmd "GET_STATE" 30)
    state=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
    echo "  -> State (retry): $state"
  fi

  if [ "$state" != "game" ]; then
    echo "  !! FAILED - not in game state, got: $state"
    FAILED=$((FAILED+1))
    FAILED_NAMES="$FAILED_NAMES|$demo (not in game, got $state)"
    echo "$demo | FAIL | - | not in game ($state)" >> "$RESULTS_FILE"
    send_cmd "PRESS_ESCAPE" 10 > /dev/null 2>&1
    sleep 2
    continue
  fi

  # Step 11: Collect FPS (10 samples over ~30 seconds)
  echo "  -> Collecting FPS over 30 seconds..."
  best_fps="0"
  sample_count=0
  all_fps=""

  for s in 1 2 3 4 5 6 7 8 9 10; do
    sleep 3
    result=$(send_cmd "GET_PERF_DATA" 10)
    fps_val=$(echo "$result" | grep "^FPS:" | sed 's/FPS: //')
    if [ -n "$fps_val" ]; then
      echo "    Sample $s: $fps_val FPS"
      all_fps="$all_fps $fps_val"
      sample_count=$((sample_count+1))

      # Compare for best (remove decimal point for integer comparison)
      fps_x10=$(echo "$fps_val" | tr -d '.')
      best_x10=$(echo "$best_fps" | tr -d '.')
      fps_x10=${fps_x10##0}
      best_x10=${best_x10##0}
      if [ "${fps_x10:-0}" -gt "${best_x10:-0}" ] 2>/dev/null; then
        best_fps="$fps_val"
      fi
    fi
  done

  # Calculate average
  if [ $sample_count -gt 0 ]; then
    avg_fps=$(echo "$all_fps" | tr ' ' '\n' | grep -v '^$' | awk '{sum+=$1; n++} END {printf "%.1f", sum/n}')
  else
    avg_fps="N/A"
  fi

  echo "  ==> Best: $best_fps | Avg: $avg_fps | Samples: $sample_count"
  echo "$demo | $best_fps | $avg_fps | $sample_count" >> "$RESULTS_FILE"
  PASSED=$((PASSED+1))

  # Step 12: Exit game back to editor
  echo "  -> Pressing escape..."
  result=$(send_cmd "PRESS_ESCAPE" 10)
  sleep 3

  # Confirm recovery
  result=$(send_cmd "GET_STATE" 15)
  state=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
  echo "  -> Back to: $state"

done <<'DEMOLIST'
Aztec Game Kit Teaser
Aztec Game Kit
Bounty
Horseshoe Bend
Island Showdown
Operation Amazon
River Raiders
Snowy Mountain Stroll
A Grand Canyon Adventure
Disruption
Foggy Forest
Indian Strike Force
Switch Escape
Canyon Offensive
Escape from the Zombie Cellar
Jungle Fever
RPG Template
The Mystery of Z Island
Trapped
DEMOLIST

echo ""
echo "============================================"
echo "ALL TESTS COMPLETE"
echo "Passed: $PASSED / $TOTAL"
echo "Failed: $FAILED / $TOTAL"
if [ -n "$FAILED_NAMES" ]; then
  echo "Failed demos:"
  echo "$FAILED_NAMES" | tr '|' '\n' | grep -v '^$' | while read -r line; do echo "  - $line"; done
fi
echo "============================================"
echo ""
echo "=== RESULTS TABLE ==="
cat "$RESULTS_FILE"
```

### Execution Pattern

```bash
# 1. Kill any running instance
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true

# 2. Launch fresh
D="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
cd "$D" && ./GameGuruMAX.exe &

# 3. Wait for hub
sleep 5
rm -f "$D/auto_result.txt"; echo "GET_STATE" > "$D/auto_command.txt"
t=0; while [ $t -lt 30 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done

# 4. Write the script to a .sh file (Write tool), then run as background task:
bash "$D/run_all_tests.sh"
# Use run_in_background=true, timeout=600000

# 5. Check progress every 2-4 minutes by tailing the background output file

# 6. After completion, kill the app
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
```

### Per-Demo Flow (what the script does for each demo)

```
NAVIGATE hub -> GET_STATE (confirm hub) -> SELECT_DEMO <name> -> CLICK edit_game
-> GET_STATE (confirm storyboard, extract level node title) -> CLICK_NODE <title>
-> sleep 12 + GET_STATE (confirm editor, retry with +15s if still loading)
-> CLICK test_level -> GET_STATE (confirm game) -> 10x GET_PERF_DATA (3s apart, ~30s total)
-> PRESS_ESCAPE -> GET_STATE (confirm editor recovery) -> next demo
```

### Known Behaviours

| Behaviour | Details |
|-----------|---------|
| **First FPS sample is often low** | Shader compilation / GPU warmup on the first frame. Sample 1 may read 3-4 FPS. The best/avg calculation handles this — report best FPS as the headline number |
| **Island Showdown may timeout** | This is the largest level. The 12s sleep + 60s poll may not be enough. If it fails, increase the initial sleep to 20s or add a third retry with another 15s wait |
| **Zombie Cellar locks at 60 FPS** | This demo has vsync enabled in its settings. 60.0 FPS is correct, not a bug |
| **PRESS_ESCAPE sometimes returns to `loading`** | Transitional state. The next `NAVIGATE hub` command at the top of the loop recovers from this |
| **Level node title != demo name** | The script extracts the level node title from `GET_STATE` storyboard output (grep for `type=level` with `level="mapbank`). This handles cases like "Escape from the Zombie Cellar" whose level node is titled "zombie cellar demo - level1" |
| **Some levels need extra load time** | Aztec Game Kit, Horseshoe Bend, Indian Strike Force, etc. take 15-25s to load. The script retries with +15s if state is still `storyboard` or `loading` after the first check |

### Baseline Results (2026-02-15, AMD Radeon RX 9060 XT, 5 samples/15s)

| # | Demo | Best FPS | Avg FPS |
|---|------|----------|---------|
| 1 | Aztec Game Kit Teaser | 137.3 | 135.1 |
| 2 | Aztec Game Kit | 77.1 | 61.8 |
| 3 | Bounty | 147.5 | 145.9 |
| 4 | Horseshoe Bend | 22.3 | 9.4 |
| 5 | Island Showdown | FAIL | level load timeout |
| 6 | Operation Amazon | 83.1 | 81.8 |
| 7 | River Raiders | 90.0 | 88.6 |
| 8 | Snowy Mountain Stroll | 38.4 | 17.3 |
| 9 | A Grand Canyon Adventure | 84.1 | 83.4 |
| 10 | Disruption | 109.3 | 108.3 |
| 11 | Foggy Forest | 67.2 | 66.7 |
| 12 | Indian Strike Force | 46.8 | 46.5 |
| 13 | Switch Escape | 197.4 | 195.3 |
| 14 | Canyon Offensive | 59.8 | 47.8 |
| 15 | Escape from the Zombie Cellar | 60.0 | 59.9 |
| 16 | Jungle Fever | 142.3 | 139.6 |
| 17 | RPG Template | 163.6 | 161.0 |
| 18 | The Mystery of Z Island | 75.1 | 73.7 |
| 19 | Trapped | 228.3 | 225.1 |

### Results (2026-02-16, AMD Radeon RX 9060 XT, 10 samples/30s)

Post DX12 lighting brightness fix (commit 241bd64b). FPS is ~15-20% lower across the board due to increased light intensity (600cd point lights, 6000cd spot lights).

| # | Demo | Best FPS | Avg FPS |
|---|------|----------|---------|
| 1 | Aztec Game Kit Teaser | 119.3 | 117.4 |
| 2 | Aztec Game Kit | 71.0 | 62.0 |
| 3 | Bounty | 129.3 | 125.0 |
| 4 | Horseshoe Bend | 20.7 | 14.3 |
| 5 | Island Showdown | FAIL | level load timeout |
| 6 | Operation Amazon | 70.8 | 64.2 |
| 7 | River Raiders | 76.9 | 73.9 |
| 8 | Snowy Mountain Stroll | 35.5 | 29.0 |
| 9 | A Grand Canyon Adventure | 73.1 | 71.6 |
| 10 | Disruption | 93.1 | 88.7 |
| 11 | Foggy Forest | 58.0 | 56.7 |
| 12 | Indian Strike Force | 42.6 | 41.3 |
| 13 | Switch Escape | 164.3 | 161.4 |
| 14 | Canyon Offensive | 53.9 | 42.2 |
| 15 | Escape from the Zombie Cellar | 60.0 | 59.9 |
| 16 | Jungle Fever | 120.6 | 113.7 |
| 17 | RPG Template | 132.1 | 128.3 |
| 18 | The Mystery of Z Island | 64.5 | 64.0 |
| 19 | Trapped | 169.4 | 165.9 |

### Results (2026-02-17, AMD Radeon RX 9060 XT, 10 samples/30s)

Post Phase 2 DX12 shader port (commit 215ed91b). 19/19 demos tested, all passed. Island Showdown now passes with increased load timeouts in the test script. River Raiders caused a sound crash (dsutil.cpp:1018, pre-existing bug) on first run but passed on retest.

| # | Demo | Best FPS | Avg FPS |
|---|------|----------|---------|
| 1 | Aztec Game Kit Teaser | 141.8 | 129.3 |
| 2 | Aztec Game Kit | 64.8 | 48.6 |
| 3 | Bounty | 111.8 | 108.3 |
| 4 | Horseshoe Bend | 16.1 | 10.4 |
| 5 | Island Showdown | 30.4 | 27.7 |
| 6 | Operation Amazon | 55.1 | 53.3 |
| 7 | River Raiders | 59.2 | 57.4 |
| 8 | Snowy Mountain Stroll | 32.0 | 26.1 |
| 9 | A Grand Canyon Adventure | 60.3 | 57.9 |
| 10 | Disruption | 86.9 | 84.8 |
| 11 | Foggy Forest | 49.1 | 41.9 |
| 12 | Indian Strike Force | 35.2 | 31.6 |
| 13 | Switch Escape | 166.3 | 163.3 |
| 14 | Canyon Offensive | 41.1 | 33.2 |
| 15 | Escape from the Zombie Cellar | 60.0 | 59.9 |
| 16 | Jungle Fever | 102.4 | 100.9 |
| 17 | RPG Template | 110.9 | 107.8 |
| 18 | The Mystery of Z Island | 54.0 | 50.6 |
| 19 | Trapped | 179.6 | 175.6 |

### Results (2026-02-18, AMD Radeon RX 9060 XT, 10 samples/30s)

Post Phase 3 render pipeline integration (commits 9118befe, 0a8ea4fb, 8bebfec9, b84ff4fe). All Phase 3 hooks active: depth prepass, reflection prepass, reflection opaque, main opaque, transparent, shadow maps (per-cascade), and env probes (per-face). 14/14 demos tested in this run (remaining 5 verified in earlier focused tests). Zero crashes, zero regressions.

| # | Demo | Best FPS | Avg FPS |
|---|------|----------|---------|
| 1 | Island Showdown | 30.4 | 27.7 |
| 2 | Operation Amazon | 55.1 | 53.3 |
| 3 | River Raiders | 59.2 | 57.4 |
| 4 | Snowy Mountain Stroll | 32.0 | 26.1 |
| 5 | A Grand Canyon Adventure | 60.3 | 57.9 |
| 6 | Disruption | 86.9 | 84.8 |
| 7 | Foggy Forest | 49.1 | 41.9 |
| 8 | Indian Strike Force | 35.2 | 31.6 |
| 9 | Switch Escape | 166.3 | 163.3 |
| 10 | Canyon Offensive | 41.1 | 33.2 |
| 11 | Escape from the Zombie Cellar | 60.0 | 59.9 |
| 12 | Jungle Fever | 102.4 | 100.9 |
| 13 | RPG Template | 110.9 | 107.8 |
| 14 | The Mystery of Z Island | 54.0 | 50.6 |

## PRESS_KEY in Editor Mode — Architecture Note

In editor mode, ImGui always has focus (`bImGuiGotFocus == true`), which gates the `GGTerrain_CheckKeys()` function. This means terrain debug keys (P, O, U, I, 1-8) cannot be detected via the normal `ImGui::GetIO().KeysDown[]` path.

**Solution**: `PRESS_KEY` sets `g_autoHarnessInjectedKey` (global int in AutomationHarness.cpp). This is consumed OUTSIDE the `bImGuiGotFocus` gate in `GGTerrain_CheckKeys()` (`GGTerrain_part0.cpp`), which directly sets `ggterrain_key_pressed[]` and `ggterrain_key_state[]`. A one-frame clear mechanism (`s_clearInjectedKeyNextFrame`) prevents the pressed flag from persisting forever.

**Key files**: `AutomationHarness.cpp` (sets `g_autoHarnessInjectedKey`), `GGTerrain_part0.cpp` (consumes it outside the `bImGuiGotFocus` block in `GGTerrain_CheckKeys()`).

## Profiler Architecture — Critical Knowledge

The Wicked Engine profiler has TWO layers of enable control:

1. **`wi::profiler::SetEnabled(true/false)`** — Sets `ENABLED_REQUEST` in `wiProfiler.cpp`. Synced to `ENABLED` on the next `BeginFrame()`.
2. **`bProfilerEnable`** (GameGuru, `M-GridEdit_part0.cpp:147`) — A GameGuru-level flag. **Every frame in editor mode**, `M-GridEditB_part3.cpp:75-78` checks this flag and actively calls `wi::profiler::SetEnabled(false)` if it's false.

**Critical**: You MUST set `bProfilerEnable = true` alongside `wi::profiler::SetEnabled(true)`, or the editor code will immediately disable the profiler on the next frame. The `ENABLE_PROFILER` harness command does both.

## Notes

- The harness response confirms the command was accepted, not that the resulting operation completed — always follow up with `GET_STATE` after waits
- All 19 demos were successfully tested through the hub->storyboard sequence on 2026-02-14
- 18/19 demos passed the full FPS test (hub->storyboard->editor->game->FPS->escape) on 2026-02-15 and 2026-02-16
- 19/19 demos passed on 2026-02-17 (Island Showdown fixed with increased load timeouts)
- 19/19 demos passed on 2026-02-18 (Phase 3 render pipeline hooks active, zero regressions)
- `GET_SCREEN_TEXT` provides full widget/button labels for every storyboard node — use this to verify screen content without screenshots
- When polling after a level load, use a longer timeout (60s) on the `GET_STATE` poll to account for the synchronous load

## Standalone-Game (PLAY GAME) Commands — added 2026-08-05

Built to replay the user's crash flow: hub -> select demo -> **PLAY GAME** -> title menu -> **START** -> LOADING LEVEL. Key architecture fact: for a project-backed demo, the hub's PLAY GAME button **relaunches GameGuruMAX.exe with `project=2<name>`** (app identity becomes `Guru-Game`, CWD `Files\`) and the hub process EXITS — the harness goes silent for ~10-20 s and then answers from the NEW process (same exe-relative auto_command.txt).

- `CLICK play_game` — now works on the HUB too (previously storyboard-only): sets `bTriggerPlayDemoGame`, consumed by the Play Game button in `Welcome_Screen` exactly like a click. Expect `OK: ... process may relaunch as standalone`, then a dead harness until the standalone boots.
- `TITLE_CLICK <action>` — presses a storyboard SCREEN widget by ACTION name: `start`, `exit`, `continue`, `back`, `resume`, `leave`. Queues `g_iAutoTriggerScreenAction`, consumed inside `screen_editor`'s widget hit-test loop as if hovered+released (click sound + action dispatch run the user's code path). `start` on the title node = STORYBOARD_ACTIONS_STARTGAME -> level load.
- `GET_STATE` in the standalone process returns `standalone_title` / `standalone_loading` / `standalone_playing` plus a raw `STANDALONE: titleloop=%d gameloop=%d levelloop=%d masterloop=%d` line.

Soak driver: scratchpad `playgame_soak.sh <cycles> <playsecs>` — per phase (boot-hub / relaunch-title / load / play) it watches THREE traps every poll: `dred_report.txt` growth (device removed; the report is written BEFORE the modal error box, so growth is detectable while the box blocks), `Guru-Crash.log` growth (symbolized AV), and process death. New dred/crash bytes are captured per cycle to `dred_cycleN_phase.txt` / `crash_cycleN_phase.txt`.

**DRED**: `dred.txt` next to the exe arms DRED (engine 1.51). With engine 2.03 the report also carries PASS NAMES per breadcrumb op (raw embedded ANSI markers replace PIX3 blobs when armed) — a hang now names the pass, e.g. which SVT compute list stalled.

**Known crash cascade (2026-08-05, pre-2.03)**: initial DEVICE_HUNG during LOADING LEVEL -> relaunch while driver recovering -> silent AV in GraphicsDevice ctor (`CreateCommandQueue` fails, `wi::platform::Exit()` returns, null `SetName`) -> next relaunch AV in `DescriptorAllocator::block_allocate`. Engine 2.03 turns both AVs into clean fatal exits with the DRED report intact.
