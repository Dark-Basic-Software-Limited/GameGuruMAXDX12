#!/bin/bash
# Phase 4.6: Full 19-demo performance test
# Runs each demo via editor test_level, collects FPS and camera data

EXE_DIR="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
CMD_FILE="$EXE_DIR/auto_command.txt"
RESULT_FILE="$EXE_DIR/auto_result.txt"
RESULTS_LOG="D:/max/GameGuruMAXDX12/GameGuru Core/phase4_test_results.txt"

send_cmd() {
    rm -f "$RESULT_FILE" 2>/dev/null
    echo "$1" > "$CMD_FILE"
    local timeout=${2:-30}
    local elapsed=0
    while [ ! -f "$RESULT_FILE" ] && [ $elapsed -lt $timeout ]; do
        sleep 1
        elapsed=$((elapsed + 1))
    done
    if [ -f "$RESULT_FILE" ]; then
        cat "$RESULT_FILE"
    else
        echo "TIMEOUT after ${timeout}s"
    fi
}

wait_for_state() {
    local target=$1
    local timeout=${2:-120}
    local elapsed=0
    while [ $elapsed -lt $timeout ]; do
        sleep 5
        elapsed=$((elapsed + 5))
        local state=$(send_cmd "GET_STATE" 10)
        if echo "$state" | grep -qi "STATE: $target"; then
            echo "$state"
            return 0
        fi
        # If timeout on GET_STATE, harness not polling
        if echo "$state" | grep -qi "TIMEOUT"; then
            continue
        fi
    done
    echo "TIMEOUT waiting for state=$target after ${timeout}s"
    return 1
}

echo "=== Phase 4.6: 19-Demo Performance Test ===" > "$RESULTS_LOG"
echo "Date: $(date)" >> "$RESULTS_LOG"
echo "" >> "$RESULTS_LOG"

# Demo name → level node name mapping
# Each entry: "demo_display_name|level_node_title"
DEMOS=(
    "Aztec Game Kit Teaser|aztec game kit teaser"
    "Aztec Game Kit|the aztec adventure"
    "Bounty|bountymap"
    "Horseshoe Bend|horseshoe bend"
    "Island Showdown|island showdown"
    "Operation Amazon|operation amazon"
    "River Raiders|river raiders"
    "Snowy Mountain Stroll|snowy mountain stroll"
    "A Grand Canyon Adventure|a grand canyon adventure"
    "Disruption|disruption"
    "Foggy Forest|foggy forest"
    "Indian Strike Force|indian strike force"
    "Switch Escape|switch escape"
    "Canyon Offensive|canyon offensive"
    "Escape from the Zombie Cellar|escape from the zombie cellar"
    "Jungle Fever|jungle fever"
    "RPG Template|rpg template"
    "The Mystery of Z Island|the mystery of z island"
    "Trapped|trapped"
)

PASS_COUNT=0
FAIL_COUNT=0

for entry in "${DEMOS[@]}"; do
    IFS='|' read -r demo level_node <<< "$entry"
    echo "--- Testing: $demo ---"
    echo "--- $demo ---" >> "$RESULTS_LOG"

    # 1. Navigate to hub demo tab
    send_cmd "NAVIGATE hub.demo_games" 10 > /dev/null
    sleep 3

    # 2. Select demo
    result=$(send_cmd "SELECT_DEMO $demo" 10)
    if echo "$result" | grep -qi "error.*not found"; then
        echo "  FAILED: demo not found"
        echo "  RESULT: FAIL (not found)" >> "$RESULTS_LOG"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        echo "" >> "$RESULTS_LOG"
        continue
    fi

    # 3. Click edit_game to open storyboard
    sleep 2
    send_cmd "CLICK edit_game" 15 > /dev/null
    sleep 10

    # 4. Click level node to load into editor
    result=$(send_cmd "CLICK_NODE $level_node" 15)
    echo "  CLICK_NODE: $result" >> "$RESULTS_LOG"

    # 5. Wait for editor state (level loading)
    state=$(wait_for_state "editor" 60)
    if echo "$state" | grep -qi "TIMEOUT"; then
        echo "  FAILED: editor timeout"
        echo "  RESULT: FAIL (editor timeout)" >> "$RESULTS_LOG"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        send_cmd "PRESS_ESCAPE" 10 > /dev/null
        sleep 5
        echo "" >> "$RESULTS_LOG"
        continue
    fi
    sleep 3

    # 6. Test level
    send_cmd "CLICK test_level" 10 > /dev/null

    # 7. Wait for game state (level loading + game start)
    state=$(wait_for_state "game" 120)
    if echo "$state" | grep -qi "TIMEOUT"; then
        echo "  FAILED: game timeout"
        echo "  RESULT: FAIL (game timeout)" >> "$RESULTS_LOG"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        send_cmd "PRESS_ESCAPE" 10 > /dev/null
        sleep 5
        echo "" >> "$RESULTS_LOG"
        continue
    fi

    # 8. Wait for scene to stabilize
    sleep 8

    # 9. Collect performance data
    perf=$(send_cmd "GET_PERF_DATA" 10)
    echo "  PERF:" >> "$RESULTS_LOG"
    echo "$perf" | while IFS= read -r line; do
        echo "    $line" >> "$RESULTS_LOG"
    done

    fps=$(echo "$perf" | grep "^FPS:" | head -1)
    camera=$(echo "$perf" | grep "^CAMERA_EYE:" | head -1)
    echo "  $fps | $camera"
    echo "  RESULT: PASS" >> "$RESULTS_LOG"
    PASS_COUNT=$((PASS_COUNT + 1))

    # 10. Exit back to editor
    send_cmd "PRESS_ESCAPE" 10 > /dev/null
    sleep 5

    # 11. Wait for return to editor/storyboard/hub
    for i in $(seq 1 6); do
        state=$(send_cmd "GET_STATE" 10)
        if echo "$state" | grep -qi "STATE: hub\|STATE: editor\|STATE: storyboard"; then
            break
        fi
        sleep 3
    done
    sleep 2
    echo "" >> "$RESULTS_LOG"
done

echo "" >> "$RESULTS_LOG"
echo "=== SUMMARY ===" >> "$RESULTS_LOG"
echo "PASSED: $PASS_COUNT / ${#DEMOS[@]}" >> "$RESULTS_LOG"
echo "FAILED: $FAIL_COUNT / ${#DEMOS[@]}" >> "$RESULTS_LOG"
echo ""
echo "=== DONE: $PASS_COUNT passed, $FAIL_COUNT failed out of ${#DEMOS[@]} ==="
echo "Results saved to: $RESULTS_LOG"
