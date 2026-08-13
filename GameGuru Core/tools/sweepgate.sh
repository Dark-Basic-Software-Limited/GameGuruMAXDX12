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

# POLYS reference: RE-BASELINED 2026-08-13 to the 2.32 sweep (results_0813.txt), which is the
# first sweep where these numbers are known CORRECT. The previous 0809 reference was stale for
# 16 of 19 demos, and worse, it was stale in a way no amount of re-running could reveal:
# GGMAX 2.32 fixed an fp16 distance overflow that had been silently skipping draws, so POLYS
# had been bit-stable at the WRONG value across the 2.25/2.27/2.28 sweeps.
# ★★ AN IDENTITY GATE PROVES "NO CHANGE", NEVER "CORRECT". If a change is EXPECTED to alter
# what gets drawn, amend C2 IN WRITING BEFORE the run (see NIGHT_INVESTIGATIONS_2026-08-12.md
# for the C2" wording used for 2.32: an increase is allowed if attributable, a decrease fails).
REF = {
 "Aztec Game Kit Teaser":10330135, "Aztec Game Kit":3438876, "Bounty":469906,
 "Horseshoe Bend":2168281, "Island Showdown":4125704, "Operation Amazon":5504271,
 "River Raiders":2362345, "Snowy Mountain Stroll":81369, "A Grand Canyon Adventure":2279506,
 "Disruption":4677579, "Foggy Forest":10220589, "Indian Strike Force":3229699,
 "Switch Escape":109358, "Canyon Offensive":8838008, "Escape from the Zombie Cellar":28048,
 "Jungle Fever":76157, "RPG Template":3247629, "The Mystery of Z Island":722872,
 "Trapped":12768,
}

rows, fails = [], []
for line in open(res, encoding="utf-8", errors="replace"):
    line = line.strip()
    if not line or "|" not in line: continue
    f = line.split("|")
    demo = f[0]
    if len(f) < 3 or f[1] != "OK":
        fails.append((demo, f[1] if len(f) > 1 else "MALFORMED")); continue
    def num(x):
        try: return float(re.sub(r"[^0-9.]", "", x) or 0)
        except: return 0.0
    ed = [num(f[2]), num(f[3]), num(f[4])]
    gstate = f[5]
    gm = [num(f[6]), num(f[7]), num(f[8])]
    vram = num(f[10]) if len(f) > 10 else 0.0
    polys = int(num(f[11])) if len(f) > 11 else 0
    # gvram = test-game VRAM. It runs HIGHER than the editor's (Aztec Game Kit 2026-08-10:
    # editor 3811.4, game 3962.5), so a gate that only checks the editor column can pass a
    # build that actually breaches 4 GB in the mode players ship in. Check both.
    gvram = num(f[12].split("=")[-1]) if len(f) > 12 and "gvram" in f[12] else 0.0
    rows.append(dict(demo=demo, ed=sum(ed)/3 if any(ed) else 0, gstate=gstate,
                     gm=sum(gm)/3 if any(gm) else 0, vram=vram, gvram=gvram, polys=polys))

print("=" * 100)
print("%-32s %8s %8s %9s %9s %12s  %s" % ("demo","edFPS","gmFPS","edVRAM","gmVRAM","POLYS","gate"))
print("=" * 100)
c2 = c3 = c4 = True
worst_vram = (0, "")
for r in sorted(rows, key=lambda x: -x["vram"]):
    d = r["demo"]; ref = REF.get(d)
    if ref is None:            g2 = "polys?"
    elif r["polys"] == ref:    g2 = "POLYS_OK"
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
print("C2 GEOMETRY  %s  POLYS identical to the 0813 (2.32) reference on all %d demos"
      % ("PASS " if c2 else "FAIL ", len(rows)))
print("C3 VRAM      %s  worst of editor+game = %.1f MB (%s), limit %.0f, headroom %.1f MB"
      % ("PASS " if c3 else "FAIL ", worst_vram[0], worst_vram[1], limit, limit - worst_vram[0]))
print("C4 GAME      %s  every demo produced in-game FPS past the loading overlays"
      % ("PASS " if c4 else "FAIL "))
print()
print("VERDICT: %s" % ("CLEAN - passes the release gate" if (c1 and c2 and c3 and c4)
                       else "NOT CLEAN - see failing criteria above"))
print()
print("(FPS columns are for the record only. The rig's GPU state moved between 08-09 and 08-10 -")
print(" Switch Escape read 201 then 142 at a matched CPU frame - so cross-day FPS is not evidence.)")
PY
