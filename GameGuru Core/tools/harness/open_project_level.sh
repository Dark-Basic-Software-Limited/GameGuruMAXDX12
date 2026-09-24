# Launch MAX -> open a My Games project -> load a level node -> (optionally) open the Object Library.
# usage: open_project_level.sh [PROJECT] [LEVELNODE] [library]      defaults: TESTPRO2 testpro2level
# ★ MAX is started as (cd && exec exe) with ALL its fds redirected. A plain `cd "$D" && ./exe &` backgrounds a
#   SUBSHELL that keeps the caller's stdout pipe open until MAX exits - `script | tail` then never returns.
source "$(dirname "$0")/lib.sh"
PROJ="${1:-TESTPRO2}"; LVL="${2:-testpro2level}"
alive && { echo "MAX ALREADY RUNNING - refusing"; exit 1; }
rm -f "$D/auto_command.txt" "$D/auto_result.txt"
(cd "$D" && exec ./GameGuruMAX.exe) > /dev/null 2>&1 < /dev/null &
sleep 25
wait_state hub 90 || { echo FAIL_HUB; exit 1; }
sleep 4
send "OPEN_PROJECT $PROJ" 30; sleep 6
wait_state storyboard 90 || { echo FAIL_STORYBOARD; exit 1; }
send "CLICK_NODE $LVL" 25
wait_state editor 420 || { echo FAIL_EDITOR; exit 1; }
sleep 15
echo EDITOR_READY
shot 00_editor
[ "$3" = "library" ] || exit 0
click 23 120 4
send "MOUSE_AT 1000 760" 10 >/dev/null; sleep 2
shot 01_library
echo LIBRARY_OPEN
