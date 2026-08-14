#!/bin/bash
# Verify the two Terrain Generator fixes on testpro2:
#   A. opening the generator from the empty "level 1" node shows TERRAIN in the preview
#      (screenshot; pre-fix the opaque ImGui window covered the whole viewport)
#   B. TERRAINGEN_GENERATE (the Generate button's countdown) does NOT crash
#      (pre-fix: guaranteed AV at M-TerrainNew_part5.cpp:3639) and produces
#      thumbbank\lastnewlevel.jpg + the Save As trigger.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; SH="$OUT/terraingen_out"; rm -rf "$SH"; mkdir -p "$SH"
LOG="$SH/terraingen.log"
PROJ="${1:-testpro2}"
NODE="${2:-level 1}"

LOCK="$OUT/.terraingen.lock"
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

say "=== $PROJ terrain generator probe (node: $NODE) ==="
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
rm -f "$D/auto_command.txt" "$D/auto_result.txt"
THUMB="$D/Files/thumbbank/lastnewlevel.jpg"
rm -f "$THUMB"
( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & )
sleep 15
wait_state "hub" 90 || { say "FAIL_HUB"; exit 1; }
sleep 4
say "open: $(send "OPEN_PROJECT $PROJ" 60)"
sleep 6
say "click node: $(send "CLICK_NODE $NODE" 60)"
# generator prepares a fresh flat level + random theme; give generation time to settle
sleep 45
say "state: $(send "TERRAINGEN_STATE" 30)"
shot "01_generator_preview"
sleep 2

say "generate: $(send "TERRAINGEN_GENERATE" 30)"
# pre-fix this crashed within 5 frames; give it a generous window then check liveness
sleep 15
if alive; then say "ALIVE after generate (pre-fix: crashed at M-TerrainNew_part5.cpp:3639)"; else say "!! PROCESS DIED after generate"; fi
say "state: $(send "TERRAINGEN_STATE" 30)"
shot "02_after_generate"
if [ -f "$THUMB" ]; then say "thumb written: $(stat -c%s "$THUMB") bytes"; cp "$THUMB" "$SH/lastnewlevel.jpg"; else say "!! no lastnewlevel.jpg"; fi
say "DONE -> $SH"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
