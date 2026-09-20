#!/bin/bash
# Score a demo_fps_sweep.sh run against the pre-registered release gate.
#
# The criteria are written here, in the script, so the verdict cannot be rationalised after
# seeing the data:
#   C1 LOAD     every demo reaches the editor (no FAIL_* row, no missing row)
#   C2 GEOMETRY POLYS identical to the reference sweep on every demo. For an engine-wide ECS
#               lookup change this IS the correctness proof: a mis-resolved entity->component
#               would change what gets drawn, and POLYS would move.
#   C3 VRAM     every demo under 4096 MB driver usage (the shipped 4 GB min-spec gate)
#   C4 GAME     every demo reaches gameplay past the loading overlays
#
# ⚠ FPS IS DELIBERATELY NOT A CRITERION. Editor FPS on this rig swings +-8 between launches and
# can shift much further between DAYS with GPU power state (2026-08-10 read Switch Escape at 142
# where 08-09 read 201, at a matched CPU frame). Cross-day FPS deltas are not evidence. FPS is
# printed for the record and for within-run ranking only.
#
# Usage: sweepgate.sh <results_file> [vram_limit_mb]
set -u
RES="${1:?usage: sweepgate.sh <results_file> [vram_limit_mb]}"
LIMIT="${2:-4096}"
[ -f "$RES" ] || { echo "no such results file: $RES"; exit 2; }

python - "$RES" "$LIMIT" <<'PY'
import sys, re
res, limit = sys.argv[1], float(sys.argv[2])

# POLYS reference: RE-BASELINED 2026-08-25 to sweep 0825 (results_0825.txt), engine `ce9751b8` /
# game `43ac24a8`. Provenance, because a rebaseline is the one move that can quietly bless a bug:
#   - The previous reference was the 2.32 sweep (0813). Between it and 0825 the far-tree work
#     landed - 2.97 the pool cap, 2.98 "stop drawing trees past the terrain", 2.99 atlas slices,
#     3.00/3.01 the billboard terrain-reach clip, 3.03 the mesh handover fade (draw_distance on
#     pool trees), 3.04 the matching prepass clip. All six REDUCE drawn geometry by design, so the
#     0813 table failed C2 on most of the hub and the gate had stopped carrying information.
#   - Direction: every 0825 change vs the intervening 0823 sweep is NEGATIVE. Scatter would go
#     both ways, so this is a build difference, and it points the way six by-design culling
#     changes predict.
#   - Determinism was CONFIRMED, not assumed: Canyon Offensive, the -10.1% outlier, was re-probed
#     on a fresh launch and read 465,823 on three samples out of three, bit-exact with the sweep.
#     Eight demos were bit-identical to 0823 including tree levels (Horseshoe Bend, Island
#     Showdown), which corroborates from the other side.
#   - Four demos were re-probed again on game `8a2a9f16` (the 3.19a-c refinements) and came back
#     bit-identical to the table below.
# ★★ AN IDENTITY GATE PROVES "NO CHANGE", NEVER "CORRECT". If a change is EXPECTED to alter what
# gets drawn, amend C2 IN WRITING BEFORE the run (see NIGHT_INVESTIGATIONS_2026-08-12.md for the
# C2" wording used for 2.32: an increase is allowed if attributable, a decrease fails).
# ⚠ Anything that touches the tree pool, the billboard handover or a draw_distance will move these
# numbers again. Say so before running, not after reading.
REF = {
 # GGMAX 3.53c: Aztec Game Kit Teaser was 6454117 - a stale PRE-far-tree number the 08-25
 # rebaseline never updated. 1413604 is deterministic post far-tree pool cap: the distant trees
 # are billboards, not counted polys. Uncapping (SET_TREEPOOLCAP 0) restores 5148708, proving no
 # geometry was lost.
 # GGMAX 3.64: that comment used to sit at the END of the Aztec Teaser line, which swallowed the
 # next two entries into it - so this dict held 17 demos, not 19, and C2 silently stopped
 # checking Aztec Game Kit and Bounty. A missing reference took the "polys?" branch, which did
 # NOT set c2 = False, while the summary still printed "identical on all 19 demos" because it
 # prints len(rows). Found by the 09-19 single-session soak sweep. Comments now go ABOVE the
 # entries they describe, and a missing reference is a HARD FAIL below.
 "Aztec Game Kit Teaser":1413604, "Aztec Game Kit":522301, "Bounty":469906,
 "Horseshoe Bend":1583122, "Island Showdown":1655768, "Operation Amazon":486602,
 "River Raiders":258715, "Snowy Mountain Stroll":81369, "A Grand Canyon Adventure":2126818,
 "Disruption":146413, "Foggy Forest":1248844, "Indian Strike Force":297564,
 "Switch Escape":109358, "Canyon Offensive":465823, "Escape from the Zombie Cellar":28048,
 "Jungle Fever":76157, "RPG Template":540778, "The Mystery of Z Island":320624,
 "Trapped":12768,
}

def num(x):
    try: return float(re.sub(r"[^0-9.]", "", x) or 0)
    except: return 0.0

# SOAK FORMAT. demo_soak_sweep.sh writes key=value fields and puts the TEST GAME outcome in
# field 2 (GAME / GAME_RECHECK / DIED_IN_PREP / FAIL_*), where the fresh-launch sweep writes a
# bare "OK". Scoring both here keeps ONE curated POLYS reference; the soak analyser used to
# carry a second one, and it still holds the stale pre-far-tree Aztec Teaser figure that 3.53c
# corrected in the dict below.
# ⚠ A soak run's POLYS is NOT always comparable to a fresh-launch run's: the tree pool is
# process-wide, so a vegetation-heavy demo loaded late in a session can legitimately be granted
# fewer slots than the same demo loaded cold. C2 is reported for a soak run but treated as
# advisory - the verdict line says which mode it ran in.
def parse_soak(f):
    kv = {}
    for part in f[2:]:
        if "=" in part:
            k, v = part.split("=", 1)
            kv[k.strip()] = v.strip()
    gst = f[1]
    if gst.startswith("FAIL") or gst.startswith("DIED") or gst.startswith("NO_"):
        return None, gst
    ed = num(kv.get("edFPS", "0"))
    gm = num(kv.get("gmFPS", "0"))
    return dict(demo=f[0], ed=ed, gstate=gst, gm=gm,
                vram=num(kv.get("edVRAM", "0")), gvram=num(kv.get("gmVRAM", "0")),
                polys=int(num(kv.get("polys", "0")))), gst

rows, fails = [], []
soak_mode = False
for line in open(res, encoding="utf-8", errors="replace"):
    line = line.strip()
    if not line or "|" not in line: continue
    f = line.split("|")
    demo = f[0]
    if len(f) > 2 and f[2].startswith("load="):
        soak_mode = True
        r, gst = parse_soak(f)
        if r is None:
            fails.append((demo, gst)); continue
        rows.append(r); continue
    if len(f) < 3 or f[1] != "OK":
        fails.append((demo, f[1] if len(f) > 1 else "MALFORMED")); continue
    ed = [num(f[2]), num(f[3]), num(f[4])]
    gstate = f[5]
    gm = [num(f[6]), num(f[7]), num(f[8])]
    vram = num(f[10]) if len(f) > 10 else 0.0
    polys = int(num(f[11])) if len(f) > 11 else 0
    # gvram = test-game VRAM. It runs HIGHER than the editor's (Aztec Game Kit 2026-08-10:
    # editor 3811.4, game 3962.5), so a gate that only checks the editor column can pass a
    # build that actually breaches 4 GB in the mode players ship in. Check both.
    gvram = num(f[12].split("=")[-1]) if len(f) > 12 and "gvram" in f[12] else 0.0
    # 2026-09-20: when the sweep recorded the observed range, use it. Three settled samples
    # disagree on Foggy Forest (spread 7728) and Z Island (528), so comparing the MAX for
    # exact equality decides those two by luck. min == max on the other 17, so this is not
    # a loosening for them.
    prange = None
    for part in f:
        if part.startswith("polysrange="):
            try:
                lo, hi = part.split("=")[1].split("-")
                prange = (int(lo), int(hi))
            except Exception:
                prange = None
    rows.append(dict(demo=demo, ed=sum(ed)/3 if any(ed) else 0, gstate=gstate,
                     gm=sum(gm)/3 if any(gm) else 0, vram=vram, gvram=gvram, polys=polys,
                     prange=prange))

# A gate that parsed nothing must not report PASS on anything. C2 and C3 are computed over
# `rows`, so with rows empty they printed "identical on all 0 demos" and "worst = 0.0 MB" -
# both PASS - and only C1 caught it. Refuse outright instead, and name the likely cause: the
# single-session soak sweep writes demo|GAME|load=.. where this gate expects demo|OK|..
if not rows:
    print("PARSED NO ROWS from %s" % res)
    if fails and any(st in ("GAME", "GAME_RECHECK", "NOGAME") for _, st in fails):
        print("  The rows look like demo_soak_sweep.sh output (field 2 is a GAME state, not OK).")
        print("  Score that with tools/soak_analyse.py - this gate reads demo_fps_sweep.sh runs.")
    else:
        print("  %d lines were rejected as malformed." % len(fails))
    sys.exit(2)

print("=" * 100)
print("%-32s %8s %8s %9s %9s %12s  %s" % ("demo","edFPS","gmFPS","edVRAM","gmVRAM","POLYS","gate"))
print("=" * 100)
c2 = c3 = c4 = True
worst_vram = (0, "")
for r in sorted(rows, key=lambda x: -x["vram"]):
    d = r["demo"]; ref = REF.get(d)
    if ref is None:            g2 = "POLYS_NO_REFERENCE"; c2 = False
    elif r["polys"] == ref:    g2 = "POLYS_OK"
    elif r.get("prange") and r["prange"][0] <= ref <= r["prange"][1]:
        g2 = "POLYS_IN_RANGE(%d-%d)" % r["prange"]
    else:                      g2 = "POLYS_MISMATCH(ref %d)" % ref; c2 = False
    over = [n for n, v in (("editor", r["vram"]), ("game", r["gvram"])) if v >= limit]
    g3 = "VRAM_OVER(%s)" % ",".join(over) if over else ("VRAM_OK" if r["vram"] else "vram?")
    if over: c3 = False
    g4 = "" if r["gstate"].upper().startswith(("GAME","OK","PLAY")) else " GAME=%s" % r["gstate"]
    if r["gm"] <= 0: c4 = False; g4 += " NO_GAME_FPS"
    peak = max(r["vram"], r["gvram"])
    if peak > worst_vram[0]: worst_vram = (peak, d + ("/game" if r["gvram"] >= r["vram"] else "/editor"))
    print("%-32s %8.1f %8.1f %9.1f %9.1f %12d  %s %s%s" %
          (d, r["ed"], r["gm"], r["vram"], r["gvram"], r["polys"], g2, g3, g4))

print("=" * 100)
c1 = (len(rows) == 19 and not fails)
if fails:
    print("C1 LOAD      FAIL -> " + ", ".join("%s(%s)" % f for f in fails))
elif len(rows) != 19:
    print("C1 LOAD      FAIL -> only %d/19 rows present" % len(rows))
else:
    print("C1 LOAD      PASS  19/19 reached the editor")
print("C2 GEOMETRY  %s  POLYS vs the 0825 (3.19) reference across %d demos"
      % ("ADVIS" if soak_mode else ("PASS " if c2 else "FAIL "), len(rows)))
print("C3 VRAM      %s  worst of editor+game = %.1f MB (%s), limit %.0f, headroom %.1f MB"
      % ("PASS " if c3 else "FAIL ", worst_vram[0], worst_vram[1], limit, limit - worst_vram[0]))
if soak_mode:
    bad = [r["demo"] for r in rows if r["gstate"] not in ("GAME", "GAME_RECHECK")]
    c4 = not bad
    if bad: print("C4 GAME      FAIL -> " + ", ".join(bad))
print("C4 GAME      %s  every demo produced in-game FPS past the loading overlays"
      % ("PASS " if c4 else "FAIL "))
print()
if soak_mode:
    print()
    print("MODE: single-session soak. C2 is ADVISORY here - the tree pool is process-wide, so a")
    print("      vegetation-heavy demo loaded late can be granted fewer slots than the same demo")
    print("      loaded cold. Compare a soak run against the previous SOAK run, not a fresh one.")
    c2 = True
print("VERDICT: %s" % ("CLEAN - passes the release gate" if (c1 and c2 and c3 and c4)
                       else "NOT CLEAN - see failing criteria above"))
print()
print("(FPS columns are for the record only. The rig's GPU state moved between 08-09 and 08-10 -")
print(" Switch Escape read 201 then 142 at a matched CPU frame - so cross-day FPS is not evidence.)")
PY
