# GGMAX 3.122: the REAL swimming-arms flow, as a player meets it. Sets the Player Start arm set and
# attaches scriptbank/global/swimming.lua to one entity (both LIVE, nothing saved), starts Test Game,
# puts the player under deep water, and lets swimming.lua equip the interactive swimming arms itself.
#
# usage: arms_swim_probe.sh <tag> "<arm set folder | none>" [element for the script, default 3] [x y z]
source "$(dirname "$0")/lib.sh"
TAG="${1:-swim}"; ARMS="${2:-Female Dark}"; ELE="${3:-3}"
X="${4:-3500}"; Y="${5:--80}"; Z="${6:-7500}"
if ! alive; then
  timeout 400 bash "$(dirname "$0")/open_project_level.sh" TESTPRO2 DX12TESTLEVEL | tail -1 || exit 1
  sleep 6
fi
send "SET_PLAYERARMS $ARMS" 10; echo
send "SET_ENTITYSCRIPT $ELE global\\swimming.lua" 10; echo
send "CLICK test_level" 10 >/dev/null
wait_state game 180 || { echo "FAIL: no game state"; exit 1; }
sleep 8
echo "start: $(send DUMP_GUNARMS 10 | cut -c1-200)"
send "RUN_LUA SetFreezePosition($X,$Y,$Z); SetFreezeAngle(0,0,0); TransportToFreezePositionOnly(); return GetWaterHeight()" 10; echo
sleep 6
send "RUN_LUA return g_PlayerPosY, GetWaterHeight(), GetGamePlayerControlInWaterState(), g_PlayerGunName" 10; echo
echo "swim: $(send DUMP_GUNARMS 10)"
for i in 1 2 3 4; do shot "${TAG}_$i" >/dev/null; sleep 0.5; done
python - "$TAG" <<'EOF'
import os, sys
from PIL import Image
b = os.path.expanduser('~/ggmax_harness_out'); tag = sys.argv[1]
ims = [Image.open(os.path.join(b, '%s_%d.png' % (tag, i))).convert('RGB').resize((768, 400)) for i in (1, 2, 3, 4)]
out = Image.new('RGB', (1536, 800))
for i, im in enumerate(ims): out.paste(im, ((i % 2) * 768, (i // 2) * 400))
out.save(os.path.join(b, '%s_grid.png' % tag))
print(os.path.join(b, '%s_grid.png' % tag))
EOF
