# content-additions

Content files this repo does not otherwise track.

**Why this folder exists.** Neither the DX11 nor the DX12 repo tracks `Files/` at all — no
`_markers/*.fpe`, no `editors/uiv3/*.png`. Content lives in the product install, which is the
project's convention and not an accident. But that leaves any NEW content file existing only in
one machine's build area, invisible to git, lost on a fresh install or a different box.

That is precisely the failure mode found on 2026-09-17: `bit32` support existed only as an
untracked hand edit in the deploy tree, so nothing in the tracked build reproduced it. These files
are kept here so the same thing cannot happen to them.

**They must be copied into the product install / packaging. Mirror this layout:**

```
Files/entitybank/_markers/NewParticles.fpe
Files/editors/uiv3/entity_new_particle.png
Files/editors/uiv3/entity_new_particle2.png
```

## NewParticles.fpe  —  "Add WPE Zone"

Completes DX11 `76731c3c`. The code was ported in the 3.38-3.47 parity work; these three content
files were absent, so the 16th Game Elements button (Advanced mode only) drew a blank icon and
placing it referenced a non-existent `.fpe`.

Modelled on `Trigger Zone.fpe` rather than `Particles.fpe`, because entity index 15's tooltip is
**"Add WPE Zone"** and `scriptbank/particles/wpe_zone.lua` ("Wicked Particle Emmitter Zone v4") is a
volume behaviour — index 9 is the separate, existing gpup `Particles.fpe`.

`stylecolor = 5` was chosen deliberately: values 1 and 2 have special-case editor handling
(`M-GridEditB_part0.cpp`, `_part12.cpp`, `_part13.cpp`), 5 has none and is what Ambience Zone uses,
which is the closest existing analogue.

## The two icons

⚠ **Derived art, not original** — assembled from GameGuru's own UI set with PIL, not drawn:
the `entity_triggerzone` frame (zone visual language: square with corner nodes) composited with the
burst glyph lifted out of `entity_particle`. Both themes produced, matching the pair convention
(`X.png` for `pref.current_style` 25 or 3, `X2.png` otherwise), 128x128 to match the other
`entity_*` icons.

They read correctly and are consistent with the set, but they are a stand-in. Replace them if you
want proper artwork.
