#!/bin/bash
# Full 19-demo VRAM audit on the current build, editor, defaults (no lowvram keys).
# Per demo: driver/census/blocks header, the category split, and the derived non-resource figure.
# Project identity is verified before loading (an aborted run once left the hub on the wrong
# project and produced correctly-labelled numbers for the wrong level).
source /c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12--claude-worktrees-determined-chebyshev-bf0892/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/floor_lib.sh
OUT=/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12--claude-worktrees-determined-chebyshev-bf0892/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/audit
mkdir -p "$OUT"
sed -i '/^lowvram=/d;/^lowvramgrassdist=/d;/^lowvramgrassdensity=/d' "$D/setup.ini"

DEMOS=("A Grand Canyon Adventure" "Aztec Game Kit" "Aztec Game Kit Teaser" "Bounty" "Canyon Offensive"
"Disruption" "Escape from the Zombie Cellar" "Foggy Forest" "Horseshoe Bend" "Indian Strike Force"
"Island Showdown" "Jungle Fever" "Operation Amazon" "RPG Template" "River Raiders"
"Snowy Mountain Stroll" "Switch Escape" "The Mystery of Z Island" "Trapped")

for DEMO in "${DEMOS[@]}"; do
  TAG=$(echo "$DEMO" | tr -cd '[:alnum:]' | tr 'A-Z' 'a-z')
  echo "===== $DEMO ====="
  t=0
  while [ $t -lt 3 ]; do
    launch && {
      sel=$(send "SELECT_DEMO $DEMO" 25 | head -1 | tr -d '\r')
      case "$sel" in OK*)
        sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
        wait_state "storyboard" 60 && {
          proj=$(send "GET_STATE" 25 | grep -i "^PROJECT" | tr -d '\r')
          case "$proj" in *"$DEMO"*)
            send "CLICK_ONLY_LEVEL" 20 >/dev/null
            wait_state "editor" 320 && { sleep 35; alive && break; };;
          *) echo "  (wrong project: $proj)";; esac
        };;
      *) echo "  (select failed: $sel)";; esac
    }
    t=$((t+1)); echo "  retry $t"
  done
  [ $t -ge 3 ] && { echo "  GAVE UP"; continue; }
  perf | grep -E "^FPS:|^POLYS:" | tr '\n' ' ' | sed 's/^/  /'; echo
  send "DUMP_VRAM audit_$TAG" 90 >/dev/null; sleep 3
  cp "$D/Files/vram_census_audit_$TAG.txt" "$OUT/" 2>/dev/null || { echo "  NO CENSUS"; continue; }
  # GGMAX 1.83 — stay inside ONE memory segment. `d3d12ma_blocks` and `census_bytes` both sum
  # video AND system memory (UPLOAD/READBACK staging lives in system RAM), while `driver_usage`
  # is video-only. The pre-1.83 form of this line charged system-RAM waste to `padding` and
  # subtracted system-memory blocks from `nonresource`, understating it by the same amount.
  # Measured on TESTPRO1: padding read 257 MB but only 152 MB of it was ever video memory.
  # Use the *_video fields; falls back to the old (mixed) arithmetic on pre-1.83 censuses.
  head -1 "$OUT/vram_census_audit_$TAG.txt" | awk '{for(i=1;i<=NF;i++){split($i,a,"=");v[a[1]]=a[2]}
    bv = ("d3d12ma_blocks_video" in v)    ? v["d3d12ma_blocks_video"]    : v["d3d12ma_blocks"];
    av = ("d3d12ma_allocated_video" in v) ? v["d3d12ma_allocated_video"] : v["census_bytes"];
    printf "  driver=%.0f blocksVid=%.0f censusVid=%.0f paddingVid=%.0f nonresourceVid=%.0f psoEager=%s%s\n",
    v["driver_usage"]/1048576, bv/1048576, av/1048576,
    (bv-av)/1048576, (v["driver_usage"]-bv)/1048576, v["pso_driver_eager"],
    ("d3d12ma_blocks_video" in v) ? "" : "  (PRE-1.83 census: figures MIX video+system memory)"}'
  awk -f /c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12--claude-worktrees-determined-chebyshev-bf0892/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/audit.awk "$OUT/vram_census_audit_$TAG.txt" | sort | awk -F'|' '{printf "    %-30s %8.1f MB %6d\n",$1,$2,$3}'
done
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "AUDIT" "FINISHED"
