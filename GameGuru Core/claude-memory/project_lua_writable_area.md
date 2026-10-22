---
name: project-lua-writable-area
description: "DX12's Lua (engine 5.4.8) opened files with plain fopen, so any script or data file living only in Documents\\...\\Files (add-on packs like Cine Guru MAX, Behavior Editor behaviours) failed with 'cannot open'; 3.124 engine hook ggLua_fopenhook -> GG_fopen restores DX11"
metadata:
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-10-22T18:49:11.607Z
---

**DX11 compiled its own Lua 5.2 (`DarkLUA\lua`) with every open-by-name patched to `GG_fopen`** (writable area first,
then the install folder): lauxlib luaL_loadfilex, liolib opencheck + io_open, loadlib readable. The DX12 port swapped
in the engine's Lua 5.4.8 (`WickedEngine\LUA`) - plain `fopen` relative to the CWD (install `Files`). The C++ side
kept checking `FileExist` (writable-aware), so the check PASSED and the Lua open then FAILED: "LUA ERROR! cannot open
scriptbank\Cine Guru MAX\cg_actor.lua: No such file or directory" (Lee, 2026-10-22, report #9).

**3.124 fix (Lee-confirmed 2026-10-22):** engine `ggLua_fopenhook` (lauxlib.h/.c, NULL = stock) at the 4 DX11 sites + the binary-chunk
`freopen` (fclose + hook "rb" - beyond DX11; exported games with `exportassets=0` precompile every .lua and take
that branch); the game sets it to `GG_fopen` in `GGLua_InstallCompatShim` (DarkLUA_part7.cpp). WICKED_ENGINE_CHANGES
row 3.124 - **an upstream Lua update silently undoes it** (still links). Lua writes (save games) go back to
Documents\...\Files\savegames as in DX11.

**Why:** third time the Lua swap dropped something DX11 relied on (3.40 math.atan2, 3.48 bit32, 3.124 file
redirect). A file-by-file sweep of DX11's GG_fopen / GG_GetRealPath calls found Lua the ONLY vendored library that
lost its redirect - but DX12-new render code (terrain materials, custom grass, tree pool) builds `<exe>/Files/...`
paths itself and skips the writable area (spawned as a side task).

**How to apply:** a "cannot open" / "module not found" / "behaviour edit does nothing" on a script that exists ->
first ask WHERE it lives (Documents vs install). Probe live with harness `RUN_LUA` (io.open / package.loaded /
dofile in the running game). When any vendored library is replaced by an engine copy, grep the DX11 copy for `GG_`
before trusting it. ⚠ Since 3.124 a same-name script in Documents overrides the install copy (DX11 parity).
[[project-writable-area]] [[project-testgame-freeze]] [[project-porting-clusters]] [[project-demo-shadow-copy]]
