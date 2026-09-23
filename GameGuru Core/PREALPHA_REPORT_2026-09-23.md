# Pre-alpha report — 2026-09-23

This is a **delta on top of `PREALPHA_REPORT_2026-09-20.md`**, not a replacement. Everything in
that report still stands; nothing in it was invalidated tonight. Read it for the DX11 comparison,
the standalone VRAM finding (§3.6) and the never-exercised list (§7).

**Build:** game `5af6e350` main · engine `3c955028` master · both clean and pushed. DESKTOP
(RX 9060 XT), 4.2 h since boot, `dred.txt` armed as for every prior baseline in this repo.

---

## 1. Bottom line

**The release gate is CLEAN, 19/19, with 312.5 MB of headroom** — the first full gate sweep since
3.79, so everything from 3.80 to 3.84c was unproven against it until tonight. Both predictions I
pre-registered before the run came out the way they had to (§3.1).

**The VRAM campaign holds on the current build.** Against the 09-19 soak: peak editor VRAM
**5743.3 → 4354.1 MB**, loads over the 4 GB floor **16/19 → 2/19**, mean **4473.3 → 3613.4 MB**.
Every demo down except one, by up to 1637 MB. 19/19 loaded, 19/19 reached Test Game, zero
restarts, zero blank frames, session ended back at the hub.

**Yes, I would send this out.** The blocking recommendation is unchanged and unglamorous: run
`tools/prealpha_clean.sh --apply` first (§4).

**Two things to read before you decide**, neither of them new tonight and neither a blocker:

- **§3.3 — after one Test Level round trip, every level you load afterwards can draw up to 35%
  fewer triangles**, for the rest of the session, silently. Root cause found and named to the line:
  `SetGlobalGraphicsSettings` is applied entering Test Game and the restore on the way back is a
  **comment**. It only fires when *Graphics Quality (Test Game)* is below High, and the shipped
  default is High — so a default install is safe, but a low-spec tester is exactly the person who
  changes it. Not fixed tonight, for the reason given there.
- **§2 — 3.84c changes how every mesh in the build is normal-mapped**, and no automated check can
  tell you a surface now looks *right* rather than merely *different*. It wants your eyes.

---

## 2. What changed since the 09-20 report

Eight numbered changes, in order. None of them had been through a gate sweep before tonight.

| # | change | how a tester reaches it | who has confirmed it |
|---|---|---|---|
| 3.80 | **Planar reflection alignment** — a DX11 fix the port had dropped | stand a crate at a puddle edge | Lee reported it, measured fix |
| 3.81 | **The Reflections checkbox loaded OFF** on levels whose author ticked it | open any level that had reflections | Lee |
| 3.81d | An **old level-visuals file force-downgraded the whole level to LOW** | load a pre-2024 level | Lee |
| 3.82 | **Honour the keys a level file states explicitly** — the downgrade still applies to levels that expressed no preference | as above | Lee chose the option |
| 3.83 | **Object Library live preview restored** | hover a thumbnail | Lee: "regression avoided" |
| 3.84 | Preview **lighting, framing and double-object release** | hover in two differently-lit levels | Lee confirmed camera/scale/ambient |
| 3.84b | Preview material + mesh diagnostics (`DUMP_OBJPREVIEW_MATS`) | harness only | n/a |
| 3.84c | **A DBO tangent slot that exists and is full of zeros** | see below | measured, **not yet eyeballed by Lee** |

### 3.84c is the one to read before you ship

The Character Creator zombie's striped vest rendered as a **pure black shirt** in the live preview.
It was not a preview bug. `wickedcalls_part0.cpp:766` pushes a tangent whenever the DBO's tangent
slot EXISTS — the slot's existence is the only test — and that model's slot is filled with zeros.
The engine then generates tangents only `if (vertex_tangents.empty())` (`wiScene_Components.cpp:727`),
so 4298 zero-length tangents are trusted. The TBN collapses, the normal-mapped normal dies, and
N·L = 0 for every light from every direction: uniform, gradient-free, light-immune black.

**So any model in the build with that slot shape has had dead normal mapping everywhere since the
port, not just in previews.** A lit level hides it behind ambient and IBL — the preview's (0,0,0)
ambient is what turned "unshaded" into "black" and made it visible at last. The fix is in the
LOADER, so it applies to every mesh the engine loads.

Measured on the zombie's torso cell: mean **26.4 → 77.9**, p90 **75.2 → 152.9**, and the tangent
stream went from `len=[0.000..0.000] zeroLen=4298` to `len=[1.000..1.000] zeroLen=0`.

**Why it is safe to ship.** The guard only fires when EVERY tangent in the mesh is degenerate, so a
mesh with a real tangent stream is never touched. And if such a mesh also has no UVs — the one case
where clearing the stream leaves the engine unable to regenerate it — the result is an invalid
`vb_tan`, which both `surfaceHF.hlsli:471/513` and `objectHF.hlsli:289` already guard: the mesh
falls back to its geometric normal. That is the same path every mesh with no DBO tangents has
always taken. The worst case is "no normal mapping", never black.

⚠ **The neighbouring gap, recorded not fixed.** There is no equivalent fallback for vertex NORMALS:
`ComputeNormals` is never called from `CreateRenderData`, and the tangent generator itself refuses
to run without normals (`!vertex_normals.empty()`). A model with a present-but-zeroed NORMAL slot
would fail the same way with nothing to catch it. I have not seen one, so I have not written a
speculative fix on the eve of a release — but it is the obvious next instance of the same shape.

---

## 3. Test results

### 3.1 Release gate — fresh launch, 19 demos, CLEAN

| criterion | result |
|---|---|
| C1 LOAD | **PASS** 19/19 reached the editor |
| C2 GEOMETRY | **PASS** POLYS identical to the 0825 reference on all 19 (18 exact, Aztec Teaser in range) |
| C3 VRAM | **PASS** worst 3783.5 MB — Aztec Game Kit *in Test Game* — headroom **312.5 MB** |
| C4 GAME | **PASS** 19/19 produced in-game FPS past the loading overlays |

Pre-registered in `tools/prereg_0923_prealpha.txt` before the run; raw in
`tools/sweep_0923_prealpha_3.84c.txt`. Both predictions settled:

- **3.84c must not move POLYS or VRAM.** It moved neither. That is the test that distinguishes
  "refills the tangent stream" from "allocates a second one", and it is the reason the fix is safe
  to ship on a gate day.
- **3.81d/3.82 MAY move POLYS**, and C2 was amended in writing to allow it. **Nothing moved.**
  Either no shipped demo's `visuals.ini` states a `shaderlevels` key, or the stated value already
  equals what the downgrade produced — I could not determine which, because the `.fpm` archives
  are encrypted and the embedded file cannot be read from outside the app. Either way that change
  is exercised only by USER levels, which is where you confirmed it.

⚠⚠ **The screenshots from this run are worthless, and the check over them passed.** `grab_shot`
looked only in `Max/Files/screenshots` while `SCREENSHOT` writes to `Max/screenshots`, so `ls -t`
matched a stale 2026-09-18 file and copied it out 38 times. My blank-frame check then reported
**0 of 38 blank** — it had checked one good old frame thirty-eight times. What caught it was a
coincidence too strong to be real: four different levels reported the same luminance std to two
decimals; `md5sum | sort -u` said 1. Fixed both halves — search both directories newest-first, and
refuse a shot older than the run start with a printed reason, so a missing screenshot reads as
missing rather than as a pass. C1–C4 never used the shots, so the verdict stands.

### 3.2 Single-session soak — 19 demos, ONE launch, each into Test Game

19/19 loaded · 19/19 reached Test Game · **0 restarts** · **0 blank frames** (38 genuinely distinct
shots this time, verified) · ended back at the hub. Raw in `tools/soak_0923_prealpha.txt`.

| | 09-19 | 09-23 |
|---|---|---|
| peak editor VRAM | 5743.3 MB | **4354.1 MB** |
| loads over the 4096 MB floor | 16/19 | **2/19** |
| mean editor VRAM | 4473.3 MB | **3613.4 MB** |
| accumulation slope | — | **+7.9 MB per load, r² = 0.02** |

r² = 0.02 means load order explains essentially none of the VRAM. The two loads still over the
floor are The Mystery of Z Island (4354.1, load 18) and Operation Amazon (4175.7, load 6).
**On a cold load — what a player does — every demo is under, with 312.5 MB to spare.** The gate
script scores the soak as C3 FAIL because it applies the same 4096 limit; that is the known
long-session behaviour from §3.1 of the 09-20 report, and it is one demo better than it was.

### 3.3 ★★★ Test Game leaves the quality preset applied for the rest of the session

**Reproducer, three steps, silent:**

1. Open any level, hit **Test Level**, press **ESC** back to the editor.
2. Load a different level.
3. It renders with up to **35% of its triangles missing**, permanently for that process.

Measured on Operation Amazon: **486602 → 315943** triangles, scene objects **1850 → 1671**. It is
not substituting cheaper geometry — **179 objects are never created**, averaging ~950 triangles
each, which is the size of a tree.

**Root cause, named.**

```
M-GridEdit_part2.cpp:1045   if (pref.iTestGameGraphicsQuality != 2)
M-GridEdit_part2.cpp:1046       SetGlobalGraphicsSettings( pref.iTestGameGraphicsQuality );
...
M-GridEdit_part2.cpp:1790   //PE: restore SetGlobalGraphicsSettings here. unless changed with
                            //    tab tab, then they are changed in gamevisuals
```

**The restore is a comment.** It sits in the middle of a ~120-line block that restores every other
field one at a time from `t.gamevisuals`, and it is the one setting that needs a *function call*
rather than a field copy — so it is the one that never got written. Grass alone is handled
properly, saved at line 1043 and restored at 1858; trees and terrain have no equivalent.
`GGTrees_SetPerformanceMode` then leaves `ggtrees_global_params.lod_dist` at the Test Game
preset's value, and every level loaded afterwards hands its trees to billboards from there.

Measured directly, via `SET_TREEPOOLCAP`'s reply and `DUMP_TREEPOOL`:

| | lod_dist | pool built | renderable |
|---|---|---|---|
| healthy | 3000 (HIGH) | 488 | 295 |
| after one Test Game round trip | **1000 (LOW)** | 116 | **101** |
| …then pool uncapped | 1000 | 6000 | 6000 |

★ **The tree pool was a red herring and the last row is what proves it.** Uncapping built 6000
pooled trees and POLYS did not move one triangle — the pool was never the limiter, the LOD
distance was. I would have "fixed" the wrong subsystem if I had stopped at the plausible story.

**Who this bites.** The apply is gated `!= 2` and the shipped default is High (2), so a default
install never triggers it. It fires only when *Graphics Quality (Test Game)* is set below High —
which is exactly what a low-spec tester does first. The preference lives in
`%LOCALAPPDATA%\TGC\gamegurumax.pref`, **outside the build area**, so testers get the default and
inherit nothing from us.

⚠ **This machine has it at Low**, which is why the soak reproduced it. So tonight's *soak* POLYS
figures are LOW-preset numbers, not what a default install produces. The **gate sweep is
unaffected** — it relaunches MAX per demo, so the preset never carries.

**Why it went unnoticed for months.** The fresh-launch gate relaunches per demo and can never see
it. The soak *did* see it, on 09-19 and again tonight, bit-identically — and it was written off in
`demo_soak_sweep.sh` as the tree pool "not finished populating". That explanation is refuted: the
warm run settles *longer* than the cold run that reads higher (60 s + 3 samples 15 s apart vs 30 s
+ 4 s apart), and all three samples are identical in both — `486602/486602/486602` cold,
`315943/315943/315943` warm. Nothing was settling. ★ **A plausible explanation written into a
script's comments stops anyone from measuring it again.** I have corrected that comment.

**This is the third defect found at that one call site.** 2.39 stopped the same preset stamping
the level's authored shadow settings the moment a test game started. Same line, same cause: an
apply with no matching restore.

**I have not fixed it**, deliberately. The fix is small — mirror what grass already does, or
re-assert the editor's quality on return — but it changes the editor↔game transition that both
sweeps run through, so landing it on release morning means either shipping ungated code or
re-running 2.5 hours of sweeps. It is the first thing I would do after the pre-alpha is out.

**Workaround for testers, worth putting in the notes:** if the editor starts looking sparse after
testing a level, restart MAX — or leave *Graphics Quality (Test Game)* on High.

---

## 4. Before you zip

### ★★★ 4.1 `tools/prealpha_clean.sh --apply`

Still the one blocking recommendation, and the pile has grown: **1377.9 MB across 706 files** now,
against 426 MB across 208 on 09-20. Almost all of the growth is my own sweep screenshots.

One **arming file** remains, `dred.txt` — down from two; `gg_atlas_trace.txt` is gone. My
recommendation on `dred.txt` is unchanged from 09-20: **keep it for a pre-alpha.** Since 3.69
guarded the walk, an armed DRED turns a tester's device removal into a real report rather than a
shrug. The script lists arming files separately from debris so the decision stays yours.

⚠ Every VRAM and FPS number in this repo, tonight's included, was measured with DRED **on**. If a
tester's numbers come back different, that is one of the reasons.

`setup.ini` is already correct (`producelogfiles=0`, `memgeneratedump=0`) and `GameGuruMAX.pdb`
should ship so tester crash logs symbolise. Four things are listed but untouched for you to judge:
three loose `*_surface.dds` at `Files/` root with no copy in entitybank, and `Files/savegames/`
(276 KB of test saves a fresh install probably should not carry). Also still there: five loose
`.sh` test scripts from Feb–Mar 2026 beside the exe, and `Files/particlesbank_old/` at 20 MB.

### 4.2 What to add to the known-issues list

Everything in §4.2 of the 09-20 report still stands. Two additions from this week:

- **The live preview does not write thumbnails to disk.** Hovering works; *generating* a thumbnail
  for imported or user content does not, because it needs a GPU→CPU readback that DX12 does not
  have yet. Stock content ships its thumbnails, so this only bites imported models.
- **Snapshot mode and the particle thumbnail grabber are disabled in DX12** for the same reason.

### 4.3 What I did not establish

- **3.84c has not been eyeballed by Lee.** The measurement is unambiguous and the render matches
  the DX11 thumbnail, but it changes how *every* mesh in the build is normal-mapped, and no
  automated check can tell you a surface now looks *right* rather than merely *different*.
- **How many models carry the zeroed tangent slot.** I found one. Counting them means parsing
  every DBO in the build, which I have not done.
- **Export Game** remains the least-tested path, untouched since 08-16.

---

## 5. Still open

Carried from 09-20 unless marked new.

1. **Standalone holds 950–1520 MB more than Test Game** on the same level (09-20 §3.6). Untouched
   tonight. Still the reason the §3.1 headroom is an *editor* figure.
2. **Export Game** — least-tested path in the build, untouched since 08-16.
3. **`visObj=7`** *(new)* — the object-library preview camera reports seven visible objects where
   subject + backdrop is two, on the first hover of a fresh launch, so it is not the object-release
   bug 3.84 fixed. Recorded, not understood. No visible symptom.
4. **Thumbnail generation to disk** *(new)* — needs a GPU→CPU readback. The preview render target
   is now exactly what such a readback would read, so the groundwork is done.
5. **A present-but-zeroed vertex NORMAL slot would fail like 3.84c with nothing to catch it**
   *(new)* — `ComputeNormals` is never called from `CreateRenderData`, and the tangent generator
   refuses to run without normals. I have seen no instance, so I have not written a speculative
   fix on release day. It is the obvious next member of the family.
6. **How many models carry the zeroed tangent slot** *(new)* — I found one. Counting them means
   parsing every DBO in the build; the `.fpm` and model archives are encrypted, so this needs to
   be done from inside the app.

---

## 6. What tonight did not establish

- **That the build looks right.** Both sweeps check that it loads, runs, draws the expected
  geometry and fits in memory. Neither can tell you a surface is shaded correctly, and 3.84c
  changed shading everywhere.
- **Anything about FPS across days.** 4.2 h uptime, no reboot. The FPS columns are within-run
  comparisons only (§3.24b).
- **Standalone or Export Game.** Neither sweep touches them.
