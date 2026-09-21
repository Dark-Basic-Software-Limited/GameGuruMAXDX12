# Which levels exceed the 4 GB video-memory limit — 2026-09-21

Build: game `3ae218ae`, engine `5e6ac8f7`. Measured on the DESKTOP (RX 9060 XT), 5.2 h since boot,
same boot session for every run below so the columns are comparable.

Four runs: two fresh-launch 19-demo sweeps, two single-session 19-demo soaks, plus a targeted
named-census probe. Raw data in `tools/sweep_0921*.txt`, `tools/soak_0921*.txt`,
`tools/vram_census_rt377.txt`.

---

## The short answer

**No level exceeds 4 GB on its own. Three exceed it once another level has been loaded first —
and that is the case that matters, because it is what a shipped multi-level game does.**

| | result |
|---|---|
| fresh launch, load one level | **0 of 19 over 4096 MB.** Worst: Aztec Game Kit **3782.7 MB**, 313 MB spare |
| one process, several levels | **3 of 19 over**, identically in both soak runs |

---

## 1. Fresh launch — the min-spec criterion

Worst of the editor and in-game readings, across both sweeps. This models a player who launches the
game and loads one level.

| demo | worst MB | headroom |
|---|---|---|
| Aztec Game Kit | 3782.7 | 313.3 |
| The Mystery of Z Island | 3682.1 | 413.9 |
| Operation Amazon | 3452.1 | 643.9 |
| Canyon Offensive | 3327.2 | 768.8 |
| RPG Template | 3306.9 | 789.1 |
| River Raiders | 3283.0 | 813.0 |
| Aztec Game Kit Teaser | 3168.2 | 927.8 |
| Horseshoe Bend | 3087.6 | 1008.4 |
| Indian Strike Force | 3049.6 | 1046.4 |
| Jungle Fever | 3019.6 | 1076.4 |
| Foggy Forest | 2993.9 | 1102.1 |
| Snowy Mountain Stroll | 2932.1 | 1163.9 |
| A Grand Canyon Adventure | 2868.8 | 1227.2 |
| Island Showdown | 2876.1 | 1219.9 |
| Disruption | 2809.1 | 1286.9 |
| Bounty | 2666.1 | 1429.9 |
| Trapped | 2360.8 | 1735.2 |
| Escape from the Zombie Cellar | 2290.4 | 1805.6 |
| Switch Escape | 2238.3 | 1857.7 |

**None over 4096.** In-game is the worse reading on 17 of 19.

⚠ **Error bars.** The two sweeps are the same build in the same boot session, so their difference is
pure instrument noise: **spread 85.3 MB**, median −0.1. Treat any single-demo figure as ±~40 MB and
do not read a 20 MB difference as meaning anything.

---

## 2. One process, several levels — where it breaks

Both soaks load all 19 demos in one MAX launch. **The same three demos exceed 4096 in both runs, at
the same positions:**

| demo | load position | soak A | soak B | over by |
|---|---|---|---|---|
| Aztec Game Kit | **1** | 4159.2 | 4112.5 | +63 / +17 |
| Operation Amazon | 5 | 4198.6 | 4170.9 | +103 / +75 |
| **The Mystery of Z Island** | 17 | **4382.5** | **4386.6** | **+287 / +291** |

They are the three largest levels. They go over because of what was loaded before them.

### ★ The residue is a STEP, not a drift

Same level, fresh vs in-session:

- mean residue **+609.6 MB**
- regression against load position: slope only **+24.3 MB per load, r² 0.253** — weak
- **the SECOND level loaded already carries +353 MB**

So this is not slow accumulation that only a 19-level marathon reaches. **Loading one extra level
costs most of it.** A player who loads a level is fine; a player who finishes level 1 and starts
level 2 is carrying several hundred megabytes they never get back.

---

## 3. Is it real data, or just allocator pooling?

`gvram` is *driver-reported* usage, which counts free space pooled inside D3D12MA blocks. That is
not a capacity risk. So the same demo was censused twice in one process — loaded first, then again
after three other levels (`tools/vram_residue_probe.sh`):

| | Z Island first | after 3 levels | delta |
|---|---|---|---|
| live resources | 6696 | 6842 | **+146** |
| `census_bytes` (live allocations) | 3128.0 MB | 3430.1 MB | **+288 MB** |
| `d3d12ma_blocks` | 3379.4 MB | 3720.7 MB | +325 MB |
| `driver_usage` | 3670.6 MB | **3957.6 MB** | +287 MB |

**`census_bytes` rose with it. This is live data, not pooling.** And note the last row: after only
*three* intervening levels Z Island is at 3957.6 MB — **138 MB from the limit**.

### What is actually retained (named, from the census diff)

| bytes | count | what |
|---|---|---|
| +128 MB | 3 → 4 | `GPUSubAllocator` — one more 128 MB pool block (reusable, not content) |
| +65 MB | 1 → 1 | `shadowMapAtlas` grew and did not shrink back |
| **+65 MB** | **0 → 15** | **`terraintextures/mat6, mat7, mat29, mat30, mat31` Color/Normal/Surface** |
| +17 MB | 33 → 34 | `frame_allocator` |
| ~+28 MB | — | `Scene::geometry/instance/material` buffers grew to the largest level |
| +9 MB | +133 | small unnamed buffers |

★★★ **The one that is a defect rather than growth: fifteen terrain material textures belonging to
OTHER levels are still resident.** `mat6/7/29/30/31` are not Z Island's — Z Island loaded first did
not have them. They come from Aztec Game Kit and Operation Amazon and survive the return.

They are loaded through the engine **resource manager**, which caches by filename for the process
(`GGTerrain_part0.cpp:6786` notes the shipping Wicked terrain loads `terraintextures/matN` that way
for its own atlas). Nothing evicts them at level unload.

★ **This is the eighth instance of the shape in [[project_vram_retention]]: per-level content held
in a process-lifetime cache.** The other seven were closed in September; this one is the same
family and was not on the list.

The `shadowMapAtlas` +65 MB is a *known* open item — notes §3.69d, `GG_ArmShadowAtlasShrink()` wants
moving to the END of the level load. It is already written up and is not new.

---

## 4. What I would do, in order

1. **Evict unreferenced `terraintextures/matN` at level load.** ~65 MB for three levels, and it
   scales with how many distinct terrain material sets a session touches. ⚠ Needs care: the
   resource manager is shared, so eviction must be by reference rather than by wiping the cache, or
   levels that share a material set will reload and hitch.
2. **Move `GG_ArmShadowAtlasShrink()` to the end of the load** (§3.69d, one line) — recovers the
   +65 MB atlas ratchet on this path too.
3. **Re-measure with the same probe.** If 1 and 2 land, Z Island after three levels should come back
   toward its fresh 3682 MB, and the three over-limit demos should clear.

⚠ **Do not read the +128 MB `GPUSubAllocator` block as a leak** — that is the allocator taking
another pool, and it will be reused. It inflates `driver_usage` without costing capacity.

---

## 5. Honest limits of this report

- One machine, one GPU (RX 9060 XT, 16 GB). A real 4 GB card behaves differently under pressure: the
  driver evicts rather than failing, so the symptom there is a stall or a hitch, not this number.
- The editor and Test Game were measured; **standalone was not**, and standalone is known to hold
  textures at full resolution where the editor reduces them ([[project_standalone_vram]], +950 to
  +1520 MB). **The numbers here are therefore a FLOOR for the shipped game, not a ceiling.**
- Every figure is at the levels' start cameras. A player walking into a denser part of a level will
  read higher.
