#!/bin/bash
# Drive the screen (HUD) editor: open project -> edit In-Game HUD -> add a centred image widget
# pointed at imagebank\hud\ammo-health-panel.png -> report whether that image ACTUALLY LOADED.
#
# HUD_DUMP's `exist` column is the verdict: it calls ImageExist() on the same id the editor's draw
# path tests before it can blit (M-GridEditB_part22.cpp:938). exist=NO means the yellow selection
# box is empty on screen — a number rather than an eyeball on a screenshot.
#
# Usage: hudimage.sh [project] [imagepath]
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; SH="$OUT/hudshots"; mkdir -p "$SH"
LOG="$SH/hudimage.log"
PROJ="${1:-testpro2}"; IMG="${2:-imagebank\\hud\\ammo-health-panel.png}"

LOCK="$OUT/.hudimage.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: hudimage.sh already running as PID $(cat "$LOCK")."; exit 3
fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
: > "$LOG"
say() { echo "$(date +%H:%M:%S) $*" >> "$LOG"; echo "$(date +%H:%M:%S) $*" >&2; }
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
shot() { local r; r=$(send "SCREENSHOT" 40); sleep 2
  local p="${r#OK: Screenshot saved to }"
  if [ "$p" != "$r" ] && [ -f "$p" ]; then cp "$p" "$SH/$1.png"; say "    saved $1.png"
  else say "    !! screenshot failed: $r"; fi; }

say "=== $PROJ / image '$IMG' ==="
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
rm -f "$D/auto_command.txt" "$D/auto_result.txt" "$D/Files/hudwidgets.txt"
( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & )
sleep 15
wait_state "hub" 90 || { say "FAIL_HUB"; exit 1; }
sleep 4
say "open: $(send "OPEN_PROJECT $PROJ" 60)"
sleep 8
shot "01_storyboard"

say "edit: $(send "HUD_EDIT In-Game HUD" 40)"
sleep 4
shot "02_hud_editor"

say "before-add: $(send "HUD_DUMP" 40)"
[ -f "$D/Files/hudwidgets.txt" ] && cp "$D/Files/hudwidgets.txt" "$SH/widgets_BEFORE.txt"

say "add: $(send "HUD_ADD_IMAGE $IMG" 40)"
sleep 4
shot "03_image_added"

say "after-add: $(send "HUD_DUMP" 40)"
[ -f "$D/Files/hudwidgets.txt" ] && cp "$D/Files/hudwidgets.txt" "$SH/widgets_AFTER.txt"

say "DONE"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
