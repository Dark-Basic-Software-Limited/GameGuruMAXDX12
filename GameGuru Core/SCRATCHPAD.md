Do not run MAX or build anything yet. This is an investigation task only.

SHOT JUST NOW
I'm working in GameGuru MAX on a tropical island scene. When I zoom in close to the terrain, the ground textures are rendering as a patchwork of misaligned square tiles rather than blending smoothly. You can see distinct rectangular texture patches — some are a sandy/beige color and others are a grey rocky texture — laid out in a grid pattern with hard, straight edges between them. The tiles don't blend into each other at all; they look like a checkerboard of different terrain textures with visible seams at every boundary. It appears as though the terrain texture splatmap or blending is broken, causing each terrain chunk or tile to display a single flat texture rather than smoothly interpolating between painted layers. The overall effect looks like mismatched floor tiles rather than natural terrain. This is likely a terrain shader or texture blending issue where the blend map data isn't being sampled correctly, causing abrupt per-tile texture assignments instead of smooth per-pixel blending across the terrain surface.

LATEST VISUAL
 there's a visible hard seam where the grass texture meets the sand/beach texture. Instead of a smooth, natural blend between the two, you can see an abrupt transition line, particularly along the lower portion of the island where the green grass area meets the sandy shore. It looks like the terrain painting has lost its gradient, creating a sharp, unnatural boundary rather than the soft falloff you'd expect on a beach-to-grass transition. The original painted blend doesn't seem to be rendering correctly, leaving that telltale straight-edged seam between texture layers.

OLD REPORT
Your last change improved things — the terrain is no longer universally blurry, and some areas now show correct high-detail textures. But the result is a patchwork of detail levels with hard seams between them. Here is what I see:

The beach/sand area (bottom half of screen): Low detail. The rocky sand texture is visibly blurry and lacking fine detail. You can see the individual texture tiles are at a low mip level.
The grass area (center/upper portion): Noticeably higher detail than the sand. The grass texture is sharper and more defined. There is a hard, straight-line seam where the high-detail grass meets the low-detail sand — this is not a natural terrain blend, it's a virtual texture page boundary.
The transition between detail levels is abrupt, not gradual. You can trace a rectangular boundary where one page is high-res and the adjacent page is low-res. This means the page generation is producing some pages at the correct detail level but leaving neighboring pages at a lower mip, with no intermediate steps.
The camera is close to the ground (roughly player height, looking slightly down at the beach). At this distance, ALL visible terrain should be at or near the highest detail level. The sand area is close to the camera and should be just as sharp as the grass.

The problem is likely in how the CPU fallback decides which pages get which mip level. It seems to be generating high-detail pages for some quadrants or regions but not others, rather than radiating detail outward uniformly from the camera position. Check whether the page generation is using the camera's XZ position correctly to center the high-detail region, and whether there's an off-by-one or coordinate space mismatch causing it to only cover part of the visible area at high detail.
Append your findings and proposed fixes to SCRATCHPAD.md. Do not overwrite existing content. Do not make code changes yet — document what you find with file names and line numbers.

---

## Investigation: Texture Selection Seams at LOD Transitions (2026-02-21)

### Root Cause: LOD-Dependent Height Sampling in Page Generation

The terrain virtual texture system bakes material content (grass, sand, rock, etc.) into physical atlas pages during page generation. **Material selection depends on terrain height**, and **height is sampled from LOD-specific height maps** that have vastly different resolutions. When the render shader falls back to pages from different LOD levels, the baked material content differs because the height data used to generate each page was different.

### The Full Mechanism (4 steps)

**Step 1: Page generation samples height from LOD-specific height maps**

In `GGTerrain_DrawPages()` (`GGTerrain_part0.cpp:7552-7554`):
```cpp
int heightLevel = detailLevel;
if ( heightLevel > 5 ) heightLevel = 5;
if ( heightLevel > (int)numLODLevels-1 ) heightLevel = numLODLevels - 1;
```
This `heightLevel` is passed to the page generation pixel shader, which samples `texLODHeight` at that LOD slice (`GGTerrainPageGenPS.hlsl:143`):
```hlsl
float height = texLODHeight.Sample( samplerBiClamp, float3(IN.uv2, heightLevel) );
```

**Step 2: Different LOD height maps have vastly different resolutions**

Each LOD level's height map is 512x512 pixels (`GGTerrain_part0.cpp:3527-3528`), but covers a different world area (doubling per level). With `segments_per_chunk=64`, `segment_size=8.0`:

| heightLevel | segSize | World coverage | Resolution |
|---|---|---|---|
| 0 | 8 | 4,096 units | 8 units/pixel |
| 1 | 16 | 8,192 units | 16 units/pixel |
| 2 | 32 | 16,384 units | 32 units/pixel |
| 3 | 64 | 32,768 units | 64 units/pixel |
| 4 | 128 | 65,536 units | 128 units/pixel |
| 5 | 256 | 131,072 units | **256 units/pixel** |

The height data is NOT downsampled from finer LODs — each LOD independently evaluates terrain height at its own resolution (`GGTerrain_part0.cpp:3593-3631`, `UpdateLODTextures()`). This means a 50-unit beach-to-grass transition spans ~6 pixels at LOD 0 but less than 1 pixel at LOD 3.

**Step 3: Height drives material selection in the page gen shader**

In `GGTerrainPageGenPS.hlsl:222-231`:
```hlsl
for( i = 0; i < 5; i++ )
{
    float t = clamp( (height-terrain_layers[i].start)*terrain_layers[i].transition, 0.0, 1.0 );
    if ( t >= 1 ) finalSurface = SampleTexture2( terrain_layers[i].material, ... );
    else if ( t > 0 )
    {
        SurfaceValues layer = SampleTexture2( terrain_layers[i].material, ... );
        finalSurface = LerpSurface( finalSurface, layer, t );
    }
}
```
The blend factor `t` depends on the sampled `height`. If the height differs between LOD levels at the same world position, `t` differs, producing different material blending. This is baked permanently into the physical atlas page.

**Step 4: The render shader ties page selection to mesh LOD**

In `GGTerrainVirtualPBR_PS.hlsl:70-71`:
```hlsl
int detailLevel = texPageTableFinal.CalculateLevelOfDetailUnclamped( sampler1, IN.uv );
detailLevel = max( detailLevel, IN.lodLevel );
```
`IN.lodLevel` comes from the terrain mesh chunk's LOD level (`GGTerrainVS.hlsl:39`). This `max()` means the shader **never searches fine LOD pages for coarse mesh chunks**. At mesh LOD boundaries (where one chunk is LOD N and the adjacent chunk is LOD N+1), adjacent screen pixels find pages from different LOD levels. Those pages have different baked material content → **visible hard seam at the LOD boundary**.

### Why it correlates with the "mipmapping system"

The terrain mesh LOD system creates concentric rings of chunk detail around the camera. Each ring boundary is a potential material seam because:
- Inside the boundary: pages with heightLevel N, finer height data, one material blend
- Outside the boundary: pages with heightLevel N+1, coarser height data, slightly different material blend

The seams form straight lines aligned with chunk boundaries, exactly matching what the user observes: "hard, straight-line seam" and "transition in line with the mipmapping system."

### Quantifying the Height Discrepancy

Each page spans exactly ~2 pixels of its own LOD's height map (because both the page grid and height map have the same 512-unit logical resolution). But those 2 pixels represent different world-space sampling:

- LOD 0 page: 2 pixels × 8 units/pixel = 16 units of smooth height data
- LOD 1 page: 2 pixels × 16 units/pixel = 32 units of (different) height data
- LOD 3 page: 2 pixels × 64 units/pixel = 128 units of very coarse height data

For the default terrain layer settings (`layerStartHeight[0]=0, layerEndHeight[0]=100`), the sand-to-grass transition spans 100 height units. At LOD 0, this transition is smooth across 12+ height map pixels. At LOD 3, it's covered by ~1.5 pixels, collapsing to a near-binary switch.

### Proposed Fixes

**Option A (Recommended): Force consistent heightLevel for all pages**

In `GGTerrain_DrawPages()` at line 7552, instead of `heightLevel = detailLevel`, always use a fixed low LOD level for material height sampling. This ensures all pages agree on material selection regardless of their own LOD level.

```cpp
// Instead of:
int heightLevel = detailLevel;
if ( heightLevel > 5 ) heightLevel = 5;

// Use:
int heightLevel = 0; // Always use finest height map for material selection
// The do-while loop below still bumps up if UVs are out of range
```

The do-while loop at lines 7559-7575 already handles the case where the page falls outside the height map's coverage area by bumping to coarser levels. So pages near the camera (within LOD 0's 4096-unit coverage) would all use the same fine height data, producing consistent materials. Only very distant pages (outside LOD 0 coverage) would fall back to coarser height.

**Risk**: The LOD 0 height map only covers 4096 units centered on the camera. Pages for LOD 3+ (covering 32768+ units) would fall outside LOD 0 coverage, and the do-while loop would bump them up to an appropriate level. This is fine — the seams are only visible near the camera where LOD 0 coverage exists.

**Option B: Use the finest heightLevel that covers the page's world area**

Similar to A but more precise — start at heightLevel 0 and let the do-while loop find the finest level that provides valid UV coverage for that specific page. This is what the existing do-while loop already does, but starting from `detailLevel` instead of 0.

The change would be:
```cpp
int heightLevel = -1; // Start from 0 after the heightLevel++ in the loop
// Remove the lines: if (heightLevel > 5) heightLevel = 5;
```

**Option C: Pre-compute a material assignment texture**

Generate a single global texture that maps world position → material index at high resolution, computed once from the finest height data. The page gen shader reads this instead of computing height-based materials per-page. This decouples material selection from LOD entirely.

**Downsides**: Extra texture memory, needs updating when terrain is sculpted, more complex pipeline.

### Files and Key Line Numbers

| File | Lines | What |
|---|---|---|
| `GGTerrain_part0.cpp` | 7552-7554 | heightLevel = detailLevel (the problematic assignment) |
| `GGTerrain_part0.cpp` | 7559-7575 | do-while loop that bumps heightLevel for out-of-range UVs |
| `GGTerrain_part0.cpp` | 7585 | Packs heightLevel into vertex `id` field |
| `GGTerrain_part0.cpp` | 3527-3529 | Height/normal map creation (512x512 per LOD, independently sampled) |
| `GGTerrain_part0.cpp` | 3593-3631 | UpdateLODTextures: assembles height data from chunks per LOD |
| `GGTerrain_part0.cpp` | 9787-9790 | terrain_LOD constant buffer setup (used by render shader) |
| `GGTerrainPageGenPS.hlsl` | 140-141 | heightLevel/detailLevel unpacking from vertex data |
| `GGTerrainPageGenPS.hlsl` | 143-146 | Height/normal sampling from LOD height map |
| `GGTerrainPageGenPS.hlsl` | 222-246 | Height-based and slope-based material blending |
| `GGTerrainVirtualPBR_PS.hlsl` | 70-71 | detailLevel = max(calculated, meshLOD) — ties page selection to mesh LOD |
| `GGTerrainVirtualPBR_PS.hlsl` | 78-88 | Page table search loop (starts at detailLevel, skips finer pages) |
| `GGTerrainVS.hlsl` | 39 | IN.lodLevel = mesh chunk LOD level |

### Secondary Issue: Mask Texture UV Discontinuity at LOD Boundaries

The anti-tiling mask UVs in the page gen VS (`GGTerrainPageGenVS.hlsl:36`) are:
```hlsl
OUT.uv3 = IN.uv * 390 * terrain_maskScale;
```
`IN.uv` is the texture tiling UV, which depends on `pageTiling` (varies with `detailLevel` via `tilingScale`). At LOD boundaries, the mask rotation pattern changes, causing the same material to appear slightly different on each side. This is cosmetic (not wrong material selection) but compounds the visual seam from the height issue.

---

## Failed Fix Attempts and Lessons Learned (2026-02-21)

### Critical Constraint Discovered: Page Content Must Be Camera-Independent

The page shift mechanism (`ShouldShiftPages()` at `GGTerrain_part0.cpp:7916-8095`) shifts page table entries when the camera moves but **preserves the physical atlas page content** without regeneration. This means any page's baked material content must be **deterministic** — it must depend only on the page's own LOD level and virtual coordinates, never on the camera position. Any fix that makes `heightLevel` selection depend on camera position (even indirectly via LOD center offsets) will produce stale/inconsistent material content after camera movement.

### Attempt 1: `heightLevel = 0` (Option A — force finest height map)

**Change**: In `GGTerrain_DrawPages()`, set `heightLevel = 0` instead of `heightLevel = detailLevel` for both mip 0 and mip 1 passes.

**Result**: Near terrain looked better (consistent materials at nearby LOD boundaries). But after moving to the map edge and returning, distant terrain had wrong materials at coarser mip levels.

**Why it failed**: LOD 0's height map only covers 4096 units centered on the camera. For pages at LOD 3+ (covering 32768+ units), the do-while UV coverage loop bumps `heightLevel` up to a level that covers the page's world area. But which level it bumps to depends on the camera position (because each LOD's center tracks the camera). So the same page at the same virtual coordinate could get `heightLevel = 3` when the camera is at position A but `heightLevel = 4` when the camera is at position B. After page shift, the old content (baked with heightLevel 3) is reused in a position where heightLevel 4 would be selected, creating material mismatches.

### Attempt 2: `min(detailLevel, 2)` cap

**Change**: Capped heightLevel at 2 instead of using detailLevel directly. `heightLevel = min(detailLevel, 2)` for mip 0, `min(detailLevel + 1, 2)` for mip 1.

**Result**: Slightly better than attempt 1, but same corruption pattern appeared after moving to the map edge and returning — visible as a "patchwork of misaligned square tiles" with completely wrong per-tile material assignments.

**Why it failed**: Same root cause as attempt 1. LOD 3+ pages start the do-while loop at heightLevel 2, but LOD 2's height map (covering 16384 units) still doesn't cover distant pages. The loop bumps to heightLevel 3, 4, etc. based on camera position → camera-dependent → breaks on page shift.

### Attempt 3: Remove mesh LOD clamp in render shader

**Change**: Reverted all C++ changes. Changed `GGTerrainVirtualPBR_PS.hlsl:71` from `max(detailLevel, IN.lodLevel)` to `max(detailLevel, (int)terrain_detailLimit)`, removing the constraint that ties page selection to mesh LOD.

**Rationale**: If coarse mesh chunks could look up fine LOD pages (which share height data with nearby fine pages), the material seams at LOD boundaries would disappear because all visible pixels would use pages from similar LOD levels.

**Result**: Distant terrain textures were severely broken. Near-camera corruption also appeared after camera movement.

**Why it failed**: Without the mesh LOD clamp, distant terrain pixels search through many empty fine LOD page table entries (which were never generated for those world positions at fine LOD). The search loop either finds no page or finds an incorrect one, producing visual garbage. The clamp exists specifically to prevent coarse mesh chunks from requesting pages that don't exist at fine LOD levels.

### Key Takeaways

1. **The do-while UV coverage loop makes heightLevel camera-dependent** whenever it has to bump from the starting level. The loop checks whether the page's world coordinates fall within a given LOD's height map coverage, but that coverage area is centered on the camera. Any starting heightLevel below detailLevel will trigger the loop for distant pages, introducing camera-dependency.

2. **`max(detailLevel, IN.lodLevel)` in the render shader is load-bearing** — it prevents coarse mesh chunks from requesting fine LOD pages that don't exist, which would cause search failures and visual artifacts.

3. **The root problem is architectural**: height maps are LOD-specific and camera-centered, material selection happens per-page using those height maps, and the page shift system assumes content is position-independent. These three design decisions are fundamentally incompatible when heightLevel ≠ detailLevel.

### Remaining Fix Approaches

Given the constraints discovered, viable approaches must ensure that every page's material content is fully determined by (detailLevel, virtual_page_x, virtual_page_y) with NO dependency on camera position:

**Option C (Pre-computed material texture)**: A single camera-independent texture mapping world XZ → material blend factors. Generated once from the finest height data available globally (not LOD-specific). The page gen shader reads this instead of sampling LOD height maps. This completely decouples material selection from LOD.

**Option D (Unified height map)**: Create a single global height map at fixed resolution (independent of LOD centers), updated when terrain is sculpted. All LOD levels sample the same height data, just at different effective resolutions via bilinear filtering. This makes height sampling deterministic per world position regardless of camera.

**Option E (Accept the seam, minimize it)**: Keep `heightLevel = detailLevel` but reduce the visual impact by ensuring LOD height maps produce more consistent results. For example, generate coarse LOD height maps by downsampling from the finest LOD rather than independently evaluating terrain height. This won't eliminate the seam but should reduce the material discrepancy between adjacent LOD levels.

---

## Decision: Port to New Wicked Engine Terrain System (2026-02-21)

### Why we're abandoning virtual texture fixes

The LOD seam issue is **architectural** — the virtual texture system bakes material content into pages using LOD-specific height maps, and the page shift mechanism requires camera-independent content. These constraints are fundamentally incompatible. All attempted fixes (heightLevel=0, heightLevel cap, removing mesh LOD clamp) either broke distant terrain or introduced camera-dependent artifacts after page shifts. See "Failed Fix Attempts" above.

### New approach

Port the terrain rendering to the **new Wicked Engine DX12 terrain system** instead of continuing to patch the old virtual texture pipeline. The new engine terrain handles LOD transitions natively without a custom page-based virtual texture system.

### Reference Color Mode — Validation Tool for Port

Implemented a "reference color" rendering mode (`GGTERRAIN_SHADER_FLAG2_REFERENCE_COLOR`, 0x0080) that completely bypasses the virtual texture system and renders the terrain's raw material data as flat colors. This serves as the **ground truth** for validating the new terrain system during the port.

**What it shows**: Each material index maps to a unique flat color from a 32-entry palette. Unpainted areas derive their material from the same height/slope layer rules used by the page gen shader, but using mesh vertex data (`IN.worldPos.y`, `IN.normal.y`) instead of LOD height maps — so it's consistent at every distance.

**Toggle**: `R` key when `pref.iTerrainDebugMode` is active.

**Confirmed working**: 2026-02-21. Terrain paint data is visible as flat colors, no LOD seams at any distance.

**Files modified**:
- `GGTerrainConstants.hlsli` — added flag define
- `GGTerrainVirtualPBR_PS.hlsl` — texMaterialMap binding (t55), 32-color palette, early-out block with material map sampling + height/slope fallback + ambient lighting + editor overlays
- `GGTerrain_part0.cpp` — toggle variable, R key binding, flag bit OR, texMaterialMap bind in main draw
- `GGTerrain.h` — extern for toggle variable

**Use during port**: Run both the old terrain (reference color mode) and new Wicked Engine terrain side-by-side. Compare material assignments at matching world positions to verify the port preserves painted data, height-based layers, and slope-based layers correctly.
