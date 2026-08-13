Explosion refraction artifact, testpro2, matched captures.

keep/       2.43 BEFORE the fix - hard-edged rectangles, table+candle duplicated,
            wallpaper smeared into stripes (game 6a86fbb6).
after_2.44/ AFTER the distortion-slot fix - geometry intact, distortion is
            continuous and texture-driven, scene clean again by frame 080.
latest/     scratch output of the most recent explosionburst.sh run (wiped each run).

Repro: ./explosionburst.sh testpro2 120
