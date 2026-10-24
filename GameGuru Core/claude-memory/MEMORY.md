# GameGuruMAX Project Memory

> ★★★ **TESTER REPORTS? `GameGuru Core/TESTER_TRIAGE.md`** (build md5s, known issues, repro tools). #1-#6 = 3.102-3.110 (Lee-confirmed 10-12); 3.111 thumbnails. 10-14..10-17 TESTPRO2 > DX12TESTLEVEL round ALL confirmed: 3.112-3.115 [terrain bake](project_terrain_bake.md) + [far terrain](project_far_terrain.md), 3.116-3.118 [grass](project_grass_density.md), 3.119-3.120 baked water, 3.121 [library previews](project_object_library_preview.md); #7 arms 3.122/b (confirmed 10-18); #8 3.123 = false modal + [saved demo copy](project_demo_shadow_copy.md) (awaiting); #9 3.124 [engine Lua writable area](project_lua_writable_area.md) (confirmed 10-22); SSAO railings 3.126/b [MSAO far-plane tolerances + depth pick](project_ssao_msao.md) (confirmed 10-24); env probes 3.127/b = [terrain missing at load + atlas mosaic + game-mode park capture](project_env_probes.md) (confirmed 10-24). Hashes in RESUME rows. Hub test 1010 CLEAN.
> ★★★ **[Custom terrain textures / grass from Documents](project_custom_terrain_textures.md)** — 3.125 (10-22, worktree `elastic-swirles-795710`, UNCOMMITTED, awaiting Lee). ★ A level SAVE deletes old `_automatedbackups`; Wicked paths UTF-8 vs GG file layer ANSI; cStr buffers = length+8.
> ★★★ **RESUMING? `GameGuru Core/RESUME_2026-09-24.md` FIRST** (top block = current round) and [Next action immediate](project_next_action_immediate.md). Open: two levels baked at a downgraded preset; 4096 near-tier bake default. Lee does file cleanup himself.
> ⚠⚠ **TWO MACHINES** — run ``[ -d /d/DEV/BUILDS ] && echo LAPTOP || echo DESKTOP`` first. DESKTOP = RX 9060 XT, source of every FPS/VRAM baseline; LAPTOP = GTX 1050, baselines INVALID there. [Machine migration](project_machine_migration.md)
> ⚙ Mirrored to repo `GameGuru Core/claude-memory/` — `tools/sync_claude_memory.sh to-repo` / `to-live`. ⚠ Live path keyed on the repo's ABSOLUTE path (wrong key fails silently).

> ★★★ **[VRAM retained across level loads](project_vram_retention.md)** — CLOSED 09-20; 7 defects, one shape: a per-level value in a process-global. ★ A bound is only as sound as its instrument.
> ★★★ **[Standalone VRAM](project_standalone_vram.md)** — PLAY GAME holds textures at FULL res (+950..1520 MB on 3 demos); cause NOT found. Test Game was the discriminator.
> ★★★ **HANDOFF DOCS (09-20)** — `PREALPHA_REPORT_2026-09-20.md` (§3.6 big finding) · `CHECKLIST_CHRONOLOGICAL.md` (numbered items Lee works through; 09-20: 133 confirmed / 10 swept / 65 never exercised, clustered at both ENDS) · `CHANGES_SINCE_2025-11.md` (by app area). All stop at 09-20 unless superseded.
> ★★ **[Pre-alpha readiness](project_prealpha_readiness.md)** — build area holds ARMING FILES (dred.txt...) + debris: `tools/prealpha_clean.sh` first. Export Game untested since 08-16.
> ★★★ **[Single-session soak](project_single_session_soak.md)** — 19/19 in ONE launch; keep BOTH sweeps (relaunch sweep is blind to accumulation). ~93 MB VRAM per sequential load.
> ★★★ **[Second level blank/hang](project_second_level_atlas.md)** — SOLVED 09-18 by BISECTION after 8 refuted theories. ★ "It worked weeks ago" = bisect FIRST.
> ★★★ **[Test Game freeze](project_testgame_freeze.md)** — a Lua MessageBox (5.4 dropped `math.atan2`). ★ A 0%-CPU "hang" = look for a DIALOG before a lock.

## Current campaign — performance / ultra-low-spec
- ★★★ **3.35b–h (08-29)** — live Texture Detail RACED streaming → DEVICE_HUNG (guard `GGReloadGuardBegin/End` existed, unused); crash path must not use RTTI; Super Quick defaults 3. Notes §3.35b–h.
- ★★★ **3.34–3.35 Super Quick** — Opaque Scene is 99% objects; floor is 1320 draws not fill; reduced VS/PS layout stock→FLAT −52%. Notes §3.34–§3.35.
- ★★★ **[Terrain Bake / Water Bake](project_terrain_bake.md)** — 3.25 milestone; 11 silent defects (3.73 bake deleted near trees). Read its rules before touching a custom draw, a bake, or removing a component.
- [Terrain + VT performance](project_terrain_vt_perf.md) — brutal off-switches, VT cost hiding as "GPU idle", bake +35%. · [Far-tree billboards](project_far_tree_billboards.md) — 2.95–3.07.
- ★★ **3.19/3.20 panel** — profiler rows stable by name; Object Detail Distance works UPWARDS; Texture Detail LIVE. ⚠ gave Lee a WRONG number from a sparse sample. Notes §3.19–§3.20b.
- ★ **Sweep 0825 = the POLYS reference** (`tools/sweepgate.sh`, 19/19). Tree pool / billboard / draw_distance changes move it — amend C2 BEFORE the run. DESKTOP numbers. `DEMO_FPS_SWEEP.md`.
- [Low-spec off-switches](project_lowspec_offswitches.md) — Shadows Off +17%; Occlusion Off is a LOSS. Read before adding a switch.

## Recently closed (detail in the linked file)
- **Env probes** [Env probes](project_env_probes.md): #157 circles = `half` overflow (ships `probeparallax=3`); 2.90 Brightness restored / Range removed (⚠ do not re-open its −13.4% FPS); ⚠ 2.77 capture defect CONFLICTED; 2.76 getRadius() is the half-diagonal.
- [gpup/.arx particles](project_gpup_particles.md) · [WPE](project_wpe_particles.md) · [Light falloff 2.10](project_light_falloff_parity.md) · [Transparency parity](project_transparency_parity.md) · [Shadow system](project_shadow_system.md) · [PLAY GAME DEVICE_HUNG](project_playgame_crash.md)
- VRAM goal met (19 demos fit 4 GB; TESTPRO1 standing test bed). Standalone arc 2.69–2.71 confirmed. [All stock DDS converted](../../../../../max/GameGuruMAXDX12/GameGuru%20Core/MILESTONE_DDS_CONVERSION.md) (revert `D:\max\mipbackup`).
- ★★★ **Reduction Scale desync 3.92/3.93** — one armature per skinned mesh, group on the world PIVOT. ★ A number that AGREES with you is worth one check.
- ★★★ [Test Game never restored the quality preset](project_testgame_preset_not_restored.md) — 3.96; ⚠ SOAK POLYS numbers are LOW-preset.
- ★★★ [Object Library live preview](project_object_library_preview.md) — engine `render_to_texture`; a preview is its OWN scene; ⚠ BindCommonResources without BindCameraCB = DEVICE HANG.
- ★★★ [Two fog paths](project_fog_two_paths.md) — engine vs GG `ApplyFogCustom`; a compat shim silently RE-DERIVES values.
- ★★★ [customDraw hook inventory](project_customdraw_hook_inventory.md) — DX11 hooks became `customDraw_*` pointers set in `master_part1.cpp`; grepping 3 files missed the rest (4 hits).
- ★★★ [Tree sway & 3 tree paths](project_tree_sway_and_paths.md) — fixed 3.57/3.58b. ★ "Feature X missing" = something MOVED: look for the caller before the bug.
- ★★ [Weather ravey → WPE](project_weather_wpe_swap.md) — DONE 3.59; diagnose with `GET_WEATHER`.

## Durable cross-cutting RULES (hard-won)
- ★★★ **[Measuring rules](project_measuring_rules.md) — READ BEFORE ANY A/B OR PERF CLAIM** (count the WORK; size the window to what you could be wrong about; interleaved same-session A/B of two BUILDS).
- ★★★ **[Rendering / DX12 rules](project_rules_rendering_dx12.md)** — throttle by the data's unit; share the compiled VS; resources on the MAIN THREAD; PSO descs store POINTERS; fp16 max 65504 = 1.66 km; new fields ride `Serialize`; streaming ON by default.
- ★★★ **[Debugging habits](project_rules_debugging.md)** — "dead" comments cite each other; CALLS ≠ SUCCESSES; correct-but-deferred = broken; validate LIVENESS; level-data mutators.
- ★★★ **[Environment / tooling rules](project_rules_environment.md)** — taskkill SUCCESS lies; vacuous tests PASS; `OPEN_PROJECT` sees MY GAMES only; modal paths read as hangs; `cmd //c "call…&&…"` can run NOTHING; `cp -r` mtimes no-op `/t:Build`.
- ★★★ **[Programmatic source edits](project_rules_patch_scripts.md)** — `<< 'EOF'` heredocs COLLAPSE `\\` (chr(92)); binary mode + assert ending counts; `re.sub` replacement escapes; ⚠ Git Bash `sed -i` STRIPS CRLF from a whole file (3.125) - Edit tool only; ⚠ no `BeginDisabled`.
- ★★★ [GameGuru time units](project_gg_time_units.md) — `g.timeelapsed_f` is 1/20 s (`>= 100` = 5 s). · ★★★ [Porting clusters](project_porting_clusters.md) — excluding a file ≠ excluding its cluster; `patch -F0`.

## CRITICAL BUILD RULES
- ★★★ **[Dual VS installs](project_toolchain_dual_vs.md)** — build.bat PICKS the install that exists; green incremental builds proved nothing — force a rebuild of what you claim about.
- **Release only** (Debug exits 3); Release outputs straight to the EXE dir. **Build**: `cd "D:/max/GameGuruMAXDX12/GameGuru Core" && ./build.bat Release` (cwd MUST be GameGuru Core).
- **Launch**: `cd "D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max/" && "./GameGuruMAX.exe" &` (works on both machines). **Kill first**: `taskkill.exe //IM GameGuruMAX.exe //F` then VERIFY with tasklist.
- **Engine lib**: `cd D:/max/WickedEngineDX12 && ./build_wicked.bat Release` (~40 s; branch `master`). An engine build clears `Max/shaders/*.cso`. [Shader build pipeline](project_shader_build_pipeline.md) — prove shader edits by .cso SIZE and with a GAME build.
- **NEVER `build.bat | tail && probe`** — gate on errors [feedback](feedback_gate_probes_on_build_errors.md) · stale `auto_command.txt` runs on next launch [feedback](feedback_stale_auto_command.md) · leaked parallel runners corrupt sweeps [feedback](feedback_leaked_parallel_runner.md)
- ★★★ **[Harness: Demo Games tab](feedback_harness_demo_games_tab.md)** — `NAVIGATE hub.demo_games` before `SELECT_DEMO`. ★ A test that can't tell its failure modes apart reports the wrong one confidently.
- **Worktree builds**: [junction + libs + cmd wrapper](feedback_worktree_build.md) · [worktree paths](feedback_edit_worktree_path.md) · [mirror every file](feedback_mirror_complete_file_set.md) · [check main repo status](feedback_check_main_repo_status.md). ⚠ The build area / harness is SHARED with other sessions — coordinate before build/launch.
- ⚠⚠ **[NEVER save TESTPRO2 > DX12TESTLEVEL](feedback_never_save_dx12testlevel.md)** - not even with a restore; test saves on a copy.
- **DX11 repos `D:\max\GameGuruMAX` / `D:\max\WickedRepo` STRICTLY READ ONLY** [feedback](feedback_dx11_reference_readonly.md) · "Commit and push" = local `main` + origin, never disturb stashes [feedback](feedback_commit_and_push_means_main.md) · notes + commit per milestone [feedback](feedback_notes_commit_as_you_go.md)
- **Be decisive; autonomous loop** [feedback](feedback_be_decisive_dont_over_confirm.md) · [feedback](feedback_autonomous_testing.md) · **kill MAX any time in bug-fixing (Lee 10-10)** [feedback](feedback_kill_max_after_10min.md) · two failed attempts = wrong approach [feedback](feedback_two_attempts_change_approach.md) · [don't thrash](feedback_dont_thrash_on_automation.md) · [instrument that NAMES the culprit](feedback_instrument_before_theory.md)

## Topic index
- **Paths**: [Writable area (user projects in Documents)](project_writable_area.md) · [CWD and file paths](project_cwd_and_file_paths.md) (debug WRITES via `GetDiagnosticPath`/`GGDiagFopen`) · [Demo shadow copy](project_demo_shadow_copy.md) · [Alpha packaging](project_alpha_packaging.md) (pdb SHIPS)
- **Lua**: [logic gating](project_lua_logic_cost.md) · [writable area](project_lua_writable_area.md)
- **VRAM**: [floor](project_vram_floor.md) · [knob plan](project_vram_audit.md) · [census](project_vram_census.md) · [grass benchmark = THE gate](project_grass_benchmark.md)
- **Performance**: [Switch Escape](project_switchescape_perf.md) · [Performance](project_performance.md) · [G&P panel](project_gfx_perf_panel_audit.md) · [FPS baseline](project_demo_fps_baseline.md) · [open My Games](project_harness_open_my_games.md)
- **Terrain**: [far terrain](project_far_terrain.md) · [render path = Wicked SVT](project_terrain_render_path.md) · [VT zoom squares](project_vt_zoom_squares.md) · [sculpt/paint](project_terrain_editing.md) · [blendmaps](project_terrain_blendmaps.md) · [texture mismatch](project_terrain_texture_mismatch.md) · [chunk scale](project_chunk_scale.md) · [custom textures](project_custom_terrain_textures.md)
- **Grass**: [density / far field](project_grass_density.md)
- **Trees / transparency / shadows / lights**: [Phase 5 trees](project_trees_phase5.md) (SetAlphaRef INVERTED) · [DX11 parity baseline](project_dx11_parity_baseline.md) · [Point-shadow budget](project_point_shadow_budget_design.md) · [Shadow flicker](project_shadow_flicker.md)
- **Arms**: [custom arm sets 3.122/b](project_custom_arms.md) · **Picking**: [CACHED layerMask](project_layer_cache_pick.md) · **Engine deltas**: [Wicked changes](project_wicked_engine_changes.md) (repo `WICKED_ENGINE_CHANGES.md` = authority)
- **Tech debt**: [UI audit](project_ui_audit_2026_07_28.md) · [Save crash thumbnails](project_save_crash_group_thumbnails.md) · [Level version](project_level_version_debt.md) · [Reload corruption](project_reload_corruption.md) · [Parrot corruption](project_parrot_corruption.md) · [Review 07-17](project_review_2026_07_17.md)
