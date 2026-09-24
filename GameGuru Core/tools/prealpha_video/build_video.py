#!/usr/bin/env python3
"""'Bugs Guaranteed' - a 35-second pre-alpha teaser from Lee's six Switch Escape screenshots.

Per shot: trimmed TTS lines laid out with gaps -> one WAV; the screenshot cropped to the 3D viewport,
upscaled, slow push-in (zoompan), captions coloured by speaker, short fades. Then an end card, and
everything joined with the concat demuxer.
"""
import os, shutil, subprocess, wave, array, math

V = os.path.dirname(os.path.abspath(__file__))
FF = r"D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\ffmpeg.exe"
SR = 44100
FPS = 30
GIRL = "0xFF7AD9"   # her chest lights
GUY = "0x5EF2E0"    # his visor / chest lights

# shot file, crop (x, y, w, h) inside the 3D viewport, timeline of lines
SHOTS = [   # Lee's own order and stage directions, from the Downloads file names
    ("A.jpg", (268, 168, 1344, 756), [("TITLE", None)]),                        # conversation started
    ("B.jpg", (268, 168, 1344, 756), [("01", "guy")]),                          # man speaks first
    ("C.jpg", (268, 168, 1344, 756), [("02", "girl")]),                         # female replies
    ("D.jpg", (268, 168, 1344, 756), [("03", "guy"), ("STING", None)]),         # man says something profound
    ("E.jpg", (268, 168, 1344, 756), [("04", "girl")]),                         # female wants to believe
    ("F.jpg", (266, 136, 1344, 756), [("05", "guy"), ("06", "girl"), ("BOTH", "both")]),  # both agree: universal truth
]
CAPTIONS = {
    "01": "I've been testing the new pre-alpha all week. I can't find a single bug.",
    "02": "Of course you can't. It's impossible. Lee did all the work.",
    "03": "Think about it. Lee did the work. So the bugs aren't just possible... they're guaranteed.",
    "04": "No... surely this time is different?",
    "05": "It never is.",
    "06": "Bugs are guaranteed.",
    "BOTH": "As is our right.",
}
LEAD, GAP, TAIL = 0.45, 0.40, 0.75

def run(args):
    r = subprocess.run(args, cwd=V, capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stderr[-3000:]); raise SystemExit("ffmpeg failed")

def read_wav(p):
    with wave.open(p, "rb") as w:
        assert w.getframerate() == SR and w.getsampwidth() == 2 and w.getnchannels() == 1, p
        return array.array("h", w.readframes(w.getnframes()))

def trim(a, thr=300):
    i = 0
    while i < len(a) and abs(a[i]) < thr: i += 1
    j = len(a)
    while j > i and abs(a[j - 1]) < thr: j -= 1
    pad = int(0.03 * SR)
    return a[max(0, i - pad):min(len(a), j + pad)]

def silence(sec):
    return array.array("h", [0] * int(sec * SR))

def sting():
    """dun... dun... DUNNN - three brassy notes, the last one long and falling away."""
    notes = [(196.0, 0.32), (185.0, 0.32), (146.8, 1.35)]
    out = array.array("h")
    for f, d in notes:
        n = int(d * SR)
        for k in range(n):
            t = k / SR
            env = min(1.0, t * 40.0) * math.exp(-t * (1.2 if d > 1 else 3.0))
            v = (math.sin(2*math.pi*f*t) + 0.55*math.sin(4*math.pi*f*t) + 0.35*math.sin(6*math.pi*f*t)
                 + 0.2*math.sin(8*math.pi*f*t))
            out.append(int(max(-1, min(1, 0.28 * env * v)) * 32767))
        out.extend(silence(0.06))
    return out

def write_wav(p, a):
    with wave.open(p, "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR); w.writeframes(a.tobytes())

def esc_font():
    shutil.copy(r"C:\Windows\Fonts\arialbd.ttf", os.path.join(V, "arialbd.ttf"))
    shutil.copy(r"C:\Windows\Fonts\impact.ttf", os.path.join(V, "impact.ttf"))

esc_font()
clips = []
for si, (img, crop, timeline) in enumerate(SHOTS):
    audio = silence(LEAD); t = LEAD; caps = []
    for k, (lid, who) in enumerate(timeline):
        if lid == "STING": a = sting()
        elif lid == "TITLE": a = silence(2.4)
        elif lid == "BOTH":
            d_, z_ = trim(read_wav(os.path.join(V, "wav", "line07.wav"))), trim(read_wav(os.path.join(V, "wav", "line08.wav")))
            n_ = max(len(d_), len(z_)); a = array.array("h", [0] * n_)
            for i_ in range(n_):
                v_ = (d_[i_] if i_ < len(d_) else 0) * 0.62 + (z_[i_] if i_ < len(z_) else 0) * 0.62
                a[i_] = int(max(-32767, min(32767, v_)))
        else: a = trim(read_wav(os.path.join(V, "wav", "line%s.wav" % lid)))
        start = t; dur = len(a) / SR
        if who:
            caps.append((lid, who, start, start + dur + 0.25))
        audio.extend(a); t += dur
        if k < len(timeline) - 1:
            audio.extend(silence(GAP)); t += GAP
    audio.extend(silence(TAIL)); t += TAIL
    wav = "shot%d.wav" % si; write_wav(os.path.join(V, wav), audio)
    dur = t
    x, y, w, h = crop
    frames = int(round(dur * FPS))
    vf = [
        "crop=%d:%d:%d:%d" % (w, h, x, y),
        "scale=3840:2160:flags=lanczos",
        # slow push-in to 7%, centred; zoompan at 4K then down to 1080p kills its integer jitter
        "zoompan=z='1+0.07*on/%d':x='iw/2-(iw/zoom/2)':y='ih/2-(ih/zoom/2)':d=1:s=3840x2160:fps=%d" % (frames, FPS),
        "scale=1920:1080:flags=lanczos",
    ]
    for lid, who, a0, a1 in caps:
        col = GIRL if who == "girl" else (GUY if who == "guy" else "white")
        txt = CAPTIONS[lid]
        parts = [txt]
        if len(txt) > 48:
            # two centred lines, broken at the sentence end (else the space) nearest the middle
            cands = [i + 1 for i, ch in enumerate(txt) if ch in ".?!" and 0 < i < len(txt) - 2 and txt[i + 1] == " "]
            if not cands: cands = [i for i, ch in enumerate(txt) if ch == " "]
            cut = min(cands, key=lambda i: abs(i - len(txt) / 2))
            parts = [txt[:cut].strip(), txt[cut:].strip()]
        for pi, part in enumerate(parts):
            tf = "cap%s_%d.txt" % (lid, pi)
            open(os.path.join(V, tf), "w", encoding="utf-8").write(part)
            yy = "h-150" if len(parts) == 1 else ("h-205" if pi == 0 else "h-140")
            vf.append("drawtext=fontfile=arialbd.ttf:textfile=%s:fontsize=56:fontcolor=%s:borderw=4:bordercolor=black@0.85"
                      ":x=(w-tw)/2:y=%s:enable='between(t,%.2f,%.2f)'" % (tf, col, yy, a0, a1))
    if any(l == "TITLE" for l, _ in timeline):
        open(os.path.join(V, "title.txt"), "w").write("PRE-ALPHA TEST DAY")
        vf.append("drawtext=fontfile=impact.ttf:textfile=title.txt:fontsize=120:fontcolor=white:borderw=6:bordercolor=black@0.7"
                  ":x=(w-tw)/2:y=110:alpha='min(1,max(0,(t-0.3)/0.5))'")
    vf.append("fade=t=in:st=0:d=0.35,fade=t=out:st=%.2f:d=0.22" % (dur - 0.22))
    out = "clip%d.mp4" % si
    run([FF, "-y", "-hide_banner", "-loglevel", "error", "-loop", "1", "-framerate", str(FPS), "-t", "%.3f" % dur,
         "-i", img, "-i", wav, "-vf", ",".join(vf), "-map", "0:v", "-map", "1:a",
         "-c:v", "libx264", "-preset", "slow", "-crf", "18", "-pix_fmt", "yuv420p", "-r", str(FPS),
         "-c:a", "aac", "-b:a", "192k", "-ar", str(SR), "-ac", "2", "-shortest", out])
    clips.append(out)
    print("%s  %.2fs  %d caption(s)" % (out, dur, len(caps)))

# ---------------------------------------------------------------- end card
a = silence(0.5); l10 = trim(read_wav(os.path.join(V, "wav", "line10.wav"))); a.extend(l10); a.extend(silence(1.8))
write_wav(os.path.join(V, "end.wav"), a); dur = len(a) / SR
open(os.path.join(V, "end1.txt"), "w").write("GAMEGURU MAX  DX12  PRE-ALPHA")
open(os.path.join(V, "end2.txt"), "w").write("BUGS GUARANTEED.")
open(os.path.join(V, "end3.txt"), "w").write("Happy hunting, testers - and thank you.")
vf = ",".join([
    "drawtext=fontfile=arialbd.ttf:textfile=end1.txt:fontsize=64:fontcolor=white:x=(w-tw)/2:y=330:alpha='min(1,t/0.5)'",
    "drawtext=fontfile=impact.ttf:textfile=end2.txt:fontsize=150:fontcolor=%s:borderw=6:bordercolor=%s@0.35:x=(w-tw)/2:y=460:enable='gte(t,0.5)'" % (GIRL, GUY),
    "drawtext=fontfile=arialbd.ttf:textfile=end3.txt:fontsize=46:fontcolor=%s:x=(w-tw)/2:y=680:enable='gte(t,1.0)'" % GUY,
    "fade=t=in:st=0:d=0.3,fade=t=out:st=%.2f:d=0.6" % (dur - 0.6),
])
run([FF, "-y", "-hide_banner", "-loglevel", "error", "-f", "lavfi", "-i", "color=c=0x0B1220:s=1920x1080:r=%d:d=%.3f" % (FPS, dur),
     "-i", "end.wav", "-vf", vf, "-map", "0:v", "-map", "1:a", "-c:v", "libx264", "-preset", "slow", "-crf", "18",
     "-pix_fmt", "yuv420p", "-c:a", "aac", "-b:a", "192k", "-ar", str(SR), "-ac", "2", "-shortest", "clip_end.mp4"])
clips.append("clip_end.mp4")
print("clip_end.mp4  %.2fs" % dur)

# ---------------------------------------------------------------- join
open(os.path.join(V, "list.txt"), "w").write("".join("file '%s'\n" % c for c in clips))
run([FF, "-y", "-hide_banner", "-loglevel", "error", "-f", "concat", "-safe", "0", "-i", "list.txt",
     "-c", "copy", "-movflags", "+faststart", "GameGuruMAX_PreAlpha_BugsGuaranteed.mp4"])
print("DONE ->", os.path.join(V, "GameGuruMAX_PreAlpha_BugsGuaranteed.mp4"))
