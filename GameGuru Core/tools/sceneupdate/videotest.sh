#!/bin/bash
# Verify the 2.50 video pipeline on DX12: load+play a real tutorial .mp4 through the SAME chain
# the thumbnail click uses, then poll VIDEO_STATUS. Pass = view handle non-null (decoded frames
# reaching a GPU texture) AND percent advancing between polls.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; SH="$OUT/videotest_out"; rm -rf "$SH"; mkdir -p "$SH"
LOG="$SH/videotest.log"
# ⚠ MUST be an ABSOLUTE Windows path: LoadVideo hands it to Media Foundation's
# CreateObjectFromURL, which rejects relative paths; the failure path raises a MODAL
# RunTimeError that hangs a headless run (cost this probe its first attempt).
VID="${1:-D:\\DEV\\BUILD\\GameGuru Wicked MAX Build Area\\Max\\Files\\tutorialbank\\0101-getting-started.mp4}"

LOCK="$OUT/.videotest.lock"
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

say "=== video pipeline probe: $VID ==="
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
rm -f "$D/auto_command.txt" "$D/auto_result.txt"
( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & )
sleep 15
wait_state "hub" 90 || { say "FAIL_HUB"; exit 1; }
sleep 4
say "load+play: $(send "VIDEO_TEST $VID" 40)"
sleep 4
say "status t+4s : $(send "VIDEO_STATUS" 30)"
sleep 4
say "status t+8s : $(send "VIDEO_STATUS" 30)"
sleep 4
say "status t+12s: $(send "VIDEO_STATUS" 30)"
say "DONE -> $SH"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
