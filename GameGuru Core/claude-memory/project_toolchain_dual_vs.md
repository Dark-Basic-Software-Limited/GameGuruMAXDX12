---
name: project-toolchain-dual-vs
description: The desktop has THREE VS installs and two provide v143 at the same version - a clean rebuild mixes CRT headers and fails. Latent since 2026-08-31.
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-15T12:19:41.414Z
---

# ★★★ A clean rebuild on the DESKTOP is broken, and an incremental one hides it

Found 2026-09-15 while porting DX11 changes. **Not caused by that work — exposed by it.**

## The failure

```
excpt.h(22,14): error C2011: '_EXCEPTION_DISPOSITION': 'enum' type redefinition
    see declaration of '_EXCEPTION_DISPOSITION'
        C:\Program Files (x86)\...\2022\BuildTools\...\excpt.h(22,14)
```
plus `C2953 __vcrt_va_list_is_reference: class template has already been defined` and
`C2641/C2783/C2780` on `wiAllocator.h(71)`. All while compiling `SimonReloaded.cpp`.

★ **The tell is the two different prefixes in one error.** `Program Files\...\2022\Community` and
`Program Files (x86)\...\2022\BuildTools`. The CRT headers are being included from BOTH installs, so
every CRT type is defined twice.

## Why

The desktop has **three** installs:

| install | present |
|---|---|
| `C:\Program Files\Microsoft Visual Studio\18\Community` (VS 2026) | yes |
| `C:\Program Files\Microsoft Visual Studio\2022\Community` | yes |
| `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools` | **yes** |

Both 2022 installs provide toolset **v143 at the same version, 14.44.35207**. So `v143` is
ambiguous: `build.bat`'s `VsDevCmd` sets `INCLUDE` from 2022 Community, MSBuild independently
resolves the toolset to BuildTools, and the compile sees both. The laptop has only 2022 Community,
so it never sees this.

The 08-31 laptop session repointed `build.bat`/`build_wicked.bat` from `18\Community` to
`2022\Community` (commit `6252c126`) — correct for the laptop, and what introduced this on the
desktop. See [[project-machine-migration]].

## ★★★ Why it stayed hidden for two weeks — the transferable lesson

`SimonReloaded.obj`, `System.lib` and `Bullet.lib` were last built **2026-03-27**. Those
sub-projects had not recompiled in five and a half months, so the VS switch never touched them.
Three green builds on 09-14/09-15 compiled **zero** files from them.

⚠ **A green incremental build says nothing about the toolchain for objects it did not rebuild.**
I told Lee on 09-14 that "engine and game both build clean on VS 2022 here" — true for what was
compiled, and worthless as a statement about the toolchain, because the affected projects were
already up to date. **To verify a toolchain change you must force a rebuild of the thing you are
claiming about** (`/t:Rebuild`, or touch a header it includes). Same family as the `cp -r` mtime
trap in [[project-rules-environment]]: msbuild's idea of "up to date" is not evidence of "correct".

Detected only because editing `wickedcalls.h` forced `SimonReloaded.cpp` to recompile for the first
time since March.

## The fix

Pick the install that exists rather than hardcoding one — prefer `18\Community`, fall back to
`2022\Community`, error if neither. No-op on the laptop, avoids the ambiguity on the desktop.
Applied to both `build.bat` and `build_wicked.bat`.

⚠ Do NOT "simplify" this back to a single hardcoded path. That is exactly what broke it.
