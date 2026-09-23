---
name: project-rules-patch-scripts
description: "Hard-won rules for editing source files programmatically - heredocs eat backslashes, CRLF slips rewrite whole files, re.sub eats escapes in the replacement. Write the invariant, not just care."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-16T11:52:15.052Z
---

# ★★★ Rules for editing source files with a script

All four cost real time on 2026-09-16 inserting a ~40-line Lua shim. Three of them produced a
**clean-looking** result that was wrong.

**Why:** these failures do not announce themselves. A mangled patch either compiles into nonsense or
shows up as a 2000-line diff for a 40-line change — both look like something else went wrong.

**How to apply:** before running any patch script, check it against this list.

## 1. ⚠⚠ A heredoc collapses one level of backslash

`cat > f.py << 'EOF'` — even **quoted** — turns `\\n` into `\n`. So a Python source line meant to
produce the two characters backslash-n inside a C string literal instead produces a real newline,
and the C literal breaks across lines.

⚠ **Writing the script to a FILE does not avoid this** — MEMORY.md said it did for months and that
was wrong. The collapse happens *in the heredoc*, so it hits `python -c` and `cat > file` alike.

Do one of:
- build backslashes from `chr(92)` / `bytes([92])` and concatenate;
- choose a form that needs no backslash at all (Lua separates statements with whitespace, so the
  whole shim became one line with zero escapes);
- use the Write tool instead of Bash for content where backslashes matter.

## 2. ★★★ Never substitute newlines after joining

`NL.join(lines)` then `.replace("\n", NL)` on a CRLF file turns every `\r\n` into `\r\r\n`. It
rewrote all 1974 line endings: `git diff` showed **2018 insertions / 1974 deletions for a 40-line
addition**, and a later line-based edit then mis-detected a block end and moved 316 lines of a
function to the top of the file.

**Work in binary. Explicit CRLF. No substitution afterwards.**

## 3. `re.sub` processes escapes in the REPLACEMENT

`re.sub(pat, b'...' + backslash + b'n' + b'...', s)` does **not** insert backslash-n — the
replacement template is escape-processed, so it inserts a newline. Pass a **lambda** returning the
bytes verbatim: `pat.sub(lambda m: repl, s)`.

## 4. ★★★ Write the invariant, not just care

What actually caught bug 1 was not attention — it was an assertion:

```
if s.count("\n") != s.count("\r\n"): print("FAIL, not writing"); sys.exit(1)
```

It refused to write and the file was never touched. Add, every time:

- **line-ending invariant** — bare-LF count and no `\r\r`;
- **count invariant** — output lines == input + expected additions, so the diff is provably a pure
  insertion (final result: **46 insertions, 0 deletions**);
- **anchor uniqueness** — `s.count(anchor) != 1` aborts, rather than patching the wrong site.

Make the script atomic: validate everything, *then* write once. A script that exits before writing
costs one retry; one that writes half a patch costs a revert and a re-derivation.

## 5. Check the enclosing scope before inserting

The first insertion landed **inside** `addFunctions()` (C2267/C2601) because a split `_partN.cpp`
can be one enormous function — this one started at line 2 and ran past 1570. Find the enclosing
function before choosing an insertion point; in a `#include`d part file, "file scope" means above
the first function in that part.

See also [[project-rules-environment]] and [[project-porting-clusters]].

- ★★★ **THIRD INSTANCE, 2026-09-23 (notes 3.88), and this one cost a FALSE PASS, not a crash.** A probe's screenshot-path conversion used sed to turn backslashes into forward slashes, written inside a quoted heredoc. The heredoc collapsed the doubled backslash, sed died with "unterminated s command", and the run carried on to print a healthy-looking status line that was ALREADY TRUE BEFORE THE FIX — so a verification that captured nothing reported success. Use `tr`, which only warns, or write the file with a tool that never goes through the shell (the Write tool). ★★ **And make the failure loud**: the repaired probe prints "!! SCREENSHOT NOT SAVED" and returns non-zero. **A verification that cannot fail loudly is not a verification** — same family as the 0923 gate sweep validating one stale image 38 times.
