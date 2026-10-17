# GGMAX 3.122: does an arm-set REBUILD leak? In one Test Game, with enhanced\AK as the start weapon, cycle the
# active arm set mid-game (SET_CUSTOMARMS_LIVE - what a standalone game's next level does) and swap between the
# swimming arms and the AK so BOTH rebuild every cycle. A control phase repeats the swaps with the set unchanged
# (no rebuilds). Scene object / mesh / material / animation counts and the VRAM census (resources, MB) must not
# grow with the number of rebuilds.
#
# usage: arms_rebuild_stress.sh [cycles, default 12]
source "$(dirname "$0")/lib.sh"
N="${1:-12}"
alive || { timeout 400 bash "$(dirname "$0")/open_project_level.sh" TESTPRO2 DX12TESTLEVEL | tail -1; sleep 6; }
if [ "$(send GET_STATE 10 | head -1)" == "STATE: game" ]; then send "PRESS_ESCAPE" 10 >/dev/null; wait_state editor 120; sleep 4; fi
send 'SET_PLAYERWEAPON enhanced\AK' 10 >/dev/null
send "SET_PLAYERARMS Female Dark" 10 >/dev/null
send "CLICK test_level" 10 >/dev/null
wait_state game 180 || { echo "FAIL: no game state"; exit 1; }
sleep 8
SW='local n="interactive"..string.char(92).."swimming"; ChangePlayerWeapon(n); return g_PlayerGunName'
AK='local n="enhanced"..string.char(92).."ak"; ChangePlayerWeapon(n); return g_PlayerGunName'
counts() { send GET_PERF_DATA 10 | tr '\n' ' ' | sed -n 's/.*SCENE_OBJECTS: \([0-9]*\) SCENE_MESHES: \([0-9]*\) SCENE_MATERIALS: \([0-9]*\).*SCENE_ANIMATIONS: \([0-9]*\).*census_mb=\([0-9.]*\).*resources=\([0-9]*\).*/objects=\1 meshes=\2 materials=\3 anims=\4 census_mb=\5 resources=\6/p'; }
swap() { send "RUN_LUA $SW" 10 >/dev/null; sleep 1.5; send "RUN_LUA $AK" 10 >/dev/null; sleep 1.5; }
swap; swap
echo "start          : $(counts)"
for i in $(seq 1 $N); do swap; done
echo "control x$N     : $(counts)   (set unchanged - no rebuilds)"
SETS=("Knight Gauntlets" "none" "Female Dark")
for i in $(seq 1 $N); do
  s="${SETS[$(( (i - 1) % 3 ))]}"
  send "SET_CUSTOMARMS_LIVE $s" 10 >/dev/null
  swap
done
echo "rebuild x$N     : $(counts)   (both guns rebuilt every cycle)"
d=$(send DUMP_GUNARMS 10)
echo "final arms     : $(echo "$d" | grep -o "customArms='[^']*'") | $(echo "$d" | grep -o "gun[0-9]* 'enhanced.ak' built='[^']*'[^|]*" | grep -o "built='[^']*'\|exist=[0-9]\|tex 'gamecore.hands[^']*'\|snd\[[0-9]*\]=[0-9]*" | tr '\n' ' ') | swim $(echo "$d" | grep -o "gun[0-9]* 'interactive.swimming' built='[^']*'[^|]*" | grep -o "built='[^']*'\|exist=[0-9]\|tex 'gamecore.hands[^']*'" | tr '\n' ' ')"
for i in $(seq 1 $N); do swap; done
echo "settle x$N      : $(counts)   (set unchanged again)"
shot arms_stress_ak >/dev/null
