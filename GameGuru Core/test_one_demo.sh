#!/bin/bash
# Test a single demo - usage: test_one_demo.sh "Demo Name" "level_node_title"
# Returns: exit 0 on PASS, exit 1 on FAIL, exit 2 on APP_CRASH
DEMO="$1"
LEVEL="$2"

EXE_DIR="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
CMD_FILE="$EXE_DIR/auto_command.txt"
RESULT_FILE="$EXE_DIR/auto_result.txt"

is_app_alive() {
    tasklist 2>/dev/null | grep -q "GameGuruMAX"
}

# Send command and wait for response. Returns 0=ok, 1=timeout, 2=app_dead
send_cmd() {
    if ! is_app_alive; then echo "APP_DEAD"; return 2; fi
    rm -f "$RESULT_FILE" "$CMD_FILE" 2>/dev/null
    sleep 1
    # Verify result file is gone
    if [ -f "$RESULT_FILE" ]; then rm -f "$RESULT_FILE" 2>/dev/null; sleep 1; fi
    echo "$1" > "$CMD_FILE"
    local timeout=${2:-20}
    local elapsed=0
    while [ ! -f "$RESULT_FILE" ] && [ $elapsed -lt $timeout ]; do
        sleep 1
        elapsed=$((elapsed + 1))
        # Check app every 5s
        if [ $((elapsed % 5)) -eq 0 ] && ! is_app_alive; then
            echo "APP_DEAD"
            return 2
        fi
    done
    if [ -f "$RESULT_FILE" ]; then
        cat "$RESULT_FILE"
        return 0
    else
        echo "TIMEOUT"
        return 1
    fi
}

# Poll state until we get expected state or timeout
wait_for_state() {
    local expected="$1"
    local max_polls=${2:-20}
    local poll_interval=${3:-5}
    for i in $(seq 1 $max_polls); do
        local result=$(send_cmd "GET_STATE" 15)
        local rc=$?
        if [ $rc -eq 2 ]; then echo "APP_DEAD"; return 2; fi
        local st=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
        if [ "$st" = "$expected" ]; then
            echo "$st"
            return 0
        fi
        if [ $i -lt $max_polls ]; then
            sleep $poll_interval
        fi
    done
    echo "$st"
    return 1
}

echo "=== Testing: $DEMO ($LEVEL) ==="

# Step 1: Verify at hub
echo "Step 1: Verify hub state"
state=$(send_cmd "GET_STATE" 15 | grep "^STATE:" | head -1 | sed 's/STATE: //')
echo "  State: $state"
if [ "$state" = "APP_DEAD" ]; then echo "  APP_CRASHED"; echo "=== CRASH ==="; exit 2; fi
if [ "$state" != "hub" ]; then
    echo "  Navigating to hub..."
    send_cmd "NAVIGATE hub.demo_games" 15 > /dev/null
    sleep 5
    state=$(wait_for_state "hub" 5 3)
    echo "  State after navigate: $state"
    if [ "$state" != "hub" ]; then
        echo "  FAIL: Cannot reach hub (state=$state)"
        echo "=== FAIL ==="
        exit 1
    fi
fi

# Step 2: Select demo
echo "Step 2: Select demo"
result=$(send_cmd "SELECT_DEMO $DEMO" 15)
rc=$?
if [ $rc -eq 2 ]; then echo "  APP_CRASHED"; echo "=== CRASH ==="; exit 2; fi
echo "  $result" | head -1
sleep 3

# Step 3: Edit game (opens storyboard)
echo "Step 3: Edit game"
send_cmd "CLICK edit_game" 15 > /dev/null
sleep 5
echo "  Waiting for storyboard..."
state=$(wait_for_state "storyboard" 8 5)
rc=$?
if [ $rc -eq 2 ]; then echo "  APP_CRASHED"; echo "=== CRASH ==="; exit 2; fi
echo "  State: $state"
if [ "$state" != "storyboard" ]; then
    echo "  FAIL: Not at storyboard (state=$state)"
    send_cmd "NAVIGATE hub.demo_games" 15 > /dev/null
    sleep 3
    echo "=== FAIL ==="
    exit 1
fi

# Step 4: Click level node (loads into editor)
echo "Step 4: Click level node '$LEVEL'"
result=$(send_cmd "CLICK_NODE $LEVEL" 20)
rc=$?
if [ $rc -eq 2 ]; then echo "  APP_CRASHED"; echo "=== CRASH ==="; exit 2; fi
echo "  $result" | head -1
echo "  Waiting for editor..."
state=$(wait_for_state "editor" 18 5)
rc=$?
if [ $rc -eq 2 ]; then echo "  APP_CRASHED"; echo "=== CRASH ==="; exit 2; fi
echo "  State: $state"
if [ "$state" != "editor" ]; then
    echo "  FAIL: Editor did not load (state=$state)"
    send_cmd "NAVIGATE hub.demo_games" 15 > /dev/null
    sleep 3
    echo "=== FAIL ==="
    exit 1
fi
sleep 3

# Step 5: Test level
echo "Step 5: Test level"
send_cmd "CLICK test_level" 15 > /dev/null
echo "  Waiting for game state..."
state=$(wait_for_state "game" 35 5)
rc=$?
if [ $rc -eq 2 ]; then echo "  APP_CRASHED"; echo "=== CRASH ==="; exit 2; fi
echo "  State: $state"
if [ "$state" != "game" ]; then
    echo "  FAIL: Game did not start (state=$state)"
    send_cmd "PRESS_ESCAPE" 10 > /dev/null
    sleep 3
    send_cmd "NAVIGATE hub.demo_games" 15 > /dev/null
    sleep 3
    echo "=== FAIL ==="
    exit 1
fi

# Step 6: Wait for scene to stabilize and collect perf
echo "Step 6: Collecting performance data"
sleep 10
if ! is_app_alive; then echo "  APP_CRASHED during gameplay"; echo "=== CRASH ==="; exit 2; fi
perf=$(send_cmd "GET_PERF_DATA" 15)
rc=$?
if [ $rc -eq 2 ]; then echo "  APP_CRASHED"; echo "=== CRASH ==="; exit 2; fi
if ! echo "$perf" | grep -q "^FPS:"; then
    echo "  Retrying perf data..."
    sleep 5
    perf=$(send_cmd "GET_PERF_DATA" 15)
fi
echo "$perf"

# Step 7: Exit game and return to hub
echo "Step 7: Exiting game"
send_cmd "PRESS_ESCAPE" 15 > /dev/null
sleep 5
send_cmd "NAVIGATE hub.demo_games" 15 > /dev/null
sleep 3

echo "=== PASS ==="
