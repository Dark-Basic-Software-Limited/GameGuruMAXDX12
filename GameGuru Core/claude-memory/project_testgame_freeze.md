---
name: project-testgame-freeze
description: "SOLVED 2026-09-16 - Test Level parking the app at zero CPU was a Lua 5.4 modal error box, because the DX12 port swapped the game's Lua 5.2 for the engine's 5.4.8 and math.atan2 no longer exists."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-16T11:51:30.104Z
---

# ★★★ SOLVED — "test game freezes" was a Lua 5.4 MessageBox

Found 2026-09-15 (§3.39), root-caused 2026-09-16 (§3.40) after Lee hit it manually and screenshotted
the actual dialog:

```
LUA ERROR: scriptbank\global.lua:419: attempt to call a nil value (field 'atan2')
```

## ★★★ The mechanism — the DX12 port changed the Lua VERSION

| tree | what it includes | Lua |
|---|---|---|
| DX11 game | `#include "lua.h"` -> its own bundled `DarkLUA/lua/` | **5.2** |
| DX11 engine (`WickedRepo`) | vendored | 5.3 |
| **DX12 game (ours)** | `WickedEngineDX12/WickedEngine/LUA/lua.h` | **5.4.8** |

There is **no local `DarkLUA/lua/` folder in the DX12 tree at all** — the port dropped the bundled
copy and picked up the engine's. `math.atan2` is a normal function in 5.2, deprecated in 5.3,
**removed in 5.4**.

Lua ships a switch that would restore it and it is NOT on: `luaconf.h:347` nests
`LUA_COMPAT_MATHLIB` (line 355) inside `#if defined(LUA_COMPAT_5_3)`, and `LUA_COMPAT_5_3` is
defined nowhere — not luaconf.h, not any vcxproj or props. `lmathlib.c:745` is what it would have
registered: `{"atan2", math_atan}`.

## ★★★ Why it read as a hang, not an error

`RunTimeError` raises a **modal MessageBox**. A modal owns the message pump, so the process sits
alive at **exactly 0% CPU**, no crash log, harness silent — indistinguishable from a deadlock unless
someone is looking at the screen. Every automated run in §3.39 was measuring the modal.

**A zero-CPU "hang" on a Windows app means look for a DIALOG before you look for a lock.** This is
why the archived pre-port 08-29 alpha froze identically — same 5.4, same scripts. §3.39's
"not from 3.38" conclusion was right, for a reason it could not see.

## Scope — measured, not assumed

`math.atan2` is the **only** removed name anything uses. The whole 5.2->5.4 removal surface was
scanned (`cosh sinh tanh pow frexp ldexp log10 mod`, `string.gfind`, `table.foreach/getn/setn`,
`loadstring`, `setfenv/getfenv`, `module`, bare `unpack`, `%d` on floats): zero hits on every one.

| | files |
|---|---|
| `Scripts/scriptbank` calling `math.atan2` | **21** |
| of those, deployed | 17 |
| any other 5.4 incompatibility | **0** |

All pre-existing. The November-onward DX11 sync added no new callers.

## The fix

`GGLua_InstallCompatShim()` in `DarkLUA_part7.cpp`, called immediately after `luaL_openlibs` at
**both** state-creation sites — lines 1499 and 1594 are the only `luaL_newstate` calls in the game
tree, so there is no third state to miss. Aliases each removed name only if absent.

★ The alias is exact, not an approximation: 5.4's `math.atan(y [, x])` computes `atan(y/x)` using
the signs of both arguments — precisely `atan2`, and precisely the mapping Lua's own compat table
makes at `lmathlib.c:745`.

★ **Timing matters.** Six scripts do `local atan = math.atan2` at file scope, capturing the value at
load time. The shim must run — and does — before any script loads.

Rejected, with reasons:
- **Define `LUA_COMPAT_5_3` in luaconf.h** — it lives in WickedEngine; the next engine pull silently
  reverts it.
- **Rewrite the 21 scripts to `math.atan`** — the next DX11 script import undoes it, and DX11 still
  runs 5.2 where `math.atan` takes one argument, so the trees would diverge.

## ⚠ Still worth knowing

The first launch after a GAME rebuild recompiles shaders — Aztec's load then takes 20+ minutes at
~2.4 cores, and the harness cannot answer during it (`auto_command.txt` is consumed but no result is
written). That is NOT the freeze: the modal reads +0.0 CPU, this reads +36 CPU-sec per 15 s. Budget
for it before concluding anything from a post-build run.

See also [[project-rules-environment]] (modal error paths read as hangs) and
[[project-lua-logic-cost]].
