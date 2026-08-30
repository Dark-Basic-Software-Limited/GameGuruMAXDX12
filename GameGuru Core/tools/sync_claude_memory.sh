#!/bin/bash
# Sync Claude Code's per-project auto-memory between the LIVE store and the versioned snapshot
# in the repo (GameGuru Core/claude-memory).
#
# Why this exists: the live memory lives under ~/.claude and is NOT in version control, so it was
# the one valuable artefact in this project that could be lost with a machine. The repo copy is a
# mirror; this script keeps the two honest in either direction.
#
#   sync_claude_memory.sh to-repo     live  -> repo   (after a work session, then commit)
#   sync_claude_memory.sh to-live     repo  -> live   (on a new machine, or after a reinstall)
#   sync_claude_memory.sh diff        show what differs, change nothing
#
# ⚠ The project key is the absolute working-directory path with every non-alphanumeric character
# replaced by '-'. D:\max\GameGuruMAXDX12 becomes D--max-GameGuruMAXDX12. If the repo lives
# somewhere else on this machine the key differs, and a wrong key fails SILENTLY - Claude simply
# starts with no project knowledge. Override with GG_MEMORY_KEY if you need to.
set -u

MODE="${1:-}"
case "$MODE" in to-repo|to-live|diff) ;; *)
  echo "usage: $0 {to-repo|to-live|diff}"; exit 2 ;; esac

# repo root = two levels up from tools/ (…/GameGuru Core/tools -> …/GameGuruMAXDX12)
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CORE="$(cd "$HERE/.." && pwd)"
REPO_DIR="$CORE/claude-memory"

# Derive the key from the repo root, the same way Claude Code does.
ROOT="$(cd "$CORE/.." && pwd)"                      # e.g. /d/max/GameGuruMAXDX12
WINROOT="$(cd "$ROOT" && pwd -W 2>/dev/null || echo "$ROOT")"   # e.g. D:/max/GameGuruMAXDX12
KEY="${GG_MEMORY_KEY:-$(echo "$WINROOT" | sed 's/[^A-Za-z0-9]/-/g')}"
LIVE_DIR="$HOME/.claude/projects/$KEY/memory"

echo "repo snapshot : $REPO_DIR"
echo "live memory   : $LIVE_DIR"
echo "project key   : $KEY"
[ -n "${GG_MEMORY_KEY:-}" ] && echo "                (overridden by GG_MEMORY_KEY)"
echo

if [ "$MODE" = "diff" ]; then
  if [ ! -d "$LIVE_DIR" ]; then echo "live memory does not exist on this machine"; exit 1; fi
  # README.md is repo-only documentation, never a memory entry
  diff -rq --exclude=README.md "$LIVE_DIR" "$REPO_DIR" && echo "identical (README.md excluded)"
  exit 0
fi

if [ "$MODE" = "to-repo" ]; then
  [ -d "$LIVE_DIR" ] || { echo "FAIL: no live memory at $LIVE_DIR"; exit 1; }
  # Remove repo entries that no longer exist live (a deleted memory should not linger), but never
  # the README.
  for f in "$REPO_DIR"/*.md; do
    b="$(basename "$f")"
    [ "$b" = "README.md" ] && continue
    [ -f "$LIVE_DIR/$b" ] || { rm -f "$f"; echo "  removed (gone from live): $b"; }
  done
  cp "$LIVE_DIR"/*.md "$REPO_DIR"/
  echo "copied $(ls "$LIVE_DIR"/*.md | wc -l) files live -> repo"
  echo "now: git add \"GameGuru Core/claude-memory\" && git commit -m \"sync claude memory\""
else
  mkdir -p "$LIVE_DIR"
  cp "$REPO_DIR"/*.md "$LIVE_DIR"/
  rm -f "$LIVE_DIR/README.md"        # documentation, not a memory entry
  echo "copied $(ls "$LIVE_DIR"/*.md | wc -l) files repo -> live"
fi
