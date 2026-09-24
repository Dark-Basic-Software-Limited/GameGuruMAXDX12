# Launch MAX -> Demo Games tab -> a SHIPPED demo -> editor -> (optionally) Test Game.
# usage: open_demo.sh "Switch Escape" [testgame]
# ★ NAVIGATE hub.demo_games FIRST - SELECT_DEMO matches an empty list otherwise. Leave Test Game with PRESS_ESCAPE
#   (PRESS_KEY ESCAPE does not reach the game loop).
source "$(dirname "$0")/lib.sh"
DEMO="${1:-Switch Escape}"
alive && { echo "MAX ALREADY RUNNING - refusing"; exit 1; }
rm -f "$D/auto_command.txt" "$D/auto_result.txt"
(cd "$D" && exec ./GameGuruMAX.exe) > /dev/null 2>&1 < /dev/null &
sleep 25
wait_state hub 90 || { echo FAIL_HUB; exit 1; }
sleep 4
send "NAVIGATE hub.demo_games" 15 > /dev/null; sleep 5
R=$(send "SELECT_DEMO $DEMO" 15); echo "select: $R" | head -1
case "$R" in OK*) ;; *) send "NAVIGATE hub.demo_games" 15 >/dev/null; sleep 6; R=$(send "SELECT_DEMO $DEMO" 15); echo "select retry: $R" | head -1;; esac
sleep 3
send "CLICK edit_game" 15 | head -1; sleep 8
wait_state storyboard 40 || { echo FAIL_STORYBOARD; exit 1; }
send "CLICK_ONLY_LEVEL" 15 | head -1
wait_state editor 240 || { echo FAIL_EDITOR; exit 1; }
sleep 15
echo EDITOR_READY
[ "$2" = "testgame" ] || exit 0
send "CLICK test_level" 15 | head -1
sleep 12
wait_state game 120 || { echo FAIL_GAME; exit 1; }
sleep 12
echo GAME_READY
