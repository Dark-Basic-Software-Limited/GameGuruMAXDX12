# Render every dialogue line to its own WAV (44.1 kHz, 16-bit, mono) with the Windows SAPI voices.
Add-Type -AssemblyName System.Speech
$dir = Split-Path -Parent $MyInvocation.MyCommand.Path
New-Item -ItemType Directory -Force (Join-Path $dir "wav") | Out-Null
$lines = @(
  @{ id = "01"; voice = "Microsoft David Desktop"; rate = 0;  text = "I've been testing the new pre-alpha all week. I can't find a single bug." },
  @{ id = "02"; voice = "Microsoft Zira Desktop";  rate = 0;  text = "Of course you can't. It's impossible. Lee did all the work." },
  @{ id = "03"; voice = "Microsoft David Desktop"; rate = -2; text = "Think about it. Lee did the work. So the bugs aren't just possible... they're guaranteed." },
  @{ id = "04"; voice = "Microsoft Zira Desktop";  rate = -1; text = "No... surely this time is different?" },
  @{ id = "05"; voice = "Microsoft David Desktop"; rate = -1; text = "It never is." },
  @{ id = "06"; voice = "Microsoft Zira Desktop";  rate = -1; text = "Bugs are guaranteed." },
  @{ id = "07"; voice = "Microsoft David Desktop"; rate = -2; text = "As is our right." },
  @{ id = "08"; voice = "Microsoft Zira Desktop";  rate = -2; text = "As is our right." },
  @{ id = "10"; voice = "Microsoft Zira Desktop";  rate = 0;  text = "Happy hunting, testers." }
)
$fmt = New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo(44100, [System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen, [System.Speech.AudioFormat.AudioChannel]::Mono)
foreach ($l in $lines) {
  $s = New-Object System.Speech.Synthesis.SpeechSynthesizer
  $s.SelectVoice($l.voice)
  $s.Rate = $l.rate
  $out = Join-Path $dir ("wav\line" + $l.id + ".wav")
  $s.SetOutputToWaveFile($out, $fmt)
  $s.Speak($l.text)
  $s.Dispose()
  Write-Output ("line" + $l.id + " -> " + (Get-Item $out).Length + " bytes")
}
