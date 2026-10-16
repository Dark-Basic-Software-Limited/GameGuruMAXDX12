# GGMAX 3.117: grass FAR-FIELD falloff, measured. MAX in the editor on a level with painted grass.
# usage: [RELAX=20] grass_falloff_probe.sh [tag] [lod_dist] ["x y z angx angy"]
# Shots: far mode 2 (DX11 rings, no dissolve = the baseline), 1 (DX11 far field), 0 (pre-3.117 strand LOD),
# and a Grass Off shot for the ground. grass_falloff_measure.py turns screen pixels into camera distances
# (GET_GROUNDDIST_GRID: a ray-march against the terrain per screen cell) and prints coverage per distance band,
# the Boolean-linearised survival -ln(1-c)/-ln(1-c_baseline) for modes 1 and 0, and DX11's model curve.
# ★ settle() waits for the REBUILD, not just "0 stale chunks": the stale count reads 0 until the density latch
# moves (15 still frames), so a shot right after a draw-distance change caught the OLD density (3.117 gfpre:
# mode 1 read 1.9x the baseline). In modes 1/2 every system must also carry the latched tier-3 count.
# ★ Then RELAX: blades a mode switch re-shows restart their physics and settle over ~15 s (the static-skip drops
# to its wind cadence after 60 frames) - a shot 10 s in read up to +0.04 coverage in the near-mid bands.
# ★ The bands use the EFFECTIVE draw distance (Low VRAM caps it - GET_GRASSDENSITY "effective").
source "$(dirname "$0")/lib.sh"
TAG="${1:-gf}"; LOD="${2:-1500}"; CAM="${3:-226 712 83 12 66}"; RELAX="${RELAX:-20}"
alive || { echo "MAX NOT RUNNING"; exit 1; }
send "SET_GRASSOFF 0" 10 >/dev/null
send "SET_GRASSPARAM drawdist $LOD" 10 >/dev/null
send "SET_CAMERA $CAM" 10 >/dev/null; sleep 4
settle() {
  local mode="$1" i=0 ok=0 d t3 mn mx st
  while [ $i -lt 60 ]; do
    sleep 1; i=$((i+1))
    d=$(send GET_GRASSDENSITY 10)
    st=$(echo "$d" | sed -n 's/.*; \([0-9]*\) chunks still at an old density.*/\1/p')
    t3=$(echo "$d" | sed -n 's/.*per system: tier3 \([0-9]*\),.*/\1/p')
    mn=$(echo "$d" | sed -n 's/.*(min \([0-9]*\), max \([0-9]*\) per system).*/\1/p')
    mx=$(echo "$d" | sed -n 's/.*(min \([0-9]*\), max \([0-9]*\) per system).*/\2/p')
    if [ "$st" = "0" ] && { [ "$mode" = "0" ] || { [ "$mn" = "$t3" ] && [ "$mx" = "$t3" ]; }; }; then ok=$((ok+1)); else ok=0; fi
    [ $i -ge 6 ] && [ $ok -ge 3 ] && return 0
  done
  echo "WARNING: mode $mode did not settle (stale=$st tier3=$t3 min=$mn max=$mx)" >&2; return 0
}
for M in 2 1 0; do
  send "SET_GRASSFARMODE $M" 10 >/dev/null; settle $M; sleep "$RELAX"
  shot "${TAG}_m$M" >/dev/null
done
send "SET_GRASSFARMODE 1" 10 >/dev/null
send GET_GRASSDENSITY 10 > "$OUT/${TAG}_density.txt"
send "GET_GROUNDDIST_GRID 64 40" 30 > "$OUT/${TAG}_grid.txt"   # true per-pixel ground distance (ray-march)
send "SET_GRASSOFF 1" 10 >/dev/null; sleep 4; shot "${TAG}_off" >/dev/null; send "SET_GRASSOFF 0" 10 >/dev/null
EFF=$(sed -n 's/.*effective \([0-9.]*\) (lowvram.*/\1/p' "$OUT/${TAG}_density.txt" | head -1)
EFF="${EFF%.*}"
[ -n "$EFF" ] && [ "$EFF" != "$LOD" ] && echo "NOTE: draw distance $LOD is capped to $EFF (Low VRAM) - measuring against $EFF"
PY="$(cygpath -w "$(dirname "$0")/grass_falloff_measure.py")"
python "$PY" "$(cygpath -w "$OUT")" "$TAG" "${EFF:-$LOD}"
