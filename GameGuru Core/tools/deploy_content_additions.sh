#!/bin/bash
# Deploy tracked content additions into the MAX build area.
#
# WHY THIS EXISTS
#   Neither the DX11 nor the DX12 repo tracks Files/ - no _markers/*.fpe, no editors/uiv3/*.png.
#   Content lives in the product install, and the alpha IS the build folder, curated. There is no
#   packaging script to add a file to.
#
#   That means a NEW content file exists on one machine only, invisible to git, and is lost on a
#   fresh install, a different box, or a rebuilt build area. On 2026-09-17 that exact failure was
#   found in the wild: bit32 support for Lua 5.4 existed solely as an untracked hand edit to two
#   deployed .lua files, so nothing in the tracked build reproduced it and every level would have
#   parked on a modal the moment the build area was recreated.
#
#   GameGuru Core/content-additions/Files/** is the tracked master. This script is what makes it
#   reproducible rather than merely archived.
#
# USAGE
#   bash "GameGuru Core/tools/deploy_content_additions.sh"            # deploy
#   bash "GameGuru Core/tools/deploy_content_additions.sh" --check    # report only, change nothing
#
# Idempotent. Safe to run after every build, on a new machine, or before cutting a package.
set -u

SRC="D:/max/GameGuruMAXDX12/GameGuru Core/content-additions/Files"
DST="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max/Files"
CHECK=0
[ "${1:-}" = "--check" ] && CHECK=1

[ -d "$SRC" ] || { echo "FAIL: source not found: $SRC"; exit 1; }
[ -d "$DST" ] || { echo "FAIL: build area not found: $DST"; exit 1; }

copied=0; same=0; missing=0; failed=0

while IFS= read -r f; do
  rel="${f#$SRC/}"
  out="$DST/$rel"
  if [ -f "$out" ] && cmp -s "$f" "$out"; then
    printf "  same     %s\n" "$rel"; same=$((same+1)); continue
  fi
  if [ ! -f "$out" ]; then state="MISSING"; missing=$((missing+1)); else state="differs"; fi
  if [ "$CHECK" -eq 1 ]; then
    printf "  %-8s %s\n" "$state" "$rel"; continue
  fi
  mkdir -p "$(dirname "$out")"
  if cp "$f" "$out" 2>/dev/null && cmp -s "$f" "$out"; then
    printf "  DEPLOYED %s\n" "$rel"; copied=$((copied+1))
  else
    printf "  FAILED   %s\n" "$rel"; failed=$((failed+1))
  fi
done < <(find "$SRC" -type f | sort)

echo "  ---"
if [ "$CHECK" -eq 1 ]; then
  echo "  check only: $same already correct, $missing missing"
  [ "$missing" -gt 0 ] && exit 1
else
  echo "  $copied deployed, $same already correct, $failed failed"
  [ "$failed" -gt 0 ] && exit 1
fi
exit 0
