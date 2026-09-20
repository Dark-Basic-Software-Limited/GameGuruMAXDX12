---
name: project-prealpha-readiness
description: "Shipping a build to testers: the build area carries ARMING FILES that switch diagnostics on for whoever receives it, plus 107 MB of debris. Run tools/prealpha_clean.sh first."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-20T00:57:52.240Z
---

**2026-09-20, ahead of the first DX12 tester build.** `tools/prealpha_clean.sh` (dry run by
default, `--apply` to delete).

★★★ **The build area contains ARMING FILES, not just log debris.** The engine checks for a file's
EXISTENCE at startup and turns a diagnostic ON if it is there. Zipping the build area ships those
switches in the ON position:
- **`dred.txt`** - D3D12 Device Removed Extended Data + auto-breadcrumbs, at a GPU cost.
- **`gg_atlas_trace.txt`** - shadow atlas create/shrink tracing to disk.
- Others the engine looks for: `leakall.txt`, `leakterraintex.txt`, `resource_hijack.txt`,
  `stream_guard.txt`, `stream_load.txt`, `gg_pso_fail.txt`, `last_upload.txt`, `anim_garbage.txt`.

⚠⚠ **`dred.txt` has been present on the DESKTOP since 2026-07-27**, so **every VRAM and FPS
baseline in this repo was measured with DRED ON**. Remember that when a tester's numbers differ.
There is a real argument for keeping it in a PRE-ALPHA (far better device-removal reports) - that
is Lee's call, so the script lists arming files separately rather than lumping them in.

Other hygiene, all verified 09-20:
- ~107 MB / 71 files of non-product text in the build root, ~96 MB of it harness screenshots.
- Five loose `.sh` test scripts from Feb-Mar 2026 next to the exe; not git-tracked, unused, flagged
  in the 08-18 alpha audit and still there.
- `setup.ini`: `producelogfiles=0` ✓ (this is what gates the alloc-tripwire ledger via
  `GGSetDiagTraceFiles`) and `memgeneratedump=0` ✓ - both already correct.
- ⚠ **KEEP** `GameGuruMAX.pdb` (tester crash logs symbolise against it), `ffmpeg.exe`,
  `steam_appid.txt`, `changelog.txt`, `dxdiagsystemspecs.bat`, `WinPixEventRuntime.dll`.
- Content added on the DX12 side lives in `GameGuru Core/content-additions/` and is deployed by
  `tools/deploy_content_additions.sh` - verified present in the build area 09-20.

★ **Untested paths a tester will reach that no sweep covers:** "Export Game" (standalone build) has
no harness verb and has not been exercised since 2026-08-16; hub **PLAY GAME** (relaunches the exe
as a standalone, `project=2`) is reachable via `CLICK play_game` and has its own crash history.
The soak sweep covers hub -> editor -> Test Game only.

Related: [[project-alpha-packaging]], [[project-single-session-soak]], [[project-vram-retention]],
[[project-playgame-crash]]
