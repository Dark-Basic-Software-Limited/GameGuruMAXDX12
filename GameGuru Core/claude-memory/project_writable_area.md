---
name: project-writable-area
description: "User projects and levels do NOT live in the build area - they are under Documents/GameGuruApps/GameGuruMAX. Searching the build area for a user level finds nothing and looks like the level does not exist."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-16T14:13:08.460Z
---

# ⚠⚠ User projects live in the WRITABLE AREA, not the build area

```
C:/Users/leeba/Documents/GameGuruApps/GameGuruMAX/
    Files/mapbank/<level>.fpm                  <- user levels
    Files/mapbank/_automatedbackups/            <- _1.._4 rolling backups, very useful as A/B controls
    Files/projectbank/<PROJECT>/                <- user projects (TESTPRO2 etc)
    Files/thumbbank/                            <- screen_<PROJECT>_*.jpg
    My Games/<PROJECT>/<PROJECT>.exe            <- published standalones
```

The build area (`D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max/Files/...`) holds only the
**shipped demos** — 19 projectbank entries, all stock. A user level is never there.

**Why this matters:** on 2026-09-16 Lee reported a corrupted level and I searched the build area,
found nothing, and told him "it isn't in my install, you must be running a different build". He
wasn't. I had simply looked in the wrong tree, and that wrong conclusion cost most of an hour and
pushed the diagnosis back onto him. `Files/levelbank/testmap/` contains a file literally named
*"testmap files now go to the writable area.txt"* — the answer was sitting in the tree I searched.

★ **When a user names a level or project you cannot find, search the writable area FIRST**, then
`find /c/Users/leeba/Documents /d/DEV -maxdepth 7 -iname "*<name>*"` before concluding it is absent.

## ★ Reading a level's format version

`.fpm` is a **password-protected zip** — password `mypassword` (`M-MapFile_part0.cpp:94`) — with
`map.ele` inside, whose first 4 bytes are the format version.

```
python "GameGuru Core/tools/ele_version.py" "<path>/level.fpm"
```

⚠ **Every shipped demo is v338**, which skips the `>= 340` block entirely. So a shipped demo
loading correctly proves NOTHING about changes in that block — see [[project-testgame-freeze]] and
the §3.41 notes for how that produced a false all-clear. User levels are v342.

`_automatedbackups/` gives you up to four earlier saves of the same level for free — the cheapest
A/B control available when a level goes wrong.

See also [[project-harness-open-my-games]] and [[project-cwd-and-file-paths]].
