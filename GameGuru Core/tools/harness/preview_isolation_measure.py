# GGMAX 3.121: measures preview_isolation_probe.sh's shots. usage: <outdir> <tag> x0 y0 x1 y1 shots
import os, sys
import numpy as np
from PIL import Image

out, tag = sys.argv[1], sys.argv[2]
x0, y0, x1, y1, n = (int(v) for v in sys.argv[3:8])
# 4 px in from the rect, clear of ImGui's border and the favourite heart's row
x0, y0, x1, y1 = x0 + 4, y0 + 4, x1 - 4, y1 - 24

def state(name):
    px = []
    for i in range(1, n + 1):
        v = np.asarray(Image.open(os.path.join(out, '%s_%s_%d.png' % (tag, name, i))).convert('RGB')).astype(float)
        px.append(v[y0:y1, x0:x1].reshape(-1, 3))
    px = np.concatenate(px)
    return np.median(px, axis=0), px.mean(axis=0)

res = {k: state(k) for k in ('base', 'perturb', 'leak', 'restored')}
for k, (med, mean) in res.items():
    print('%-9s median %-16s mean %s' % (k, med.astype(int).tolist(), mean.round(1).tolist()))
bm, ba = res['base']
pm, pa = res['perturb']
lm, la = res['leak']
rm, ra = res['restored']
d_p = max(np.abs(pm - bm).max(), np.abs(pa - ba).max())
d_l = max(np.abs(lm - bm).max(), np.abs(la - ba).max())
d_r = max(np.abs(rm - bm).max(), np.abs(ra - ba).max())
print('perturbed vs base: max %.1f   studio-off leak vs base: max %.1f   restored vs base: max %.1f' % (d_p, d_l, d_r))
ok = d_p <= 2.0 and d_l > 10.0
print('%s: %s' % ('PASS' if ok else 'FAIL',
      'level settings do not reach the preview' if ok else
      ('the instrument saw no leak with the studio OFF - it cannot judge' if d_l <= 10.0 else 'the preview moved with the level')))
