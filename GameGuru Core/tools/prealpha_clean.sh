#!/bin/bash
# PRE-ALPHA BUILD-AREA HYGIENE.
#
# The build area accumulates two kinds of file that must not go out to testers:
#
#   1. DIAGNOSTIC OUTPUT - logs and dumps written by harness commands and debug paths.
#      Harmless but noisy, and some are large (auto_log.txt alone has been 7.8 MB).
#
#   2. ARMING FILES - these are not output, they are SWITCHES. The engine checks for the
#      file's existence at startup and turns a diagnostic ON if it is there. Shipping one
#      turns that diagnostic on for every tester:
#        dred.txt           - D3D12 Device Removed Extended Data + auto-breadcrumbs (GPU cost)
#        gg_atlas_trace.txt - shadow atlas create/shrink tracing
#        leakall.txt, leakterraintex.txt, resource_hijack.txt, stream_guard.txt,
#        stream_load.txt, gg_pso_fail.txt, last_upload.txt, anim_garbage.txt
#      ⚠ dred.txt is the one that matters. It has been present on this machine since
#      2026-07-27, which means EVERY VRAM and FPS baseline in this repo was measured with
#      DRED on - keep that in mind when comparing a tester's numbers against ours.
#
# Product files that live in the same folder and must be KEPT are whitelisted below.
# Default is a DRY RUN. Pass --apply to actually delete.
set -u
D="${GGMAX_BUILD_AREA:-/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max}"
APPLY=0
[ "${1:-}" = "--apply" ] && APPLY=1

[ -d "$D" ] || { echo "build area not found: $D"; exit 1; }

# Files that look like debris and are NOT. ffmpeg.exe and GameGuruMAX.pdb are covered in
# ALPHA packaging notes; the .pdb SHIPS so tester crash logs symbolise.
KEEP="steam_appid.txt changelog.txt gamecontrollerdb.txt dxdiagsystemspecs.bat"

is_kept(){ for k in $KEEP; do [ "$1" = "$k" ] && return 0; done; return 1; }

total=0; n=0
echo "=== build area: $D"
echo
for f in "$D"/*.txt "$D"/*.log "$D"/*.sh; do
  [ -e "$f" ] || continue
  b=$(basename "$f")
  is_kept "$b" && continue
  sz=$(wc -c < "$f" | tr -d ' ')
  total=$((total+sz)); n=$((n+1))
  tag="diagnostic output"
  case "$b" in
    dred.txt|gg_atlas_trace.txt|leakall.txt|leakterraintex.txt|resource_hijack.txt|\
    stream_guard.txt|stream_load.txt|gg_pso_fail.txt|last_upload.txt|anim_garbage.txt)
      tag="*** ARMING FILE - turns a diagnostic ON for the tester ***" ;;
    *.sh) tag="loose test script (predates this campaign, not git-tracked)" ;;
  esac
  printf "%10s  %-28s %s\n" "$sz" "$b" "$tag"
  [ $APPLY -eq 1 ] && rm -f "$f"
done

# Harness scratch that is regenerated on demand.
for f in "$D/Files/vram_census_"*.txt "$D/screenshots/"*.png; do
  [ -e "$f" ] || continue
  sz=$(wc -c < "$f" | tr -d ' '); total=$((total+sz)); n=$((n+1))
  printf "%10s  %-28s %s\n" "$sz" "$(basename "$f")" "harness scratch"
  [ $APPLY -eq 1 ] && rm -f "$f"
done

# Files/ debris. NAMED explicitly rather than pattern-matched: Files/ is mostly product and a
# glob here would eventually eat something real.
FILES_DEBRIS="applytransform_garbage.txt gap_trace.txt log.txt reload_quiesce.txt treepool_dump.txt videotrace.txt"
for b in $FILES_DEBRIS; do
  f="$D/Files/$b"; [ -e "$f" ] || continue
  sz=$(wc -c < "$f" | tr -d " "); total=$((total+sz)); n=$((n+1))
  printf "%10s  %-30s %s\n" "$sz" "Files/$b" "diagnostic output"
  [ $APPLY -eq 1 ] && rm -f "$f"
done
for d in "Files/particlesbank_old" "Files/screenshots" "Files/testmap"; do
  [ -d "$D/$d" ] || continue
  sz=$(du -sk "$D/$d" 2>/dev/null | cut -f1); sz=$((sz*1024)); total=$((total+sz)); n=$((n+1))
  printf "%10s  %-30s %s\n" "$sz" "$d/" "working directory, not product"
  [ $APPLY -eq 1 ] && rm -rf "$D/$d"
done
echo
printf "%d files, %.1f MB\n" "$n" "$(awk -v t=$total 'BEGIN{print t/1048576}')"
if [ $APPLY -eq 1 ]; then
  echo "REMOVED."
else
  echo "DRY RUN - nothing deleted. Re-run with --apply to remove."
fi
echo
echo "LISTED BUT NOT TOUCHED - decide these yourself:"
for b in bookcase_surface.dds books_surface.dds brass308_surface.dds; do
  [ -e "$D/Files/$b" ] && printf "  %-32s loose surface map at Files/ root, no copy in entitybank - may be product\n" "Files/$b"
done
[ -d "$D/Files/savegames" ] && printf "  %-32s %s of save games from testing - a fresh install should probably not ship them\n" "Files/savegames/" "$(du -sh "$D/Files/savegames" 2>/dev/null | cut -f1)"
echo
echo "Also check before zipping:"
echo "  setup.ini producelogfiles  -> should be 0 (currently: $(grep -i '^producelogfiles' "$D/setup.ini" 2>/dev/null | head -1))"
echo "  setup.ini memgeneratedump  -> should be 0 (currently: $(grep -i '^memgeneratedump' "$D/setup.ini" 2>/dev/null | head -1))"
echo "  GameGuruMAX.pdb present    -> KEEP IT, tester crash logs need it to symbolise"
