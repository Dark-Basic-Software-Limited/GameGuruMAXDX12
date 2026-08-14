Explosion refraction artifact, testpro2, matched captures.

keep/       2.43 BEFORE the fix - hard-edged rectangles, table+candle duplicated,
            wallpaper smeared into stripes (game 6a86fbb6).
after_2.44/ AFTER the distortion-slot fix - geometry intact, distortion is
            continuous and texture-driven, scene clean again by frame 080.
latest/     scratch output of the most recent explosionburst.sh run (wiped each run).

Repro: ./explosionburst.sh testpro2 120

after_2.45/ AFTER the rotation-unit fixes as well. Effect renders and decays clean
            (turbulent ~f006 -> moderate ~f020 -> clean by ~f040), no artifacts.
            NOTE: burst runs are NOT time-aligned to the blast, so frame N of one
            run is a different phase from frame N of another. Do not read a
            cross-run frame-N diff as a change in the effect.

after_2.46/ AFTER the engine-side velocity-aligned rotation restore as well.
            Renders and decays clean (clear by ~f060). Same alignment caveat.

after_2.47/ AFTER the positive-only distortion clamp. Measure runs with
            burstdisturb.py, NOT by eyeballing frame N across runs.
