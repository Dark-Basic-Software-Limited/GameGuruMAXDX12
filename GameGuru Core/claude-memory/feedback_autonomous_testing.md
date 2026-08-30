---
name: feedback-autonomous-testing
description: "For visual/UI tuning, the user wants Claude to drive the whole build/run/screenshot/iterate loop itself (kill the MAX EXE, rebuild, launch, navigate, screenshot) without asking them each cycle."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
---

# Autonomous MAX test loop — standing delegation

For iterative visual/behavioural tuning in GameGuru MAX, the user wants Claude to drive the entire loop autonomously: kill the running `GameGuruMAX.exe` itself, rebuild Release, launch MAX, load the test level, take SCREENSHOTs, compare against the reference, and iterate — WITHOUT asking the user to close/launch the app each cycle.

**Why:** Stated 2026-05-29 during grass-appearance polish; the delegation stands and was exercised heavily through the 2026-07-12/13 tree sessions.

**How to apply:** Current mechanics (kill command, build command, `./GameGuruMAX.exe &` launch, `OPEN_PROJECT TESTPRO1` → `CLICK_ONLY_LEVEL` → `SCREENSHOT` sequence with timings) live in MEMORY.md → CRITICAL BUILD RULES and [[project-harness-open-my-games]] — do not duplicate them here. Honour the CLAUDE.md guard: if a launch itself fails/crashes, STOP and report rather than blindly retrying.

*(2026-07-17 cleanup: removed a stale 2026-05-29 claim that bash `./exe &` launches tear MAX down mid-init — the file itself had retracted the rationale (those crashes were the grass VRAM bug, since fixed), and the bash-`&` launch is the proven form used by every session since. Also removed the "user must click the project open manually" section — superseded by OPEN_PROJECT, DONE 2026-07-12.)*
