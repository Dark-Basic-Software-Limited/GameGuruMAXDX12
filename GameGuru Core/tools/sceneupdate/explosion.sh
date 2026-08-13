#!/bin/bash
# Open testpro2, enter test game (armed, facing an exploding barrel), fire, and capture the
# explosion frame by frame so the refraction artifact can be studied without a human at the mouse.
#
# FIRE_WEAPON drives the engine's own script-control hook (g.playeraction=1 -> firingmode=1,
# M-Physics_part1.cpp:218), which self-clears each frame, so one command is one trigger pull.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; SH="$OUT/explosionshots"; mkdir -p "$SH"
LOG="$SH/explosion.log"
PROJ="${1:-testpro2}"; SHOTS="${2:-14}"

LOCK="$OUT/.explosion.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: explosion.sh already running as PID $(cat "$LOCK")."; exit 3
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
  if [ "$p" != "$r" ] && [ -f "$p" ]; then cp "$p" "$SH/$1.png"; else say "    !! shot failed: $r"; fi; }

say "=== $PROJ explosion capture ==="
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
rm -f "$D/auto_command.txt" "$D/auto_result.txt"
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
say "armed and facing the barrel"
shot "00_before_fire"

# Burst, not a single round: an explosive barrel may need several hits, and the plume only lasts a
# second or two while each harness screenshot round-trip costs ~1s. Fire first, capture immediately
# after with no added sleeps, so the frames land inside the blast rather than after it.
for b in 1 2 3 4 5 6; do send "FIRE_WEAPON 3" 30 >/dev/null; done
say "burst of 6 fired"
say "fire: OK"
sleep 3
say "TRACE: $(send "DUMP_FIRE" 40)"
[ -f "$D/Files/firetrace.txt" ] && cp "$D/Files/firetrace.txt" "$SH/firetrace.txt"
# Capture the blast as fast as the harness round-trips. Each shot is ~1-2s of wall clock, so this
# samples the plume's life rather than a single instant -- the refraction artifact may only appear
# on particular frames, and one screenshot could easily miss or misrepresent it.
for i in $(seq 1 $SHOTS); do
  shot "$(printf '%02d_blast' $i)"
done
say "captured $SHOTS post-fire frames"

# A second shot, in case the first missed the barrel.
say "fire again: $(send "FIRE_WEAPON" 30)"
for i in $(seq 1 6); do shot "$(printf '%02d_blast2' $i)"; done

say "DONE -> $SH"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
