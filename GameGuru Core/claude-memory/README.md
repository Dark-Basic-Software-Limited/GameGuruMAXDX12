# Claude auto-memory — versioned snapshot

This folder is a **mirror** of Claude Code's per-project auto-memory. It is here so the knowledge
survives a machine change, a reinstall, or a lost profile — it was the only valuable artefact in
this project that lived outside version control.

## ⚠ This is a copy, not the live store

Claude reads and writes the LIVE copy at:

```
~/.claude/projects/<project-key>/memory/
```

Editing files **here** changes nothing at runtime. This folder is a snapshot you restore *from*,
and re-sync *to* when the live memory has moved on.

## Restoring on another machine

The `<project-key>` is the absolute path of the working directory with every non-alphanumeric
character replaced by `-`. So:

| repo location | project key |
|---|---|
| `D:\max\GameGuruMAXDX12` | `D--max-GameGuruMAXDX12` |
| `C:\dev\GameGuruMAXDX12` | `C--dev-GameGuruMAXDX12` |

⚠ **Get this wrong and the memory is silently not found** — no error, Claude just starts with no
project knowledge. The simplest way to avoid it is to check the repo out at the same path on every
machine.

```bash
# adjust the key to match where THIS machine has the repo
mkdir -p ~/.claude/projects/D--max-GameGuruMAXDX12/memory
cp "GameGuru Core/claude-memory"/*.md ~/.claude/projects/D--max-GameGuruMAXDX12/memory/
# do NOT copy this README into the live folder - it is not a memory entry
rm -f ~/.claude/projects/D--max-GameGuruMAXDX12/memory/README.md
```

There is also `tools/sync_claude_memory.sh`, which does the copy in either direction and skips the
README for you.

## Re-syncing after a work session

The live memory changes as work proceeds. To bring this snapshot back up to date:

```bash
bash tools/sync_claude_memory.sh to-repo
git add "GameGuru Core/claude-memory" && git commit -m "sync claude memory"
```

## What is in here

`MEMORY.md` is the index and is loaded into context at the start of every session; everything else
is one fact per file, pulled in on relevance. Files cross-reference each other with `[[name]]`
links, so **restore the whole folder** — a partial copy leaves dangling links.

The single most useful entry is **`project_next_action_immediate.md`**: current state, the exact
next step, and the do-not list. `MEMORY.md` points at it first for that reason.

## Relationship to the other docs

- `NIGHT_INVESTIGATIONS_2026-08-12.md` is the **narrative** — what was tried, what the numbers were,
  what turned out to be wrong. Long, chronological, the place to go for "why is it like this".
- `CLAUDE.md` is the **standing instructions** for working in this repo.
- This folder is the **distilled** form: the rules and state worth carrying into a fresh session
  without re-reading 9,000 lines.

Snapshot taken 2026-08-30, at engine `a159b93e` / game `91b5c146`, sweep `0829a` CLEAN 19/19.
