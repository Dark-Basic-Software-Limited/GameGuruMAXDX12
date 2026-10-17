# GGMAX 3.122b: does an ARM-SET change leave the weapon itself alone? In one Test Game (enhanced\AK start
# weapon): a script changes the AK's damage and clip capacity, the active arm set changes (SET_CUSTOMARMS_LIVE -
# what a standalone game's next level does), the player swaps away and back. The AK must come back with the new
# arms AND the script's values, its sound ids unchanged (no reload).
source "$(dirname "$0")/lib.sh"
alive || { timeout 400 bash "$(dirname "$0")/open_project_level.sh" TESTPRO2 DX12TESTLEVEL | tail -1; sleep 6; }
if [ "$(send GET_STATE 10 | head -1)" == "STATE: game" ]; then send "PRESS_ESCAPE" 10 >/dev/null; wait_state editor 120; sleep 4; fi
send 'SET_PLAYERWEAPON enhanced\AK' 10 >/dev/null
send "SET_PLAYERARMS Female Dark" 10 >/dev/null
send "CLICK test_level" 10 >/dev/null
wait_state game 180 || { echo "FAIL: no game state"; exit 1; }
sleep 8
SW='local n="interactive"..string.char(92).."swimming"; ChangePlayerWeapon(n); return g_PlayerGunName'
AK='local n="enhanced"..string.char(92).."ak"; ChangePlayerWeapon(n); return g_PlayerGunName'
ID='GetWeaponID("enhanced"..string.char(92).."ak")'
akdump() { send DUMP_GUNARMS 10 | grep -o "gun[0-9]* 'enhanced.ak' built='[^']*'[^|]*" | grep -o "built='[^']*'\|exist=[0-9]\|tex 'gamecore.hands[^']*'\|snd\[[0-9]*\]=[0-9]*" | tr '\n' ' '; }
echo "start            : $(akdump) | $(send "RUN_LUA local id=$ID; return GetWeaponDamage(id,0), GetWeaponClipCapacity(id,0)" 10)"
send "RUN_LUA local id=$ID; SetWeaponDamage(id,0,777); SetWeaponClipCapacity(id,0,99); return GetWeaponDamage(id,0), GetWeaponClipCapacity(id,0)" 10; echo "   <- script set damage/clip"
send "RUN_LUA $SW" 10 >/dev/null; sleep 1.5
send "SET_CUSTOMARMS_LIVE Knight Gauntlets" 10 >/dev/null
send "RUN_LUA $AK" 10 >/dev/null; sleep 2
echo "after arm change : $(akdump) | damage/clip $(send "RUN_LUA local id=$ID; return GetWeaponDamage(id,0), GetWeaponClipCapacity(id,0)" 10)"
send "RUN_LUA $SW" 10 >/dev/null; sleep 1.5
send "SET_CUSTOMARMS_LIVE none" 10 >/dev/null
send "RUN_LUA $AK" 10 >/dev/null; sleep 2
echo "after -> none    : $(akdump) | damage/clip $(send "RUN_LUA local id=$ID; return GetWeaponDamage(id,0), GetWeaponClipCapacity(id,0)" 10)"
shot arms_keepstate_none >/dev/null
