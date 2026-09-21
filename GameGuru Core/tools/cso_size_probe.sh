#!/bin/bash
# Prove an ENGINE shader edit reached the binary. build.bat DELETES stale engine .cso
# ("refresh_shaders") and MAX recompiles them at launch, so the file's timestamp says nothing and
# its mere existence says nothing either - only the SIZE compared against a build of the other
# source proves the edit is in there. See the shader-build-pipeline rule (3.33 shipped a sweep
# that passed on stale .cso).
#   usage: cso_size_probe.sh <shader.cso> <label>
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
CSO="$D/shaders/${1:?usage: cso_size_probe.sh <name.cso> <label>}"
LABEL="${2:-probe}"

send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 3; rm -f "$D/auto_command.txt"
rm -f "$CSO"          # force the recompile, so a leftover file cannot be mistaken for a fresh one
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 &
sleep 30
t=0; while [ $t -lt 90 ]; do [ "$(send GET_STATE 15 | head -1)" == "STATE: hub" ] && break; sleep 5; t=$((t+7)); done
sleep 8
if [ -f "$CSO" ]; then
  echo "$LABEL $(basename "$CSO") = $(stat -c %s "$CSO") bytes  md5=$(md5sum "$CSO" | cut -c1-12)"
else
  echo "$LABEL $(basename "$CSO") = MISSING - it was not recompiled at launch"
fi
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
