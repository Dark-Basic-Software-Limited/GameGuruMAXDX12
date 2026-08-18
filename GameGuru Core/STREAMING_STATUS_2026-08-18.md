# Texture streaming status — confirmed active (2026-08-18)

Lee: *"Can you confirm the texture streaming from the latest Wicked Engine is active for my
static and dynamic objects in the level scenes?"*

**YES — for object material textures, in the editor, and it is doing real work.**
Measured on Island Showdown: **240 resources enrolled, 8.9 MB resident against 769 MB of
full-resolution content**.

This document also **corrects an error I made earlier the same day** in
`ALPHA_READINESS_2026-08-18.md`, where I wrote that streaming was default-OFF. It is not.

---

## The controlling facts

| Fact | Where |
|---|---|
| Game-side enrolment switch **defaults to ON** | `wickedcalls_part0.cpp:350` — `bool g_bTextureStreamingEnabled = true;` |
| Enrolment gate — three conditions | `wickedcalls_part0.cpp:471` — `if (g_bTextureStreamingEnabled && bAllowStreaming && t.game.gameisexe == 0)` |
| Streaming is **opt-out**, not opt-in | `wickedcalls_part0.cpp:548-556` — both short `WickedCall_LoadImage` overloads pass `bAllowStreaming = true` |
| Only **4 of 43** call sites opt out | sky ×2, lens flare, HUD — their shaders write no mip feedback, so they would decay to the floor and stay blurred |
| Residency is driven by **GPU mip feedback, per material** | `wiScene.cpp:5042-5057` — if a material's feedback word is non-zero, every texture slot gets `StreamingRequestResolution(...)` |
| Per-file requirement | the on-disk file must begin with the `DDS ` magic, sniffed **before** decrypt (`wickedcalls_part0.cpp:473-481`) |
| No `setup.ini` key exists | nothing in `Common_part1.cpp` parses a streaming key; the only levers are runtime harness commands |

### The two conditions worth knowing

1. **Editor only.** `t.game.gameisexe == 0` excludes exported standalone games, deliberately —
   the comment explains the file gets re-encrypted after load and the streaming thread re-reads
   it later at mip offsets. So a game your testers *export* does not stream; the editor they
   build it in does.
2. **Plain DDS only.** The magic sniff means encrypted or non-DDS files never enrol. This is
   why the 08-16 DDS-conversion milestone matters more than it first appeared: streaming needs
   a real mip chain to stream *between*, and that pass gave 1641 stock files full chains.

---

## Live confirmation — Island Showdown, editor, 45 s settle

```
STREAM: on=1 enrolled=240 replaced=10 resident_mb=8.9 full_mb=769.0
        gpumem_pct=17.3 copies=5824 fb=2699229 req=8453377 guard_rejects=0
STREAM3: req0=140 reqLow=85 noMips=1 cancel=14 reqMask=0x81FF
```

- `on=1`, `enrolled=240` — the system is running and 240 resources are enrolled.
- **`resident_mb=8.9` vs `full_mb=769.0`** — the enrolled set would cost 769 MB at full
  resolution; 8.9 MB is actually resident. That is the whole point of the feature, working.
- `fb=2699229` / `req=8453377` — shaders are actively writing feedback and the resolution
  requests are flowing.
- `req0=140` — 140 of the 240 currently request nothing (off-screen), so they sit at the floor.
- `guard_rejects=0` — the bounds guard added after the load-crash work is not firing.

### Both categories the question asked about are enrolled

**Static scenery** — `flagSTREAM=1`, parked at 256×256 out of 11–12 full mips:
`Tent Clean Framed-Canvas_surface.dds`, `Barbed Wire Barricade-Wires_normal.dds`,
`Crates-Box_surface.dds`, `Watchtower Sentry-Details_surface.dds`, `Pallet_surface.dds`,
`Old Jail_color.dds`, `Barrel Explosive_normal.dds`.

**Dynamic / animated** — same treatment:
`charactercreatorplus\parts\adult male head 07_normal.dds`, `gamecore\guns\enhanced\AR\AR_surface.dds`,
`kitagrenade\m67_D.dds`, `bio_grenade\Volume58_normal.dds`.

**Not enrolled** (`static`, `flagSTREAM=0`) — and correctly so:
`terraintextures/mat20/Surface.dds` (terrain goes through SVT, a separate system),
tree billboards, and `bloodsplash5.png` (not a DDS, so it fails the magic sniff).

### There is no static-vs-dynamic distinction in the engine

Worth stating plainly because the question implies one: the engine does **not** key on object
mobility at all. Enrolment is per **texture resource** at load; residency is per **material**
per frame, driven by whether a shader actually sampled it. A character that walks toward the
camera and a crate that never moves are treated identically — both get their mips pulled in
when they are sampled at high resolution, and both decay when they are not.

---

## Correction to the alpha report

`ALPHA_READINESS_2026-08-18.md` recommendation 6 originally read *"Don't revisit texture
streaming (#37) — it is default-OFF because it crashed Trapped and RPG Template."* **Wrong.**

What actually happened: streaming *was* forced to default 0 on **2026-08-01** while the load
crash was open, and per `WETEST.md` it was **"restored the same day by delta 1.73"**. The crash
was then root-caused and fixed (task #47 — *"verify all 19 demos load clean with streaming
ON"*). My memory note recorded the one-day emergency state as if it were permanent, and I
repeated it into a decision document without checking the source.

Consequence: streaming needs **no decision** before the alpha. It is already the shipping
behaviour, and the 19/19 sweep in `DEMO_FPS_SWEEP.md` §0818b ran with it on.

---

## Levers (runtime only — there is no ini key)

| Command | Effect |
|---|---|
| `SET_TEXSTREAM 0\|1` | Enrolment kill-switch (default 1). Gates whether *newly loaded* textures get `Flags::STREAMING`; already-loaded ones keep their state, so reload the level for a clean A/B |
| `SET_STREAMING 0\|1` | Pauses/resumes the running system. ⚠ the level-reload guard resumes it after every load |
| `DUMP_STREAM2` | Engine-side authoritative enrolled set → `Files/stream_resources.txt` (used for this report) |
| `DUMP_STREAM` | Scene-side per-material feedback + current slot sizes |
| `SET_TEXSTREAMTRACE 1` | Per-load forensics; expensive, for hunting load faults only |
