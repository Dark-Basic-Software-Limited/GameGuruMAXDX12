#!/bin/bash
# Measure Scene::Update CPU cost on one demo, for BUILD-vs-BUILD A/B.
#
# WHY NOT FPS: the editor frame on these levels is GPU-FENCE-BOUND (99.4% of frames stall a
# mean 0.89 ms on the frame fence), so a real CPU saving can show 0 FPS. The PRIMARY metric
# here is the Scene-S1..S5 CPU range sum in milliseconds, read from the profiler. FPS is
# captured too, but only as a secondary signal.
#
# Usage: sumeasure.sh <label> [demo] [samples]
#   label   - goes in the output filename, e.g. "base" / "sparse"
#   demo    - default "Switch Escape"
#   samples - profiler samples to average, default 6
#
# Output: tools/sceneupdate/su_<label>.txt  (summary)  + su_<label>_raw.txt (every dump)
#
# TRAPS THIS GUARDS (all cost real time on 2026-08-09, see tools/singlequeue/README.md):
#  - PID lockfile: pkill silently does nothing under Git Bash; leaked runners corrupt quietly.
#  - Settle gate, not a fixed soak: a fixed soak once sampled a level mid-load.
#  - Profiler ON in BOTH arms or the numbers are not comparable (it changes the frame).
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"
LABEL="${1:?need a label}"; DEMO="${2:-Switch Escape}"; NSAMP="${3:-6}"
RES="$OUT/su_${LABEL}.txt"; RAW="$OUT/su_${LABEL}_raw.txt"; LOG="$OUT/su_${LABEL}.log"

LOCK="$OUT/.sumeasure.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: sumeasure.sh already running as PID $(cat "$LOCK")."; exit 3
fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM

: > "$RES"; : > "$RAW"; : > "$LOG"
say() { echo "$(date +%H:%M:%S) $*" | tee -a "$LOG"; }
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
wait_stable() {
  local maxs=${1:-300} t=0 pf=-1 pp=-1 good=0
  while [ $t -lt $maxs ]; do
    sleep 15; t=$((t+15)); alive || return 2
    local d=$(send "GET_PERF_DATA" 45)
    local f=$(echo "$d" | grep -m1 '^FPS:' | awk '{print $2}')
    local p=$(echo "$d" | grep -m1 '^POLYS:' | awk '{print $2}')
    [ -z "$f" ] && continue
    if [ "$pf" != "-1" ]; then
      if [ "$p" == "$pp" ] && awk -v a="$pf" -v b="$f" 'BEGIN{exit !(a>0 && (b-a)/a<0.03 && (b-a)/a>-0.03)}'; then
        good=$((good+1)); [ $good -ge 2 ] && { say "  settled ${t}s fps=$f polys=$p"; return 0; }
      else good=0; fi
    fi
    pf=$f; pp=$p
  done
  say "  WARN never settled (last fps=$pf polys=$pp)"; return 1
}

# Pull one named CPU range's ms out of a PROFILER_DATA dump. Rows look like
#   "\tScene-S1 Anim+Transform: 0.91 ms"  and may carry an "(Nx)" hit-count marker.
row() { echo "$1" | grep -m1 -F "$2" | sed -n 's/.*: *\([0-9.]*\) ms.*/\1/p'; }

say "=== Scene::Update measurement: label=$LABEL demo=$DEMO samples=$NSAMP ==="
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
rm -f "$D/auto_command.txt" "$D/auto_result.txt"
cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 &
sleep 15
wait_state "hub" 90 || { say "FAIL_HUB"; echo "FAIL_HUB" >> "$RES"; exit 1; }
sleep 4
R=$(send "SELECT_DEMO $DEMO" 20); [[ "$R" == OK:* ]] || { say "FAIL_SELECT $R"; echo "FAIL_SELECT" >> "$RES"; exit 1; }
sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 50
wait_state "editor" 240 || say "WARN not editor"
wait_stable 300

# --- FPS first, WITHOUT the profiler (profiler changes the frame) ---
say "-- FPS (profiler off) --"
FPSS=""
for i in $(seq 1 3); do
  sleep 10; d=$(send "GET_PERF_DATA" 45)
  f=$(echo "$d" | grep -m1 '^FPS:' | awk '{print $2}')
  p=$(echo "$d" | grep -m1 '^POLYS:' | awk '{print $2}')
  o=$(echo "$d" | grep -m1 '^SCENE_OBJECTS:' | awk '{print $2}')
  say "  fps=$f polys=$p objects=$o"; FPSS="$FPSS $f"
  echo "FPS_SAMPLE $f POLYS $p OBJECTS $o" >> "$RES"
done

# --- CPU ranges WITH the profiler ---
say "-- profiler ranges --"
send "ENABLE_PROFILER" 30 >/dev/null; sleep 8
SUMS=""
for i in $(seq 1 "$NSAMP"); do
  sleep 4; d=$(send "GET_PERF_DATA" 60)
  echo "########## sample $i" >> "$RAW"; echo "$d" >> "$RAW"
  s1=$(row "$d" "Scene-S1"); s2=$(row "$d" "Scene-S2"); s3=$(row "$d" "Scene-S3")
  s4=$(row "$d" "Scene-S4"); s5=$(row "$d" "Scene-S5")
  cf=$(echo "$d" | grep -m1 '^CPU_FRAME_MS:' | awk '{print $2}')
  uw=$(row "$d" "Update - Wicked")
  tot=$(awk -v a="${s1:-0}" -v b="${s2:-0}" -v c="${s3:-0}" -v e="${s4:-0}" -v f="${s5:-0}" 'BEGIN{printf "%.3f", a+b+c+e+f}')
  say "  S1=$s1 S2=$s2 S3=$s3 S4=$s4 S5=$s5 -> SUM=$tot  CPU=$cf  UW=$uw"
  echo "STAGES $i S1=$s1 S2=$s2 S3=$s3 S4=$s4 S5=$s5 SUM=$tot CPU=$cf UW=$uw" >> "$RES"
  SUMS="$SUMS $tot"
done

# --- per-system shares (diagnostic; serialising inflates the total, compare shares only) ---
say "-- SET_SCENESERIAL 1 (shares only) --"
send "SET_SCENESERIAL 1" 30 >/dev/null; sleep 8
for i in $(seq 1 3); do
  sleep 4; d=$(send "GET_PERF_DATA" 60)
  echo "########## serial sample $i" >> "$RAW"; echo "$d" >> "$RAW"
  line=""
  for sys in Animation Physics Transform Hierarchy Mesh Material Object; do
    v=$(row "$d" "SU-$sys"); line="$line $sys=${v:-NA}"
  done
  say " $line"; echo "SERIAL $i$line" >> "$RES"
done
send "SET_SCENESERIAL 0" 30 >/dev/null
send "DISABLE_PROFILER" 30 >/dev/null

MEAN=$(echo "$SUMS" | awk '{s=0;n=0;for(i=1;i<=NF;i++){s+=$i;n++} if(n)printf "%.3f", s/n}')
FMEAN=$(echo "$FPSS" | awk '{s=0;n=0;for(i=1;i<=NF;i++){s+=$i;n++} if(n)printf "%.2f", s/n}')
say "=== RESULT $LABEL: Scene::Update mean = $MEAN ms | FPS mean = $FMEAN ==="
echo "RESULT label=$LABEL sceneupdate_mean_ms=$MEAN fps_mean=$FMEAN" >> "$RES"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
