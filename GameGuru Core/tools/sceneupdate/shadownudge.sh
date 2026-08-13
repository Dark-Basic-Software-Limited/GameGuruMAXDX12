#!/bin/bash
# Verify the 2.35 cached-local-shadow fix on the user's spotshadowtest scene (camera already
# framed on the ammo, so the shadow is actually legible — the Trapped attempt failed only because
# the pickup was ~4 px wide from across the room).
#
# Three shots, and the MIDDLE one is the point:
#   01_before   ammo present, shadow present
#   02_removed  caster removed -> if the shadow STAYS, the cached atlas is holding it (the bug)
#   03_nudged   InvalidateLocalShadows -> if the shadow GOES, that cache was provably the cause
# Shot 3 calls the same engine entry the shipped fix calls from the delete/collect paths, so this
# tests the real mechanism rather than a lookalike.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; SH="$OUT/pickupshots"; mkdir -p "$SH"
LOG="$SH/shadownudge.log"
PROJ="${1:-testpro2}"; TARGET="${2:-ammo}"

LOCK="$OUT/.shadownudge.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: shadownudge.sh already running as PID $(cat "$LOCK")."; exit 3
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

say "=== $PROJ / target '$TARGET' ==="
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
rm -f "$D/auto_command.txt" "$D/auto_result.txt"
( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & )
sleep 15
wait_state "hub" 90 || { say "FAIL_HUB"; exit 1; }
sleep 4
say "open: $(send "OPEN_PROJECT $PROJ" 60)"
sleep 5
send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 45
wait_state "editor" 240 || say "WARN not editor"
sleep 20

say "census: $(send "DUMP_BIGAABB" 90)"
say "BEFORE"; shot "01_before"

# Remove the caster via SetRenderable, not a transform poke. MOVE_ENTITY writes the Wicked
# transform directly and GameGuru re-asserts entity positions in the editor, so the box was still
# on the table afterwards. IsRenderable() is what the shadow caster gate actually reads
# (wiRenderer.cpp:8129), so hiding is both effective and the faithful stand-in for a delete — and
# it exercises the exact guard the cache's change detector skips at
# `if (!object.IsRenderable()) continue;`.
say "remove caster: $(send "SET_ENTITY_VIS $TARGET 0" 40)"
sleep 5
shot "02_removed"

say "nudge: $(send "INVALIDATE_LOCALSHADOWS" 40)"
sleep 5
shot "03_nudged"

say "DONE — 02 shadow present + 03 shadow gone == cached local atlas convicted"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
