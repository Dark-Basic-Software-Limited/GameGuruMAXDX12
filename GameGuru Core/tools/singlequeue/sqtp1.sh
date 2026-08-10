#!/bin/bash
# TESTPRO1 came back -2.5% (against a hub sweep that was 18/19 positive). That single result
# is now the pivot for whether the 2.17 default is right, so repeat it rather than act on one
# three-arm run. Five arms this time: 0/1/0/1/0 — two independent ON measurements against
# three OFF, which distinguishes a real regression from one unlucky arm.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; LOG="$OUT/sqtp1.log"; : > "$LOG"
LOCK="$OUT/.sqtp1.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then echo "REFUSING: running as $(cat "$LOCK")"; exit 3; fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
say(){ echo "$(date +%H:%M:%S) $*" | tee -a "$LOG"; }
send(){ rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive(){ tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state(){ local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
wait_stable(){ local maxs=${1:-300} t=0 pf=-1 pp=-1 good=0
  while [ $t -lt $maxs ]; do sleep 15; t=$((t+15)); alive || return 2
    local d=$(send "GET_PERF_DATA" 45)
    local f=$(echo "$d"|grep -m1 '^FPS:'|awk '{print $2}'); local p=$(echo "$d"|grep -m1 '^POLYS:'|awk '{print $2}')
    [ -z "$f" ] && continue
    if [ "$pf" != "-1" ] && [ "$p" == "$pp" ] && awk -v a="$pf" -v b="$f" 'BEGIN{exit !(a>0&&(b-a)/a<0.03&&(b-a)/a>-0.03)}'; then
      good=$((good+1)); [ $good -ge 2 ] && { say "  settled ${t}s fps=$f"; return 0; }
    else good=0; fi
    pf=$f; pp=$p; done; say "  WARN unsettled"; return 1; }
arm(){ send "$1" 30 >/dev/null; sleep 5; send "SET_SUBMITSTATS 1" 30 >/dev/null; sleep 45
  local d=$(send "GET_PERF_DATA" 45)
  echo "$(echo "$d"|grep -m1 '^FPS:'|awk '{print $2}')" ; }

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
rm -f "$D/auto_command.txt" "$D/auto_result.txt"
cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & sleep 15
wait_state "hub" 90 || { say "FAIL hub"; exit 1; }
sleep 4; say "open: $(send "OPEN_PROJECT TESTPRO1" 180 | head -c 80)"; sleep 15
say "level: $(send "CLICK_ONLY_LEVEL" 120 | head -c 80)"; sleep 50
wait_state "editor" 240 || say "WARN not editor"
wait_stable 300
O1=$(arm "SET_SINGLEQUEUE 0"); N1=$(arm "SET_SINGLEQUEUE 1")
O2=$(arm "SET_SINGLEQUEUE 0"); N2=$(arm "SET_SINGLEQUEUE 1")
O3=$(arm "SET_SINGLEQUEUE 0")
say "OFF: $O1 $O2 $O3   ON: $N1 $N2"
awk -v a="$O1" -v b="$O2" -v c="$O3" -v x="$N1" -v y="$N2" 'BEGIN{
  off=(a+b+c)/3; on=(x+y)/2;
  printf "TESTPRO1 5-arm: OFF mean %.1f (%.1f/%.1f/%.1f)  ON mean %.1f (%.1f/%.1f)  delta %+.1f%%  OFF spread %.1f\n",
    off,a,b,c,on,x,y,(on-off)/off*100,(c>a?c-a:a-c);}' | tee -a "$LOG"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
say "=== TP1 DONE ==="
