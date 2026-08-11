#!/bin/bash
# NAME the entities retained across a level change (the residual +79 objects / +17 materials
# left after the 2.23 pool-release fix). Captures DUMP_MATERIALS + DUMP_TREEPOOL's orphan
# detector on the SAME target level loaded two ways, and diffs them by name.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; TARGET="${1:-Switch Escape}"; DECOY="${2:-Island Showdown}"
LOCK="$OUT/.sumeasure.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then echo "REFUSING: PID $(cat "$LOCK")"; exit 3; fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
say(){ echo "$(date +%H:%M:%S) $*"; }
send(){ rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive(){ tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state(){ local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
enter(){ send "SELECT_DEMO $1" 20 >/dev/null; sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
  send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 60; wait_state "editor" 300; sleep 30; }
grab(){ # $1 = label
  send "DUMP_MATERIALS" 60 >/dev/null; sleep 2
  cp "$D/Files/materials_dump.txt" "$OUT/mat_$1.txt" 2>/dev/null
  send "DUMP_TREEPOOL" 60 >/dev/null; sleep 2
  cp "$D/Files/treepool_dump.txt" "$OUT/pool_$1.txt" 2>/dev/null
  local d=$(send "GET_PERF_DATA" 60)
  say "  [$1] $(echo "$d" | grep -E '^SCENE_OBJECTS:|^SCENE_MESHES:|^SCENE_MATERIALS:' | tr '\n' ' ')"
  say "  [$1] materials in dump: $(wc -l < "$OUT/mat_$1.txt" 2>/dev/null)"; }
launch(){ taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
  rm -f "$D/auto_command.txt" "$D/auto_result.txt"; cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 &
  sleep 15; wait_state "hub" 90 || exit 1; sleep 4; }

say "=== FRESH: $TARGET first ==="; launch; enter "$TARGET"; grab FRESH
say "=== AFTER: $DECOY -> hub -> $TARGET ==="; launch; enter "$DECOY"
send "NAVIGATE hub" 60 >/dev/null; sleep 12; wait_state "hub" 120; sleep 4
enter "$TARGET"; grab AFTER
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
say "=== MATERIAL NAMES PRESENT ONLY IN 'AFTER' (the leak, named) ==="
python - "$OUT/mat_FRESH.txt" "$OUT/mat_AFTER.txt" <<'PY'
import sys,re,collections
def names(p):
    c=collections.Counter()
    for l in open(p,encoding='utf-8',errors='replace'):
        m=re.search(r'name=(\S.*?)(?:\s+parent=|\s*$)', l)
        if m: c[m.group(1).strip()]+=1
        elif l.strip(): c[l.strip()[:80]]+=1
    return c
a,b=names(sys.argv[1]),names(sys.argv[2])
extra=[(n,b[n]-a.get(n,0)) for n in b if b[n]>a.get(n,0)]
extra.sort(key=lambda x:-x[1])
print("total extra material entries: %d" % sum(n for _,n in extra))
for n,k in extra[:40]: print("  +%-4d %s" % (k,n))
PY
