#!/bin/bash
# GGMAX 2.29 — DUMP_RAGDOLL: name the dead stage in the ragdoll death chain.
#
# WHY A SCRIPT AND NOT AN EYEBALL: ragdoll only runs in TEST GAME (ragdollManager::Update is
# driven from BulletPhysics Update, which the editor does not step), and the harness provably
# cannot drive the player to shoot someone (2.14 walk-test note). RAGDOLL_TEST reproduces
# exactly what G-Entity_part1.cpp:722-732 does at the instant the death path picks ragdoll.
#
# The run is an A/B on ONE launch, because cross-launch comparison on this rig is worthless:
#   arm 1  writeback ON  (2.29 default) — the candidate fix
#   arm 2  writeback OFF (SET_RAGDOLLWRITEBACK 0) — reproduces 2.28's T-pose
#
# ⚠⚠ JUDGE FROM `RAGDOLL_BONE:` (HELD vs OVERWRITTEN), NOT FROM THE SCREENSHOTS. The 2.29 run on
# 2026-08-12 ragdolled a character who was nowhere near the camera: the before/after images
# differed by 0.018% of pixels and showed an empty cellar. That is not evidence the writeback did
# nothing — it is no evidence at all. The bone read-back compares what the writeback WROTE with
# what is live in the scene a moment later, and needs no camera. Keep the shots as context only.
# ⚠ Each arm ragdolls a DIFFERENT character (RAGDOLL_TEST skips anyone already ragdollified) —
# re-picking the same body makes the second arm a no-op behind BPhys_RagdollExist.
#
# Usage: ragdolltest.sh [demo]
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"
DEMO="${1:-Zombie Cellar}"
RES="$OUT/ragdoll_test.txt"; LOG="$OUT/ragdoll_test.log"
SHOTS="$OUT/ragdollshots"; mkdir -p "$SHOTS"

LOCK="$OUT/.ragdolltest.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: already running as PID $(cat "$LOCK")."; exit 3
fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
: > "$RES"; : > "$LOG"
say() { echo "$(date +%H:%M:%S) $*" >> "$LOG"; echo "$(date +%H:%M:%S) $*" >&2; }
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
# ⚠ SCREENSHOT lands in Files/screenshots/sc_<date>.png, NOT auto_screenshot.* next to the exe:
# runtime file writes use the game's Files/ CWD. The first 2.29 run collected nothing because of
# this — and the shots it did not collect would have been useless anyway (see below).
shot() { send "SCREENSHOT" 30 >/dev/null; sleep 2
  local src; src=$(ls -t "$D"/Files/screenshots/sc_*.png 2>/dev/null | head -1)
  [ -n "$src" ] && cp "$src" "$SHOTS/$1" && say "   shot -> $1"; }

say "=== ragdoll chain test: demo=$DEMO ==="
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
rm -f "$D/auto_command.txt" "$D/auto_result.txt"
( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & )
sleep 15
wait_state "hub" 90 || { say "FAIL_HUB"; exit 1; }
sleep 4
R=$(send "SELECT_DEMO $DEMO" 20); [[ "$R" == OK:* ]] || { say "FAIL_SELECT $R"; exit 1; }
sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 45
wait_state "editor" 240 || say "WARN not editor"

# Into test game — the ONLY mode where Bullet steps and ragdolls update.
say "-- entering test game: $(send "CLICK test_level" 30)"
wait_state "game" 300 || { say "FAIL_GAME_STATE"; }
sleep 20   # let the level settle and the AI spawn its characters

say "-- baseline (nothing ragdolled yet):"
send "DUMP_RAGDOLL" 30 | tee -a "$RES" | while read -r l; do say "   $l"; done
shot "00_before.png"

# ---- ARM 1: writeback ON (the 2.29 default) --------------------------------------------
send "SET_RAGDOLLWRITEBACK 1" 20 >/dev/null
echo "== ARM writeback=1 ==" >> "$RES"
say "-- RAGDOLL_TEST (writeback ON): $(send "RAGDOLL_TEST" 45)"
sleep 3;  shot "01_wb_on_t3.png"
sleep 5;  shot "02_wb_on_t8.png"
send "DUMP_RAGDOLL" 30 | tee -a "$RES" | while read -r l; do say "   $l"; done

# ---- ARM 2: writeback OFF (2.28 behaviour, the control) --------------------------------
send "SET_RAGDOLLWRITEBACK 0" 20 >/dev/null
echo "== ARM writeback=0 ==" >> "$RES"
say "-- RAGDOLL_TEST (writeback OFF): $(send "RAGDOLL_TEST" 45)"
sleep 3;  shot "03_wb_off_t3.png"
sleep 5;  shot "04_wb_off_t8.png"
send "DUMP_RAGDOLL" 30 | tee -a "$RES" | while read -r l; do say "   $l"; done

send "PRESS_ESCAPE" 20 >/dev/null; sleep 3
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
say "=== done — results in $RES, shots in $SHOTS ==="
cat "$RES"
