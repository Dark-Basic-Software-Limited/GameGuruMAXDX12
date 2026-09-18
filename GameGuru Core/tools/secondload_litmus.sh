#!/bin/bash
# SECOND-LOAD LITMUS (GGMAX 3.53). The 19-demo sweep relaunches MAX per demo, so it can NEVER see a
# bug that needs two levels in one process - which is how the 2026-09-18 blank second level lived for
# four weeks under 19/19 green. Run this after any change near level load, teardown, trees, or the
# render path: River Raiders -> hub -> Island Showdown -> editor renders -> Test Game renders, N times.
#   bash "GameGuru Core/tools/secondload_litmus.sh" <outdir> [N=2]
# -> editor renders -> CLICK test_level -> game renders. Prints a table; PASS only if every
# iteration passes both stages. No checkouts, no builds - the exe in the build area as it is.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$1"; N="${2:-2}"; mkdir -p "$OUT"
OVERLAY='PREPARING|RASTERIZING|BUILDING|GENERATING|[0-9]+.?[0-9]*00 Complete'
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0; local to=${2:-30}
  while [ $t -lt $to ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; return 1; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
killmax() { local i=0; while [ $i -lt 10 ]; do taskkill.exe //IM GameGuruMAX.exe //F >/dev/null 2>&1; sleep 2; alive || return 0; i=$((i+1)); done; return 1; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
wait_prep_clear() { local t=0; while [ $t -lt $1 ]; do alive || return 2
  send "GET_SCREEN_TEXT" 25 | grep -qiE "$OVERLAY" || return 0; sleep 5; t=$((t+6)); done; return 1; }
load_demo() { send "NAVIGATE hub.demo_games" 20 >/dev/null; sleep 3; send "SELECT_DEMO $1" 20 >/dev/null; sleep 3
  send "CLICK edit_game" 20 >/dev/null; sleep 8; wait_state "storyboard" 60 || return 1
  send "CLICK_ONLY_LEVEL" 20 >/dev/null; wait_state "editor" 300 || return 1; }
shot() { send "SCREENSHOT" 60 >/dev/null; sleep 4; local n=$(ls -t "$D/screenshots/"*.png "$D/Files/screenshots/"*.png 2>/dev/null | head -1); [ -n "$n" ] && cp -f "$n" "$OUT/$1.png"; wc -c < "$OUT/$1.png" 2>/dev/null || echo 0; }
fps() { send "GET_PERF_DATA" 40 | grep -m1 '^FPS:' | awk '{print $2}'; }
echo "exe: $(ls -la --time-style=+%m-%d\ %H:%M "$D/GameGuruMAX.exe" | awk '{print $6,$7}')"
echo "iter | editor shot | editor FPS | game reached | prep | game shot | game FPS | verdict"
ALLPASS=1
for i in $(seq 1 "$N"); do
  killmax; rm -f "$D/auto_command.txt" "$D/auto_result.txt"
  cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 &
  sleep 20; wait_state "hub" 120 || { echo "  $i | BOOT FAIL"; ALLPASS=0; continue; }; sleep 4
  load_demo "River Raiders" >/dev/null 2>&1 || { echo "  $i | river load FAIL"; ALLPASS=0; continue; }
  sleep 15; send "NAVIGATE hub" 20 >/dev/null; sleep 8
  load_demo "Island Showdown" >/dev/null 2>&1; sleep 30
  EF=$(fps); EB=$(shot "prove${i}_editor"); EFI=${EF%.*}; EFI=${EFI:-0}
  ED=FAIL; [ "$EB" -gt 1000000 ] && [ "$EFI" -gt 100 ] && ED=ok
  GR=no; PC=-; GB=0; GF=""; GM=skipped
  if [ "$ED" = ok ]; then
    send "CLICK test_level" 15 >/dev/null; sleep 12
    if wait_state "game" 120; then GR=yes; wait_prep_clear 240; PC=$?; sleep 15; GF=$(fps); GB=$(shot "prove${i}_game")
      GM=FAIL; [ "$GB" -gt 1000000 ] && GM=ok
    else GM=NOGAME; fi
  fi
  V=FAIL; [ "$ED" = ok ] && [ "$GM" = ok ] && V=PASS; [ "$V" = PASS ] || ALLPASS=0
  printf "  %d  | %10s | %10s | %12s | %4s | %9s | %8s | %s\n" "$i" "$EB" "${EF:-?}" "$GR" "$PC" "$GB" "${GF:-?}" "$V"
  send "QUIT" 10 >/dev/null 2>&1; t=0; while alive && [ $t -lt 20 ]; do sleep 1; t=$((t+1)); done; killmax
done
[ "$ALLPASS" = 1 ] && echo "PROVE: PASS ($N/$N)" || echo "PROVE: FAIL"
