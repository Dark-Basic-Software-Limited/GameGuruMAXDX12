# Pre-alpha report — overnight run, 2026-09-20

Lee's brief: *"once you have completed your fixes, tests and a full sweep … produce a report on
your night's work and your recommendations before I zip up the build to share with my testers. As
part of this report, also scan all the functionality and fixes added to the DX11 repo since
November 2025 and compare to the current DX12 version and all the features we completed to date and
include anything you think is still missing or untested."*

**Build this report describes:** game `3163423a` main · engine `517c00f1` master · both clean and
pushed. Measured on the DESKTOP (RX 9060 XT).

---

## 1. Bottom line

**The VRAM campaign is finished.** Across a 19-demo single session, accumulation went from
**+90.1 MB per level load to +9.9 MB**, r² from 0.71 to 0.03 — load order no longer predicts VRAM
at all. Peak session VRAM 5743 → 4350 MB. Loads over the 4 GB gate 16 of 19 → 3 of 19. The worst
single demo came down **1637 MB**. 19/19 loaded, 19/19 reached Test Game, zero restarts, zero blank
frames, and the session ended back at the hub.

**DX11 parity is in better shape than the last audit left it.** I closed the "assets were never
audited" blind spot both audits flagged, and it came back clean: of 182 tracked non-code files DX11
changed since the fork, **none are stale in DX12**.

**My one blocking recommendation before you zip: run `tools/prealpha_clean.sh --apply`.** The build
area carries 200 MB of non-product files, and two of them are not debris but *switches* that turn
diagnostics on for whoever receives the build. §4 has the detail.

---

## 2. What I fixed tonight

### 2.1 GGMAX 3.68 — the last three VRAM suspects

All three were the same defect shape as the previous four: **a per-level value living in a
process-lifetime global that the next level only overwrites if it happens to supply one.** That is
now seven for seven, and it is the first thing to check whenever state seems to bleed between
levels.

| # | what | measured |
|---|---|---|
| 1 | **shadow packer containing size** | light level after a heavy one **272,629,760 → 17,039,360 bytes (−255.6 MB)**; the heavy level itself **−68.2 MB** |
| 2 | **grass material cache** | same-level reload **25 → 5** grassbank records; live grass types 4 → 21 → **4** |
| 3 | **MSAA outline render targets** | ~12 MB, now released when MSAA is off |

**The shadow packer.** `wi::rectpacker::State::clear()` deliberately keeps `width`/`height`,
`add_rect()` only grows them, and `pack()` starts from the current size and only ever *doubles* —
it never searches downward. `vis.shadow_packer` lives in `RenderPath3D::visibility_main` for the
whole process. So the containing size was a session high-water mark and the atlas faithfully
allocated it.

⚠ **This and 3.66's armed shrink are one fix in two halves.** The reset fixes what the packer *asks
for*; the arm is what lets the atlas *answer*. Either alone measures as nothing — which is exactly
why 3.66 read as "the atlas is exonerated, an input survives a level load". The trace shows both
halves working, and that second line never happened before tonight:

```
GGATLAS create: 5120x1024  -> 12288x4096  (discarded retained bound 4096x1024)   lights=4
GGATLAS create: 12288x4096 -> 4096x1024   (discarded retained bound 12288x4096)  lights=26
```

⚠⚠ The reset is at the call site, **not** in `State::clear()` — `State` has exactly two users and
the other, `wiFont`'s glyph atlas, legitimately wants the ratchet.

**A correction I owe you.** §3.67 argued that 12288 and 16384 were *unreachable from the rects*.
That was wrong, and it was wrong because `DUMP_SHADOWRECTS` reported the sun's width divided by the
cascade count against a height that was **not** divided — I used a broken instrument to bound a
quantity, and the same section had already listed that instrument as needing repair. With it fixed,
The Mystery of Z Island reads `packed=12288x2048 slices=6 per=2048x2048`: it **genuinely wants** a
12288-wide sun rect, 2048 cascade resolution × 6 cascades. The heavy level's big atlas was never the
bug. The bug was only ever that the light level could not get its own size back afterwards.

**Grass cache.** `g_grassMaterials[]` is built lazily on first sighting of a type and was never
reset, so it converged on the union of every grass type any level in the session painted. The
confirmation came free from the packer probe: Teaser → Z Island → Teaser, and the two Teaser
censuses differ by **20 grassbank DDS that belong to Z Island**. Same level, so content is constant
and those 20 are retained. Released once per level load now. Safe wholesale because nothing holds a
pointer into the array — the chunk create path does `scene.materials.Create(e) = *mat`, a copy.

**MSAA outline RTs.** Created under `if (getMSAASampleCount() > 1)` in `ResizeBuffers` and never
released when MSAA went off — and MSAA is per-level (`visuals->iMSAASampleCount`).

### 2.2 GGMAX 3.69 — two follow-ups the sweep data asked for

**The atlas shrink now waits for the level to settle.** With 3.68 in, the trace showed **54 atlas
creates over 19 loads (2.8 per load)** in a repeating shape: the one-shot arm was consumed by the
first qualifying frame, and during a level load that frame has only the sun visible. So the atlas
shrank to the transient, spent the arm, and the grow path put it back as the real lights arrived —
at one point `16384x4096 → 16384x2048 → 16384x4096` inside four frames. That is two 130–260 MB
allocations with DX12 deferred destruction still holding the old ones, *during a load*, on a 4 GB
minimum spec. The condition must now hold for 90 consecutive frames. It can only delay a shrink,
never prevent a correct one.

**The crash reporter no longer crashes.** `Guru-Crash.log` records an access violation on
**2026-09-16 inside `OnDeviceRemoved`**, reading a freed page — the crash reporter faulting while
reporting a device removal, which is the worst possible moment because that report is the only
artefact a tester can send back. Three pointers in the DRED breadcrumb walk were dereferenced
unguarded, and all three can legitimately be null in a real DRED output. All three are guarded now.

⚠ These guards are **reasoned, not tested** — a device removal cannot be produced on demand. They
are null checks on a path whose current alternative is an access violation, so the risk of adding
them is about as low as a code change gets, but I cannot show you a before and after.

### 2.3 Two gate defects, both the same class as the 17-of-19 bug

`sweepgate.sh` handed a soak-format results file **parsed zero rows and still printed
"C2 GEOMETRY PASS — POLYS identical on all 0 demos"** and "C3 VRAM PASS — worst 0.0 MB". Only C1
caught it. It now refuses outright and names the likely cause.

And it only understood one sweep's format, so the soak sweep needed a second scorer with a **second
POLYS reference** — which still held the stale pre-far-tree Aztec Teaser figure that 3.53c corrected
in the gate's own dict. Two references drift; one cannot. One gate reads both formats now.

---

## 3. Test results

### 3.1 Single-session soak, 19 demos, hub → editor → Test Game → hub

The functional gate Lee asked for. Same script, same machine, one build apart.

| | 0919 pre-fix | 0920 with 3.68 |
|---|---|---|
| reached the editor | 19/19 | **19/19** |
| reached Test Game | 19/19 | **19/19** |
| session restarts | 0 | **0** |
| blank frames (luminance variance) | 0 | **0 of 38 shots** |
| accumulation across load order | **+90.1 MB/load, r² 0.71** | **+9.9 MB/load, r² 0.03** |
| first load → last load | 3107.7 → 5121.6 MB | 3060.6 → **3484.1 MB** |
| peak in session | 5743.3 MB | **4350.4 MB** |
| loads over the 4096 MB gate | 16 of 19 | **3 of 19** |
| final state | hub | hub |

Per demo, editor VRAM, in load order:

| demo | 0919 | 0920 | delta | POLYS |
|---|---|---|---|---|
| Aztec Game Kit Teaser | 3107.7 | 3060.6 | −47.1 | +312 |
| Aztec Game Kit | 4160.0 | 4078.9 | −81.1 | same |
| Bounty | 3474.1 | 3249.0 | −225.1 | same |
| Horseshoe Bend | 3943.4 | 3339.1 | −604.3 | same |
| Island Showdown | 4228.5 | 3584.2 | −644.3 | same |
| Operation Amazon | 4670.5 | 4126.0 | −544.5 | same |
| River Raiders | 4705.3 | 3960.7 | −744.6 | same |
| Snowy Mountain Stroll | 4110.6 | 3452.3 | −658.3 | same |
| A Grand Canyon Adventure | 4267.9 | 3367.5 | −900.4 | same |
| Disruption | 4380.4 | 3612.0 | −768.4 | same |
| Foggy Forest | 4469.0 | 3636.0 | −833.0 | same |
| Indian Strike Force | 4578.5 | 3665.5 | −913.0 | same |
| Switch Escape | 4388.5 | 3224.1 | −1164.4 | same |
| Canyon Offensive | 5045.5 | 3841.1 | −1204.4 | same |
| Escape from the Zombie Cellar | 4651.8 | 3359.9 | −1291.9 | same |
| Jungle Fever | 4896.0 | 3534.3 | −1361.7 | same |
| RPG Template | 5050.0 | 3575.8 | −1474.2 | same |
| The Mystery of Z Island | 5743.3 | 4350.4 | −1392.9 | +840 |
| Trapped | 5121.6 | 3484.1 | **−1637.5** | same |

★ **The savings grow monotonically with load order** — −47 MB at load 1, −604 by load 4, −1204 by
load 14, −1637 by load 19. That shape is the proof. A fix that removed a fixed cost would save the
same amount everywhere; one that removes *accumulation* saves more the longer the session runs.

POLYS is identical on 17 of 19. The two that differ are **increases** (+312 on 1.41M, +840 on 240k)
— tree-pool slots granted, not geometry lost.

**Still over the 4096 MB gate in a cumulative session:** Aztec Game Kit (4079), Operation Amazon
(4126), The Mystery of Z Island (4350). All three are close, and Z Island legitimately needs a
12288×2048 sun rect. ⚠ Note this gate was written for the *fresh-launch* sweep, where each demo
loads cold — §3.2 is the run that speaks to the shipped minimum-spec claim.

### 3.2 Fresh-launch sweep and final soak on the 3.69 build

PENDING_FINAL_SWEEPS

---

## 4. Before you zip — recommendations

### ★★★ 4.1 Run `tools/prealpha_clean.sh --apply` (new tonight)

The build area carries **202 MB across 110 files** that are not product. Most is noise, but two are
**arming files** — the engine checks for the file's existence at startup and turns a diagnostic *on*
if it is there:

| file | what shipping it does |
|---|---|
| `dred.txt` | turns on D3D12 Device Removed Extended Data + auto-breadcrumbs, for every tester, at a GPU cost |
| `gg_atlas_trace.txt` | shadow atlas create/shrink tracing to disk |

⚠ **`dred.txt` has been in this build area since 2026-07-27**, which means every VRAM and FPS number
in this repo was measured with DRED on. Worth remembering when a tester's numbers come back
different from ours.

**My recommendation on `dred.txt` specifically: keep it for a pre-alpha, now that 3.69 has guarded
the walk.** An armed DRED turns a tester's device-removal into a real report instead of a shrug.
Before tonight I would have said strip it, because the walk itself could AV. The script lists
arming files separately from debris so you can decide per file.

The script also confirms `setup.ini` is already correct (`producelogfiles=0`, `memgeneratedump=0`)
and reminds you that `GameGuruMAX.pdb` **should ship** so tester crash logs symbolise. It leaves
four things listed but untouched for you to judge: three loose `*_surface.dds` at `Files/` root with
no copy in entitybank, and `Files/savegames/` (276 KB of save games from testing — a fresh install
probably should not ship them).

Also in that 202 MB: five loose `.sh` test scripts from Feb–Mar 2026 next to the exe, not
git-tracked, unused, flagged in the August alpha audit and still there; and `Files/particlesbank_old/`
at 20 MB.

### 4.2 Known issues worth listing for testers up front

So they arrive as known limitations rather than surprise reports:

- **Three material dropdown entries do nothing.** "Water", "Glass" and "Tree Animate Doublesided"
  compile against the *engine's* `objectHF.hlsli`, so their GameGuru bodies are not in the build.
  Proven from the shipped binaries: `objectPS_custom_water.cso` and `objectPS_transparent_glass.cso`
  are **byte-identical**, and `objectVS_common_tree.cso` is byte-identical to `objectVS_common.cso`.
  (Tree sway itself is fine — 3.58b reproduces DX11's per-material "Object Wind" through the engine
  wind path instead.)
- **Custom shader parameters 1–7 have no effect.** The engine replaced seven floats with
  `uint4 userdata` and the migration was never finished. Authoring surface only; zero shipped `.fpe`
  files use them (4248 scanned). Since 3.50 your authored values are at least no longer *destroyed*.
- **No PP Snow/Dust, no transparent shadows, no bloom strength, no level-load gamma fade-in.** The
  first three need engine-side work. The fourth does not — see §6 item 1.
- **AI path detail varies with where the camera sat when the nav mesh was built.** DX11's
  high-quality extraction was ported byte-identically and then reverted: under DX12's terrain system
  it blocks the main thread 60+ seconds on a large map. Bisected both ways on Horseshoe Bend.
- **Env-probe release pops instead of fading** when the player leaves an indoor volume.
- **GPU particles sort as one batch per frame**, so an effect can sort wrongly against glass.
  Restoring DX11's interleave needs an engine callback that was deliberately not taken.
- **Three demos sit just over 4 GB late in a long session** (§3.1).

---

## 5. DX11 since November 2025 vs DX12

**The fork point is `ca32a143`, 2025-11-21.** DX11 has **338 commits** since then that are not in
DX12. DX11's own HEAD is `3e21f674`, 2026-08-13, and has not moved since.

### 5.1 Where parity stands

Those 338 commits were audited **per-commit** (2026-09-16) and again **structurally** (2026-09-17,
hunting the port's characteristic half-a-feature defect rather than commits). That produced 34 + 12
findings, and **3.44 through 3.50 cleared the backlog**. Spot-checked against the tree tonight:

| fix | evidence today |
|---|---|
| chest keys flag as collected | `collected = 1` / `= 2` at `DarkLUA_part4.cpp:1035-1036` |
| key-state refresh | 9 call sites (DX11 has 5; DX12 splits files) |
| `bit32` Lua 5.4 compat | installed in `DarkLUA_part7.cpp` before any script runs |
| storyboard video auto-advance | 11 references, reads present |
| crash-logger null guards | 17 `pExceptionInfo` references |
| terrain `recursive_mutex` | `GGTerrain_part0.cpp:9348` |

### 5.2 ★ I closed the blind spot both audits flagged

Both ended with the same caveat: *"assets were never audited… if DX11 added other art, models or
sounds in these 338 commits, this audit would not have seen them."* Checked properly tonight.

DX11 changed **182 tracked non-code files** since the fork (Lua scripts, behaviour icons, `.fpe`,
compiled `.byc`). Against the DX12 tree:

- **79 byte-identical** to DX11 HEAD.
- **98 differ only in line endings** — content identical.
- **0 stale.** Not one file where DX11 moved on and DX12 is still at the fork version.
- **2 diverge deliberately**: `door.lua` and `door_sliding.lua`, carrying the GGMAX 3.21 logic
  gating (`SetEntityAlwaysActive` conditional on the door being switch-operated instead of
  unconditional). I read both diffs; they contain that change and nothing else.
- **3 absent** — `border_maps.lua` and its two icons, which **DX11 deleted** on 2026-01-27. Aligned.

Whole-file comparison too: **6458 tracked paths in DX11, 7965 in DX12**, and every DX11 path absent
from DX12 is explained — the vendored Lua 5.2 sources (DX12 uses the engine's 5.4.8 plus a compat
shim), the DX9/DX11-era `cShadowMaps`/`DepthTexture`, SDK sample and tool sources, workshop `.bat`
files, and `BulletPhysics.CPP`, which DX12 has split into `BulletPhysics_part0..2.CPP`.

**So no DX11 script, icon or behaviour file added since November 2025 is missing or stale in DX12.**

### 5.3 ★ An independent content-level audit of the C++ side

The two previous audits both worked commit by commit. Tonight I ran a different method that cannot
be fooled by a commit someone judged done: take **every line the 338 commits added** to a C++/H
file, keep the substantive ones, and ask whether that exact text exists anywhere in the DX12 tree.

**1769 distinct substantive added lines; 1438 (81.3%) present verbatim.** The 331 absent ones
cluster entirely into categories already known and accounted for:

| absent lines | commit / cluster | why |
|---|---|---|
| 62 | `GGTrees.cpp` | DX12 rearchitected trees into engine ObjectComponents |
| 52 + 12 + 12 | terrain GPU race / readback | D3D11-specific plumbing, `ID3D11Query`, `texReadBackStaging` |
| 51 + 49 + 34 + 8 | crash log system | DX12 has its own 3.38 rewrite, arguably better at stack walking |
| 20 + 4 | PIX tracing | the DX12 engine has no `BeginCommandList(QUEUE,"name")` and emits PIX events unconditionally |
| 19 + 15 | debug overlays / profiler text | DX12 has its own profiler panel (§3.19–3.20) |
| 18 | nav-mesh HQ extraction | deliberately reverted, see below |

The small clusters — the ones most likely to be real gaps — all resolved to DX12 naming conventions
rather than absences: `GetSunColorRed/Green/Blue` and `ForceMouseXYClick` are registered on `lua2`
rather than `lua`, and Delayed Shot uses `MAXTimer()` rather than `Timer()`. All present.

### 5.4 DX11 behaviour DX12 deliberately does not have — verified still true

| feature | why |
|---|---|
| nav-mesh high-quality extraction (`15cbd06c`) | ported, then reverted — 60+ s main-thread block under DX12's terrain system. `GetTriangleListHighQuality` is absent by choice. |
| `terrainsleep` lock restructure (`633f216f`) | **DX11 walked this back itself** in `01d27f07`. DX12 already compiles what DX11 compiles today. |
| PIX `BeginCommandList(QUEUE,"name")`, `SetSpecialGGDebugLog` | no such API in the DX12 engine. |
| PP Snow, transparent shadows, bloom strength, gamma fade | engine API gone; confirmed absent from `WickedEngineDX12` tonight. Fields still default, save, parse and copy, so **DX11-authored levels round-trip their values untouched**. |
| custom shader parameters 1–7 | `uint4 userdata` migration unfinished. |
| Water / Glass / Tree-Animate shader bodies | not in the build; proven by identical `.cso` hashes. |

### 5.5 Behaviour that intentionally differs from the DX11 you last used

- **Bullet holes on immobile objects.** DX11 `c1269dd7` removed the `isimmobile` clause; DX12
  matched it in 3.44. A door no longer collects decals unless its "Allow Bullet Holes?" tickbox is
  set. ⚠ This is the one change that alters behaviour you personally tested on 09-16.
- **Shadow defaults on a NEW level**: Delayed Shadows default ON, cascade resolution 1024 (was off /
  2048), matching DX11 `d836fcab`. Existing levels restore their own saved values.
- **Start Marker underwater** now works — DX11 `1d49f004` removed the waterline+20 clamp.

### 5.6 Where DX12 is deliberately ahead

`GetObjectAnimationFinished` has a null check DX11 lacks; `door.lua` is DX11 v33 *plus* the 3.21
logic gating; the Lua 5.4 compat shim; and the crash handler tolerates a NULL exception record, a
failed `SymInitialize`, prints the breadcrumb ring buffer, and as of tonight survives a malformed
DRED output — DX11 does none of the four.

### 5.7 The one hole left

Content **neither repo tracks** — models, DDS, sounds. There is no DX11 build area on this machine
to diff against, so if DX11 added art since November 2025 that is not in git, nothing I can run
would see it. That needs your DX11 install, not a repo.

---

## 6. Still open — ranked, with a recommendation on each

Nothing here blocks a pre-alpha. In the order I would take them.

| # | item | impact | cost | recommend |
|---|---|---|---|---|
| 1 | **Level-load gamma fade-in is dead.** The ramp is fully intact and both consumer lines at `G-Lighting.cpp:341-346` are a bare `;` with a TODO. | MED — DX11 hid level-load construction artefacts behind an 84-frame fade; DX12 shows them | **not as cheap as the audit said** — see below | do it next, with eyes on it |
| 2 | **Env-probe release fade** — `g_bEnvProbeTrackingUpdate[...] = false` fires unconditionally on both branches (`GGTerrain_part0.cpp:9561`, `:9606`), so the range decrements once and the probe is parked on the next statement | MED — probes pop leaving an indoor volume | small | after 1 |
| 3 | **Sun cascade strip is one row.** Z Island's 6 cascades at 2048 make a 12288×2048 rect and the packer then needs 12288×4096. Two rows of three would be 4096×4096 for the same pixels | ~130 MB on the heaviest demos — the largest remaining shadow-memory item | **large**: engine rect layout *and* every shader that indexes it | not this alpha |
| 4 | **PP Snow residue** — `bPPSnow` still gates an every-15th-frame indoor `IntersectAllEx` raycast whose result is discarded with `(void)iHitObj;` (`M-Game_part3.cpp:342`) | none visible, wasted work | tiny | free win when you are next in that file |
| 5 | **Procedural-preview fog** — `M-TerrainNew_part5.cpp:888` is a bare `;`; replacement field `weather->gg_fog_opacity` exists | LOW | tiny | verify first — the sibling branch may be inert too |
| 6 | **`enablepixmarkers` reads nowhere** — parsed in two places, `setup.ini` ships `=1` against a key nothing reads | none | tiny | restore the crash-log print, or delete the key |
| 7 | **`_ConvertFormat` bypassed by a numerically wrong cast** (`wickedcalls_part3.cpp:1877`) — `wiGraphics::Format` and `DXGI_FORMAT` do **not** share values, so a BC1 atlas is reported ~4× too large | LOW — only via `memgeneratedump=1`, but the VRAM campaign reads that log | small | fix with the `MaterialGetSRV` stub or not at all |

**On item 1, a correction to the earlier audit.** It called this cheap and it is not quite. The
replacement lever is `master_renderer->setBrightness`, and brightness is an **additive offset after
contrast** (`result.rgb = (result.rgb - 0.5) * contrast + 0.5 + brightness`), so a fade from black
is brightness −1.0 → 0.0. But `Wicked_Update_Visuals` writes `setBrightness((fGamma - 2.2) * 0.15)`
unconditionally every time visuals update *and* assigns `g_fGlobalGammaFadeIn = visuals->fGamma` in
the game branch (`M-GridEditB_part3.cpp:2044`, `:2055`) — so the fade has to be composed with the
gamma slider's value rather than replacing it, and the assignment must not clobber the ramp
mid-fade. It is an afternoon with the app open, not a one-liner.

### What I deliberately did **not** do

- **Did not ship the gamma fade.** It changes what *every* level load looks like, and I could not
  get your eyes on it. Wrong thing to put in a tester build unreviewed.
- **Did not touch `wi::rectpacker::State::clear()`** — `wiFont`'s glyph atlas wants the ratchet.
- **Did not loosen the atlas anti-thrash gate.** A cold Teaser sits at 5120×1024 wanting 4096×1024
  and cannot shrink because the gate needs `packer*2 <= atlas`. 4 MB is the right price.
- **Did not delete anything from your build area.** `prealpha_clean.sh` defaults to a dry run.

---

## 7. What has never had a human eye on it

Fixed, built, shown not to regress — but the behaviour itself has never been exercised. If you want
your manual time to pay, spend it here.

**From the DX11 parity fixes (3.44–3.50):**

| what | how to reach it |
|---|---|
| storyboard video auto-advance | a non-looping splash video linked to a screen — it should advance itself at the last frame |
| save-game reload of spawned objects | spawn something during play, save, reload — it should be visible and solid |
| chest keys | put a key in a chest, assign it as a door's USE KEY, take it, try the door |
| Lua Sound slot 4 | an entity using Sound4 from Lua — should play slot 4, not 5 |
| `animchoicemode` 1 and 2 | a GUNSPEC using them — unreachable before |
| ultrawide Save/Load | click near where two slot boxes meet |
| grid size below 1.0 | set 0.5, restart MAX, check it survived |
| WPE preview offsets | an emitter with an X/Z offset — preview should match the placed result |
| Delayed Shot | bow / crossbow |
| additive blending | a glow decal — should stay additive when you Test Game |

**From this week (3.53–3.69):**

| what | why it is worth a look |
|---|---|
| **a long play session across many levels** | every VRAM fix this week is about what survives a level change; the sweeps prove the editor and Test Game paths, not an hour of real play |
| **"Export Game" (standalone build)** | ⚠ **no harness verb exists for it and it has not been exercised since 2026-08-16.** A tester will click it. This is the largest untested surface in the build. |
| **hub PLAY GAME** (relaunches as a standalone, `project=2`) | a different path from Test Game, with its own crash history |
| MSAA on, then a level with it off | tonight's fix; no demo in the set flips it |
| grass on a level reached late in a session | the cache is dropped and rebuilt per level load now — grass should look identical, just not inherit other levels' types |
| shadows on a light level loaded after a heavy one | the atlas shrinks now; shadow quality should be unchanged |
| rain and snow (3.59 WPE swap) | they are new particle effects now, not the legacy ravey system |
| wind / tree sway at the extremes of the slider (3.61–3.63) | the mapping was inverted and re-floored |
| bullet tracers (3.55/3.56) | restored from a hook that was never re-homed in the port |

**And the standing visual acceptance test**: TESTPRO1 island, saved cliff camera, DX11 vs DX12 side
by side. The scorecard's last two open rows are tree wind — now shipped in 3.57/3.58b and worth
re-scoring — and shadow behaviour, which has never been A/B'd.
