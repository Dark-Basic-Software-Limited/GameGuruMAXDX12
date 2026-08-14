#!/bin/bash
# Reproduce "zoomed firing does no damage" and read the DUMP_SHOT verdict.
#
# Flow: open testpro2, enter test game (armed, barrel in the crosshair), ZOOM_FIRE (RMB held
# 120 frames, LMB pressed at frame 60 through the SHIPPED Lua input path), then DUMP_SHOT
# prints what the bullet ray actually did: hit / terrain / miss / SWALLOWED-by-object-N.
# An unzoomed FIRE_WEAPON control shot is taken FIRST so the same run shows both behaviours.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; SH="$OUT/zoomfire_out"; rm -rf "$SH"; mkdir -p "$SH"
LOG="$SH/zoomfire.log"
PROJ="${1:-testpro2}"

LOCK="$OUT/.zoomfire.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: already running as PID $(cat "$LOCK")."; exit 3
fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
: > "$LOG"
say() { echo "$(date +%H:%M:%S) $*" >> "$LOG"; echo "$(date +%H:%M:%S) $*" >&2; }
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
shot() { local r; r=$(send "SCREENSHOT" 40)
  local p="${r#OK: Screenshot saved to }"
  if [ "$p" != "$r" ] && [ -f "$p" ]; then cp "$p" "$SH/$1.png"; else say "  !! shot failed: $r"; fi; }

say "=== $PROJ zoom-fire probe ==="
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
rm -f "$D/auto_command.txt" "$D/auto_result.txt" "$D/Files/shottrace.txt"
( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & )
sleep 15
wait_state "hub" 90 || { say "FAIL_HUB"; exit 1; }
sleep 4
say "open: $(send "OPEN_PROJECT $PROJ" 60)"
sleep 6
send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 45
wait_state "editor" 240 || say "WARN not editor"
sleep 15
say "test game: $(send "CLICK test_level" 120)"
sleep 40
wait_state "game" 240 || { say "FAIL never reached game"; exit 1; }
sleep 12

# the probe FIRST: zoomed fire at the untouched barrel (the user's exact repro). If the fix
# works, THIS shot detonates it. The unzoomed control comes after, so pre/post-fix runs stay
# comparable: pre-fix the order was control-then-zoom (control had to prove the harness).
say "zoom fire: $(send "ZOOM_FIRE 120 60" 30)"
sleep 5
shot "01_zoomed"
sleep 3
say "zoom DUMP_SHOT: $(send "DUMP_SHOT" 30)"

# control shot, unzoomed
say "control fire: $(send "FIRE_WEAPON 3" 30)"
sleep 3
say "control DUMP_SHOT: $(send "DUMP_SHOT" 30)"
[ -f "$D/Files/shottrace.txt" ] && cp "$D/Files/shottrace.txt" "$SH/shottrace.txt"
shot "02_after"
say "DONE -> $SH"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
