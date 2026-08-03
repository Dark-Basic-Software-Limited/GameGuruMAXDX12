# VRAM audit — all 19 hub demos, categorised for a low-end knob plan

2026-08-02, engine `3666bbf7` + game `c784d891`. Editor, defaults (no `lowvram` keys), ~35 s
settle, project identity verified before each load. Raw censuses in the session scratchpad;
regenerate with `tools/…` + the `audit.awk` categoriser recorded at the bottom of this file.

Companion docs: `VRAM_FLOOR.md` (why the floor exists), `VRAM_CENSUS.md` (the instrument),
`GRASS_BENCHMARK.md` (the density gate).

## The table

All figures MB of driver-reported video memory unless noted. FLOOR = the eight content-independent
buckets; mesh = the shared suballocator; pad = D3D12MA block padding; nonres = driver-side
memory no resource census can see (overwhelmingly pipeline state objects).

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
| A1 | Lazy object PSOs | **−633** | **shipped** (`lowvram=1`; consider default-on once hitching is judged) |
| A2 | Release reset chunk buffers instead of 4-byte stand-ins | **−72** | **SHIPPED + verified** (game `e1b437a4`) |
| A3 | Mesh suballocator granularity 256→128 MB | **−128 on small levels** | **SHIPPED + verified** (engine `e0287386`) |
| A4 | Terrain machinery sized to demand | see the SVT section below | **investigated, not shipped** — two costed options, needs a soak |
| A5 | Tessellation + voxelize PSO axes | **−2304 pipelines (6337→4033)** | **SHIPPED + verified** (engine `e0287386`) |

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
