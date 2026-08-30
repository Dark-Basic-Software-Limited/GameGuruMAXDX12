---
name: project-cwd-and-file-paths
description: "The process CWD moves at runtime - every file WRITE must anchor to the exe, and one live feature still depends on startup ordering"
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-30T22:29:32.068Z
---

**The current directory is not stable in MAX, so a relative path resolved at write time lands
somewhere unpredictable.** This is a property of the app, not a bug to be fixed once.

## Where it moves

| when | what | notes |
|---|---|---|
| startup | `SetCurrentDirectoryA("Files")` — `master_part0.cpp:1417` | save/restore pair, undone at `:1453` |
| startup | `SetDir("Files")` — `Common_part2.cpp:380`, from `FPSC_Setup()` | **permanent** — this is the one that sticks |
| editor | `SetCurrentDirectoryA(...)` to `GameGuruApps\GameGuruMAX` — `M-GridEditB_part14.cpp:814` | |
| editor | `SetCurrentDirectoryA("..\\")` — `M-GridEditB_part16.cpp:2606` | |
| any time | **Windows FILE DIALOGS change it** | the engine already knew: `wiRenderer.cpp:91` uses an absolute shader path "to avoid the case when something (eg. file dialog) overrides working directory" |

## The rule for any NEW file writer

- engine → `wi::helper::GetDiagnosticPath("name.txt")`
- game → `GGDiagFopen("name.txt", "w")` (defined in `Guru-WickedMAX/master_part1.cpp`)

Both resolve to the **executable's** directory via `GetModuleFileName`, so they are runtime-resolved
and machine-independent — NOT hardcoded. ~29 writers were routed in 3.35i/j (screenshots, `log.txt`,
every `DUMP_*`, the `gpup_*` traces, `gg_pso_fail`, `gap_trace`).

⚠ **Deliberate exceptions — do not "fix" these:** `setup.ini` (`Common_part3.cpp:1036`) is a PRODUCT
file being READ, and `GG_fopen("files/treebank/FractalGenerator.dat")` is product data. The rule is
"debug OUTPUT anchors to the exe", not "every relative path is wrong".

## ★★ The one that is still load-bearing on ordering

`g_pAbsPathToConverter` is built with **`GetCurrentDirectoryA`** at `Common_part0.cpp:233`, and both
ffmpeg call sites derive their path from it by replacing `\Guru-Converter.exe` → `\ffmpeg.exe`
(see [[project-alpha-packaging]]). It is correct **today only because of call ordering**: the
`SetCurrentDirectoryA("Files")` above is already restored, and the permanent `SetDir("Files")`
happens 494 lines later in the same function. Verified — but nothing pins it.

⚠ If anything moves `common_init()` later in startup, or adds a CWD change before it, the icon
export and the Import Music And Sound button **silently stop working** — no error, the tool just
isn't where the path says. Anchoring it to `GetExecutablePath()` would remove the dependency; not
done, because the two features work and the change touches a variable four other things read.

## How to detect this class of problem

★★★ **Enumerate by DESTINATION, not by call pattern.** Exercise the feature and look at where the
file actually appeared. Two greps reported "clean" while nine writers were still wrong, because a
sweep can only report on the sites its regex matched — it cannot report the ones it never looked
at. Evidence it was wrong: 8 byte-identical blank screenshots accumulated in the exe root over six
months, `log.txt` appeared in two folders in one session, and a 1.28 GB nested `Max/Files/Files/`
tree grew from one `"Files\\hairkill_dump.txt"` written while the CWD was already `Max\Files`.

Narrative: `NIGHT_INVESTIGATIONS_2026-08-12.md` §3.35h–j. See also [[project-measuring-rules]].
