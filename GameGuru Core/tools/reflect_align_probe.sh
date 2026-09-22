#!/bin/bash
# GGMAX 3.80: the planar reflection does not line up with the geometry above it (Lee, TESTPRO2 /
# testpro2level - crate strut vs its reflection in the Puddle).
#
# ★ The point of this probe is the OFFSET SIGNATURE, which is what separates the candidate causes:
#   - mirror plane at the wrong HEIGHT  -> the gap at the contact point is 2*(plane error), the
#     SAME for every object and every camera. Moving the camera does not change it.
#   - sampling UV displaced (bumpColor / half-texel) -> the whole reflection slides, and the slide
#     is in SCREEN space, so it changes as the camera moves and as the surface fills more screen.
#   - clip-plane mismatch -> the break appears ONLY at the contact point and nowhere else.
# So: same scene, several cameras, and the entity facts. One run answers which family it is.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${GGMAX_REFLECT_OUT:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/reflect}"
LABEL="${1:-before}"
mkdir -p "$OUT"

send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { sleep 0.2; cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
shot() { local line; line=$(send "SCREENSHOT x" 40); local p
  p=$(echo "$line" | sed 's/^OK: Screenshot saved to //' | tr '\\' '/')
  if [ -f "$p" ]; then cp "$p" "$OUT/${LABEL}_$1.png"; echo "   -> ${LABEL}_$1.png"
  else echo "   SHOT FAILED: $line"; fi }

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 &
sleep 25
wait_state hub 90 || { echo FAIL_HUB; exit 1; }
sleep 4
send "OPEN_PROJECT TESTPRO2" 30 >/dev/null; sleep 6
wait_state storyboard 90 || { echo FAIL_STORYBOARD; exit 1; }
send "CLICK_NODE testpro2level" 25 >/dev/null
wait_state editor 420 || { echo FAIL_EDITOR; exit 1; }
sleep 40

CAM=$(send "GET_CAMERA" 20); echo "saved camera: $CAM"
echo "$LABEL $CAM" >> "$OUT/cameras.txt"
X=$(echo "$CAM" | sed -n 's/.*pos=(\([-0-9.]*\),.*/\1/p')
Y=$(echo "$CAM" | sed -n 's/.*pos=([-0-9.]*,\([-0-9.]*\),.*/\1/p')
Z=$(echo "$CAM" | sed -n 's/.*pos=([-0-9.]*,[-0-9.]*,\([-0-9.]*\)).*/\1/p')
AX=$(echo "$CAM" | sed -n 's/.*ang=(\([-0-9.]*\),.*/\1/p')
AY=$(echo "$CAM" | sed -n 's/.*ang=([-0-9.]*,\([-0-9.]*\)).*/\1/p')

echo "== Lee's framing"
shot v0_saved

echo "== the entities: is the Puddle FLAT? that decides the AABB-centre hypothesis"
send "LIST_ENTITIES" 40 | head -40 | sed 's/^/   /'
for n in Puddle Crate Cargo Box; do
  R=$(send "GET_ENTITY $n" 30 | head -6)
  [ -n "$R" ] && echo "   GET_ENTITY $n: $R" | head -8
done

echo "== signature: same scene, cameras at different heights and distances"
i=0
for dy in 0 150 400; do
  for dz in 0 400; do
    i=$((i+1))
    NY=$(python -c "print(float('${Y:-0}') + $dy)")
    NZ=$(python -c "print(float('${Z:-0}') + $dz)")
    send "SET_CAMERA $X $NY $NZ ${AX:-0} ${AY:-0}" 20 >/dev/null; sleep 5
    shot "sig${i}_dy${dy}_dz${dz}"
  done
done

echo "== control: reflections OFF at Lee's camera (proves which pixels are the reflection)"
send "SET_CAMERA $X $Y $Z ${AX:-0} ${AY:-0}" 20 >/dev/null; sleep 5
send "SET_REFLECTIONS 0" 20 | head -2; sleep 5; shot v0_reflections_off
send "SET_REFLECTIONS 1" 20 >/dev/null; sleep 5; shot v0_reflections_on

alive && echo "STILL ALIVE" || echo "!!! DIED"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $LABEL $(date +%H:%M:%S)"
