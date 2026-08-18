# Pre-alpha readiness — dead-control audit + recommendations (2026-08-18)

Lee, ahead of the first tester alpha: *"run a full hub demo sweep to make sure all is well.
We can then hunt down any last remaining 'buttons that do not work' before creating our
first alpha build to send to testers. Any recommendations you want to make before we
produce this version feel free to make as a report."*

Build audited: engine `bde205ad` (2.90) / game `26482fe6`.

---

## 1. Hub sweep — CLEAN

19/19 demos loaded, zero failures, **POLYS bit-identical on all 19**, 4 GB gate holds
(worst editor VRAM 3586 MB). Full table + the FPS caveat in `DEMO_FPS_SWEEP.md` §0818b.

⚠ The headline number is a trap worth repeating here: against the 0816 baseline the sweep
reads −13.4% across *every* demo. That is machine drift, not code — **this morning's 2.89
run, before 2.90 existed, was already −11.2% below 0816**. Isolated against that same-day
control, 2.90 is −2.5% hub-wide with two demos up.

---

## 2. Dead-control hunt — method and results

The 2026-07-28 audit inventoried 330 widgets and traced each binding to a consumer. That
method catches a **severed chain** (control → … → nothing). It cannot catch a **clobbered
value** (control → field → something overwrites the field every frame), which is exactly
how Probe Range hid for the entire life of the port and through that audit. So this pass
ran three scans, deliberately including the class the last one was blind to.

### Scan A — clobbered values (the Probe Range class) — **CLEAN**

Enumerated every component field that `Scene::Update`'s systems recompute each frame
(`probe.position/range/inverseMatrix`, `decal.world/range/color/…`, `object.center/radius/
lod/fadeDistance/sort_bits/mesh_index`, `light.occlusionquery/maskTexDescriptor`), then
checked what the game writes into any of them.

| Field | GG writes | Verdict |
|---|---|---|
| `probe->range` | yes | **was the bug** — UI removed in 2.90 |
| `light->range` | yes | safe: `RunLightUpdateSystem` does **not** recompute it |
| `decal->range` | none | — |
| `->center` `->radius` `->lod` `->fadeDistance` `->sort_bits` `->inverseMatrix` `->occlusionquery` `->maskTexDescriptor` `->mesh_index` | **0 writes each** | — |

No second instance of the class exists. That is a genuinely reassuring negative.

### Scan B — widgets whose click body is empty — **no real defects**

55 raw hits, all resolved:

- **14 are commented out** — the 07-28 audit's correctly-hidden set (Bloom Strength,
  Transparent Shadows, Water Fog ×3, PP Snow/Dust, Disable When Indoor, PP Alpha, voxel
  steps, 7 custom-shader-param sliders). Invisible to testers. Correct as-is.
- **The rest write their bound variable directly** and assign through on the next line —
  the house style (`bool bSpray = …; if (Checkbox(&bSpray)){} t.gridedit.entityspraymode =
  bSpray;`). All verified to have a real consumer.
- Two genuine no-consumer cases, both benign:
  - `fFloatSliderTest` (`M-GridEditB_part14.cpp:430`) — a copy-paste template slider inside
    a "Template Window" gated behind `bTemplate_Window = false` + an early `return`. Not
    user-reachable.
  - `texture_set_selection` (`M-EBE_part0.cpp:1531`) — the Easy Building Editor's
    "Texture Set" combo. Has exactly one item ("Default") so nothing can be selected, but
    its tooltip says "Select Texture Palette", implying more. Cosmetic only.

### Scan C — menu items with unhandled clicks — **0 found**

Every `ImGui::MenuItem` in the editor is handled.

### The one real find: a no-op bridge trio

`WickedCall_CreateReflectionProbe` / `MoveReflectionProbe` / `DeleteReflectionProbe`
(`wickedcalls_part3.cpp:1538-1552`) are **empty**, and the author's own comment asks *"no
code was here, can we delete all calls to this function?"* — yet the game still calls them
from 8 sites: the asset/model preview panels (`M-GridEditB_part6/7/9`, the `"editorProbe"`)
and one per-entity probe in `G-Entity_part3.cpp:558`.

This is a quietly absent feature rather than a visible dead button: the editor's preview
viewport has no dedicated reflection probe on DX12. Low tester impact, but it should either
be implemented or the calls removed so it stops looking like working code.

---

## 3. Recommendations before cutting the alpha

**Do before shipping**

1. **Decide which `setup.ini` ships.** The build folder currently has `producelogfiles=1`
   and `profileinstandalone=1`. `producelogfiles=1` is arguably *right* for an alpha (you
   want tester logs) but it re-enables every routine trace writer the 2.71 gate was built
   to silence — confirm that is intended and that log growth is bounded over a long
   session. `profileinstandalone=1` leaves debug options inside the test game; exported
   standalones get `0` written explicitly (`M-MapFile_part2.cpp:215`), so this only affects
   whoever runs the editor — which is exactly who your testers are.
2. **Your test level authors Probe Brightness = 10.** That value was inert before today and
   is live from 2.90. Any level carrying a non-1.0 value will look different in the alpha
   than it did last week. Worth a pass over the shipped demos for probe markers with
   non-default brightness before testers see them.

**Worth considering**

3. **Close the mip-0 brightness gap.** The restored slider covers filtered mips 1..N-1 only;
   mip 0 is an unfiltered `CopyResource`, so mirror-sharp reflections ignore it. DX11 had
   the same gap, but it measurably caps the knob on exactly the shiny props an artist would
   use it for (the metal barrel moved only +3.1 luma across a 25× sweep). Fix is one scale
   pass over `envrenderingColorBuffer` mip 0 before `GenerateMipChain`.
4. **Resolve the reflection-probe trio** above — implement or delete.
5. **Populate or hide the EBE "Texture Set" combo** (one item today).

**Explicitly NOT recommended before the alpha**

6. ~~**Don't revisit texture streaming (#37).** It is default-OFF because it crashed Trapped
   and RPG Template.~~ **CORRECTION (same day, after Lee asked me to confirm streaming
   status): this was WRONG and the error was mine.** Texture streaming is **ON by default
   and has been since 2026-08-01** — `g_bTextureStreamingEnabled = true`
   (`wickedcalls_part0.cpp:350`), and per WETEST.md it "briefly defaulted 0 on 2026-08-01
   while the load crash was open, restored the same day by delta 1.73". The crash was fixed
   (task #47, "verify all 19 demos load clean with streaming ON"), not worked around by
   disabling the feature. My MEMORY note carried the one-day state as if permanent and I
   repeated it here without checking. Streaming needs no decision before the alpha — it is
   already the shipping behaviour and the 19/19 sweep above ran with it on. See
   `STREAMING_STATUS_2026-08-18.md`.
7. **Don't chase the hub-wide FPS number.** It is ambient drift; the same-day control proves
   it. Chasing it would burn the alpha window on a non-bug.

**Known-open items testers may hit** (so they can be listed as known issues rather than
arriving as surprise reports)

- Env-probe capture: my notes disagree with themselves on whether the 2.77 "probe
  photographs its own root shell" defect is still live. `WickedCall_MakeObjectEnvMatte`
  excludes only the frames of the **one** object id it is given, and a `%probe` marker is
  known to be more than one GG object. Today's local-probe cube rendered clean on the debug
  sphere (sky, palms, no portholes), so it does not present on that level — but this wants
  one deliberate eyeball on a freshly placed probe before you call it closed.
- Point-light cube shadow faces pack small (task #107) — quality, not correctness.
- Light-power parity (#109) and gpup particle parity (#118/#122) are shipped but have never
  had your eye on them; testers will be the first to judge.
