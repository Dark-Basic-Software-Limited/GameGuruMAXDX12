# GGMAX 3.117: interleaved same-session A/B of the grass far-field modes (1 = DX11 far field, 0 = pre-3.117
# strand LOD). MAX in the editor on a level with painted grass. Each round sets the mode, waits for the
# rebuild to land, then averages FPS over N one-second samples; strands built + VRAM ride along.
# usage: grass_farmode_ab.sh [rounds] [samples] ["x y z angx angy"] [drawdist]   (drawdist empty = level's own)
source "$(dirname "$0")/lib.sh"
ROUNDS="${1:-3}"; SAMPLES="${2:-10}"; CAM="${3:-226 476 83 17 336}"; DD="${4:-}"
alive || { echo "MAX NOT RUNNING"; exit 1; }
[ -n "$DD" ] && send "SET_GRASSPARAM drawdist $DD" 10 >/dev/null
send "SET_CAMERA $CAM" 10 >/dev/null; sleep 4
settle() { local i=0; while [ $i -lt 40 ]; do sleep 1; send GET_GRASSDENSITY 10 | grep -q "; 0 chunks still at an old density" && [ $i -ge 4 ] && return 0; i=$((i+1)); done; return 0; }
for r in $(seq 1 "$ROUNDS"); do
  for M in 1 0; do
    send "SET_GRASSFARMODE $M" 10 >/dev/null; settle; sleep 4
    sum=0; n=0
    for s in $(seq 1 "$SAMPLES"); do
      f=$(send GET_PERF_DATA 10 | grep "^FPS:" | awk '{print $2}')
      [ -n "$f" ] && { sum=$(awk -v a="$sum" -v b="$f" 'BEGIN{print a+b}'); n=$((n+1)); }
      sleep 1
    done
    built=$(send GET_GRASSDENSITY 10 | grep "built" | sed 's/.*built *: //; s/;.*//')
    vram=$(send GET_PERF_DATA 10 | grep "^VRAM_MB:" | awk '{print $2}')
    awk -v r="$r" -v m="$M" -v s="$sum" -v n="$n" -v b="$built" -v v="$vram" 'BEGIN{printf "round %d mode %d  FPS %.1f (%d samples)  VRAM %s MB  %s\n", r, m, (n>0?s/n:0), n, v, b}'
  done
done
send "SET_GRASSFARMODE 1" 10 >/dev/null
