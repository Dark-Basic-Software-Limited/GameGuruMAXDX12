#!/usr/bin/env python3
"""Measure how much a BURST_FRAMES capture disturbs the scene, WITHOUT needing the
runs to be time-aligned to the event.

WHY THIS EXISTS
  Comparing frame N of run A against frame N of run B is not a control. The delay
  between FIRE_WEAPON and the burst arming varies run to run, so the same frame
  index is a different phase of the blast. Two builds can look wildly different
  for no reason but timing. (Cost me a false read before this existed - see
  WETEST.md's BURST_FRAMES row, trap 3.)

WHAT IT DOES INSTEAD
  The camera is fixed and the scene static, so a CLEAN frame from the SAME run is
  a valid per-run reference. Mean |pixel diff| against it is that frame's
  disturbance. The summary stats - value at capture start, peak, and mean over the
  run - are then comparable across runs because none of them depends on where in
  the burst the event landed.

  ⚠ Still not a controlled A/B. If every run's curve DECAYS from frame 0 (printed
  below), the true peak happened before the window opened, so these are
  "disturbance at capture start", which is timing-sensitive. Compare only across
  runs made with the same script, prefer several runs per build, and treat a
  difference smaller than the spread between builds as noise.

USAGE
  python burstdisturb.py <dir-of-frames> [<dir> ...]
  python burstdisturb.py burstshots burstshots/latest
"""
import sys
import glob
import os

import numpy as np
from PIL import Image


def load(path):
    a = np.asarray(Image.open(path).convert("RGB"), dtype=np.int16)
    a[:60, 1100:] = 0    # FPS / mem / VRAM readout changes every frame
    a[760:, :420] = 0    # ammo HUD
    return a


def measure(label, files):
    files = sorted(files)
    if len(files) < 2:
        print(f"{label:<34} (need at least 2 frames)")
        return
    ref = load(files[-1])          # last frame of the run = the settled scene
    v = [float(np.abs(load(f) - ref).mean()) for f in files]
    rises = max(v) > v[0] * 1.15
    print(f"{label:<34} n={len(v):>3}  start={v[0]:6.2f}  peak={max(v):6.2f}  mean={sum(v)/len(v):6.2f}"
          f"   {'peak captured' if rises else 'DECAYS from frame 0 - true peak preceded the window'}")
    print("     " + " ".join(f"{x:5.1f}" for x in v[:16]))


if __name__ == "__main__":
    args = sys.argv[1:] or ["burstshots", "burstshots/latest"]
    print("Disturbance vs a clean frame of the SAME run (0 = identical to settled scene):\n")
    for d in args:
        measure(os.path.basename(d.rstrip("/\\")) or d, glob.glob(os.path.join(d, "frame_*.png")))
