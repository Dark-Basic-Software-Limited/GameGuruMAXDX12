# Everything that changed since November 2025

Two documents in one, because they come from the same list:

- **Part 1** is a walkthrough for Lee — every change you can actually *reach* in the app, ordered by
  where you are when you'd reach it, with what to do and what you should see.
- **Part 2** is the same material rewritten for alpha testers, with the internals taken out.
- **Part 3** is what to tell testers *not* to report.

**Scope:** the DX12 fork point is `ca32a143`, **2025-11-21**. Since then DX11 has 338 commits and
DX12 has 847, of which 217 are numbered GGMAX changes. This covers both: what DX11 added and DX12
ported, and what DX12 built on its own.

### How each item is marked

| mark | meaning |
|---|---|
| **[Lee]** | you confirmed it by hand at the time |
| **[swept]** | the automated 19-demo sweeps exercise it every run (it loads, it renders, it plays) |
| **[untested]** | shipped, built, shown not to regress — **but no human or harness has ever exercised the behaviour**. These are where your time pays. |

---

# PART 1 — Manual walkthrough

## 1. Hub, projects and storyboard

| ✔ | what | how to exercise it | expect |
|---|---|---|---|
| ☐ | **Storyboard intro video auto-advance** **[untested]** | link a **non-looping** splash video to a screen, run it | it advances itself at the last frame — previously it sat on the frame forever and Escape wouldn't skip |
| ☐ | **Title screen in the project system** **[swept]** | open any demo's storyboard, run it | title screen appears and works (DX11 6188) |
| ☐ | **Storyboard HUD "Use Square Grid"** **[untested]** | Storyboard Editor → HUD grid options | square grid option present and applies (DX11 4331) |
| ☐ | **Remote-project thumbnails** **[untested]** | create a remote project, look at entity thumbs | thumbs are the project's own, not stale/shared (DX11 Discord issue) |
| ☐ | **Hub demo browsing** **[swept]** | Demo Games tab, all 19 | every demo selects, opens and loads — 19/19 three times last night |
| ☐ | **Boot splash in standalone** | export/run standalone | GameGuru splash, not the debug boot screen (2.69) |

## 2. Editor — placing and editing objects

| ✔ | what | how to exercise it | expect |
|---|---|---|---|
| ☐ | **Grid size below 1.0** **[untested]** | set grid to 0.5, then **restart MAX** | still 0.5 after restart. Three separate places used to clamp or reset it (DX11 6278) |
| ☐ | **Drag retains Y relative position** **[untested]** | drag an object that sits above ground | it keeps its height offset instead of snapping |
| ☐ | **Newly added object is not locked** **[untested]** | place a fresh object, try to move it | moves immediately |
| ☐ | **Load a level whose group no longer exists** **[untested]** | a level referencing a deleted group | loads instead of crashing |
| ☐ | **Game Settings with Object Tools open** **[untested]** | open Object Tools tab, then Game Settings | Game Settings opens (DX11 6174) |
| ☐ | **Ctrl+Z across a level load** **[untested]** | edit, load another level, Ctrl+Z | undo does **not** replay the previous level's events |
| ☐ | **Selection outline** **[Lee]** | click any object | outline draws (restored earlier in the port) |
| ☐ | **Emissive Strength live in the editor** **[untested]** | change Emissive Strength on a material | updates without a reload (DX11 6282) |
| ☐ | **Material Type (for sound)** **[untested]** | Material Properties → Material Type | field present; footstep/impact sound follows it (DX11 2455) |
| ☐ | **Allow Bullet Holes tickbox** **[Lee]** | tick it on a door, shoot the door | decals only appear when ticked. ⚠ **behaviour change** — see Part 3 |
| ☐ | **Custom fonts install/uninstall** **[untested]** | Game Settings → fonts | install and uninstall work |
| ☐ | **Button fonts on a widescreen monitor** **[untested]** | run on ultrawide | buttons legible, not shrunk |
| ☐ | **Save/Load menu on ultrawide** **[untested]** | click where two slot boxes meet | one click selects one slot, not two (DX11 6283) |
| ☐ | **UI on a non-primary monitor** **[untested]** | run the game on a second monitor | UI stays on screen (DX11 6347) |

## 3. Editor — visuals and the performance panel

Nearly all of this is DX12-only work that has no DX11 equivalent.

| ✔ | what | how to exercise it | expect |
|---|---|---|---|
| ☐ | **Graphics & Performance panel** **[Lee]** | TAB TAB | stable row order; "Hide idle rows" on by default at 0.05 ms (3.19–3.20c) |
| ☐ | **Object Detail Distance** **[Lee]** | slide it | works upwards; the curve is **U-shaped**, so more is not always faster (3.11) |
| ☐ | **Texture Detail** **[Lee]** | slide it | applies **live**, no reload (3.12/3.19). ⚠ this one raced the streaming thread and caused a device hang before 3.35b — worth a poke |
| ☐ | **Super Quick Objects** + rung slider **[Lee]** | tick it, slide the rung | progressively simpler object shading; stock→FLAT was −52% GPU (3.31/3.34) |
| ☐ | **Shadow Detail Steps** **[Lee]** | slide it | shadow resolution steps down (3.29) |
| ☐ | **Delayed Shadows** **[Lee]** | toggle | ⚠ now defaults **ON** for a NEW level, cascade resolution 1024 (was off / 2048) |
| ☐ | **Occlusion Cull Delay** (per level) **[Lee]** | slide it | ⚠ Occlusion **Off** is a net loss, not a win (3.08/3.28) |
| ☐ | **Post FX / AO / Simple Sky / Shadows off-switches** **[Lee]** | toggle each | Shadows Off is worth ~+17% (3.08) |
| ☐ | **Reduction Scale** (animation) **[Lee]** | slide it | defaults 25; distant characters animate less often (3.25/3.25o) |
| ☐ | **Terrain Bake** + near-tier detail **[Lee]** | enable on a heavy level | terrain becomes plain meshes; −820 MB on Canyon Offensive (3.25/3.25l) |
| ☐ | **Water Bake** **[Lee]** | enable | water plane bakes (3.25a/3.25d/3.25f) |
| ☐ | **Four brutal terrain off-switches** **[Lee]** | the low-spec group | Aztec editor GPU 15.6 → 5.85 ms, 63 → 167 FPS (2.94) |
| ☐ | **SSAO Power accepts negative values** **[untested]** | set SSAO Power below 0 | darkens rather than clamping (DX11 6232) |
| ☐ | **Gamma slider** **[untested]** | slide it | brightness changes. ⚠ the *level-load fade-in* is a separate thing and is **not** in this build |
| ☐ | **Global Probe Brightness / probe markers** **[Lee]** | place a `%probe`, look at the balls | no circles on the cube faces (2.89), picking doesn't resize the ball (2.76), Brightness works and Range is gone (2.90) |
| ☐ | **Env-probe capture** ⚠ **[conflicted]** | place a fresh probe, inspect its capture | my notes disagree with themselves on whether a probe still photographs its own marker. **Wants one deliberate eyeball.** |

## 4. Behaviours and scripting

DX11 added a batch of new behaviour scripts; **all are present in this build** (verified on disk).

| ✔ | what | how to exercise it | expect |
|---|---|---|---|
| ☐ | **`process_zone`** **[untested]** | place the marker, set it up | zone processes as documented |
| ☐ | **`prefill_zone` / `refill_zone`** **[untested]** | place, enter | inventory prefill / refill fires |
| ☐ | **`global_check_zone`** **[untested]** | place, set a global | zone reacts to the global |
| ☐ | **`teleport_node`** **[untested]** | pair two nodes | player teleports |
| ☐ | **`action_talk`** **[untested]** | on an NPC | talk action runs, with its sounds |
| ☐ | **`prompter`** **[untested]** | place, approach | prompt shows |
| ☐ | **`view_activator`** **[untested]** | place, look at it | activates on view |
| ☐ | **`entity_in_zone`** **[untested]** | place, push an entity in | detects |
| ☐ | **`resource_node`** **[untested]** | place, harvest | resource node behaviour |
| ☐ | **SPRITELIB** **[untested]** | `spritelib.lua` / `spritelib_demo.lua` | sprite library API works |
| ☐ | **Cover & prone attack sounds** **[untested]** | NPC using cover_attack / prone_attack | sounds play |
| ☐ | **`wpe_area` X and Z offsets** **[untested]** | set X/Z on a WPE area | placement matches the offsets |
| ☐ | **Entity Sound Slot 4 from Lua** **[untested]** | a script using Sound4 | plays slot **4**, not 5; doesn't leak on level change or go silent on AI death |
| ☐ | **Chest keys unlock doors** **[untested]** | key in a chest → assign as a door's USE KEY → TAKE ALL → try the door | door unlocks. Dropping the key should re-lock it |
| ☐ | **Save-game reload restores spawned objects** **[untested]** | spawn something in play, save, reload | it comes back **visible and solid** |
| ☐ | **Winzone after loading a save** **[untested]** | load a save, hit the winzone | advances instead of repeating the level (DX11 4909) |
| ☐ | **Custom HUD does not leak to the next level** **[untested]** | set a custom HUD, go to level 2 | level 2 uses its own (DX11 6252) |
| ☐ | **`GetPlrLookingAt`** **[untested]** | a script using it | reliable (DX11 6253) |
| ☐ | **`GetSunColorRed/Green/Blue`** **[untested]** | call from Lua | returns the sun colour |
| ☐ | **`ForceMouseXYClick`** **[untested]** | from Lua | synthesises a click |
| ☐ | **Lua runtime errors produce a crash log** **[untested]** | force a Lua error | `Guru-Crash.log` gets an entry — previously this class produced **no log at all** |
| ☐ | **Lua 5.4 compatibility** **[Lee]** | any script using `math.atan2`, `bit32`, `unpack`… | works. DX11 ships Lua 5.2; DX12 links 5.4.8 and shims the removed surface in C++ (3.40/3.48) |
| ☐ | **Door logic gating** **[Lee]** | a level with many doors | only *switch-operated* doors opt out of the 750-unit logic freeze now (3.21) — a DX12 improvement over DX11 |

## 5. Weapons and combat

| ✔ | what | how to exercise it | expect |
|---|---|---|---|
| ☐ | **Delayed Shot** **[untested]** | bow or crossbow | the shot holds until the release frame |
| ☐ | **`animchoicemode` 1 and 2** **[untested]** | a GUNSPEC using them | attack anim selection changes. Modes 1/2 were unreachable before — the parser never set the field |
| ☐ | **Bullet tracers** **[Lee]** | fire a tracer weapon | tracers draw. The DX11 engine hook was never re-homed in the port until 3.55 |
| ☐ | **Tracers fired from behind the camera** **[Lee]** | fire while spinning | no stray tracers (3.56 — a DX11-era bug too) |
| ☐ | **Bullet holes** **[Lee]** | shoot a wall | decals land. `COUNT_BULLETHOLES` exists if you want a number |
| ☐ | **Melee custom fpe decal impacts** **[untested]** | melee near but not on an object | impact does **not** trigger on a miss |
| ☐ | **Custom explosions reach standalone** **[untested]** | custom explosion, export | present in the standalone (DX11 6257) |
| ☐ | **Case-insensitive custom decals** **[untested]** | wrong-cased decal name in `SetCustomExplosion()` | still finds it instead of falling back to default |
| ☐ | **FP weapon depth carve / weapon shadow** **[Lee]** | look down at your hands | no clipping (2.09/2.14) |

## 6. Trees, terrain and water

| ✔ | what | how to exercise it | expect |
|---|---|---|---|
| ☐ | **Tree sway** **[Lee]** | Tree Wind slider on a vegetated level | canopy moves, trunk base planted (3.57) |
| ☐ | **Entity tree sway** **[Lee]** | imported tree with "Object Wind" set, global wind at 0 | still sways — DX11 behaviour reproduced through the engine wind path (3.58b) |
| ☐ | **Wind Gust Size** **[Lee]** | slide it end to end | ⚠ the mapping was inverted and re-floored — the bottom of the range is now usable (3.62/3.63) |
| ☐ | **Far-tree billboards** **[Lee]** | look at a distant ridge | distant forest present, no pop at the handover (2.95–3.07) |
| ☐ | **Terrain Bake keeps your trees** **[Lee]** | tick Terrain Bake on a vegetated level | near 3D trees stay put. ⚠ they used to be **deleted outright**, leaving only distant billboards, and a tick/untick cycle also lost the far-tree shadows until reload (3.73) |
| ☐ | **Fog reaches billboard trees at the right strength** **[Lee]** | strong Horizon/Fog colour, Fog Opacity 100, then pull Fog Range in | distant trees haze at the same rate as the terrain under them. ⚠ they used to go **solid fog colour** over barely-tinted ground — the custom draws were fogged 4× too hard (3.72). Grass and baked terrain/water shared the fault |
| ☐ | **Second level loads** **[Lee]** | load one demo, then another, in one session | ⚠ **this was broken for four weeks** — a treeless first level poisoned the billboard atlas for everything after it (3.53) |
| ☐ | **Terrain painting & sculpting** **[swept]** | paint, sculpt | applies. ⚠ an unpainted level used to inherit the previous level's paint map entirely (3.65) |
| ☐ | **Grass** **[Lee]** | paint grass, change types | works; the type cache is now dropped per level load (3.68) |
| ☐ | **Water** **[Lee]** | place water, drag the slider | no black band, no 4 FPS hold (2.68a) |
| ☐ | **Start Marker underwater** **[untested]** | put the Start Marker below the waterline | ⚠ **behaviour change** — you now start underwater instead of being teleported to waterline+20 |

## 7. Particles and weather

| ✔ | what | how to exercise it | expect |
|---|---|---|---|
| ☐ | **Rain and snow** **[Lee]** | enable weather | ⚠ these are now **WPE particle effects**, not the legacy ravey system (3.59) |
| ☐ | **WPE particle effects & WPE Zone** **[Lee]** | place a `.pe` effect | renders, follows camera where the `.pe` says to |
| ☐ | **gpup / `.arx` particles** **[Lee]** | a level using them | render without the "ping" or white-out |
| ☐ | **"Add New Particle" button** **[Lee]** | Entity → Add New Particle | icon shows and it places a real marker (3.49 authored the three missing content files) |
| ☐ | **Newer `.PE` files load** **[Lee]** | a `.pe` from a later authoring version | loads — the version check is a sanity bound, not a ceiling (3.60) |
| ☐ | **Wicked impact particles face the camera** **[untested]** | shoot a surface | impact sprite faces you (DX11 fix) |
| ☐ | **Additive blending survives Test Game** **[untested]** | AvengingEagle-style glow decal | stays additive in test/standalone instead of going flat with dark edge boxes |

## 8. Test Game

| ✔ | what | how to exercise it | expect |
|---|---|---|---|
| ☐ | **Test Game runs** **[swept]** | any demo | 19/19 reached gameplay three times last night |
| ☐ | **Doors keep collision on the second level** **[untested]** | load level 2, walk into a door | solid (DX11 6246 / 6183) |
| ☐ | **Lights don't vanish via the graphics menu** **[untested]** | in game, open graphics menu, return | lights still there (DX11 6256) |
| ☐ | **Loading screen between levels** **[untested]** | level 1 → level 2 | no flash of the *previous* level's loading screen (DX11 6207) |
| ☐ | **Music volume** **[untested]** | set it, play | respected |

## 9. Standalone (PLAY GAME and Export Game)

⚠ **This is the least-tested area in the build.**

| ✔ | what | how to exercise it | expect |
|---|---|---|---|
| ☐ | **Hub PLAY GAME** **[harness]** | select a demo → PLAY GAME | relaunches as a standalone, title screen, then the level plays. I verified this by driving it from Lua last night — **no key or synthetic click I tried moved the title screen on, so please confirm a real mouse click works** |
| ☐ | **"Export Game"** ⚠ **[untested since 2026-08-16]** | export a standalone folder and run it | **the single largest untested surface in this build.** No harness verb exists for it |
| ☐ | **Standalone save/load slots** **[Lee]** | save and load in a standalone | works — Lua 5.4 writes float filenames, which the integer readers choked on (2.70) |
| ☐ | **Standalone ships splash + FSR shaders** **[Lee]** | run an export | correct splash, FSR available (2.69) |
| ☐ | **Standalone folders stay clean** **[Lee]** | export, look in the folder | no diagnostic trace droppings — `producelogfiles=0` gates them (2.71) |
| ☐ | **`version.ini` copied for standalones** **[untested]** | export | present |
| ☐ | **Standalone loading speed** **[untested]** | load a level in standalone | DX11 added a nav-mesh vertex cache for this; ⚠ see Part 3 — DX12 has the cache-clearing half only |
| ☐ | **Standalone VRAM** ⚠ | watch VRAM in standalone | **known issue** — standalone holds textures at full resolution where the editor and Test Game hold them reduced, +950 to +1520 MB. See Part 3 |

---

# PART 2 — Notes for alpha testers

*Plain-language version. Everything here is new since the last DX11 build testers saw.*

## The big one: this is the DirectX 12 build

GameGuru MAX now runs on DirectX 12 with a rebuilt renderer. Most of the work you cannot see
directly — it is the same editor and the same games — but a few things are genuinely different and
a few are genuinely better.

## New things you can use

**Behaviours** — nine new behaviour scripts: `process_zone`, `prefill_zone`, `refill_zone`,
`global_check_zone`, `teleport_node`, `action_talk`, `prompter`, `view_activator` and
`entity_in_zone`, plus the **SPRITELIB** sprite API and a **resource node** for harvesting. New
behaviour icons throughout.

**Weapons** — **Delayed Shot** for bows and crossbows (the shot holds until the release frame), and
a new **`animchoicemode`** GUNSPEC setting controlling how attack animations are chosen.

**Building** — grid sizes below 1.0 now stick across a restart; dragging an object keeps its height
offset; **Material Type** on a material drives its sound; **Emissive Strength** updates live in the
editor; SSAO Power accepts negative values; you can install and uninstall custom fonts from Game
Settings.

**Performance controls** — a large set of new switches for lower-spec machines: Object Detail
Distance, live Texture Detail, Super Quick Objects, Shadow Detail Steps, Occlusion Cull Delay,
animation Reduction Scale, and **Terrain Bake** / **Water Bake**, which trade some fidelity for a
large VRAM and GPU saving on heavy levels. There is a rebuilt Graphics & Performance panel behind
TAB TAB to see what they do.

**Chest keys** now flag correctly, so a key taken out of a chest unlocks its door.

## Things that behave differently from the DX11 build you know

- **Bullet holes on doors.** A non-static but immobile object no longer collects bullet decals
  unless you tick **"Allow Bullet Holes?"** on it. Previously decals stuck to doors and then hung in
  mid-air when the door swung.
- **Start Marker underwater.** If you place the Start Marker below the waterline you now start
  underwater. It used to teleport you up to the surface.
- **Shadow defaults on a NEW level** — Delayed Shadows default on, cascade resolution defaults 1024.
  Existing levels keep whatever they were saved with.
- **Rain and snow** are now built on the Wicked particle system rather than the old emitters.
- **Doors and logic** — only switch-operated doors now stay "always active"; this makes levels with
  many doors noticeably cheaper.

## Fixed since November

Ultrawide save/load double-selection · UI appearing off-screen on a second monitor · Game Settings
refusing to open with the Object Tools tab open · button fonts shrinking on widescreen · newly
placed objects arriving locked · crash loading a level whose group was deleted · Ctrl+Z replaying
the previous level after a level change · doors losing collision on the second level · lights
vanishing after visiting the graphics menu · the previous level's loading screen flashing between
levels · custom HUDs leaking into the next level · loading a save then hitting the winzone repeating
the level · melee decal impacts triggering on a miss · custom explosions not reaching standalone ·
case-sensitive custom decal names · `GetPlrLookingAt` being unreliable · entity Sound Slot 4 · a
heap corruption when loading a game · impact particles not facing the camera · the storyboard title
screen in the project system · storyboard video not advancing at the end · spawned objects vanishing
after a save reload.

Plus, on the DX12 side: distant tree billboards restored, tree sway restored, bullet tracers
restored, the selection outline restored, second-level loading fixed, and a long VRAM campaign that
removed roughly **90 MB of growth per level load** — a long editing session no longer climbs.

## What we would most like you to hammer

1. **Export Game** and running the exported standalone. Least-tested area in the build.
2. **Long sessions** — load and edit many levels in one sitting without restarting MAX.
3. **The new behaviour scripts** above.
4. **Chest keys, Delayed Shot, Sound Slot 4** — fixed but never exercised by us.
5. **Anything on a 4 GB graphics card**, especially standalone play.

---

# PART 3 — Known limitations (tell testers, so they don't report them)

- **Three material dropdown entries do nothing**: "Water", "Glass" and "Tree Animate Doublesided".
  Their shader bodies are not in this build. (Tree sway itself works — it comes from a different
  path now.)
- **Custom shader parameters 1–7 have no effect.** Authoring surface only; no shipped content uses
  them.
- **No PP Snow/Dust, no transparent shadows, no bloom strength, and no fade-in from black at level
  load.** The first three need engine-side work. Values you set are saved and round-trip correctly.
- **AI path detail varies** depending on where the camera was when the nav mesh was built. DX11's
  high-quality version was ported and then reverted — under the DX12 terrain system it froze the
  editor for a minute on a large map.
- **Standalone loading is slower than it needs to be** — DX11 added a nav-mesh vertex cache; this
  build clears that cache but does not yet fill it.
- **Environment probes pop** rather than fade when you leave an indoor volume.
- **GPU particles sort as one batch**, so an effect can sort wrongly against glass.
- **Standalone uses ~1–1.5 GB more video memory than Test Game** on the same level. If you are on a
  4 GB card, expect the exported game to be the demanding case, not the editor. Found 2026-09-20,
  not yet fixed.
- **A long editing session can drift a few hundred MB above a fresh load** on the three heaviest
  demos. A cold load of any demo is comfortably inside 4 GB.
- **Point-light cube shadow faces pack small** — quality, not correctness.
- **FPS numbers are not comparable between sessions on the same machine** — this rig drifts ±18%
  across days with background load.

---

## Where the detail lives

| document | what it holds |
|---|---|
| `PREALPHA_REPORT_2026-09-20.md` | last night's work, test results, and the DX11↔DX12 comparison in full |
| `DX11_VS_DX12_KNOWN_DIFFERENCES.md` | the intentional differences, with the reasoning for each |
| `NIGHT_INVESTIGATIONS_2026-08-12.md` | every numbered change §2.x–§3.70, with the evidence |
| `DX11_PARITY_DEEP_AUDIT_2026-09-16.md` | the per-commit audit of all 338 DX11 commits |
| `DX12_SECOND_PASS_AUDIT_2026-09-17.md` | the structural audit that found the half-ported features |
