# GameGuru MAX harness helpers - source this:  source "tools/harness/lib.sh"
# send <cmd> [timeout]   one harness command, reply with CR stripped (GET_STATE ends in CRLF -
#                        comparing it raw to "STATE: editor" never matches: that hung a launch for 10 min)
# alive                  is MAX running
# wait_state <s> <secs>  poll GET_STATE until "STATE: <s>"
# shot <name>            SCREENSHOT, copied to $OUT/<name>.png
# click <x> <y> [wait]   MOUSE_AT / MOUSE_DOWN / sleep / MOUSE_UP - ImGui needs the button held across a frame
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${GGMAX_OUT:-$HOME/ggmax_harness_out}"; mkdir -p "$OUT"   # NEVER the build output dir
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { sleep 0.2; tr -d "\r" < "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
shot() { local l p; l=$(send "SCREENSHOT" 40)
  p=$(echo "$l" | sed "s/^OK: Screenshot saved to //" | tr '\134' '/')
  if [ -f "$p" ]; then cp "$p" "$OUT/$1.png"; echo "$OUT/$1.png"; else echo "SHOT FAILED: $l" >&2; return 1; fi; }
click() { send "MOUSE_AT $1 $2" 20 >/dev/null; sleep 1; send "MOUSE_DOWN" 20 >/dev/null; sleep 0.5
          send "MOUSE_UP" 20 >/dev/null; sleep "${3:-2}"; }
