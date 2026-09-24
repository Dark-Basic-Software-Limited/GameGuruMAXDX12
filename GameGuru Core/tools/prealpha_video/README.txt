Pre-alpha teaser video "Bugs Guaranteed" (2026-09-24) - GameGuruMAX_PreAlpha_BugsGuaranteed.mp4

Made from Lee's six Switch Escape screenshots (Downloads\A-..F-*.jpg; the file names are the stage
directions). Voices are the built-in Windows SAPI voices: Zira = Futuristic Girl, David = Futuristic Guy.

To rebuild (e.g. after changing a line):
  1. copy the six shots here as A.jpg .. F.jpg
  2. edit the lines in tts.ps1 (audio) and CAPTIONS in build_video.py (on-screen text) - keep them in step
  3. powershell -ExecutionPolicy Bypass -File tts.ps1        -> wav\line*.wav
  4. python build_video.py                                    -> GameGuruMAX_PreAlpha_BugsGuaranteed.mp4

Uses the ffmpeg.exe in the build area (path at the top of build_video.py). Output 1920x1080 H.264/AAC.
Nothing here ships - it is a tool, like the sweep scripts.
