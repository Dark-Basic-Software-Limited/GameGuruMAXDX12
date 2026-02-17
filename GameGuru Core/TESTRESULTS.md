# Full Demo FPS Test Results

## Test Info
- **Date:** 2026-02-17
- **Build:** Release (post Phase 2 DX12 shader port)
- **GPU:** AMD Radeon RX 9060 XT (RDNA 4)
- **Samples:** 10 per demo, 3 seconds apart (~30 seconds total)
- **Commit:** 215ed91b (main)

## Results: 19/19 Demos Tested, 18/19 Passed

| # | Demo | Best FPS | Avg FPS | Samples | Notes |
|---|------|----------|---------|---------|-------|
| 1 | Aztec Game Kit Teaser | 141.8 | 129.3 | 10 | |
| 2 | Aztec Game Kit | 64.8 | 48.6 | 9 | Slow shader warmup (samples 2-3 at 3 FPS) |
| 3 | Bounty | 111.8 | 108.3 | 10 | |
| 4 | Horseshoe Bend | 16.1 | 10.4 | 10 | Heavy scene, slow warmup |
| 5 | Island Showdown | 30.4 | 27.7 | 10 | Largest level - previously always timed out, now passes |
| 6 | Operation Amazon | 55.1 | 53.3 | 10 | |
| 7 | River Raiders | 59.2 | 57.4 | 10 | Sound crash on 1st run (dsutil.cpp:1018), passed on retest |
| 8 | Snowy Mountain Stroll | 32.0 | 26.1 | 10 | Slow warmup |
| 9 | A Grand Canyon Adventure | 60.3 | 57.9 | 10 | |
| 10 | Disruption | 86.9 | 84.8 | 10 | |
| 11 | Foggy Forest | 49.1 | 41.9 | 10 | |
| 12 | Indian Strike Force | 35.2 | 31.6 | 10 | |
| 13 | Switch Escape | 166.3 | 163.3 | 10 | |
| 14 | Canyon Offensive | 41.1 | 33.2 | 10 | Slow warmup |
| 15 | Escape from the Zombie Cellar | 60.0 | 59.9 | 10 | Vsync locked at 60 FPS (expected) |
| 16 | Jungle Fever | 102.4 | 100.9 | 10 | |
| 17 | RPG Template | 110.9 | 107.8 | 10 | |
| 18 | The Mystery of Z Island | 54.0 | 50.6 | 10 | |
| 19 | Trapped | 179.6 | 175.6 | 10 | |

## Comparison with Previous Runs

| Demo | Feb-15 Best | Feb-16 Best | Feb-17 Best | Delta (16->17) |
|------|-------------|-------------|-------------|----------------|
| Aztec Game Kit Teaser | 137.3 | 119.3 | 141.8 | +18.8% |
| Aztec Game Kit | 77.1 | 71.0 | 64.8 | -8.7% |
| Bounty | 147.5 | 129.3 | 111.8 | -13.5% |
| Horseshoe Bend | 22.3 | 20.7 | 16.1 | -22.2% |
| Island Showdown | FAIL | FAIL | 30.4 | NEW PASS |
| Operation Amazon | 83.1 | 70.8 | 55.1 | -22.2% |
| River Raiders | 90.0 | 76.9 | 59.2 | -23.0% |
| Snowy Mountain Stroll | 38.4 | 35.5 | 32.0 | -9.9% |
| A Grand Canyon Adventure | 84.1 | 73.1 | 60.3 | -17.5% |
| Disruption | 109.3 | 93.1 | 86.9 | -6.7% |
| Foggy Forest | 67.2 | 58.0 | 49.1 | -15.3% |
| Indian Strike Force | 46.8 | 42.6 | 35.2 | -17.4% |
| Switch Escape | 197.4 | 164.3 | 166.3 | +1.2% |
| Canyon Offensive | 59.8 | 53.9 | 41.1 | -23.7% |
| Escape from the Zombie Cellar | 60.0 | 60.0 | 60.0 | 0.0% |
| Jungle Fever | 142.3 | 120.6 | 102.4 | -15.1% |
| RPG Template | 163.6 | 132.1 | 110.9 | -16.0% |
| The Mystery of Z Island | 75.1 | 64.5 | 54.0 | -16.3% |
| Trapped | 228.3 | 169.4 | 179.6 | +6.0% |

## Observations

### FPS Trends
- Most demos show a ~15-20% FPS decrease compared to Feb-16 results
- This is likely due to cumulative shader/lighting changes during the DX12 port
- Island Showdown now passes (previously always timed out) with the increased load timeouts
- Three demos (Aztec Teaser, Switch Escape, Trapped) actually improved vs Feb-16
- Zombie Cellar remains vsync-locked at 60 FPS as expected

### Crash During Testing
- **River Raiders** caused a crash on the first test run at `dsutil.cpp:1018` (CSound::GetBuffer)
- This is a pre-existing sound system bug (null `this` pointer), not related to DX12 rendering
- The crash killed the app mid-test, causing demos 8-19 to fail in the first run
- On retest with a fresh app launch, River Raiders passed cleanly at 59.2 FPS
- Previous crash entries in Guru-Crash.log (from earlier sessions) are at different locations:
  - `M-TerrainNew_part5.cpp:3575` (terrain, pre-existing)
  - `wiGraphicsDevice.h:254` (GPU allocation, pre-existing)

### Other Known Issues
- Two pre-existing DX12 `CreateTexture E_INVALIDARG` errors still appear in engine log (benign, from engine's own RenderPath3D)
- GPU particles remain disabled (not yet DX12 compatible)
- Custom terrain/grass/tree draw functions not yet integrated into render pipeline (Phase 3 TODO)
