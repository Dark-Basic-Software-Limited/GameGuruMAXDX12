#!/bin/bash
# NAME the meshes/materials retained across a level swap (+15 meshes / +17 materials measured
# River Raiders -> Island Showdown). Loads the TARGET level two ways on the SAME build and diffs
# the dumps by name. Descendant of tools/sceneupdate/suleakname.sh (2.23b), extended to meshes.
#   FRESH : TARGET is the first level of the process
#   AFTER : DECOY -> hub -> TARGET
# Anything in AFTER but not FRESH is carried over from the DECOY = the leak, named.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")/leak"; mkdir -p "$OUT"
TARGET="${1:-Island Showdown}"; DECOY="${2:-River Raiders}"
LOCK="$OUT/.leak.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then echo "REFUSING: PID $(cat "$LOCK") running"; exit 3; fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
say(){ echo "$(date +%H:%M:%S) $*"; }
send(){ rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0; local to=${2:-40}
  while [ $t -lt $to ]; do [ -f "$D/auto_result.txt" ] && { sleep 0.3; cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; return 1; }
alive(){ tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
killmax(){ local i=0; while [ $i -lt 10 ]; do taskkill.exe //IM GameGuruMAX.exe //F >/dev/null 2>&1; sleep 2; alive || return 0; i=$((i+1)); done; return 1; }
wait_state(){ local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
enter(){ send "NAVIGATE hub.demo_games" 20 >/dev/null; sleep 3
  send "SELECT_DEMO $1" 20 >/dev/null; sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 8
  wait_state "storyboard" 60 || return 1; send "CLICK_ONLY_LEVEL" 20 >/dev/null
  wait_state "editor" 300 || return 1; sleep 30; }
pick(){ for p in "$D/$1" "$D/Files/$1"; do [ -f "$p" ] && { echo "$p"; return; }; done; }
grab(){ # $1 = label
  send "DUMP_MATERIALS" 60 >/dev/null; sleep 2; local m=$(pick materials_dump.txt); [ -n "$m" ] && cp -f "$m" "$OUT/mat_$1.txt"
  send "DUMP_MESHES" 60 >/dev/null; sleep 2;   local x=$(pick mesh_census.txt);    [ -n "$x" ] && cp -f "$x" "$OUT/mesh_$1.txt"
  local d=$(send "GET_PERF_DATA" 60)
  say "  [$1] $(echo "$d" | grep -E '^SCENE_OBJECTS:|^SCENE_MESHES:|^SCENE_MATERIALS:|^SCENE_TRANSFORMS:' | tr '\n' ' ')"
  say "  [$1] mat lines=$(wc -l < "$OUT/mat_$1.txt" 2>/dev/null)  mesh census lines=$(wc -l < "$OUT/mesh_$1.txt" 2>/dev/null)"; }
launch(){ killmax; rm -f "$D/auto_command.txt" "$D/auto_result.txt"
  cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & sleep 20; wait_state "hub" 120 || exit 1; sleep 4; }

say "=== FRESH: $TARGET as the first level ==="; launch; enter "$TARGET" || { say "load failed"; exit 1; }; grab FRESH
say "=== AFTER: $DECOY -> hub -> $TARGET ==="; launch; enter "$DECOY" || { say "decoy load failed"; exit 1; }
send "NAVIGATE hub" 30 >/dev/null; sleep 10; wait_state "hub" 120; sleep 4
enter "$TARGET" || { say "target load failed"; exit 1; }; grab AFTER
send "QUIT" 10 >/dev/null 2>&1; t=0; while alive && [ $t -lt 20 ]; do sleep 1; t=$((t+1)); done; killmax

say "=== MATERIALS present only in AFTER (carried over from $DECOY) ==="
python - "$OUT/mat_FRESH.txt" "$OUT/mat_AFTER.txt" <<'PY'
import sys, collections
def names(p):
    c = collections.Counter()
    try:
        for l in open(p, encoding='utf-8', errors='replace'):
            parts = l.rstrip('\n').split('\t')
            if len(parts) >= 2 and parts[0].strip().isdigit():
                c[parts[1].strip() or '(unnamed)'] += 1
    except Exception as e:
        print('  read failed:', e)
    return c
a, b = names(sys.argv[1]), names(sys.argv[2])
extra = [(n, b[n]-a.get(n,0)) for n in b if b[n] > a.get(n,0)]
print('  FRESH materials=%d  AFTER materials=%d  delta=%+d' % (sum(a.values()), sum(b.values()), sum(b.values())-sum(a.values())))
for n, d in sorted(extra, key=lambda x: -x[1])[:40]:
    print('    +%-3d %s' % (d, n[:110]))
if not extra: print('    (no material name appears more often in AFTER)')
PY
say "=== MESH census: owner rows that differ ==="
python - "$OUT/mesh_FRESH.txt" "$OUT/mesh_AFTER.txt" <<'PY'
import sys, re, collections
def owners(p):
    c = {}; sect = None
    try:
        for l in open(p, encoding='utf-8', errors='replace'):
            if l.startswith('=='): sect = l.strip(); continue
            if sect and 'BY OWNER NAME' in sect:
                m = re.match(r'\s*(\d+)\s+(\d+)\s+(.{1,46}?)\s{2,}(.*)$', l.rstrip('\n'))
                if m: c[m.group(3).strip()] = int(m.group(1))
    except Exception as e:
        print('  read failed:', e)
    return c
a, b = owners(sys.argv[1]), owners(sys.argv[2])
print('  FRESH owners=%d  AFTER owners=%d' % (len(a), len(b)))
diff = [(n, b[n]-a.get(n,0)) for n in b if b[n] != a.get(n,0)]
for n, d in sorted(diff, key=lambda x: -abs(x[1]))[:40]:
    print('    %+4d  %s' % (d, n[:110]))
onlyb = [n for n in b if n not in a]
if onlyb: print('  owners ONLY in AFTER: ' + ', '.join(onlyb[:20]))
if not diff and not onlyb: print('    (owner rows identical)')
PY
say "=== header lines (counts + orphans) ==="
for t in FRESH AFTER; do echo "  [$t] $(head -4 "$OUT/mesh_$t.txt" 2>/dev/null | tr '\n' ' ' | cut -c1-200)"; done
say "LEAKNAME DONE"
