#!/bin/bash
# Reproduce "collected pickup leaves its shadow behind" WITHOUT playing the level.
#
# SetEntityCollectedEx (DarkLUA_part0.cpp:1452) does not hide a collected entity — it teleports it
# by -999999 on every axis. MOVE_ENTITY applies exactly that, so the editor can trigger the bug in
# one command. Two arms, because there are two candidate mechanisms and they need separating:
#   MOVE arm  — the collect path's real effect; GGMAX 2.07d's dynamic-caster test should catch it
#   HIDE arm  — SetRenderable(false); 2.07d's loop SKIPS non-renderable objects at its first guard
# Shadow persisting in one arm and not the other names the mechanism.
#
# Usage: pickupshadow.sh <demo> <name-substr>
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; SH="$OUT/pickupshots"; mkdir -p "$SH"
LOG="$SH/pickupshadow.log"
DEMO="${1:-Trapped}"; TARGET="${2:-ammo}"

LOCK="$OUT/.pickupshadow.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: pickupshadow.sh already running as PID $(cat "$LOCK")."; exit 3
fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
: > "$LOG"
say() { echo "$(date +%H:%M:%S) $*" >> "$LOG"; echo "$(date +%H:%M:%S) $*" >&2; }
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
# NEVER discard the harness reply here: the first run of this script sent SCREENSHOT to
# /dev/null, no file appeared, and the reason was invisible. The reply carries the actual path.
shot() { local r; r=$(send "SCREENSHOT" 40); say "    SCREENSHOT -> $r"; sleep 2
  local p="${r#OK: Screenshot saved to }"
  if [ "$p" != "$r" ] && [ -f "$D/$p" ]; then cp "$D/$p" "$SH/$1.${p##*.}"; say "    saved $1.${p##*.}"
  elif [ "$p" != "$r" ] && [ -f "$p" ]; then cp "$p" "$SH/$1.${p##*.}"; say "    saved $1.${p##*.}"
  else say "    !! could not resolve screenshot path from reply"; fi; }

say "=== $DEMO / target '$TARGET' ==="
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
rm -f "$D/auto_command.txt" "$D/auto_result.txt" "$D"/auto_screenshot.*
( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & )
sleep 15
wait_state "hub" 90 || { say "FAIL_HUB"; exit 1; }
sleep 4
R=$(send "SELECT_DEMO $DEMO" 20); [[ "$R" == OK:* ]] || { say "FAIL_SELECT $R"; exit 1; }
sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 50
wait_state "editor" 240 || say "WARN not editor"
sleep 20

say "census (names the pickups): $(send "DUMP_BIGAABB" 90)"
[ -f "$D/Files/dumpbigaabb.txt" ] && cp "$D/Files/dumpbigaabb.txt" "$SH/${DEMO// /_}_bigaabb.txt"

say "BEFORE"
shot "01_before"

# --- MOVE arm: exactly what collection does ---
say "move: $(send "MOVE_ENTITY $TARGET -999999 -999999 -999999" 40)"
sleep 4
shot "02_after_move"
# EXECUTED-CHECK. The first run of this script compared two screenshots that turned out to be
# indistinguishable, and I could not tell "the move did nothing" from "the move worked but the
# object is 4 px wide at this camera distance". The census settles it without needing a camera:
# if the mesh really teleported to -999999 while the armature box stays at +100000, the inflated
# radius must EXPLODE (~86k -> ~550k). No change = the move never reached the scene.
say "post-move census: $(send "DUMP_BIGAABB" 90)"
[ -f "$D/Files/dumpbigaabb.txt" ] && cp "$D/Files/dumpbigaabb.txt" "$SH/${DEMO// /_}_bigaabb_AFTERMOVE.txt"

say "DONE — compare 01_before vs 02_after_move: is the shadow still on the surface?"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
