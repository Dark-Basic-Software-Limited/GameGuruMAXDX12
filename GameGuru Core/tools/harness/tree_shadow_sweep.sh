# Report #5 instrument: dolly the editor camera straight back from one tree and measure the tree's
# on-screen brightness at each step, to find the distance where its shadowing pops.
# usage: tree_shadow_sweep.sh <shadowdist> <dStart> <dEnd> <dStep> <tag>
# The tree and the line of travel are fixed below (TESTPRO2 > testpro2snow, the pine in front of the
# player start, picked with PICK_AT). MAX must be in the editor with that level loaded.
# Output: $OUT/<tag>.csv  (d, treeLuma, shadowCasters) and $OUT/<tag>/d#####.png
source "$(dirname "$0")/lib.sh"
SD=$1; A=$2; B=$3; S=$4; TAG=$5
alive || { echo "MAX NOT RUNNING"; exit 1; }
send "SET_TREES shadowdist $SD" 10; echo
mkdir -p "$OUT/$TAG"; CSV="$OUT/$TAG.csv"; echo "d,luma,casters" > "$CSV"
for d in $(seq $A $S $B); do
  read x y z ax ay < <(python -c "
import math
tx,ty,tz=-1650.0,3096.0,-1665.0
ux,uz=0.7447,0.6674
d=$d; camy=ty+250; aim=ty+250
print('%.1f %.1f %.1f %.2f %.2f'%(tx+ux*d,camy,tz+uz*d,math.degrees(math.atan2(camy-aim,d)),228.15))")
  send "SET_CAMERA $x $y $z $ax $ay" 10 >/dev/null; sleep 1.5
  f=$(shot "$TAG/d$(printf %05d $d)") || continue
  c=$(send GET_PERF_DATA 10 | grep SHADOW_CASTERS | awk '{print $2}')
  l=$(python -c "
from PIL import Image, ImageStat
a=Image.open(r'$(cygpath -w "$f")').convert('L'); d=$d; f=584.0
box=(int(752-f*80/d),int(441-f*130/d),int(752+f*80/d),int(441+f*130/d))   # crown core: +-80 u, 120..380 u above the base
print('%.2f'%ImageStat.Stat(a.crop(box)).mean[0])")
  echo "$d,$l,$c" >> "$CSV"
done
cat "$CSV" | tr '\n' ' '; echo
