# GGMAX 3.118: grass on BARE ground (sand/rock/mud: materials with no matching grass), measured. MAX in the editor.
# usage: grass_bare_probe.sh [tag] ["patchX patchZ"] [material+1 (default 4 = dry sand)] ["x y z angx angy"]
# In memory only - nothing is saved. Paints a bare-material patch (PAINT_TEST), then runs every path that writes
# grass there, auditing the map (GET_GRASSMAP_AUDIT: the "bare ground" row's course(2) column is the bug) and
# shooting the view after each:
#   1 sand painted (MATERIAL only: PAINT_TEST drives GGTerrain_Update_Painting alone, so grass already in the
#     patch is NOT re-resolved the way the editor's Match Painted Grass stroke would - leftover types show here)
#   2 Populate Vegetation Everywhere with Match Terrain Color (every cell auto -> resolved by the ground: the
#     3.118 resolve fix - pre-3.118 wrote Course Grass on the sand)
#   3 Grass brush, Match Terrain Color ON, Course Grass (the 3.118 brush fix - pre-3.118 wrote Course Grass)
#   4 Grass brush, Match Terrain Color OFF (deliberate Course Grass on sand: DX11 allows it, so it must grow)
#   5 GRASS_CLEAR_BARE (the opt-in repair) - clears the step-4 grass again
source "$(dirname "$0")/lib.sh"
TAG="${1:-gb}"; PATCH="${2:--99 814}"; MAT="${3:-4}"; CAM="${4:-226 476 83 17 336}"
alive || { echo "MAX NOT RUNNING"; exit 1; }
send "SET_CAMERA $CAM" 10 >/dev/null; sleep 6
bare() { send "GET_GRASSMAP_AUDIT 4" 60 | grep "bare ground" | sed 's/  */ /g'; }
step() { sleep "$2"; shot "${TAG}_$1" >/dev/null; echo "$1: $(bare)"; }
step 0_start 2
send "SET_TERRAINBRUSH 450" 10 >/dev/null
send "PAINT_TEST $PATCH $MAT 60" 10 >/dev/null
step 1_sand 8
send "GRASS_POPULATE 1" 10 >/dev/null
step 2_populate_match 20
send "GRASS_PAINT_TEST $PATCH 40 1 1" 10 >/dev/null
step 3_brush_match 10
send "GRASS_PAINT_TEST $PATCH 40 0 1" 10 >/dev/null
step 4_brush_nomatch 10
echo "repair dry run: $(send 'GRASS_CLEAR_BARE 0' 120)"
echo "repair: $(send 'GRASS_CLEAR_BARE 1' 120)"
step 5_repaired 12
