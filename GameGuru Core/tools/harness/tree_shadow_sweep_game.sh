# Report #5 instrument, Test Game version: teleport the PLAYER straight back from one tree
# (SetFreezePosition / TransportToFreezePosition) and measure the tree crown's brightness.
# usage: tree_shadow_sweep_game.sh <shadowdist> <dStart> <dEnd> <dStep> <tag>
# Tree + line as tree_shadow_sweep.sh (TESTPRO2 > testpro2snow). MAX must be in Test Game.
# Projection calibrated from a frame at d=1600: f = 759 px, screen centre (768,432), the tree
# 12 px right of centre at 1600 (the pick point is ~25 u off the trunk axis), eye = posY + 19.
# Output: $OUT/<tag>.csv (d, crownLuma, eyeHeightAboveTreeBase) and $OUT/<tag>/d#####.png
source "$(dirname "$0")/lib.sh"
SD=$1; A=$2; B=$3; S=$4; TAG=$5
alive || { echo "MAX NOT RUNNING"; exit 1; }
send "SET_TREES shadowdist $SD" 10; echo
mkdir -p "$OUT/$TAG"; CSV="$OUT/$TAG.csv"; echo "d,luma,eyeH" > "$CSV"
for d in $(seq $A $S $B); do
  send "RUN_LUA local tx,tz=-1650,-1665; local ux,uz=0.7447,0.6674; local d=$d; local x,z=tx+ux*d,tz+uz*d; SetFreezePosition(x,GetTerrainHeight(x,z)+60,z); SetFreezeAngle(0,228.15,0); TransportToFreezePosition(); return 1" 10 >/dev/null
  sleep 1.8
  py=$(send "RUN_LUA return string.format('%.1f',g_PlayerPosY)" 10 | sed 's/^OK: //')
  f=$(shot "$TAG/d$(printf %05d $d)") || continue
  l=$(python -c "
from PIL import Image, ImageStat
a=Image.open(r'$(cygpath -w "$f")').convert('L'); d=$d; f=759.0
eyeH=($py+19.0)-3096.0
cx=768+12*1600.0/d
y0=432+f*(eyeH-380)/d; y1=432+f*(eyeH-120)/d
box=(int(cx-f*80/d),int(y0),int(cx+f*80/d),int(y1))   # crown core: +-80 u, 120..380 u above the base
print('%.2f,%.0f'%(ImageStat.Stat(a.crop(box)).mean[0],eyeH))")
  echo "$d,$l" >> "$CSV"
done
cat "$CSV" | tr '\n' ' '; echo
