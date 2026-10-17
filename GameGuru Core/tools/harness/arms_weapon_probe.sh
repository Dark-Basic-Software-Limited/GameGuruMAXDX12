# GGMAX 3.122: the tester's set-up - a Player Start with a STARTING WEAPON (the Hands dropdown only shows
# for a new-weapon-system weapon) and an arm set, plus swimming.lua. Shows the starting weapon on dry land,
# then swims (the interactive swimming arms), then leaves the water (back to the weapon). Live-only.
#
# usage: arms_weapon_probe.sh <tag> "<start weapon, e.g. enhanced\AK>" "<arm set | none>"
source "$(dirname "$0")/lib.sh"
TAG="$1"; WEAPON="$2"; ARMS="$3"
if ! alive; then
  timeout 400 bash "$(dirname "$0")/open_project_level.sh" TESTPRO2 DX12TESTLEVEL | tail -1 || exit 1
  sleep 6
fi
if [ "$(send GET_STATE 10 | head -1)" == "STATE: game" ]; then send "PRESS_ESCAPE" 10 >/dev/null; wait_state editor 120; sleep 4; fi
send "SET_PLAYERWEAPON $WEAPON" 10; echo
send "SET_PLAYERARMS $ARMS" 10; echo
send "SET_ENTITYSCRIPT 3 global\\swimming.lua" 10 >/dev/null
send "CLICK test_level" 10 >/dev/null
wait_state game 180 || { echo "FAIL: no game state"; exit 1; }
sleep 8
# dry land: the first spot around the spawn whose ground is well above the water
LAND=$(send 'RUN_LUA local w=GetWaterHeight(); local px,pz=g_PlayerPosX,g_PlayerPosZ; for r=200,3000,200 do for a=0,330,30 do local x=px+r*math.cos(math.rad(a)); local z=pz+r*math.sin(math.rad(a)); local h=GetTerrainHeight(x,z); if h>w+120 then return x,h,z end end end; return px,g_PlayerPosY,pz' 10)
echo "land: $LAND"
read LX LY LZ <<< "$(echo "$LAND" | sed 's/^OK: //; s/ | / /g')"
send "RUN_LUA SetFreezePosition($LX,$LY+70,$LZ); SetFreezeAngle(10,0,0); TransportToFreezePositionOnly(); return 1" 10 >/dev/null
sleep 5
echo "on land: $(send DUMP_GUNARMS 10)"
send 'RUN_LUA return g_PlayerGunName' 10; echo
for i in 1 2; do shot "${TAG}_land$i" >/dev/null; sleep 0.4; done
# swim
send "RUN_LUA SetFreezePosition(3500,-80,7500); SetFreezeAngle(20,0,0); TransportToFreezePositionOnly(); return 1" 10 >/dev/null
sleep 4
send 'RUN_LUA SetGunAnimationSpeed(100); PlayGunAnimation(70,110); return g_PlayerGunName' 10; echo
for i in 1 2 3 4 5 6; do sleep 0.15; shot "${TAG}_swim$i" >/dev/null; done
echo "swimming: $(send DUMP_GUNARMS 10)"
# out of the water again
send "RUN_LUA SetFreezePosition($LX,$LY+70,$LZ); SetFreezeAngle(10,0,0); TransportToFreezePositionOnly(); return 1" 10 >/dev/null
sleep 6
send 'RUN_LUA return g_PlayerGunName' 10; echo
for i in 1 2; do shot "${TAG}_back$i" >/dev/null; sleep 0.4; done
echo "back on land: $(send DUMP_GUNARMS 10 | cut -c1-260)"
python - "$TAG" <<'EOF'
import os, sys
from PIL import Image
b = os.path.expanduser('~/ggmax_harness_out'); tag = sys.argv[1]
names = ['land1', 'swim3', 'swim4', 'swim5', 'back1', 'back2']
ims = [Image.open(os.path.join(b, '%s_%s.png' % (tag, n))).convert('RGB').resize((768, 432)) for n in names]
out = Image.new('RGB', (1536, 432 * 3))
for i, im in enumerate(ims): out.paste(im, ((i % 2) * 768, (i // 2) * 432))
out.save(os.path.join(b, '%s_grid.png' % tag)); print(os.path.join(b, '%s_grid.png' % tag))
EOF
