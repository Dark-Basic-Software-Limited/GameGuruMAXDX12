#!/bin/bash
# Two questions the soak sweep raised but cannot answer, settled in ONE session.
#
# Q1 POLYS: 11 of 19 demos read LOWER than the C2 reference, all vegetation-heavy, while every
#    sparse/indoor level matched exactly. That is the signature of the tree pool still streaming
#    in - the documented 0826 failure - not of geometry loss. Test: sample POLYS out to 150 s and
#    see whether it climbs to the reference. Under-settling can only ever UNDER-count.
#
# Q2 VRAM: editor VRAM rose +90 MB per level loaded across the session (r2 0.71). But every level
#    was loaded exactly ONCE, so level content and load order are perfectly confounded. Test:
#    load A, B, then A and B AGAIN in the same session. If a level's own second load costs more
#    than its first, that is accumulation. If it costs the same, the climb was level content.
#
# A = Operation Amazon (soak 315943 vs ref 486602), B = RPG Template (351661 vs 540778).
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/settle0919"
mkdir -p "$OUT"; RES="$OUT/results.txt"; : > "$RES"
LOCK="$OUT/.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then echo "REFUSING"; exit 3; fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
say(){ echo "$(date +%H:%M:%S) $*" | tee -a "$RES"; }
send(){ rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0; local to=${2:-30}
  while [ $t -lt $to ]; do [ -f "$D/auto_result.txt" ] && { sleep 0.2; cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done
  echo TIMEOUT; return 1; }
alive(){ tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
killmax(){ local i=0; while [ $i -lt 10 ]; do taskkill.exe //IM GameGuruMAX.exe //F >/dev/null 2>&1; sleep 2; alive || return 0; i=$((i+1)); done; return 1; }
wait_state(){ local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }

load(){ send "NAVIGATE hub" 20 >/dev/null; sleep 6; send "NAVIGATE hub.demo_games" 20 >/dev/null; sleep 4
  send "SELECT_DEMO $1" 20 >/dev/null; sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 8
  wait_state "storyboard" 90 || return 1; send "CLICK_ONLY_LEVEL" 20 >/dev/null
  wait_state "editor" 420 || return 1; }

# Sample POLYS and the VRAM census on a settle ladder. POLYS is reported as the running MAX
# because under-settling can only under-count.
probe(){ # $1 = label
  local best=0
  for w in 30 30 30 30 30; do
    sleep $w
    local p=$(send "GET_PERF_DATA" 40 | tr -d '\r')
    local poly=$(echo "$p" | grep -m1 "^POLYS:" | awk '{print $2+0}')
    local cen=$(echo "$p" | grep -m1 "^VRAM:" | sed 's/.*census_mb=\([0-9.]*\).*/\1/')
    local res=$(echo "$p" | grep -m1 "^VRAM:" | sed 's/.*resources=\([0-9]*\).*/\1/')
    local dru=$(echo "$p" | grep -m1 "^VRAM:" | sed 's/.*driver_usage_mb=\([0-9.]*\).*/\1/')
    [ "${poly:-0}" -gt "$best" ] 2>/dev/null && best=$poly
    say "   $1 t+? polys=$poly (max $best) census=$cen res=$res driver=$dru"
  done
  echo "$best" > "$OUT/.lastpoly"
}

killmax; rm -f "$D/auto_command.txt" "$D/auto_result.txt"
( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & ); sleep 22
wait_state "hub" 180 || { say "FATAL no hub"; exit 1; }; sleep 5

for pass in 1 2; do
  for demo in "Operation Amazon" "RPG Template"; do
    say "=== pass $pass : $demo"
    if load "$demo"; then probe "$demo(p$pass)"; say "   PASS$pass $demo settled POLYS = $(cat "$OUT/.lastpoly")"
    else say "   LOAD FAILED"; fi
  done
done
send "QUIT" 10 >/dev/null 2>&1; t=0; while alive && [ $t -lt 25 ]; do sleep 1; t=$((t+1)); done; killmax
say "SETTLE+REPEAT DONE"
echo
echo "C2 reference: Operation Amazon 486602 | RPG Template 540778"
