# GGMAX 3.119: Water Bake DX11 water fog, shot for shot. MAX in the editor on a level with water.
# usage: water_bake_fog_probe.sh [tag] ["x y z angx angy" (default: leave the camera)]
# Turns Terrain Bake + Water Bake on (in memory), then shoots: 0 = the 3.35f flat plane, 1 = DX11 water fog lit,
# 1u = unlit, a = opacity as grey (view 3), t = thickness / Water Fog Max as grey (view 4); live = the real ocean.
# DUMP_BAKE records the mode and the level's Water Fog values.
source "$(dirname "$0")/lib.sh"
TAG="${1:-wf}"; CAM="${2:-}"
alive || { echo "MAX NOT RUNNING"; exit 1; }
[ -n "$CAM" ] && send "SET_CAMERA $CAM" 10 >/dev/null
send "SET_BAKETERRAIN 1" 20 >/dev/null; send "SET_BAKEWATER 1" 20 >/dev/null; sleep 12
for m in "0 1 0:0" "1 1 0:1" "1 0 0:1u" "1 1 3:a" "1 1 4:t"; do
  send "SET_WATERBAKEDEPTH ${m%%:*}" 10 >/dev/null; sleep 3; shot "${TAG}_${m##*:}" >/dev/null
done
send "SET_WATERBAKEDEPTH 1 1 0" 10 >/dev/null
send DUMP_BAKE 20 | sed -n '/WATER BAKE/,$p' > "$OUT/${TAG}_bake.txt"
send "SET_BAKEWATER 0" 20 >/dev/null; sleep 8; shot "${TAG}_live" >/dev/null; send "SET_BAKEWATER 1" 20 >/dev/null
cat "$OUT/${TAG}_bake.txt"
