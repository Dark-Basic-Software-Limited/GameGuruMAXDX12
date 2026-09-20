# Numbered chronological checklist — everything since November 2025

Every change you can actually **reach and exercise** in the current DX12 build, in the order it was
made. Fork point `ca32a143`, **2025-11-21**. Sources: 338 DX11 commits and 847 DX12 commits (217 of
them numbered GGMAX changes), filtered to what a person can go and try.

**Dates are when the change was made**, so DX11-origin items sit at their DX11 date even though
DX12 ported them later (most in the 3.44–3.50 parity round, 2026-09-16/17).

| mark | meaning |
|---|---|
| **L** | Lee confirmed by hand at the time |
| **S** | the automated 19-demo sweeps exercise it every run |
| **U** | **never exercised by anyone** — shipped, built, not regressing, but untried |
| **11** | originated in DX11 and was ported |

> ⚠ Note on DX12 dates: the port's own work starts **February 2026**. Between the fork and then,
> DX12 had three refactor commits and nothing checkable, while DX11 carried on shipping features —
> which is why December and January below are almost entirely DX11-origin.

---

## December 2025 — the big DX11 feature and fix wave

| # | what to check | mark |
|---|---|---|
| 1 | `GetSunColorRed()` / `GetSunColorGreen()` / `GetSunColorBlue()` return the sun colour from Lua | U 11 |
| 2 | Install and uninstall **custom fonts** from Game Settings | U 11 |
| 3 | A **non-looping storyboard video** links to another screen or level when it finishes | U 11 |
| 4 | Button fonts stay legible on a **widescreen** monitor | U 11 |
| 5 | Loading a level whose **group was deleted** no longer crashes | U 11 |
| 6 | **Game Settings opens** with the Object Tools tab already open (6174) | U 11 |
| 7 | No heap corruption when **loading a game** | U 11 |
| 8 | A **newly added object is not locked** | U 11 |
| 9 | **SSAO Power** accepts negative values to darken the effect (6232) | U 11 |
| 10 | No flash of the **previous level's loading screen** when moving to the next (6207) | U 11 |
| 11 | `wpe_area.lua` honours **X and Z offsets** | U 11 |
| 12 | The **WPE particle preview** honours X and Z offsets | U 11 |
| 13 | More **PNG→DDS conversions** happen automatically (saves video memory) | S 11 |
| 14 | Terrain does not **freeze the app** under lock contention (recursive_mutex) | S 11 |
| 15 | New behaviour: **`wpe_activator`** | U 11 |
| 16 | **Entity Sound Slot 4** works from Lua — plays slot 4, not 5 | U 11 |
| 17 | New behaviour: **`entity_in_zone`** | U 11 |
| 18 | Dragging an object **retains its Y relative position** | U 11 |
| 19 | **Bullet holes no longer stick to doors** unless opted in (6236) | L 11 |
| 20 | New per-object setting: **"Allow Bullet Holes ?"** (6215) | L 11 |
| 21 | **Nav mesh system** improvements — ⚠ see item 199, DX12 reverted the high-quality half | — 11 |
| 22 | New behaviour: **`dispenser`** | U 11 |
| 23 | A **custom HUD does not transfer** into the next level (6252) | U 11 |
| 24 | **Doors keep their collision on the second level** (6246 / 6183) | U 11 |
| 25 | **`GetPlrLookingAt`** is reliable (6253) | U 11 |
| 26 | **Lights do not vanish** after visiting the graphics menu (6256) | U 11 |
| 27 | **Custom explosions reach standalone** (6257) | U 11 |
| 28 | **Title screen works in the project system** (6188) | S 11 |
| 29 | **Start Marker underwater** — ⚠ you now start underwater instead of being pushed to the surface | U 11 |
| 30 | **Music volume** is respected | U 11 |
| 31 | Hub tweaks | S 11 |
| 32 | New behaviours: **`deployable`** and **`deployable_named`** | U 11 |
| 33 | **Delayed Shot** — bow/crossbow hold the shot until the release frame | U 11 |
| 34 | **`animchoicemode`** GUNSPEC field — modes 1 and 2 were unreachable before | U 11 |
| 35 | **Keys taken from a chest unlock their door**; dropping one re-locks it | U 11 |
| 36 | Crash log system improved — a crash writes a usable `Guru-Crash.log` | U 11 |
| 37 | New behaviour: **`teleport_node`** | U 11 |

## January 2026 — DX11 fixes, zones and diagnostics

| # | what to check | mark |
|---|---|---|
| 38 | New behaviours: **`prefill_zone`** and **`refill_zone`** | U 11 |
| 39 | New behaviour: **`process_zone`** | U 11 |
| 40 | **Grid settings below 1.0** stick, including across a restart (6278) | U 11 |
| 41 | **Emissive Strength updates live** in the editor (6282) | U 11 |
| 42 | **Save/Load menu on ultrawide** — one click selects one slot (6283) | U 11 |
| 43 | **Wicked impact particles face the camera** | U 11 |
| 44 | **Door collision on the second level** — the script-side half (6246) | U 11 |
| 45 | **Ctrl+Z does not retain memory** across a level change (6288) | U 11 |
| 46 | **Melee custom fpe decal impacts** do not trigger on a miss | U 11 |
| 47 | **Additive blending survives test/standalone** (AvengingEagle light effects) | U 11 |
| 48 | **Standalone loading is faster** (6286) — ⚠ DX12 has the cache-clearing half only, see item 200 | — 11 |
| 49 | **Storyboard HUD "Use Square Grid"** (4331) | U 11 |
| 50 | **Material Type (for sound)** in Material Properties (2455) | U 11 |
| 51 | **`justgrass` flag** — skip the grass system entirely | U 11 |
| 52 | **`version.ini` is copied for standalones**, and features can be disabled in MAX | U 11 |
| 53 | Crash logging can be **turned off** for performance | U 11 |
| 54 | Lua **runtime errors produce a crash log** (previously this class produced none) | U 11 |
| 55 | Terrain **GPU race condition** hardening | S 11 |
| 56 | **TerrainReadBack** buffer resource handling | S 11 |

## February 2026 — the DX12 port starts: terrain, animation, the editor comes back up

| # | what to check | mark |
|---|---|---|
| 57 | **New Wicked terrain is the default**; `Y` toggles the old one | S |
| 58 | **Painted terrain materials** render (4-layer, via the VT pipeline) | L |
| 59 | **Blendmap painting covers the whole terrain**, not a quarter | L |
| 60 | Painted chunks show their **real textures immediately**, not defaults until you walk closer | L |
| 61 | **Grass and tree reference overlays** on terrain (debug visualisation) | L |
| 62 | **Character animation works** — skinning, head/spine tracking, no sideways characters, no 90° snap at walk→idle, no root-motion drift | L |
| 63 | **Lighting brightness matches DX11** | L |
| 64 | **Editor mouselook** does not snap straight up | L |
| 65 | **Entity thumbnails** present for the 58 assets with .dds/.png thumbs | L |
| 66 | **Screen editor** saves, captures thumbnails, and shows the Title Screen thumb | L |
| 67 | **Storyboard thumbnails** refresh when you switch projects | L |
| 68 | Changing the **splash screen image** does not cause a device-removed crash | L |
| 69 | Opening the **performance panel in test game** does not TDR | L |
| 70 | Old **DX11 particle effect files** load without message-box errors | L |
| 71 | New behaviour: **`global_check_zone`** | U 11 |
| 72 | **Cover and prone attack sounds** | U 11 |
| 73 | DX11: `GGTrees.cpp` update | — 11 |

## March 2026 — terrain scale, animation culling, the panel

| # | what to check | mark |
|---|---|---|
| 74 | **Terrain chunk scale** matches the Wicked 1 m convention; generation popping is near-invisible | L |
| 75 | **Terrain texture corruption** after a long camera excursion is fixed | L |
| 76 | **Animation culling** — Update-Wicked 25.8 → 11.4 ms | L |
| 77 | **Performance Data Panel** works | L |
| 78 | DX11: **Controller fix** — and `ForceMouseXYClick` added to Lua | U 11 |
| 79 | New behaviour: **`action_talk`** | U 11 |

## April 2026 — terrain transitions

| # | what to check | mark |
|---|---|---|
| 80 | **Terrain texture transitions** between materials | L |
| 81 | **Animation system fix** | L |
| 82 | New behaviour: **`prompter`** | U 11 |
| 83 | New behaviour: **`view_activator`** | U 11 |
| 84 | DX11 April fixes batch | — 11 |
| 85 | Updated **behaviour icons** throughout | S 11 |

## May 2026 — grass arrives

| # | what to check | mark |
|---|---|---|
| 86 | **Grass renders** without the VRAM crash; golden-wheat look | L |
| 87 | Live **`SET_GRASS`** tuning | L |
| 88 | DX11: **remote-project thumbnails** — entities show the project's own thumbs | U 11 |
| 89 | DX11: **UI stays on screen** on a non-primary monitor (6347) | U 11 |
| 90 | DX11: **loading a save then hitting the winzone** advances instead of repeating (4909) | U 11 |
| 91 | DX11: **custom decals are case-insensitive** | U 11 |

## June 2026 — grass becomes a real system

| # | what to check | mark |
|---|---|---|
| 92 | **Painted grass renders** with the correct per-type DDS from grassbank | L |
| 93 | **Brush cursor ring** on the terrain, and it self-heals after a scene wipe | L |
| 94 | **Grass paint applies in place** — per-chunk dirty, stable strand count | L |
| 95 | **Grass size matches DX11** (2×, FP32 positions for smooth sway) | L |
| 96 | **Flower category** silhouette matches DX11 | L |
| 97 | **Per-strand paint-mask visibility** — grass appears exactly where painted | L |
| 98 | **Grass Draw Distance slider**, decoupled from the LOD ring | L |
| 99 | **Advanced Grass Settings** wired: Clear/Populate, per-strand slope and altitude filters | L |
| 100 | DX11: **no terrain update during the standalone saver** (faster exports) | U 11 |

## July 2026 — trees, shadows, water, streaming

| # | what to check | mark |
|---|---|---|
| 101 | **Real tree meshes** — trunk + branches per type, all 38 | L |
| 102 | **Tree positions match DX11** | L |
| 103 | **Distant treeline** — alpha-test mip erosion fixed, pool 20K | L |
| 104 | **Tree paint latency** — no 3–5 s delay after painting trees | L |
| 105 | **Leaf colour passes through as authored** (no permanent green tint) | L |
| 106 | **Entities load** — the .ele v342 record round-trips | L |
| 107 | **"Delayed Shadows" checkbox** is wired to the engine; shadow flicker fixed | L |
| 108 | **Staggered shadow-cascade refresh** (DX11 parity) | L |
| 109 | **Shadow distance parity** — production 5-cascade splits, billboard shadow proxies | L |
| 110 | **Tree shadow LOD distance and range sliders**, in Visuals ▸ Shadows, live in test game, persisting with the level and surviving the test-game quality preset | L |
| 111 | **Water height** works (whole custom frame CB was reading 0) | L |
| 112 | **Water ripples** — engine-native, half-strength distortion, tuned footprint | L |
| 113 | **Light Shafts** — sun-mask tamed, DX11 exposure parity | L |
| 114 | **Terrain sculpt/paint bridge** — five review-confirmed defects fixed | L |
| 115 | **Grey-terrain regression** fixed; VT working set expanded | L |
| 116 | **"Exploded parrot"** skinned-model corruption on level load fixed | L |
| 117 | Saving a level with **smart-object groups** does not crash | L |
| 118 | **Frustum / Apparent Culled** readout in the editor is live, not hardcoded 0 | L |
| 119 | **Texture streaming is ON by default** — the load crash is root-caused and fixed | L |
| 120 | **Normal-map UV-swimming flicker** fixed (DBO tangent handedness) | L |
| 121 | **Character dedicated sun shadows** — up to 3 high-res per-character slots | L |
| 122 | **Post-load FPS dip** fixed | L |
| 123 | DX11: **SPRITELIB** sprite API | U 11 |
| 124 | DX11: **resource node** behaviour | U 11 |
| 125 | DX11: `lightingHF.hlsli` update | — 11 |

## August 2026 — parity, particles, performance, the low-spec campaign

| # | what to check | mark |
|---|---|---|
| 126 | **Spot cone angle** correct (the port renamed fov→outerConeAngle without halving) | L |
| 127 | **Sculpt-grass** — grass records survive in-place chunk regeneration | L |
| 128 | **Selection outline RESTORED** | L |
| 129 | **PLAY GAME device hang FIXED** — the outgoing terrain texture set is retained across the swap | L |
| 130 | **Standalone save games** work (Lua 5.4 float filenames vs integer readers) | L |
| 131 | **Shadow Quantity knobs un-crossed** — spot count and resolution live again | L |
| 132 | **Light radius** and **shadow resolution** respond | L |
| 133 | **Navmesh cache** prunes at 48 files / 512 MB | L |
| 134 | **Hair rendering** matches DX11 | L |
| 135 | **First-person weapon depth carve** — no clipping through your hands | L |
| 136 | **Light power parity** with DX11 (energy 30) | L |
| 137 | **gpup / `.arx` legacy particles RE-ENABLED** — steam markers live again | L |
| 138 | gpup: no **~25 s particle snap**, no **camera-provoked shape shift**, no **"ping"**, no **white-out** | L |
| 139 | **WPE particles** — invisible-effect bug fixed | L |
| 140 | **Merged grass** — both bugs fixed, −2005 MB | L |
| 141 | **Weapon shadow** pull wired up | L |
| 142 | **Completely Empty Level** reachable on any level, and restores on untick | L |
| 143 | **Switch Escape performance** — single-queue submission, +2.0% to +18.6% on 6 demos | L |
| 144 | **Zoomed firing does damage** (the weapon mesh was swallowing the bullet ray) | L |
| 145 | **Tutorial videos play**, and the streaming video ring cut the cutscene tax (Grand Canyon intro 47 → 61 FPS) | L |
| 146 | **Terrain Generator**: preview restored, Generate does not crash, pinned to the editable-area marker, wipes leftovers on entry, wide chunk ring (5 km), grab/hover confined to the handle, **biome buttons live**, no grass in the generator | L |
| 147 | **Screen (HUD) editor images draw** — no image ever drew on DX12 before this | L |
| 148 | **Deleted or collected objects do not leave their shadow behind** | L |
| 149 | **Quality presets stop stamping over the level's authored shadow settings** | L |
| 150 | **Explosion refraction squares** fixed (DX11 distortion texture slot restored) | L |
| 151 | **WPE rotation speed** matches the fork | L |
| 152 | **Female head surface** roughness corrected | L |
| 153 | **Lazy tree-pool growth**, **tree pool released on a sustained park**, **per-level tree-type assets** — big level-change leak closed | L |
| 154 | **Decal pool** prewarm and grow | L |
| 155 | **Env probe** work: re-bake, the "circle on each cube face" root-caused and fixed (fp16 parallax overflow), matte `%probe` marker ball, large accurate preview sphere, probe pick no longer resizes the balls, **Probe Brightness restored and Range removed** | L |
| 156 | **Terrain roughness dry-look floor** — 11 glossy materials lifted | L |
| 157 | **DDS conversion milestone** — 1641 files converted to full mip chains | L |
| 158 | **`producelogfiles=0`** keeps diagnostic trace files out of players' folders | L |
| 159 | **Standalone export ships the splash screen and FSR shaders** | L |
| 160 | **Terrain Generator marker** release leaves the terrain alone; recentre folds into Generate | L |
| 161 | **Camera glide** on release (pan only) | L |
| 162 | **Water slider** debounced — 4.3 → 84.5 FPS, black band gone | L |
| 163 | **Hidden terrain stays hidden** across chunk rebirth | L |
| 164 | **Generator Vegetation checkbox** live; RMB look pins the pointer | L |
| 165 | **Four brutal terrain off-switches** — Aztec editor 63 → 167 FPS | L |
| 166 | **Far-tree billboards** — distant forest restored, handover no longer pops, per-level atlas slices, tree pool radius cap | L |
| 167 | **Billboard shade wrap** slider | L |
| 168 | **Object Detail Distance** (works upwards; U-shaped curve) | L |
| 169 | **Texture Detail** applies **live** | L |
| 170 | **Low-spec off-switches**: Post FX, AO, Simple Sky, Shadows | L |
| 171 | **Occlusion Cull Delay**, per level | L |
| 172 | **Shadow Detail Steps** slider | L |
| 173 | **Super Quick Objects** tickbox + rung slider | L |
| 174 | **Terrain Bake** (+ near-tier detail slider) and **Water Bake** | L |
| 175 | **Reduction Scale** for animation | L |
| 176 | **Per-level low-spec switches** persist with the level | L |
| 177 | **Graphics & Performance panel**: stable row order, call tree, **Hide idle rows** on by default | L |
| 178 | **Door logic gating** — only switch-operated doors stay always-active | L |
| 179 | **Inert-entity skip** — CPU frame −8.7% | L |
| 180 | **Per-object footfall keyframe cache** | L |
| 181 | DX11: **"Add New Particle"** content and **Particle System Upgrade** | L 11 |
| 182 | **Live Texture Detail no longer races the streaming thread** into a device hang | L |
| 183 | **Tooltips wrap**; Super Quick defaults to 3; baked-water Schlick ramp | L |
| 184 | **Crash path does not use RTTI** — a device loss is reported, not silently closed | L |

## September 2026 — parity round, restoration, VRAM campaign

| # | what to check | mark |
|---|---|---|
| 185 | **Test Level runs** — the Lua 5.4 shim (`math.atan2`, `bit32`, `unpack`, `mod`, `pow`, `log10`) | L |
| 186 | **Entity record parity with DX11** — v342 round-trips, real content loads and plays | L |
| 187 | **Water splash** duration correct (it was emitting for a flat 5 seconds) | L |
| 188 | **Delayed Shadows tooltip** carries the measured numbers and a true claim | L |
| 189 | **Storyboard video auto-advance**, **save-game spawned objects**, **chest keys** — the three half-ports | U |
| 190 | **Lua Sound Slot 4** and six dead knobs (`animchoicemode`, ultrawide click, grid < 1.0, Show Object Debug Visuals, `disablejustgrasssystem`, WPE preview offsets) | U |
| 191 | **DX11 parity backlog cleared** — 13 fixes, 61 edits, 24 files | U |
| 192 | **`bit32` shim** and the **`MoveInventoryItem`** hunk | L |
| 193 | **"Add New Particle"** places a real marker with a real icon | L |
| 194 | **Authored custom-shader parameters are no longer destroyed** on entity selection | L |
| 195 | **The GG render path issues resource barriers** (it never had) | L |
| 196 | **Second level loads** — a treeless first level had poisoned the billboard atlas for every level after it | L |
| 197 | **Level-swap mesh/material leak** closed (tree TYPE assets) | L |
| 198 | **Bullet tracers restored**, and ownerless tracers fired from behind the camera are culled | L |
| 199 | **Tree sway restored** — vegetation via engine per-vertex wind, entity trees via per-material "Object Wind", **Wind Gust Size** mapping inverted and re-floored | L |
| 200 | **Rain and snow are now WPE particle effects**, and newer `.PE` files load | L |
| 201 | **VRAM retained across level loads fixed** — stale terrain paint map, orphaned material entities, `GPUP_DeleteTexture` stub, depth-chain keep-alive. +90.1 → +5.2 MB per load | L |
| 202 | **Shadow packer / grass cache / MSAA outline targets** — accumulation to +9.9 MB per load, peak 5743 → 4350 MB | S |
| 203 | **The shadow atlas shrinks** when a light level follows a heavy one (272.6 MB → 17.0 MB) | S |
| 204 | **The crash reporter survives a malformed DRED output** | U |
| 205 | **Standalone PLAY GAME loads a level and plays it** — ⚠ verify a **real mouse click** starts it; only a direct Lua call worked for me | U |

---

## Not in this build — don't check for these

| # | what | why |
|---|---|---|
| 206 | **Nav-mesh high-quality extraction** (DX11 `15cbd06c`) | ported byte-identically, then reverted — it froze the main thread 60+ s on a large map under the DX12 terrain system |
| 207 | **Standalone nav-mesh vertex cache** (DX11 `fd09784f`) | DX12 clears the cache but does not yet fill it, so standalone level loads rebuild the triangle soup |
| 208 | **"Water", "Glass", "Tree Animate Doublesided"** material shaders | their bodies are not in the build; the `.cso` files are byte-identical to stock |
| 209 | **Custom shader parameters 1–7** | the engine replaced seven floats with `uint4 userdata` and the migration is unfinished. No shipped `.fpe` uses them |
| 210 | **PP Snow / Dust, transparent shadows, bloom strength** | the engine APIs no longer exist. Values still save and round-trip |
| 211 | **Level-load fade-in from black** | the ramp is intact and drives nothing; DX12 shows the construction artefacts DX11 hid |
| 212 | **PIX marker on/off, `SetSpecialGGDebugLog`** | no equivalent API; DX12 emits PIX events unconditionally |
| 213 | **`terrainsleep` lock restructure** | DX11 walked this back itself — DX12 already compiles what DX11 compiles today |

---

## Where I'd start, if you only have an hour

The **U** marks are the whole point of this list — 52 of them. In descending order of what I'd
expect to find something:

1. **#205 / Export Game** — standalone is the least-tested path in the build, and it is where I
   found the +1.5 GB texture residency issue last night.
2. **#35 chest keys, #33 Delayed Shot, #34 `animchoicemode`, #16 Sound Slot 4** — all four were
   genuinely unreachable before, so nobody has ever seen them work.
3. **#37–#39, #71, #79, #82–#83, #15, #17, #22, #32 — the eleven new behaviour scripts.** All
   present on disk, none ever run.
4. **#40 grid below 1.0** and **#45 Ctrl+Z across a level load** — both had multiple places that
   undid the fix, so both are the kind that quietly comes back.
5. **#189 storyboard video auto-advance** and **#47 additive blending in Test Game** — two where the
   symptom is obvious the moment you look at it.
