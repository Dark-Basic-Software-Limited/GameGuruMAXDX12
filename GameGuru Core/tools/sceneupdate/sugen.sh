#!/bin/bash
# Scene::Update A/B across terrain ring radii, on ONE binary.
#
# Wraps sumeasure.sh with the setup.ini `terraingen=<N>` key so both arms are the same build —
# no stash-and-rebuild between them. `generation` is latched once at GGTerrainWicked_Init, so
# this key is the only way to vary it; a runtime knob would be inert (the SET_TREES pool trap,
# SWITCHESCAPE_PERF.md §2).
#
# Usage: sugen.sh <demo> <gen> [gen ...]     e.g.  sugen.sh "Switch Escape" 14 12
#   gen 0 = leave the shipped default alone (writes no key at all)
#
# ⚠ Writes setup.ini and RESTORES IT ON EXIT (trap). Judge by the Scene-S1..S5 ms sum that
#   sumeasure.sh prints, NEVER by FPS — the editor frame is GPU-fence-bound.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"
DEMO="${1:?need a demo name}"; shift
INI="$D/setup.ini"
cp "$INI" "$OUT/.setup.ini.sugen.bak"
cleanup() { cp "$OUT/.setup.ini.sugen.bak" "$INI" 2>/dev/null; }
trap cleanup EXIT INT TERM

for G in "$@"; do
  echo "########## terraingen=$G ##########"
  grep -viE '^[[:space:]]*terraingen[[:space:]]*=' "$OUT/.setup.ini.sugen.bak" > "$INI"
  [ "$G" != "0" ] && echo "terraingen=$G" >> "$INI"
  "$OUT/sumeasure.sh" "gen$G" "$DEMO" 6
done
echo "===== SUMMARY ====="
for G in "$@"; do
  # Confirm the knob REACHED the terrain before trusting the number beside it.
  grep -h '^RESULT' "$OUT/su_gen$G.txt" 2>/dev/null | sed "s/^/gen=$G /"
  grep -hm1 'TERRAIN_RING' "$OUT/su_gen${G}_raw.txt" 2>/dev/null | sed 's/^/   /'
done
