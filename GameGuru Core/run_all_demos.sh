#!/bin/bash
# Phase 4.6: Full 19-demo performance test using test_one_demo.sh
# Resilient to app crashes - auto-relaunches when process dies
RESULTS_LOG="D:/max/GameGuruMAXDX12/GameGuru Core/phase4_test_results.txt"
SCRIPT_DIR="D:/max/GameGuruMAXDX12/GameGuru Core"
EXE_DIR="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
CMD_FILE="$EXE_DIR/auto_command.txt"
RESULT_FILE="$EXE_DIR/auto_result.txt"
APP_EXE="$EXE_DIR/GameGuruMAX.exe"

is_app_running() {
    tasklist 2>/dev/null | grep -q "GameGuruMAX" && return 0 || return 1
}

send_cmd() {
    rm -f "$RESULT_FILE" 2>/dev/null
    rm -f "$CMD_FILE" 2>/dev/null
    local guard=0
    while [ -f "$RESULT_FILE" ] && [ $guard -lt 5 ]; do
        sleep 1
        rm -f "$RESULT_FILE" 2>/dev/null
        guard=$((guard + 1))
    done
    sleep 1
    echo "$1" > "$CMD_FILE"
    local timeout=${2:-20}
    local elapsed=0
    while [ ! -f "$RESULT_FILE" ] && [ $elapsed -lt $timeout ]; do
        sleep 1
        elapsed=$((elapsed + 1))
    done
    if [ -f "$RESULT_FILE" ]; then
        cat "$RESULT_FILE"
        return 0
    fi
    echo "TIMEOUT"
    return 1
}

launch_and_wait() {
    echo "  Launching GameGuruMAX..."
    cd "$EXE_DIR"
    rm -f "$CMD_FILE" "$RESULT_FILE" 2>/dev/null
    ./GameGuruMAX.exe &
    echo "  Waiting for app to be ready (up to 240s)..."
    for i in $(seq 1 24); do
        sleep 10
        local result=$(send_cmd "GET_STATE" 20)
        local st=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
        if [ -n "$st" ]; then
            echo "  App ready at ${i}0s (state=$st)"
            send_cmd "NAVIGATE hub.demo_games" 20 > /dev/null
            sleep 5
            return 0
        fi
        echo "  Still loading... (${i}0s)"
    done
    echo "  TIMEOUT waiting for app"
    return 1
}

# Ensure app is at hub, wait up to 30s
ensure_at_hub() {
    if ! is_app_running; then
        echo "  App not running! Relaunching..."
        launch_and_wait || return 1
    fi
    # Clean up stale files
    rm -f "$CMD_FILE" "$RESULT_FILE" 2>/dev/null
    sleep 2
    # Check current state
    local result=$(send_cmd "GET_STATE" 20)
    local st=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
    if [ "$st" = "hub" ]; then
        echo "  At hub (confirmed)"
        return 0
    fi
    echo "  State=$st, navigating to hub..."
    send_cmd "PRESS_ESCAPE" 15 > /dev/null
    sleep 3
    send_cmd "PRESS_ESCAPE" 15 > /dev/null
    sleep 3
    send_cmd "NAVIGATE hub.demo_games" 20 > /dev/null
    sleep 10
    # Verify
    local result2=$(send_cmd "GET_STATE" 20)
    local st2=$(echo "$result2" | grep "^STATE:" | head -1 | sed 's/STATE: //')
    if [ "$st2" = "hub" ]; then
        echo "  At hub (after recovery)"
        return 0
    fi
    echo "  WARNING: State=$st2 (not hub)"
    return 1
}

echo "=== Phase 4.6: 19-Demo Performance Test ===" > "$RESULTS_LOG"
echo "Date: $(date)" >> "$RESULTS_LOG"
echo "" >> "$RESULTS_LOG"

# Verify app is running
if ! is_app_running; then
    echo "App not running. Launching..."
    launch_and_wait
else
    echo "App already running."
    ensure_at_hub
fi

# Demo name | level node name
DEMOS=(
    "Trapped|trapped"
    "Switch Escape|switch escape"
    "Disruption|disruption"
    "Escape from the Zombie Cellar|escape from the zombie cellar"
    "Aztec Game Kit Teaser|aztec game kit teaser"
    "Aztec Game Kit|the aztec adventure"
    "Bounty|bountymap"
    "Horseshoe Bend|horseshoe bend"
    "Island Showdown|island showdown"
    "Operation Amazon|operation amazon"
    "River Raiders|river raiders"
    "Snowy Mountain Stroll|snowy mountain stroll"
    "A Grand Canyon Adventure|a grand canyon adventure"
    "Foggy Forest|foggy forest"
    "Indian Strike Force|indian strike force"
    "Canyon Offensive|canyon offensive"
    "Jungle Fever|jungle fever"
    "RPG Template|rpg template"
    "The Mystery of Z Island|the mystery of z island"
)

PASS_COUNT=0
FAIL_COUNT=0
CRASH_COUNT=0
TOTAL=${#DEMOS[@]}
PASS_LIST=""
FAIL_LIST=""

for entry in "${DEMOS[@]}"; do
    IFS='|' read -r demo level_node <<< "$entry"
    idx=$((PASS_COUNT + FAIL_COUNT + 1))
    echo ""
    echo "=========================================="
    echo "Testing [$idx/$TOTAL]: $demo"
    echo "=========================================="

    # Ensure app is running and at hub before each demo
    ensure_at_hub
    # Clean stale files before handing off to test script
    rm -f "$CMD_FILE" "$RESULT_FILE" 2>/dev/null
    sleep 2

    output=$(bash "$SCRIPT_DIR/test_one_demo.sh" "$demo" "$level_node" 2>&1)
    exit_code=$?
    echo "$output"

    # Check if app crashed during test
    if ! is_app_running; then
        echo ">>> APP CRASHED during $demo"
        output="$output"$'\n'"APP_CRASHED: Process died during test"
        CRASH_COUNT=$((CRASH_COUNT + 1))
    fi

    echo "--- $demo ---" >> "$RESULTS_LOG"
    echo "$output" >> "$RESULTS_LOG"
    echo "" >> "$RESULTS_LOG"

    if echo "$output" | grep -q "=== PASS ==="; then
        PASS_COUNT=$((PASS_COUNT + 1))
        fps=$(echo "$output" | grep "^FPS:" | head -1)
        camera=$(echo "$output" | grep "^CAMERA_EYE:" | head -1)
        echo ">>> PASS: $fps | $camera"
        PASS_LIST="$PASS_LIST  PASS: $demo ($fps)\n"
    else
        FAIL_COUNT=$((FAIL_COUNT + 1))
        reason=$(echo "$output" | grep -E "(FAIL:|CRASH)" | tail -1)
        echo ">>> FAIL: $reason"
        FAIL_LIST="$FAIL_LIST  FAIL: $demo ($reason)\n"
    fi

    # Brief pause between demos for state to settle
    sleep 5
done

echo "" >> "$RESULTS_LOG"
echo "=== SUMMARY ===" >> "$RESULTS_LOG"
echo "PASSED: $PASS_COUNT / $TOTAL" >> "$RESULTS_LOG"
echo "FAILED: $FAIL_COUNT / $TOTAL" >> "$RESULTS_LOG"
echo "CRASHES: $CRASH_COUNT" >> "$RESULTS_LOG"
echo "" >> "$RESULTS_LOG"
echo "Pass list:" >> "$RESULTS_LOG"
echo -e "$PASS_LIST" >> "$RESULTS_LOG"
echo "Fail list:" >> "$RESULTS_LOG"
echo -e "$FAIL_LIST" >> "$RESULTS_LOG"

echo ""
echo "======================================="
echo "DONE: $PASS_COUNT passed, $FAIL_COUNT failed ($CRASH_COUNT crashes) out of $TOTAL"
echo "======================================="
echo -e "Passed:\n$PASS_LIST"
echo -e "Failed:\n$FAIL_LIST"
echo "Full results: $RESULTS_LOG"
