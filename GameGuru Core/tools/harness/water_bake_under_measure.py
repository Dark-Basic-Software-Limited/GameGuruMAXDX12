# GGMAX 3.120: companion to water_bake_under_probe.sh - median colour per screen band of the 3D panel.
import sys, os
import numpy as np
from PIL import Image
out, tag = sys.argv[1], sys.argv[2]
VX0, VX1, VY0, VY1 = 212, 1285, 105, 777
bands = [('ceiling top', 0.02, 0.15), ('ceiling low', 0.30, 0.40), ('horizon', 0.45, 0.50), ('mid bed', 0.60, 0.70), ('near bed', 0.88, 0.98)]
print('%-10s' % 'shot' + ''.join('%22s' % b[0] for b in bands))
for n in ('live', 'old', 'new', 'ghost'):
    p = os.path.join(out, '%s_%s.png' % (tag, n))
    if not os.path.exists(p): continue
    v = np.asarray(Image.open(p).convert('RGB')).astype(float)[VY0:VY1, VX0:VX1]
    h = v.shape[0]
    cells = []
    for _, y0, y1 in bands:
        m = np.median(v[int(y0 * h):int(y1 * h)].reshape(-1, 3), axis=0).astype(int)
        cells.append('%4d %4d %4d' % tuple(m))
    print('%-10s' % n + ''.join('%22s' % c for c in cells))
