#!/bin/bash
# SVT atlas fast-travel soak. Usage: svtsoak.sh <atlasheight> <tag> [demo]
#   svtsoak.sh 16384 stock
#   svtsoak.sh 8192  half
#
# Route is the land-facing subset the probe proved demanding (the +-40000 diagonals sit over open
# ocean on Z Island and barely load tiles, so they are excluded). Three laps with short dwells so
# the atlas must evict and re-stream rather than accumulate. What matters is the MINIMUM of
# VT free: the halved atlas has 1922 tiles total, and the probe peaked at 1853 used.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12--claude-worktrees-determined-chebyshev-bf0892/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/svt"
H="$1"; TAG="$2"; DEMO="${3:-The Mystery of Z Island}"
mkdir -p "$OUT/$TAG"

send() {
  rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"
  local t=0; local timeout=${2:-30}
  while [ $t -lt $timeout ]; do
    [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; return 0; }
    sleep 1; t=$((t+1))
  done; echo "TIMEOUT"; return 1
}
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2; [ "$(send 'GET_STATE' 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
shot() { local r=$(send "SCREENSHOT" 25); local f=$(echo "$r"|sed -n 's/.*saved to //p'|head -1|tr -d '\r'); [ -n "$f" ] && cp "$f" "$OUT/$TAG/$1.png"; }
vtline() { send "GET_PERF_DATA" 40 | grep -m1 "^VT:" | tr -d '\r'; }
vtfree() { vtline | sed 's/.*free=\([0-9]*\).*/\1/'; }

# --- set the atlas height in setup.ini (append or replace) ---
sed -i '/^svtatlasheight=/d' "$D/setup.ini"
if [ "$H" != "16384" ]; then echo "svtatlasheight=$H" >> "$D/setup.ini"; fi
echo "setup.ini svtatlasheight -> ${H} ($(grep -c '^svtatlasheight=' "$D/setup.ini") key present)"

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
sleep 4
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 &
sleep 15
wait_state "hub" 60 || { echo "no hub"; exit 1; }
sleep 3
send "SELECT_DEMO $DEMO" 20 > /dev/null; sleep 3
send "CLICK edit_game" 20 > /dev/null; sleep 8
wait_state "storyboard" 40 || { echo "no storyboard"; exit 1; }
send "CLICK_ONLY_LEVEL" 20 > /dev/null
wait_state "editor" 180 || { echo "no editor"; exit 1; }
sleep 25

echo "SPAWN  $(vtline)"
send "GET_PERF_DATA" 40 | grep -E "^VRAM:|^FPS:" | tr -d '\r'
shot spawn

MINFREE=999999
WPS=("0 900 0" "-60000 900 0" "60000 900 0" "0 900 60000" "0 900 -60000" "-40000 900 -40000" "-30000 900 30000" "0 6000 0")
for lap in 1 2 3; do
  echo "--- lap $lap"
  i=0
  for P in "${WPS[@]}"; do
    set -- $P
    send "SET_CAMERA $1 $2 $3 15 45" 20 > /dev/null
    sleep 5
    F=$(vtfree)
    [ -n "$F" ] && [ "$F" -lt "$MINFREE" ] 2>/dev/null && MINFREE=$F
    echo "  lap$lap wp$i ($1,$3) free=$F"
    [ $lap -eq 3 ] && shot "lap3_wp$i"
    i=$((i+1))
  done
done

echo "FINAL  $(vtline)"
send "GET_PERF_DATA" 40 | grep -E "^VRAM:|^FPS:" | tr -d '\r'
echo "MIN_FREE_TILES=$MINFREE"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "SOAK_DONE_$TAG"
