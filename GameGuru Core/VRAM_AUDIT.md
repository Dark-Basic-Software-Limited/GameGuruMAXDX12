# VRAM audit — all 19 hub demos, categorised for a low-end knob plan

> **⚡ SUPERSEDED FOR CURRENT NUMBERS — see [2026-08-04 post-merge re-audit](#2026-08-04-post-merge-re-audit)
> at the bottom.** Merged grass + lazy PSOs + 16 MB blocks landed after this table was taken;
> mean driver is now **3188 MB (was 5082)** and **14 of 19 demos fit a 4 GB card at pure
> defaults**. The analysis below remains the reference for bucket definitions and the knob plan.

2026-08-02, engine `3666bbf7` + game `c784d891`. Editor, defaults (no `lowvram` keys), ~35 s
settle, project identity verified before each load. Raw censuses in the session scratchpad;
regenerate with `tools/…` + the `audit.awk` categoriser recorded at the bottom of this file.

Companion docs: `VRAM_FLOOR.md` (why the floor exists), `VRAM_CENSUS.md` (the instrument),
`GRASS_BENCHMARK.md` (the density gate).

## The table

All figures MB of driver-reported video memory unless noted. FLOOR = the eight content-independent
buckets; mesh = the shared suballocator; pad = D3D12MA block padding; nonres = driver-side
memory no resource census can see (overwhelmingly pipeline state objects).

> **CORRECTION 2026-08-03 — the `pad` and `nonres` columns below are individually wrong.**
> Both were derived from `d3d12ma_blocks`, which sums video AND system memory (UPLOAD/READBACK
> staging), while `driver_usage` is video-only. So `pad` charges system-RAM waste to the video
> budget and `nonres` subtracts system blocks from a video figure, understating it by the same
> amount. On TESTPRO1 `pad` read 257 MB of which only **152 was video**, and true `nonres` was
> **483, not 259**. **The `driver` column and every conclusion drawn from the totals are
> unaffected** — the errors cancel in `pad + nonres`. Engine 1.83 added
> `d3d12ma_blocks_video` / `d3d12ma_allocated_video` and `tools/vram_audit.sh` now uses them, so
> a re-run of this table will have correct columns. Detail in `VRAM_FLOOR.md`.

| Demo | driver | FLOOR | mesh | GRASS | TREES | content | misc | pad | nonres | FPS |
|---|---|---|---|---|---|---|---|---|---|---|
| The Mystery of Z Island | 8999 | 1613 | 512 | **4297** | 75 | 909 | 548 | 348 | 698 | 69.9 |
| Aztec Game Kit | 7095 | 1565 | 1024 | 1919 | 81 | 1038 | 569 | 234 | 666 | 75.2 |
| Jungle Fever | 6691 | 1614 | 512 | 2408 | 70 | 526 | 208 | 483 | 871 | 93.8 |
| Canyon Offensive | 6138 | 1483 | 512 | 1883 | 77 | 709 | 305 | 367 | 802 | 57.9 |
| Operation Amazon | 5361 | 1620 | 512 | 963 | 77 | 682 | 264 | 467 | 775 | 82.3 |
| Foggy Forest | 5269 | 1657 | 512 | 1106 | 74 | 408 | 440 | 422 | 650 | 59.2 |
| RPG Template | 5264 | 1593 | 512 | 1331 | 84 | 509 | 162 | 180 | 893 | 72.8 |
| River Raiders | 4840 | 1483 | 512 | 728 | 74 | 653 | 267 | 330 | 793 | 105.0 |
| Aztec Game Kit Teaser | 4786 | 1433 | 512 | 1041 | 81 | 402 | 204 | 205 | 908 | 63.8 |
| Indian Strike Force | 4731 | 1492 | 512 | 1 | 71 | 851 | 448 | 601 | 755 | 101.4 |
| Horseshoe Bend | 4485 | 1480 | 768 | 4 | 73 | 405 | 287 | 504 | 964 | 95.2 |
| Snowy Mountain Stroll | 4348 | 1543 | 512 | 334 | 68 | 376 | 182 | 343 | 990 | 123.1 |
| Island Showdown | 4328 | 1544 | 512 | 603 | 85 | 372 | 192 | 121 | 899 | 63.5 |
| A Grand Canyon Adventure | 4227 | 1476 | 512 | 503 | 73 | 374 | 155 | 252 | 882 | 96.6 |
| Disruption | 4213 | 1670 | 512 | 208 | 66 | 291 | 147 | 419 | 900 | 85.1 |
| Bounty | 4184 | 1670 | 512 | 0 | 63 | 463 | 377 | 438 | 660 | 114.7 |
| Trapped | 3950 | 1470 | 512 | 0 | 63 | 451 | 127 | 386 | 941 | 132.9 |
| Escape from the Zombie Cellar | 3871 | 1535 | 512 | 5 | 63 | 297 | 174 | 411 | 874 | 126.2 |
| Switch Escape | 3780 | 1476 | 512 | 0 | 63 | 255 | 120 | 392 | 963 | 128.7 |
| **mean** | **5082** | **1548** | 552 | **912** | **73** | 525 | 272 | 363 | 836 | |
| min | 3780 | 1433 | 512 | 0 | 63 | 255 | 120 | 121 | 650 | |
| max | 8999 | 1670 | 1024 | 4297 | 85 | 1038 | 569 | 601 | 990 | |

Hub-wide totals: **grass 17,335 MB · content 9,970 · floor 29,415 · trees 1,380.**

## What the shape of it says

**1. The base cost is ~3.3 GB before a single asset.** FLOOR 1548 + mesh 552 + nonres 836 +
padding 363 = **3299 MB mean, and it barely varies** — FLOOR ranges only 1433–1670 across an
indoor cellar, a 10 M-poly teaser and a tropical island. On a 4 GB card (~3.5 GB usable once
Windows and the desktop have taken their share) that leaves **~200 MB for content**. This, not
grass, is why the *smallest* demo still misses: Switch Escape has 255 MB of content textures and
still totals 3780.

**2. Grass is the entire spread, and nothing else is close.** 0 to 4297 MB, mean 912. Six demos
have none at all. The gap between the biggest and smallest demo is 5219 MB, of which grass alone
is 4297. Any per-user quality dial that matters most of the time is a grass dial.

**3. TREES ARE A NON-ISSUE — do not build a tree knob.** 63–85 MB on every single demo, a 22 MB
spread across the whole hub. Tree *textures* are small and tree *mesh* data lives in the shared
suballocator, which is itself near-constant. A "tree density/distance" VRAM setting would be
theatre; if trees need a dial it is for framerate, not memory.

**4. Content textures are modest and already adaptive.** 255–1038 MB, mean 525, with mip
streaming live. Aztec Game Kit is the outlier (1038 MB plus a doubled 1024 MB mesh pool) and is
the one demo where a content-side budget cap would earn its keep.

**5. `nonres` is still 836 MB mean and it is nearly all pipeline state objects.** Lazy PSOs
(shipped, `lowvram=1`) take it to roughly 200. That is the single largest fixed-cost win
available and it costs no visual quality at all — only first-use compile hitches.

## What the shipped preset actually achieves

Projected from measured lever effects — lazy PSOs −633, grass draw cap −11 % of grass, density
multiplier on the remainder — against a ~3500 MB usable budget on a 4 GB card:

| Demo | now | `lowvram=1` (density 75) | density 50 | fits 4 GB? |
|---|---|---|---|---|
| The Mystery of Z Island | 8999 | 6937 | 5981 | no |
| Aztec Game Kit | 7095 | 5824 | 5397 | no |
| Jungle Fever | 6691 | 5257 | 4722 | no |
| Canyon Offensive | 6138 | 4879 | 4460 | no |
| Operation Amazon | 5361 | 4408 | 4193 | no |
| Foggy Forest | 5269 | 4268 | 4022 | no |
| RPG Template | 5264 | 4189 | 3893 | no |
| River Raiders | 4840 | 3965 | 3803 | no |
| Aztec Game Kit Teaser | 4786 | 3807 | 3575 | no |
| Indian Strike Force | 4731 | 4098 | 4097 | no |
| Horseshoe Bend | 4485 | 3851 | 3850 | no |
| Snowy Mountain Stroll | 4348 | 3604 | 3530 | no |
| Island Showdown | 4328 | **3495** | 3361 | **yes** |
| A Grand Canyon Adventure | 4227 | **3427** | 3315 | **yes** |
| Disruption | 4213 | 3511 | **3464** | at 50 |
| Bounty | 4184 | 3551 | 3551 | no |
| Trapped | 3950 | **3317** | 3317 | **yes** |
| Escape from the Zombie Cellar | 3871 | **3236** | 3235 | **yes** |
| Switch Escape | 3780 | **3147** | 3147 | **yes** |

**5 of 19 at density 75, 6 of 19 at density 50.** The preset as it stands is a big win — it takes
about 1 GB off a typical demo — but **it does not reach the 4 GB goal**, and the reason is
arithmetic rather than effort: after lazy PSOs the immovable base is still ≈2666 MB, leaving
~830 MB for everything the level contains. Content plus misc already averages 797.

So the remaining work is **floor reduction, not content reduction**. Squeezing grass harder
cannot fix a demo like Bounty, which has *no grass at all* and still misses.

## The knob plan

Split by who the dial is for. Anything marked "engineering" is not a user-facing setting — it is
a fix that should simply happen.

### Tier A — fixed-cost work, no visual cost, benefits every user

| # | Item | Saving | Status |
|---|---|---|---|
| A0 | **Legacy grass/tree blade atlases on demand** | **−220** | **SHIPPED + verified** — see below |
| A1 | Lazy object PSOs | **−392** (was −633 before A5 took eager pipelines 6337→4033) | **SHIPPED DEFAULT-ON 2026-08-03** (engine 1.82). Hitch measured, not assumed: worst single compile **1.1 ms**, zero compiles in the first 25 s of play. Revert = `lazypso=0`. See `VRAM_FLOOR.md` |
| A2 | Release reset chunk buffers instead of 4-byte stand-ins | **−72** | **SHIPPED + verified** (game `e1b437a4`) |
| A3 | Mesh suballocator granularity 256→128 MB | **−128 on small levels** | **SHIPPED + verified** (engine `e0287386`) |
| A4 | Terrain machinery sized to demand | see the SVT section below | **investigated, not shipped** — two costed options, needs a soak |
| A5 | Tessellation + voxelize PSO axes | **−2304 pipelines (6337→4033)** | **SHIPPED + verified** (engine `e0287386`) |
| A6 | **D3D12MA block size 64→16 MB** | **−92 to −140 every level** | **SHIPPED + verified 2026-08-03** (engine 1.83). Padding 104-152 → 11.6 MB, non-resource flat, POLYS bit-identical. Revert `mablockmb=64`. TESTPRO1-only evidence so far |

### Tier B — user-facing quality dials, real visual trade

| # | Knob | Range | Saving | Status |
|---|---|---|---|---|
| B1 | **Grass density** | 100 / 75 / 50 | up to −50 % of grass (−2148 on Z Island) | **shipped**, clumpCV-gated, 75 default, do not ship <50 |
| B2 | **Grass draw distance** | slider, capped | −11 % of grass | **shipped** |
| ~~B3~~ | ~~Terrain detail (source arrays 1024²→512²)~~ | — | — | **withdrawn — those arrays were the dead legacy atlases and are gone entirely in A0** |
| B4 | Terrain paint resolution (global maps 4096²→2048²) | 2 steps | −49 | proposed |
| B5 | Shadow resolution (cascades 2048→1024) | existing slider | −60 | wire into the preset |
| B6 | Texture budget cap by card size | auto | caps content at a target | proposed — matters most on Aztec |
| B7 | Weather off ⇒ drop wetmaps | auto | −52 | proposed |
| B8 | SVT atlas + mip bias together | paired | −288 | proposed, **needs its own fast-travel soak** — the atlas alone cannot go below 12288 (1864–2001 tile peak demand) |

### Not worth building

- **Tree knobs.** 63–85 MB, flat. Nothing to win.
- **SVT atlas alone.** Starves below 12288; only viable paired with mip bias (B8).

### If Tier A + B all land

Base drops from ≈3299 to roughly **1800–1900 MB**, leaving ~1.6 GB for content on a 4 GB card.
That fits every demo except the grass giants (Z Island, Aztec Game Kit, Jungle Fever), which
would still need density 50 and would then land near 3.5–4.0 GB. **A 4 GB minimum spec is
reachable, but only if the fixed cost comes down — it cannot be bought with quality dials alone.**

## A0 shipped — and a correction to this document's own naming

The 220 MB bucket labelled **`FLOOR terrain src arrays`** above is **not terrain**. It is five
legacy blade atlases: `texGrass` (1024² × 46 = 63.2 MB) plus `texTree` / `texBranchesHigh` /
`texTreeNormal` / `texTreeHigh` (1024² × 38 = 156.6 MB together). They were byte-identical on
every demo because they are sized from the fixed grass/tree *banks*, not from what a level uses.

They are dead on the shipping path, provably and statically:

- created **empty**, and their only writer (`GGGrass_LoadTextureDDSIntoSlice` /
  `GGTrees_LoadTextureDDSIntoSlice`) is a **no-op stub** — "stub - DDS loading disabled", with the
  real implementation sitting inside an `#if 0`;
- their only readers are `GGGrass_BindGrassArray` (called solely from GGTerrain's own draw),
  `GGGrass_Draw_Prepass` (declared but never registered) and GGTrees' draw functions — and
  **every `customDraw_*` callback in `master_part1.cpp` early-returns while
  `ggterrain_use_wicked_terrain` is set**, which it is;
- shipping grass renders through Wicked's hair particle system and shipping trees through the
  Wicked object path, both sampling per-type materials directly.

Now allocated on demand from the same legacy-activation hook as the 1.70 page atlas, so flipping
the Y-key debug toggle still works. Verified:

| Level | driver before | after | saved | POLYS |
|---|---|---|---|---|
| TESTPRO1 | 5926–5990 | **5710.4** | ~216–280 | 3,149,243 identical |
| Island Showdown | 4328 | **4144.6** | 183 | 4,115,636 identical |
| Foggy Forest | 5269 | **5036.8** | 232 | 10,195,894 identical |

Zero `tex` array records remain in any census. TESTPRO1 also **passes the density gate**:
coverage 9.458 (ref 9.45), clumpCV 1.102 (ref 1.104, noise ±0.006), bands 13.52 13.12 18.80 9.93
1.34 against 13.6 13.1 18.7 9.9 1.3 — inside tolerance on every metric.

## Caveats on two buckets

- **`FLOOR small mesh bufs`** (~222 MB) is dominated by the 576 terrain chunk meshes
  (84,500 B VB + 16,766 B IB + two 4-byte empty streams each = 180 MB, of which 72 MB is pure
  64 KB-granularity waste on the empty pair). It also sweeps up a little content mesh data, so
  treat it as "mostly terrain" rather than purely fixed.
- **`mesh` (suballocator)** is shared by terrain, trees and entity meshes, so tree *mesh* bytes
  cannot be separated out. Since the pool is near-constant at 512 MB this does not change any
  conclusion, but it is why the TREES column is textures only.


## USER VERDICT 2026-08-02

Tested and approved: **"A Grand Canyon Adventure was great, under 4 GB of VRAM on start and over
100 FPS."** That level entered this campaign at 4227 MB / 96.6 FPS in the audit table above.

## Tier A results so far

Cumulative effect of A0 + A2 + A3 + A5, editor, defaults, against this document's own baseline
table. POLYS bit-identical on every level, and TESTPRO1 passes the grass density gate at each
step (final: coverage 9.486, clumpCV 1.103 against a reference of 1.104 ± 0.006):

| Level | audit baseline | after Tier A | saved |
|---|---|---|---|
| Switch Escape | 3780 | **3116** | −664 (−17.6 %) |
| Island Showdown | 4328 | **3841** | −487 (−11.3 %) |
| Foggy Forest | 5269 | **4701** | −568 (−10.8 %) |
| TESTPRO1 (benchmark) | 5926–5990 | **5391** | ~−535 |

Eager driver pipelines 6337 → **4033**. Zero 4-byte stand-in buffers and zero legacy `tex` array
records remain in any census. None of this costs a single pixel — every item was dead weight.

## A4 — the SVT tile pool: findings, and why nothing shipped yet

The pool is **576 MB at the shipping 12288 atlas height**, and it is one `CreateBuffer` sized for
the *whole* atlas across *four* maps (`wiTerrain.cpp`, `tile_pool_desc.size +=` per map). Two
independent reductions exist, and they are very different in cost and risk.

### Option 1 — drop the EMISSIVE map. −144 MB, bounded, low risk

`VirtualTextureAtlas` allocates `maps[4]`: basecolor, normal, surface **and emissive**. The pool
is the sum over all four, so emissive is exactly a quarter of it.

**MAX never uses it.** `GGTerrainWicked.cpp` sets terrain material textures in exactly three
slots — `BASECOLORMAP` (line 334), `NORMALMAP` (335), `SURFACEMAP` (339). No code path anywhere
in the game assigns `EMISSIVEMAP` on a terrain material. The engine allocates the fourth map
unconditionally regardless.

The change is three guarded loops (`wiTerrain.cpp` lines ~1761 creation, ~1843 tile mapping,
~1925 material binding) plus the update dispatch at ~2350. Roughly 20 lines.

### Option 2 — grow the pool on demand. up to −384 MB, but a real refactor

Measured residency is far below capacity: the indoor cellar peaks at **890 of 2852 tiles** (~180
MB of the 576). Sizing the pool to demand and growing it in blocks would reclaim most of that on
light levels while leaving heavy ones untouched.

The obstacle is that `atlas.tile_pool` is a **single** `GPUBuffer`, referenced as one object by
`SetTextureVirtual(atlas.tile_pool, …)` and by `commands[0].tile_pool`. Supporting several pools
means tracking which pool backs each tile and threading that through the tile-mapping commands —
a genuine change to the sparse-residency bookkeeping, not a parameter tweak.

### ATTEMPTED 2026-08-02 AND REVERTED — read this before retrying

Option 1 was implemented, built, soaked and **reverted**. Two things came out of it.

**1. The saving is −96 MB, not −144. My "quarter of the pool" arithmetic was wrong.**
The pool is `sum over maps of (total_tile_count * sparse_page_size)`, and the maps do **not** have
equal page sizes — BASECOLORMAP and EMISSIVEMAP share the same `case` and are **BC1** (4 bpp),
while NORMALMAP is **BC5** and SURFACEMAP is **BC3** (8 bpp each). So the pool is
`X + 2X + 2X + X = 6X = 576` ⇒ `X = 96`. **Emissive is the cheapest map, one sixth of the pool,
not one quarter.** Measured on Z Island: tile pool 576.0 → **480.0 MB**, and the atlas dropped
from 8 to 6 sparse textures (3 maps × texture + texture_raw_block), exactly as intended.

**2. It breaks terrain rendering — the whole scene blows out to white.** Confirmed on both
A Grand Canyon Adventure and The Mystery of Z Island: terrain, objects and sky wash to near-white
with only faint silhouettes. Not subtle, and not a transient load state.

**The soak itself passed**, which is the important diagnostic detail: three laps of fast travel on
Z Island gave `MIN_FREE = 881 of 2852` tiles, peak used **1971** — statistically identical to the
pre-change peak of ~2001. So this is **not** tile starvation and **not** a residency bug. The
guarded loops did what they were supposed to. It is a *shading* failure.

**Most likely mechanism, NOT yet confirmed:** the four map loops were guarded, but the material
binding loop was skipped for emissive with a plain `continue`, leaving
`material->textures[EMISSIVEMAP]` unbound. If the terrain shader reads that slot unconditionally
and an unbound bindless slot resolves to the engine's default **white** texture, emissive becomes
full white and is added to every terrain pixel — which is exactly what the screenshots look like.

**Do not retry by guessing.** Next step is an instrument, not another theory: confirm what the
terrain shader actually samples for a missing emissive slot (`SET_TANGENTVIS 15` renders emissive
directly — that is the cheapest possible check). Then either bind an explicit 1×1 black texture
into the emissive slot, or zero the terrain material's emissive colour, and re-verify with
**screenshots as well as numbers**. The VRAM figures alone looked excellent while the picture was
destroyed.

### Why neither shipped in this session

Both touch terrain residency, and the user's instruction was explicitly "the SVT pool **with a
soak**". A fast-travel soak is what catches tile starvation (the failure mode that killed
`svtatlasheight=8192`), and it cannot be compressed — the 8192 experiment only failed visibly
after sustained travel. There was not enough time left to implement, build and soak properly, and
a half-soaked terrain change is worth less than no change.

**Recommendation: take Option 1 first.** It is bounded, provably dead, worth 144 MB on every
level, and one full fast-travel soak signs it off. Option 2 is worth more but deserves its own
session.

## 2026-08-04 post-merge re-audit

Engine `0aaab86c` + game `d7b1f1cf` (merged grass default-ON + NURI flicker fix 1.96 + slider
fix 1.97). Same instrument, same method as the 2026-08-02 table: editor, defaults (no `lowvram`
keys), ~35 s settle, project identity verified per load, `tools/vram_audit.sh`. Two column
corrections vs the old table: `pad`/`nonres` are now the **video-only** GGMAX 1.83 fields
(the old columns mixed system memory — see the CORRECTION note at the top), and defaults now
include everything shipped since: merged grass, lazy PSOs, D3D12MA 16 MB blocks, A0/A2/A3/A5/A6,
texture streaming ON. ✔ = fits the ~3450 MB usable budget of a 4 GB card **at pure defaults**.

| Demo | driver | Δ vs 08-02 | FLOOR | mesh | GRASS | TREES | content | misc | pad | nonres | FPS (was) |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Aztec Game Kit | **4375** | −2720 | 1273 | 896 | 226 | 81 | 904 | 694 | 177 | 496 | 67.5 (75.2) |
| The Mystery of Z Island | **4067** | −4932 | 1321 | 512 | 378 | 75 | 776 | 673 | 171 | 527 | 109.7 (69.9) |
| Operation Amazon | **3632** | −1729 | 1328 | 512 | 342 | 77 | 677 | 268 | 184 | 480 | 104.3 (82.3) |
| Indian Strike Force | **3505** | −1226 | 1200 | 512 | 1 | 71 | 718 | 572 | 210 | 589 | 118.3 (101.4) |
| RPG Template | **3467** | −1797 | 1301 | 512 | 469 | 84 | 503 | 172 | 78 | 469 | 118.9 (72.8) |
| Canyon Offensive | **3412** ✔ | −2726 | 1191 | 512 | 247 | 77 | 704 | 304 | 114 | 505 | 75.8 (57.9) |
| River Raiders | **3360** ✔ | −1480 | 1191 | 384 | 408 | 74 | 646 | 272 | 125 | 497 | 131.9 (105.0) |
| Foggy Forest | **3245** ✔ | −2024 | 1365 | 512 | 200 | 74 | 274 | 566 | 132 | 482 | 69.1 (59.2) |
| Aztec Game Kit Teaser | **3209** ✔ | −1577 | 1141 | 512 | 399 | 81 | 397 | 225 | 98 | 486 | 80.1 (63.8) |
| Horseshoe Bend | **3144** ✔ | −1341 | 1188 | 640 | 4 | 73 | 400 | 298 | 151 | 541 | 96.0 (95.2) |
| Disruption | **3082** ✔ | −1131 | 1378 | 512 | 208 | 66 | 286 | 151 | 127 | 478 | 91.8 (85.1) |
| Jungle Fever | **3045** ✔ | −3646 | 1322 | 384 | 148 | 70 | 520 | 213 | 53 | 509 | 162.1 (93.8) |
| A Grand Canyon Adventure | **2907** ✔ | −1320 | 1184 | 512 | 252 | 73 | 369 | 165 | 19 | 459 | 97.2 (96.6) |
| Bounty | **2869** ✔ | −1315 | 1378 | 384 | 0 | 63 | 330 | 510 | 79 | 492 | 137.0 (114.7) |
| Island Showdown | **2859** ✔ | −1469 | 1252 | 512 | 66 | 85 | 366 | 180 | 30 | 477 | 87.7 (63.5) |
| Snowy Mountain Stroll | **2824** ✔ | −1524 | 1251 | 384 | 96 | 68 | 371 | 185 | 79 | 504 | 143.1 (123.1) |
| Trapped | **2590** ✔ | −1360 | 1178 | 384 | 0 | 63 | 446 | 135 | 60 | 457 | 152.9 (132.9) |
| Escape from the Zombie Cellar | **2508** ✔ | −1363 | 1243 | 384 | 5 | 63 | 259 | 197 | 64 | 450 | 139.4 (126.2) |
| Switch Escape | **2473** ✔ | −1307 | 1184 | 384 | 0 | 63 | 244 | 145 | 83 | 478 | 164.5 (128.7) |
| **mean** | **3188** | **−1894** | 1256 | 492 | 182 | 73 | 484 | 312 | 107 | 493 | 113.0 (92.0) |

### What changed

- **14 of 19 demos now fit a 4 GB card at PURE DEFAULTS** — no `lowvram` preset, no quality
  dial touched. On 2026-08-02 the count was **zero at defaults**, and even the shipped preset
  only projected 5–6 of 19 with grass density cut to 50.
- **Grass collapsed as a category: hub total 17,335 → 3,450 MB (−80 %).** Z Island's grass
  went 4297 → 378. The spread problem this document was written around no longer exists;
  the remaining spread is content + misc.
- **Every fixed bucket moved the way the Tier A ledger said it would**: FLOOR mean 1548 → 1256,
  nonres 836 → 493 (lazy PSOs), pad 363-mixed → 107-video (16 MB blocks), mesh pool now drops
  to 384 on small levels (128 MB granularity).
- **FPS came along for free: hub mean 92.0 → 113.0.** Jungle Fever 93.8 → 162.1,
  Z Island 69.9 → 109.7. Sole decline is Aztec Game Kit (75.2 → 67.5), within the known ±8
  cross-launch editor variance.

### The remaining five, and what actually moves them

| Demo | over budget by | dominant residue |
|---|---|---|
| Aztec Game Kit | +925 | content 904 + mesh 896 + misc 694 |
| The Mystery of Z Island | +617 | content 776 + misc 673 |
| Operation Amazon | +182 | content 677 |
| Indian Strike Force | +55 | content 718 + misc 572 |
| RPG Template | +17 | noise — effectively fits already |

The pattern is uniform: **the stragglers are content-texture and misc demos now, not grass
demos**. Levers in flight / next, in order of expected effect on exactly these five:

1. **Mip-chain the 990 single-mip DDS (5.5 GB of unstreamable source textures)** — **DONE,
   swapped in and measured, see the next section.** Single-mip textures are pinned at full
   size forever; with mips the streamer can demote them. Directly attacks the content column
   on all five stragglers.
2. **A4 Option 1, emissive SVT map (−96 floor, every level)** — retry with an explicit 1×1
   black bind; `SET_TANGENTVIS 15` first to confirm what the shader samples (see the A4
   post-mortem above).
3. **Dead terrain chunk VB/IB (−108 floor)** + **wetmap-on-weather B7 (−52)** + B5/B4.
4. **Misc autopsy for Aztec/Z Island/ISF** (500–700 MB each; skinned streamout + unnamed +
   upload) — no lever exists yet because nobody has itemised it since the merge.

The `lowvram` preset remains the safety net for the stragglers in the meantime: grass density
is no longer the lever that matters there — a content/texture budget cap (B6) is.

## 2026-08-04 mip swap-in: measured result

990 single-mip DDS under `Files/entitybank` (5511.7 MB — every DDS > 64 KB, 2D, non-array,
single-mip) converted to full mip chains with DirectXTex texconv 2026.5.8.1, format-preserving
(`-f` from a header scan; `-dx9` legacy headers except BC7/SRGB), zero failures, dims verified
identical per file. **Originals preserved in a mirror tree at `D:/max/mipbackup/entitybank/` —
revert = copy the mirror back.** Mipped files add ~33 % on disk (5.5 → 7.3 GB) but become
STREAMABLE: the mip streamer can finally demote them, so resident VRAM goes *down*.

Re-census under the identical protocol, same session as the sweep (POLYS **bit-identical** on
both demos — same scene, same geometry, only the texture files changed):

| Demo | driver before | after | Δ | content before | after | FPS before | after |
|---|---|---|---|---|---|---|---|
| Aztec Game Kit | 4375 | **3911** | **−464** | 904 | **476** (−47 %) | 67.5 | **105.7** |
| Jungle Fever | 3045 | **2917** | −128 | 520 | **387** (−26 %) | 162.1 | 165.5 |

Every other census bucket matched the sweep run to within a few MB — the delta is pure
content-column demotion, exactly the mechanism predicted. Resource *count* is unchanged
(444/438 content textures in both runs); the streamer simply keeps fewer mips resident.

**The Aztec FPS jump is real and repeat-confirmed**: 67.5 unmipped → **105.7 / 113.5** across
two independent mipped launches (driver 3911/3943, content 475.8 MB byte-identical both
times, POLYS bit-identical all three runs). Mechanism: sampling minified single-mip textures
destroys texture-cache locality, and Aztec was the hub's worst offender. The mip conversion
is a ~+40 FPS *performance* fix there on top of the VRAM saving.

Visual spot-checks: Aztec temple interior crisp near and far (screenshot at census), Jungle
Fever rocks/crates clean. The 8192² `waterfall 8x8.dds` sprite sheet (largest converted file,
67 MB) was verified by decoding backup vs converted mip 0, since the start camera cannot see
the waterfall in-editor: **meanAbsDiff 0.36/765, max single-pixel 20/765** — pure BC3
decompress/recompress round-trip noise, visually invisible.

**Caveats / follow-ups:**
- **These files are NOT in git.** The build area is the only place the swap exists; a
  reinstall or asset refresh silently reverts it. The same conversion must be run on the
  MASTER asset store once the user approves the visual result.
- Aztec is still +461 over the 3450 budget — its residue is now mesh 896 + misc 694, no
  longer content. The misc autopsy and A4/chunk-VB/IB floor work are the remaining levers.
- Straggler projection with the content column shrinking 26–47 %: RPG Template (+17) and
  Indian Strike Force (+55) should now fit outright; Operation Amazon (+182, content 677)
  is borderline; Z Island (+617) improves but likely still needs one more lever.

## 2026-08-04 misc-bucket autopsy (Aztec / Z Island / Indian Strike Force) — and two shipped levers

User-directed follow-up on the three stragglers. The census "misc" columns (500–700 MB each)
were itemised per resource and every family traced to its allocation site (8-agent workflow,
findings adversarially verified; full detail in the session transcript). What "misc" actually
was:

| Family | Aztec | Z Island | ISF | Verdict |
|---|---|---|---|---|
| CopyAllocator upload pool | 283 | 282 | 283 | **SYSTEM RAM, never video** — pow2 freelist, never trimmed; the 256 MB member was the uncompressed sky cube upload rounded up |
| Legacy GPU-particle textures | 149 | 46 | 0 | **100 % dead VRAM** — system hard-disabled in DX12 (`gpup_init` returns −1) but `gpup_loadEffectFile` still allocated 5 renderTex + 3-4 imageTex per placed emitter |
| Sky cubemap (R11G11B10, uncompressed) | 134 | 134 | 134 | content file; BC6H-safe (sky is a read-only SRV, mip 0 only; probes already prove BC6H cubes work) |
| Skinned streamout | 38 | 57 | 73 | real machinery, created for every rigged mesh at load, never gated on animation — lazy-create lever identified, own session |
| Z Island visibility payload_0/1 | — | 39 | — | allocated-never-touched when only SSR is on (Z Island is the only demo authored with SSREnabled=1) — engine skip lever identified |
| Always-on ocean/weather/debugUAV | ~21 | ~21 | ~21 | levers identified, small |

### Shipped this session

1. **GGMAX 1.98 — particle-texture guard** (`GPUParticles_part0.cpp`): one line —
   `gpup_loadEffectFile` returns −1 while `gpu_particles_initialised == 0`. Every caller
   already handled −1. Also fixed the `vram_categorize.sh` w≥4096 rule that mis-binned the
   4096×64 particle noise strips as terrain VT pages.
2. **Sky cubemap compression** (content, BOTH skybank trees — **the engine loads the
   DOCUMENTS tree** `…\GameGuruApps\GameGuruMAX\Files\skybank`, the only resource that does):
   the 7 uncompressed R11G11B10 2048² cubes (clear, cloudy, highcloud, overcast, stormy,
   sunny, sunset) → **BC6H_UF16** with mips, 134.5 → 33.6 MB VRAM each; dreamnebulamoon
   (B8G8R8A8 4096², LDR) → **BC7_UNORM** (not BC6H — the engine sRGB-decodes it via the
   _SRGB subresource, which BC6H cannot provide), 384 → 97 MB. **`night` (already BC7) and
   `sunrise` (already BC1) untouched — BC6H would have doubled sunrise.** Backups:
   `D:/max/mipbackup/skybank_documents` + `skybank_buildarea`. Five demos paid the sky tax:
   the three stragglers plus Foggy Forest and Bounty.

### Measured (same protocol, POLYS bit-identical on all three)

| Demo | before | after | Δ | what landed |
|---|---|---|---|---|
| Aztec Game Kit | 3911 | **3671** | −240 | particles −149 (misc 207.8→58.7, exactly the 360 dead textures) + sky −96. Upload pool also −165 **system** RAM — the 256 MB staging buffer is never minted now that the sky upload is 34 MB |
| The Mystery of Z Island | 4067 | **3859** | −208 | sky −96 + particles −47 + staging −47 (Z Island baseline was pre-mip-swap; its content barely moved, its single-mip population was small) |
| Indian Strike Force | 3505 | **3121 ✔ FITS** | −384 | sky −96 + the mip swap-in finally measured here (content 718→438) — 329 MB headroom |

Aztec sky bucket 133.5 → 37.5 and ISF's converted sky renders with smooth gradients
(screenshot check). **Score at pure defaults: 15 of 19 fit**, and Operation Amazon + RPG
Template likely also fit now (their baselines predate the mip swap) — pending re-census.

### Remaining, ranked for the last two stragglers

1. Aztec (+221): A4 emissive (−96) + dead chunk VB/IB (−108) + B5 shadow slider (−60) close
   the gap arithmetically. Its mesh pool (896) is content-driven (10 M polys).
2. Z Island (+409): the payload skip (−39, engine delta: allocate payload_0/1 only when
   `visibility_shading_in_compute`) + A4 + chunk VB/IB + grass density 75 (−94) ≈ fits.
3. Streamout lazy-create (38–73 on these three): real but deserves its own session — BLAS
   rebuild + first-frame motion-vector caveats documented in the transcript.
4. CopyAllocator freelist trim: ~250 MB **system** RAM per session; engine-side aging,
   low risk. Not a VRAM item — schedule as hygiene.
5. MASTER ASSET STORE: the mip conversion AND the sky conversion exist only in the build
   area + Documents. Both must be run on the master store or a reinstall reverts them.

## 2026-08-04 overnight autonomous session: 16 of 19 at defaults, all 19 with the preset

User mandate (verbatim intent): perform all identified levers, retest all hub demos, and take
whatever steps needed — including fidelity knobs — so every demo runs on a 4 GB card.

### Levers shipped overnight (each verified before the next started)

| Lever | Where | Saving | Verification |
|---|---|---|---|
| Visibility payload skip | engine 1.97 | −39 on SSR levels | Z Island census: zero `res.texture_payload_*` rows, POLYS identical |
| Lazy debugUAV | engine 1.97 | −5.2 every level | zero `debugUAV` rows |
| Cloud-noise free path | engine 1.97 | −5.4 when clouds off | bind sites verified clouds-gated |
| CopyAllocator freelist trim + 16 MB rounding | engine 1.97 | ~250–283 **system RAM** every level | upload pool drains to zero after settle; buffers recreate on demand |
| **A4: SVT emissive map dropped** | engine 1.98 | **−96 every terrain level** (pool 576→480) | Grand Canyon: pool 480.0 exact, 8→6 sparse textures, POLYS bit-identical, **screenshot normal — no white-out** |
| B5 shadow cascade cap 1024 + SSR clamp | game, `lowvram=1` members | preset-only | applied at the visuals application points, cap-style |

**The A4 white-out is solved.** The 2026-08-02 failure was the UNBOUND emissive slot: an
undefined sparse sample fed the tonemapper. Binding an explicit `wi::texturehelper::getBlack()`
1×1 with residency/feedback descriptors −1 fixes it with zero visual change. Revert:
`svtemissive=1`.

**Designed but NOT shipped** (deliberate): ocean auto-gate (below-terrain-min water is
physics-safe to skip, but `Wicked_Update_Visuals` doesn't re-run on sculpt, so editor-dug holes
would show no water until the next visuals refresh — 28 MB on demos that already fit);
chunk VB/IB suballocation (the "−108" figure in earlier notes was stale — the real remaining
item is ~52 MB post-A2 via suballocating LIVE buffers, medium risk, floor follow-up #5);
streamout lazy-create (38–73 MB, needs its own session for BLAS-rebuild caveats).

### The final table — defaults, editor, engine `901da1f1`+1.98 / game `04:48` build

| Demo | driver | FLOOR | mesh | GRASS | TREES | content | misc | pad | nonres | FPS |
|---|---|---|---|---|---|---|---|---|---|---|
| The Mystery of Z Island | 3740 | 1225 | 512 | 378 | 75 | 744 | 199 | 164 | 527 | 105.3 |
| Operation Amazon | 3567 | 1232 | 512 | 342 | 77 | 635 | 106 | 269 | 480 | 91.5 |
| Aztec Game Kit | 3559 | 1177 | 896 | 226 | 81 | 476 | 155 | 141 | 496 | 99.6 |
| Canyon Offensive | 3252 | 1095 | 512 | 247 | 77 | 685 | 137 | 79 | 505 | 78.8 |
| River Raiders | 3217 | 1095 | 384 | 408 | 74 | 619 | 111 | 114 | 497 | 119.4 |
| RPG Template | 3179 | 1205 | 512 | 469 | 84 | 358 | 95 | 71 | 469 | 104.1 |
| Foggy Forest | 3085 | 1269 | 512 | 200 | 74 | 265 | 180 | 187 | 482 | 67.7 |
| Indian Strike Force | 3041 | 1104 | 512 | 1 | 71 | 438 | 182 | 229 | 589 | 106.6 |
| Disruption | 2986 | 1282 | 512 | 208 | 66 | 279 | 99 | 145 | 477 | 88.0 |
| Horseshoe Bend | 2984 | 1092 | 640 | 4 | 73 | 367 | 221 | 134 | 541 | 93.0 |
| Aztec Game Kit Teaser | 2952 | 1045 | 512 | 399 | 81 | 299 | 92 | 123 | 486 | 73.0 |
| Jungle Fever | 2849 | 1226 | 384 | 148 | 70 | 386 | 95 | 113 | 509 | 134.8 |
| Island Showdown | 2747 | 1156 | 512 | 66 | 85 | 330 | 129 | 75 | 477 | 77.3 |
| A Grand Canyon Adventure | 2730 | 1088 | 512 | 252 | 73 | 271 | 106 | 55 | 459 | 107.5 |
| Snowy Mountain Stroll | 2696 | 1155 | 384 | 96 | 68 | 333 | 101 | 130 | 504 | 130.0 |
| Bounty | 2677 | 1282 | 384 | 0 | 63 | 295 | 118 | 124 | 492 | 119.8 |
| Trapped | 2478 | 1082 | 384 | 0 | 63 | 435 | 65 | 66 | 457 | 145.2 |
| Escape from the Zombie Cellar | 2444 | 1147 | 384 | 5 | 63 | 235 | 102 | 131 | 451 | 136.9 |
| Switch Escape | 2345 | 1088 | 384 | 0 | 63 | 244 | 62 | 100 | 478 | 143.7 |
| **mean** | **2975** | **1160** | **492** | **182** | **73** | **405** | **124** | **129** | **493** | |
| min | 2345 | 1045 | 384 | 0 | 63 | 235 | 62 | 55 | 451 | |
| max | 3740 | 1282 | 896 | 469 | 85 | 744 | 221 | 269 | 589 | |

POLYS regression gate: 17 of 19 bit-identical vs the same-day pre-lever sweep; Horseshoe Bend
(−1.3 %) and Island Showdown (−0.03 %) differ only by roaming-NPC frustum sampling (no
geometry-touching change shipped; both sit 450+ MB under budget).

### The three stragglers under `lowvram=1` + `lowvramgrassdensity=75`

| Demo | defaults | with preset | headroom vs 3450 | FPS | POLYS |
|---|---|---|---|---|---|
| The Mystery of Z Island | 3740 | **3309 ✔** | 141 | 118.3 | bit-identical |
| Operation Amazon | 3567 | **3234 ✔** | 216 | 99.8 | bit-identical |
| Aztec Game Kit | 3559 | **3355 ✔** | 95 | 103.9 | bit-identical |

Preset = `lowvram=1` + `lowvramgrassdensity=75` (members now: grass dist cap 750, grass
density 75 %, shadow cascade cap 1024, SSR off). **Every hub demo is now measured inside a
4 GB card** — 16 at pure defaults, 3 with the preset.

### Where the campaign stands

From the 2026-08-02 baseline: mean driver **5082 → 2975 MB (−41 %)**, hub grass total
17,335 → 3,450 MB, FLOOR mean 1548 → 1160, misc mean 272 → 124, and mean FPS 92 → ~106.
Every one of the 19 hub demos now has a measured configuration inside a 4 GB card.

Master-store debt (IMPORTANT): the mip conversion (990 DDS) and the sky compression (8 cubes,
both skybank trees) live only in the build area + Documents. **Run both conversions on the
master asset store** or a reinstall silently reverts them. Scripts:
session scratchpad `list_singlemip.py` / `mipconvert.sh` / `skyconvert.sh`.

### A4 soak sign-off (the "with a soak" requirement, closed)

Fast-travel soak on Z Island, 3 laps, shipping 12288 atlas, on the 1.98 build:
**MIN_FREE_TILES = 881 of 2852 — the identical figure to the pre-A4 2026-08-02 soak.**
Zero starvation, FPS 106→96 across the eviction laps, lap-3 screenshots show fully
detailed terrain. A4 is now verified on every axis the original instruction demanded:
pool −96, residency identical, shading correct, POLYS bit-identical.

## 2026-08-05: WPE particle re-enable — regression check CLOSED, gate holds

The particle re-enable (engine 2.00–2.01) loads 26 `.PE` effects' textures and builds the
5-clone decal cache **at test-game start** — the tables above are EDITOR state and are
untouched (no `.PE` loads in the editor; SCENE_EMITTERS stays 0 there).

Same-build A/B in TEST-GAME state, `disablewparticlesystem=1` as the OFF lever, 10/10 runs
clean (SCENE_EMITTERS 95–98 on / 0 off validated the lever every time):

| demo | FPS on/off | driver on/off (MB) | Δdriver |
|---|---|---|---|
| TESTPRO1 | 43.5*/88.2 | 3283 / 3155 | +128 |
| Z Island | 115.5 / 115.4 | 3665 / 3522 | +143 |
| Aztec | 83.1 / 84.9 | 3741 / 3661 | +80 |
| Island Showdown | 95.6 / 95.3 | 3314 / 3154 | +160 |
| Trapped | 160.8 / 167.4 | 2710 / 2582 | +128 |

**VRAM verdict: particles cost a consistent ~+135 MB in test game. Worst absolute is Aztec
3741 at pure defaults — still inside 4096 with ~350 MB headroom. The 4 GB gate HOLDS.**

Attribution (per-resource census diff, TESTPRO1 + Z Island agree):
- **~81 MB: `EmittedParticleSystem` GPU buffers × ~96 emitters** (11 buffer types,
  ~0.85 MB/emitter, allocated even for paused+invisible cache clones). Lever if ever
  needed: lazy buffer creation on first burst (hitch risk — measure first) or smaller
  MAX_PARTICLES for burst-only effects.
- **~55 MB: `wpe*_color.png` textures load uncompressed** (5.4 MB each). Lever: BC7 like
  the skybank job. Parity-neutral as-is (DX11 loaded the same PNGs).
- `statisticsReadbackBuffer` ×2/emitter is READBACK heap = system RAM, not video.

*TESTPRO1's "43 FPS" was a run-order artifact, closed after 3 measurements: it was the
global FIRST run of the freshly linked build (cold driver/PSO caches; lazypso compiles on
first use), its OFF partner ran second and warm. A warm re-run reads 84.3 (GPU 7.42 ms,
all 98 emitters alive=0) and a 90 s timeline is flat 80–86 — within the documented ±8
launch-to-launch noise. All later A/B pairs ran warm on both sides and show no FPS gap.
Caveat: no harness run ever triggers the zone's area effect (player never moves), so
ACTIVE-effect render cost is still unmeasured — it belongs to the user's manual parity pass.
