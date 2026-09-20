---
name: project-standalone-vram
description: "Standalone play holds textures at FULL resolution where the editor and Test Game hold them reduced - +950 to +1520 MB on the same level. Found 2026-09-20, cause not yet identified."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-20T05:17:52.291Z
---

**2026-09-20, found while driving past the standalone title screen.** Notes §3.70.

★★★ **The hub's PLAY GAME (standalone, `project=2`) holds the SAME textures at FULL resolution
where the editor and Test Game hold them reduced.** Measured on three demos, back-to-back, same
machine:

| demo | Test Game census | standalone census | delta | records TG -> SA |
|---|---|---|---|---|
| Aztec Game Kit Teaser | 2415 MB | 3938 MB | **+1523** | 6694 -> 6543 |
| Bounty | 1992 MB | 3008 MB | **+1016** | 6024 -> 6028 |
| Snowy Mountain Stroll | 2151 MB | 3105 MB | **+954** | 6009 -> **5716** |

★★ **Snowy Mountain Stroll is the clean proof: 293 FEWER resources, 954 MB MORE memory.** Named
examples from the census: `Ruin J_color.dds` 1024x1024 (0.7 MB) in the editor vs **4096x4096
(10.7 MB)** in standalone; `Aztec Witch1.dds` 256x256 (0.4 MB) vs **2048x2048 (21.4 MB)**.

★★★ **Test Game is what makes this a finding rather than a measurement artefact.** The natural
reading is "gameplay demands detail, so a parked editor camera was the wrong yardstick" - and that
would make it nothing. Test Game refutes it: same gameplay, player spawned, same 90 s settle,
textures stay reduced, census reads LOWER than the editor. **Six minutes of discriminator changed
the conclusion completely.**

⚠ **"Streaming is off in standalone" is REFUTED** - `DUMP_STREAM` shows comparable enrollment in all
three modes (streaming-enabled materials 1132 editor / 1346 Test Game / 1239 standalone). I ruled
out the obvious cause and did not find the real one. Next, in order: does the standalone load path
request the full-size resource BEFORE enrollment (nothing to demote from); is demotion gated on
something the standalone loop does not run; is `texturedetail` applied on that path at all.

⚠ **NOT a regression** - nothing in 3.68/3.69 touches texture residency and the path had not been
exercised since 2026-08-16. ⚠ **NOT automatically a 4 GB crash** - 4235 MB of driver usage on a
16 GB card means the working set exceeds the minimum spec, not that a 4 GB card fails; it demotes
and evicts. A performance cliff, on the LIGHTEST demo in the set.

★ **Every VRAM number this project has published was measured in the editor or in Test Game** -
including the min-spec claim. Neither is the mode players run.

### How to drive the standalone from the harness (this took finding out)

`CLICK play_game` from the hub relaunches the exe; wait for `standalone_title`. Then
**`RUN_LUA StartGame()`** - the same Lua the title button calls (`global.lua:1201` ->
`SendMessage_startgame` -> `lua_startgame`, setting `levelloop=1`/`titleloop=0`). State becomes
`standalone_playing`.
⚠ `PRESS_KEY ENTER`, `PRESS_KEY SPACE` and `ForceMouseXYClick` at eight screen positions **all
failed to move the title**, so the harness still cannot drive that screen the way a player does, and
whether a real mouse click works on it is UNTESTED.
⚠ `DUMP_STREAM` writes to `stream_dump.txt` and returns only an OK line - copy the file per
measurement or each dump overwrites the last (it did, on the first run).

Related: [[project-vram-retention]], [[project-vram-floor]], [[project-prealpha-readiness]],
[[project-playgame-crash]], [[project-measuring-rules]]
