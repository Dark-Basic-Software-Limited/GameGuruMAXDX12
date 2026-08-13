#!/bin/bash
# Sample the shadow quantity/resolution from all three visuals structs in the EDITOR and again in
# TEST GAME. The editor shows 8 spot / 16 point; test game shows 0 / 0. Reading the call sites did
# not settle which copy is overwritten, because visuals <-> gamevisuals are copied both ways and
# save/load handles both fields. Whichever struct changes between the two samples names the stage.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; LOG="$OUT/shadowqty.log"
DEMO="${1:-Snowy Mountain Stroll}"

LOCK="$OUT/.shadowqty.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: shadowqty.sh already running as PID $(cat "$LOCK")."; exit 3
fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
: > "$LOG"
say() { echo "$*" >> "$LOG"; echo "$*" >&2; }
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }

say "=== $DEMO ==="
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
rm -f "$D/auto_command.txt" "$D/auto_result.txt"
( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & )
sleep 15
wait_state "hub" 90 || { say "FAIL_HUB"; exit 1; }
sleep 4
R=$(send "SELECT_DEMO $DEMO" 20); [[ "$R" == OK:* ]] || { say "FAIL_SELECT $R"; exit 1; }
sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 50
wait_state "editor" 240 || say "WARN not editor"
sleep 20

say ""; say "---------- A: EDITOR ----------"; say "$(send "DUMP_SHADOWQTY" 40)"

say ""; say "---------- entering test game ----------"
# 'PLAY_GAME' does not exist — the real lever is CLICK test_level (WETEST:248), and my first run
# silently sampled the EDITOR twice because the unknown command just returned an error. Gate on the
# state actually reaching 'game' rather than trusting a sleep.
say "$(send "CLICK test_level" 120)"
sleep 45
wait_state "game" 240 || say "WARN never reached test game"
say "state now: $(send "GET_STATE" 30 | head -1)"
say ""; say "---------- B: TEST GAME ----------"; say "$(send "DUMP_SHADOWQTY" 40)"

# Visual confirmation of the thing actually reported: the on-screen panel. TOGGLE_PROFILER cycles
# g.tabmode 0->1->2, and mode 1 is the in-game Visuals panel where Shadow Quantity lives.
# tabmode 1 came up as the PERFORMANCE panel on this build despite the label, so cycle until the
# Visuals panel (which owns Shadow Quantity) is the one on screen, shooting each step.
say ""; say "tab1: $(send "TOGGLE_PROFILER" 30)"; sleep 3
say "tab2: $(send "TOGGLE_PROFILER" 30)"; sleep 4
R=$(send "SCREENSHOT" 40); sleep 2
P="${R#OK: Screenshot saved to }"
if [ "$P" != "$R" ] && [ -f "$P" ]; then cp "$P" "$(dirname "$0")/shadowqty_panel.png"; say "  saved shadowqty_panel.png"; else say "  !! screenshot failed: $R"; fi

say ""; say "DONE — the struct whose numbers differ between A and B is the culprit"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
