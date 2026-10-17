# GGMAX 3.122: tester report "the interactive swimming arms ignore the player's arm set".
# Launches MAX on a level, sets the Player Start arm set LIVE (nothing saved), starts Test Game,
# equips the interactive swimming arms exactly as scriptbank/global/swimming.lua does, then
# screenshots and dumps the custom-arms chain (DUMP_GUNARMS).
#
# usage: arms_probe.sh <tag> "<arm set folder | none>" [project] [level]
source "$(dirname "$0")/lib.sh"
TAG="${1:-arms}"; ARMS="${2:-Female Dark}"; PROJ="${3:-TESTPRO2}"; LEVEL="${4:-DX12TESTLEVEL}"
if ! alive; then
  timeout 400 bash "$(dirname "$0")/open_project_level.sh" "$PROJ" "$LEVEL" | tail -1 || exit 1
  sleep 6
fi
echo "editor: $(send DUMP_GUNARMS 10 | cut -c1-400)"
send "SET_PLAYERARMS $ARMS" 10; echo
send "CLICK test_level" 10 >/dev/null
wait_state game 180 || { echo "FAIL: no game state"; exit 1; }
sleep 8
echo "game start: $(send DUMP_GUNARMS 10)"
shot "${TAG}_start" >/dev/null
# swimming.lua: AddPlayerWeapon / GetWeaponID / SetWeaponSlot(11) / ChangePlayerWeapon
send 'RUN_LUA local n="interactive"..string.char(92).."swimming"; AddPlayerWeapon(n); return GetWeaponID(n)' 10; echo
sleep 2
send 'RUN_LUA local n="interactive"..string.char(92).."swimming"; local id=GetWeaponID(n); SetWeaponSlot(11,id,id); ChangePlayerWeapon(n); return id' 10; echo
sleep 6
echo "swimming arms: $(send DUMP_GUNARMS 10)"
shot "${TAG}_swim" >/dev/null
echo "shots: $OUT/${TAG}_start.png $OUT/${TAG}_swim.png"
