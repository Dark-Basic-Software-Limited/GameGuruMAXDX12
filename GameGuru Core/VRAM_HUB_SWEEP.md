# VRAM sweep — every hub demo, opening scene

**LATEST: 2026-08-02, engine `a6cb310e` / game `8fd09cb7`** (texture streaming ON, merged grass
default OFF). Cold launch per demo so no reading is contaminated by the previous level; level
opened in the editor, 35 s settle, then `GET_PERF_DATA` + `DUMP_VRAM`. Reproduce with the sweep
script pattern in `tools/`. "driver" is what the FPS panel shows as VRam — the number a user feels.

**Range 4644 -> 10023 MB, mean 6060 MB across 19 demos.** All 19 load clean.

| # | Demo | driver MB | census MB | grass MB | hair sys | strands | polys | FPS |
|---|---|---|---|---|---|---|---|---|
| 1 | **The Mystery of Z Island** | **10023** | 8522 | 4297 | 185 | 12,432,000 | 704,717 | 69.3 |
| 2 | **Aztec Game Kit** | **8069** | 6693 | 1919 | 64 | 4,104,000 | 3,438,876 | 75.5 |
| 3 | **Jungle Fever** | **7651** | 5853 | 2403 | 63 | 6,300,000 | 74,124 | 93.0 |
| 4 | **Canyon Offensive** | **7002** | 5314 | 1883 | 62 | 5,298,000 | 8,816,163 | 58.5 |
| 5 | Operation Amazon | 6577 | 4844 | 963 | 26 | 2,354,000 | 5,496,922 | 83.3 |
| 6 | Foggy Forest | 6468 | 4900 | 1097 | 30 | 3,000,000 | 10,195,894 | 58.7 |
| 7 | RPG Template | 6223 | 4633 | 1330 | 51 | 2,786,000 | 3,235,005 | 72.4 |
| 8 | River Raiders | 5703 | 4065 | 728 | 23 | 1,726,000 | 1,906,072 | 104.8 |
| 9 | Indian Strike Force | 5690 | 3809 | 0 | 0 | 0 | 3,184,527 | 99.6 |
| 10 | Aztec Game Kit Teaser | 5650 | 4024 | 1041 | 48 | 2,335,000 | 10,313,511 | 64.6 |
| 11 | Disruption | 5509 | 3585 | 208 | 6 | 436,000 | 4,665,184 | 83.9 |
| 12 | Bounty | 5383 | 3801 | 0 | 0 | 0 | 463,210 | 113.7 |
| 13 | Snowy Mountain Stroll | 5372 | 3538 | 333 | 7 | 700,000 | 81,081 | 124.1 |
| 14 | Horseshoe Bend | 5348 | 3376 | 0 | 0 | 0 | 2,133,269 | 94.6 |
| 15 | Island Showdown | 5192 | 3649 | 602 | 34 | 1,350,000 | 4,115,636 | 64.3 |
| 16 | A Grand Canyon Adventure | 5139 | 3437 | 503 | 48 | 1,942,000 | 2,272,361 | 95.9 |
| 17 | Trapped | 4814 | 2969 | 0 | 0 | 0 | 11,209 | 138.1 |
| 18 | Escape from the Zombie Cellar | 4688 | 2937 | 0 | 0 | 0 | 28,048 | 127.4 |
| 19 | Switch Escape | 4644 | 2794 | 0 | 0 | 0 | 109,358 | 130.4 |

## Where it goes (MB)

| Demo | grass | SVT pool | mesh alloc | content tex | arrays | shadow-T | shadow-D | chunk maps |
|---|---|---|---|---|---|---|---|---|
| The Mystery of Z Island | 4297 | 768 | 512 | 979 | 220 | 384 | 195 | 105 |
| Aztec Game Kit | 1919 | 768 | 1024 | 1114 | 220 | 320 | 162 | 105 |
| Jungle Fever | 2403 | 768 | 512 | 595 | 220 | 320 | 162 | 158 |
| Canyon Offensive | 1883 | 768 | 512 | 781 | 220 | 160 | 81 | 105 |
| Operation Amazon | 963 | 768 | 512 | 754 | 220 | 512 | 260 | 105 |
| Foggy Forest | 1097 | 768 | 512 | 486 | 220 | 512 | 260 | 105 |
| RPG Template | 1330 | 768 | 512 | 587 | 220 | 256 | 130 | 158 |
| River Raiders | 728 | 768 | 512 | 722 | 220 | 160 | 81 | 105 |
| Indian Strike Force | 0 | 768 | 512 | 918 | 220 | 256 | 130 | 105 |
| Aztec Game Kit Teaser | 1041 | 768 | 512 | 478 | 220 | 160 | 81 | 105 |
| Disruption | 208 | 768 | 512 | 352 | 220 | 512 | 260 | 105 |
| Bounty | 0 | 768 | 512 | 521 | 220 | 512 | 260 | 105 |
| Snowy Mountain Stroll | 333 | 768 | 512 | 440 | 220 | 320 | 162 | 105 |
| Horseshoe Bend | 0 | 768 | 768 | 476 | 220 | 160 | 81 | 105 |
| Island Showdown | 602 | 768 | 512 | 452 | 220 | 160 | 81 | 158 |
| A Grand Canyon Adventure | 503 | 768 | 512 | 442 | 220 | 160 | 81 | 105 |
| Trapped | 0 | 768 | 512 | 509 | 220 | 160 | 81 | 105 |
| Escape from the Zombie Cellar | 0 | 768 | 512 | 359 | 220 | 160 | 81 | 105 |
| Switch Escape | 0 | 768 | 512 | 312 | 220 | 160 | 81 | 105 |
| **HUB TOTAL** | **17308** | 14592 | 10496 | 11278 | 4180 | 5344 | 2713 | 2156 |

## What the numbers say

**1. A ~1.85 GB floor is paid by EVERY level, including an indoor cellar.** SVT pool 768 +
mesh allocator 512 + source arrays 220 + chunk maps ~106 + SVT bookkeeping ~134, present and
near-identical on all 19. Switch Escape draws 109 k polys with no grass and still needs 4.6 GB,
which is why nothing in the hub goes below that. Attacking the floor improves **every** project.

**2. Grass is the entire spread: 17.3 GB hub-wide**, 0 on six demos and 4.3 GB on Z Island
alone. It is the single biggest prize, and the merged-grass work (delta 1.74, default off)
targets exactly it — Z Island reports 12 chunks carrying all 185 systems at 8+ types each, so
merging would take 185 -> 12.

**3. The transparent shadow atlas reaches 512 MB** on Operation Amazon, Foggy Forest, Disruption
and Bounty (vs 160 elsewhere), and pairs with ~260 MB of depth atlas. On **Bounty that is 772 MB
of shadow atlas on a level with no grass at all** — proportionally the largest non-grass item in
the hub. RGBA16F -> RGBA8 halves the transparent half; confirm nothing actually renders into it
first, because the alpha channel carries a depth value compared against the shadow test.

**4. Aztec Game Kit is the content outlier**: 1114 MB of textures and a doubled 1024 MB mesh
allocator. Its 128 MB of un-mipped 4096-square character skins (see the single-mip DDS audit in
`VRAM_CENSUS.md`) sit inside that content figure.

**5. Polygons and VRAM are unrelated.** Aztec Teaser draws 10.3 M polys in 5.7 GB; Jungle Fever
draws 74 k in 7.7 GB. Measure, never infer VRAM from scene complexity. (The converse also holds —
see `DEMO_FPS_SWEEP.md`: FPS tracks polygons, not VRAM.)


## Transparent shadow atlas — RGBA8 REJECTED, but the feature looks visually inert (2026-08-02)

**RGBA8 is NOT safe.** The atlas alpha carries a depth value consumed by
`TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK` (`transparent_shadow.a > cmp` in shadowHF.hlsli),
and 8 bits would quantise that compare to 256 levels. The standing caveat was "verify the
alpha-depth test first" — so it was verified, with a counter on every batch reaching the
`FILTER_TRANSPARENT | FILTER_WATER` shadow queue (harness `SHADOWT:` line):

| Demo | editor | in game |
|---|---|---|
| Operation Amazon | 41,212 | 59,305 |
| Foggy Forest | 24,773 | 33,758 |
| Disruption | 48,121 | 77,311 |
| Bounty | 59,222 | **632,167** |
| Island Showdown | 20,866 | 32,157 |

The atlas is written heavily on **every** level tested, so the depth value is genuinely produced
and the format cannot be narrowed. There is also no escape hatch: every 4-byte RGBA format has
8-bit alpha or worse (R10G10B10A2 gives 2). **Do not re-attempt the RGBA8 conversion.**

### The bigger question, and a surprising answer

Since the format cannot shrink, the real question is whether the atlas earns its 512 MB at all.
`SET_TRANSPARENTSHADOWS <0|1>` skips the draws — the atlas clears to `(1,1,1,0)`, so rgb white
makes the tint multiply a no-op and alpha 0 makes the depth check always fail, i.e. exactly
"feature off" with nothing else altered. Same-session A/B, same camera:

| Demo | ON vs OFF | noise floor (ON vs ON, later frame) | verdict |
|---|---|---|---|
| Bounty | 0.023% differing, meanAbs 0.031 | 0.021%, 0.022 | at the noise floor |
| Island Showdown | 16.493%, 1.848 | 16.414%, 1.751 | at the noise floor |
| Operation Amazon | 3.623%, 0.262 | 5.531%, 0.511 | **BELOW the noise floor** |

**No measurable visual difference on any of the three**, including the two strongest candidates
(Island Showdown has ocean — `FILTER_WATER` feeds this same queue — and Amazon is dense foliage).
FPS is unchanged too (115.2/114.3, 64.6/64.4, 85.3/84.5), so the cost is memory and the
per-frame clear, not draw time.

**The gate was proven before the A/B was trusted**: with it off the batch counter is exactly
frozen (72276 -> 72276) and climbs again when restored. Without that check the comparison would
have been worthless, because both screenshots could have had the feature live.

**Implication:** the prize is not RGBA8's half — it is the whole atlas, ~512 MB on four demos and
~5.3 GB hub-wide. But that is a rendering-feature removal, not a free win, and it is measured on
3 of 19 demos in the editor only. **Left DEFAULT ON pending a decision plus a full-hub sweep**;
user content could legitimately use coloured transparent shadows (stained glass, tinted water)
even where the stock demos do not.

## Suggested targeting order

| Target | Per-demo | Hub-wide | Risk |
|---|---|---|---|
| Grass merged systems (delta 1.74) | up to −3 GB | ~−14 GB | BUILT, default OFF — fails the density gate, see `VRAM_CENSUS.md` |
| SVT atlas halving (`svtatlasheight=8192`) | −384 MB × 19 | ~−7 GB | needs a fast-travel soak; switch already exists |
| Transparent shadow atlas RGBA8 | — | — | **REJECTED, measured — see below** |
| Single-mip content DDS (967 files) | varies | ~−5.5 GB resident | content-side, no code |
| `texGrass` dead array | −61 MB × 19 | ~−1 GB | none — provably never written or read |

---

# Previous sweep (2026-08-01, streaming verification run)

## Verification sweep, streaming ON, after the 1.73 fix (2026-08-01 evening)

**All 19 demos reach the editor. Zero crashes, zero `guard_rejects`, `stream_guard.txt` never
created.** Cold launch per demo, same method as the table above.

Two independent checks that each level genuinely loaded rather than the process merely surviving:
every driver-VRAM figure lands within **±1.2%** of the pre-fix streaming-on sweep, and the
per-demo enrolled/resident counters are all populated.

| # | Demo | driver MB | enrolled | resident MB | full MB | reclaimed MB | replaced | guard_rejects |
|---|---|---|---|---|---|---|---|---|
| 1 | The Mystery of Z Island | **10086** | 329 | 67.4 | 1284.4 | 1217 | 179 | 0 |
| 2 | Aztec Game Kit | **8117** | 346 | 195.5 | 1389.3 | 1194 | 111 | 0 |
| 3 | Jungle Fever | **7714** | 338 | 92.6 | 1508.7 | 1416 | 280 | 0 |
| 4 | Canyon Offensive | **7001** | 352 | 61.7 | 1126.3 | 1065 | 33 | 0 |
| 5 | Operation Amazon | **6592** | 199 | 84.2 | 624.2 | 540 | 285 | 0 |
| 6 | Foggy Forest | **6533** | 293 | 41.2 | 1146.2 | 1105 | 198 | 0 |
| 7 | RPG Template | **6222** | 132 | 8.7 | 406.3 | 398 | 31 | 0 |
| 8 | River Raiders | **5703** | 397 | 42.0 | 1213.5 | 1172 | 483 | 0 |
| 9 | Indian Strike Force | **5626** | 643 | 21.5 | 1493.3 | 1472 | 220 | 0 |
| 10 | Aztec Game Kit Teaser | **5602** | 257 | 62.4 | 1286.8 | 1224 | 213 | 0 |
| 11 | Disruption | **5428** | 216 | 47.3 | 511.3 | 464 | 83 | 0 |
| 12 | Bounty | **5400** | 227 | 39.1 | 862.0 | 823 | 287 | 0 |
| 13 | Snowy Mountain Stroll | **5324** | 263 | 62.4 | 745.0 | 683 | 162 | 0 |
| 14 | Horseshoe Bend | **5300** | 383 | 47.5 | 1382.8 | 1335 | 26 | 0 |
| 15 | Island Showdown | **5240** | 202 | 7.8 | 638.7 | 631 | 152 | 0 |
| 16 | A Grand Canyon Adventure | **5202** | 136 | 8.9 | 529.2 | 520 | 16 | 0 |
| 17 | Trapped | **4750** | 116 | 9.7 | 490.8 | 481 | 12 | 0 |
| 18 | Escape from the Zombie Cellar | **4671** | 152 | 75.7 | 595.8 | 520 | 31 | 0 |
| 19 | Switch Escape | **4580** | 198 | 76.9 | 481.1 | 404 | 116 | 0 |

**"reclaimed" = `full_mb − resident_mb`**: what the enrolled textures would occupy at full mip
chains versus what is actually resident. Streaming is holding back **16.7 GB across the hub,
mean 877 MB per demo**, ranging 398 MB (RPG Template) to 1472 MB (Indian Strike Force).

`replaced` is now in the sane 12–483 range everywhere. Before the no-progress fix, Trapped alone
read **260,468** — it was rebuilding byte-identical textures every streaming pass.

Also verified in one session: three level loads plus a test-game round trip (the travel-churn
path the 1.44 reload guard and 1.47 allocator fix exist for) — no crash, `guard_rejects=0`,
no `resource_hijack.txt`, and **zero `OVERLAP-ALLOC` / `OOB-ALLOC`** in the allocator ledger.

> Reading `alloc_tripwire.txt`: it is a **ledger**, not a fire-only tripwire. It is always
> non-empty (A/D/R allocate-defer-release lines, easily 100+ MB). Only `OVERLAP-ALLOC` and
> `OOB-ALLOC` lines indicate a real problem — grep for those, never test the file for emptiness.

## The sweep found a CRASH, and it was ours — now FIXED (engine 1.73)

**Trapped** and **RPG Template** did not reach the editor. Both died ~15 s into the level load
with an access violation **inside `memcpy`** (`Guru-Crash.log`: `0xc0000005`, vcruntime
`memcpy.asm`). All 19 demos loaded fine in the 2026-07-31 sweep, so it was a regression from the
texture-streaming enrollment that shipped that morning — A/B'd three times each with
`SET_TEXSTREAM 0`.

### Root cause: block-alignment in the streaming mip reduction

Both leading suspects were **wrong**. It was not the background streaming thread, and not the
decrypt/re-encrypt cycle. What settled it was adding a **symbolized stack walk to the crash
handler** (`CrashLogger.cpp`), which put the fault in the *initial* upload
(`LoadResourceDirectly` → `CreateTexture`), on the **main thread** — not the streaming job at
all. A per-load breadcrumb (`SET_TEXSTREAMTRACE 1` → `stream_load.txt`) then named the texture:

```
DOOR1_surface.dds | file 500x500 mips 9 fmt DXT1 | upload 250x250 mips 8 mip_offset 1
```

The engine's mip reduction halves width/height with no regard for block-compression alignment.
**500 is a multiple of 4; 250 is not** — and a BC resource's *top* mip must be block-aligned
(sub-mips are exempt, which is why the unreduced load always worked). `GetCopyableFootprints`
refuses such a desc and, rather than failing, writes `0xFFFF..` sentinels into every output.
Upstream's only guard against that is `rowSizesInBytes[i] > (SIZE_T)-1`, which on a 64-bit build
compares UINT64 against UINT64_MAX and **can never be true** — so `MemcpySubresource` looped
`numRows = 4,294,967,295` straight off the end of the file buffer.

That explains the content-dependence exactly: only a level holding a BC texture whose dimension
is 4×odd trips it. Everything else in the hub is power-of-two and halves safely.

### The fix

1. **Stop the reduction before it produces a misaligned top mip** — applied to *both* halving
   sites (the load-time reduction and the stream-out decay). Such textures simply keep a larger
   base mip; they save no VRAM, which is the right trade for not crashing.
2. **Bail when no mips could be shed.** Without this the desc is unchanged and the job rebuilds
   a byte-identical replacement every pass: Trapped read **`replaced=260468`**, and **`12`**
   after the fix.
3. **Make the sentinel check actually work** in `wiGraphicsDevice_DX12.cpp`, so no future bad
   desc can ever be a memory fault again.

`g_bTextureStreamingEnabled` is back to **true**, verified by re-running this whole sweep.

### Instruments this left behind

- **Symbolized crash stacks in `Guru-Crash.log`** — every future crash now names its call chain,
  the faulting thread (main vs worker), and for an AV the read/write flag, target address and
  page state. This turned a week-class hunt into one repro.
- `SET_TEXSTREAMTRACE 1` → `stream_load.txt` + `last_upload.txt` (per-load forensics).
- `stream_guard.txt` tripwire + `guard_rejects=` on the `STREAM:` line — should always be 0.
