#!/bin/bash
# Three-arm WITHIN-LAUNCH A/B of a live Scene::Update knob, plus the arm-1 value as this
# build's Scene::Update baseline (comparable to su_base.txt from the previous build).
#
# Within-launch arms remove cross-launch drift (~2.5% on this rig) from the knob comparison.
# The A/A2 pair is the control: if A1 and A2 disagree by more than the A-vs-B gap, the result
# is noise. PRIMARY metric is the Scene-S1..S5 ms sum, not FPS (the frame is GPU-fence-bound).
#
# Usage: suarms.sh <label> <CMD_OFF> <CMD_ON> [demo] [samples-per-arm]
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"
LABEL="${1:?label}"; COFF="${2:?off cmd}"; CON="${3:?on cmd}"
DEMO="${4:-Switch Escape}"; NS="${5:-6}"
RES="$OUT/su_${LABEL}.txt"; LOG="$OUT/su_${LABEL}.log"; RAW="$OUT/su_${LABEL}_raw.txt"

LOCK="$OUT/.sumeasure.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING: another measurement running as PID $(cat "$LOCK")."; exit 3
fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
: > "$RES"; : > "$LOG"; : > "$RAW"
say() { echo "$(date +%H:%M:%S) $*" | tee -a "$LOG"; }
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
wait_stable() { local maxs=${1:-300} t=0 pf=-1 pp=-1 good=0
  while [ $t -lt $maxs ]; do sleep 15; t=$((t+15)); alive || return 2
    local d=$(send "GET_PERF_DATA" 45)
    local f=$(echo "$d"|grep -m1 '^FPS:'|awk '{print $2}'); local p=$(echo "$d"|grep -m1 '^POLYS:'|awk '{print $2}')
    [ -z "$f" ] && continue
    if [ "$pf" != "-1" ]; then
      if [ "$p" == "$pp" ] && awk -v a="$pf" -v b="$f" 'BEGIN{exit !(a>0&&(b-a)/a<0.03&&(b-a)/a>-0.03)}'; then
        good=$((good+1)); [ $good -ge 2 ] && { say "  settled ${t}s fps=$f polys=$p"; return 0; }
      else good=0; fi
    fi; pf=$f; pp=$p; done
  say "  WARN never settled"; return 1; }
row() { echo "$1" | grep -m1 -F "$2" | sed -n 's/.*: *\([0-9.]*\) ms.*/\1/p'; }

# one arm -> mean Scene::Update ms (+ logs each sample)
arm() {
  local name="$1" cmd="$2"
  say "-- arm $name: $cmd --"
  send "$cmd" 30 >/dev/null; sleep 6
  local sums=""
  for i in $(seq 1 "$NS"); do
    sleep 4; local d=$(send "GET_PERF_DATA" 60)
    echo "##### $name sample $i" >> "$RAW"; echo "$d" >> "$RAW"
    local s1=$(row "$d" "Scene-S1") s2=$(row "$d" "Scene-S2") s3=$(row "$d" "Scene-S3")
    local s4=$(row "$d" "Scene-S4") s5=$(row "$d" "Scene-S5")
    local cf=$(echo "$d"|grep -m1 '^CPU_FRAME_MS:'|awk '{print $2}')
    local pol=$(echo "$d"|grep -m1 '^POLYS:'|awk '{print $2}')
    local t=$(awk -v a="${s1:-0}" -v b="${s2:-0}" -v c="${s3:-0}" -v e="${s4:-0}" -v f="${s5:-0}" 'BEGIN{printf "%.3f",a+b+c+e+f}')
    say "   S1=$s1 S2=$s2 S4=$s4 SUM=$t CPU=$cf POLYS=$pol"
    echo "$name $i S1=$s1 S2=$s2 S3=$s3 S4=$s4 S5=$s5 SUM=$t CPU=$cf POLYS=$pol" >> "$RES"
    sums="$sums $t"
  done
  echo "$sums" | awk '{s=0;n=0;for(i=1;i<=NF;i++){s+=$i;n++} if(n)printf "%.3f",s/n}'
}

say "=== $LABEL : 3-arm within-launch A/B on $DEMO ==="
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
rm -f "$D/auto_command.txt" "$D/auto_result.txt"
cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 &
sleep 15
wait_state "hub" 90 || { say FAIL_HUB; exit 1; }
sleep 4
R=$(send "SELECT_DEMO $DEMO" 20); [[ "$R" == OK:* ]] || { say "FAIL_SELECT $R"; exit 1; }
sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 50
wait_state "editor" 240 || say "WARN not editor"
wait_stable 300

FPS0=$(send "GET_PERF_DATA" 45 | grep -m1 '^FPS:' | awk '{print $2}')
OBJ=$(send "GET_PERF_DATA" 45 | grep -m1 '^SCENE_OBJECTS:' | awk '{print $2}')
say "pre-profiler FPS=$FPS0 objects=$OBJ"
echo "PRE fps=$FPS0 objects=$OBJ" >> "$RES"

send "ENABLE_PROFILER" 30 >/dev/null; sleep 8
A1=$(arm A1 "$COFF")
B=$(arm  B  "$CON")
A2=$(arm A2 "$COFF")
send "DISABLE_PROFILER" 30 >/dev/null

say "=== $LABEL RESULT: A1=$A1  B=$B  A2=$A2 (Scene::Update ms) ==="
echo "RESULT label=$LABEL A1=$A1 B=$B A2=$A2 objects=$OBJ fps_pre=$FPS0" >> "$RES"
awk -v a="$A1" -v b="$B" -v c="$A2" 'BEGIN{
  am=(a+c)/2; if(am>0) printf "  knob delta = %+.3f ms (%+.1f%%) | A-vs-A2 control drift = %.3f ms\n", b-am, (b-am)/am*100, (a>c?a-c:c-a) }' | tee -a "$LOG"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
