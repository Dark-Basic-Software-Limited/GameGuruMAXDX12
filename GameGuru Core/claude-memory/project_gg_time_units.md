---
name: project-gg-time-units
description: "GameGuru's game-loop time unit is 1/20th of a second, so any threshold compared against a timeelapsed_f accumulation is 20x what it reads as"
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-14T15:00:43.076Z
---

# ★★★ One real second = 20 units. A `>= 100` is FIVE SECONDS, not 100 ms.

```cpp
// M-Game_part2.cpp:1541, game_timeelapsed()
t.ElapsedTime_f = fThisTimeCount - t.LastTimeStamp_f;   // seconds, from timeGetSecond()
g.timeelapsed_f = t.ElapsedTime_f * 20.0;               // ★ 1 second = 20 UNITS
if (g.timeelapsed_f > 0.75f)   g.timeelapsed_f = 0.75f;   // clamp ~25 fps floor
if (g.timeelapsed_f < 0.00833f) g.timeelapsed_f = 0.00833f;
```

Anything that accumulates `g.timeelapsed_f` (or `t.decaltimeelapsed_f`, which is a copy) and
compares against a bare number is in these units. **Divide by 20 to read it in seconds.** Because
the accumulation scales with frame time, the resulting duration is frame-rate INDEPENDENT — a
`>= 100` is a flat 5.0 s at 30 fps and at 300 fps alike, which is why such a bug never looks like
a timing/perf problem.

## The one this cost (3.37, 2026-09-14)

Lee: "when I shoot into the water, the splash repeats 5-6 times… WAY too long for a splash".
`M-Decal.cpp` stopped a WPE decal emitter at `framedelay >= 100`, written as though it meant
100 ms. It meant **5.0 s of continuous emission**, then the live particles dissipated on top —
the 6-8 s he saw. Fixed with a named constant: `GG_WPE_EMIT_SECONDS * 20.0f`.

★ **Why only the WPE branch was wrong, and the lesson in it:** the legacy branch a few lines below
is the same test on the same field, but accumulates `decaltimeelapsed_f * 2 * playspeed_f`, which
scales that same `100` to ~0.8 s for the impact decal. The threshold had only ever been tuned
*through* that multiplier. When the WPE path was added it copied the constant but not the
multiplier — so **a magic number was carried across into a context where its unit silently
changed.** Two call sites, one literal, two different meanings.

## How it was found — and what did NOT find it

⚠ Reading the code produced THREE wrong theories, all plausible, all disproved:
1. "the legacy branch spawns `splash_droplets` five times" — real code, but that branch never ran
2. "`weapon_projectile_destroy` clears the wrong slot (`t.tProj` vs `t.tNewProj`)" — disproved by
   line 303, which assigns `t.tNewProj = t.tProj` inside the same loop
3. "the emitter is re-fired every frame" — disproved: `bParticle_Fire` is cleared immediately
   after the burst (`M-Entity_part5.cpp:277`)

★★★ What settled it in one run: a **counter at the single choke point** every caller funnels
through, logging call number, elapsed ms and which branch ran. One shot produced exactly ONE line
with `bWPE=1` — which killed theories 1 and 2 outright and left only "one emitter, running too
long". Then the fix was arithmetic, not inspection. Same rule as [[project-measuring-rules]]:
build the instrument that NAMES the culprit instead of reaching for the next theory.

⚠ Instrument design note: the trace grouped calls into bursts separated by 3 s of quiet. Lee
reported "one shot" but had fired three; each read as `call #1` at a *different* xyz, which is what
proved they were separate shots rather than one repeating event. **Log position as well as count**
— the count alone would have been ambiguous.

## ⚠ Separate, still open: the content

`Files/gamecore/decals/splash_large/wpe.pe` holds **five** emitters — `pe-emitter-d`,
`pe-emitter-d - Copy`, `pe-emitter-ripple 1` and two further Copies of that ripple — plus seven
`wpe*_color.png` pages. `impact/wpe.pe` has ONE. They all burst together, so one splash pays for
five emitters. Not the cause of the duration bug, but worth re-authoring; it is also a per-splash
particle cost on exactly the low-spec hardware the campaign targets. Enumerate emitters with a
printable-string scan of the `.pe` — `strings` does not exist in this Git Bash.
