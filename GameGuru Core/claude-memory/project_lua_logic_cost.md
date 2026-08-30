---
name: project-lua-logic-cost
description: "How GameGuru MAX gates per-entity Lua logic by distance, what the \"Top Ten Most Expensive Logic\" box actually measures, and the standing script defects found in door/weapon/patrol/FlickerLight"
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-25T22:21:14.856Z
---

# Per-entity Lua logic cost (investigated 2026-08-25 from Lee's Canyon screenshot)

## ★★★ The engine ALREADY has the distance early-exit — some scripts opt OUT of it

`M-LUA.cpp` ~line 906, inside the per-entity loop:

```cpp
if (phyalways == 0 && ischaracter == 0) iDistanceForLogicToBeProcessed = 750;
if (plrdist < iDistanceForLogicToBeProcessed || phyalways != 0 || lua.flagschanged == 2) { ...logic... }
```

| entity | gate |
|---|---|
| ordinary object | **750** units |
| character (`ischaracter == 1`) | `MAXFREEZEDISTANCE` = **2000** |
| anything with `phyalways != 0` | `MAXNEVERFREEZEDISTANCE` = **999000**, i.e. never — and the `\|\| phyalways != 0` short-circuits the test outright |

★ **`SetEntityAlwaysActive(e,1)` sets `phyalways`** — `DarkLUA_part1.cpp:195` → `RawSetEntityData`
field **22** → `eleprof.phyalways` (`DarkLUA_part1.cpp:102`). So any script calling it in its init
runs full cost every frame from any distance. **`objects/door.lua` and `objects/door_sliding.lua`
both do**, which is why door.lua tops the cost list on a level where the player is nowhere near a
door. Before optimising a script, check whether it opted out of a gate that already exists.

## ⚠ The "Top Ten Most Expensive Logic This Cycle" box is misleading in four ways

`M-LUA.cpp:1262-1315`, triggered by `producelogfiles=3`.

1. ★★ **The numbers are raw `QueryPerformanceCounter` TICKS, not microseconds**, despite the `u`
   suffix. QPC is 10 MHz on this machine (`[Diagnostics.Stopwatch]::Frequency`), so **1 unit =
   0.1 µs** and "00321u" is **32 µs**. Everything in the box is 10× smaller than it reads.
2. **It prints NINE entries, not ten** — `for (int j = 9; j > 0; j--)` never emits index 0.
3. **The trigger threshold is 30000 ticks = 3 ms**, yet nothing listed is anywhere near it, so
   whatever tripped the box is usually NOT in the list. Most likely cause: the scan zeroes any
   entity with `active == 0`, so a script that spikes and then deactivates erases its own evidence.
4. ★★ **The timer brackets the WHOLE per-entity update, not just the script** — start stamp at
   `M-LUA.cpp:871`, delta at `:1247`, with ~380 lines of C++ in between (coordinate sync, the
   usekey scan, animation handling). "door.lua = 32 µs" means everything done for that door.

⚠ Consequence: the nine listed rows totalled 1474 ticks = **0.147 ms**, while the panel's
`Update - Logic - LUA` read **1.17 ms**. The box shows ~13% of the cost; the rest is long tail.

## Standing script defects (all in `Files/scriptbank/`)

- **`objects/door.lua`** — `SetEntityAlwaysActive(e,1)` in `door_init` (the big one);
  `GetPlrLookingAtEx(e,1)` called BEFORE the `PlayerDist < nRange` test in both `use_switch`
  branches; six `if x == nil` guards re-run every frame that belong in init;
  `GetEntitySpawnAtStart(e)` + often `SetEntityHasKey(e,1)` every frame forever.
- **`objects/door_sliding.lua`** — same `SetEntityAlwaysActive`, same ungated `GetPlrLookingAt`.
- **`weapon.lua`** — lines 71-72 call `GetPlayerDistance` and `GetPlrLookingAtEx` unconditionally,
  before the `PlayerDist < pickup_range` tests at 77 and 87. Does NOT set AlwaysActive, so it is
  correctly gated at 750.
- **`people/patrol.lua`** — calls `GetPlayerDistance(e)` **twice** per frame; leaks `PlayerDist` as
  a **global**; calls `LoopSound` + `SetSoundVolume` every frame regardless of distance. ⚠
  `SetSoundVolume` takes no entity, so every patrolling character stomps one global each frame —
  a correctness bug, not just cost.
- **`markers/FlickerLight.lua`** — `GetEntityLightNumber(e)` every frame (again inside
  `module_lightcontrol.control`), `lightNum` is a **global**, and it flickers regardless of whether
  the light is anywhere near the player.
- **`people/character_attack.lua`** — a thin wrapper; its cost IS `masterinterpreter` walking the
  behaviour bytecode. No cheap win in the script itself.

## `GetPlrLookingAtEx` is pure Lua and has NO distance check

`scriptbank/global.lua:403` (`GetPlrLookingAtExThreshold`). Per call: a `GetHeadTracker()` engine
call, `math.atan2`, two modulo normalisations, several global-table lookups — all before anything
range-related. ★ The RAY inside it is already well guarded (FOV first, one cast per cycle globally
via `g_PlayerCastDoneOneForThisCycle`, 250 ms per entity, `dist < 120`), and the underlying
`WickedCall_SentRay4` had its unbounded-TMax bug fixed on 2026-07-31 (was ~26 ms/call). So the ray
is NOT the per-frame cost — the unconditional trig is.

Related: [[project-performance]], [[project-measuring-rules]].

## ✅ Fixed 2026-08-25/26 (3.21, game `7cbd8fe0`)

- **Doors gated.** `SetEntityAlwaysActive` moved from `*_init` into `*_properties` and made
  conditional on `use_switch == 1` / `door_type == 'Switched'`. Measured after: **0 entities
  bypassing the gate**, door.lua down to 1.4 us with 0/1 running.
  ⚠ Edit `Scripts/scriptbank/` in the REPO, not the build-area copy — both exist, only one is
  versioned.
- **Reporter fixed**: microseconds not ticks, ten rows not nine, latched offender, and a SECOND
  timer around just `LuaSetFunction/PushInt/Call` so a slow script can be told from a slow entity.
- **`DUMP_LOGICCOST`** aggregates one armed frame BY SCRIPT; profiler sub-ranges `LUA-loop
  begin/allentities/finish` split the `Update - Logic - LUA` row.

### The TESTPRO2 answer (spotshadowtest, test game)

`Update - Logic - LUA` 1.77 ms → `LUA-loop allentities` **1.52 ms (86%)**; of the 908 us of timed
per-entity work, **`no_behavior_selected.lua` is 593.8 us (65%) across 1985 entities that execute
ZERO Lua**. All real script execution on the level is **169.7 us, under 10% of the row**.
★ The cost is the WALK, not the scripts. Biggest available win: skip the per-entity body for
entities with no behaviour (`bCanSkipNow` already detects them, but only to skip the Lua call).
★ A scripted entity costs ~10× a script-less one before its script even runs — partly
`t.strwork = cstr(cstr(aimainname) + "_main")`, a heap concat per scripted entity per frame.
⚠ Characters scale worst: patrol **38.8 us of Lua each**, character_attack 24.6 us, all
`masterinterpreter`.

## ★★★ A harness command MUST be idempotent

`DUMP_LOGICCOST` first consumed the report on read and **never once returned data from a script**,
while the same code filled Lee's message box perfectly. The harness re-executes a command file that
is still present, so the command ran again by itself between arm and read, consumed the report, and
wrote it to an `auto_result.txt` nobody read. ★ What cracked it: making the FAILURE reply state its
own internals (`arm was 0, last pass saw 2083 entities`) — one reading ruled out "the pass never
ran". See [[feedback-stale-auto-command]].

## ✅ 3.22 — the inert-entity skip SHIPPED (game `74f0d295`), CPU frame −8.7%

★★★ **The obvious suspect was measured first and was WRONG.** `UpdateEntityRT` is a 21-argument
Lua call made per entity per frame, gated on `staticflag` and NOT on having a behaviour — it looked
like the entire answer. A timer around it: **49.3 us total, 38 refreshes, 5.6 us of the 531**. The
real cost is ~0.27 us each of diffuse C++ (`ObjectExist` + `GetFrame` + three `ObjectPosition`, the
freeze-distance calc, the waypoint/usekey/animation blocks) times two thousand. **No hot call to
fix, only a walk to avoid.**

⚠ **A blanket "no script → skip" is UNSAFE**: the body mirrors the object position into
`entityelement[e].x/y/z`, and **~450 places in the scriptbank index `g_Entity` by something other
than their own `e`**. ★ So the predicate is narrowed until safety is **structural**: a mirror cannot
go stale for an entity that cannot move. **INERT = no behaviour + `staticflag != 0` + no waypoint
zone + not animating + not a character.** Every term is a property that cannot change while the
entity is inert. On TESTPRO2 that costs 31 entities of 1993 and buys the whole argument.

**Measured, interleaved A/B, four rounds, no overlap between arms**: `LUA-loop allentities`
1.175 → **0.515 ms** (−56%), `CPU Frame` 7.21 → **6.58 ms (−8.7%)**.

★★ **`TEST_INERTSKIP` measures the invariant instead of arguing it** — walks every entity the skip
would elide and compares the mirror to the live object position. Six samples, **11,772 comparisons,
0 differing, worst drift 0.0000 units**. A static entity that turned out to move would show up here
and nowhere else in the game.
⚠ It does NOT prove other fields are safe — `haskey`, `plrinzone` and the animation flag are
excluded by the predicate rather than verified. `SET_LOGICSKIP 0` reverts live.

## 3.23 — name cache shipped, function ref measured and REFUSED (game `ecd25725`)

- **Per-entity `"<script>_main"` cache** (`SET_LUANAMECACHE 0` reverts). Stored in its OWN array,
  not in `t.entityelement`, which rides save/load. Rebuilt whenever the source name differs, so a
  runtime script swap is caught next frame; a stale name calls the wrong `_main` and visibly
  breaks behaviour — loud, never silent. **30.9/32.1/33.1/30.2 → 4.0/3.8/4.1/4.0 us, −87%**,
  no overlap, and per-script `ran/have` identical in both arms for all 16 scripts.
- ⚠⚠ **27 us is INVISIBLE at frame level** — CPU Frame 6.72/6.72, 6.73/6.66, 6.71/6.76,
  6.71/6.70. Kept because it is free and scales with scripted-entity count (90 here; 1000 would be
  ~300 us), **not** because it moved the frame. Do not report it as an FPS win.
- ★★★ **The function-ref cache was MEASURED AND NOT BUILT.** A timer around `LuaSetFunction`
  (which contains the `lua_getglobal`) reads **~6 us total across every scripted entity**, ~67 ns
  each. The entire prize is 6 us and collecting it needs a new API in the **DarkLUA DLL every
  script call in the product goes through**. One build to measure; several to build, for 0.09% of
  a frame. Say no with the number.
- ⚠ **Latent trap, unfixed**: `LuaCall()` opens with a **linear `strcmp` scan over
  `FunctionsWithErrors`** on every lua call. Empty today, only ever grows, and entries are pushed
  on any script error or missing function — so **after one bad script every lua call in the game
  pays a strcmp per entry for the rest of the session**. A hash set or a per-entity flag fixes it.

## 3.24 — FunctionsWithErrors scan fixed (game `f3fc33a8`)

★★ **It was never latent.** TESTPRO2 in test game holds **39 entries**, and their names explain
it: GameGuru probes each script for OPTIONAL entry points (`_init_name`, `_init_file`, `_init`,
`_properties`) and an absent one is recorded **exactly like a real failure**. Every ordinary
project fills this list within seconds of loading, then pays a strcmp per entry on every lua call
forever. ⚠ 3.23's "after one bad script" framing was too generous to it.

★★★ **The obvious O(1) fix was a 2.7x REGRESSION at the real size** and I would have shipped it.
`std::unordered_set<std::string>` measured **231 ns** a lookup vs **85 ns** for the linear scan at
39 entries, only winning past ~110. `find()` on a `char*` builds a temporary `std::string` (names
are 20-35 chars, past SSO → **heap allocation**) and hashes it, every lookup.

Shipped: **FNV-1a into a parallel array of hashes**, strcmp only on a hash hit.
1→12.7 ns, 10→15.3, 50→42.2, 200→141.9 vs linear 3.3 / 21.8 / 107.8 / 409.6.
⚠ Loses below ~4 entries by ~9 ns; a small-n fast path was rejected as a branch on every lookup to
recover nothing. `TEST_LUAERRSET` proves sync and restores real state after benchmarking on the
live helpers.

