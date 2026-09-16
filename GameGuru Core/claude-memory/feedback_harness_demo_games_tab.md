---
name: feedback-harness-demo-games-tab
description: "SELECT_DEMO silently matches an empty list unless the hub is on the Demo Games tab - and the hub defaults to My Games as soon as any user project exists. Always NAVIGATE hub.demo_games first."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-16T17:34:46.704Z
---

# ⚠⚠ Harness: `NAVIGATE hub.demo_games` BEFORE `SELECT_DEMO`

The hub opens on the **My Games** tab whenever `projectbank_list` is non-empty
(`M-GridEditB_part16.cpp:851-861`, the `bCheckForAnyProjectFiles` block), and the demo library
only populates while the **Demo Games** tab is rendering. So `SELECT_DEMO` matches against an empty
`g_LibraryFileList` and the sweep goes nowhere.

```
NAVIGATE hub.demo_games     # tab 0; my_games is 1. Cmd_Navigate, AutomationHarness.cpp:531
sleep 5
SELECT_DEMO <name>          # check the reply is OK, retry if not
CLICK edit_game
CLICK_ONLY_LEVEL            # settle ~8 s first, CHECK the reply, retry
```

**Why:** this default flipped the moment Lee created his first user project. Sweeps that worked
before it existed stop working after, with no error — which is the worst possible failure shape.

**How to apply:** always navigate to the tab explicitly; never assume the hub's default.

## ★★★ The bigger rule this cost two hours to relearn

**A test that cannot distinguish its own failure modes will report the wrong one with total
confidence.** Three instances in one day (2026-09-16):

- `| tail` on a long test buffered all output — a stall looked like silence (36 min lost)
- `CLICK_ONLY_LEVEL`'s reply piped to `/dev/null` — a *refused click* looked identical to a *slow
  level load*, so each demo burned its full 16-minute timeout. **7 false failures.** Horseshoe Bend
  then loaded in 50 s the moment it was clicked by hand.
- §3.39's window enumeration returned nothing even on the known-good control

★ **Check every reply, retry on non-OK, and budget in measured seconds.** The instant v1 printed
what `CLICK_ONLY_LEVEL` actually returned, this was a two-minute fix.

## Two bash traps in harness scripts

- `local a="$1" b="$2" c=$((b))` — bash expands **every** argument before assigning any, so `b` is
  unbound under `set -u`. Assign on separate lines.
- `res=$(run_demo ...)` where the function backgrounds the app — command substitution waits for the
  pipe to close and the backgrounded process inherits it, so it **hangs forever**. Never wrap a
  launcher in `$( )`; have it print to the log instead.

★ Free blank-render detector: a near-black 1536x864 capture compresses to under 0.15 MB. A real
scene is 0.7-2.5 MB. Sort the screenshots by size and any failed render is the outlier.

See also [[project-harness-open-my-games]], [[project-writable-area]], [[feedback-instrument-before-theory]].
